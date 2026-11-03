#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace frogcontexto {

struct ApiConfig {
    std::string api_key;
    std::string model = "gpt-5.3-codex";
    std::size_t compact_threshold = 200000;
};

struct ApiResponse {
    std::string raw_json;
    std::string output_text;
    std::vector<std::string> output_items;
    bool compacted = false;
};

class ResponseProvider {
public:
    virtual ~ResponseProvider() = default;
    virtual ApiResponse respond(const std::vector<std::string>& input_items) = 0;
};

class OpenAIClient final : public ResponseProvider {
public:
    explicit OpenAIClient(ApiConfig config);

    ApiResponse respond(const std::vector<std::string>& input_items) override;

private:
    ApiConfig config_;
};

class SimulationClient final : public ResponseProvider {
public:
    explicit SimulationClient(std::size_t compaction_every_turns = 3);

    ApiResponse respond(const std::vector<std::string>& input_items) override;

private:
    std::size_t compaction_every_turns_;
    std::size_t turn_count_ = 0;
};

std::string json_escape(const std::string& value);
std::string make_message_item(const std::string& role, const std::string& content);
std::string extract_string_field(const std::string& object, const std::string& field);
std::string extract_array_field(const std::string& object, const std::string& field);
std::vector<std::string> split_json_array(const std::string& array);

}  // namespace frogcontexto
