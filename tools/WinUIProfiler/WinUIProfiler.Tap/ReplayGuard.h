#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace WinUIProfiler::Tap
{
    // Never evict identities: replay detection has no false negatives. A fixed
    // 8 MiB Bloom filter avoids a connection request cap (and unbounded memory).
    // False positives reject conservatively; at 1M requests p is about 1e-7.
    class ReplayGuard final
    {
    public:
        ReplayGuard() : words(BitCount / 64) {}
        bool Insert(const GUID& request)
        {
            std::array<uint64_t, 2> identity{};
            std::memcpy(identity.data(), &request, sizeof(request));
            const auto first = Mix(identity[0] ^ Mix(identity[1]));
            const auto step = Mix(identity[1] ^ Mix(identity[0])) | 1;
            bool fresh = false;
            for (size_t i = 0; i < 7; ++i)
            {
                const auto bit = (first + i * step) & (BitCount - 1);
                auto& word = words[static_cast<size_t>(bit / 64)];
                const uint64_t mask = uint64_t{1} << (bit % 64);
                fresh |= (word & mask) == 0;
                word |= mask;
            }
            return fresh;
        }
    private:
        static constexpr uint64_t BitCount = 64 * 1024 * 1024;
        static uint64_t Mix(uint64_t value) noexcept
        {
            value ^= value >> 30;
            value *= 0xbf58476d1ce4e5b9;
            value ^= value >> 27;
            value *= 0x94d049bb133111eb;
            return value ^ (value >> 31);
        }
        std::vector<uint64_t> words;
    };
}
