#include "frogcontexto/agent_runtime.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>

int main() {
    const auto workspace =
        std::filesystem::temp_directory_path() / "frogcontexto_simulation_test";
    std::filesystem::remove_all(workspace);

    frogcontexto::ApiConfig config;
    config.model = "simulation-model";
    auto provider = std::make_unique<frogcontexto::SimulationClient>(2);
    frogcontexto::AgentRuntime runtime(workspace, config, std::move(provider));

    const auto first = runtime.send_user_message("Remember the first decision.");
    const auto second = runtime.send_user_message("Continue the same task.");

    assert(first.find("simulation") != std::string::npos);
    assert(second.find("Simulated compaction occurred") != std::string::npos);
    assert(std::filesystem::exists(workspace / ".frogcontexto" / "context.md"));
    assert(std::filesystem::exists(workspace / ".frogcontexto" / "history"));

    std::filesystem::remove_all(workspace);
    std::cout << "test_simulation: PASS\n";
    return 0;
}
