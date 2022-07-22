//
// Created by michal on 21.07.2022.
//

#ifndef COMMANDPATTERN_COMMANDQUEUE_H
#define COMMANDPATTERN_COMMANDQUEUE_H

#include <tuple>
#include <vector>
#include <condition_variable>

#include "TicketMutex.h"
#include "RingBuffer.h"

//zamiast usigned long long powinno być size_t ale kompilator wywala jakimś błędem głupim
[[nodiscard]] static constexpr size_t operator ""_MB(unsigned long long n)
{
    return n * 1024 * 1024;
}

struct CommandBase
{
    typedef void(*ExecuteFn)(CommandBase* self, uintptr_t& next);

    inline explicit CommandBase(ExecuteFn fn)
            : fn(fn)
    {
    }

    virtual ~CommandBase() = default;

    [[nodiscard]] inline CommandBase* Execute()
    {
        uintptr_t next = 0;
        fn(this, next);
        return reinterpret_cast<CommandBase*>(reinterpret_cast<uintptr_t>(this) + next);
    }

private:
    ExecuteFn fn;
};

template<typename FuncT, typename ... Args>
struct CustomCommand : CommandBase
{
    inline explicit CustomCommand(FuncT&& func, Args&& ... args)
            : CommandBase(Execute), func(std::forward<FuncT>(func)), args(std::forward<Args>(args)...)
    {
    }

    ~CustomCommand() override = default;

    static inline void Execute(CommandBase* base, uintptr_t& next)
    {
        next = align(sizeof(CustomCommand));
        auto b = static_cast<CustomCommand*>(base);
        std::apply(std::forward<FuncT>(b->func), std::move(b->args));
        b->~CustomCommand();
    }

private:
    FuncT func;
    std::tuple<std::remove_reference_t<Args>...> args;
};

struct NoopCommand : CommandBase
{
    inline explicit NoopCommand(void* next)
            : CommandBase(Execute), next(size_t((char*)next - (char*)this))
    {
    }

    ~NoopCommand() override = default;

    static inline void Execute(CommandBase* self, uintptr_t& next)
    {
        next = dynamic_cast<NoopCommand*>(self)->next;
    }

private:
    uintptr_t next = 0;
};

struct CommandQueue
{
    inline explicit CommandQueue(bool threadDispatch = true)
            : buffer(BUFFER_SIZE), THREAD_DISPATCH(threadDispatch)
    {
    }

    ~CommandQueue() = default;

    template<typename FuncT, typename ... Args>
    inline void Submit(FuncT&& funcT, Args&& ... args)
    {
        AllocateCommand<CustomCommand<FuncT, Args...>>(std::forward<FuncT>(funcT), std::forward<Args>(args)...);
    }

    template<typename Class, typename ... Args>
    inline void AllocateCommand(Args&& ... args)
    {
        auto buff = buffer.Allocate<Class>();
        buffer.Construct(buff, std::forward<Args>(args)...);
    }

    [[nodiscard]] std::vector<RingBuffer::Range> WaitForCommands() const;

    void Flush();
    [[nodiscard]] bool Execute();
    void ReleaseRange(const RingBuffer::Range& range);
    void Quit();

    mutable TicketMutex mutex;
    mutable std::condition_variable_any cv;
    mutable std::vector<RingBuffer::Range> commandsToExecute;

    RingBuffer buffer;

    static constexpr uint32_t REQUIRED_SIZE = (uint32_t)1_MB;
    static constexpr uint32_t BUFFER_SIZE = (uint32_t)3_MB;
    const bool THREAD_DISPATCH = true;

    size_t requiredSize = (REQUIRED_SIZE + RingBuffer::BLOCK_MASK) & ~RingBuffer::BLOCK_MASK;
    size_t freeSpace = BUFFER_SIZE;

    std::atomic_bool quit{ false };
};

#endif //COMMANDPATTERN_COMMANDQUEUE_H
