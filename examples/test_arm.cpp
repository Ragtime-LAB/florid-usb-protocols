#include <ArmCommand.hpp>
#include <ArmStatus.hpp>
#include <ProtocolStack.hpp>
#include <ISRRingBuffer.hpp>

#include <HySerial/HySerial.hpp>
#include <RPL/Serializer.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace florid::usb;
using namespace std::chrono_literals;

using HostStack = ProtocolStack<std::chrono::steady_clock,
    AckPacket,
    SetMitLimitRequestPacket, SetMitLimitResponsePacket,
    SetMotorStateRequestPacket, SetMotorStateResponsePacket,
    JointCommandPacket, EmergencyStopPacket,
    ArmStatus, MotorFeedbackArray>;

struct HostTransport
{
    std::unique_ptr<HySerial::Serial> serial;
    HostStack stack;
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    std::atomic<uint8_t> last_status{0};
    ArmStatus last_arm_status{};
    MotorFeedbackArray last_motor_fb{};
    std::atomic<bool> arm_status_received{false};
    std::atomic<bool> motor_fb_received{false};

    HostTransport(const std::string& device)
        : stack([]() {
            HostStack::Config cfg{};
            cfg.session.window_size = 1;
            cfg.session.timeout = 500ms;
            cfg.session.max_retries = 3;
            cfg.rx_ring_capacity = 4096;
            return cfg;
        }())
    {
        s_instance = this;

        HySerial::Builder b;
        b.device(device).baud_rate(115200)
         .data_bits(HySerial::DataBits::BITS_8)
         .parity(HySerial::Parity::NONE)
         .stop_bits(HySerial::StopBits::ONE);

        b.on_read([this](std::span<const std::byte> data) {
            stack.isr_feed(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        });
        b.on_error([](ssize_t e) { std::cerr << "[host] serial error: " << e << "\n"; });

        auto res = b.build();
        if (!res) throw std::runtime_error(std::string("[host] open: ") + res.error().message);
        serial = std::move(res.value());
        serial->start_read(4096);

        stack.set_send_frame([](const uint8_t* frame, size_t len) -> bool {
            if (!s_instance || !s_instance->serial) return false;
            s_instance->serial->send(std::span(reinterpret_cast<const std::byte*>(frame), len));
            return true;
        });

        stack.register_handler(RPL::Meta::PacketTraits<ArmStatus>::cmd,
            [this](uint16_t, TransactionId, const uint8_t* data, size_t size) {
                if (size >= sizeof(ArmStatus)) {
                    std::memcpy(&last_arm_status, data, sizeof(ArmStatus));
                    arm_status_received.store(true);
                }
            });
        stack.register_handler(RPL::Meta::PacketTraits<MotorFeedbackArray>::cmd,
            [this](uint16_t, TransactionId, const uint8_t* data, size_t size) {
                if (size >= sizeof(MotorFeedbackArray)) {
                    std::memcpy(&last_motor_fb, data, sizeof(MotorFeedbackArray));
                    motor_fb_received.store(true);
                }
            });
    }

    bool send_fire_and_forget(const auto& packet) {
        if (!serial) return false;
        using Pkt = std::decay_t<decltype(packet)>;
        RPL::Serializer<Pkt> ser;
        uint8_t buf[512];
        auto result = ser.serialize(buf, sizeof(buf), packet);
        if (!result.has_value()) return false;
        serial->send(std::span(reinterpret_cast<const std::byte*>(buf), *result));
        return true;
    }

    static inline HostTransport* s_instance = nullptr;
};

namespace {
std::atomic<bool> g_running{true};
void sig_handler(int) { g_running.store(false); }

void print_arm_status(const ArmStatus& s) {
    std::cout << std::fixed << std::setprecision(3)
              << "  ArmStatus [mode=" << static_cast<int>(s.mode) << "]"
              << "  q=[" << s.status.q[0] << "," << s.status.q[1] << "," << s.status.q[2]
              << "," << s.status.q[3] << "," << s.status.q[4] << "," << s.status.q[5] << "]"
              << "  tau=" << s.status.tau[0]
              << "  g=[" << s.base_gravity[0] << "," << s.base_gravity[1] << "," << s.base_gravity[2] << "]\n";
}
} // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, sig_handler);
    const std::string port = (argc > 1) ? argv[1] : "/dev/ttyACM0";
    std::cout << "[test] opening " << port << "\n";

    try {
        HostTransport t(port);
        std::cout << "[test] serial ready\n";

        // Enable J6
        {
            t.done.store(false); t.success.store(false);
            t.stack.send<SetMotorStateRequestPacket, SetMotorStateResponsePacket>(
                SetMotorStateCommand{.joint_id = 5, .state = MotorStateValue::Enable},
                [&](const uint8_t* bytes, size_t size) {
                    if (size == sizeof(SetMotorStateResponsePacket)) {
                        SetMotorStateResponsePacket rsp{};
                        std::memcpy(&rsp, bytes, sizeof(rsp));
                        t.last_status.store(static_cast<uint8_t>(rsp.payload.status));
                        t.success.store(rsp.payload.status == SetMotorStateStatus::Ok);
                    }
                    t.done.store(true);
                },
                [&](SessionError) { t.last_status.store(0xFF); t.done.store(true); });
            for (int i = 0; i < 200 && !t.done.load(); ++i) { t.stack.run_one_cycle(); std::this_thread::sleep_for(10ms); }
            std::cout << "  SetMotorState J6 -> enable: " << (t.success.load() ? "OK" : "FAIL") << "\n";
        }

        // Send JointCommand ramp on J6
        double start_q[6]{}, target_q[6]{};
        target_q[5] = 0.5;
        constexpr int kSteps = 200;
        for (int step = 0; step < kSteps && g_running.load(); ++step) {
            double frac = static_cast<double>(step) / kSteps;
            JointCommandPacket cmd{};
            cmd.control_mode = 0x01;
            for (int i = 0; i < 6; ++i)
                cmd.q[i] = static_cast<float>(start_q[i] + frac * (target_q[i] - start_q[i]));
            t.send_fire_and_forget(cmd);
            t.stack.run_one_cycle();
            std::this_thread::sleep_for(10ms);
            if (step % 10 == 0 && t.arm_status_received.load()) {
                print_arm_status(t.last_arm_status);
                t.arm_status_received.store(false);
            }
        }

        // Disable J6
        {
            t.done.store(false); t.success.store(false);
            t.stack.send<SetMotorStateRequestPacket, SetMotorStateResponsePacket>(
                SetMotorStateCommand{.joint_id = 5, .state = MotorStateValue::Disable},
                [&](const uint8_t* bytes, size_t size) {
                    if (size == sizeof(SetMotorStateResponsePacket)) {
                        SetMotorStateResponsePacket rsp{};
                        std::memcpy(&rsp, bytes, sizeof(rsp));
                        t.last_status.store(static_cast<uint8_t>(rsp.payload.status));
                        t.success.store(rsp.payload.status == SetMotorStateStatus::Ok);
                    }
                    t.done.store(true);
                },
                [&](SessionError) { t.last_status.store(0xFF); t.done.store(true); });
            for (int i = 0; i < 200 && !t.done.load(); ++i) { t.stack.run_one_cycle(); std::this_thread::sleep_for(10ms); }
            std::cout << "  SetMotorState J6 -> disable: " << (t.success.load() ? "OK" : "FAIL") << "\n";
        }

        std::cout << "[test] done.\n";
    } catch (const std::exception& e) {
        std::cerr << "[test] error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
