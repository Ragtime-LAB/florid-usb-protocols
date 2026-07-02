#ifndef FLORID_PROTOCOLS_PROTOCOLDISPATCH_HPP
#define FLORID_PROTOCOLS_PROTOCOLDISPATCH_HPP

#include <cstddef>
#include <cstdint>

namespace florid::usb
{

struct ProtocolDispatch
{
    using OnPacketFn = void (*)(uint16_t cmd_id, const void* data, size_t size);

    static OnPacketFn& on_packet()
    {
        thread_local OnPacketFn fn = nullptr;
        return fn;
    }

    static void*& current_stack()
    {
        thread_local void* ptr = nullptr;
        return ptr;
    }
};

} // namespace florid::usb

#endif // FLORID_PROTOCOLS_PROTOCOLDISPATCH_HPP
