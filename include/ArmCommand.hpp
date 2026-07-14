#ifndef FLORID_PROTOCOLS_ARMCOMMAND_HPP
#define FLORID_PROTOCOLS_ARMCOMMAND_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#include <ProtocolDispatch.hpp>
#include <ArmStatus.hpp>

#pragma pack(push, 1)

namespace florid::usb
{
    using TransactionId = uint32_t;

    // ──────────────────────────────────────────────
    //  Enums
    // ──────────────────────────────────────────────

    enum class SetMitLimitStatus : uint8_t
    {
        Ok = 0,
        InvalidJoint = 1,
        StorageFailed = 2,
    };

    enum class MotorStateValue : uint8_t
    {
        Disable = 0,
        Enable = 1,
    };

    enum class SetMotorStateStatus : uint8_t
    {
        Ok = 0,
        UnsupportedInCurrentMode = 1,
        InvalidJoint = 2,
        StorageFailed = 3,
    };

    enum class MotorControlMode : uint8_t
    {
        Mit = 1,
        PosVel = 2,
        Vel = 3,
        Hybrid = 4,
    };

    enum class SetMotorControlModeStatus : uint8_t
    {
        Ok = 0,
        UnsupportedInCurrentMode = 1,
        InvalidJoint = 2,
        InvalidMode = 3,
        CanTxFailed = 4,
    };

    enum class SetZeroStatus : uint8_t
    {
        Ok = 0,
        InvalidJoint = 1,
        CalibrationFailed = 2,
    };

    enum class ClearErrorStatus : uint8_t
    {
        Ok = 0,
        InvalidJoint = 1,
    };

    enum class HomeAllStatus : uint8_t
    {
        Ok = 0,
        NotIdle = 1,
        CalibrationFailed = 2,
    };

    enum class CalibrationType : uint8_t
    {
        Home = 0,
        Zero = 1,
    };

    enum class RunCalibrationStatus : uint8_t
    {
        Ok = 0,
        NotIdle = 1,
        CalibrationFailed = 2,
    };

    enum class ClearFaultsStatus : uint8_t
    {
        Ok = 0,
    };

    enum class CollisionSensitivity : uint8_t
    {
        Low = 0,
        Medium = 1,
        High = 2,
    };

    enum class SetSafetyStatus : uint8_t
    {
        Ok = 0,
        StorageFailed = 1,
    };

    enum class SdkClientNotifyStatus : uint8_t
    {
        Ok = 0,
    };

    enum class HomeDoneStatus : uint8_t
    {
        Ok = 0,
    };

    // ──────────────────────────────────────────────
    //  Command payloads (request body)
    // ──────────────────────────────────────────────

    struct SetMitLimitCommand
    {
        uint8_t joint_id;
        float torque_limit_nm;
        float velocity_limit_rps;
    };

    struct SetMotorStateCommand
    {
        uint8_t joint_id;
        MotorStateValue state;
    };

    struct SetMotorControlModeCommand
    {
        uint8_t joint_id;
        MotorControlMode mode;
    };

    struct SetZeroCommand
    {
        uint8_t joint_id;
    };

    struct ClearErrorCommand
    {
        uint8_t joint_id;
    };

    struct HomeAllCommand
    {
        uint8_t dummy;
    };

    struct RunCalibrationCommand
    {
        CalibrationType calib_type;
    };

    struct ClearFaultsCommand
    {
        uint8_t dummy;
    };

    struct SetSafetyCommand
    {
        float e_stop_deceleration_dps2;
        float max_payload_kg;
    };

    struct SdkClientConnectedCommand
    {
        uint8_t dummy;
    };

    struct SdkClientDisconnectedCommand
    {
        uint8_t dummy;
    };

    struct HomeDoneCommand
    {
        uint8_t dummy;
    };

    // ──────────────────────────────────────────────
    //  Response payloads
    // ──────────────────────────────────────────────

    struct SetMitLimitResponse
    {
        SetMitLimitStatus status;
    };

    struct SetMotorStateResponse
    {
        SetMotorStateStatus status;
    };

    struct SetMotorControlModeResponse
    {
        SetMotorControlModeStatus status;
    };

    struct SetZeroResponse
    {
        SetZeroStatus status;
    };

    struct ClearErrorResponse
    {
        ClearErrorStatus status;
    };

    struct HomeAllResponse
    {
        HomeAllStatus status;
    };

    struct RunCalibrationResponse
    {
        RunCalibrationStatus status;
    };

    struct ClearFaultsResponse
    {
        ClearFaultsStatus status;
    };

    struct SetSafetyResponse
    {
        SetSafetyStatus status;
    };

    struct SdkClientNotifyResponse
    {
        SdkClientNotifyStatus status;
    };

    struct HomeDoneResponse
    {
        HomeDoneStatus status;
    };

    // ──────────────────────────────────────────────
    //  Request packets  (host → firmware)
    // ──────────────────────────────────────────────

    struct AckPacket
    {
        TransactionId tx_id;
        uint16_t cmd_id;
        uint8_t status;
    };

    struct SetMitLimitRequestPacket
    {
        TransactionId tx_id;
        SetMitLimitCommand payload;
    };

    struct SetMotorStateRequestPacket
    {
        TransactionId tx_id;
        SetMotorStateCommand payload;
    };

    struct SetMotorControlModeRequestPacket
    {
        TransactionId tx_id;
        SetMotorControlModeCommand payload;
    };

    struct SetZeroRequestPacket
    {
        TransactionId tx_id;
        SetZeroCommand payload;
    };

    struct ClearErrorRequestPacket
    {
        TransactionId tx_id;
        ClearErrorCommand payload;
    };

    struct HomeAllRequestPacket
    {
        TransactionId tx_id;
        HomeAllCommand payload;
    };

    struct RunCalibrationRequestPacket
    {
        TransactionId tx_id;
        RunCalibrationCommand payload;
    };

    struct ClearFaultsRequestPacket
    {
        TransactionId tx_id;
        ClearFaultsCommand payload;
    };

    struct SetSafetyRequestPacket
    {
        TransactionId tx_id;
        SetSafetyCommand payload;
    };

    struct SdkClientConnectedRequestPacket
    {
        TransactionId tx_id;
        SdkClientConnectedCommand payload;
    };

    struct SdkClientDisconnectedRequestPacket
    {
        TransactionId tx_id;
        SdkClientDisconnectedCommand payload;
    };

    struct HomeDoneRequestPacket
    {
        TransactionId tx_id;
        HomeDoneCommand payload;
    };

    // ──────────────────────────────────────────────
    //  Response packets  (firmware → host)
    // ──────────────────────────────────────────────

    struct SetMitLimitResponsePacket
    {
        TransactionId tx_id;
        SetMitLimitResponse payload;
    };

    struct SetMotorStateResponsePacket
    {
        TransactionId tx_id;
        SetMotorStateResponse payload;
    };

    struct SetMotorControlModeResponsePacket
    {
        TransactionId tx_id;
        SetMotorControlModeResponse payload;
    };

    struct SetZeroResponsePacket
    {
        TransactionId tx_id;
        SetZeroResponse payload;
    };

    struct ClearErrorResponsePacket
    {
        TransactionId tx_id;
        ClearErrorResponse payload;
    };

    struct HomeAllResponsePacket
    {
        TransactionId tx_id;
        HomeAllResponse payload;
    };

    struct RunCalibrationResponsePacket
    {
        TransactionId tx_id;
        RunCalibrationResponse payload;
    };

    struct ClearFaultsResponsePacket
    {
        TransactionId tx_id;
        ClearFaultsResponse payload;
    };

    struct SetSafetyResponsePacket
    {
        TransactionId tx_id;
        SetSafetyResponse payload;
    };

    struct SdkClientConnectedResponsePacket
    {
        TransactionId tx_id;
        SdkClientNotifyResponse payload;
    };

    struct SdkClientDisconnectedResponsePacket
    {
        TransactionId tx_id;
        SdkClientNotifyResponse payload;
    };

    struct HomeDoneResponsePacket
    {
        TransactionId tx_id;
        HomeDoneResponse payload;
    };

    // ──────────────────────────────────────────────
    //  USB Session protocol
    // ──────────────────────────────────────────────

    enum class UsbSessionStartStatus : uint8_t
    {
        Ok = 0,
    };

    enum class UsbSessionStopStatus : uint8_t
    {
        Ok = 0,
    };

    struct UsbSessionStartCommand
    {
        uint8_t dummy;
    };

    struct UsbSessionStopCommand
    {
        uint8_t dummy;
    };

    struct UsbSessionStartResponse
    {
        UsbSessionStartStatus status;
    };

    struct UsbSessionStopResponse
    {
        UsbSessionStopStatus status;
    };

    struct UsbSessionStartRequestPacket
    {
        TransactionId tx_id;
        UsbSessionStartCommand payload;
    };

    struct UsbSessionStartResponsePacket
    {
        TransactionId tx_id;
        UsbSessionStartResponse payload;
    };

    struct UsbSessionStopRequestPacket
    {
        TransactionId tx_id;
        UsbSessionStopCommand payload;
    };

    struct UsbSessionStopResponsePacket
    {
        TransactionId tx_id;
        UsbSessionStopResponse payload;
    };

    // ──────────────────────────────────────────────
    //  Real-time control  (fire-and-forget, no tx_id)
    // ──────────────────────────────────────────────

    struct JointCommandPacket
    {
        float q[6];
        float dq[6];
        float tau[6];
        float kp[6];
        float kd[6];
        uint32_t dt_us;
        uint16_t seq;
        uint8_t control_mode; // bit[1:0]=type(0=hold,1=MIT,3=torque), bit[2]=gravity_enable
    };

    struct JointPosVelCommandPacket
    {
        float q[6];
        float dq[6];
        uint8_t enabled_mask;
        uint16_t seq;
    };

    struct JointVelocityCommandPacket
    {
        float dq[6];
        uint8_t enabled_mask;
        uint16_t seq;
    };

    struct JointHybridCommandPacket
    {
        float q[6];
        float dq_limit[6];
        float current_limit_norm[6];
        uint8_t enabled_mask;
        uint16_t seq;
    };

    struct EmergencyStopPacket
    {
        uint8_t dummy;
    };

    struct GripperCommandPacket
    {
        float q;
        float dq;
        float tau;
        float kp;
        float kd;
        uint32_t dt_us;
        uint16_t seq;
        uint8_t control_mode;
    };

    // ──────────────────────────────────────────────
    //  Motor feedback (request-response, replaces broadcast)
    // ──────────────────────────────────────────────

    struct GetMotorFeedbackCommand
    {
        uint8_t dummy;
    };

    struct GetMotorFeedbackResponse
    {
        MotorFeedbackArray motors;
    };

    struct GetMotorFeedbackRequestPacket
    {
        TransactionId tx_id;
        GetMotorFeedbackCommand payload;
    };

    struct GetMotorFeedbackResponsePacket
    {
        TransactionId tx_id;
        GetMotorFeedbackResponse payload;
    };
} // namespace florid::usb

// ══════════════════════════════════════════════
//  RPL PacketTraits specialisations
// ══════════════════════════════════════════════

namespace florid::usb
{
    namespace detail
    {
        template <typename T>
        inline void dispatch_after_parse(const T& pkt)
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
struct RPL::Meta::PacketTraits<florid::usb::AckPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::AckPacket>>
{
    static constexpr uint16_t cmd = 0x6FF0;
    static constexpr size_t size = sizeof(florid::usb::AckPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::AckPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMitLimitRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMitLimitRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6103;
    static constexpr size_t size = sizeof(florid::usb::SetMitLimitRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMitLimitRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMitLimitResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMitLimitResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6104;
    static constexpr size_t size = sizeof(florid::usb::SetMitLimitResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMitLimitResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMotorStateRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMotorStateRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6105;
    static constexpr size_t size = sizeof(florid::usb::SetMotorStateRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMotorStateRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMotorStateResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMotorStateResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6106;
    static constexpr size_t size = sizeof(florid::usb::SetMotorStateResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMotorStateResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMotorControlModeRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMotorControlModeRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6215;
    static constexpr size_t size = sizeof(florid::usb::SetMotorControlModeRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMotorControlModeRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetMotorControlModeResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetMotorControlModeResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6216;
    static constexpr size_t size = sizeof(florid::usb::SetMotorControlModeResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetMotorControlModeResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetZeroRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetZeroRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6107;
    static constexpr size_t size = sizeof(florid::usb::SetZeroRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetZeroRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetZeroResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetZeroResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6108;
    static constexpr size_t size = sizeof(florid::usb::SetZeroResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetZeroResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::ClearErrorRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::ClearErrorRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6109;
    static constexpr size_t size = sizeof(florid::usb::ClearErrorRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::ClearErrorRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::ClearErrorResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::ClearErrorResponsePacket>>
{
    static constexpr uint16_t cmd = 0x610A;
    static constexpr size_t size = sizeof(florid::usb::ClearErrorResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::ClearErrorResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::HomeAllRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::HomeAllRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6201;
    static constexpr size_t size = sizeof(florid::usb::HomeAllRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::HomeAllRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::HomeAllResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::HomeAllResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6202;
    static constexpr size_t size = sizeof(florid::usb::HomeAllResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::HomeAllResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::RunCalibrationRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::RunCalibrationRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6203;
    static constexpr size_t size = sizeof(florid::usb::RunCalibrationRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::RunCalibrationRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::RunCalibrationResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::RunCalibrationResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6204;
    static constexpr size_t size = sizeof(florid::usb::RunCalibrationResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::RunCalibrationResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::ClearFaultsRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::ClearFaultsRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6205;
    static constexpr size_t size = sizeof(florid::usb::ClearFaultsRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::ClearFaultsRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::ClearFaultsResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::ClearFaultsResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6206;
    static constexpr size_t size = sizeof(florid::usb::ClearFaultsResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::ClearFaultsResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetSafetyRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetSafetyRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6207;
    static constexpr size_t size = sizeof(florid::usb::SetSafetyRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetSafetyRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SetSafetyResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SetSafetyResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6208;
    static constexpr size_t size = sizeof(florid::usb::SetSafetyResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SetSafetyResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SdkClientConnectedRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SdkClientConnectedRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6209;
    static constexpr size_t size = sizeof(florid::usb::SdkClientConnectedRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SdkClientConnectedRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SdkClientConnectedResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SdkClientConnectedResponsePacket>>
{
    static constexpr uint16_t cmd = 0x620A;
    static constexpr size_t size = sizeof(florid::usb::SdkClientConnectedResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SdkClientConnectedResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SdkClientDisconnectedRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SdkClientDisconnectedRequestPacket>>
{
    static constexpr uint16_t cmd = 0x620B;
    static constexpr size_t size = sizeof(florid::usb::SdkClientDisconnectedRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SdkClientDisconnectedRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::SdkClientDisconnectedResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::SdkClientDisconnectedResponsePacket>>
{
    static constexpr uint16_t cmd = 0x620C;
    static constexpr size_t size = sizeof(florid::usb::SdkClientDisconnectedResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::SdkClientDisconnectedResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::HomeDoneRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::HomeDoneRequestPacket>>
{
    static constexpr uint16_t cmd = 0x620D;
    static constexpr size_t size = sizeof(florid::usb::HomeDoneRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::HomeDoneRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::HomeDoneResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::HomeDoneResponsePacket>>
{
    static constexpr uint16_t cmd = 0x620E;
    static constexpr size_t size = sizeof(florid::usb::HomeDoneResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::HomeDoneResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::UsbSessionStartRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::UsbSessionStartRequestPacket>>
{
    static constexpr uint16_t cmd = 0x620F;
    static constexpr size_t size = sizeof(florid::usb::UsbSessionStartRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::UsbSessionStartRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::UsbSessionStartResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::UsbSessionStartResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6210;
    static constexpr size_t size = sizeof(florid::usb::UsbSessionStartResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::UsbSessionStartResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::UsbSessionStopRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::UsbSessionStopRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6211;
    static constexpr size_t size = sizeof(florid::usb::UsbSessionStopRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::UsbSessionStopRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::UsbSessionStopResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::UsbSessionStopResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6212;
    static constexpr size_t size = sizeof(florid::usb::UsbSessionStopResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::UsbSessionStopResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::JointCommandPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::JointCommandPacket>>
{
    static constexpr uint16_t cmd = 0x6301;
    static constexpr size_t size = sizeof(florid::usb::JointCommandPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::JointCommandPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::EmergencyStopPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::EmergencyStopPacket>>
{
    static constexpr uint16_t cmd = 0x6303;
    static constexpr size_t size = sizeof(florid::usb::EmergencyStopPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::EmergencyStopPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::GripperCommandPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::GripperCommandPacket>>
{
    static constexpr uint16_t cmd = 0x6305;
    static constexpr size_t size = sizeof(florid::usb::GripperCommandPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::GripperCommandPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::JointPosVelCommandPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::JointPosVelCommandPacket>>
{
    static constexpr uint16_t cmd = 0x6306;
    static constexpr size_t size = sizeof(florid::usb::JointPosVelCommandPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::JointPosVelCommandPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::JointVelocityCommandPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::JointVelocityCommandPacket>>
{
    static constexpr uint16_t cmd = 0x6307;
    static constexpr size_t size = sizeof(florid::usb::JointVelocityCommandPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::JointVelocityCommandPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::JointHybridCommandPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::JointHybridCommandPacket>>
{
    static constexpr uint16_t cmd = 0x6308;
    static constexpr size_t size = sizeof(florid::usb::JointHybridCommandPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::JointHybridCommandPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::GetMotorFeedbackRequestPacket>
    : PacketTraitsBase<PacketTraits<florid::usb::GetMotorFeedbackRequestPacket>>
{
    static constexpr uint16_t cmd = 0x6213;
    static constexpr size_t size = sizeof(florid::usb::GetMotorFeedbackRequestPacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::GetMotorFeedbackRequestPacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

template <>
struct RPL::Meta::PacketTraits<florid::usb::GetMotorFeedbackResponsePacket>
    : PacketTraitsBase<PacketTraits<florid::usb::GetMotorFeedbackResponsePacket>>
{
    static constexpr uint16_t cmd = 0x6214;
    static constexpr size_t size = sizeof(florid::usb::GetMotorFeedbackResponsePacket);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const florid::usb::GetMotorFeedbackResponsePacket& pkt)
    {
        florid::usb::detail::dispatch_after_parse(pkt);
    }
};

#pragma pack(pop)

#endif // FLORID_PROTOCOLS_ARMCOMMAND_HPP
