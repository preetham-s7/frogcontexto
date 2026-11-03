#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace frogcontexto {

struct ContextState {
    std::string objective;
    std::string status;
    std::vector<std::string> decisions;
    std::vector<std::string> completed;
    std::vector<std::string> pending;
    std::vector<std::string> facts;
    std::vector<std::string> open_questions;
    std::vector<std::string> recent_context;
};

struct Reconciliation {
    std::vector<std::string> confirmed;
    std::vector<std::string> missing;
    std::vector<std::string> new_items;
    std::vector<std::string> conflicts;
    std::string next_action;
};

ContextState parse_markdown(const std::string& markdown);
std::string render_markdown(const ContextState& state, const std::string& timestamp_utc);
Reconciliation reconcile(const ContextState& checkpoint, const ContextState& current);
std::string render_reconciliation(const Reconciliation& result);

class CheckpointStore {
public:
    explicit CheckpointStore(std::filesystem::path workspace);

    std::filesystem::path current_path() const;
    std::filesystem::path save(const ContextState& state);

private:
    std::filesystem::path workspace_;
};

std::string read_text_file(const std::filesystem::path& path);
void write_text_file(const std::filesystem::path& path, const std::string& contents);

}  // namespace frogcontexto
