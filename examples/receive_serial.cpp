#include <ArmCommand.hpp>
#include <ArmStatus.hpp>
#include <ProtocolStack.hpp>

#include <astrial.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <atomic>

using namespace florid::usb;
using namespace std::chrono_literals;

using DeviceStack = ProtocolStack<std::chrono::steady_clock,
                                  AckPacket,
                                  SetMitLimitRequestPacket, SetMitLimitResponsePacket>;

static DeviceStack* g_stack_ptr = nullptr;
static Serial* g_serial = nullptr;

static bool send_over_serial(const uint8_t* frame, size_t len)
{
    if (!g_serial) return false;
    return g_serial->write(std::span(frame, len)).has_value();
}

static void handle_set_mit_limit(const uint16_t, const TransactionId tx_id,
                                  const uint8_t* data, const size_t size)
{
    if (size != sizeof(SetMitLimitRequestPacket)) return;
    SetMitLimitRequestPacket req{};
    std::memcpy(&req, data, sizeof(req));

    std::cout << "[device] SetMitLimit joint=" << static_cast<int>(req.payload.joint_id)
        << " tq=" << req.payload.torque_limit_nm << " vel=" << req.payload.velocity_limit_rps << "\n";

    SetMitLimitResponsePacket rsp{};
    rsp.tx_id = tx_id;
    rsp.payload.status = (req.payload.joint_id < 6)
        ? SetMitLimitStatus::Ok : SetMitLimitStatus::InvalidJoint;

    g_stack_ptr->send_response_and_cache<SetMitLimitRequestPacket, SetMitLimitResponsePacket>(tx_id, rsp);
}

int main(int argc, char** argv)
{
    const std::string port = (argc > 1) ? argv[1] : "/tmp/ttyV0";
    std::cout << "[device] opening " << port << "\n";

    DeviceStack::Config cfg{};
    cfg.session.window_size = 1;
    cfg.session.timeout = 500ms;
    cfg.session.max_retries = 3;
    cfg.session.device_cache_capacity = 64;

    DeviceStack stack(cfg);
    g_stack_ptr = &stack;

    stack.register_handler(RPL::Meta::PacketTraits<SetMitLimitRequestPacket>::cmd, handle_set_mit_limit);

    auto result = Serial::builder()
        .buad_rate(115200)
        .parity(Parity::None)
        .stop_bits(StopBits::One)
        .open(port);
    if (!result) throw std::runtime_error(std::string("[device] open: ") + result.error().message);

    auto serial = std::make_optional(std::move(result.value()));
    g_serial = &serial.value();
    serial->on_data([&stack](std::span<const uint8_t> data)
    {
        stack.isr_feed(data.data(), data.size());
    });
    stack.set_send_frame(send_over_serial);

    std::cout << "[device] serial ready, waiting for requests ...\n";

    while (true) { stack.run_one_cycle(); std::this_thread::sleep_for(10ms); }
}
