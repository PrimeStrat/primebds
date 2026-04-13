/// @file tag_overrides.h
/// Tag-based combat mechanic override system.

#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace primebds::handlers::combat
{

    std::optional<double> getCustomTag(const nlohmann::json &config,
                                       const std::vector<std::string> &tags,
                                       const std::string &key);

} // namespace primebds::handlers::combat
