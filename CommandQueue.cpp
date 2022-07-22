//
// Created by michal on 21.07.2022.
//

#include "CommandQueue.h"

std::vector<RingBuffer::Range> CommandQueue::WaitForCommands() const
{
    std::unique_lock lock(mutex);
    if (THREAD_DISPATCH)
        cv.wait(lock, [this]() { return !commandsToExecute.empty() || quit.load(std::memory_order_relaxed); });

    return std::move(commandsToExecute);
}

void CommandQueue::Flush()
{
    if (buffer.empty()) {
        return;
    }

    //ostatnia komenta musi być bez operacji, oznacza ona stop wykonywania się pętli while w funkcji Execute()
    new(buffer.Allocate(sizeof(NoopCommand))) NoopCommand(nullptr);

    auto range = buffer.getRange();
    size_t usedSpace = buffer.size();
    buffer.Circularize();

    std::unique_lock lock(mutex);
    commandsToExecute.push_back(range);

    freeSpace -= usedSpace;
    const size_t requiredSize = this->requiredSize;

    cv.notify_one();

    if (freeSpace < requiredSize) {
        if(THREAD_DISPATCH)
            cv.wait(lock, [this, requiredSize]() { return freeSpace >= requiredSize; });
    }
}

bool CommandQueue::Execute()
{
    auto ranges = WaitForCommands();

    if (ranges.empty()) {
        return false;
    }

    for (auto& item : ranges) {
        if (item.begin) {
            CommandBase* base = static_cast<CommandBase*>(item.begin);
            while (base) {
                base = base->Execute();
            } ReleaseRange(item);
        }
    }

    return true;
}

void CommandQueue::ReleaseRange(const RingBuffer::Range& range)
{
    {
        std::lock_guard guard(mutex);
        freeSpace += range.size();
    }
    cv.notify_one();
}

void CommandQueue::Quit()
{
    quit.store(true, std::memory_order_relaxed);
    cv.notify_one();
}