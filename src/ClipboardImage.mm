#include "ClipboardImage.h"

#if defined(__APPLE__)

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace ClipboardImage {

auto HasImage() -> bool {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSArray* classes = @[ [NSImage class] ];
        return [pb canReadObjectForClasses:classes options:nil];
    }
}

auto SaveToPng(const std::filesystem::path& destPath) -> bool {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];

        // Prefer raw image data on the pasteboard (PNG/TIFF), fall back to NSImage.
        NSData* imageData = nil;
        if (NSData* png = [pb dataForType:NSPasteboardTypePNG]) {
            imageData = png;
        } else if (NSData* tiff = [pb dataForType:NSPasteboardTypeTIFF]) {
            imageData = tiff;
        } else {
            NSArray* classes = @[ [NSImage class] ];
            NSArray* objs = [pb readObjectsForClasses:classes options:nil];
            if (objs.count > 0) {
                NSImage* img = objs.firstObject;
                imageData = [img TIFFRepresentation];
            }
        }
        if (!imageData) return false;

        // Normalise whatever we got into PNG bytes.
        NSBitmapImageRep* rep = [NSBitmapImageRep imageRepWithData:imageData];
        if (!rep) {
            NSImage* img = [[NSImage alloc] initWithData:imageData];
            if (img) rep = [NSBitmapImageRep imageRepWithData:[img TIFFRepresentation]];
        }
        if (!rep) return false;

        NSData* pngData = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        if (!pngData) return false;

        NSString* path = [NSString stringWithUTF8String:destPath.string().c_str()];
        return [pngData writeToFile:path atomically:YES] == YES;
    }
}

}  // namespace ClipboardImage

#else  // non-Apple stub

namespace ClipboardImage {
    auto HasImage() -> bool { return false; }
    auto SaveToPng(const std::filesystem::path&) -> bool { return false; }
}

#endif
