#pragma once

#include <filesystem>

// Reads an image from the OS clipboard and writes it as a PNG to `destPath`.
// Returns true if the clipboard held an image and it was saved. macOS impl uses
// NSPasteboard (see ClipboardImage.mm); other platforms return false for now.
namespace ClipboardImage {
    auto SaveToPng(const std::filesystem::path& destPath) -> bool;
    auto HasImage() -> bool;
}
