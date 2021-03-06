#include "BitBoard.hpp"

/* assignation of static variables */
const std::array<int16_t, DIRS>  BitBoard::shifts = {{-19, -18, 1, 20, 19, 18, -1, -20}};
const BitBoard                BitBoard::full = (std::array<uint64_t, NICB>){{0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFF800000}};
const BitBoard                BitBoard::empty = (std::array<uint64_t, NICB>){{0, 0, 0, 0, 0, 0}};
const BitBoard                BitBoard::border_right = (std::array<uint64_t, NICB>){{0x200004000080, 0x10000200004000, 0x800010000200004, 0x800010000200, 0x40000800010000, 0x2000040000800000}};
const BitBoard                BitBoard::border_left = (std::array<uint64_t, NICB>){{0x8000100002000040, 0x8000100002000, 0x400008000100002, 0x400008000100, 0x20000400008000, 0x1000020000000000}};
const BitBoard                BitBoard::border_top = (std::array<uint64_t, NICB>){{0xFFFFE00000000000, 0, 0, 0, 0, 0}};
const BitBoard                BitBoard::border_bottom = (std::array<uint64_t, NICB>){{0, 0, 0, 0, 0, 0x3FFFF800000}};
const std::array<t_pattern,8> BitBoard::patterns = {{
    (t_pattern){0xF8, 5, 4, 500, 5000},  //   OOOOO  :  five
    (t_pattern){0x78, 6, 4, 500, 1100},  //  -OOOO-  :  open four
    (t_pattern){0x70, 5, 4,  50,  500},  //   -OOO-  :  open three
    (t_pattern){0x68, 6, 8,  49,  495},  //  -OO-O-  :  open split three
    (t_pattern){0xF0, 5, 8,  50, 1000},  //  |OOOO-  :  close four
    (t_pattern){0xB8, 5, 8,  50, 1000},  //   O-OOO  :  split four #1
    (t_pattern){0xD8, 5, 8,  50, 1000},  //   OO-OO  :  split four #2
    (t_pattern){0xE8, 5, 8,  50, 1000}   //   OOO-O  :  split four #3
}};
std::array<int, 8>  BitBoard::p1_pattern_weights = {{0, 0, 0, 0, 0, 0, 0, 0}};
std::array<int, 8>  BitBoard::p2_pattern_weights = {{0, 0, 0, 0, 0, 0, 0, 0}};


/* population count of a 64-bit unsigned integer (count the number of set bits) */
static int popcount64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555);
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
}

BitBoard::BitBoard(void) {
    this->zeros();
}

BitBoard::BitBoard(std::array<uint64_t, NICB> values) {
    this->values = values;
}

BitBoard::BitBoard(BitBoard const &src) {
    *this = src;
}

BitBoard::~BitBoard(void) {
}

BitBoard	&BitBoard::operator=(BitBoard const &rhs) {
    this->values = rhs.values;
    return (*this);
}

BitBoard	&BitBoard::operator=(uint64_t const &val) {
    this->values[4] = (val >> 41);
    this->values[5] = (val << 23);
    return (*this);
}

/*
** Helper functions
*/
/*  return a given row of the bitboard, the lookup table are the binary masks for fast
    access to the values. there are 24 of them because 5 rows out of the 19 are splitted
    in two uint64_t (see BitBoard.hpp to see where the splits are).
*/
uint64_t    BitBoard::row(uint8_t i) const {
    static const uint64_t masks[24] = {
        0xFFFFE00000000000,     0x1FFFFC000000,         0x3FFFF80,              0x7F,
        0xFFFFE00000000,           0x1FFFFC000,            0x3FFF, 0x7FFFF0000000000,
        0xFFFFE00000,                 0x1FFFFC,               0x3,    0x7FFFF0000000,
        0xFFFFE00,                       0x1FF,  0x3FFFF800000000,       0x7FFFF0000,
        0xFFFF,             0x1FFFFC0000000000,     0x3FFFF800000,0xFFF0000000000000,
        0xF800000000000000, 0xFFFF800000000000,0xFFC0000000000000, 0xE000000000000000 };
    uint64_t    n = (i * 19) / BITS;
    uint64_t    s = (i * 19) % BITS;
    if (i==3||i==6||i==10||i==13||i==16)
        return (((this->values[n] & masks[i]) << s) | ((this->values[n+1] & masks[19+n]) >> (BITS-s)) );
    return ((this->values[n] & masks[i]) << s);
}

void    BitBoard::zeros(void) {
    for (int i = 0; i < NICB; ++i)
        this->values[i] = 0;
}

/*  will broadcast the given row (only the first 19 bits) to all rows,
    the pattern must be encoded in the first 19 bits
*/
void    BitBoard::broadcast_row(uint64_t row) {
    uint16_t    offset = 19;
    int32_t     shift = 0;

    row >>= BITS-19;
    this->zeros();
    for (int i = 0; i < NICB; ++i) {
        shift = 1;
        for (int j = 0; shift > 0; ++j) {
            shift = BITS - offset - (19 * j);
            this->values[i] |= (shift > 0 ? row << shift : row >> -shift);
        }
        offset = -shift;
    }
}

void    BitBoard::write(const uint64_t x, const uint64_t y) {
    const uint64_t    n = (19 * y + x);
    this->values[n >> 6] |= (0x8000000000000000 >> (n & 0x3F));
}

void    BitBoard::remove(const uint64_t x, const uint64_t y) {
    const uint64_t    n = (19 * y + x);
    this->values[n >> 6] &= ~(0x8000000000000000 >> (n & 0x3F));
}

void    BitBoard::write(const uint64_t i) {
    this->values[i >> 6] |= (0x8000000000000000 >> (i & 0x3F));
}

void    BitBoard::remove(const uint64_t i) {
    this->values[i >> 6] &= ~(0x8000000000000000 >> (i & 0x3F));
}

int    BitBoard::set_count(void) const {
    int         res = 0;
    for (int i = NICB; i--; )
        res += popcount64(this->values[i]);
    return (res);
}

bool    BitBoard::check_bit(const uint64_t i) const {
    return ((this->values[i >> 6] & (0x8000000000000000 >> (i & 0x3F))) == 0 ? false : true);
}

bool    BitBoard::check_bit(const uint64_t x, const uint64_t y) const {
    const uint16_t    n = (19 * y + x);
    return ((this->values[n >> 6] & (0x8000000000000000 >> (n & 0x3F))) == 0 ? false : true);
}

bool    BitBoard::is_empty(void) const {
    for (int i = 0; i < NICB; ++i)
        if (this->values[i])
            return (false);
    return (true);
}

int     BitBoard::leftmost_bit(void) const {
    for (int i = 0; i < NICB; ++i)
        if (this->values[i]) {
            uint64_t x = this->values[i];
            x |= (x >> 1);
            x |= (x >> 2);
            x |= (x >> 4);
            x |= (x >> 8);
            x |= (x >> 16);
            x |= (x >> 32);
            return (BITS-popcount64(x)+(i<<6));
        }
    return (-1);
}

int     BitBoard::rightmost_bit(void) const {
    for (int i = NICB; i--;)
        if (this->values[i])
            return (((i+1) << 6) - popcount64((this->values[i] & -this->values[i]) - 1)-1);
    return (-1);
}

BitBoard    BitBoard::neighbours(void) const {
    return (this->dilated() ^ *this);
}

/* shift either right or left depending on the sign of the shift, if we shift right we handle the overflow to the extra bits */
BitBoard    BitBoard::shifted(const uint8_t dir, const uint8_t n) const {
    return (BitBoard::shifts[dir] > 0 ? (*this >> BitBoard::shifts[dir]*n) & BitBoard::full : (*this << -BitBoard::shifts[dir]*n));
}

/* shift in the inverse direction from BitBoard::shifted (useful for pattern detection) */
BitBoard    BitBoard::shifted_inv(const uint8_t dir, const uint8_t n) const {
    return (BitBoard::shifted(dir < 4 ? dir + 4 : dir - 4, n));
}

/* return the dilated board taking into account the board boundaries */
BitBoard    BitBoard::dilated(void) const {
	BitBoard	res = *this;
    for (int i = direction::north_east; i < direction::south; ++i)
        res |= this->shifted(i) & ~BitBoard::border_left;
    for (int i = direction::south_west; i < 8; ++i)
        res |= this->shifted(i) & ~BitBoard::border_right;
    res |= this->shifted(direction::north);
    res |= this->shifted(direction::south);
    return (res);
}

/* return the eroded board taking into account the board boundaries */
BitBoard    BitBoard::eroded(void) const {
	BitBoard	res = *this;
    for (int i = direction::north_east; i < direction::south; ++i)
        res &= this->shifted(i) & ~BitBoard::border_left;
    for (int i = direction::south_west; i < 8; ++i)
        res &= this->shifted(i) & ~BitBoard::border_right;
    res &= this->shifted(direction::north);
    res &= this->shifted(direction::south);
    return (res);
}

BitBoard    BitBoard::rotated_45(void) {
    /* non optimized, but working */
    BitBoard    res;
    BitBoard    mask;
    uint64_t    v = 1;

    v <<= BITS - 1;
    for (int j = 0; j < 19; j++) {
        mask.broadcast_row(v);
        res |= ((*this >> (19 * j) | *this << (19 * (19 - j)))) & mask;
        v >>= 1;
    }
    return (res);
}

/*
** Arithmetic operator overload
*/
BitBoard	BitBoard::operator|(const BitBoard &rhs) const {
	BitBoard	res;
    for (int i = 0; i < NICB; ++i)
        res.values[i] = this->values[i] | rhs.values[i];
	return (res);
}

BitBoard	BitBoard::operator&(const BitBoard &rhs) const {
	BitBoard	res;
    for (int i = 0; i < NICB; ++i)
        res.values[i] = this->values[i] & rhs.values[i];
	return (res);
}

BitBoard    BitBoard::operator^(BitBoard const &rhs) const {
    BitBoard	res;
    for (int i = 0; i < NICB; ++i)
        res.values[i] = this->values[i] ^ rhs.values[i];
    return (res);
}

BitBoard	BitBoard::operator~(void) const {
	BitBoard	res;
    for (int i = 0; i < NICB; ++i)
        res.values[i] = ~this->values[i];
	return (res);
}

BitBoard    BitBoard::operator>>(const int32_t shift) const {
    BitBoard	res;
    if (shift <= 0)
        return (*this);
    else if (shift < BITS) {
        for (int i = NICB-1; i > 0; i--)
            res.values[i] = (this->values[i] >> shift) | (this->values[i-1] << (BITS - shift));
        res.values[0] = (this->values[0] >> shift);
    } else {
        const uint16_t    n = shift >> 6;
        const uint16_t    a = shift & 0x3F;
        for (int i = NICB-1; i > n; i--) {
            if (a == 0)
                res.values[i] = (this->values[i-n] << (BITS - a));
            else
                res.values[i] = (this->values[i-n] >> a) | (this->values[i-(n+1)] << (BITS - a));
        }
        res.values[n] = (this->values[0] >> a);
    }
	return (res);
}

BitBoard    BitBoard::operator<<(const int32_t shift) const {
    BitBoard	res;
    if (shift <= 0)
        return (*this);
    else if (shift < BITS) {
        for (int i = 0; i < NICB; ++i)
            res.values[i] = (this->values[i] << shift) | (this->values[i+1] >> (BITS - shift));
        res.values[NICB-1] = (this->values[NICB-1] << shift);
    } else {
        const uint16_t    n = shift >> 6;
        const uint16_t    a = shift & 0x3F;
        const uint16_t    p = NICB - (n + 1);
        for (int i = 0; i < p; ++i) {
            if (a == 0)
                res.values[i] = (this->values[i+n] >> (BITS - a));
            else
                res.values[i] = (this->values[i+n] << a) | (this->values[i+n+1] >> (BITS - a));
        }
        res.values[p] = (this->values[p+n] << a);
    }
	return (res);
}

/*
** Assignation operator overload
*/
BitBoard    &BitBoard::operator|=(BitBoard const &rhs) {
    for (int i = 0; i < NICB; ++i)
        this->values[i] = (this->values[i] | rhs.values[i]);
    return (*this);
}

BitBoard    &BitBoard::operator&=(BitBoard const &rhs) {
    for (int i = 0; i < NICB; ++i)
        this->values[i] = (this->values[i] & rhs.values[i]);
    return (*this);
}

BitBoard    &BitBoard::operator^=(BitBoard const &rhs) {
    for (int i = 0; i < NICB; ++i)
        this->values[i] = (this->values[i] ^ rhs.values[i]);
    return (*this);
}

BitBoard    &BitBoard::operator<<=(const int32_t shift) {
    *this = (*this << shift);
    return (*this);
}

BitBoard    &BitBoard::operator>>=(const int32_t shift) {
    *this = (*this >> shift);
    return (*this);
}

/*
** Member access operator overload
*/
uint64_t    BitBoard::operator[](const int i) const {
    return (this->values[i >> 6] & (0x8000000000000000 >> (i & 0x3F)));
}

/*
** Comparison operator overload
*/
bool        BitBoard::operator==(BitBoard const &rhs) const {
    for (int i = 0; i < NICB; ++i)
        if (this->values[i] != rhs.values[i])
            return (false);
    return (true);
}

bool        BitBoard::operator!=(BitBoard const &rhs) const {
    for (int i = 0; i < NICB; ++i)
        if (this->values[i] != rhs.values[i])
            return (true);
    return (false);
}

/*
** Non-member functions
*/

BitBoard    get_threat_moves(BitBoard const &p1, BitBoard const &p2, int p2_pairs_captured) {
    BitBoard    res;
    res |= win_by_capture_detector(p2, p1, p2_pairs_captured);
    res |= pattern_detector_highlight_open(p2, p1, { 0x70, 5, 4, 0, 0 }); // -OOO-
    res |= pattern_detector_highlight_open(p2, p1, { 0x68, 6, 8, 0, 0 }); // -OO-O-
    res |= future_pattern_detector(p2, p1, { 0xF8, 5, 8, 0, 0 }); // future pattern detection on OOOOO
    return (res & ~p1 & ~p2);
}

/* return the moves that instant win (no possible counter by opponent) */
BitBoard    get_winning_moves(BitBoard const &p1, BitBoard const &p2, int p1_pairs_captured, int p2_pairs_captured) {
    BitBoard    res = future_pattern_detector(p1, p2, { 0xF8, 5, 8, 0, 0 }); // OOOOO
    /* if there is no possibility of breaking the alignment and no winning pair capture either */
    res = highlight_five_aligned((p1 | res) ^ pair_capture_detector_highlight(p2, (p1 | res)));
    if (!res.is_empty() && win_by_capture_detector(p2, p1, p2_pairs_captured).is_empty())
        return (res & ~p1 & ~p2);
    res = win_by_capture_detector(p1, p2, p1_pairs_captured);
    return (res & ~p1 & ~p2);
}

/* detect a sub-pattern in a single direction */
static BitBoard single_direction_pattern_detector(BitBoard const &p1, BitBoard const &p2, uint8_t const &pattern, uint8_t const &length, uint8_t const &s, uint8_t const &type, uint8_t const &dir) {
    const BitBoard  open_cells = (~p1 & ~p2);
    BitBoard        res;

    res = (type == 0x80 ? p2 : BitBoard::full);
    for (int n = 0; n < length && !res.is_empty(); ++n) {
        res = (dir > 0 && dir < 4 ? res & ~BitBoard::border_right : (dir > 4 && dir < 8 ? res & ~BitBoard::border_left : res));
        res = res.shifted(dir) & ((pattern << n & 0x80) == 0x80 ? p1 : open_cells);
    }
    return (res.shifted_inv(dir, s) & open_cells);
}

/*  will return a bitboard showing the forbidden cells for p1, it achieves that
    by searching for open threes patterns and checking if we have overlapping open
    moves leading to those. It's not the most efficient algorithm, we must improve
    it in the future, but it is robust.
*/
BitBoard        forbidden_detector(BitBoard const &p1, BitBoard const &p2) {
    const uint8_t   patterns[3] = { 0x58, 0x68, 0x70 }; // -O-OO- , -OO-O- , -OOO-
    const uint8_t    lengths[3] = {    6,    6,    5 };
    BitBoard        res;
    BitBoard        tmp[4]; // 4 is number of directions, use 2 instead of 4 ? for the num of mirror
    uint8_t         sub;

    for (int p = 0; p < 3; ++p) { // iterate through patterns
        for (int s = 0; s < lengths[p]; ++s) { // iterate through sub patterns
            sub = patterns[p] & ~(0x80 >> s);
            if (sub != patterns[p]) {
                for (int d = direction::north; d < 4; ++d) { // iterate through directions
                    tmp[d] |= single_direction_pattern_detector(p1, p2, sub, lengths[p], lengths[p]-s-1, 0, d);
                    if (!tmp[d].is_empty()) {
                        for (int n = d-1; n >= 0; --n)
                            res |= (tmp[d] & tmp[n]);
                    }
                }
            }
        }
    }
    return (res & ~p1 & ~p2);
}

BitBoard    four_four_detector(BitBoard const &p1, BitBoard const &p2) {
    const uint8_t   patterns[4] = { 0x78, 0xF0, 0xE8, 0xB8 }; // -OOOO~, ~OOOO-, ~OOO-O~, ~O-OOO~
    const uint8_t    lengths[4] = {    5,    5,    5,    5 };
    BitBoard        res;
    BitBoard        tmp[4];
    uint8_t         sub;

    for (int p = 0; p < 4; ++p) { // iterate through patterns
        for (int s = 0; s < lengths[p]; ++s) { // iterate through sub patterns
            sub = patterns[p] & ~(0x80 >> s);
            if (sub != patterns[p])
                for (int d = direction::north; d < 4; ++d) { // iterate through directions
                    tmp[d] |= single_direction_pattern_detector(p1, p2, sub, lengths[p], lengths[p]-s-1, 0, d);
                    if (!tmp[d].is_empty())
                        for (int n = d-1; n >= 0; --n)
                            res |= (tmp[d] & tmp[n]);
                }
        }
    }
    return (res & ~p1 & ~p2);
}

BitBoard    three_four_detector(BitBoard const &p1, BitBoard const &p2) {
    const uint8_t   patterns[7] = { 0x58, 0x68, 0x70, 0x78, 0xF0, 0xE8, 0xB8 }; // -O-OO-, -OO-O-, -OOO-, -OOOO~, ~OOOO-, ~OOO-O~, ~O-OOO~
    const uint8_t    lengths[7] = {    6,    6,    5,    5,    5,    5,    5 };
    BitBoard        res;
    BitBoard        tmp[8];
    uint8_t         sub;

    for (int p = 0; p < 7; ++p) { // iterate through patterns
        for (int s = 0; s < lengths[p]; ++s) { // iterate through sub patterns
            sub = patterns[p] & ~(0x80 >> s);
            if (sub != patterns[p])
                for (int d = direction::north; d < 4; ++d) { // iterate through directions
                    tmp[d+(p>=3?4:0)] |= single_direction_pattern_detector(p1, p2, sub, lengths[p], lengths[p]-s-1, 0, d);
                    if (!tmp[d].is_empty())
                        for (int n = d-1; n >= 0; --n)
                            res |= (tmp[d] & tmp[n+4]);
                }
        }
    }
    return (res & ~p1 & ~p2);
}

static BitBoard sub_pattern_detector(BitBoard const &p1, BitBoard const &p2, t_pattern const &pattern, uint8_t const &s, uint8_t const &type) {
    const BitBoard  open_cells = (~p1 & ~p2);
    BitBoard        res;
    BitBoard        tmp;

    for (int d = direction::north; d < pattern.dirs; ++d) {
        tmp = (type == 0x80 ? p2 : BitBoard::full);
        for (int n = 0; n < pattern.size && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & ((pattern.repr << n & 0x80) == 0x80 ? p1 : open_cells);
        }
        res |= tmp.shifted_inv(d, s);
    }
    return (res);
}

/*  will return the bitboard showing the open moves for p1 leading to the specified pattern.
    the patterns must be encoded in big-endian and have a length defined (-OO-O- is 01101000
    so 0x68 with length 6) see the Patterns table in BitBoard.hpp for all the patterns.
*/
BitBoard        future_pattern_detector(BitBoard const &p1, BitBoard const &p2, t_pattern const &pattern) { // TODO : patterns are detected also on top/bottom borders ????
    const uint8_t   type = (pattern.repr & 0x80) | (0x1 << (8-pattern.size) & pattern.repr);
    BitBoard        res;
    t_pattern       sub = pattern;

    for (int s = 0; s < pattern.size; ++s) {
        sub.repr = pattern.repr & ~(0x80 >> s);
        if (sub.repr != pattern.repr)
            res |= sub_pattern_detector(p1, p2, sub, pattern.size-s-1, type);
    }
    return (res & ~p1 & ~p2);
}

BitBoard        pattern_detector(BitBoard const &p1, BitBoard const &p2, t_pattern const &pattern) {
    return (sub_pattern_detector(p1, p2, pattern, 0, (pattern.repr & 0x80) | (0x1 << (8-pattern.size) & pattern.repr)));
}

BitBoard    pattern_detector_highlight_open(BitBoard const &p1, BitBoard const &p2, t_pattern const &pattern) {
    const uint8_t   type = (pattern.repr & 0x80) | (0x1 << (8-pattern.size) & pattern.repr);
    const BitBoard  open_cells = (~p1 & ~p2);
    BitBoard        res;
    BitBoard        tmp;

    for (int d = direction::north; d < pattern.dirs; ++d) {
        tmp = (type == 0x80 ? p2 : BitBoard::full);
        for (int n = 0; n < pattern.size && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & ((pattern.repr << n & 0x80) == 0x80 ? p1 : open_cells);
        }
        if (!tmp.is_empty()) {
            for (int n = 0; n < pattern.size-1; ++n)
                tmp |= tmp.shifted_inv(d);
            res |= tmp & open_cells;
        }
    }
    return (res);
}

bool        detect_five_aligned(BitBoard const &bitboard) {
    BitBoard    tmp;

    for (int d = direction::north; d < DIRS; ++d) {
        tmp = bitboard;
        for (int n = 1; !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp &= tmp.shifted(d);
            if (n >= 5)
                return (true);
        }
    }
    return (false);
}

BitBoard    highlight_five_aligned(BitBoard const &bitboard) {
    BitBoard    res;
    BitBoard    tmp;

    for (int d = direction::north; d < 4; ++d) {
        tmp = bitboard;
        for (int n = 0; n < 4 && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & bitboard;
        }
        for (int i = 0; i < 5 && !tmp.is_empty(); ++i)
            res |= tmp.shifted_inv(d, i);
    }
    return (res);
}

/*  detect a pair and return the positions on the bitboard where it leads to capture
*/
BitBoard    pair_capture_detector(BitBoard const &p1, BitBoard const &p2) {
    const BitBoard  open_cells = (~p1 & ~p2);
    BitBoard        res;
    BitBoard        tmp;

    for (int d = direction::north; d < 8; ++d) {
        tmp = p1;
        for (int n = 0; n < 3 && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & ((0xC0 << n & 0x80) == 0x80 ? p2 : open_cells);
        }
        res |= tmp;
    }
    return (res);
}

BitBoard    pair_capture_detector_highlight(BitBoard const &p1, BitBoard const &p2) {
    const BitBoard  open_cells = (~p1 & ~p2);
    BitBoard        res;
    BitBoard        tmp;

    for (int d = direction::north; d < 8; ++d) {
        tmp = p1;
        for (int n = 0; n < 3 && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & ((0xC0 << n & 0x80) == 0x80 ? p2 : open_cells);
        }
        if (!tmp.is_empty())
            res |= (tmp.shifted_inv(d, 2) | tmp.shifted_inv(d, 1));
    }
    return (res);
}

BitBoard    pair_capture_breaking_five_detector(BitBoard const &p1, BitBoard const &p2) {
    BitBoard        res;
    BitBoard        tmp;
    BitBoard        alignment = highlight_five_aligned(p2);

    for (int d = direction::north; d < 8; ++d) {
        tmp = single_direction_pattern_detector(p2, p1, 0xC0, 3, 0, 0x80, d); // |OO- check if you can capture
        if (((tmp.shifted_inv(d) | tmp.shifted_inv(d,2)) & alignment).is_empty() == false)
            res |= tmp;
    }
    return (res);
}

BitBoard    win_by_capture_detector(BitBoard const &p1, BitBoard const &p2, int p1_pairs_captured) { // TODO : optimization
    BitBoard    captures = pair_capture_detector(p1, p2);
    BitBoard    res;
    if (!captures.is_empty()) {
        for (int i = 0; i < 361; ++i) {
            if (captures.check_bit(i)) {
                BitBoard tmp = p1;
                tmp.write(i);
                tmp = highlight_captured_stones(tmp, p2, i);
                if (p1_pairs_captured + (tmp.set_count() / 2) >= 5) {
                    tmp.zeros();
                    tmp.write(i);
                    res |= tmp;
                }
            }
        }
    }
    return (res);
}

/* return the positions of moves leading to unbreakable five */
BitBoard    win_by_alignment_detector(BitBoard const &p1, BitBoard const &p2, BitBoard const &p1_forbidden, int p2_pairs_captured) {
    BitBoard    res = (p1 | future_pattern_detector(p1, p2, { 0xF8, 5, 8, 0, 0 })) & ~p1_forbidden;
    res = highlight_five_aligned(res ^ pair_capture_detector_highlight(p2, res)) & ~p1 & ~p2;
    if (!res.is_empty() && win_by_capture_detector(p2, p1, p2_pairs_captured).is_empty())
        return (res);
    return (BitBoard::empty);
}

/*  returns the bitboard with the positions of the stones captured
*/
BitBoard    highlight_captured_stones(BitBoard const &p1, BitBoard const &p2, int move) {
    BitBoard        res;
    BitBoard        tmp;

    for (int d = direction::north; d < 8; ++d) {
        tmp.zeros();
        tmp.write(move);
        tmp &= p1;
        for (int n = 0; n < 3 && !tmp.is_empty(); ++n) {
            tmp = (d > 0 && d < 4 ? tmp & ~BitBoard::border_right : (d > 4 && d < 8 ? tmp & ~BitBoard::border_left : tmp));
            tmp = tmp.shifted(d) & ((0xC0 << n & 0x80) == 0x80 ? p2 : p1);
        }
        if (!tmp.is_empty())
            res |= (tmp.shifted_inv(d, 2) | tmp.shifted_inv(d, 1));
    }
    return (res);
}

std::ostream	&operator<<(std::ostream &os, BitBoard const &bitboard) {
    std::string         tmp = "";
    std::string         sub = "";
    std::stringstream   ss;

    for (int i = 0; i < NICB; ++i)
        tmp += std::bitset<64>(bitboard.values[i]).to_string();

    for (int i = 0; i < 19; ++i) {
        sub = tmp.substr(i*19, 19);
        for (int j = 0; j < 19; ++j)
            ss << (((i*19)+j)%BITS!=0?" ":"/") << (sub[j]=='0'?"◦":"◉");
        ss << std::endl;
    }
    /* show the extra bits */
    // sub = tmp.substr(19*19, 23);
    // for (uint32_t j = 0; j < 21; j++)
    //     ss << (((19*19)+j)%BITS!=0?" ":"/") << (sub[j]=='0'?"◦":"◉");
    // os << ss.str()  << std::endl;
    os << ss.str();
	return (os);
}
