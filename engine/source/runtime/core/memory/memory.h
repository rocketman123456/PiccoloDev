#pragma once

#include <cstdint>
#include <cstddef>

namespace Piccolo
{
    struct MemoryStatistics
    {
        size_t allocated_bytes;
        size_t total_bytes;

        uint32_t allocation_count;

        void add(size_t a)
        {
            if (a)
            {
                allocated_bytes += a;
                ++allocation_count;
            }
        }

        void remove(size_t a)
        {
            if (a)
            {
                allocated_bytes -= a;
                // --allocation_count;
            }
        }
    }; // struct MemoryStatistics

    class Allocator
    {
    public:
        virtual ~Allocator() = default;

        virtual void* allocate(size_t size, size_t alignment)                                 = 0;
        virtual void* allocate(size_t size, size_t alignment, const char* file, int32_t line) = 0;

        virtual void deallocate(void* pointer) = 0;
    }; // struct Allocator

} // namespace Piccolo