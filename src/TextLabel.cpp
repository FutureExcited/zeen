#include "TextLabel.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeTexture.h"

namespace {
    // 5x7 bitmap font. Each glyph is 7 rows, each row a 5-bit mask (bit 4 = leftmost).
    using Glyph = std::array<uint8_t, 7>;

    auto Font() -> const std::unordered_map<char, Glyph>& {
        static const std::unordered_map<char, Glyph> font = {
            {' ', {0,0,0,0,0,0,0}},
            {'-', {0b00000,0b00000,0b00000,0b11111,0b00000,0b00000,0b00000}},
            {'.', {0,0,0,0,0,0,0b00100}},
            {':', {0,0b00100,0,0,0,0b00100,0}},
            {'!', {0b00100,0b00100,0b00100,0b00100,0b00100,0,0b00100}},
            {'/', {0b00001,0b00010,0b00010,0b00100,0b01000,0b01000,0b10000}},
            {'A', {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
            {'B', {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110}},
            {'C', {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110}},
            {'D', {0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110}},
            {'E', {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111}},
            {'F', {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000}},
            {'G', {0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01111}},
            {'H', {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
            {'I', {0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110}},
            {'J', {0b00111,0b00010,0b00010,0b00010,0b00010,0b10010,0b01100}},
            {'K', {0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001}},
            {'L', {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111}},
            {'M', {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001}},
            {'N', {0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001}},
            {'O', {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}},
            {'P', {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000}},
            {'Q', {0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101}},
            {'R', {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001}},
            {'S', {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110}},
            {'T', {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100}},
            {'U', {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}},
            {'V', {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100}},
            {'W', {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001}},
            {'X', {0b10001,0b10001,0b01010,0b00100,0b01010,0b10001,0b10001}},
            {'Y', {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100}},
            {'Z', {0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111}},
            {'0', {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}},
            {'1', {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}},
            {'2', {0b01110,0b10001,0b00001,0b00110,0b01000,0b10000,0b11111}},
            {'3', {0b11111,0b00010,0b00100,0b00110,0b00001,0b10001,0b01110}},
            {'4', {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}},
            {'5', {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}},
            {'6', {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110}},
            {'7', {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}},
            {'8', {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}},
            {'9', {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100}},
        };
        return font;
    }

    constexpr int kGlyphW = 5;
    constexpr int kSpacing = 1;   // pixels between glyphs

    auto GlyphFor(char c) -> const Glyph& {
        static const Glyph blank{0,0,0,0,0,0,0};
        const auto& f = Font();
        const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        auto it = f.find(up);
        return it != f.end() ? it->second : blank;
    }
}

namespace TextLabel {

auto MeasureWidth(const std::string& text) -> int {
    if (text.empty()) return 0;
    return static_cast<int>(text.size()) * (kGlyphW + kSpacing) - kSpacing;
}

auto MakeTexture(const std::string& text, const std::string& registryName,
                 int pixelScale, uint32_t& outWidth, uint32_t& outHeight)
    -> std::shared_ptr<BeTexture> {
    const int pad = 2;   // 2-px transparent margin around the text
    const int textW = MeasureWidth(text);
    const int pxW = (textW + pad * 2) * pixelScale;
    const int pxH = (GlyphHeight + pad * 2) * pixelScale;
    outWidth = static_cast<uint32_t>(pxW);
    outHeight = static_cast<uint32_t>(pxH);

    // RGBA8 buffer. Deferred geometry pass doesn't alpha-blend, so use an opaque
    // dark-blue background plate with bright white glyphs — reads as a clean sign.
    std::vector<uint8_t> px(static_cast<size_t>(pxW) * pxH * 4);
    for (size_t i = 0; i < px.size(); i += 4) {
        px[i + 0] = 8; px[i + 1] = 16; px[i + 2] = 34; px[i + 3] = 255;
    }
    auto setPixel = [&](int x, int y) {
        for (int sy = 0; sy < pixelScale; ++sy) {
            for (int sx = 0; sx < pixelScale; ++sx) {
                const int X = x * pixelScale + sx;
                const int Y = y * pixelScale + sy;
                const size_t i = (static_cast<size_t>(Y) * pxW + X) * 4;
                px[i + 0] = 255; px[i + 1] = 255; px[i + 2] = 255; px[i + 3] = 255;
            }
        }
    };

    int cursorX = pad;
    for (char c : text) {
        const Glyph& g = GlyphFor(c);
        for (int row = 0; row < GlyphHeight; ++row) {
            const uint8_t bits = g[row];
            for (int col = 0; col < kGlyphW; ++col) {
                if (bits & (1 << (kGlyphW - 1 - col))) {
                    setPixel(cursorX + col, pad + row);
                }
            }
        }
        cursorX += kGlyphW + kSpacing;
    }

    return BeTexture::Create(registryName)
        .SetSize(static_cast<uint32_t>(pxW), static_cast<uint32_t>(pxH))
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillFromMemory(px.data())
        .AddToRegistry()
        .Build();
}

}  // namespace TextLabel
