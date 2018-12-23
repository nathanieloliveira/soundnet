#ifndef FAST_BITSET_HPP
#define FAST_BITSET_HPP

#include <cmath>
#include <vector>

template <typename block_type>
class fast_bitset
{
public:
    typedef block_type block;
    static const size_t bits_per_block = sizeof(block) * 8;

    fast_bitset()
        : is_open_(true)
        , blocks_n(1)
        , blocks_(1)
        , space_(blocks_.size()*bits_per_block)
    {
        blocks_.reserve(1);
    }

    fast_bitset(size_t bits)
        : is_open_(true),
          blocks_n(std::ceil(bits / static_cast<float>(bits_per_block))),
          blocks_(1),
          space_(blocks_.size()*bits_per_block)
    {
        blocks_.reserve(1);
    }

    void append(const fast_bitset& other)
    {
        assert(!other.is_open_);

        for(size_t n = 0; n < other.blocks_.size()-1; ++n)
            append(other.blocks_[n], bits_per_block);
        append(other.blocks_.back() >> other.space_, bits_per_block - other.space_);
    }

    void append(block value, size_t n_bits)
    {
        assert(is_open_);
        assert(n_bits <= bits_per_block);

        if(space_ < n_bits)
        {
            blocks_.push_back(0);
            space_ = bits_per_block;
        }

        blocks_.back() = blocks_.back() << n_bits;
        blocks_.back() = blocks_.back() | value;
        space_ -= n_bits;
    }

    void push_back(bool bit)
    {
        append(bit, 1);
    }

    bool operator[](size_t index) const
    {
        assert(!is_open_);

        static const size_t high_bit = 1 << (bits_per_block-1);
        const size_t block_index = index / bits_per_block;
        const size_t bit_index = index % bits_per_block;
        const size_t bit_mask = high_bit >> bit_index;
        return blocks_[block_index] & bit_mask;
    }

    void close()
    {
//        blocks_.back() = blocks_.back() << space_;
        is_open_ = false;
    }

    size_t size() const
    {
        return blocks_.size()*bits_per_block-space_;
    }

    const std::vector<block>& blocks() const {return blocks_;}

    class reader
    {
    public:
        reader(const fast_bitset& bitset)
            : bitset_(bitset)
            , index_(0)
            , size_(bitset.size()){}
        bool next_bit(){return bitset_[index_++];}
        bool eof() const{return index_ >= size_;}
    private:
        const fast_bitset& bitset_;
        size_t index_;
        size_t size_;
    };

private:
    bool is_open_;
    size_t blocks_n;
    std::vector<block> blocks_;
    size_t space_;
};

#endif // FAST_BITSET_HPP
