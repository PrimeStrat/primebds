/// @file intervals.cpp
/// Interval manager for recurring scheduled tasks.

#include "primebds/utils/intervals.h"

#include <iostream>

namespace primebds::utils
{

    void IntervalManager::addCheck(CheckFunc func)
    {
        std::lock_guard lock(mutex_);
        global_checks_.push_back(std::move(func));
    }

    void IntervalManager::removeCheck(CheckFunc *func)
    {
        // Note: std::function cannot be compared for equality.
        // Callers should track indices or use a different mechanism.
        (void)func;
    }

    void IntervalManager::addPlayerCheck(const std::string &player_name, CheckFunc func)
    {
        std::lock_guard lock(mutex_);
        player_checks_[player_name] = std::move(func);
    }

    void IntervalManager::removePlayerCheck(const std::string &player_name)
    {
        std::lock_guard lock(mutex_);
        player_checks_.erase(player_name);
    }

    void IntervalManager::start(endstone::Plugin &plugin)
    {
        if (running_)
            return;
        running_ = true;

        // Schedule a repeating task every 20 ticks (1 second)
        task_ = plugin.getServer().getScheduler().runTaskTimer(plugin, [this]()
                                                               {
        std::lock_guard lock(mutex_);

        // Run global checks
        for (auto &func : global_checks_) {
            try {
                func();
            }
            catch (const std::exception &e) {
                std::cerr << "[PrimeBDS] Error in interval check: " << e.what() << "\n";
            }
        }

        // Run player-specific checks
        for (auto it = player_checks_.begin(); it != player_checks_.end();) {
            try {
                it->second();
                ++it;
            }
            catch (const std::exception &e) {
                std::cerr << "[PrimeBDS] Error in player interval " << it->first << ": " << e.what() << "\n";
                it = player_checks_.erase(it);
            }
        } }, 0, 20);
    }

    void IntervalManager::stop()
    {
        if (!running_)
            return;
        running_ = false;
        if (task_)
        {
            task_->cancel();
            task_ = nullptr;
        }
    }

} // namespace primebds::utils
