#include "Core/Scheduler.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace fantasy::server {

Scheduler::TaskId Scheduler::scheduleAfter(std::uint64_t delayTicks, std::function<void()> callback) {
    if (!callback) throw std::runtime_error("Scheduler callback cannot be empty");
    const auto id = nextTaskId_++;
    const auto due = currentTick_ + (delayTicks == 0 ? 1 : delayTicks);
    tasks_.push_back(Task{id, due, std::move(callback)});
    return id;
}

bool Scheduler::cancel(TaskId id) {
    const auto before = tasks_.size();
    std::erase_if(tasks_, [id](const Task& task) { return task.id == id; });
    return tasks_.size() != before;
}

void Scheduler::tick() {
    ++currentTick_;

    std::vector<std::function<void()>> ready;
    for (const auto& task : tasks_) {
        if (task.dueTick <= currentTick_) ready.push_back(task.callback);
    }

    std::erase_if(tasks_, [this](const Task& task) {
        return task.dueTick <= currentTick_;
    });

    for (auto& callback : ready) callback();
}

void Scheduler::clear() {
    tasks_.clear();
    currentTick_ = 0;
}

} // namespace fantasy::server
