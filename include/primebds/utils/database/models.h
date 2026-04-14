/// @file models.h
/// Database model structs ported from Python dataclasses.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace primebds::db {

    struct ServerData {
        int64_t last_shutdown_time = 0;
        std::string allowlist_profile;
        int can_interact = 1;
        int can_emote = 1;
        int can_decay_leaves = 1;
        int can_change_skin = 1;
        int can_pickup_items = 1;
        int can_sleep = 1;
        int can_split_screen = 1;
    };

    struct NameBan {
        std::string name;
        int64_t banned_time = 0;
        std::string ban_reason;
    };

    struct User {
        std::string xuid;
        std::string uuid;
        std::string name;
        int ping = 0;
        std::string device_os;
        std::string device_id;
        int64_t unique_id = 0;
        std::string client_ver;
        std::string internal_rank;
        int gamemode = 0;
        int xp = 0;
        std::string perms;
        int is_silent_muted = 0;
        int is_afk = 0;
        std::string last_messaged;
        int64_t last_join = 0;
        int64_t last_leave = 0;
        std::string last_logout_pos;
        std::string last_logout_dim;
        int enabled_mt = 0;
        int enabled_ss = 0;
        int enabled_ms = 0;
        int enabled_as = 0;
        int enabled_sc = 0;
    };

    struct ModLog {
        std::string xuid;
        std::string name;
        bool is_muted = false;
        int64_t mute_time = 0;
        std::string mute_reason;
        bool is_banned = false;
        int64_t banned_time = 0;
        std::string ban_reason;
        std::string ip_address;
        bool is_ip_banned = false;
        bool is_ip_muted = false;
    };

    struct Warn {
        int id = 0;
        std::string xuid;
        std::string name;
        std::string warn_reason;
        int64_t warn_time = 0;
        std::string added_by;
    };

    struct Note {
        int id = 0;
        std::string xuid;
        std::string name;
        std::string note;
        int64_t timestamp = 0;
        std::string added_by;
    };

    struct PunishmentLog {
        int id = 0;
        std::string xuid;
        std::string name;
        std::string action_type;
        std::string reason;
        int64_t timestamp = 0;
        std::optional<int64_t> duration;
    };

    struct Warp {
        std::string name;
        std::string pos;
        std::string displayname;
        std::string category;
        std::string description;
        double cost = 0.0;
        double cooldown = 0.0;
        double delay = 0.0;
        std::vector<std::string> aliases;
    };

    struct Home {
        std::string xuid;
        std::string username;
        std::string name;
        std::string pos;
        double cost = 0.0;
        double cooldown = 0.0;
        double delay = 0.0;
    };

    struct Spawn {
        std::string pos;
        double cost = 0.0;
        double cooldown = 0.0;
        double delay = 0.0;
    };

    struct LastWarp {
        std::string xuid;
        std::string username;
        std::string name;
        std::string pos;
        double cost = 0.0;
        double cooldown = 0.0;
        double delay = 0.0;
    };

    struct HomeSettings {
        double delay = 0.0;
        double cooldown = 0.0;
        double cost = 0.0;
    };

    struct Alt {
        std::string main_name;
        std::string main_xuid;
        std::string alt_name;
        std::string alt_xuid;
        int64_t expiry = 0;
    };

    struct Rank {
        std::string name;
        int weight = 0;
        std::string prefix;
        std::string suffix;
        std::string inherits;
        std::vector<std::string> permissions;
    };

    struct AllowlistEntry {
        std::string name;
        std::string profile;
    };

    struct AllowlistProfile {
        std::string name;
        bool active = false;
        std::string inherits;
    };

} // namespace primebds::db
