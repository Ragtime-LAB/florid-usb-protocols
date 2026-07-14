#ifndef FLORID_PROTOCOLS_RELIABLESESSION_HPP
#define FLORID_PROTOCOLS_RELIABLESESSION_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <optional>
#include <vector>

#include <ankerl/unordered_dense.h>

#include <IdRingBuffer.hpp>

namespace florid::usb
{
    enum class SessionError : uint8_t
    {
        TransportFailure = 0,
        Timeout = 1,
    };

    template <typename Clock = std::chrono::steady_clock>
    class ReliableSession
    {
    public:
        using ErrorCallback = std::function<void(SessionError)>;
        using ResponseCallback = std::function<void(const uint8_t* bytes, size_t size)>;

        struct Config
        {
            size_t window_size = 1;
            typename Clock::duration timeout{200000}; // 200ms in µs
            uint8_t max_retries = 3;
            size_t device_cache_capacity = 64;
        };

        ReliableSession() : ReliableSession(Config{}) {}

        explicit ReliableSession(const Config& config)
            : m_config(config), m_next_tx_id(1)
        {
            if (m_config.window_size == 0)
            {
                m_config.window_size = 1;
            }
        }

        // ──────────────────────────────────────────────
        //  Host side — allocate + track pending request
        // ──────────────────────────────────────────────

        TransactionId next_tx_id()
        {
            return m_next_tx_id++;
        }

        void set_next_tx_id(TransactionId next_tx_id)
        {
            m_next_tx_id = (next_tx_id == 0) ? 1 : next_tx_id;
        }

        void register_pending(const TransactionId tx_id,
                              const uint16_t request_cmd,
                              const uint16_t response_cmd,
                              const uint8_t* request_bytes,
                              const size_t request_len,
                              ResponseCallback on_response,
                              ErrorCallback on_error,
                              const typename Clock::duration& timeout,
                              const uint8_t max_retries)
        {
            PendingEntry pending{};
            pending.tx_id = tx_id;
            pending.request_cmd = request_cmd;
            pending.response_cmd = response_cmd;
            pending.request_len = (request_len <= MAX_PACKET_SIZE) ? request_len : MAX_PACKET_SIZE;
            std::memcpy(pending.request_bytes.data(), request_bytes, pending.request_len);
            pending.on_response = std::move(on_response);
            pending.on_error = std::move(on_error);
            pending.timeout = timeout;
            pending.max_retries = max_retries;
            pending.deadline = Clock::now() + timeout;

            m_pending_by_tx.insert_or_assign(tx_id, std::move(pending));
            m_outbound_queue.push_back(tx_id);
        }

        void on_ack(const TransactionId tx_id)
        {
            const auto it = m_pending_by_tx.find(tx_id);
            if (it == m_pending_by_tx.end() || !it->second.inflight)
            {
                return;
            }
            it->second.deadline = Clock::now() + it->second.timeout;
        }

        void on_response(const uint16_t cmd_id,
                         const TransactionId tx_id,
                         const uint8_t* bytes,
                         const size_t size)
        {
            const DedupKey dk = compose_dedup_key(cmd_id, tx_id);
            if (m_seen_responses.test_and_insert(dk))
            {
                return;
            }

            const auto it = m_pending_by_tx.find(tx_id);
            if (it == m_pending_by_tx.end())
            {
                return;
            }

            auto pending = std::move(it->second);
            if (pending.inflight && m_inflight_count > 0)
            {
                m_inflight_count -= 1;
            }
            m_pending_by_tx.erase(it);

            if (pending.response_cmd == cmd_id && pending.on_response)
            {
                pending.on_response(bytes, size);
            }
        }

        // ──────────────────────────────────────────────
        //  Device side — dedup + cache
        // ──────────────────────────────────────────────

        struct RequestResult
        {
            bool should_ack = true;
            bool is_duplicate = false;
            const uint8_t* cached_response_data = nullptr;
            size_t cached_response_len = 0;
            uint16_t cached_response_cmd = 0;
        };

        RequestResult on_request(const uint16_t cmd_id, const TransactionId tx_id)
        {
            const DedupKey dk = compose_dedup_key(cmd_id, tx_id);

            if (m_seen_requests.test_and_insert(dk))
            {
                RequestResult r{};
                r.should_ack = true;
                r.is_duplicate = true;
                if (const auto cached = m_completed_cache.find(dk);
                    cached != m_completed_cache.end())
                {
                    r.cached_response_data = cached->second.response_bytes.data();
                    r.cached_response_len = cached->second.response_len;
                    r.cached_response_cmd = cached->second.response_cmd;
                }
                return r;
            }

            RequestResult r{};
            r.should_ack = true;
            r.is_duplicate = false;
            return r;
        }

        void cache_response(const uint16_t cmd_id,
                            const TransactionId tx_id,
                            const uint16_t response_cmd,
                            const uint8_t* response_bytes,
                            const size_t response_len)
        {
            const DedupKey dk = compose_dedup_key(cmd_id, tx_id);

            CompletedResponse completed{};
            completed.response_cmd = response_cmd;
            completed.response_len = (response_len <= MAX_PACKET_SIZE) ? response_len : MAX_PACKET_SIZE;
            std::memcpy(completed.response_bytes.data(), response_bytes, completed.response_len);

            m_completed_cache.insert_or_assign(dk, std::move(completed));
            m_completed_order.push_back(dk);
            shrink_device_cache();
        }

        // ──────────────────────────────────────────────
        //  Poll — timeout check, retransmit
        // ──────────────────────────────────────────────

        struct RetransmitEntry
        {
            TransactionId tx_id;
            uint16_t request_cmd;
            const uint8_t* request_bytes;
            size_t request_len;
        };

        std::vector<RetransmitEntry> poll()
        {
            const auto now = Clock::now();
            std::vector<RetransmitEntry> retries;
            std::vector<std::pair<ErrorCallback, SessionError>> callbacks;

            for (auto it = m_pending_by_tx.begin(); it != m_pending_by_tx.end();)
            {
                auto& pending = it->second;
                if (!pending.inflight || now < pending.deadline)
                {
                    ++it;
                    continue;
                }

                if (pending.retry_count >= pending.max_retries)
                {
                    if (pending.inflight && m_inflight_count > 0)
                    {
                        m_inflight_count -= 1;
                    }
                    callbacks.emplace_back(std::move(pending.on_error), SessionError::Timeout);
                    it = m_pending_by_tx.erase(it);
                    continue;
                }

                pending.retry_count += 1;
                pending.deadline = Clock::now() + pending.timeout;
                retries.push_back(RetransmitEntry{
                    .tx_id = pending.tx_id,
                    .request_cmd = pending.request_cmd,
                    .request_bytes = pending.request_bytes.data(),
                    .request_len = pending.request_len,
                });
                ++it;
            }

            for (auto& [cb, err] : callbacks)
            {
                if (cb)
                {
                    cb(err);
                }
            }

            return retries;
        }

        // ──────────────────────────────────────────────
        //  TX pump — window-based outbound
        // ──────────────────────────────────────────────

        std::optional<RetransmitEntry> next_outbound()
        {
            while (m_inflight_count < m_config.window_size && !m_outbound_queue.empty())
            {
                const auto tx_id = m_outbound_queue.front();
                m_outbound_queue.pop_front();

                const auto it = m_pending_by_tx.find(tx_id);
                if (it == m_pending_by_tx.end())
                {
                    continue;
                }

                auto& pending = it->second;
                pending.inflight = true;
                pending.deadline = Clock::now() + pending.timeout;
                m_inflight_count += 1;

                return RetransmitEntry{
                    .tx_id = pending.tx_id,
                    .request_cmd = pending.request_cmd,
                    .request_bytes = pending.request_bytes.data(),
                    .request_len = pending.request_len,
                };
            }
            return std::nullopt;
        }

        size_t inflight_count() const { return m_inflight_count; }

        // ──────────────────────────────────────────────
        //  Deadline query — for poll period calc
        // ──────────────────────────────────────────────

        std::optional<typename Clock::time_point> next_deadline() const
        {
            std::optional<typename Clock::time_point> earliest;
            for (const auto& [tx_id, pending] : m_pending_by_tx)
            {
                if (!pending.inflight)
                {
                    continue;
                }
                if (!earliest.has_value() || pending.deadline < *earliest)
                {
                    earliest = pending.deadline;
                }
            }
            return earliest;
        }

        void reset_device_state()
        {
            m_seen_requests.clear();
            m_seen_responses.clear();
            m_completed_cache.clear();
            m_completed_order.clear();
        }

    private:
        static constexpr size_t MAX_PACKET_SIZE = 256;
        static constexpr size_t SEEN_ID_RING_SIZE = 64;

        struct PendingEntry
        {
            TransactionId tx_id = 0;
            uint16_t request_cmd = 0;
            uint16_t response_cmd = 0;
            std::array<uint8_t, MAX_PACKET_SIZE> request_bytes{};
            size_t request_len = 0;
            ResponseCallback on_response;
            ErrorCallback on_error{};
            typename Clock::duration timeout{200000};
            uint8_t max_retries = 3;
            uint8_t retry_count = 0;
            bool inflight = false;
            typename Clock::time_point deadline{};
        };

        struct CompletedResponse
        {
            uint16_t response_cmd = 0;
            std::array<uint8_t, MAX_PACKET_SIZE> response_bytes{};
            size_t response_len = 0;
        };

        void shrink_device_cache()
        {
            while (m_completed_order.size() > m_config.device_cache_capacity)
            {
                const auto oldest = m_completed_order.front();
                m_completed_order.pop_front();
                m_completed_cache.erase(oldest);
            }
        }

        Config m_config{};
        TransactionId m_next_tx_id{1};
        size_t m_inflight_count = 0;

        ankerl::unordered_dense::map<TransactionId, PendingEntry> m_pending_by_tx;
        std::deque<TransactionId> m_outbound_queue;

        ankerl::unordered_dense::map<DedupKey, CompletedResponse> m_completed_cache;
        std::deque<DedupKey> m_completed_order;

        DedupRingBuffer m_seen_requests{};
        DedupRingBuffer m_seen_responses{};
    };

} // namespace florid::usb

#endif // FLORID_PROTOCOLS_RELIABLESESSION_HPP
