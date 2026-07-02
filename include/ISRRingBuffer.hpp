#ifndef FLORID_PROTOCOLS_ISRRINGBUFFER_HPP
#define FLORID_PROTOCOLS_ISRRINGBUFFER_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace florid::usb
{

template <size_t Capacity>
class ISRRingBuffer
{
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two");

    static constexpr size_t kMask = Capacity - 1;

    std::array<uint8_t, Capacity> m_buf{};
    std::atomic<size_t> m_write_idx{0};
    std::atomic<size_t> m_drop_count{0};
    size_t m_read_idx = 0;

public:
    ISRRingBuffer() = default;

    ISRRingBuffer(const ISRRingBuffer&) = delete;
    ISRRingBuffer& operator=(const ISRRingBuffer&) = delete;

    bool write(const uint8_t* data, size_t len) noexcept
    {
        if (len > Capacity)
        {
            m_drop_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        const size_t w = m_write_idx.load(std::memory_order_relaxed);
        const size_t avail = Capacity - (w - m_read_idx);
        if (len > avail)
        {
            m_drop_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        const size_t wi = w & kMask;
        const size_t first = Capacity - wi;
        if (len <= first)
        {
            std::memcpy(m_buf.data() + wi, data, len);
        }
        else
        {
            std::memcpy(m_buf.data() + wi, data, first);
            std::memcpy(m_buf.data(), data + first, len - first);
        }

        m_write_idx.store(w + len, std::memory_order_release);
        return true;
    }

    size_t available() const noexcept
    {
        return m_write_idx.load(std::memory_order_acquire) - m_read_idx;
    }

    size_t dropped_writes() const noexcept
    {
        return m_drop_count.load(std::memory_order_relaxed);
    }

    size_t read(uint8_t* out, size_t max) noexcept
    {
        const size_t avail = available();
        if (avail == 0 || max == 0)
        {
            return 0;
        }

        const size_t n = (max < avail) ? max : avail;
        const size_t ri = m_read_idx & kMask;
        const size_t first = Capacity - ri;
        if (n <= first)
        {
            std::memcpy(out, m_buf.data() + ri, n);
        }
        else
        {
            std::memcpy(out, m_buf.data() + ri, first);
            std::memcpy(out + first, m_buf.data(), n - first);
        }

        m_read_idx += n;
        return n;
    }

    void clear() noexcept
    {
        m_write_idx.store(0, std::memory_order_release);
        m_drop_count.store(0, std::memory_order_release);
        m_read_idx = 0;
    }
};

} // namespace florid::usb

#endif // FLORID_PROTOCOLS_ISRRINGBUFFER_HPP
