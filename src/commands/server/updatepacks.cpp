#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <fstream>
#include <filesystem>

namespace primebds::commands
{

    static bool cmd_updatepacks(PrimeBDS &plugin, endstone::CommandSender &sender,
                                const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /updatepacks <resource|behavior> [version]");
            return false;
        }

        std::string pack_type = args[0];
        for (auto &c : pack_type)
            c = (char)std::tolower(c);

        if (pack_type != "resource" && pack_type != "behavior")
        {
            sender.sendMessage("\u00a7cPack type must be 'resource' or 'behavior'");
            return false;
        }

        std::string version = (args.size() >= 2) ? args[1] : "";

        // Path depends on pack type
        std::string dir = (pack_type == "resource") ? "resource_packs" : "behavior_packs";
        std::string base_path = dir;

        if (!std::filesystem::exists(base_path))
        {
            sender.sendMessage("\u00a7cPack directory not found: " + dir);
            return false;
        }

        int updated = 0;
        for (auto &entry : std::filesystem::directory_iterator(base_path))
        {
            if (!entry.is_directory())
                continue;
            auto manifest_path = entry.path() / "manifest.json";
            if (!std::filesystem::exists(manifest_path))
                continue;

            try
            {
                std::ifstream ifs(manifest_path);
                nlohmann::json manifest;
                ifs >> manifest;
                ifs.close();

                if (!version.empty() && manifest.contains("header") && manifest["header"].contains("version"))
                {
                    // Parse version string "x.y.z"
                    auto parts_str = version;
                    int major = 0, minor = 0, patch = 0;
                    auto p1 = parts_str.find('.');
                    auto p2 = parts_str.find('.', p1 + 1);
                    major = std::atoi(parts_str.substr(0, p1).c_str());
                    if (p1 != std::string::npos)
                        minor = std::atoi(parts_str.substr(p1 + 1, p2 - p1 - 1).c_str());
                    if (p2 != std::string::npos)
                        patch = std::atoi(parts_str.substr(p2 + 1).c_str());
                    manifest["header"]["version"] = {major, minor, patch};

                    std::ofstream ofs(manifest_path);
                    ofs << manifest.dump(4);
                    ++updated;
                }
            }
            catch (...)
            {
                continue;
            }
        }

        if (updated > 0)
        {
            sender.sendMessage("\u00a7aUpdated " + std::to_string(updated) + " " + pack_type + " pack(s) to version " + version);
        }
        else
        {
            sender.sendMessage("\u00a7eNo packs were updated");
        }
        return true;
    }

    REGISTER_COMMAND(updatepacks, "Update resource or behavior pack versions!", cmd_updatepacks,
                     info.usages = {"/updatepacks <resource|behavior> [version: string]"};
                     info.permissions = {"primebds.command.updatepacks"};);
} // namespace primebds::commands
