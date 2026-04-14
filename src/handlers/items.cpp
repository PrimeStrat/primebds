/// @file items.cpp
/// Item pickup event handler.

#include "primebds/handlers/items.h"
#include "primebds/plugin.h"

namespace primebds::handlers {

    void handleItemPickupEvent(PrimeBDS &plugin, endstone::PlayerPickupItemEvent &event) {
        auto it = plugin.gamerules.find("can_pickup_items");
        if (it != plugin.gamerules.end() && !it->second)
            event.setCancelled(true);
    }

} // namespace primebds::handlers
