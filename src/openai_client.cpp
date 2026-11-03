#include "frogcontexto/openai_client.hpp"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace frogcontexto {
namespace {

std::wstring to_wide_ascii(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

std::string from_hex_escape(const std::string& input, std::size_t& index) {
    if (index + 4 >= input.size()) {
        return "";
    }
    const auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    int value = 0;
    for (std::size_t offset = 1; offset <= 4; ++offset) {
        const int digit = hex(input[index + offset]);
        if (digit < 0) return "";
        value = (value * 16) + digit;
    }
    index += 4;
    if (value < 0x80) {
        return std::string(1, static_cast<char>(value));
    }
    if (value < 0x800) {
        return std::string({
            static_cast<char>(0xC0 | (value >> 6)),
            static_cast<char>(0x80 | (value & 0x3F))});
    }
    return std::string({
        static_cast<char>(0xE0 | (value >> 12)),
        static_cast<char>(0x80 | ((value >> 6) & 0x3F)),
        static_cast<char>(0x80 | (value & 0x3F))});
}

std::string read_json_string(const std::string& input, std::size_t start) {
    std::string result;
    for (std::size_t index = start + 1; index < input.size(); ++index) {
        const char c = input[index];
        if (c == '"') {
            return result;
        }
        if (c != '\\') {
            result.push_back(c);
            continue;
        }
        if (++index >= input.size()) break;
        switch (input[index]) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u':
                --index;
                result += from_hex_escape(input, index);
                break;
            default: result.push_back(input[index]); break;
        }
    }
    return result;
}

std::string http_post(const std::string& api_key, const std::string& body) {
    HINTERNET session = WinHttpOpen(
        L"FrogContexto/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session) {
        throw std::runtime_error("WinHttpOpen failed");
    }

    HINTERNET connection = WinHttpConnect(session, L"api.openai.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) {
        WinHttpCloseHandle(session);
        throw std::runtime_error("WinHttpConnect failed");
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        L"/v1/responses",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("WinHttpOpenRequest failed");
    }

    const std::wstring headers =
        L"Content-Type: application/json\r\nAuthorization: Bearer " +
        to_wide_ascii(api_key);
    const BOOL sent = WinHttpSendRequest(
        request,
        headers.c_str(),
        static_ast<DWORD>(headers.size()),
        const_cast<char>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("OpenAI request failed: " + std::to_string(GetLastError()));
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status,
        &status_size,
        WINHTTP_NO_HEADER_INDEX);

    std::string response;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
        std::string buffer(available, '\0');
        DWORD received = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &received)) {
            break;
        }
        response.append(buffer.data(), received);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "OpenAI API returned HTTP " + std::to_string(status) + ": " + response);
    }
    return response;
}

std::string join_json(const std::vector<std::string>& values) {
    std::ostringstream output;
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) output << ',';
        output << values[index];
    }
    output << ']';
    return output.str();
}

bool has_compaction_type(const std::vector<std::string>& items) {
    return std::any_of(items.begin(), items.end(), [](const std::string& item) {
        return extract_string_field(item, "type") == "compaction";
    });
}

}  // namespace

std::string json_escape(const std::string& value) {
    std::ostringstream output;
    for (const unsigned char c : value) {
        switch (c) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (c < 0x20) {
                    output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(c) << std::setfill(' ') << std::dec;
                } else {
                    output << static_cast<char>(c);
                }
        }
    }
    return output.str();
}

std::string make_message_item(const std::string& role, const std::string& content) {
    return "{\"type\":\"message\",\"role\":\"" + json_escape(role) +
           "\",\"content\":\"" + json_escape(content) + "\"}";
}

std::string extract_string_field(const std::string& object, const std::string& field) {
    const std::string marker = "\"" + field + "\"";
    const std::size_t marker_position = object.find(marker);
    if (marker_position == std::string::npos) return {};
    const std::size_t colon = object.find(':', marker_position + marker.size());
    if (colon == std::string::npos) return {};
    const std::size_t quote = object.find('"', colon + 1);
    if (quote == std::string::npos) return {};
    return read_json_string(object, quote);
}

std::string extract_array_field(const std::string& object, const std::string& field) {
    const std::string marker = "\"" + field + "\"";
    const std::size_t marker_position = object.find(marker);
    if (marker_position == std::string::npos) return {};
    const std::size_t opening = object.find('[', marker_position + marker.size());
    if (opening == std::string::npos) return {};

    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = opening; index < object.size(); ++index) {
        const char c = object[index];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') {
            in_string = true;
        } else if (c == '[') {
            ++depth;
        } else if (c == ']' && --depth == 0) {
            return object.substr(opening, index - opening + 1);
        }
    }
    return {};
}

std::vector<std::string> split_json_array(const std::string& array) {
    std::vector<std::string> result;
    if (array.size() < 2 || array.front() != '[' || array.back() != ']') return result;

    std::size_t start = 1;
    int object_depth = 0;
    int array_depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = 1; index < array.size() - 1; ++index) {
        const char c = array[index];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '{') ++object_depth;
        else if (c == '}') --object_depth;
        else if (c == '[') ++array_depth;
        else if (c == ']') --array_depth;
        else if (c == ',' && object_depth == 0 && array_depth == 0) {
            const std::string item = array.substr(start, index - start);
            if (!item.empty()) result.push_back(item);
            start = index + 1;
        }
    }
    const std::string last = array.substr(start, array.size() - 1 - start);
    if (!last.empty()) result.push_back(last);
    return result;
}

OpenAIClient::OpenAIClient(ApiConfig config) : config_(std::move(config)) {
    if (config_.api_key.empty()) {
        throw std::invalid_argument("OPENAI_API_KEY is not set");
    }
}

ApiResponse OpenAIClient::respond(const std::vector<std::string>& input_items) {
    const std::string body =
        "{\"model\":\"" + json_escape(config_.model) +
        "\",\"input\":" + join_json(input_items) +
        ",\"store\":false,\"context_management\":[{\"type\":\"compaction\",\"compact_threshold\":" +
        std::to_string(config_.compact_threshold) + "}]}";

    ApiResponse result;
    result.raw_json = http_post(config_.api_key, body);
    result.output_text = extract_string_field(result.raw_json, "output_text");
    result.output_items = split_json_array(extract_array_field(result.raw_json, "output"));
    result.compacted = has_compaction_type(result.output_items);
    return result;
}

}  // namespace frogcontexto
