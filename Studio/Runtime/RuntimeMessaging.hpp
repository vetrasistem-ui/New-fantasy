#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fantasy::studio::runtime {

[[nodiscard]] constexpr bool asciiAlphaNumeric(unsigned char ch) noexcept {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z');
}

// Stable Fantasy-owned identifiers intentionally do not expose TFS opcodes or
// external-runtime implementation details. Backends map these semantic IDs to
// their own wire representation.
[[nodiscard]] constexpr bool validSemanticIdentifier(const char* value) noexcept {
    if (value == nullptr || *value == '\0') return false;
    bool hasAlphaNumeric = false;
    for (const char* cursor = value; *cursor != '\0'; ++cursor) {
        const unsigned char ch = static_cast<unsigned char>(*cursor);
        if (asciiAlphaNumeric(ch)) {
            hasAlphaNumeric = true;
            continue;
        }
        if (ch == '.' || ch == '_' || ch == '-' || ch == ':') continue;
        return false;
    }
    return hasAlphaNumeric;
}

[[nodiscard]] inline bool validSemanticIdentifier(const std::string& value) noexcept {
    return validSemanticIdentifier(value.c_str());
}

struct SystemChannelId {
    std::string value;

    [[nodiscard]] static SystemChannelId parse(std::string candidate) {
        if (!validSemanticIdentifier(candidate)) {
            throw std::invalid_argument("invalid Fantasy system channel id: " + candidate);
        }
        return SystemChannelId{std::move(candidate)};
    }

    friend bool operator==(const SystemChannelId&, const SystemChannelId&) = default;
};

struct AssetProfileId {
    std::string value;

    [[nodiscard]] static AssetProfileId parse(std::string candidate) {
        if (!validSemanticIdentifier(candidate)) {
            throw std::invalid_argument("invalid Fantasy asset profile id: " + candidate);
        }
        return AssetProfileId{std::move(candidate)};
    }

    friend bool operator==(const AssetProfileId&, const AssetProfileId&) = default;
};

struct SemanticTag {
    std::string value;

    [[nodiscard]] static SemanticTag parse(std::string candidate) {
        if (!validSemanticIdentifier(candidate)) {
            throw std::invalid_argument("invalid Fantasy semantic tag: " + candidate);
        }
        return SemanticTag{std::move(candidate)};
    }

    friend bool operator==(const SemanticTag&, const SemanticTag&) = default;
};

struct RuntimeMessage {
    SystemChannelId channel;
    std::uint16_t version = 1;
    std::vector<std::uint8_t> payload;
};

// Backend-specific wire mappings live outside Fantasy Core. TFS1098 can bind a
// semantic channel to an extended opcode while another runtime can use a
// completely different transport without changing authored systems.
struct RuntimeChannelBinding {
    SystemChannelId channel;
    std::uint32_t wireCode = 0;

    friend bool operator==(const RuntimeChannelBinding&, const RuntimeChannelBinding&) = default;
};

struct SemanticComponent {
    std::string type;
    std::map<std::string, std::string> properties;
};

struct EntityDefinition {
    std::string id;
    std::vector<SemanticTag> tags;
    std::vector<SemanticComponent> components;
};

inline void validateEntityDefinition(const EntityDefinition& definition) {
    if (!validSemanticIdentifier(definition.id)) {
        throw std::invalid_argument("invalid Fantasy entity id: " + definition.id);
    }
    for (const auto& component : definition.components) {
        if (!validSemanticIdentifier(component.type)) {
            throw std::invalid_argument("invalid Fantasy component type: " + component.type);
        }
        for (const auto& [key, value] : component.properties) {
            (void)value;
            if (!validSemanticIdentifier(key)) {
                throw std::invalid_argument("invalid Fantasy component property key: " + key);
            }
        }
    }
}

static_assert(validSemanticIdentifier("ui.inventory"));
static_assert(validSemanticIdentifier("system.quest:v1"));
static_assert(validSemanticIdentifier("appearance.shader"));
static_assert(!validSemanticIdentifier("../unsafe"));
static_assert(!validSemanticIdentifier("contains space"));

} // namespace fantasy::studio::runtime
