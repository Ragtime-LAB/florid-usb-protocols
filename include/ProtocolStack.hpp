#ifndef FLORID_PROTOCOLS_PROTOCOLSTACK_HPP
#define FLORID_PROTOCOLS_PROTOCOLSTACK_HPP

#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Meta/PacketTraits.hpp>

#include <ISRRingBuffer.hpp>
#include <ReliableSession.hpp>
#include <ProtocolDispatch.hpp>
#include <ArmCommand.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <type_traits>

namespace florid::usb
{

template <typename Clock, typename... PacketTypes>
class ProtocolStack
{
public:
    using Session = ReliableSession<Clock>;
    using ErrorCallback = typename Session::ErrorCallback;
    using ResponseCallback = typename Session::ResponseCallback;
    using RequestResult = typename Session::RequestResult;

    using RequestHandler = std::function<void(uint16_t cmd_id, TransactionId tx_id,
                                              const uint8_t* request_bytes, size_t request_len)>;

    struct Config
    {
        typename Session::Config session{};
        size_t rx_ring_capacity = 4096;
        size_t tx_queue_capacity = 16;
        typename Clock::duration poll_interval{5000};
    };

    explicit ProtocolStack(const Config& config = Config{})
        : m_config(config), m_session(config.session), m_parser(m_deserializer)
    {
        ProtocolDispatch::on_packet() = +[](const uint16_t cmd_id, const void* data, const size_t size)
        {
            if (instance())
                instance()->handle_packet_internal(cmd_id,
                    static_cast<const uint8_t*>(data), size);
        };
        instance() = this;
    }

    ~ProtocolStack()
    {
        if (instance() == this)
        {
            instance() = nullptr;
            ProtocolDispatch::on_packet() = nullptr;
        }
    }

    ProtocolStack(const ProtocolStack&) = delete;
    ProtocolStack& operator=(const ProtocolStack&) = delete;

    // ──────────────────────────────────────────────
    //  Transport binding
    // ──────────────────────────────────────────────

    using SendFrameFn = std::function<bool(const uint8_t* frame, size_t len)>;
    void set_send_frame(SendFrameFn fn) { m_send_frame = fn; }

    void bind_to_current_thread()
    {
        ProtocolDispatch::on_packet() = +[](const uint16_t cmd_id, const void* data, const size_t size)
        {
            if (instance())
                instance()->handle_packet_internal(cmd_id,
                    static_cast<const uint8_t*>(data), size);
        };
        instance() = this;
    }

    // ──────────────────────────────────────────────
    //  ISR entry (called from USB/serial ISR)
    // ──────────────────────────────────────────────

    void isr_feed(const uint8_t* data, const size_t len)
    {
        m_rx_ring.write(data, len);
    }

    // ──────────────────────────────────────────────
    //  Zero-copy DMA receive (UART DMA integration)
    // ──────────────────────────────────────────────

    std::span<uint8_t> get_write_buffer() noexcept
    {
        return m_parser.get_write_buffer();
    }

    tl::expected<void, RPL::Error> advance_write_index(const size_t length)
    {
        return m_parser.advance_write_index(length);
    }

    // ──────────────────────────────────────────────
    //  Handler registry (device side)
    // ──────────────────────────────────────────────

    void register_handler(const uint16_t cmd_id, RequestHandler handler)
    {
        m_handlers.insert_or_assign(cmd_id, std::move(handler));
    }

    // ──────────────────────────────────────────────
    //  Host send — allocate + enqueue
    // ──────────────────────────────────────────────

    template <typename RequestPacket, typename ResponsePacket, typename Payload>
    TransactionId send(const Payload& payload,
                       ResponseCallback on_response,
                       ErrorCallback on_error,
                       const typename Clock::duration timeout = typename Clock::duration{200000},
                       const uint8_t max_retries = 3)
    {
        static_assert(std::is_standard_layout_v<RequestPacket>, "Request must be standard layout");
        static_assert(std::is_standard_layout_v<ResponsePacket>, "Response must be standard layout");

        using ReqTraits = RPL::Meta::PacketTraits<RequestPacket>;
        using RspTraits = RPL::Meta::PacketTraits<ResponsePacket>;

        const TransactionId tx_id = m_session.next_tx_id();

        RequestPacket request{};
        request.tx_id = tx_id;
        request.payload = payload;

        m_session.register_pending(
            tx_id, ReqTraits::cmd, RspTraits::cmd,
            reinterpret_cast<const uint8_t*>(&request), sizeof(RequestPacket),
            std::move(on_response), std::move(on_error), timeout, max_retries);

        return tx_id;
    }

    // ──────────────────────────────────────────────
    //  Device side — serialize response and cache
    // ──────────────────────────────────────────────

    template <typename RequestPacket, typename ResponsePacket>
    bool send_response_and_cache(const TransactionId tx_id, const ResponsePacket& response)
    {
        using ReqTraits = RPL::Meta::PacketTraits<RequestPacket>;
        using RspTraits = RPL::Meta::PacketTraits<ResponsePacket>;

        uint8_t frame_buf[MAX_FRAME_SIZE];
        size_t frame_len = 0;
        if (!serialize_packet_to_frame(response, frame_buf, frame_len))
        {
            return false;
        }

        if (!enqueue_prebuilt_frame(frame_buf, frame_len, true))
        {
            return false;
        }

        m_session.cache_response(ReqTraits::cmd, tx_id, RspTraits::cmd,
                                 frame_buf, frame_len);
        return true;
    }

    // ──────────────────────────────────────────────
    //  Unsolicited broadcast (status / telemetry)
    // ──────────────────────────────────────────────

    template <typename T>
    bool broadcast(const T& packet)
    {
        uint8_t frame_buf[MAX_FRAME_SIZE];
        size_t frame_len = 0;
        if (!serialize_packet_to_frame(packet, frame_buf, frame_len))
        {
            return false;
        }
        return enqueue_prebuilt_frame(frame_buf, frame_len);
    }

    // ──────────────────────────────────────────────
    //  Main loop — call from ProtocolTask
    // ──────────────────────────────────────────────

    void run_one_cycle()
    {
        drain_rx();
        handle_session_outbound();
        handle_session_retries();
        drain_tx();
    }

    // ──────────────────────────────────────────────
    //  Query
    // ──────────────────────────────────────────────

    typename Clock::duration poll_interval() const { return m_config.poll_interval; }
    std::optional<typename Clock::time_point> next_wake_time() const { return m_session.next_deadline(); }
    Session& session() { return m_session; }
    const Session& session() const { return m_session; }
    size_t rx_available() const { return m_rx_ring.available(); }
    size_t rx_dropped_writes() const { return m_rx_ring.dropped_writes(); }
    size_t tx_queue_size() const { return m_tx_queue.size(); }

private:
    static constexpr uint16_t ACK_CMD = 0x6FF0;
    static constexpr size_t MAX_FRAME_SIZE = 512;
    static constexpr size_t TX_QUEUE_CAP = 16;

    void handle_packet_internal(const uint16_t cmd_id,
                                const uint8_t* data, const size_t size)
    {
        if (cmd_id == ACK_CMD)
        {
            handle_ack_packet(data, size);
            return;
        }

        if (is_request_cmd(cmd_id))
        {
            handle_request_packet(cmd_id, data, size);
            return;
        }

        handle_response_packet(cmd_id, data, size);
    }

    // ── RX drain ──

    void drain_rx()
    {
        uint8_t buf[256];
        while (true)
        {
            const size_t avail = m_rx_ring.available();
            if (avail == 0) break;
            const size_t n = m_rx_ring.read(buf, avail < sizeof(buf) ? avail : sizeof(buf));
            if (n == 0) break;
            (void)m_parser.push_data(buf, n);
        }
    }

    // ── Session outbound pumping ──

    void handle_session_outbound()
    {
        while (true)
        {
            auto entry = m_session.next_outbound();
            if (!entry.has_value()) break;

            if (!enqueue_pending_frame(entry->request_cmd,
                                       entry->request_bytes,
                                       entry->request_len))
            {
                break;
            }
        }
    }

    // ── Session retries ──

    void handle_session_retries()
    {
        auto retries = m_session.poll();
        for (const auto& r : retries)
        {
            enqueue_pending_frame(r.request_cmd, r.request_bytes, r.request_len);
        }
    }

    // ── TX helpers ──

    bool enqueue_pending_frame(const uint16_t cmd_id, const uint8_t* raw, const size_t len)
    {
        if (m_tx_queue.size() >= TX_QUEUE_CAP)
        {
            if (!drop_one_telemetry())
                return false;
        }

        uint8_t frame_buf[MAX_FRAME_SIZE];
        size_t frame_len = 0;
        if (!serialize_raw_to_frame(cmd_id, raw, len, frame_buf, frame_len))
        {
            return false;
        }

        TxEntry entry{};
        std::memcpy(entry.frame.data(), frame_buf, frame_len);
        entry.len = frame_len;
        entry.telemetry = is_telemetry_cmd(cmd_id);
        m_tx_queue.push_back(std::move(entry));
        return true;
    }

    bool enqueue_prebuilt_frame(const uint8_t* frame, const size_t len, const bool priority = false)
    {
        if (len > MAX_FRAME_SIZE) return false;
        const bool telemetry = is_telemetry_frame(frame, len);

        if (m_tx_queue.size() >= TX_QUEUE_CAP)
        {
            if (priority || telemetry)
            {
                if (!drop_one_telemetry())
                    m_tx_queue.pop_back();
            }
            else
            {
                return false;
            }
        }

        TxEntry entry{};
        std::memcpy(entry.frame.data(), frame, len);
        entry.len = len;
        entry.telemetry = telemetry;
        m_tx_queue.push_back(std::move(entry));
        return true;
    }

    void drain_tx()
    {
        if (!m_send_frame || m_tx_queue.empty()) return;

        const auto& entry = m_tx_queue.front();
        if (m_send_frame(entry.frame.data(), entry.len))
        {
            m_tx_queue.pop_front();
        }
    }

    // ── Serialization helpers ──

    template <typename T>
    bool serialize_packet_to_frame(const T& packet, uint8_t* frame_buf, size_t& frame_len)
    {
        const auto result = m_serializer.template serialize<>(frame_buf, MAX_FRAME_SIZE, packet);
        if (result.has_value())
        {
            frame_len = *result;
            return true;
        }
        return false;
    }

    static constexpr bool is_telemetry_cmd(const uint16_t cmd_id)
    {
        return cmd_id >= 0x6000U && cmd_id <= 0x60FFU;
    }

    static bool is_telemetry_frame(const uint8_t* frame, const size_t len)
    {
        if (frame == nullptr || len < 7)
            return false;

        const auto cmd_id = static_cast<uint16_t>(frame[5])
            | (static_cast<uint16_t>(frame[6]) << 8U);
        return is_telemetry_cmd(cmd_id);
    }

    bool drop_one_telemetry()
    {
        for (auto it = m_tx_queue.begin(); it != m_tx_queue.end(); ++it)
        {
            if (it->telemetry)
            {
                m_tx_queue.erase(it);
                return true;
            }
        }
        return false;
    }

    template <typename T>
    bool try_serialize_raw(const uint16_t cmd_id,
                           const uint8_t* raw, const size_t len,
                           uint8_t* frame_buf, size_t& frame_len)
    {
        if (cmd_id != RPL::Meta::PacketTraits<T>::cmd) return false;
        if (len != sizeof(T)) return false;

        T packet{};
        std::memcpy(&packet, raw, len);
        return serialize_packet_to_frame(packet, frame_buf, frame_len);
    }

    bool serialize_raw_to_frame(const uint16_t cmd_id,
                                const uint8_t* raw, const size_t len,
                                uint8_t* frame_buf, size_t& frame_len)
    {
        return (try_serialize_raw<PacketTypes>(cmd_id, raw, len, frame_buf, frame_len) || ...);
    }

    // ── Request/response classification ──

    static constexpr bool is_request_cmd(const uint16_t cmd_id)
    {
        return ((cmd_id & 0x0001U) == 0x0001U) || (cmd_id >= 0x6300 && cmd_id < 0x6400);
    }

    // ── TX ID extraction (runtime) ──

    template <typename T>
    static constexpr bool has_tx_id_v = requires(const T& t) { t.tx_id; };

    TransactionId extract_tx_id_from_raw(const uint16_t cmd_id,
                                         const uint8_t* data, const size_t size) const
    {
        TransactionId tx_id = 0;
        auto try_extract = [&]<typename T>() -> bool {
            if (cmd_id != RPL::Meta::PacketTraits<T>::cmd) return false;
            if (size != sizeof(T)) return false;
            if constexpr (has_tx_id_v<T>)
            {
                tx_id = reinterpret_cast<const T*>(data)->tx_id;
            }
            return true;
        };
        (try_extract.template operator()<PacketTypes>() || ...);
        return tx_id;
    }

    // ── ACK handling ──

    void handle_ack_packet(const uint8_t* data, const size_t size)
    {
        if (size != sizeof(AckPacket)) return;
        AckPacket ack{};
        std::memcpy(&ack, data, sizeof(AckPacket));
        m_session.on_ack(ack.tx_id);
    }

    // ── Request handling (device side) ──

    void handle_request_packet(const uint16_t cmd_id, const uint8_t* data, const size_t size)
    {
        const TransactionId tx_id = extract_tx_id_from_raw(cmd_id, data, size);
        const bool is_reliable = (cmd_id < 0x6300);

        if (!is_reliable)
        {
            const auto handler_it = m_handlers.find(cmd_id);
            if (handler_it != m_handlers.end() && handler_it->second)
            {
                handler_it->second(cmd_id, tx_id, data, size);
            }
            return;
        }

        // Status broadcast range (0x6000–0x60FF): no tx_id, dispatch directly
        if (tx_id == 0)
        {
            if (cmd_id >= 0x6000 && cmd_id < 0x6100)
            {
                const auto handler_it = m_handlers.find(cmd_id);
                if (handler_it != m_handlers.end() && handler_it->second)
                {
                    handler_it->second(cmd_id, tx_id, data, size);
                }
            }
            return;
        }

        const auto result = m_session.on_request(cmd_id, tx_id);

        AckPacket ack{};
        ack.tx_id = tx_id;
        ack.cmd_id = cmd_id;
        ack.status = 0;

        uint8_t ack_frame[MAX_FRAME_SIZE];
        size_t ack_frame_len = 0;
        if (serialize_packet_to_frame(ack, ack_frame, ack_frame_len))
        {
            enqueue_prebuilt_frame(ack_frame, ack_frame_len, true);
        }

        if (result.is_duplicate)
        {
            if (result.cached_response_data && result.cached_response_len > 0)
            {
                enqueue_prebuilt_frame(result.cached_response_data, result.cached_response_len, true);
            }
            return;
        }

        const auto handler_it = m_handlers.find(cmd_id);
        if (handler_it != m_handlers.end() && handler_it->second)
        {
            handler_it->second(cmd_id, tx_id, data, size);
        }
    }

    // ── Response handling (host side) ──

    void handle_response_packet(const uint16_t cmd_id, const uint8_t* data, const size_t size)
    {
        const TransactionId tx_id = extract_tx_id_from_raw(cmd_id, data, size);
        if (tx_id == 0) return;
        m_session.on_response(cmd_id, tx_id, data, size);
    }

    // ── Members ──

    Config m_config;
    Session m_session;
    RPL::Deserializer<PacketTypes...> m_deserializer;
    RPL::Parser<PacketTypes...> m_parser;
    RPL::Serializer<PacketTypes...> m_serializer;

    ISRRingBuffer<4096> m_rx_ring;

    struct TxEntry
    {
        std::array<uint8_t, MAX_FRAME_SIZE> frame{};
        size_t len = 0;
        bool telemetry = false;
    };
    std::deque<TxEntry> m_tx_queue;
    SendFrameFn m_send_frame = nullptr;

    ankerl::unordered_dense::map<uint16_t, RequestHandler> m_handlers;

    static ProtocolStack*& instance()
    {
        thread_local ProtocolStack* ptr = nullptr;
        return ptr;
    }
};

} // namespace florid::usb

#endif // FLORID_PROTOCOLS_PROTOCOLSTACK_HPP
