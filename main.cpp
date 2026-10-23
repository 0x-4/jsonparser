
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <map>
#include <stdexcept>

struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;
    using Value = std::variant<std::nullptr_t, bool, double,
                               std::string, Array, Object>;
    Value value;
};

class Parser {
    std::string_view input;
    std::size_t pos = 0;

    [[noreturn]] void error(const char* msg) const {
        throw std::runtime_error(
            std::string(msg) + " at byte " + std::to_string(pos));
    }

    void whitespace() {
        while (pos < input.size() &&
               std::isspace(static_cast<unsigned char>(input[pos])))
            ++pos;
    }

    bool consume(char c) {
        if (pos < input.size() && input[pos] == c) {
            ++pos;
            return true;
        }
        return false;
    }

    void expect(char c) {
        if (!consume(c)) error("Unexpected character");
    }

    std::string string() {
        expect('"');
        std::string out;

        while (pos < input.size()) {
            char c = input[pos++];

            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20)
                error("Control character in string");

            if (c != '\\') {
                out += c;
                continue;
            }

            if (pos >= input.size()) error("Incomplete escape");

            switch (input[pos++]) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: error("Unsupported escape");
            }
        }

        error("Unterminated string");
    }

    Json number() {
        std::size_t start = pos;

        consume('-');

        if (consume('0')) {
            if (pos < input.size() &&
                input[pos] >= '0' && input[pos] <= '9')
                error("Leading zero");
        } else {
            if (pos >= input.size() ||
                input[pos] < '1' || input[pos] > '9')
                error("Invalid number");

            while (pos < input.size() &&
                   input[pos] >= '0' && input[pos] <= '9')
                ++pos;
        }

        if (consume('.')) {
            std::size_t startFraction = pos;
            while (pos < input.size() &&
                   input[pos] >= '0' && input[pos] <= '9')
                ++pos;

            if (pos == startFraction) error("Missing fraction");
        }

        if (consume('e') || consume('E')) {
            if (pos < input.size() &&
                (input[pos] == '+' || input[pos] == '-'))
                ++pos;

            std::size_t startExponent = pos;
            while (pos < input.size() &&
                   input[pos] >= '0' && input[pos] <= '9')
                ++pos;

            if (pos == startExponent) error("Missing exponent");
        }

        std::string token(input.substr(start, pos - start));
        std::size_t used = 0;
        double n;

        try {
            n = std::stod(token, &used);
        } catch (...) {
            error("Number out of range");
        }

        if (used != token.size()) error("Invalid number");
        return Json{n};
    }

    Json array() {
        expect('[');
        whitespace();
        Json::Array items;

        if (consume(']')) return Json{items};

        while (true) {
            whitespace();
            items.push_back(value());
            whitespace();

            if (consume(']')) break;
            expect(',');
            whitespace();
        }

        return Json{items};
    }

    Json object() {
        expect('{');
        whitespace();
        Json::Object items;

        if (consume('}')) return Json{items};

        while (true) {
            whitespace();
            if (pos >= input.size() || input[pos] != '"')
                error("Expected object key");

            std::string key = string();
            whitespace();
            expect(':');
            whitespace();

            if (!items.emplace(key, value()).second)
                error("Duplicate object key");

            whitespace();
            if (consume('}')) break;
            expect(',');
        }

        return Json{items};
    }

    Json value() {
        whitespace();
        if (pos >= input.size()) error("Expected value");

        switch (input[pos]) {
            case '"': return Json{string()};
            case '{': return object();
            case '[': return array();
            case 't':
                if (input.substr(pos, 4) == "true") {
                    pos += 4;
                    return Json{true};
                }
                break;
            case 'f':
                if (input.substr(pos, 5) == "false") {
                    pos += 5;
                    return Json{false};
                }
                break;
            case 'n':
                if (input.substr(pos, 4) == "null") {
                    pos += 4;
                    return Json{nullptr};
                }
                break;
            default:
                if (input[pos] == '-' ||
                    (input[pos] >= '0' && input[pos] <= '9'))
                    return number();
        }

        error("Invalid value");
    }

public:
    explicit Parser(std::string_view text) : input(text) {}

    Json parse() {
        Json result = value();
        whitespace();

        if (pos != input.size()) error("Trailing input");
        return result;
    }
};

int main() {
    std::string input{
        std::istreambuf_iterator<char>(std::cin), {}
    };

    try {
        Json result = Parser(input).parse();
        std::cout << "Valid JSON\n";
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << '\n';
        return 1;
    }
}
