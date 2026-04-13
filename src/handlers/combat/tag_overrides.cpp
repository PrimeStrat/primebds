/// @file tag_overrides.cpp
/// Tag-based combat mechanic override system.

#include "primebds/handlers/combat/tag_overrides.h"

namespace primebds::handlers::combat
{

    std::optional<double> getCustomTag(const nlohmann::json &config,
                                       const std::vector<std::string> &tags,
                                       const std::string &key)
    {
        // Parse dot-separated key path
        auto deepGet = [](const nlohmann::json &obj, const std::string &dotted) -> std::optional<double>
        {
            const nlohmann::json *current = &obj;
            std::string part;
            std::istringstream stream(dotted);
            while (std::getline(stream, part, '.'))
            {
                if (!current->is_object() || !current->contains(part))
                    return std::nullopt;
                current = &(*current)[part];
            }
            if (current->is_number())
                return current->get<double>();
            return std::nullopt;
        };

        // Check tag overrides first
        if (config.contains("modules") && config["modules"].contains("combat"))
        {
            auto &combat = config["modules"]["combat"];
            if (combat.contains("tag_overrides"))
            {
                auto &overrides = combat["tag_overrides"];
                for (auto &tag : tags)
                {
                    if (overrides.contains(tag))
                    {
                        auto val = deepGet(overrides[tag], key);
                        if (val.has_value())
                            return val;
                    }
                }
            }

            // Fall back to global combat config
            return deepGet(combat, key);
        }

        return std::nullopt;
    }

} // namespace primebds::handlers::combat
