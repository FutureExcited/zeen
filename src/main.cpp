#include <iostream>

#include "App.h"

auto main() -> int {
    try {
        App app;
        return app.Run();
    } catch (const std::exception& e) {
        std::cerr << "zeen failed: " << e.what() << '\n';
        return 1;
    }
}
