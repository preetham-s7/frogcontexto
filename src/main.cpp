#include "frogcontexto/context.hpp"
#include "frogcontexto/agent_runtime.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void print_usage() {
    std::cout << "FrogContexto MVP\n\n"
              << "Usage:\n"
              << "  frogcontexto checkpoint <workspace> <input.md>\n"
              << "  frogcontexto reconcile <checkpoint.md> <current.md>\n"
              << "  frogcontexto inspect <context.md>\n"
              << "  frogcontexto run <workspace>\n\n"
              << "run requires OPENAI_API_KEY. Optional: FROGCONTEXTO_MODEL and "
                 "FROGCONTEXTO_COMPACT_THRESHOLD.\n";
}

std::size_t environment_size(const char* name, std::size_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    try {
        return std::stoull(value);
    } catch (...) {
        return fallback;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            print_usage();
            return 2;
        }

        const std::string command = argv[1];
        if (command == "checkpoint" && argc == 4) {
            const std::filesystem::path workspace = argv[2];
            const auto state = frogcontexto::parse_markdown(
                frogcontexto::read_text_file(argv[3]));
            frogcontexto::CheckpointStore store(workspace);
            const auto path = store.save(state);
            std::cout << "Checkpoint saved to " << path.string() << "\n";
            return 0;
        }

        if (command == "reconcile" && argc == 4) {
            const auto checkpoint = frogcontexto::parse_markdown(
                frogcontexto::read_text_file(argv[2]));
            const auto current = frogcontexto::parse_markdown(
                frogcontexto::read_text_file(argv[3]));
            std::cout << frogcontexto::render_reconciliation(
                frogcontexto::reconcile(checkpoint, current));
            return 0;
        }

        if (command == "inspect" && argc == 3) {
            const auto state = frogcontexto::parse_markdown(
                frogcontexto::read_text_file(argv[2]));
            std::cout << frogcontexto::render_markdown(state, "inspection") << "\n";
            return 0;
        }

        if (command == "run" && argc == 3) {
            const char* key = std::getenv("OPENAI_API_KEY");
            if (key == nullptr || *key == '\0') {
                throw std::runtime_error("Set OPENAI_API_KEY before using 'run'");
            }
            frogcontexto::ApiConfig config;
            config.api_key = key;
            if (const char* model = std::getenv("FROGCONTEXTO_MODEL");
                model != nullptr && *model != '\0') {
                config.model = model;
            }
            config.compact_threshold = environment_size(
                "FROGCONTEXTO_COMPACT_THRESHOLD", config.compact_threshold);

            frogcontexto::AgentRuntime runtime(argv[2], config);
            std::cout << "FrogContexto agent ready. Type 'exit' to stop.\n";
            std::string message;
            while (std::cout << "> " && std::getline(std::cin, message)) {
                if (message == "exit" || message == "quit") break;
                if (message.empty()) continue;
                std::cout << runtime.send_user_message(message) << "\n";
            }
            return 0;
        }

        print_usage();
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "frogcontexto error: " << error.what() << "\n";
        return 1;
    }
}
