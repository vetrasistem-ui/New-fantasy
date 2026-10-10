#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

enum class SystemAuthoringMode {
    Visual,
    Hybrid,
    Code,
};

enum class SystemValueType {
    Boolean,
    Integer,
    Decimal,
    String,
    EntityRef,
    ItemRef,
    ZoneRef,
};

struct SystemVariableDefinition {
    std::string id;
    SystemValueType type = SystemValueType::String;
    std::string defaultValue;
    bool persistent = false;

    void validate() const {
        requireIdentifier(id, "system variable id");
    }
};

struct SystemEventDefinition {
    std::string id;
    std::map<std::string, SystemValueType> payload;

    void validate() const {
        requireIdentifier(id, "system event id");
        for (const auto& [field, type] : payload) {
            (void)type;
            requireIdentifier(field, "system event payload field");
        }
    }
};

struct SystemTimerDefinition {
    std::string id;
    std::uint64_t intervalMs = 0;
    bool repeating = false;

    void validate() const {
        requireIdentifier(id, "system timer id");
        if (intervalMs == 0) throw std::invalid_argument("system timer interval must be greater than zero");
    }
};

struct SystemTestExpectation {
    std::string actionId;
    std::map<std::string, std::string> properties;

    void validate() const {
        requireIdentifier(actionId, "system test expected action id");
        for (const auto& [key, value] : properties) {
            (void)value;
            requireIdentifier(key, "system test expectation property");
        }
    }
};

struct SystemTestScenario {
    std::string id;
    std::string triggerNodeId;
    std::map<std::string, std::string> initialState;
    std::map<std::string, std::string> inputPayload;
    std::vector<SystemTestExpectation> expectedActions;

    void validate() const {
        requireIdentifier(id, "system test scenario id");
        requireIdentifier(triggerNodeId, "system test trigger node id");
        if (expectedActions.empty()) {
            throw std::invalid_argument("system test scenario requires at least one expected action");
        }
        for (const auto& expectation : expectedActions) expectation.validate();
    }
};

struct SystemPackageDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::uint32_t version = 1;
    SystemAuthoringMode authoringMode = SystemAuthoringMode::Visual;
    SystemDefinition graph;
    std::vector<SystemVariableDefinition> variables;
    std::vector<SystemEventDefinition> events;
    std::vector<SystemTimerDefinition> timers;
    std::vector<SystemTestScenario> tests;
    std::string codeEntry;

    void validate() const {
        requireIdentifier(id, "system package id");
        if (version == 0) throw std::invalid_argument("system package version must be at least 1");
        graph.validate();
        if (graph.id != id) {
            throw std::invalid_argument("system package id must match graph id");
        }
        if (authoringMode == SystemAuthoringMode::Code && codeEntry.empty()) {
            throw std::invalid_argument("code-mode system package requires a code entry");
        }
        if (authoringMode == SystemAuthoringMode::Visual && !codeEntry.empty()) {
            throw std::invalid_argument("visual-mode system package cannot declare a code entry");
        }

        std::set<std::string> variableIds;
        for (const auto& variable : variables) {
            variable.validate();
            if (!variableIds.insert(variable.id).second) {
                throw std::invalid_argument("duplicate system variable: " + variable.id);
            }
        }
        std::set<std::string> eventIds;
        for (const auto& event : events) {
            event.validate();
            if (!eventIds.insert(event.id).second) {
                throw std::invalid_argument("duplicate system event: " + event.id);
            }
        }
        std::set<std::string> timerIds;
        for (const auto& timer : timers) {
            timer.validate();
            if (!timerIds.insert(timer.id).second) {
                throw std::invalid_argument("duplicate system timer: " + timer.id);
            }
        }
        std::set<std::string> testIds;
        for (const auto& test : tests) {
            test.validate();
            if (!testIds.insert(test.id).second) {
                throw std::invalid_argument("duplicate system test scenario: " + test.id);
            }
        }
    }
};

struct SystemTestResult {
    std::string scenarioId;
    bool passed = false;
    std::vector<std::string> failures;
};

class FantasySystemLabValidator {
public:
    [[nodiscard]] static std::vector<SystemTestResult> validateScenarios(
        const SystemPackageDefinition& package) {

        package.validate();
        std::set<std::string> triggerIds;
        std::set<std::string> actionIds;
        for (const auto& trigger : package.graph.triggers) triggerIds.insert(trigger.id);
        for (const auto& action : package.graph.actions) actionIds.insert(action.id);

        std::vector<SystemTestResult> results;
        for (const auto& scenario : package.tests) {
            SystemTestResult result;
            result.scenarioId = scenario.id;
            if (!triggerIds.contains(scenario.triggerNodeId)) {
                result.failures.push_back("scenario trigger does not exist: " + scenario.triggerNodeId);
            }
            for (const auto& expectation : scenario.expectedActions) {
                if (!actionIds.contains(expectation.actionId)) {
                    result.failures.push_back("expected action does not exist: " + expectation.actionId);
                }
            }
            result.passed = result.failures.empty();
            results.push_back(std::move(result));
        }
        return results;
    }
};

} // namespace fantasy::studio::foundation
