#include "frogcontexto/openai_client.hpp"

#include <cassert>
#include <iostream>

int main() {
    const std::string message = frogcontexto::make_message_item(
        "user", "line one\nline two with a \"quote\"");
    assert(message.find("\\n") != std::string::npos);
    assert(message.find("\\\"") != std::string::npos);
    assert(frogcontexto::extract_string_field(message, "role") == "user");
    assert(frogcontexto::extract_string_field(message, "content") ==
           "line one\nline two with a \"quote\"");

    const std::string response =
        R"({"output_text":"hello","output":[{"type":"message"},{"type":"compaction"}]})";
    const auto items = frogcontexto::split_json_array(
        frogcontexto::extract_array_field(response, "output"));
    assert(items.size() == 2);
    assert(frogcontexto::extract_string_field(items[1], "type") == "compaction");

    std::cout << "test_json: PASS\n";
    return 0;
}
