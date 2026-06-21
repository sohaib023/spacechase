#include "core/App.hpp"

// Entry point. SDL3 provides its own main shim via <SDL3/SDL_main.h>, which
// must be included exactly once in the translation unit that defines main().
#include <SDL3/SDL_main.h>

int main(int /*argc*/, char* /*argv*/[]) {
    sc::App app;
    if (!app.init()) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
