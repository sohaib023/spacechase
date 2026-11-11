#!/usr/bin/env bash
# Sets up an Ubuntu (24.04+) machine for Space Chase development.
#
# Downloads the pinned toolchain (clang/LLVM, ninja, dxc) into <repo>/.tools and
# checks the system packages SDL3 needs. Nothing is installed system-wide unless
# you pass --install-apt (then missing packages are installed with sudo apt-get).
# Safe to re-run: tools already at the pinned version are skipped.
# Details: docs/getting-started.md (Linux section).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS="$ROOT/.tools"
CACHE="$TOOLS/cache"

# --- Pinned downloads (bump version + url + sha256 together) ----------------
# LLVM 19 matches the clang-cl 19 used on Windows, so both report the same warnings.
LLVM_VERSION=19.1.7
LLVM_URL=https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-Linux-X64.tar.xz
LLVM_SHA256=4a5ec53951a584ed36f80240f6fbf8fdd46b4cf6c7ee87cc2d5018dc37caf679
# Only these parts of the ~1.7 GB archive are extracted (keeps .tools under 1 GB).
LLVM_PARTS=(
    bin/clang bin/clang++ bin/clang-19 bin/clang-cpp
    bin/lld bin/ld.lld
    bin/clangd bin/clang-format bin/clang-tidy bin/llvm-symbolizer
    lib/clang
)

NINJA_VERSION=1.13.2
NINJA_URL=https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-linux.zip
NINJA_SHA256=5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6

DXC_VERSION=1.9.2609
DXC_URL=https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/linux_dxc_2026_09_28.x86_x64.tar.gz
DXC_SHA256=96faadc7f5c282d2ffda49804beb4c3ee38127bc252b723234e3c5cdf7aa39a1

# --- System packages ---------------------------------------------------------
# Required: compilers' C++ runtime, build tools, and the SDL3 video/audio/input
# backends (without the -dev headers SDL3 silently builds without a window system).
REQUIRED_PKGS=(
    build-essential cmake git pkg-config curl xz-utils unzip gdb
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev
    libxkbcommon-dev libwayland-dev libdecor-0-dev libegl-dev
    libasound2-dev libpulse-dev libdbus-1-dev libudev-dev
    libvulkan1 mesa-vulkan-drivers
)
# Optional: extra SDL3 features we don't rely on today.
OPTIONAL_PKGS=(libpipewire-0.3-dev libibus-1.0-dev libdrm-dev libgbm-dev)

INSTALL_APT=0
for arg in "$@"; do
    case "$arg" in
        --install-apt) INSTALL_APT=1 ;;
        -h|--help) sed -n '2,9p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

log() { printf '\033[1m==> %s\033[0m\n' "$*" >&2; }
die() { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }

missing_pkgs() {
    local p
    for p in "$@"; do
        dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q 'install ok installed' || echo "$p"
    done
}

# fetch URL SHA256 -> prints the path of the verified file in the cache.
fetch() {
    local url=$1 sha=$2 file
    file="$CACHE/$(basename "$url")"
    mkdir -p "$CACHE"
    if ! { [[ -f $file ]] && echo "$sha  $file" | sha256sum -c --status; }; then
        log "downloading $(basename "$url")"
        curl -fL --progress-bar -o "$file.part" "$url"
        mv "$file.part" "$file"
        echo "$sha  $file" | sha256sum -c --status || die "checksum mismatch: $file"
    fi
    printf '%s' "$file"
}

# Each tool dir carries a .version stamp; a matching stamp means "done".
is_current() { [[ -f $TOOLS/$1/.version && $(<"$TOOLS/$1/.version") == "$2" ]]; }

install_tool() { # name version url sha256 extract-function
    local name=$1 version=$2 archive
    if is_current "$name" "$version"; then
        log "$name $version already installed"
        return
    fi
    archive=$(fetch "$3" "$4")
    log "installing $name $version"
    rm -rf "${TOOLS:?}/$name"
    mkdir -p "$TOOLS/$name"
    "$5" "$archive" "$TOOLS/$name"
    echo "$version" > "$TOOLS/$name/.version"
    rm -f "$archive"
}

extract_ninja() { unzip -q -o "$1" -d "$2"; chmod +x "$2/ninja"; }
extract_dxc() { tar -xzf "$1" -C "$2" ./bin ./lib; }
extract_llvm() {
    local top="LLVM-$LLVM_VERSION-Linux-X64" parts=() p
    for p in "${LLVM_PARTS[@]}"; do parts+=("$top/$p"); done
    tar -xJf "$1" -C "$2" --strip-components=1 "${parts[@]}"
}

# --- 1. System packages -------------------------------------------------------
[[ -r /etc/os-release ]] && . /etc/os-release
[[ ${ID:-} == ubuntu ]] || log "warning: written for Ubuntu, this is ${PRETTY_NAME:-unknown}"

mapfile -t MISSING < <(missing_pkgs "${REQUIRED_PKGS[@]}")
mapfile -t MISSING_OPT < <(missing_pkgs "${OPTIONAL_PKGS[@]}")
if ((${#MISSING[@]})); then
    if ((INSTALL_APT)); then
        log "installing system packages: ${MISSING[*]}"
        sudo apt-get update && sudo apt-get install -y "${MISSING[@]}"
    else
        die "missing system packages. Install them with:
    sudo apt-get install ${MISSING[*]}
or re-run this script with --install-apt."
    fi
else
    log "required system packages present"
fi
((${#MISSING_OPT[@]})) && log "optional packages not installed (fine): ${MISSING_OPT[*]}"

CMAKE_VER=$(cmake --version | head -1 | awk '{print $3}')
[[ $(printf '3.24\n%s\n' "$CMAKE_VER" | sort -V | head -1) == 3.24 ]] ||
    die "cmake $CMAKE_VER is too old (need >= 3.24)"

# --- 2. Local toolchain -------------------------------------------------------
install_tool ninja "$NINJA_VERSION" "$NINJA_URL" "$NINJA_SHA256" extract_ninja
install_tool dxc   "$DXC_VERSION"   "$DXC_URL"   "$DXC_SHA256"   extract_dxc
install_tool llvm  "$LLVM_VERSION"  "$LLVM_URL"  "$LLVM_SHA256"  extract_llvm
rmdir "$CACHE" 2>/dev/null || true

# --- 3. Verify ----------------------------------------------------------------
log "toolchain in $TOOLS"
"$TOOLS/llvm/bin/clang++" --version | head -1
echo "ninja $("$TOOLS/ninja/ninja" --version)"
"$TOOLS/dxc/bin/dxc" --version
echo "cmake $CMAKE_VER, $(gdb --version | head -1)"

cat >&2 <<EOF

Done. Next (from $ROOT):
  cmake --preset linux-clang
  cmake --build --preset linux-clang-debug
  ctest --preset linux-clang
  ./build/linux-clang/bin/Debug/SpaceChase
EOF
