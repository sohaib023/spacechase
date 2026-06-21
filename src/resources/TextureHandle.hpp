#pragma once

// Lightweight handle to a texture owned by the TextureRegistry. Components store
// this (not a raw Texture*), so the registry owns lifetimes and the value stays
// trivially copyable / serializable.

#include <cstdint>

namespace sc {

using TextureHandle = std::uint32_t;
inline constexpr TextureHandle kInvalidTexture = 0xFFFFFFFFu;

} // namespace sc
