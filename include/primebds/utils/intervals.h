/// @file intervals.h
/// Interval manager for scheduled recurring tasks.

#pragma once

#include <endstone/endstone.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace primebds::utils
{

    class IntervalManager
    {
    public:
        using CheckFunc = std::function<void()>;

        void addCheck(CheckFunc func);
        void removeCheck(CheckFunc *func);
        void addPlayerCheck(const std::string &player_name, CheckFunc func);
        void removePlayerCheck(const std::string &player_name);

        void start(endstone::Plugin &plugin);
        void stop();

        bool isRunning() const { return running_; }

    private:
        std::vector<CheckFunc> global_checks_;
        std::map<std::string, CheckFunc> player_checks_;
        mutable std::mutex mutex_;
        bool running_ = false;
        std::shared_ptr<endstone::Task> task_;
    };

} // namespace primebds::utils
