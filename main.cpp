#include <cstdint>
#include <array>
#include <iostream>
#include <immintrin.h>

constexpr int roundUp(int x, int by) {
    return x + (by - (x % by));
}

template<int... Pattern>
constexpr int numMasksForPattern() {
    return roundUp(sizeof...(Pattern), sizeof(__m256i)) / sizeof(__m256i);
}

template<int... Pattern>
constexpr auto splitPattern(int padNum) {
    std::array flat{Pattern...};
    constexpr auto chunks = numMasksForPattern<Pattern...>();
    std::array<std::array<int, sizeof(__m256i)>, chunks> out{};

    int flatIndex = 0;
    for (int i = 0; i < chunks; i++) {
        for (int p = 0; p < sizeof(__m256i); p++) {
            out[i][p] = flatIndex < flat.size() ? flat[flatIndex] : padNum;
            flatIndex++;
        }
    }
    return out;
}


/*constexpr uint8_t bitAsMaskByte(bool bit) {
    return bit ? 0xFF : 0;
}

constexpr __m256i toAvxMask(uint32_t mask) {
    std::array<int64_t, 4> vectorInts{};

    for (int i = 0; i < 4; i++) {
        const uint8_t byte = (mask >> (i * 8)) & 0xFF;
        uint64_t word{};
        for (int bit = 0; bit < 8; bit++) {
            const bool b = (byte >> bit) & 1;
            word |= (static_cast<int64_t>(bitAsMaskByte(b)) << (bit * 8));
        }
        vectorInts[i] = word;
    }

    return __m256i{vectorInts[0], vectorInts[1], vectorInts[2], vectorInts[3]};
}

template<size_t N>
constexpr auto toAvxMasks(const std::array<uint32_t, N>& masks) -> std::array<__m256i, N> {
    std::array<__m256i, N> out{};
    for (int i = 0; i < N; i++) {
        out[i] = toAvxMask(masks[i]);
    }
    return out;
}*/

constexpr uint32_t maskForChunk(const std::array<int, sizeof(__m256i)>& array) {
    uint8_t out{};
    for (int i = 0; i < array.size(); i++) {
        const uint8_t bit = array[i] < 0 ? 0 : 1;
        out |= (bit << i);
    }
    return out;
}

template<int... Pattern>
constexpr auto makePatternMasks() {
    constexpr std::array split = splitPattern<Pattern...>(-1);
    std::array<uint32_t, split.size()> out{};

    for (int i = 0; i < split.size(); i++) {
        out[i] = maskForChunk(split[i]);
    }
    return out;
}

constexpr __m256i chunkToRegister(const std::array<int, sizeof(__m256i)>& chunk) {
    std::array<int64_t, 4> vectorInts{};

    for (int i = 0; i < 4; i++) {
        uint64_t word{};
        for (int bit = 0; bit < 8; bit++) {
            const int patternByte = chunk[i * 8 + bit];
            const uint8_t byte = patternByte;
            word |= (static_cast<int64_t>(byte) << (bit * 8));
        }
        vectorInts[i] = word;
    }

    return __m256i{vectorInts[0], vectorInts[1], vectorInts[2], vectorInts[3]};
}

template<int... Pattern>
constexpr auto makePatternData() {
    constexpr std::array split = splitPattern<Pattern...>(69);
    std::array<__m256i, split.size()> out{};

    for (int i = 0; i < split.size(); i++) {
        out[i] = chunkToRegister(split[i]);
    }
    return out;
}

/*template<int... Pattern>
constexpr auto makePatternMasks() {
   return toAvxMasks(makePatternIntMasks<Pattern...>());
}*/

template<int... Pattern>
uint8_t* FindPattern(uint8_t* const data, const size_t size) {
    constexpr std::array masks = makePatternMasks<Pattern...>();
    constexpr std::array patterns = makePatternData<Pattern...>();
    static_assert(masks.size() == patterns.size());

    const auto test = [&](const uint8_t* section) -> bool {
        for (int j = 0; j < patterns.size(); j++) {
            const __m256i block = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(section + (j * sizeof(__m256i))));
            const __m256i patternRegister = patterns[j];
            const __m256i cmp = _mm256_cmpeq_epi8(block, patternRegister);
            const int cmpMask = _mm256_movemask_epi8(cmp);
            const auto patternMask = masks[j];

            if ((cmpMask & patternMask) != patternMask) return false;
        }
        return true;
    };

    for (size_t i = 0; i < size; i++) {
        if (test(data + i)) return data + i;
    }
    return nullptr;
}

uint8_t* test(uint8_t* const data, const size_t size) {
    return FindPattern<2, -1, 6, 8, 10>(data, size);
}

int main() {
    uint8_t uwu[666]{2, 55, 6, 8, 10};
    auto* result = test(uwu, sizeof(uwu));

    std::cout << (result != nullptr) << '\n';
}
