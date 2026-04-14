/// @file target_selector.cpp
/// Target selector parsing and entity matching.

#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace primebds::utils {

    namespace {

        struct ParsedSelector {
            char type;
            std::unordered_map<std::string, std::pair<std::string, bool>> args; // key -> (value, negate)
        };

        std::vector<std::string> splitArgs(const std::string &arg_str) {
            std::vector<std::string> parts;
            int depth = 0;
            std::string current;
            for (char ch : arg_str) {
                if (ch == '{')
                    depth++;
                else if (ch == '}')
                    depth--;
                if (ch == ',' && depth == 0) {
                    // Trim
                    auto s = current.find_first_not_of(" \t");
                    auto e = current.find_last_not_of(" \t");
                    if (s != std::string::npos)
                        parts.push_back(current.substr(s, e - s + 1));
                    else
                        parts.push_back("");
                    current.clear();
                } else {
                    current += ch;
                }
            }
            if (!current.empty()) {
                auto s = current.find_first_not_of(" \t");
                auto e = current.find_last_not_of(" \t");
                if (s != std::string::npos)
                    parts.push_back(current.substr(s, e - s + 1));
            }
            return parts;
        }

        std::optional<ParsedSelector> parseSelector(const std::string &selector) {
            static const std::regex sel_re(R"(@([aprs])(?:\[(.*?)\])?)");
            std::smatch match;
            if (!std::regex_match(selector, match, sel_re))
                return std::nullopt;

            ParsedSelector parsed;
            parsed.type = match[1].str()[0];

            if (match[2].matched) {
                auto args_str = match[2].str();
                for (auto &pair : splitArgs(args_str)) {
                    bool negate = false;
                    std::string key, value;
                    auto neg_pos = pair.find("=!");
                    if (neg_pos != std::string::npos) {
                        key = pair.substr(0, neg_pos);
                        value = pair.substr(neg_pos + 2);
                        negate = true;
                    } else {
                        auto eq_pos = pair.find('=');
                        if (eq_pos == std::string::npos)
                            continue;
                        key = pair.substr(0, eq_pos);
                        value = pair.substr(eq_pos + 1);
                    }
                    // Trim
                    auto trim = [](std::string &s) {
                        auto a = s.find_first_not_of(" \t");
                        auto b = s.find_last_not_of(" \t");
                        s = (a != std::string::npos) ? s.substr(a, b - a + 1) : "";
                    };
                    trim(key);
                    trim(value);
                    parsed.args[key] = {value, negate};
                }
            }

            return parsed;
        }

        std::string toLower(const std::string &s) {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(), ::tolower);
            return out;
        }

        double distanceSq(const endstone::Location &a, const endstone::Location &b) {
            double dx = a.getX() - b.getX();
            double dy = a.getY() - b.getY();
            double dz = a.getZ() - b.getZ();
            return dx * dx + dy * dy + dz * dz;
        }

        bool passesFilters(endstone::Player *player, const ParsedSelector &parsed,
                           const endstone::Location &origin) {
            // Name filter
            auto it = parsed.args.find("name");
            if (it != parsed.args.end()) {
                bool match = toLower(player->getName()) == toLower(it->second.first);
                if (it->second.second)
                    match = !match;
                if (!match)
                    return false;
            }

            // Radius min/max
            it = parsed.args.find("rm");
            if (it != parsed.args.end()) {
                double rm = std::stod(it->second.first);
                if (distanceSq(player->getLocation(), origin) < rm * rm)
                    return false;
            }

            it = parsed.args.find("r");
            if (it != parsed.args.end()) {
                double r = std::stod(it->second.first);
                if (distanceSq(player->getLocation(), origin) > r * r)
                    return false;
            }

            return true;
        }

    } // anonymous namespace

    std::vector<endstone::Actor *> getMatchingActors(endstone::Server &server,
                                                     const std::string &selector,
                                                     endstone::CommandSender &origin) {
        auto players = server.getOnlinePlayers();

        if (!selector.empty() && selector[0] != '@') {
            // Plain name lookup
            auto lower = toLower(selector);
            for (auto *p : players) {
                if (toLower(p->getName()) == lower)
                    return {p};
            }
            return {};
        }

        auto parsed = parseSelector(selector);
        if (!parsed)
            return {};

        auto *p_sender = dynamic_cast<endstone::Player *>(&origin);
        if (!p_sender)
            return {}; // Target selectors require a player context

        auto origin_loc = p_sender->getLocation();

        // Check for explicit x/y/z overrides
        auto get_coord = [&](const std::string &key, double def) -> double {
            auto it = parsed->args.find(key);
            if (it != parsed->args.end())
                return std::stod(it->second.first);
            return def;
        };
        origin_loc = endstone::Location(
            p_sender->getDimension(),
            get_coord("x", origin_loc.getX()),
            get_coord("y", origin_loc.getY()),
            get_coord("z", origin_loc.getZ()));

        std::vector<endstone::Actor *> result;

        if (parsed->type == 's') {
            if (p_sender && passesFilters(p_sender, *parsed, origin_loc))
                result.push_back(p_sender);
            return result;
        }

        for (auto *p : players) {
            if (passesFilters(p, *parsed, origin_loc))
                result.push_back(p);
        }

        if (parsed->type == 'p' && !result.empty()) {
            // Closest
            auto *closest = result[0];
            double best = distanceSq(static_cast<endstone::Player *>(closest)->getLocation(), origin_loc);
            for (size_t i = 1; i < result.size(); ++i) {
                double d = distanceSq(static_cast<endstone::Player *>(result[i])->getLocation(), origin_loc);
                if (d < best) {
                    best = d;
                    closest = result[i];
                }
            }
            return {closest};
        }

        if (parsed->type == 'r' && !result.empty()) {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<size_t> dist(0, result.size() - 1);
            return {result[dist(rng)]};
        }

        return result;
    }

    endstone::Player *resolvePlayerTarget(endstone::Server &server,
                                          const std::string &arg,
                                          endstone::CommandSender &origin) {
        auto actors = getMatchingActors(server, arg, origin);
        if (actors.size() == 1)
            return dynamic_cast<endstone::Player *>(actors[0]);
        return nullptr;
    }

} // namespace primebds::utils
