/// @file gamerules.h
/// Custom gamerule enforcement handlers.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds {
    class PrimeBDS;
}

namespace primebds::handlers {

    void handleBedEnterEvent(PrimeBDS &plugin, endstone::PlayerBedEnterEvent &event);
    void handleEmoteEvent(PrimeBDS &plugin, endstone::PlayerEmoteEvent &event);
    void handleLeavesDecayEvent(PrimeBDS &plugin, endstone::LeavesDecayEvent &event);
    void handleSkinChangeEvent(PrimeBDS &plugin, endstone::PlayerSkinChangeEvent &event);
    void handleServerLoadEvent(PrimeBDS &plugin, endstone::ServerLoadEvent &event);

} // namespace primebds::handlers
