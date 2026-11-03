#include "frogcontexto/context.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace frogcontexto {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalize(std::string value) {
    value = lower(trim(std::move(value)));
    std::string result;
    bool previous_space = false;
    for (const char c : value) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!previous_space) {
                result.push_back(' ');
            }
            previous_space = true;
        } else {
            result.push_back(c);
            previous_space = false;
        }
    }
    return result;
}

std::string timestamp_utc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream output;
    output << std::put_time(&utc, "%Y%m%dT%H%M%SZ");
    return output.str();
}

void append_item(std::vector<std::string>& target, const std::string& line) {
    const std::string value = trim(line.substr(2));
    if (!value.empty()) {
        target.push_back(value);
    }
}

struct SectionRef {
    std::vector<std::string>* items = nullptr;
    std::string name;
};

SectionRef section_for(ContextState& state, const std::string& heading) {
    const std::string normalized = normalize(heading);
    if (normalized == "decisions") return {&state.decisions, "Decisions"};
    if (normalized == "completed") return {&state.completed, "Completed"};
    if (normalized == "pending") return {&state.pending, "Pending"};
    if (normalized == "confirmed facts") return {&state.facts, "Confirmed Facts"};
    if (normalized == "open questions") return {&state.open_questions, "Open Questions"};
    if (normalized == "recent context") return {&state.recent_context, "Recent Context"};
    return {};
}

struct LabeledItem {
    std::string section;
    std::string value;
};

std::vector<LabeledItem> labeled_items(const ContextState& state) {
    std::vector<LabeledItem> items;
    const auto add = [&items](const std::string& section, const std::vector<std::string>& values) {
        for (const auto& value : values) {
            items.push_back({section, value});
        }
    };
    add("Decisions", state.decisions);
    add("Completed", state.completed);
    add("Pending", state.pending);
    add("Confirmed Facts", state.facts);
    add("Open Questions", state.open_questions);
    add("Recent Context", state.recent_context);
    return items;
}

bool contains_normalized(const std::vector<LabeledItem>& items, const LabeledItem& needle) {
    return std::any_of(items.begin(), items.end(), [&needle](const LabeledItem& item) {
        return normalize(item.section) == normalize(needle.section) &&
               normalize(item.value) == normalize(needle.value);
    });
}

void render_list(std::ostringstream& output, const std::vector<std::string>& values) {
    if (values.empty()) {
        output << "- None\n";
        return;
    }
    for (const auto& value : values) {
        output << "- " << value << "\n";
    }
}

void render_result_list(std::ostringstream& output, const char* heading,
                        const std::vector<std::string>& values) {
    output << heading << ":\n";
    if (values.empty()) {
        output << "- None\n\n";
        return;
    }
    for (const auto& value : values) {
        output << "- " << value << "\n";
    }
    output << "\n";
}

}  // namespace

ContextState parse_markdown(const std::string& markdown) {
    ContextState state;
    SectionRef current_section;
    std::istringstream input(markdown);
    std::string line;

    while (std::getline(input, line)) {
        const std::string clean = trim(line);
        if (clean.rfind("# ", 0) == 0) {
            continue;
        }
        if (clean.rfind("## ", 0) == 0) {
            current_section = section_for(state, clean.substr(3));
            continue;
        }
        if (clean.rfind("objective:", 0) == 0) {
            state.objective = trim(clean.substr(std::string("objective:").size()));
            continue;
        }
        if (clean.rfind("status:", 0) == 0) {
            state.status = trim(clean.substr(std::string("status:").size()));
            continue;
        }
        if (clean.rfind("- ", 0) == 0 && current_section.items != nullptr) {
            append_item(*current_section.items, clean);
        }
    }
    return state;
}

std::string render_markdown(const ContextState& state, const std::string& timestamp) {
    std::ostringstream output;
    output << "# FrogContexto Checkpoint\n\n";
    output << "version: 1\n";
    output << "timestamp_utc: " << timestamp << "\n";
    output << "objective: " << state.objective << "\n";
    output << "status: " << state.status << "\n\n";
    output << "## Decisions\n";
    render_list(output, state.decisions);
    output << "\n## Completed\n";
    render_list(output, state.completed);
    output << "\n## Pending\n";
    render_list(output, state.pending);
    output << "\n## Confirmed Facts\n";
    render_list(output, state.facts);
    output << "\n## Open Questions\n";
    render_list(output, state.open_questions);
    output << "\n## Recent Context\n";
    render_list(output, state.recent_context);
    return output.str();
}

Reconciliation reconcile(const ContextState& checkpoint, const ContextState& current) {
    Reconciliation result;
    const auto saved_items = labeled_items(checkpoint);
    const auto current_items = labeled_items(current);

    for (const auto& item : saved_items) {
        if (contains_normalized(current_items, item)) {
            result.confirmed.push_back(item.section + ": " + item.value);
        } else {
            result.missing.push_back(item.section + ": " + item.value);
        }
    }
    for (const auto& item : current_items) {
        if (!contains_normalized(saved_items, item)) {
            result.new_items.push_back(item.section + ": " + item.value);
        }
    }

    if (!checkpoint.objective.empty() && !current.objective.empty() &&
        normalize(checkpoint.objective) != normalize(current.objective)) {
        result.conflicts.push_back("Objective changed from '" + checkpoint.objective +
                                   "' to '" + current.objective + "'");
    }
    if (!checkpoint.status.empty() && !current.status.empty() &&
        normalize(checkpoint.status) != normalize(current.status)) {
        result.conflicts.push_back("Status changed from '" + checkpoint.status +
                                   "' to '" + current.status + "'");
    }

    if (!result.conflicts.empty()) {
        result.next_action = "Resolve conflicts before continuing.";
    } else if (!result.missing.empty()) {
        result.next_action = "Restore missing checkpoint items before continuing.";
    } else {
        result.next_action = "Continue from the reconciled current context.";
    }
    return result;
}

std::string render_reconciliation(const Reconciliation& result) {
    std::ostringstream output;
    output << "# FrogContexto Reconciliation\n\n";
    render_result_list(output, "Confirmed", result.confirmed);
    render_result_list(output, "Missing", result.missing);
    render_result_list(output, "New", result.new_items);
    render_result_list(output, "Conflicts", result.conflicts);
    output << "Next action:\n" << result.next_action << "\n";
    return output.str();
}

CheckpointStore::CheckpointStore(std::filesystem::path workspace)
    : workspace_(std::move(workspace)) {}

std::filesystem::path CheckpointStore::current_path() const {
    return workspace_ / ".frogcontexto" / "context.md";
}

std::filesystem::path CheckpointStore::save(const ContextState& state) {
    const auto directory = workspace_ / ".frogcontexto";
    const auto history = directory / "history";
    std::filesystem::create_directories(history);
    const std::string timestamp = timestamp_utc();
    const std::string contents = render_markdown(state, timestamp);
    const auto history_path = history / ("checkpoint-" + timestamp + ".md");
    write_text_file(history_path, contents);
    write_text_file(current_path(), contents);
    return current_path();
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to read file: " + path.string());
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void write_text_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to write file: " + path.string());
    }
    output << contents;
}

}  // namespace frogcontexto
