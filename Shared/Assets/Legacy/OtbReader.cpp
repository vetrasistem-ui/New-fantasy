#include "Shared/Assets/Legacy/OtbReader.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace fantasy::assets::legacy {
namespace {

constexpr std::uint8_t kNodeStart = 0xFE;
constexpr std::uint8_t kNodeEnd = 0xFF;
constexpr std::uint8_t kEscape = 0xFD;
constexpr std::uint8_t kRootVersionAttribute = 0x01;
constexpr std::uint8_t kServerIdAttribute = 0x10;
constexpr std::uint8_t kClientIdAttribute = 0x11;
constexpr std::uint8_t kSpriteHashAttribute = 0x20;

struct Node {
    std::uint8_t type = 0;
    std::vector<std::uint8_t> properties;
    std::vector<Node> children;
};

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("Unable to open OTB file: " + path.string());
    }
    const std::streamsize size = stream.tellg();
    if (size < 0) {
        throw std::runtime_error("Unable to determine OTB file size: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("Unable to read OTB file: " + path.string());
    }
    return bytes;
}

std::uint16_t readU16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) {
        throw std::runtime_error("OTB property is truncated while reading uint16");
    }
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t readU32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 4 > bytes.size()) {
        throw std::runtime_error("OTB property is truncated while reading uint32");
    }
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

Node parseNode(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    if (cursor >= bytes.size() || bytes[cursor] != kNodeStart) {
        throw std::runtime_error("OTB node does not start with 0xFE");
    }
    ++cursor;
    if (cursor >= bytes.size()) {
        throw std::runtime_error("OTB node type is missing");
    }

    Node node;
    node.type = bytes[cursor++];

    while (cursor < bytes.size()) {
        const std::uint8_t token = bytes[cursor];
        if (token == kEscape) {
            if (cursor + 1 >= bytes.size()) {
                throw std::runtime_error("OTB escape byte is truncated");
            }
            node.properties.push_back(bytes[cursor + 1]);
            cursor += 2;
            continue;
        }
        if (token == kNodeStart) {
            node.children.push_back(parseNode(bytes, cursor));
            continue;
        }
        if (token == kNodeEnd) {
            ++cursor;
            return node;
        }
        node.properties.push_back(token);
        ++cursor;
    }

    throw std::runtime_error("OTB node is missing its 0xFF terminator");
}

std::vector<OtbAttribute> parseAttributes(
    const std::vector<std::uint8_t>& properties,
    std::size_t start) {
    std::vector<OtbAttribute> result;
    std::size_t cursor = start;
    while (cursor < properties.size()) {
        if (cursor + 3 > properties.size()) {
            throw std::runtime_error("OTB TLV attribute header is truncated");
        }
        OtbAttribute attribute;
        attribute.id = properties[cursor++];
        const std::uint16_t length = readU16(properties, cursor);
        cursor += 2;
        if (cursor + length > properties.size()) {
            throw std::runtime_error("OTB TLV attribute payload is truncated");
        }
        attribute.value.insert(
            attribute.value.end(),
            properties.begin() + static_cast<std::ptrdiff_t>(cursor),
            properties.begin() + static_cast<std::ptrdiff_t>(cursor + length));
        cursor += length;
        result.push_back(std::move(attribute));
    }
    return result;
}

OtbVersionInfo parseVersion(const Node& root) {
    if (root.properties.size() < 4) {
        throw std::runtime_error("OTB root properties are truncated");
    }
    const auto attributes = parseAttributes(root.properties, 4);
    const auto versionIt = std::find_if(
        attributes.begin(), attributes.end(),
        [](const OtbAttribute& attribute) { return attribute.id == kRootVersionAttribute; });
    if (versionIt == attributes.end() || versionIt->value.size() < 12) {
        throw std::runtime_error("OTB root version attribute is missing or invalid");
    }

    OtbVersionInfo version;
    version.major = readU32(versionIt->value, 0);
    version.minor = readU32(versionIt->value, 4);
    version.build = readU32(versionIt->value, 8);
    if (versionIt->value.size() > 12) {
        const auto begin = versionIt->value.begin() + 12;
        const auto nullIt = std::find(begin, versionIt->value.end(), static_cast<std::uint8_t>(0));
        version.description.assign(begin, nullIt);
    }
    return version;
}

OtbItemRecord parseItem(const Node& node) {
    OtbItemRecord record;
    record.group = node.type;
    if (node.properties.size() < 4) {
        throw std::runtime_error("OTB item properties are truncated");
    }
    record.flags = readU32(node.properties, 0);
    record.attributes = parseAttributes(node.properties, 4);

    for (const OtbAttribute& attribute : record.attributes) {
        if (attribute.id == kServerIdAttribute && attribute.value.size() == 2) {
            record.serverId = readU16(attribute.value, 0);
        } else if (attribute.id == kClientIdAttribute && attribute.value.size() == 2) {
            record.clientId = readU16(attribute.value, 0);
        } else if (attribute.id == kSpriteHashAttribute && attribute.value.size() == 16) {
            std::array<std::uint8_t, 16> hash{};
            std::copy(attribute.value.begin(), attribute.value.end(), hash.begin());
            record.spriteHash = hash;
        }
    }
    return record;
}

} // namespace

OtbReader::OtbReader(const std::filesystem::path& path) {
    const std::vector<std::uint8_t> bytes = readFile(path);
    if (bytes.size() < 7) {
        throw std::runtime_error("OTB file is too small");
    }

    // The first four bytes are the OTB identifier. The node tree follows.
    std::size_t cursor = 4;
    Node root = parseNode(bytes, cursor);
    if (cursor != bytes.size()) {
        throw std::runtime_error("OTB contains trailing bytes after the root node");
    }

    version_ = parseVersion(root);
    items_.reserve(root.children.size());
    for (const Node& child : root.children) {
        items_.push_back(parseItem(child));
    }
}

const OtbVersionInfo& OtbReader::version() const noexcept {
    return version_;
}

const std::vector<OtbItemRecord>& OtbReader::items() const noexcept {
    return items_;
}

const OtbItemRecord* OtbReader::findByServerId(std::uint16_t serverId) const noexcept {
    const auto it = std::find_if(
        items_.begin(), items_.end(),
        [serverId](const OtbItemRecord& item) {
            return item.serverId.has_value() && *item.serverId == serverId;
        });
    return it == items_.end() ? nullptr : &*it;
}

} // namespace fantasy::assets::legacy
