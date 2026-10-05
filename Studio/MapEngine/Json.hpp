#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace fantasy::studio::json {

class Value {
public:
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value>;
    using Storage = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, Array, Object>;

    Value() : data_(nullptr) {}
    Value(std::nullptr_t) : data_(nullptr) {}
    Value(bool value) : data_(value) {}
    Value(std::int64_t value) : data_(value) {}
    Value(int value) : data_(static_cast<std::int64_t>(value)) {}
    Value(double value) : data_(value) {}
    Value(std::string value) : data_(std::move(value)) {}
    Value(const char* value) : data_(std::string(value)) {}
    Value(Array value) : data_(std::move(value)) {}
    Value(Object value) : data_(std::move(value)) {}

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(data_); }
    bool isBool() const { return std::holds_alternative<bool>(data_); }
    bool isInteger() const { return std::holds_alternative<std::int64_t>(data_); }
    bool isNumber() const { return isInteger() || std::holds_alternative<double>(data_); }
    bool isString() const { return std::holds_alternative<std::string>(data_); }
    bool isArray() const { return std::holds_alternative<Array>(data_); }
    bool isObject() const { return std::holds_alternative<Object>(data_); }

    bool asBool() const { return std::get<bool>(data_); }
    std::int64_t asInteger() const { return std::get<std::int64_t>(data_); }
    double asNumber() const {
        return isInteger() ? static_cast<double>(std::get<std::int64_t>(data_)) : std::get<double>(data_);
    }
    const std::string& asString() const { return std::get<std::string>(data_); }
    const Array& asArray() const { return std::get<Array>(data_); }
    Array& asArray() { return std::get<Array>(data_); }
    const Object& asObject() const { return std::get<Object>(data_); }
    Object& asObject() { return std::get<Object>(data_); }

    const Value& at(std::string_view key) const {
        const auto& object = asObject();
        const auto it = object.find(std::string(key));
        if (it == object.end()) {
            throw std::runtime_error("JSON object missing key: " + std::string(key));
        }
        return it->second;
    }

private:
    Storage data_;
};

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    Value parse() {
        skipWhitespace();
        Value value = parseValue();
        skipWhitespace();
        if (position_ != text_.size()) {
            fail("unexpected trailing data");
        }
        return value;
    }

private:
    Value parseValue() {
        skipWhitespace();
        if (position_ >= text_.size()) {
            fail("unexpected end of input");
        }

        const char ch = text_[position_];
        if (ch == '{') return parseObject();
        if (ch == '[') return parseArray();
        if (ch == '"') return Value(parseString());
        if (ch == 't') return parseLiteral("true", Value(true));
        if (ch == 'f') return parseLiteral("false", Value(false));
        if (ch == 'n') return parseLiteral("null", Value(nullptr));
        if (ch == '-' || (ch >= '0' && ch <= '9')) return parseNumber();
        fail("unexpected token");
        return {};
    }

    Value parseObject() {
        consume('{');
        Value::Object object;
        skipWhitespace();
        if (peek('}')) {
            consume('}');
            return Value(std::move(object));
        }

        while (true) {
            skipWhitespace();
            if (!peek('"')) {
                fail("object key must be a string");
            }
            std::string key = parseString();
            skipWhitespace();
            consume(':');
            Value value = parseValue();
            if (!object.emplace(std::move(key), std::move(value)).second) {
                fail("duplicate object key");
            }
            skipWhitespace();
            if (peek('}')) {
                consume('}');
                break;
            }
            consume(',');
        }
        return Value(std::move(object));
    }

    Value parseArray() {
        consume('[');
        Value::Array array;
        skipWhitespace();
        if (peek(']')) {
            consume(']');
            return Value(std::move(array));
        }
        while (true) {
            array.push_back(parseValue());
            skipWhitespace();
            if (peek(']')) {
                consume(']');
                break;
            }
            consume(',');
        }
        return Value(std::move(array));
    }

    std::string parseString() {
        consume('"');
        std::string result;
        while (position_ < text_.size()) {
            const char ch = text_[position_++];
            if (ch == '"') {
                return result;
            }
            if (ch == '\\') {
                if (position_ >= text_.size()) fail("unfinished escape sequence");
                const char escaped = text_[position_++];
                switch (escaped) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: fail("unsupported JSON escape sequence");
                }
            } else {
                if (static_cast<unsigned char>(ch) < 0x20) {
                    fail("control character inside JSON string");
                }
                result += ch;
            }
        }
        fail("unterminated JSON string");
        return {};
    }

    Value parseNumber() {
        const std::size_t start = position_;
        if (peek('-')) ++position_;
        if (position_ >= text_.size()) fail("invalid number");

        if (peek('0')) {
            ++position_;
        } else {
            if (!isDigit(text_[position_])) fail("invalid number");
            while (position_ < text_.size() && isDigit(text_[position_])) ++position_;
        }

        bool floating = false;
        if (peek('.')) {
            floating = true;
            ++position_;
            if (position_ >= text_.size() || !isDigit(text_[position_])) fail("invalid fraction");
            while (position_ < text_.size() && isDigit(text_[position_])) ++position_;
        }

        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            floating = true;
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            if (position_ >= text_.size() || !isDigit(text_[position_])) fail("invalid exponent");
            while (position_ < text_.size() && isDigit(text_[position_])) ++position_;
        }

        const std::string token(text_.substr(start, position_ - start));
        try {
            if (!floating) {
                return Value(static_cast<std::int64_t>(std::stoll(token)));
            }
            return Value(std::stod(token));
        } catch (...) {
            fail("number out of range");
        }
        return {};
    }

    Value parseLiteral(std::string_view literal, Value value) {
        if (text_.substr(position_, literal.size()) != literal) {
            fail("invalid literal");
        }
        position_ += literal.size();
        return value;
    }

    void skipWhitespace() {
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                ++position_;
            } else {
                break;
            }
        }
    }

    bool peek(char expected) const {
        return position_ < text_.size() && text_[position_] == expected;
    }

    void consume(char expected) {
        if (!peek(expected)) {
            fail(std::string("expected '") + expected + "'");
        }
        ++position_;
    }

    static bool isDigit(char ch) {
        return ch >= '0' && ch <= '9';
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error("JSON parse error at offset " + std::to_string(position_) + ": " + message);
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

inline std::string escapeString(std::string_view value) {
    std::string result;
    result.reserve(value.size() + 8);
    for (const unsigned char raw : value) {
        const char ch = static_cast<char>(raw);
        switch (ch) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (raw < 0x20) {
                    std::ostringstream hex;
                    hex << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(raw);
                    result += hex.str();
                } else {
                    result += ch;
                }
        }
    }
    return result;
}

inline void stringifyInto(const Value& value, std::ostringstream& out, int indent, int depth) {
    const std::string padding(static_cast<std::size_t>(indent * depth), ' ');
    const std::string childPadding(static_cast<std::size_t>(indent * (depth + 1)), ' ');

    if (value.isNull()) { out << "null"; return; }
    if (value.isBool()) { out << (value.asBool() ? "true" : "false"); return; }
    if (value.isInteger()) { out << value.asInteger(); return; }
    if (value.isNumber()) { out << std::setprecision(15) << value.asNumber(); return; }
    if (value.isString()) { out << '"' << escapeString(value.asString()) << '"'; return; }

    if (value.isArray()) {
        const auto& array = value.asArray();
        out << '[';
        if (!array.empty()) {
            if (indent > 0) out << '\n';
            for (std::size_t index = 0; index < array.size(); ++index) {
                if (indent > 0) out << childPadding;
                stringifyInto(array[index], out, indent, depth + 1);
                if (index + 1 < array.size()) out << ',';
                if (indent > 0) out << '\n';
            }
            if (indent > 0) out << padding;
        }
        out << ']';
        return;
    }

    const auto& object = value.asObject();
    out << '{';
    if (!object.empty()) {
        if (indent > 0) out << '\n';
        std::size_t index = 0;
        for (const auto& [key, child] : object) {
            if (indent > 0) out << childPadding;
            out << '"' << escapeString(key) << "\":";
            if (indent > 0) out << ' ';
            stringifyInto(child, out, indent, depth + 1);
            if (++index < object.size()) out << ',';
            if (indent > 0) out << '\n';
        }
        if (indent > 0) out << padding;
    }
    out << '}';
}

inline std::string stringify(const Value& value, int indent = 2) {
    std::ostringstream out;
    stringifyInto(value, out, indent, 0);
    if (indent > 0) out << '\n';
    return out.str();
}

inline Value parse(std::string_view text) {
    return Parser(text).parse();
}

} // namespace fantasy::studio::json
