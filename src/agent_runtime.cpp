#include "frogcontexto/agent_runtime.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace frogcontexto {
namespace {

constexpr std::size_t kCheckpointTriggerPercent = 80;
constexpr std::size_t kRecentItems = 8;

std::string first_user_message(const std::vector<std::string>& items) {
    for (const auto& item : items) {
        if (extract_string_field(item, "role") == "user") {
            const std::string content = extract_string_field(item, "content");
            if (!content.empty()) return content;
        }
    }
    return "Continue the current conversation";
}

}  // namespace

AgentRuntime::AgentRuntime(std::filesystem::path workspace, ApiConfig config)
    : AgentRuntime(
          workspace,
          config,
          std::make_unique<OpenAIClient>(config)) {}

AgentRuntime::AgentRuntime(std::filesystem::path workspace,
                           ApiConfig config,
                           std::unique_ptr<ResponseProvider> provider)
    : workspace_(std::move(workspace)),
      config_(std::move(config)),
      provider_(std::move(provider)),
      store_(workspace_) {}

std::size_t AgentRuntime::estimated_tokens() const {
    std::size_t characters = 0;
    for (const auto& item : conversation_items_) {
        characters += item.size();
    }
    return characters / 4;
}

ContextState AgentRuntime::build_checkpoint_state() const {
    ContextState state;
    state.objective = first_user_message(conversation_items_);
    state.status = "In progress; checkpoint created before context compaction";
    state.decisions.push_back("Use FrogContexto to preserve state across context windows");
    state.facts.push_back("Automatic checkpoint created by the C++ agent runtime");
    state.facts.push_back("Conversation item count: " + std::to_string(conversation_items_.size()));
    state.pending.push_back("Reconcile this checkpoint with the next compacted context");
    state.pending.push_back("Continue the user's latest task");

    const std::size_t start = conversation_items_.size() > kRecentItems
                                   ? conversation_items_.size() - kRecentItems
                                   : 0;
    for (std::size_t index = start; index < conversation_items_.size(); ++index) {
        state.recent_context.push_back(conversation_items_[index]);
    }
    return state;
}

std::filesystem::path AgentRuntime::checkpoint_now() {
    const auto path = store_.save(build_checkpoint_state());
    checkpoint_exists_for_current_window_ = true;
    return path;
}

void AgentRuntime::checkpoint_if_near_threshold() {
    const std::size_t trigger =
        (config_.compact_threshold * kCheckpointTriggerPercent) / 100;
    if (estimated_tokens() >= trigger && !checkpoint_exists_for_current_window_) {
        checkpoint_now();
    }
}

void AgentRuntime::append_recovery_message() {
    const std::string checkpoint = read_text_file(store_.current_path());
    std::ostringstream recovery;
    recovery << "FrogContexto recovery checkpoint. Context compaction occurred.\n\n"
             << "Use this checkpoint to recover important state. Compare it with the "
                "current conversation, prefer the latest user instructions, and do not "
                "repeat completed work. Report only if a conflict needs clarification.\n\n"
             << checkpoint;
    conversation_items_.push_back(make_message_item("user", recovery.str()));
}

std::string AgentRuntime::send_user_message(const std::string& message) {
    conversation_items_.push_back(make_message_item("user", message));
    checkpoint_if_near_threshold();

    const ApiResponse response = provider_->respond(conversation_items_);
    for (const auto& item : response.output_items) {
        conversation_items_.push_back(item);
    }

    if (response.compacted) {
        if (!checkpoint_exists_for_current_window_) {
            checkpoint_now();
        }

        const auto compaction_item = std::find_if(
            conversation_items_.begin(),
            conversation_items_.end(),
            [](const std::string& item) {
                return extract_string_field(item, "type") == "compaction";
            });
        if (compaction_item != conversation_items_.end()) {
            conversation_items_.erase(conversation_items_.begin(), compaction_item);
        }

        append_recovery_message();
        checkpoint_exists_for_current_window_ = false;
    }
    return response.output_text;
}

}  // namespace frogcontexto
