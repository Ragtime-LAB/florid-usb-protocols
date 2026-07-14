#ifndef FLORID_PROTOCOLS_ARMSTATUS_HPP
#define FLORID_PROTOCOLS_ARMSTATUS_HPP

#include <cstdint>
#include <array>
#include <RPL/Meta/PacketTraits.hpp>
#include <ProtocolDispatch.hpp>

#pragma pack(push, 1)

namespace florid::usb
{

enum class ArmMode : uint8_t
{
    INIT = 0,
    IDLE = 1,
    RUNNING = 2,
    FAULT = 3,
    ESTOP = 4,
};

struct JointStatus
{
    std::array<float, 6> q;
    std::array<float, 6> dq;
    std::array<float, 6> tau;
};

struct GripperStatus
{
    float q;
    float dq;
    float tau;
    float temp_c;
    uint8_t enabled;
    uint8_t _pad[3];
};

struct ArmStatus
{
    ArmMode mode;
    uint8_t _pad0[3]{};
    uint32_t seq{};
    uint64_t timestamp_us{};
    JointStatus status;
    float base_gravity[3];
    GripperStatus gripper;
};

// ──────────────────────────────────────────────
//  Per-motor telemetry broadcast
// ──────────────────────────────────────────────


    struct MotorFeedback
    {
        uint8_t joint_id : 3;
        uint8_t device_status : 4; // 0=ok, 1=warn, 2=error
        uint8_t enabled : 1;
        float position_rad;
        float speed_rad_s;
        float torque_nm;
        float temp_c;
    };

    struct MotorFeedbackArray
    {
        MotorFeedback motors[7];
    };

} // namespace florid::usb

namespace florid::usb
{
    namespace detail
    {
        template <typename T>
        inline void dispatch_status_after_parse(const T& pkt)
        {
            if (ProtocolDispatch::on_packet())
            {
                ProtocolDispatch::on_packet()(
                    RPL::Meta::PacketTraits<T>::cmd,
                    &pkt,
                    sizeof(T));
            }
        }
    } // namespace detail
} // namespace florid::usb

template <>
struct RPL::Meta::PacketTraits<florid::usb::ArmStatus>
    : PacketTraitsBase<PacketTraits<florid::usb::ArmStatus>>
{
    static constexpr uint16_t cmd = 0x6001;
    static constexpr size_t size = sizeof(florid::usb::ArmStatus);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::ArmStatus& pkt)
    {
        florid::usb::detail::dispatch_status_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::MotorFeedbackArray>
    : PacketTraitsBase<PacketTraits<florid::usb::MotorFeedbackArray>>
{
    static constexpr uint16_t cmd = 0x6002;
    static constexpr size_t size = sizeof(florid::usb::MotorFeedbackArray);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::MotorFeedbackArray& pkt)
    {
        florid::usb::detail::dispatch_status_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::GripperStatus>
    : PacketTraitsBase<PacketTraits<florid::usb::GripperStatus>>
{
    static constexpr uint16_t cmd = 0x6003;
    static constexpr size_t size = sizeof(florid::usb::GripperStatus);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::GripperStatus& pkt)
    {
        florid::usb::detail::dispatch_status_after_parse(pkt);
    }
};

#pragma pack(pop)

#endif // FLORID_PROTOCOLS_ARMSTATUS_HPP
