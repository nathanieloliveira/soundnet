#ifndef MEDIA_ACCESS_H
#define MEDIA_ACCESS_H

#include "fsk_modulator.h"
#include "fsk_demodulator.h"
#include <boost/circular_buffer.hpp>
#include <boost/range.hpp>
#include <boost/dynamic_bitset.hpp>
#include <functional>
#include "fast_bitset.hpp"

enum class MEDIA_STATE
{
    FREE,
    OCCUPIED
};

enum class MODEM_STATE
{
    READY,
    RX,
    TX,
    BACKING_OFF
};

constexpr int PREAMBLE_SIZE = 15;
constexpr int PREAMBLE_SYMBOL_SEQ[] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
constexpr auto PREAMBLE_ARRAY = std::array<int, PREAMBLE_SIZE>{1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

enum ERRORS : uint8_t {
    INVALID_SIZE
};

constexpr size_t MIN_PACKET_SIZE     = 5;
constexpr size_t MAX_PACKET_SIZE     = 133; // this is the maximum size of a layer 2 packet
constexpr size_t MAX_FRAME_SIZE      = 128;

/* all offsets are in bytes */
/* all sizes are in bits */
constexpr size_t SIZE_OFFSET         = 0;
constexpr size_t SIZE_SIZE           = 8;

constexpr size_t CONTROL_OFFSET      = 1;
constexpr size_t CONTROL_SIZE        = 8;

constexpr size_t ADDRESS_SRC_OFFSET  = 2;
constexpr size_t ADDRESS_SRC_SIZE    = 8;

constexpr size_t ADDRESS_DEST_OFFSET = 3;
constexpr size_t ADDRESS_DEST_SIZE   = 8;

constexpr size_t FRAME_BODY_OFFSET   = 4;

typedef struct mac_packet_t
{
    uint8_t size;
    uint8_t control;
    uint8_t addr_src;
    uint8_t addr_dest;
    uint8_t frame[MAX_FRAME_SIZE];
    uint8_t frame_size;
    uint8_t checksum;
} mac_packet;

class media_access
{
    typedef uint8_t block;
    typedef fast_bitset<block> bitset;
    typedef std::function<void(mac_packet& packet)> packet_callback;

    size_t bits_per_symbol;
    boost::circular_buffer<int> last_symbols;

    fsk_modulator mod;
    fsk_demodulator demod;

    MODEM_STATE state = MODEM_STATE::READY;

    packet_callback p_cb = nullptr;

    void check_for_preamble();
    void preamble_found();
    void check_packet_complete();
    bool deserialize_packet(mac_packet& packet, uint8_t size, size_t min_symbols);
    bitset symbol_to_bits(int symbol);
    uint8_t get_current_packet_size();
    uint8_t calc_checksum(const std::vector<block> &data) const;

public:
    media_access(float _sample_rate, int samples_per_symbol, int _bits_per_symbol, float center_frequency);

    void process(const float* input, float* output, int count);
    void set_packet_callback(packet_callback callback);

    void dump_last_symbols() const;
};

#endif // MEDIA_ACCESS_H
