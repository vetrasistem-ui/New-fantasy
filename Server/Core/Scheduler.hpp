#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace fantasy::server {

class Scheduler {
public:
    using TaskId = std::uint64_t;

    TaskId scheduleAfter(std::uint64_t delayTicks, std::function<void()> callback);
    bool cancel(TaskId id);
    void tick();
    void clear();

    std::uint64_t currentTick() const { return currentTick_; }
    std::size_t pendingTaskCount() const { return tasks_.size(); }

private:
    struct Task {
        TaskId id = 0;
        std::uint64_t dueTick = 0;
        std::function<void()> callback;
    };

    std::uint64_t currentTick_ = 0;
    TaskId nextTaskId_ = 1;
    std::vector<Task> tasks_;
};

} // namespace fantasy::server
