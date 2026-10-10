#pragma once

#include "MapCommand.hpp"
#include "../Core/MapDocument.hpp"

#include <optional>
#include <string>
#include <vector>

namespace fantasy::studio::mapcore {

enum class ValidationSeverity {
    Warning,
    Error,
};

struct ValidationIssue {
    ValidationSeverity severity = ValidationSeverity::Error;
    std::string code;
    std::string message;
    std::optional<Position> position;
};

struct ValidationReport {
    std::vector<ValidationIssue> issues;

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] std::string firstError() const;
};

class CommandValidator {
public:
    [[nodiscard]] ValidationReport validate(const MapDocument& document, const MapCommand& command) const;

private:
    static void validatePosition(ValidationReport& report, const Position& position);
    static void validateItem(ValidationReport& report, const Item& item, const std::optional<Position>& position);
};

} // namespace fantasy::studio::mapcore
