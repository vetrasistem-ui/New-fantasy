#pragma once

#include "RuntimeMessaging.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

class Tfs1098SystemChannelRegistry {
public:
    static constexpr std::uint32_t SchemaVersion = 1;

    // 0 is used by the OTClient extended-opcode handshake and upstream TFS ships
    // an example language channel on 1. Fantasy keeps its generated channels in
    // a high, explicit range so they do not inherit numbers from reference forks.
    static constexpr std::uint32_t MinFantasyOpcode = 200;
    static constexpr std::uint32_t MaxFantasyOpcode = 239;

    [[nodiscard]] static std::filesystem::path pathForProject(
        const std::filesystem::path& projectRoot) {
        return projectRoot / "Game" / "Config" / "tfs1098.channels.json";
    }

    [[nodiscard]] static std::vector<RuntimeChannelBinding> load(
        const std::filesystem::path& projectRoot) {

        const auto path = pathForProject(projectRoot);
        if (!std::filesystem::exists(path)) return {};

        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Unable to open TFS1098 system-channel registry: " + path.string());
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        const std::string json = buffer.str();

        if (extractInteger(json, "schemaVersion") != static_cast<int>(SchemaVersion)) {
            throw std::runtime_error("Unsupported TFS1098 system-channel registry schemaVersion");
        }
        if (extractString(json, "profile") != "otc_extended") {
            throw std::runtime_error("TFS1098 system-channel registry profile must be 'otc_extended'");
        }

        std::vector<RuntimeChannelBinding> bindings;
        const std::regex entry(
            R"FANTASY_REGEX(\{\s*"channel"\s*:\s*"([^"]+)"\s*,\s*"opcode"\s*:\s*([0-9]+)\s*\})FANTASY_REGEX");
        for (std::sregex_iterator it(json.begin(), json.end(), entry), end; it != end; ++it) {
            const auto channel = SystemChannelId::parse((*it)[1].str());
            const auto opcode = static_cast<std::uint32_t>(std::stoul((*it)[2].str()));
            bindings.push_back(RuntimeChannelBinding{channel, opcode});
        }
        validate(bindings);
        sortStable(bindings);
        return bindings;
    }

    static void save(
        const std::filesystem::path& projectRoot,
        std::vector<RuntimeChannelBinding> bindings) {

        validate(bindings);
        sortStable(bindings);

        const auto path = pathForProject(projectRoot);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to write TFS1098 system-channel registry: " + path.string());
        }

        output
            << "{\n"
            << "  \"schemaVersion\": " << SchemaVersion << ",\n"
            << "  \"profile\": \"otc_extended\",\n"
            << "  \"bindings\": [\n";
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            const auto& binding = bindings[i];
            output
                << "    {\"channel\": \"" << escapeJson(binding.channel.value)
                << "\", \"opcode\": " << binding.wireCode << "}";
            if (i + 1U != bindings.size()) output << ',';
            output << '\n';
        }
        output << "  ]\n}\n";
        if (!output) {
            throw std::runtime_error("Failed while writing TFS1098 system-channel registry: " + path.string());
        }
    }

    [[nodiscard]] static RuntimeChannelBinding bind(
        std::vector<RuntimeChannelBinding>& bindings,
        const SystemChannelId& channel) {

        validate(bindings);
        const auto existing = std::find_if(bindings.begin(), bindings.end(), [&](const auto& binding) {
            return binding.channel == channel;
        });
        if (existing != bindings.end()) return *existing;

        std::array<bool, 256> used{};
        for (const auto& binding : bindings) used.at(binding.wireCode) = true;

        for (std::uint32_t opcode = MinFantasyOpcode; opcode <= MaxFantasyOpcode; ++opcode) {
            if (used.at(opcode)) continue;
            RuntimeChannelBinding result{channel, opcode};
            bindings.push_back(result);
            sortStable(bindings);
            return result;
        }
        throw std::runtime_error("Fantasy TFS1098 extended-opcode registry is full");
    }

    static void validate(const std::vector<RuntimeChannelBinding>& bindings) {
        std::set<std::string> channels;
        std::set<std::uint32_t> opcodes;
        for (const auto& binding : bindings) {
            if (!validSemanticIdentifier(binding.channel.value)) {
                throw std::runtime_error("Invalid Fantasy semantic channel in TFS1098 registry");
            }
            if (binding.wireCode < MinFantasyOpcode || binding.wireCode > MaxFantasyOpcode) {
                throw std::runtime_error("TFS1098 Fantasy extended opcode is outside the reserved range 200-239");
            }
            if (!channels.insert(binding.channel.value).second) {
                throw std::runtime_error("Duplicate semantic channel in TFS1098 registry: " + binding.channel.value);
            }
            if (!opcodes.insert(binding.wireCode).second) {
                throw std::runtime_error("Duplicate extended opcode in TFS1098 registry");
            }
        }
    }

private:
    static void sortStable(std::vector<RuntimeChannelBinding>& bindings) {
        std::sort(bindings.begin(), bindings.end(), [](const auto& left, const auto& right) {
            return left.wireCode < right.wireCode;
        });
    }

    [[nodiscard]] static std::string extractString(const std::string& json, const std::string& key) {
        const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) {
            throw std::runtime_error("Missing string field in TFS1098 system-channel registry: " + key);
        }
        return match[1].str();
    }

    [[nodiscard]] static int extractInteger(const std::string& json, const std::string& key) {
        const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) {
            throw std::runtime_error("Missing integer field in TFS1098 system-channel registry: " + key);
        }
        return std::stoi(match[1].str());
    }

    [[nodiscard]] static std::string escapeJson(const std::string& value) {
        std::string result;
        result.reserve(value.size());
        for (const char ch : value) {
            switch (ch) {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += ch; break;
            }
        }
        return result;
    }
};

} // namespace fantasy::studio::runtime
