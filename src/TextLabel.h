#pragma once

#include <memory>
#include <string>

class BeTexture;

// Renders a short string into an RGBA texture using a built-in 5x7 bitmap font
// (uppercased; supports A-Z 0-9 space - . : ! and a few symbols). Zero external
// deps — perfect for portal destination signs floating in 3D.
//
// White glyphs on transparent background; the caller puts the texture on a quad.
namespace TextLabel {
    // Build (or fetch from the asset registry) a texture for `text`. `pixelScale`
    // is how many texels each font pixel becomes (bigger = crisper when scaled up).
    // Returns the texture and writes the texel width/height (for aspect).
    auto MakeTexture(const std::string& text, const std::string& registryName,
                     int pixelScale, uint32_t& outWidth, uint32_t& outHeight)
        -> std::shared_ptr<BeTexture>;

    // Pixel size of `text` at scale 1 (before pixelScale), so callers can size quads.
    auto MeasureWidth(const std::string& text) -> int;
    constexpr int GlyphHeight = 7;
}
