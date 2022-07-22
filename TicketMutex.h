//
// Created by michal on 21.07.2022.
//

#ifndef COMMANDPATTERN_TICKETMUTEX_H
#define COMMANDPATTERN_TICKETMUTEX_H

#include <atomic>
#include <mutex>

struct TicketMutex
{
    void lock()
    {
        const auto my = in.fetch_add(1, std::memory_order_acquire);
        while (true) {
            const auto now = out.load(std::memory_order_acquire);
            if (my == now) {
                return;
            }

            out.wait(now, std::memory_order_relaxed);
        }
    }

    void unlock()
    {
        out.fetch_add(1, std::memory_order_release);
        out.notify_all();
    }

private:
    alignas(std::hardware_destructive_interference_size) std::atomic<int> in = ATOMIC_VAR_INIT(0);
    alignas(std::hardware_destructive_interference_size) std::atomic<int> out = ATOMIC_VAR_INIT(0);
};


#endif //COMMANDPATTERN_TICKETMUTEX_H
