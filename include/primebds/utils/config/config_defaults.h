/// @file config_defaults.h
/// Default configuration values for all modules.

#pragma once

#include <nlohmann/json.hpp>

namespace primebds::config {

    nlohmann::json getDefaultModules();
    nlohmann::json getDefaultPermissions();
    std::vector<std::string> getDefaultRules();

} // namespace primebds::config
