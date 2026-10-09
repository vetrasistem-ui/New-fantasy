#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

class FantasySystemCatalog {
public:
    void add(SystemDefinition definition) {
        definition.validate();
        const auto id = definition.id;
        if (!systems_.emplace(id, std::move(definition)).second) {
            throw std::invalid_argument("duplicate Fantasy system id: " + id);
        }
    }

    [[nodiscard]] bool contains(const std::string& id) const noexcept {
        return systems_.find(id) != systems_.end();
    }

    [[nodiscard]] const SystemDefinition& at(const std::string& id) const {
        return systems_.at(id);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return systems_.size();
    }

    void validateDependencies() const {
        for (const auto& [id, definition] : systems_) {
            for (const auto& dependency : definition.dependencies) {
                if (!contains(dependency)) {
                    throw std::runtime_error(
                        "Fantasy system dependency missing: " + id + " -> " + dependency);
                }
            }
        }
        (void)executionOrder();
    }

    [[nodiscard]] std::vector<std::string> executionOrder() const {
        enum class Mark { Visiting, Done };
        std::map<std::string, Mark> marks;
        std::vector<std::string> ordered;

        const auto visit = [&](const auto& self, const std::string& id) -> void {
            const auto mark = marks.find(id);
            if (mark != marks.end()) {
                if (mark->second == Mark::Visiting) {
                    throw std::runtime_error("Fantasy system dependency cycle detected at: " + id);
                }
                return;
            }
            marks.emplace(id, Mark::Visiting);
            const auto& definition = systems_.at(id);
            for (const auto& dependency : definition.dependencies) {
                if (!contains(dependency)) {
                    throw std::runtime_error(
                        "Fantasy system dependency missing: " + id + " -> " + dependency);
                }
                self(self, dependency);
            }
            marks[id] = Mark::Done;
            ordered.push_back(id);
        };

        for (const auto& [id, definition] : systems_) {
            (void)definition;
            visit(visit, id);
        }
        return ordered;
    }

    [[nodiscard]] std::set<std::string> requiredChannels() const {
        std::set<std::string> channels;
        for (const auto& [id, definition] : systems_) {
            (void)id;
            channels.insert(definition.requiredChannels.begin(), definition.requiredChannels.end());
        }
        return channels;
    }

private:
    std::map<std::string, SystemDefinition> systems_;
};

} // namespace fantasy::studio::foundation
