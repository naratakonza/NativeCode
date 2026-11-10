#pragma once

#include <cstddef>
#include <cstdint>

constexpr unsigned char ObfuscateKeyByte(std::uint64_t seed, std::size_t index) noexcept
{
    const auto key = (seed ^ (index * 0x123456789ABCDEF1ULL)) + index * 16777619ULL;
    const auto byte = static_cast<unsigned char>(key >> ((index % 8) * 8));
    return byte == 0 ? 0x7F : byte;
}

template<std::size_t N>
class CObfuscatedText
{
public:
    CObfuscatedText(const volatile unsigned char* encoded, std::uint64_t seed) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            m_text[i] = encoded[i] ^ ObfuscateKeyByte(seed, i);
        }
    }

    CObfuscatedText(const CObfuscatedText&) = delete;
    CObfuscatedText& operator=(const CObfuscatedText&) = delete;

    ~CObfuscatedText()
    {
        volatile unsigned char* text = m_text;
        for (std::size_t i = 0; i < N; ++i) {
            text[i] = 0;
        }
    }

    operator const char*() const noexcept
    {
        return reinterpret_cast<const char*>(m_text);
    }

private:
    unsigned char m_text[N];
};

template<std::size_t N>
class CObfuscatedString
{
public:
    constexpr CObfuscatedString(const char (&text)[N], std::uint64_t seed) noexcept : m_seed(seed)
    {
        for (std::size_t i = 0; i < N; ++i) {
            m_encoded[i] = static_cast<unsigned char>(text[i]) ^ ObfuscateKeyByte(seed, i);
        }
    }

    CObfuscatedText<N> Decode() const noexcept
    {
        return CObfuscatedText<N>(m_encoded, m_seed);
    }

private:
    unsigned char m_encoded[N]{};
    std::uint64_t m_seed;
};

// Use the returned temporary in the same expression as the consuming call.
#define OBFUSCATE(text) ([] { \
    static_assert((text)[sizeof(text) - 1] == '\0', "Expected a null-terminated string literal"); \
    static constexpr CObfuscatedString<sizeof(text)> encoded(text, \
        0x9D6C8B6A8F4E3D2CULL ^ (static_cast<std::uint64_t>(__LINE__) * 16777619ULL)); \
    return encoded.Decode(); \
}())
