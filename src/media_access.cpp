#include <utility>

#include "media_access.h"

media_access::media_access(float _sample_rate, int samples_per_symbol, int _bits_per_symbol, float center_frequency)
    : bits_per_symbol(static_cast<size_t>(_bits_per_symbol)),
    last_symbols(2048),
    mod(_sample_rate, samples_per_symbol, static_cast<int>(bits_per_symbol), center_frequency),
    demod(_sample_rate, samples_per_symbol, static_cast<int>(bits_per_symbol), center_frequency)
{
//    for (auto v : PREAMBLE_ARRAY)
//    {
//        last_symbols.push_back(v);
//    }
}

void media_access::check_for_preamble()
{
    if (last_symbols.size() >= PREAMBLE_SIZE)
    {
        auto last_15 = (last_symbols.end() - PREAMBLE_SIZE);
        bool found_preamble = std::equal(std::begin(PREAMBLE_ARRAY), std::end(PREAMBLE_ARRAY), last_15, last_15 + PREAMBLE_SIZE);
        if (found_preamble)
        {
            // found preamble. clear buffer
#if SNET_DEBUG
            dump_last_symbols();
#endif
            last_symbols.clear();
            state = MODEM_STATE::RX;
        }
    }
}

void media_access::dump_last_symbols() const {
    printf("LAST_SYMBOLS DUMP: SIZE %ld\n", last_symbols.size());
    for (size_t i = 0; i < last_symbols.size(); i++)
            {
                printf("%d ", last_symbols[i]);
            }
    printf("\n");
}

void media_access::preamble_found()
{

}

media_access::bitset media_access::symbol_to_bits(int symbol)
{
    assert(symbol >= 0);
    bitset bits(bits_per_symbol);
    auto block = static_cast<std::uint8_t>(symbol);
    auto n_bits = static_cast<size_t>(bits_per_symbol);
    bits.append(block, n_bits);
    return bits;
}

uint8_t media_access::get_current_packet_size()
{
    bitset size_bits(SIZE_SIZE);
    auto min_symbols = static_cast<size_t>(std::ceil(static_cast<float>(SIZE_SIZE) / bits_per_symbol));
    auto buffer_size = last_symbols.size();
    if (buffer_size < min_symbols)
    {
        return 0;
    }
    for (size_t i = 0; i < min_symbols; i++)
    {
        int symbol = last_symbols[i];
        bitset symbol_bits = symbol_to_bits(symbol);
        symbol_bits.close();
        // this would break if bits per symbol > block_size
        size_bits.append(symbol_bits.blocks()[0], bits_per_symbol);
    }

    uint8_t size = size_bits.blocks().at(0);
    if (size < MIN_PACKET_SIZE)
    {
        return ERRORS::INVALID_SIZE;
    }
    return size;
}

bool media_access::deserialize_packet(mac_packet& packet, uint8_t size, size_t min_symbols)
{
    bitset bits(static_cast<size_t>(size) * 8);
    for (size_t i = 0; i < min_symbols; i++)
    {
        int s = last_symbols.front();
        auto symbol = static_cast<media_access::block>(s); // breaks if n_symbols > block_size
        bits.append(symbol, bits_per_symbol);
        last_symbols.pop_front();
    }
    auto data = bits.blocks();

    uint8_t checksum = calc_checksum(data);
    if (data[data.size() - 1] != checksum)
    {
#if SNET_DEBUG
        printf("checksum error");
#endif
//        return false;
    }

    // deserialize data
    uint8_t frame_size = size - static_cast<uint8_t >(5);
    if (frame_size > 0 && frame_size <= MAX_FRAME_SIZE)
    {
        std::copy(data.begin() + FRAME_BODY_OFFSET, data.begin() + FRAME_BODY_OFFSET + frame_size, std::begin(packet.frame));
    }
    else if (frame_size > MAX_FRAME_SIZE)
    {
        return false;
    }

    packet.size = data[SIZE_OFFSET];
    packet.control = data[CONTROL_OFFSET];
    packet.addr_src = data[ADDRESS_SRC_OFFSET];
    packet.addr_dest = data[ADDRESS_DEST_OFFSET];
    packet.frame_size = frame_size;
    packet.checksum = data[size - 1];
    return true;
}

uint8_t media_access::calc_checksum(const std::vector<block> &data) const {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < data.size() - 1; i++)
    {
        uint8_t byte = data[i];
        checksum += byte;
    }
    return checksum;
}

void media_access::check_packet_complete()
{
    uint8_t size = get_current_packet_size();
    auto min_symbols = static_cast<size_t>(std::ceil(size * 8.0f / bits_per_symbol));
    if (size > 0 && last_symbols.size() >= min_symbols)
    {
        mac_packet packet;
        if (deserialize_packet(packet, size, min_symbols))
        {
            state = MODEM_STATE::READY;
            if (p_cb != nullptr)
            {
                p_cb(packet);
            }
        }
    }
}

void media_access::process(const float* input, float* output, int count)
{
    int recv_symbol = demod.demodulate_symbol(input);
    last_symbols.push_back(recv_symbol);
    if (recv_symbol < 0 && state != MODEM_STATE::READY)
    {
        // we got noise, collision or invalid symbol -- clear buffer and reset modem
#if SNET_DEBUG
        dump_last_symbols();
#endif
        last_symbols.clear();
        state = MODEM_STATE::READY;
        return;
    }

    switch (state) {
    case MODEM_STATE::READY:
        // check for preamble
        check_for_preamble();
        break;

    case MODEM_STATE::RX:
        check_packet_complete();
        break;

    case MODEM_STATE::TX:
        break;

    case MODEM_STATE::BACKING_OFF:
        break;
    }
}

void media_access::set_packet_callback(packet_callback callback)
{
    p_cb = std::move(callback);
}
