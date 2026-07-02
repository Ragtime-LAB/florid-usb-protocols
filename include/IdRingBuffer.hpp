#ifndef FLORID_PROTOCOLS_IDRINGBUFFER_HPP
#define FLORID_PROTOCOLS_IDRINGBUFFER_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace florid::usb
{
    template <typename T, size_t Capacity>
    struct IdRingBuffer
    {
        static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                      "Capacity must be a power of two");

        std::array<T, Capacity> ids{};
        size_t write_idx = 0;

        bool test_and_insert(const T id) noexcept
        {
            for (size_t i = 0; i < Capacity; ++i)
            {
                if (ids[i] == id)
                {
                    return true;
                }
            }
            ids[write_idx] = id;
            write_idx = (write_idx + 1) & (Capacity - 1);
            return false;
        }

        void clear() noexcept
        {
            ids.fill(T{});
            write_idx = 0;
        }

        size_t size() const noexcept
        {
            return Capacity;
        }
    };

    using DedupKey = uint64_t;

    inline DedupKey compose_dedup_key(const uint16_t cmd_id, const uint32_t tx_id) noexcept
    {
        return (static_cast<uint64_t>(cmd_id) << 32U) | static_cast<uint64_t>(tx_id);
    }

    using DedupRingBuffer = IdRingBuffer<DedupKey, 64>;

} // namespace florid::usb

#endif // FLORID_PROTOCOLS_IDRINGBUFFER_HPP
