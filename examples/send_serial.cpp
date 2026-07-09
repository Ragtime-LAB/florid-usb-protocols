#include <ArmCommand.hpp>
#include <ArmStatus.hpp>
#include <ProtocolStack.hpp>
#include <ISRRingBuffer.hpp>

#include <astrial.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

using namespace florid::usb;
using namespace std::chrono_literals;

using HostStack = ProtocolStack<std::chrono::steady_clock,
                                AckPacket,
                                SetMitLimitRequestPacket, SetMitLimitResponsePacket>;

struct HostTransport
{
    std::optional<Serial> serial;
    HostStack stack;
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};

    HostTransport(const std::string& device)
        : stack([]()
        {
            HostStack::Config cfg{};
            cfg.session.window_size = 1;
            cfg.session.timeout = 500ms;
            cfg.session.max_retries = 3;
            return cfg;
        }())
    {
        s_instance = this;

        auto result = Serial::builder()
            .buad_rate(115200)
            .parity(Parity::None)
            .stop_bits(StopBits::One)
            .open(device);
        if (!result)
            throw std::runtime_error(std::string("[host] open: ") + result.error().message);
        serial = std::move(result.value());

        serial->on_data([this](std::span<const uint8_t> data)
        {
            stack.isr_feed(data.data(), data.size());
        });

        stack.set_send_frame([](const uint8_t* frame, size_t len) -> bool
        {
            if (!s_instance || !s_instance->serial.has_value()) return false;
            auto ec = s_instance->serial->write(std::span(frame, len));
            return ec.has_value();
        });
    }

    static inline HostTransport* s_instance = nullptr;
};

int main(int argc, char** argv)
{
    const std::string port = (argc > 1) ? argv[1] : "/dev/ttyACM1";
    std::cout << "[host] opening " << port << "\n";

    HostTransport t(port);
    std::cout << "[host] serial ready, sending SetMitLimit ...\n";

    t.stack.send<SetMitLimitRequestPacket, SetMitLimitResponsePacket>(
        SetMitLimitCommand{.joint_id = 2, .torque_limit_nm = 12.0F, .velocity_limit_rps = 6.28F},
        [&](const uint8_t* bytes, size_t size)
        {
            if (size == sizeof(SetMitLimitResponsePacket))
            {
                SetMitLimitResponsePacket rsp{};
                std::memcpy(&rsp, bytes, sizeof(rsp));
                t.success.store(rsp.payload.status == SetMitLimitStatus::Ok);
            }
            t.done.store(true);
        },
        [&](SessionError) { t.success.store(false); t.done.store(true); });

    for (int i = 0; i < 1000 && !t.done.load(); ++i)
    {
        t.stack.run_one_cycle();
        std::this_thread::sleep_for(10ms);
    }

    if (!t.done.load())  { std::cerr << "[host] timeout\n"; return 1; }
    if (!t.success.load()) { std::cerr << "[host] request failed\n"; return 1; }
    std::cout << "[host] SetMitLimit success\n";
    return 0;
}
