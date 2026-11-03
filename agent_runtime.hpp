#pragma once

#include "frogcontexto/context.hpp"
#include "frogcontexto/openai_client.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace frogcontexto {

class AgentRuntime {
public:
    AgentRuntime(std::filesystem::path workspace, ApiConfig config);
    AgentRuntime(std::filesystem::path workspace,
                 ApiConfig config,
                 std::unique_ptr<ResponseProvider> provider);

    std::string send_user_message(const std::string& message);
    std::filesystem::path checkpoint_now();
    std::size_t estimated_tokens() const;

private:
    ContextState build_checkpoint_state() const;
    void checkpoint_if_near_threshold();
    void append_recovery_message();

    std::filesystem::path workspace_;
    ApiConfig config_;
    std::unique_ptr<ResponseProvider> provider_;
    CheckpointStore store_;
    std::vector<std::string> conversation_items_;
    bool checkpoint_exists_for_current_window_ = false;
};

}  // namespace frogcontexto
