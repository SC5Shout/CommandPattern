//
// Created by michal on 21.07.2022.
//

#ifndef COMMANDPATTERN_RINGBUFFER_H
#define COMMANDPATTERN_RINGBUFFER_H

#include <cstdlib>

static constexpr size_t OBJECT_ALIGNMENT = alignof(std::max_align_t);
[[nodiscard]] static constexpr size_t align(size_t v)
{
    return (v + (OBJECT_ALIGNMENT - 1)) & -OBJECT_ALIGNMENT;
}

struct RingBuffer
{
    static constexpr size_t BLOCK_BITS = 12;
    static constexpr size_t BLOCK_SIZE = 1 << BLOCK_BITS;
    static constexpr size_t BLOCK_MASK = BLOCK_SIZE - 1;

    inline explicit RingBuffer(size_t bufferSize)
    {
        data = malloc(2 * bufferSize);
        mSize = bufferSize;
        tail = data;
        head = data;
    }

    RingBuffer(RingBuffer const& rhs) = delete;
    RingBuffer(RingBuffer&& rhs) = delete;
    RingBuffer& operator=(RingBuffer const& rhs) = delete;
    RingBuffer& operator=(RingBuffer&& rhs) = delete;

    inline ~RingBuffer()
    {
        free(data);
        data = nullptr;
    }

    [[nodiscard]] inline void* Allocate(size_t size)
    {
        char* const cur = static_cast<char*>(head);
        head = cur + size;
        return cur;
    }

    template<typename T>
    [[nodiscard]] inline T* Allocate(size_t count = 1, bool aligned = true)
    {
        return aligned ? (T*)Allocate(align(sizeof(T)) * count) : (T*)Allocate(sizeof(T) * count);
    }

    template<typename U, typename ... Args>
    inline void Construct(U* mem, Args&& ... args)
    {
        new(mem) U(std::forward<Args>(args)...);
    }

    [[nodiscard]] inline size_t max_size() const
    {
        return mSize;
    }
    [[nodiscard]] inline size_t size() const
    {
        return uintptr_t(head) - uintptr_t(tail);
    }

    [[nodiscard]] inline bool empty() const
    {
        return tail == head;
    }

    [[nodiscard]] inline void* getHead() const
    {
        return head;
    }
    [[nodiscard]] inline void* getTail() const
    {
        return tail;
    }

    struct Range
    {
        void* const begin = nullptr;
        void* const end = nullptr;
        [[nodiscard]] inline size_t size() const
        {
            return uintptr_t(end) - uintptr_t(begin);
        }
    };

    [[nodiscard]] inline Range getRange() const
    {
        return { tail, head };
    }

    inline void Circularize()
    {
        if(intptr_t(head) - intptr_t(data) > intptr_t(mSize)) {
            head = data;
        }
        tail = head;
    }

private:
    size_t mSize = 0;

    void* data = nullptr;
    void* tail = nullptr;
    void* head = nullptr;
};

#endif //COMMANDPATTERN_RINGBUFFER_H
