#pragma once

// Thin logging layer over SDL3's categorized/prioritized logging. No external
// dependency (anti-bloat: SDL already provides this). Use a dedicated app log
// category so engine vs. SDL-internal messages stay distinguishable.
//
// Usage: SC_LOG_INFO("loaded %d ships", count);

#include <SDL3/SDL_log.h>

namespace sc {

// Application log category. SDL_LOG_CATEGORY_CUSTOM is the first id reserved
// for application use; later subsystems can add CUSTOM + 1, + 2, ...
inline constexpr int kLogCategoryApp = SDL_LOG_CATEGORY_CUSTOM;

} // namespace sc

#define SC_LOG_TRACE(...) SDL_LogTrace(sc::kLogCategoryApp, __VA_ARGS__)
#define SC_LOG_DEBUG(...) SDL_LogDebug(sc::kLogCategoryApp, __VA_ARGS__)
#define SC_LOG_INFO(...)  SDL_LogInfo(sc::kLogCategoryApp, __VA_ARGS__)
#define SC_LOG_WARN(...)  SDL_LogWarn(sc::kLogCategoryApp, __VA_ARGS__)
#define SC_LOG_ERROR(...) SDL_LogError(sc::kLogCategoryApp, __VA_ARGS__)
