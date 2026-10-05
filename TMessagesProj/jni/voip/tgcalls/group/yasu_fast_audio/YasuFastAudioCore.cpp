#include "YasuFastAudioCore.h"
#include <thread>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>

#include <opus.h>
#include "modules/audio_coding/codecs/opus/opus_interface.h"

#include "rtc_base/logging.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/units/time_delta.h"
#include "voip/tgcalls/YasuMeasurementGate.h"

namespace tgcalls {

namespace {

constexpr int kMaxStreams = 32;

// Very small packet reorder window.
// We prefer a tiny amount of loss/stutter over waiting for late packets.
constexpr int kPacketSlots = 4;
constexpr int kMaxPayload = 1600;

// Fixed packet handoff queue between the WebRTC WorkerThread and the
// dedicated realtime FAST audio queue. No per-packet heap allocation.
constexpr int kFastPacketQueueSlots = 8;

// Small low-latency PCM ring. Opus decode uses its own larger buffer;
// this ring remains intentionally small to avoid adding playback latency.
// MUST be a power of two: every index update uses `& (kStreamRingFrames - 1)`.
// The previous value (12288) is NOT a power of two, so the mask 0x2FFF made the
// write/read index wrap into [0x2000..0x2FFF] and corrupted occupancy math.
constexpr int kStreamRingFrames = 16384;
static_assert((kStreamRingFrames & (kStreamRingFrames - 1)) == 0,
              "kStreamRingFrames must be a power of two");

// Adaptive jitter pad (per stream, derived from measured RTP arrival jitter).
constexpr uint32_t kPadMinFrames = 60;       // 1.25 ms
constexpr uint32_t kPadMaxFrames = 240;       // 5 ms
constexpr uint32_t kPadInitialFrames = 0;    // no artificial startup delay
constexpr int kJitterHistory = 64;
// Extra headroom above (pad + one packet) before we start trimming backlog.
constexpr uint32_t kTrimHeadroomFrames = 120;  // 2.5 ms
// Do not hard-jump the PCM read pointer for small/normal backlog.
// A hard resync is allowed only when the excess itself reaches 30 ms.
constexpr uint32_t kHardTrimExcessFrames = 1440;  // 30 ms @ 48 kHz
// Backlog above (pad + packet + headroom + this) is dropped at once (a short
// glitch beats hundreds of ms of permanent delay after a network burst).
// Adaptive network protection.
// The controller prefers minimum latency and only increases protection
// when the RTP stream proves that reordering/jitter actually requires it.
constexpr uint64_t kAdaptiveReorderMinUs = 220;   // 0.22 ms
constexpr uint64_t kAdaptiveReorderMaxUs = 1200;  // 1.20 ms
constexpr uint64_t kAdaptiveRecoveryUs = 180000;  // 180 ms
constexpr uint64_t kAdaptiveDecayUs = 350000;     // 350 ms
constexpr double kAdaptiveJitterWeight = 0.45;
constexpr double kAdaptiveReorderWeight = 0.35;
constexpr double kAdaptiveLossWeight = 0.20;

// Never conceal more than this many consecutive missing packets; resync instead.
constexpr uint16_t kMaxConcealPackets = 2;

constexpr int kOutputChunk = 480;

// WebRTC uses a maximum normal Opus decode frame size of 120 ms.
// At 48 kHz this is 5760 samples per channel.
constexpr int kDecodeSamples = 5760;

using Clock = std::chrono::steady_clock;

uint64_t NowUs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now().time_since_epoch()).count());
}

// Soft knee limiter for the multi-speaker sum (avoids hard-clip distortion).
inline int16_t SoftLimit(int32_t v) {
    constexpr int32_t kKnee = 24000;
    constexpr int32_t kRoom = 32767 - kKnee;
    if (v > -kKnee && v < kKnee) {
        return static_cast<int16_t>(v);
    }
    const bool neg = v < 0;
    const int32_t a = neg ? -v : v;
    const int32_t e = a - kKnee;
    const int32_t o = kKnee + static_cast<int32_t>(
        static_cast<int64_t>(kRoom) * e / (e + kRoom));
    return static_cast<int16_t>(neg ? -o : o);
}

inline bool SeqAhead(uint16_t sequence, uint16_t expected) {
    return static_cast<int16_t>(
        static_cast<uint16_t>(sequence - expected)) > 0;
}

inline bool SeqBehind(uint16_t sequence, uint16_t expected) {
    return static_cast<int16_t>(
        static_cast<uint16_t>(sequence - expected)) < 0;
}

inline uint16_t SeqForwardDistance(
    uint16_t sequence,
    uint16_t expected) {
    return static_cast<uint16_t>(sequence - expected);
}

struct Packet {
    bool valid = false;
    uint16_t sequence = 0;
    uint32_t timestamp = 0;
    uint16_t size = 0;

    // Arrival time used for diagnostics (0 when measurements are off).
    uint64_t arrival_us = 0;

    // Real arrival time of the RTP packet (always set; drives reorder timer).
    uint64_t real_arrival_us = 0;

    uint8_t data[kMaxPayload]{};
};

struct Stream {
    std::atomic<bool> active{false};

    uint32_t ssrc = 0;

    OpusDecoder* decoder = nullptr;

    bool have_sequence = false;
    uint16_t next_sequence = 0;

    bool have_timestamp = false;
    uint32_t next_timestamp = 0;

    Packet packets[kPacketSlots];

    // Mono PCM ring after stereo Opus downmix.
    int16_t pcm[kStreamRingFrames]{};

    // Arrival timestamp associated with each PCM sample.
    // 0 means the sample was generated by PLC and has no real
    // RTP arrival timestamp.
    uint64_t pcm_time_us[kStreamRingFrames]{};

    std::atomic<uint32_t> pcm_read{0};
    std::atomic<uint32_t> pcm_write{0};

    // Written by packet path, read only for diagnostics.
    std::atomic<uint64_t> last_packet_us{0};

    // ---- YASU adaptive jitter pad (producer-owned, consumer reads) ----
    // Real duration of the last decoded packet (frames @48k). Used for PLC
    // length and trim threshold. Works for 10..120 ms senders.
    std::atomic<uint32_t> last_packet_frames{960};
    // Silence inserted before audio when the ring is empty (start/underrun).
    std::atomic<uint32_t> pad_frames{kPadInitialFrames};

    bool have_jitter_base = false;
    uint32_t jitter_first_ts = 0;
    uint64_t jitter_first_arrival_us = 0;
    int64_t jitter_base_us = 0;
    int32_t jitter_hist_us[kJitterHistory]{};
    int jitter_hist_n = 0;
    int jitter_hist_pos = 0;
    uint32_t jitter_update_counter = 0;
    double pad_smooth = static_cast<double>(kPadInitialFrames);

    // A delayed re-drain is already scheduled for a reordered packet.
    bool reorder_timer_posted = false;

    // ---- YASU adaptive network controller ----
    // These values are producer-owned and stay local to the FAST stream.
    // They describe current network pressure, not a permanent buffer target.
    double network_jitter_ewma_us = 0.0;
    double network_reorder_score = 0.0;
    double network_loss_score = 0.0;
    double network_pressure = 0.0;
    uint8_t network_level = 0;
    uint64_t network_last_update_us = 0;
    uint64_t network_recovery_until_us = 0;
    uint64_t network_last_event_us = 0;
    uint64_t adaptive_reorder_wait_us = kAdaptiveReorderMinUs;

    void ResetAdaptive() {
        last_packet_frames.store(960, std::memory_order_relaxed);
        pad_frames.store(kPadInitialFrames, std::memory_order_relaxed);
        have_jitter_base = false;
        jitter_first_ts = 0;
        jitter_first_arrival_us = 0;
        jitter_base_us = 0;
        jitter_hist_n = 0;
        jitter_hist_pos = 0;
        pad_smooth = static_cast<double>(kPadInitialFrames);
        reorder_timer_posted = false;
        network_jitter_ewma_us = 0.0;
        network_reorder_score = 0.0;
        network_loss_score = 0.0;
        network_pressure = 0.0;
        network_level = 0;
        network_last_update_us = 0;
        network_recovery_until_us = 0;
        network_last_event_us = 0;
        adaptive_reorder_wait_us = kAdaptiveReorderMinUs;
    }
};

struct Stats {
    std::atomic<uint64_t> started_us{0};

    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> packets_decoded{0};
    std::atomic<uint64_t> packets_plc{0};
    std::atomic<uint64_t> packets_fec{0};
    std::atomic<uint64_t> packets_late{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> packets_reordered{0};
    std::atomic<uint64_t> packets_decode_failed{0};

    std::atomic<uint64_t> packet_queue_wait_count{0};
    std::atomic<uint64_t> packet_queue_wait_us{0};
    std::atomic<uint64_t> packet_queue_wait_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> packet_queue_wait_max_us{0};

    std::atomic<uint64_t> decode_count{0};
    std::atomic<uint64_t> decode_cost_us{0};
    std::atomic<uint64_t> decode_cost_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> decode_cost_max_us{0};

    // Packet arrival -> beginning/end of decoded PCM buffering.
    std::atomic<uint64_t> packet_to_pcm_count{0};
    std::atomic<uint64_t> packet_to_pcm_us{0};
    std::atomic<uint64_t> packet_to_pcm_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> packet_to_pcm_max_us{0};

    // RTP arrival -> AAudio ReadPcm.
    std::atomic<uint64_t> packet_to_play_count{0};
    std::atomic<uint64_t> packet_to_play_us{0};
    std::atomic<uint64_t> packet_to_play_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> packet_to_play_max_us{0};

    std::atomic<uint64_t> pcm_frames_written{0};
    std::atomic<uint64_t> pcm_frames_read{0};

    // YASU REAL PLAYOUT DIAGNOSTICS.
    // These counters prove whether AAudio actually reaches ReadPcm().
    std::atomic<uint64_t> readpcm_calls{0};
    std::atomic<uint64_t> readpcm_enabled_calls{0};
    std::atomic<uint64_t> readpcm_invalid_calls{0};
    std::atomic<uint64_t> readpcm_wrong_rate_calls{0};
    std::atomic<uint64_t> readpcm_zero_source_calls{0};
    std::atomic<int32_t> readpcm_last_sample_rate{0};
    std::atomic<int32_t> readpcm_last_channels{0};

    // Real callback starvation.
    std::atomic<uint64_t> pcm_underruns{0};

    // Old PCM discarded because ring was full.
    std::atomic<uint64_t> pcm_overruns{0};
    std::atomic<uint64_t> pcm_overrun_frames{0};

    std::atomic<uint64_t> residence_count{0};
    std::atomic<uint64_t> residence_us{0};
    std::atomic<uint64_t> residence_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> residence_max_us{0};

    std::atomic<uint64_t> callback_count{0};
    std::atomic<uint64_t> callback_frames{0};
    std::atomic<uint64_t> callback_cost_us{0};
    std::atomic<uint64_t> callback_cost_min_us{
        std::numeric_limits<uint64_t>::max()};
    std::atomic<uint64_t> callback_cost_max_us{0};

    std::atomic<uint64_t> max_ring_depth{0};
    std::atomic<uint64_t> active_streams_peak{0};

    void Reset() {
        started_us.store(NowUs(), std::memory_order_relaxed);

        packets_received.store(0);
        packets_decoded.store(0);
        packets_plc.store(0);
        packets_fec.store(0);
        packets_late.store(0);
        packets_dropped.store(0);
        packets_reordered.store(0);
        packets_decode_failed.store(0);

        packet_queue_wait_count.store(0);
        packet_queue_wait_us.store(0);
        packet_queue_wait_min_us.store(
            std::numeric_limits<uint64_t>::max());
        packet_queue_wait_max_us.store(0);

        decode_count.store(0);
        decode_cost_us.store(0);
        decode_cost_min_us.store(
            std::numeric_limits<uint64_t>::max());
        decode_cost_max_us.store(0);

        packet_to_pcm_count.store(0);
        packet_to_pcm_us.store(0);
        packet_to_pcm_min_us.store(
            std::numeric_limits<uint64_t>::max());
        packet_to_pcm_max_us.store(0);

        packet_to_play_count.store(0);
        packet_to_play_us.store(0);
        packet_to_play_min_us.store(
            std::numeric_limits<uint64_t>::max());
        packet_to_play_max_us.store(0);

        pcm_frames_written.store(0);
        pcm_frames_read.store(0);

        readpcm_calls.store(0);
        readpcm_enabled_calls.store(0);
        readpcm_invalid_calls.store(0);
        readpcm_wrong_rate_calls.store(0);
        readpcm_zero_source_calls.store(0);
        readpcm_last_sample_rate.store(0);
        readpcm_last_channels.store(0);

        pcm_underruns.store(0);
        pcm_overruns.store(0);
        pcm_overrun_frames.store(0);

        residence_count.store(0);
        residence_us.store(0);
        residence_min_us.store(
            std::numeric_limits<uint64_t>::max());
        residence_max_us.store(0);

        callback_count.store(0);
        callback_frames.store(0);
        callback_cost_us.store(0);
        callback_cost_min_us.store(
            std::numeric_limits<uint64_t>::max());
        callback_cost_max_us.store(0);

        max_ring_depth.store(0);
        active_streams_peak.store(0);
    }

    static void AddMin(
        std::atomic<uint64_t>& value,
        uint64_t v) {

        uint64_t old =
            value.load(std::memory_order_relaxed);

        while (v < old &&
               !value.compare_exchange_weak(
                   old,
                   v,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed)) {
        }
    }

    static void AddMax(
        std::atomic<uint64_t>& value,
        uint64_t v) {

        uint64_t old =
            value.load(std::memory_order_relaxed);

        while (v > old &&
               !value.compare_exchange_weak(
                   old,
                   v,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed)) {
        }
    }

    void DecodeCost(uint64_t us) {
        decode_count.fetch_add(1);
        decode_cost_us.fetch_add(us);
        AddMin(decode_cost_min_us, us);
        AddMax(decode_cost_max_us, us);
    }

    void PacketToPcm(uint64_t us) {
        packet_to_pcm_count.fetch_add(1);
        packet_to_pcm_us.fetch_add(us);
        AddMin(packet_to_pcm_min_us, us);
        AddMax(packet_to_pcm_max_us, us);
    }

    void PacketToPlay(uint64_t us) {
        packet_to_play_count.fetch_add(1);
        packet_to_play_us.fetch_add(us);
        AddMin(packet_to_play_min_us, us);
        AddMax(packet_to_play_max_us, us);
    }

    void Residence(uint64_t us) {
        residence_count.fetch_add(1);
        residence_us.fetch_add(us);
        AddMin(residence_min_us, us);
        AddMax(residence_max_us, us);
    }

    void Callback(uint64_t us, int frames) {
        callback_count.fetch_add(1);
        callback_frames.fetch_add(
            static_cast<uint64_t>(frames));
        callback_cost_us.fetch_add(us);
        AddMin(callback_cost_min_us, us);
        AddMax(callback_cost_max_us, us);
    }
};

}  // namespace

struct YasuFastAudioCore::Impl {
    struct QueuedPacket {
        uint32_t ssrc = 0;
        uint16_t sequence = 0;
        uint32_t timestamp = 0;
        uint16_t size = 0;
        uint64_t generation = 0;
        uint64_t arrival_us = 0;
        uint8_t data[kMaxPayload]{};
    };

    // FAST is permanently active for Group Voice.
    // This flag only protects Reset() from concurrent realtime access.
    std::atomic<bool> resetting{false};

    // Number of realtime AAudio callbacks currently inside ReadPcm().
    // Reset() must wait for these readers before destroying stream state.
    std::atomic<uint32_t> active_readers{0};

    // Every explicit enable/reset advances this generation.
    // Queued packets from an older Group Call are discarded.
    std::atomic<uint64_t> generation{0};

    static constexpr size_t kMaxGroupSsrcs = 32;
    std::atomic<uint32_t> group_ssrcs[kMaxGroupSsrcs]{};

    // Dedicated FAST audio worker.
    // HIGH maps to rtc::ThreadPriority::kRealtime in this build.
    // This does NOT raise the priority of WebRTC's shared WorkerThread.
    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory;
    std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> fast_queue;

    // Preallocated packet handoff queue.
    // Producer: WebRTC WorkerThread.
    // Consumer: dedicated realtime FAST queue.
    QueuedPacket packet_queue[kFastPacketQueueSlots];
    uint32_t packet_queue_read = 0;
    uint32_t packet_queue_write = 0;

    // The worker keeps the read slot owned while ProcessPacket() is using
    // its payload. This removes the second payload memcpy without allowing
    // the producer to overwrite an in-flight packet.
    uint32_t packet_queue_inflight = 0;
    bool packet_queue_inflight_active = false;

    std::atomic_flag packet_queue_lock = ATOMIC_FLAG_INIT;
    std::atomic<bool> packet_task_posted{false};

    void LockPacketQueue() {
        while (packet_queue_lock.test_and_set(
            std::memory_order_acquire)) {
        }
    }

    void UnlockPacketQueue() {
        packet_queue_lock.clear(std::memory_order_release);
    }

    bool PacketQueueEmpty() const {
        return packet_queue_read == packet_queue_write;
    }

    uint32_t PacketQueueNext(uint32_t index) const {
        return (index + 1) % kFastPacketQueueSlots;
    }

    // Serializes packet/decode/reset operations.
    // The AAudio callback never takes this lock.
    std::mutex processing;

    Stream streams[kMaxStreams];
    Stats stats;

    void Lock() {
        processing.lock();
    }

    void Unlock() {
        processing.unlock();
    }

    bool Measurements() const {
        return YasuFastMeasurementsEnabled();
    }

    Stream* Find(uint32_t ssrc) {
        for (auto& stream : streams) {
            if (stream.active.load(
                    std::memory_order_acquire) &&
                stream.ssrc == ssrc) {
                return &stream;
            }
        }

        return nullptr;
    }

    Stream* Create(uint32_t ssrc) {
        Stream* stream = nullptr;

        // Prefer an inactive slot.
        for (auto& item : streams) {
            if (!item.active.load(
                    std::memory_order_acquire)) {
                stream = &item;
                break;
            }
        }

        // All 32 streams are occupied.
        if (!stream) {
            // Reclaim only a stream that has been silent for a long time.
            constexpr uint64_t kStaleStreamTimeoutUs =
                30ULL * 1000ULL * 1000ULL;

            const uint64_t now = NowUs();
            Stream* stale = nullptr;

            for (auto& item : streams) {
                if (!item.active.load(
                        std::memory_order_acquire)) {
                    continue;
                }

                const uint64_t last =
                    item.last_packet_us.load(
                        std::memory_order_acquire);

                if (last == 0 ||
                    now < last ||
                    now - last < kStaleStreamTimeoutUs) {
                    continue;
                }

                stale = &item;
                break;
            }

            if (!stale) {
                if (YasuFastMeasurementsEnabled()) {
                    stats.packets_dropped.fetch_add(1);
                }
                return nullptr;
            }

            // Hide the stale stream from new realtime callbacks.
            stale->active.store(
                false,
                std::memory_order_release);

            // Wait for callbacks that may already be reading this stream.
            while (active_readers.load(
                       std::memory_order_acquire) != 0) {
                std::this_thread::yield();
            }

            if (stale->decoder) {
                opus_decoder_destroy(
                    stale->decoder);

                stale->decoder = nullptr;
            }

            stale->ssrc = 0;

            stale->have_sequence = false;
            stale->next_sequence = 0;

            stale->have_timestamp = false;
            stale->next_timestamp = 0;

            stale->last_packet_us.store(
                0,
                std::memory_order_release);

            stale->pcm_read.store(
                0,
                std::memory_order_release);

            stale->pcm_write.store(
                0,
                std::memory_order_release);

            stream = stale;
        }

        int error = OPUS_OK;

        stream->decoder =
            opus_decoder_create(
                48000,
                2,
                &error);

        if (stream->decoder && error == OPUS_OK) {
            // Match the low-complexity Opus configuration used by
            // the optimized Yasu audio path.
        }

        if (!stream->decoder ||
            error != OPUS_OK) {

            stream->decoder = nullptr;

            stream->active.store(
                false,
                std::memory_order_release);

            if (YasuFastMeasurementsEnabled()) {
                stats.packets_dropped.fetch_add(1);
            }

            return nullptr;
        }

        std::memset(
            stream->packets,
            0,
            sizeof(stream->packets));

        std::memset(
            stream->pcm,
            0,
            sizeof(stream->pcm));

        std::memset(
            stream->pcm_time_us,
            0,
            sizeof(stream->pcm_time_us));

        stream->pcm_read.store(
            0,
            std::memory_order_release);

        stream->pcm_write.store(
            0,
            std::memory_order_release);

        stream->ssrc = ssrc;
        stream->ResetAdaptive();

        stream->have_sequence = false;
        stream->next_sequence = 0;

        stream->have_timestamp = false;
        stream->next_timestamp = 0;

        stream->last_packet_us.store(
            NowUs(),
            std::memory_order_release);

        stream->active.store(
            true,
            std::memory_order_release);

        uint64_t count = 0;

        for (const auto& item : streams) {
            if (item.active.load(
                    std::memory_order_acquire)) {
                ++count;
            }
        }

        Stats::AddMax(
            stats.active_streams_peak,
            count);

        return stream;
    }

    void PushPcm(
        Stream* stream,
        const int16_t* stereo,
        int samples,
        uint64_t source_arrival_us) {

        if (!stereo || samples <= 0) {
            return;
        }

        uint32_t write =
            stream->pcm_write.load(
                std::memory_order_relaxed);

        const uint32_t read =
            stream->pcm_read.load(
                std::memory_order_acquire);

        uint32_t used =
            write >= read
                ? write - read
                : kStreamRingFrames - read + write;

        const uint32_t capacity =
            kStreamRingFrames - 1;

        // YASU JITTER PAD: the ring is empty => stream is starting or just
        // underran. Re-prime with adaptive silence so the next packet that
        // arrives a little late does not cut the audio again.
        bool padded = false;
        if (used == 0) {
            uint32_t pad = stream->pad_frames.load(
                std::memory_order_relaxed);
            if (pad > capacity / 2) {
                pad = capacity / 2;
            }
            for (uint32_t i = 0; i < pad; ++i) {
                stream->pcm[write] = 0;
                stream->pcm_time_us[write] = 0;
                write = (write + 1) & (kStreamRingFrames - 1);
            }
            used = pad;
            padded = pad != 0;
        }

        const uint32_t free_frames =
            capacity - used;

        const uint32_t frames_to_write =
            std::min(
                static_cast<uint32_t>(samples),
                free_frames);

        const uint32_t dropped_frames =
            static_cast<uint32_t>(samples) -
            frames_to_write;

        // SPSC rule:
        // pcm_write is owned by the producer.
        // pcm_read is owned by the consumer.
        // Never modify pcm_read from the producer.
        for (uint32_t i = 0; i < frames_to_write; ++i) {
            const int32_t left =
                stereo[i * 2];

            const int32_t right =
                stereo[i * 2 + 1];

            int32_t mono = (left + right) / 2;
            if (padded && source_arrival_us != 0 && i < 48) {
                mono = mono * static_cast<int32_t>(i) / 48;  // 1 ms fade-in
            }

            stream->pcm[write] =
                static_cast<int16_t>(mono);

            stream->pcm_time_us[write] =
                source_arrival_us;

            write =
                (write + 1) &
                (kStreamRingFrames - 1);
        }

        if (YasuFastMeasurementsEnabled() &&
            dropped_frames != 0 &&
            source_arrival_us != 0) {
            stats.pcm_overruns.fetch_add(
                1,
                std::memory_order_relaxed);

            stats.pcm_overrun_frames.fetch_add(
                dropped_frames,
                std::memory_order_relaxed);
        }

        stream->pcm_write.store(
            write,
            std::memory_order_release);

        if (YasuFastMeasurementsEnabled() &&
            source_arrival_us != 0 &&
            frames_to_write != 0) {
            stats.pcm_frames_written.fetch_add(
                static_cast<uint64_t>(frames_to_write));

            const uint32_t final_depth =
                write >= read
                    ? write - read
                    : kStreamRingFrames - read + write;

            Stats::AddMax(
                stats.max_ring_depth,
                static_cast<uint64_t>(final_depth));

            const uint64_t now = NowUs();

            if (now >= source_arrival_us) {
                stats.PacketToPcm(
                    now - source_arrival_us);
            }
        }
    }

    int DecodePacket(
        Stream* stream,
        const uint8_t* data,
        int size,
        bool plc,
        bool fec,
        uint64_t arrival_us,
        int frame_samples = 0) {

        // Only one packet is decoded at a time because the packet
        // path is serialized. thread_local avoids stack churn.
        thread_local int16_t decoded[
            kDecodeSamples * 2];

        const bool measure =
            YasuFastMeasurementsEnabled();

        // FEC must only be attempted when the packet actually
        // carries Opus in-band FEC. Otherwise opus_decode(..., 1)
        // must not be treated as successful recovery.
        if (fec &&
            (!data ||
             size <= 0 ||
             WebRtcOpus_PacketHasFec(data, static_cast<size_t>(size)) != 1)) {
            return 0;
        }

        // PLC/FEC must be asked for exactly the duration that is missing
        // (multiple of 2.5 ms). The old code always asked PLC for 120 ms.
        //
        // For normal packets, determine the actual Opus packet duration
        // first instead of always handing opus_decode() the full 120 ms
        // output capacity. This keeps the normal decode working set small
        // while preserving support for larger valid Opus packets.
        int decode_frame_size = 480;

        if (!plc && !fec) {
            int packet_samples =
                opus_packet_get_nb_samples(
                    data,
                    size,
                    48000);

            if (packet_samples > 0) {
                packet_samples -= packet_samples % 120;

                if (packet_samples >= 120 &&
                    packet_samples <= kDecodeSamples) {
                    decode_frame_size = packet_samples;
                }
            }
        }

        if (fec) {
            int fec_frame_size = frame_samples;
            if (fec_frame_size <= 0) {
                fec_frame_size =
                    opus_packet_get_samples_per_frame(
                        data,
                        48000);
            }

            if (fec_frame_size <= 0 ||
                fec_frame_size > kDecodeSamples) {
                return 0;
            }

            fec_frame_size -= fec_frame_size % 120;
            if (fec_frame_size <= 0) {
                return 0;
            }

            decode_frame_size = fec_frame_size;
        } else if (plc) {
            int plc_size = frame_samples > 0 ? frame_samples : 960;
            plc_size -= plc_size % 120;
            if (plc_size < 120) {
                plc_size = 120;
            }
            if (plc_size > kDecodeSamples) {
                plc_size = kDecodeSamples;
            }
            decode_frame_size = plc_size;
        }

        const uint64_t start =
            measure ? NowUs() : 0;

        const int samples =
            opus_decode(
                stream->decoder,
                data,
                size,
                decoded,
                decode_frame_size,
                fec ? 1 : 0);

        if (measure) {
            const uint64_t cost =
                NowUs() - start;

            stats.DecodeCost(cost);
        }

        if (samples <= 0) {
            if (YasuFastMeasurementsEnabled()) {
                stats.packets_decode_failed.fetch_add(1);
            }

            static int yasu_decode_error_logs = 0;
            if (yasu_decode_error_logs < 10) {
                ++yasu_decode_error_logs;

                RTC_LOG(LS_ERROR)
                    << "YASU FAST OPUS DECODE FAILED"
                    << " error=" << samples
                    << " size=" << size
                    << " fec=" << (fec ? 1 : 0)
                    << " plc=" << (plc ? 1 : 0)
                    << " frame_size=" << decode_frame_size
                    << " decoder=" << static_cast<const void*>(stream->decoder);

                if (data && size > 0) {
                    RTC_LOG(LS_ERROR)
                        << "YASU FAST OPUS PAYLOAD"
                        << " b0=" << static_cast<int>(data[0])
                        << " b1=" << (size > 1 ? static_cast<int>(data[1]) : -1)
                        << " b2=" << (size > 2 ? static_cast<int>(data[2]) : -1)
                        << " b3=" << (size > 3 ? static_cast<int>(data[3]) : -1);
                }
            }

            return 0;
        }

        if (YasuFastMeasurementsEnabled()) {
            if (plc) {
                stats.packets_plc.fetch_add(1);
            } else if (fec) {
                stats.packets_fec.fetch_add(1);
                stats.packets_decoded.fetch_add(1);
            } else {
                stats.packets_decoded.fetch_add(1);
            }
        }

        if (!plc && !fec) {
            // Remember the real packet duration (works for any ptime).
            stream->last_packet_frames.store(
                static_cast<uint32_t>(samples),
                std::memory_order_relaxed);
        }

        PushPcm(
            stream,
            decoded,
            samples,
            arrival_us);

        return samples;
    }

    void DecodePlc(Stream* stream, int frame_samples) {
        DecodePacket(
            stream,
            nullptr,
            0,
            true,
            false,
            0,
            frame_samples);
    }

    int LastPacketFrames(Stream* stream) const {
        int n = static_cast<int>(
            stream->last_packet_frames.load(
                std::memory_order_relaxed));
        if (n < 120) n = 960;
        if (n > kDecodeSamples) n = kDecodeSamples;
        return n;
    }

    Packet* FindExact(
        Stream* stream,
        uint16_t sequence) {

        for (auto& packet : stream->packets) {
            if (packet.valid &&
                packet.sequence == sequence) {
                return &packet;
            }
        }

        return nullptr;
    }

    Packet* FindNearestAhead(
        Stream* stream,
        uint16_t expected,
        uint16_t* distance_out) {

        Packet* nearest = nullptr;
        uint16_t nearest_distance = 0xffff;

        for (auto& packet : stream->packets) {
            if (!packet.valid) {
                continue;
            }

            if (!SeqAhead(
                    packet.sequence,
                    expected)) {
                continue;
            }

            const uint16_t distance =
                SeqForwardDistance(
                    packet.sequence,
                    expected);

            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = &packet;
            }
        }

        if (distance_out) {
            *distance_out =
                nearest ? nearest_distance : 0xffff;
        }

        return nearest;
    }

    void UpdateJitterPad(
        Stream* stream,
        uint32_t timestamp,
        uint64_t arrival_us) {

        if (!stream->have_jitter_base) {
            stream->have_jitter_base = true;
            stream->jitter_first_ts = timestamp;
            stream->jitter_first_arrival_us = arrival_us;
            stream->jitter_base_us = 0;
            return;
        }

        const int64_t media_us =
            static_cast<int64_t>(
                static_cast<int32_t>(
                    timestamp - stream->jitter_first_ts)) *
            1000000LL / 48000LL;

        const int64_t wall_us =
            static_cast<int64_t>(arrival_us) -
            static_cast<int64_t>(stream->jitter_first_arrival_us);

        const int64_t d = wall_us - media_us;

        // Running minimum with a slow upward drift (clock skew tolerance).
        if (d < stream->jitter_base_us) {
            stream->jitter_base_us = d;
        } else {
            stream->jitter_base_us += 100;
        }

        int64_t rel = d - stream->jitter_base_us;
        if (rel < 0) rel = 0;
        if (rel > 2000000) rel = 2000000;

        stream->jitter_hist_us[stream->jitter_hist_pos] =
            static_cast<int32_t>(rel);
        stream->jitter_hist_pos =
            (stream->jitter_hist_pos + 1) % kJitterHistory;
        if (stream->jitter_hist_n < kJitterHistory) {
            ++stream->jitter_hist_n;
        }

        // Do not recompute the percentile on every 10 ms packet.
        // Four packets still gives a 40 ms reaction window while
        // substantially reducing hot-path jitter analysis cost.
        ++stream->jitter_update_counter;

        if (stream->jitter_hist_n < 8 ||
            (stream->jitter_update_counter & 3u) != 0) {
            return;
        }

        int32_t sorted[kJitterHistory];
        std::memcpy(
            sorted,
            stream->jitter_hist_us,
            sizeof(int32_t) * stream->jitter_hist_n);

        const int idx =
            (stream->jitter_hist_n * 95) / 100;

        // nth_element is sufficient for P95 and avoids a full sort.
        std::nth_element(
            sorted,
            sorted + std::min(
                idx,
                stream->jitter_hist_n - 1),
            sorted + stream->jitter_hist_n);

        const int32_t p95_us =
            sorted[std::min(
                idx,
                stream->jitter_hist_n - 1)];

        // Convert jitter to 48 kHz frames.
        double target =
            static_cast<double>(p95_us) * 48.0 / 1000.0;
        target = std::max<double>(target, kPadMinFrames);
        target = std::min<double>(target, kPadMaxFrames);

        // Fast attack (protect against bursts), slow decay (low latency).
        if (target > stream->pad_smooth) {
            stream->pad_smooth = target;
        } else {
            // Fast decay: once network jitter settles, release
            // accumulated playback padding quickly instead of carrying
            // old latency forward.
            stream->pad_smooth =
                stream->pad_smooth * 0.80 + target * 0.20;
        }

        // Recovery logic must never be able to increase the latency budget.
        stream->pad_smooth = std::min<double>(
            stream->pad_smooth,
            static_cast<double>(kPadMaxFrames));

        stream->pad_frames.store(
            static_cast<uint32_t>(stream->pad_smooth),
            std::memory_order_relaxed);

        // Feed the measured RTP arrival jitter directly into the
        // adaptive network controller. Do not derive network pressure
        // from pad_smooth because the pad has an intentional minimum.
        UpdateAdaptiveNetwork(
            stream,
            arrival_us,
            static_cast<uint32_t>(
                std::min<int64_t>(
                    std::max<int64_t>(0, p95_us),
                    12000)));
    }

    void UpdateAdaptiveNetwork(
        Stream* stream,
        uint64_t now_us,
        uint32_t jitter_us = 0,
        bool reorder_event = false,
        bool loss_event = false,
        bool late_event = false) {

        if (now_us == 0) {
            return;
        }

        if (stream->network_last_update_us != 0 &&
            now_us >= stream->network_last_update_us) {

            const uint64_t elapsed =
                now_us - stream->network_last_update_us;

            // Fast enough to react to a bad burst, but never lets one
            // packet permanently poison the network state.
            const double decay =
                elapsed >= kAdaptiveDecayUs
                    ? 0.0
                    : static_cast<double>(
                          kAdaptiveDecayUs - elapsed) /
                      static_cast<double>(kAdaptiveDecayUs);

            stream->network_reorder_score *= decay;
            stream->network_loss_score *= decay;
        }

        stream->network_last_update_us = now_us;

        if (jitter_us != 0) {
            const double sample =
                static_cast<double>(
                    std::min<uint32_t>(jitter_us, 12000));

            if (stream->network_jitter_ewma_us == 0.0) {
                stream->network_jitter_ewma_us = sample;
            } else {
                // Fast attack, faster recovery.
                const double alpha =
                    sample > stream->network_jitter_ewma_us
                        ? 0.35
                        : 0.18;

                stream->network_jitter_ewma_us =
                    stream->network_jitter_ewma_us * (1.0 - alpha) +
                    sample * alpha;
            }
        }

        if (reorder_event) {
            stream->network_reorder_score =
                std::min(1.0,
                    stream->network_reorder_score + 0.22);
            stream->network_last_event_us = now_us;
        }

        if (loss_event) {
            stream->network_loss_score =
                std::min(1.0,
                    stream->network_loss_score + 0.30);
            stream->network_last_event_us = now_us;
        }

        if (late_event) {
            stream->network_loss_score =
                std::min(1.0,
                    stream->network_loss_score + 0.08);
        }

        const double jitter_pressure =
            std::min(
                1.0,
                stream->network_jitter_ewma_us / 1800.0);

        // Reordering is the only strong reason to wait for an ahead packet.
        // High jitter alone must NOT create playback latency.
        const double reorder_pressure =
            stream->network_reorder_score;

        const double loss_pressure =
            stream->network_loss_score;

        // Network quality score is useful for classification/telemetry,
        // but it is deliberately NOT used as the reorder timer directly.
        double pressure =
            jitter_pressure * kAdaptiveJitterWeight +
            reorder_pressure * kAdaptiveReorderWeight +
            loss_pressure * kAdaptiveLossWeight;

        // Genuine loss means freshness is more important than waiting.
        pressure -= loss_pressure * 0.30;

        pressure = std::max(0.0, std::min(1.0, pressure));

        // Healthy periods rapidly release accumulated network pressure.
        if (stream->network_last_event_us != 0 &&
            now_us > stream->network_last_event_us &&
            now_us - stream->network_last_event_us >
                kAdaptiveRecoveryUs) {

            pressure *= 0.45;
            stream->network_reorder_score *= 0.65;
            stream->network_loss_score *= 0.65;
        }

        stream->network_pressure =
            stream->network_pressure * 0.60 +
            pressure * 0.40;

        if (stream->network_pressure < 0.15) {
            stream->network_level = 0;
        } else if (stream->network_pressure < 0.30) {
            stream->network_level = 1;
        } else if (stream->network_pressure < 0.50) {
            stream->network_level = 2;
        } else if (stream->network_pressure < 0.70) {
            stream->network_level = 3;
        } else if (stream->network_pressure < 0.85) {
            stream->network_level = 4;
        } else {
            stream->network_level = 5;
        }

        // Reorder wait is driven primarily by measured reordering.
        // Jitter contributes only a small secondary component.
        double wait =
            static_cast<double>(kAdaptiveReorderMinUs) +
            stream->network_reorder_score * 700.0 +
            jitter_pressure * 120.0;

        // Packet loss actively pulls the wait back down.
        wait -= loss_pressure * 300.0;

        wait = std::max<double>(
            kAdaptiveReorderMinUs,
            std::min<double>(
                kAdaptiveReorderMaxUs,
                wait));

        stream->adaptive_reorder_wait_us =
            static_cast<uint64_t>(wait);

        // Severe loss/recovery: aggressively favor fresh audio.
        if (stream->network_level >= 4 || loss_event) {
            stream->adaptive_reorder_wait_us =
                std::min<uint64_t>(
                    stream->adaptive_reorder_wait_us,
                    500);
        }

        if (loss_event) {
            stream->network_recovery_until_us =
                now_us + kAdaptiveRecoveryUs;
        }
    }

    uint64_t GetReorderWaitUs(
        const Stream* stream,
        uint64_t now_us) const {

        uint64_t wait =
            stream->adaptive_reorder_wait_us;

        if (stream->network_recovery_until_us != 0 &&
            now_us < stream->network_recovery_until_us) {
            // Loss/recovery: do not sit on the missing packet.
            wait = std::min<uint64_t>(wait, 500);
        }

        return std::max<uint64_t>(
            kAdaptiveReorderMinUs,
            std::min<uint64_t>(
                kAdaptiveReorderMaxUs,
                wait));
    }

    // Re-run Drain() for one stream after a short delay. Without this a
    // packet that is waiting for its reordered predecessor would sit until
    // the NEXT packet arrives (up to one full packet duration later).
    void ScheduleReorderDrain(Stream* stream, uint64_t wait_us) {
        if (stream->reorder_timer_posted || !fast_queue) {
            return;
        }
        stream->reorder_timer_posted = true;

        const uint32_t ssrc = stream->ssrc;
        const uint64_t gen = generation.load(std::memory_order_acquire);
        // Keep timer scheduling overhead below 0.25 ms.
        const int64_t delay_us =
            static_cast<int64_t>(wait_us) + 250;

        fast_queue->PostDelayedTask(
            [this, ssrc, gen]() {
                if (resetting.load(std::memory_order_acquire) ||
                    generation.load(std::memory_order_acquire) != gen) {
                    return;
                }
                Lock();
                if (!resetting.load(std::memory_order_acquire) &&
                    generation.load(std::memory_order_acquire) == gen) {
                    Stream* st = Find(ssrc);
                    if (st) {
                        st->reorder_timer_posted = false;
                        Drain(st);
                    }
                }
                Unlock();
            },
            webrtc::TimeDelta::Micros(delay_us));
    }

    void Drain(Stream* stream) {
        if (!stream->have_sequence) {
            // Start with the OLDEST buffered packet (lowest sequence),
            // not simply the first occupied slot.
            Packet* first = nullptr;

            for (auto& packet : stream->packets) {
                if (!packet.valid) {
                    continue;
                }
                if (!first ||
                    SeqBehind(packet.sequence, first->sequence)) {
                    first = &packet;
                }
            }

            if (!first) {
                return;
            }

            const uint16_t first_sequence = first->sequence;
            const uint32_t first_timestamp = first->timestamp;

            const int first_samples =
                DecodePacket(
                    stream,
                    first->data,
                    first->size,
                    false,
                    false,
                    first->arrival_us);

            stream->next_sequence =
                static_cast<uint16_t>(
                    first_sequence + 1);

            stream->next_timestamp =
                first_timestamp +
                static_cast<uint32_t>(
                    first_samples > 0 ? first_samples : 960);

            stream->have_timestamp = true;

            first->valid = false;

            stream->have_sequence = true;
        }

        for (;;) {
            Packet* exact =
                FindExact(
                    stream,
                    stream->next_sequence);

            if (exact) {
                const uint16_t seq =
                    exact->sequence;

                const uint32_t timestamp =
                    exact->timestamp;

                const uint64_t arrival =
                    exact->arrival_us;

                const int samples =
                    DecodePacket(
                        stream,
                        exact->data,
                        exact->size,
                        false,
                        false,
                        arrival);

                exact->valid = false;

                stream->next_sequence =
                    static_cast<uint16_t>(
                        seq + 1);

                stream->next_timestamp =
                    timestamp +
                    static_cast<uint32_t>(
                        samples > 0 ? samples : 960);

                stream->have_timestamp = true;

                continue;
            }

            uint16_t gap = 0;

            Packet* ahead =
                FindNearestAhead(
                    stream,
                    stream->next_sequence,
                    &gap);

            if (!ahead) {
                break;
            }

            /*
             * `gap` packets are missing before `ahead`.
             *
             * 1) Give a reordered packet a short chance to show up
             *    (timer-driven, never stalls until the next packet).
             * 2) Conceal each missing packet with the REAL packet
             *    duration (not a fixed 10 ms / 120 ms guess).
             * 3) The last missing packet is recovered with Opus in-band
             *    FEC when the next packet carries it (FEC decodes the
             *    END of the lost packet, so PLC covers the earlier part).
             * 4) Never conceal more than kMaxConcealPackets in a row:
             *    resync instead of queueing seconds of fake audio.
             */
            if (gap == 1) {
                const uint64_t now_us = NowUs();
                const uint64_t ahead_arrival = ahead->real_arrival_us;
                const uint64_t reorder_wait_us =
                    GetReorderWaitUs(stream, now_us);

                if (ahead_arrival != 0 &&
                    now_us >= ahead_arrival &&
                    now_us - ahead_arrival < reorder_wait_us) {
                    ScheduleReorderDrain(
                        stream,
                        reorder_wait_us - (now_us - ahead_arrival));
                    break;
                }

                // A packet remained missing long enough to require
                // concealment. Feed actual loss into the adaptive
                // controller. Loss reduces waiting rather than increasing
                // latency.
                UpdateAdaptiveNetwork(
                    stream,
                    now_us,
                    0,
                    false,
                    true,
                    false);
            } else {
                // Multiple missing packets are one loss burst, not two
                // independent events. Register it once; the adaptive
                // controller already raises loss pressure and shortens
                // reorder waiting.
                UpdateAdaptiveNetwork(
                    stream,
                    NowUs(),
                    0,
                    false,
                    true,
                    false);
            }

            const int lost_frames = LastPacketFrames(stream);

            if (gap > kMaxConcealPackets) {
                // Long outage: one short concealment, then resync.
                DecodePlc(stream, lost_frames);
                stream->next_sequence = ahead->sequence;
                stream->next_timestamp = ahead->timestamp;
                continue;
            }

            if (gap > 1) {
                DecodePlc(stream, lost_frames);
                stream->next_sequence =
                    static_cast<uint16_t>(
                        stream->next_sequence + 1);
                stream->next_timestamp +=
                    static_cast<uint32_t>(lost_frames);
                continue;
            }

            // gap == 1: last missing packet -> FEC (+PLC for the rest).
            bool recovered = false;

            if (ahead->size > 0 &&
                WebRtcOpus_PacketHasFec(
                    ahead->data,
                    static_cast<size_t>(ahead->size)) == 1) {

                int fec_part =
                    opus_packet_get_samples_per_frame(
                        ahead->data,
                        48000);

                if (fec_part > 0) {
                    if (fec_part > lost_frames) {
                        fec_part = lost_frames;
                    }
                    fec_part -= fec_part % 120;

                    const int plc_part = lost_frames - fec_part;

                    if (fec_part > 0) {
                        if (plc_part >= 120) {
                            DecodePlc(stream, plc_part);
                        }

                        recovered =
                            DecodePacket(
                                stream,
                                ahead->data,
                                ahead->size,
                                false,
                                true,
                                ahead->arrival_us,
                                fec_part) > 0;
                    }
                }
            }

            if (!recovered) {
                DecodePlc(stream, lost_frames);
            }

            stream->next_sequence =
                static_cast<uint16_t>(
                    stream->next_sequence + 1);

            stream->next_timestamp +=
                static_cast<uint32_t>(lost_frames);

            // `ahead` is decoded by the exact lookup on the next loop turn.
        }
    }

    int ReadStream(
        Stream* stream,
        int32_t* mix,
        uint8_t* contributors,
        int frames) {

        const bool measure =
            YasuFastMeasurementsEnabled();

        uint32_t read =
            stream->pcm_read.load(
                std::memory_order_relaxed);

        const uint32_t write =
            stream->pcm_write.load(
                std::memory_order_acquire);

        const uint32_t used =
            write >= read
                ? write - read
                : kStreamRingFrames - read + write;

        // YASU ULTRA-LOW-LATENCY BACKLOG CONTROL.
        //
        // Diagnostics showed:
        //   packet_to_pcm  ~= 2.2 ms
        //   packet_to_play ~= 76.9 ms
        //
        // Therefore the dominant latency is PCM residence in this ring.
        //
        // Keep only:
        //   adaptive jitter pad + one packet + 2.5 ms headroom.
        //
        // If PCM accumulates beyond that level, remove the oldest excess
        // immediately. Prefer removing silence first. If the excess is
        // audible, accept a very small skip rather than keeping tens of
        // milliseconds of stale audio.

        const uint32_t pad =
            stream->pad_frames.load(
                std::memory_order_relaxed);

        const uint32_t pkt =
            stream->last_packet_frames.load(
                std::memory_order_relaxed);

        // Keep the PCM residence window as small as possible.
        // Do not reserve a full codec packet unconditionally: the
        // packet size is a decode quantum, not required playback latency.
        //
        // Target:
        //   adaptive network pad + small safety headroom.
        //
        // A full packet is only useful when the stream is actually
        // accumulating; allowing it permanently would add unnecessary
        // latency even on a healthy network.
        const uint32_t packet_guard =
            std::min<uint32_t>(
                pkt,
                480);  // <= 10 ms @ 48 kHz

        const uint32_t high =
            std::min<uint32_t>(
                pad + packet_guard + kTrimHeadroomFrames,
                kStreamRingFrames - 1);

        if (used > high) {
            const uint32_t excess =
                used - high;

            // Low-latency backlog recovery:
            // Never allow a large PCM backlog to drain slowly over many
            // callbacks. That would turn temporary network/callback
            // imbalance into tens or hundreds of milliseconds of latency.
            //
            // First discard as much old silence as possible. If the
            // backlog is still above the target, jump directly to the
            // newest low-latency window.
            // Keep realtime callback work bounded. Never scan a large
            // backlog sample-by-sample inside the AAudio callback.
            constexpr uint32_t kSilenceProbeFrames = 240; // 5 ms

            const uint32_t max_silence_probe =
                std::min<uint32_t>(
                    std::min<uint32_t>(
                        excess,
                        kSilenceProbeFrames),
                    used > 1 ? used - 1 : 0);

            uint32_t silence_frames = 0;

            // Only inspect a bounded 5 ms window. Larger backlogs are
            // handled by the direct low-latency jump below.
            const uint32_t silence_probe =
                max_silence_probe;

            while (silence_frames < silence_probe) {
                const int32_t v =
                    stream->pcm[
                        (read + silence_frames) &
                        (kStreamRingFrames - 1)];

                if (v > 300 || v < -300) {
                    break;
                }

                ++silence_frames;
            }

            if (silence_frames != 0) {
                read =
                    (read + silence_frames) &
                    (kStreamRingFrames - 1);
            }

            // Recalculate the remaining backlog after removing silence.
            const uint32_t remaining_used =
                write >= read
                    ? write - read
                    : kStreamRingFrames - read + write;

            if (remaining_used > high &&
                remaining_used - high >= kHardTrimExcessFrames) {
                // Only perform a hard resync for a genuinely large
                // backlog. Small/medium backlog is preserved so the
                // realtime output path does not repeatedly skip audible
                // PCM and create chopping.
                const uint32_t keep = high;

                read =
                    (write + kStreamRingFrames - keep) &
                    (kStreamRingFrames - 1);
            }
        }

        int count = 0;
        while (count < frames &&
               read != write) {

            const uint64_t stamp =
                stream->pcm_time_us[read];

            mix[count] +=
                static_cast<int32_t>(stream->pcm[read]);

            if (contributors) {
                if (contributors[count] < 255) {
                    ++contributors[count];
                }
            }

            if (measure &&
                stamp != 0) {

                const uint64_t now = NowUs();

                if (now >= stamp) {
                    const uint64_t residence =
                        now - stamp;

                    stats.Residence(residence);
                    stats.PacketToPlay(residence);
                }
            }

            ++count;

            read =
                (read + 1) & (kStreamRingFrames - 1);

        }

        stream->pcm_read.store(
            read,
            std::memory_order_release);

        return count;
    }

    int ReadPcm(
        int16_t* output,
        int frames,
        int channels,
        int sample_rate) {

        struct RealtimeReaderGuard {
            Impl* impl;

            explicit RealtimeReaderGuard(Impl* value)
                : impl(value) {
                impl->active_readers.fetch_add(
                    1,
                    std::memory_order_acq_rel);
            }

            ~RealtimeReaderGuard() {
                impl->active_readers.fetch_sub(
                    1,
                    std::memory_order_acq_rel);
            }
        } reader_guard(this);

        const bool measure =
            YasuFastMeasurementsEnabled();

        // Diagnostics are completely outside the normal hot path.
        if (measure) {
            stats.readpcm_calls.fetch_add(1, std::memory_order_relaxed);
            stats.readpcm_last_sample_rate.store(
                sample_rate,
                std::memory_order_relaxed);
            stats.readpcm_last_channels.store(
                channels,
                std::memory_order_relaxed);
        }

        const uint64_t start =
            measure ? NowUs() : 0;

        if (!output ||
            frames <= 0 ||
            channels <= 0) {

            if (measure) {
                stats.readpcm_invalid_calls.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }

            return 0;
        }

        const bool is_resetting =
            resetting.load(std::memory_order_acquire);

        if (measure) {
            if (!is_resetting) {
                stats.readpcm_enabled_calls.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }

            if (sample_rate != 48000) {
                stats.readpcm_wrong_rate_calls.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
        }

        if (is_resetting ||
            sample_rate != 48000) {

            std::memset(
                output,
                0,
                static_cast<size_t>(frames) *
                    static_cast<size_t>(channels) *
                    sizeof(int16_t));

            return 0;
        }

        int32_t mix[kOutputChunk];
        uint8_t contributors[kOutputChunk];

        int remaining = frames;
        int offset = 0;

        while (remaining > 0) {
            const int chunk =
                std::min(
                    remaining,
                    kOutputChunk);

            std::memset(
                mix,
                0,
                static_cast<size_t>(chunk) *
                    sizeof(int32_t));

            std::memset(
                contributors,
                0,
                static_cast<size_t>(chunk) *
                    sizeof(uint8_t));

            for (auto& stream : streams) {
                if (!stream.active.load(
                        std::memory_order_acquire)) {
                    continue;
                }

                ReadStream(
                    &stream,
                    mix,
                    contributors,
                    chunk);
            }

            // Count an underrun only when the final mixed output
            // has no source PCM at all.
            uint64_t missing_frames = 0;

            for (int i = 0; i < chunk; ++i) {
                if (contributors[i] == 0) {
                    ++missing_frames;
                }
            }

            if (missing_frames != 0) {
                if (measure) {
                    stats.readpcm_zero_source_calls.fetch_add(
                        1,
                        std::memory_order_relaxed);
                }
                if (measure) {
                    stats.pcm_underruns.fetch_add(
                        missing_frames);
                }
            }

            for (int i = 0; i < chunk; ++i) {
                const int16_t value =
                    contributors[i] != 0
                        ? SoftLimit(mix[i])
                        : static_cast<int16_t>(0);

                for (int channel = 0;
                     channel < channels;
                     ++channel) {

                    output[
                        (offset + i) * channels +
                        channel] = value;
                }
            }

            if (measure) {
                stats.pcm_frames_read.fetch_add(
                    static_cast<uint64_t>(chunk));
            }

            offset += chunk;
            remaining -= chunk;
        }

        if (Measurements()) {
            stats.Callback(
                NowUs() - start,
                frames);
        }

        return frames;
    }

};

YasuFastAudioCore&
YasuFastAudioCore::Instance() {
    static YasuFastAudioCore instance;
    return instance;
}

void YasuFastAudioCore::RegisterGroupSsrc(uint32_t ssrc) {
    if (ssrc == 0) {
        return;
    }

    for (auto& slot : impl_->group_ssrcs) {
        uint32_t expected = 0;

        if (slot.compare_exchange_strong(
                expected,
                ssrc,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return;
        }

        if (expected == ssrc) {
            return;
        }
    }
}

void YasuFastAudioCore::UnregisterGroupSsrc(uint32_t ssrc) {
    if (ssrc == 0) {
        return;
    }

    for (auto& slot : impl_->group_ssrcs) {
        uint32_t current =
            slot.load(std::memory_order_acquire);

        if (current == ssrc) {
            slot.compare_exchange_strong(
                current,
                0,
                std::memory_order_acq_rel,
                std::memory_order_acquire);
            return;
        }
    }
}

bool YasuFastAudioCore::IsGroupSsrc(uint32_t ssrc) const {
    if (ssrc == 0) {
        return false;
    }

    for (const auto& slot : impl_->group_ssrcs) {
        if (slot.load(std::memory_order_acquire) == ssrc) {
            return true;
        }
    }

    return false;
}

bool YasuFastAudioCore::HasRegisteredGroupSsrc() const {
    for (const auto& slot : impl_->group_ssrcs) {
        if (slot.load(std::memory_order_acquire) != 0) {
            return true;
        }
    }
    return false;
}


YasuFastAudioCore::YasuFastAudioCore()
    : impl_(new Impl()) {
    impl_->task_queue_factory =
        webrtc::CreateDefaultTaskQueueFactory();

    impl_->fast_queue =
        impl_->task_queue_factory->CreateTaskQueue(
            "yasu-fast-audio",
            webrtc::TaskQueueFactory::Priority::HIGH);

    impl_->stats.Reset();
}

YasuFastAudioCore::~YasuFastAudioCore() {
    Reset();

    delete impl_;
}

void YasuFastAudioCore::Reset() {
    // Stop new FAST readers/packets while the call state is being cleared.
    impl_->resetting.store(
        true,
        std::memory_order_release);

    // Wait only for callbacks that were already inside ReadPcm().
    // No mutex is taken by the realtime callback.
    while (impl_->active_readers.load(
               std::memory_order_acquire) != 0) {
        std::this_thread::yield();
    }

    // Invalidate every queued packet belonging to the previous call.
    impl_->generation.fetch_add(
        1,
        std::memory_order_acq_rel);

    impl_->LockPacketQueue();

    // The generation change above makes any in-flight packet obsolete.
    // Clear its ownership so the worker will not advance the queue after
    // Reset() has already moved the read/write boundary.
    impl_->packet_queue_inflight_active = false;
    impl_->packet_queue_read = impl_->packet_queue_write;

    impl_->UnlockPacketQueue();

    impl_->Lock();

    for (auto& stream : impl_->streams) {
        if (stream.decoder) {
            opus_decoder_destroy(
                stream.decoder);

            stream.decoder = nullptr;
        }

        stream.active.store(
            false,
            std::memory_order_release);

        stream.ssrc = 0;
        stream.ResetAdaptive();

        stream.have_sequence = false;
        stream.next_sequence = 0;

        stream.have_timestamp = false;
        stream.next_timestamp = 0;

        stream.last_packet_us.store(
            0,
            std::memory_order_release);

        stream.pcm_read.store(
            0,
            std::memory_order_release);

        stream.pcm_write.store(
            0,
            std::memory_order_release);
    }

    impl_->stats.Reset();

    for (auto& slot : impl_->group_ssrcs) {
        slot.store(
            0,
            std::memory_order_release);
    }

    impl_->Unlock();

    // FAST is permanently active. Resetting only blocks the path
    // while this call's state is being cleared.
    impl_->resetting.store(
        false,
        std::memory_order_release);
}

void YasuFastAudioCore::PushPacket(
    uint32_t ssrc,
    uint16_t sequence,
    uint32_t timestamp,
    const uint8_t* payload,
    size_t payload_size) {

    if (impl_->resetting.load(std::memory_order_acquire) ||
        payload == nullptr ||
        payload_size == 0 ||
        payload_size > kMaxPayload) {
        return;
    }

    const uint64_t packet_generation =
        impl_->generation.load(std::memory_order_acquire);

    bool need_post = false;

    impl_->LockPacketQueue();

    if (impl_->resetting.load(std::memory_order_acquire) ||
        impl_->generation.load(std::memory_order_acquire) !=
            packet_generation) {
        impl_->UnlockPacketQueue();
        return;
    }

    const uint32_t write = impl_->packet_queue_write;
    const uint32_t next = impl_->PacketQueueNext(write);

    // The read slot can be owned by the FAST worker while ProcessPacket()
    // is decoding it. Never recycle that slot until the worker releases it.
    if (next == impl_->packet_queue_read) {
        if (impl_->packet_queue_inflight_active &&
            impl_->packet_queue_read ==
                impl_->packet_queue_inflight) {

            // The queue is full while one slot is actively being decoded.
            // Drop the newest packet instead of advancing read and
            // corrupting the in-flight payload.
            if (YasuFastMeasurementsEnabled()) {
                impl_->stats.packets_dropped.fetch_add(1);
            }

            impl_->UnlockPacketQueue();
            return;
        }

        // Normal bounded low-latency overflow: discard the oldest queued
        // packet so the queue cannot accumulate unbounded delay.
        impl_->packet_queue_read =
            impl_->PacketQueueNext(impl_->packet_queue_read);

        if (YasuFastMeasurementsEnabled()) {
            impl_->stats.packets_dropped.fetch_add(1);
        }
    }

    auto& queued = impl_->packet_queue[write];

    queued.ssrc = ssrc;
    queued.sequence = sequence;
    queued.timestamp = timestamp;
    queued.size = static_cast<uint16_t>(payload_size);
    queued.generation = packet_generation;

    // Arrival time is part of FAST jitter control, not just diagnostics.
    queued.arrival_us = NowUs();

    // This remains the only payload copy in the FAST handoff.
    std::memcpy(
        queued.data,
        payload,
        payload_size);

    impl_->packet_queue_write = next;

    need_post = !impl_->packet_task_posted.exchange(
        true,
        std::memory_order_acq_rel);

    impl_->UnlockPacketQueue();

    if (!need_post) {
        return;
    }

    impl_->fast_queue->PostTask(
        [this]() {
            for (;;) {
                uint32_t packet_ssrc = 0;
                uint16_t packet_sequence = 0;
                uint32_t packet_timestamp = 0;
                uint16_t packet_size = 0;
                uint64_t packet_generation = 0;
                uint64_t packet_arrival_us = 0;
                const uint8_t* packet_data = nullptr;
                uint32_t packet_slot = 0;

                impl_->LockPacketQueue();

                if (impl_->PacketQueueEmpty() ||
                    impl_->packet_queue_inflight_active) {

                    // Mark idle while holding the queue lock so a producer
                    // cannot miss the transition.
                    impl_->packet_task_posted.store(
                        false,
                        std::memory_order_release);

                    // Close the producer race before leaving the worker.
                    if (!impl_->PacketQueueEmpty() &&
                        !impl_->packet_queue_inflight_active) {
                        impl_->packet_task_posted.store(
                            true,
                            std::memory_order_release);
                        impl_->UnlockPacketQueue();
                        continue;
                    }

                    impl_->UnlockPacketQueue();
                    return;
                }

                packet_slot = impl_->packet_queue_read;

                auto& queued =
                    impl_->packet_queue[packet_slot];

                packet_ssrc = queued.ssrc;
                packet_sequence = queued.sequence;
                packet_timestamp = queued.timestamp;
                packet_size = queued.size;
                packet_generation = queued.generation;
                packet_arrival_us = queued.arrival_us;

                // Transfer ownership of this queue slot to the worker.
                // The producer is now forbidden from recycling this slot
                // until ProcessPacket() returns.
                impl_->packet_queue_inflight = packet_slot;
                impl_->packet_queue_inflight_active = true;

                // Directly decode from the queue slot.
                // No second payload memcpy is performed.
                packet_data = queued.data;

                impl_->UnlockPacketQueue();

                ProcessPacket(
                    packet_ssrc,
                    packet_sequence,
                    packet_timestamp,
                    packet_data,
                    packet_size,
                    packet_generation,
                    packet_arrival_us);

                impl_->LockPacketQueue();

                // Reset() may have invalidated this ownership while
                // ProcessPacket() was running. In that case it already
                // moved the queue boundary and we must not advance it again.
                if (impl_->packet_queue_inflight_active &&
                    impl_->packet_queue_inflight == packet_slot) {

                    impl_->packet_queue_read =
                        impl_->PacketQueueNext(packet_slot);

                    impl_->packet_queue_inflight_active = false;
                }

                impl_->UnlockPacketQueue();
            }
        });
}


std::string YasuFastAudioCore::GetDiagnostics() const {
    const Impl& impl = *impl_;
    const Stats& s = impl.stats;

    auto avg = [](uint64_t total, uint64_t count) -> uint64_t {
        return count ? total / count : 0;
    };

    auto min_or_zero = [](uint64_t value) -> uint64_t {
        return value == std::numeric_limits<uint64_t>::max()
            ? 0
            : value;
    };

    std::ostringstream out;

    const uint64_t queue_count =
        s.packet_queue_wait_count.load(std::memory_order_relaxed);
    const uint64_t decode_count =
        s.decode_count.load(std::memory_order_relaxed);
    const uint64_t pcm_count =
        s.packet_to_pcm_count.load(std::memory_order_relaxed);
    const uint64_t play_count =
        s.packet_to_play_count.load(std::memory_order_relaxed);
    const uint64_t residence_count =
        s.residence_count.load(std::memory_order_relaxed);
    const uint64_t callback_count =
        s.callback_count.load(std::memory_order_relaxed);

    out << "YASU FAST AUDIO DIAGNOSTICS\n";
    out << "started_us="
        << s.started_us.load(std::memory_order_relaxed) << '\n';

    out << "packets_received="
        << s.packets_received.load(std::memory_order_relaxed) << '\n';
    out << "packets_decoded="
        << s.packets_decoded.load(std::memory_order_relaxed) << '\n';
    out << "packets_plc="
        << s.packets_plc.load(std::memory_order_relaxed) << '\n';
    out << "packets_fec="
        << s.packets_fec.load(std::memory_order_relaxed) << '\n';
    out << "packets_late="
        << s.packets_late.load(std::memory_order_relaxed) << '\n';
    out << "packets_reordered="
        << s.packets_reordered.load(std::memory_order_relaxed) << '\n';
    out << "packets_dropped="
        << s.packets_dropped.load(std::memory_order_relaxed) << '\n';
    out << "packets_decode_failed="
        << s.packets_decode_failed.load(std::memory_order_relaxed) << '\n';

    out << "queue_wait_avg_us="
        << avg(
            s.packet_queue_wait_us.load(std::memory_order_relaxed),
            queue_count) << '\n';
    out << "queue_wait_min_us="
        << min_or_zero(
            s.packet_queue_wait_min_us.load(std::memory_order_relaxed))
        << '\n';
    out << "queue_wait_max_us="
        << s.packet_queue_wait_max_us.load(std::memory_order_relaxed) << '\n';

    out << "decode_avg_us="
        << avg(
            s.decode_cost_us.load(std::memory_order_relaxed),
            decode_count) << '\n';
    out << "decode_min_us="
        << min_or_zero(
            s.decode_cost_min_us.load(std::memory_order_relaxed))
        << '\n';
    out << "decode_max_us="
        << s.decode_cost_max_us.load(std::memory_order_relaxed) << '\n';

    out << "packet_to_pcm_avg_us="
        << avg(
            s.packet_to_pcm_us.load(std::memory_order_relaxed),
            pcm_count) << '\n';
    out << "packet_to_pcm_min_us="
        << min_or_zero(
            s.packet_to_pcm_min_us.load(std::memory_order_relaxed))
        << '\n';
    out << "packet_to_pcm_max_us="
        << s.packet_to_pcm_max_us.load(std::memory_order_relaxed) << '\n';

    out << "packet_to_play_avg_us="
        << avg(
            s.packet_to_play_us.load(std::memory_order_relaxed),
            play_count) << '\n';
    out << "packet_to_play_min_us="
        << min_or_zero(
            s.packet_to_play_min_us.load(std::memory_order_relaxed))
        << '\n';
    out << "packet_to_play_max_us="
        << s.packet_to_play_max_us.load(std::memory_order_relaxed) << '\n';

    out << "pcm_frames_written="
        << s.pcm_frames_written.load(std::memory_order_relaxed) << '\n';
    out << "pcm_frames_read="
        << s.pcm_frames_read.load(std::memory_order_relaxed) << '\n';

    out << "readpcm_calls="
        << s.readpcm_calls.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_enabled_calls="
        << s.readpcm_enabled_calls.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_invalid_calls="
        << s.readpcm_invalid_calls.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_wrong_rate_calls="
        << s.readpcm_wrong_rate_calls.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_zero_source_calls="
        << s.readpcm_zero_source_calls.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_last_sample_rate="
        << s.readpcm_last_sample_rate.load(std::memory_order_relaxed) << '\n';
    out << "readpcm_last_channels="
        << s.readpcm_last_channels.load(std::memory_order_relaxed) << '\n';

    out << "pcm_underruns="
        << s.pcm_underruns.load(std::memory_order_relaxed) << '\n';
    out << "pcm_overruns="
        << s.pcm_overruns.load(std::memory_order_relaxed) << '\n';
    out << "pcm_overrun_frames="
        << s.pcm_overrun_frames.load(std::memory_order_relaxed) << '\n';

    out << "residence_avg_us="
        << avg(
            s.residence_us.load(std::memory_order_relaxed),
            residence_count) << '\n';
    out << "residence_min_us="
        << min_or_zero(
            s.residence_min_us.load(std::memory_order_relaxed)) << '\n';
    out << "residence_max_us="
        << s.residence_max_us.load(std::memory_order_relaxed) << '\n';

    out << "callback_avg_us="
        << avg(
            s.callback_cost_us.load(std::memory_order_relaxed),
            callback_count) << '\n';
    out << "callback_min_us="
        << min_or_zero(
            s.callback_cost_min_us.load(std::memory_order_relaxed)) << '\n';
    out << "callback_max_us="
        << s.callback_cost_max_us.load(std::memory_order_relaxed) << '\n';

    out << "max_ring_depth="
        << s.max_ring_depth.load(std::memory_order_relaxed) << '\n';
    out << "active_streams_peak="
        << s.active_streams_peak.load(std::memory_order_relaxed) << '\n';

    return out.str();
}


int YasuFastAudioCore::ReadPcm(
    int16_t* output,
    int frames,
    int channels,
    int sample_rate) {
    return impl_->ReadPcm(
        output,
        frames,
        channels,
        sample_rate);
}

void YasuFastAudioCore::ProcessPacket(
    uint32_t ssrc,
    uint16_t sequence,
    uint32_t timestamp,
    const uint8_t* payload,
    size_t payload_size,
    uint64_t packet_generation,
    uint64_t packet_arrival_us) {

    if (impl_->resetting.load(std::memory_order_acquire) ||
        payload == nullptr ||
        payload_size == 0 ||
        payload_size > kMaxPayload) {
        return;
    }

    if (impl_->generation.load(std::memory_order_acquire) !=
        packet_generation) {
        return;
    }

    impl_->Lock();

    if (impl_->resetting.load(std::memory_order_acquire) ||
        impl_->generation.load(std::memory_order_acquire) !=
            packet_generation) {
        impl_->Unlock();
        return;
    }

    // Only currently registered Group Voice SSRCs may enter FAST.
    // This also prevents queued stale packets from recreating a
    // Stream after the Group Voice channel has been removed.
    if (!IsGroupSsrc(ssrc)) {
        impl_->Unlock();
        return;
    }

    const bool measure =
        YasuFastMeasurementsEnabled();

    // The packet already carries its arrival timestamp.
    // Avoid a second clock read on the normal FAST path.
    const uint64_t now =
        packet_arrival_us != 0
            ? packet_arrival_us
            : NowUs();

    const uint64_t arrival =
        packet_arrival_us;

    if (measure) {
        impl_->stats.packets_received.fetch_add(1);

        if (packet_arrival_us != 0 && now >= packet_arrival_us) {
            const uint64_t queue_wait =
                now - packet_arrival_us;

            impl_->stats.packet_queue_wait_count.fetch_add(1);
            impl_->stats.packet_queue_wait_us.fetch_add(queue_wait);
            Stats::AddMin(
                impl_->stats.packet_queue_wait_min_us,
                queue_wait);
            Stats::AddMax(
                impl_->stats.packet_queue_wait_max_us,
                queue_wait);
        }
    }

    // Reset() may have started after the initial check.
    // Never recreate FAST state while Reset() is clearing the call.
    if (impl_->resetting.load(std::memory_order_acquire)) {
        impl_->Unlock();
        return;
    }

    Stream* stream =
        impl_->Find(ssrc);

    if (!stream) {
        stream =
            impl_->Create(ssrc);
    }

    if (!stream) {
        if (measure) {
            impl_->stats.packets_dropped.fetch_add(1);
        }

        impl_->Unlock();
        return;
    }

    stream->last_packet_us.store(
        now,
        std::memory_order_release);

    if (stream->have_sequence) {
        const uint16_t expected =
            stream->next_sequence;

        if (sequence == expected) {
            // Exact next packet.
        } else if (SeqBehind(
                       sequence,
                       expected)) {

            // Already played / no longer useful.
            impl_->UpdateAdaptiveNetwork(
                stream,
                now,
                0,
                false,
                false,
                true);

            if (measure) {
                impl_->stats.packets_late.fetch_add(1);
            }

            impl_->Unlock();
            return;

        } else if (SeqAhead(
                       sequence,
                       expected)) {

            // Packet is ahead of playback. This is genuine reordering
            // information and feeds the adaptive reorder controller.
            impl_->UpdateAdaptiveNetwork(
                stream,
                now,
                0,
                true,
                false,
                false);

            if (measure) {
                impl_->stats.packets_reordered.fetch_add(1);
            }
        }
    }

    // Reject duplicate packet already sitting in the reorder slots.
    for (auto& packet : stream->packets) {
        if (packet.valid &&
            packet.sequence == sequence) {

            if (measure) {
                impl_->stats.packets_dropped.fetch_add(1);
            }

            impl_->Unlock();
            return;
        }
    }

    // FAST HOT PATH:
    // An in-order packet is already the exact packet needed for playback.
    // Do not copy it into the reorder buffer. Decode it immediately.
    //
    // Out-of-order packets still use the reorder buffer below.
    if (stream->have_sequence &&
        sequence == stream->next_sequence) {

        if (packet_arrival_us != 0) {
            impl_->UpdateJitterPad(
                stream,
                timestamp,
                packet_arrival_us);
        }

        const int samples =
            impl_->DecodePacket(
                stream,
                payload,
                static_cast<int>(payload_size),
                false,
                false,
                arrival);

        if (samples > 0) {
            stream->next_sequence =
                static_cast<uint16_t>(sequence + 1);

            stream->next_timestamp =
                timestamp +
                static_cast<uint32_t>(samples);

            stream->have_timestamp = true;

            // A previously buffered out-of-order packet may now become
            // playable immediately.
            impl_->Drain(stream);
        }

        impl_->Unlock();
        return;
    }

    Packet* destination = nullptr;

    for (auto& packet : stream->packets) {
        if (!packet.valid) {
            destination = &packet;
            break;
        }
    }

    if (!destination) {
        /*
         * Buffer is full.
         *
         * Drop the packet farthest ahead instead of destroying
         * the packet closest to playback.
         */
        uint16_t worst_distance = 0;
        Packet* worst = nullptr;

        for (auto& packet : stream->packets) {
            if (!packet.valid) {
                continue;
            }

            const uint16_t distance =
                stream->have_sequence
                    ? SeqForwardDistance(
                          packet.sequence,
                          stream->next_sequence)
                    : 0;

            if (!worst ||
                distance > worst_distance) {
                worst = &packet;
                worst_distance = distance;
            }
        }

        // If the incoming packet is farther ahead than every packet
        // already buffered, keep the existing reorder window and drop
        // the incoming packet instead of replacing a closer packet.
        if (stream->have_sequence) {
            const uint16_t incoming_distance =
                SeqForwardDistance(
                    sequence,
                    stream->next_sequence);

            if (!worst ||
                incoming_distance > worst_distance) {
                if (measure) {
                    impl_->stats.packets_dropped.fetch_add(1);
                }

                impl_->Unlock();
                return;
            }
        }

        destination = worst;

        if (measure) {
            impl_->stats.packets_dropped.fetch_add(1);
        }
    }

    if (!destination) {
        impl_->Unlock();
        return;
    }

    destination->sequence = sequence;
    destination->timestamp = timestamp;

    destination->size =
        static_cast<uint16_t>(
            payload_size);

    destination->arrival_us = arrival;
    destination->real_arrival_us = packet_arrival_us;

    // YASU JITTER ESTIMATOR: relative one-way delay = arrival time minus
    // RTP media time. Its 95th percentile (minus the running minimum)
    // sets the per-stream pad, so low-jitter streams stay low-latency and
    // bursty streams get exactly the protection they need.
    if (packet_arrival_us != 0) {
        impl_->UpdateJitterPad(
            stream,
            timestamp,
            packet_arrival_us);
    }

    std::memcpy(
        destination->data,
        payload,
        payload_size);

    destination->valid = true;

    impl_->Drain(stream);

    impl_->Unlock();
}

}  // namespace tgcalls
