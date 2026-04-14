/// @file items.h
/// Item pickup event handler.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds {
    class PrimeBDS;
}

namespace primebds::handlers {

    void handleItemPickupEvent(PrimeBDS &plugin, endstone::PlayerPickupItemEvent &event);

} // namespace primebds::handlers
