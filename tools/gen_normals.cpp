// Generates placeholder normal maps for sprites. Hand-run asset tool, not part
// of the game: `cmake --build --preset <preset> --target normal_maps`, then
// commit assets/sprites/*_normal.png.
//
// Height = alpha * (0.5 + 0.5*luminance): alpha shapes the silhouette (rounded
// body for sprites with transparency), luminance adds drawn detail (planks /
// rivets on opaque tiles). Central-difference gradient -> tangent-space normal.
//
// Convention: normal.x = -dH/dx (image x == world +X); normal.y = +dH/dy_image
// (image rows go down = world -Y, so the world-up component is +dH/dy_image).
// The shader rotates normal.xy by the sprite rotation. If lighting looks
// inverted in the rotation test, flip the ny sign here (one place).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>
#include <vector>

namespace {

constexpr double kStrength = 6.0;

struct Pair {
    const char* src;
    const char* out;
};
constexpr Pair kPairs[] = {
    {"ship_sheet.png", "ship_sheet_normal.png"},
    {"wall.png", "wall_normal.png"},
    {"crate.png", "crate_normal.png"},
};

// Round half to even, like the original PowerShell [Math]::Round.
unsigned char to_byte(double v) {
    return static_cast<unsigned char>(std::clamp(std::nearbyint(v), 0.0, 255.0));
}

bool generate(const std::string& dir, const Pair& pair) {
    int w = 0;
    int h = 0;
    int channels = 0;
    const std::string srcPath = dir + "/" + pair.src;
    unsigned char* px = stbi_load(srcPath.c_str(), &w, &h, &channels, 4);
    if (px == nullptr) {
        std::fprintf(stderr, "failed to load %s: %s\n", srcPath.c_str(), stbi_failure_reason());
        return false;
    }

    std::vector<double> heights(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < heights.size(); ++i) {
        const unsigned char* p = px + i * 4;
        const double lum = (0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]) / 255.0;
        heights[i] = (p[3] / 255.0) * (0.5 + 0.5 * lum);
    }

    std::vector<unsigned char> out(heights.size() * 4);
    for (int y = 0; y < h; ++y) {
        const int ym = std::max(0, y - 1);
        const int yp = std::min(h - 1, y + 1);
        for (int x = 0; x < w; ++x) {
            const int xm = std::max(0, x - 1);
            const int xp = std::min(w - 1, x + 1);
            const double dx = heights[y * w + xp] - heights[y * w + xm];
            const double dy = heights[yp * w + x] - heights[ym * w + x];
            double nx = -dx * kStrength;
            double ny = dy * kStrength;
            double nz = 1.0;
            const double len = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= len;
            ny /= len;
            nz /= len;
            const size_t i = static_cast<size_t>(y) * w + x;
            out[i * 4 + 0] = to_byte((nx * 0.5 + 0.5) * 255.0);
            out[i * 4 + 1] = to_byte((ny * 0.5 + 0.5) * 255.0);
            out[i * 4 + 2] = to_byte((nz * 0.5 + 0.5) * 255.0);
            out[i * 4 + 3] = px[i * 4 + 3];
        }
    }
    stbi_image_free(px);

    const std::string outPath = dir + "/" + pair.out;
    if (stbi_write_png(outPath.c_str(), w, h, 4, out.data(), w * 4) == 0) {
        std::fprintf(stderr, "failed to write %s\n", outPath.c_str());
        return false;
    }
    std::printf("generated %s -> %s (%dx%d)\n", pair.src, pair.out, w, h);
    return true;
}

} // namespace

int main() {
    const std::string dir = std::string(SC_ASSET_DIR) + "/sprites";
    bool ok = true;
    for (const Pair& pair : kPairs) {
        ok = generate(dir, pair) && ok;
    }
    return ok ? 0 : 1;
}
