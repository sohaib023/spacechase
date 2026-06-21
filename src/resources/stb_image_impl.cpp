// Single translation unit that compiles the stb_image implementation. Kept
// isolated so the library's internal warnings don't trip our -Werror policy;
// CMake compiles this file with warnings disabled (it's vendored third-party).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG // M1 only needs PNG; widen here if other formats appear.
#include <stb_image.h>
