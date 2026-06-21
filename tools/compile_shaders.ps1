# Compiles HLSL shaders to DXIL for the D3D12 backend (M1 staged approach).
# Run from a Developer PowerShell, or it will locate dxc in the Windows SDK.
# Later this is replaced by an SDL_shadercross build step that also emits
# SPIR-V (Vulkan) and MSL (Metal) from the same HLSL sources.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$shaderDir = Join-Path $root "assets\shaders"
$outDir = Join-Path $shaderDir "compiled"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Find dxc: prefer PATH (Developer shell), else newest Windows SDK copy.
$dxc = (Get-Command dxc.exe -ErrorAction SilentlyContinue).Source
if (-not $dxc) {
    $dxc = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\dxc.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName | Select-Object -Last 1 -Expand FullName
}
if (-not $dxc) { throw "dxc.exe not found (install Windows SDK or run from a Developer shell)." }
Write-Host "Using dxc: $dxc"

# (source.hlsl, entrypoint, profile, output) tuples.
$jobs = @(
    @{ src = "sprite.hlsl";     entry = "vs_main"; profile = "vs_6_0"; out = "sprite.vert.dxil" },
    @{ src = "sprite.hlsl";     entry = "ps_main"; profile = "ps_6_0"; out = "sprite.frag.dxil" },
    @{ src = "lit_sprite.hlsl"; entry = "vs_main"; profile = "vs_6_0"; out = "lit_sprite.vert.dxil" },
    @{ src = "lit_sprite.hlsl"; entry = "ps_main"; profile = "ps_6_0"; out = "lit_sprite.frag.dxil" }
)
foreach ($j in $jobs) {
    $in = Join-Path $shaderDir $j.src
    $o = Join-Path $outDir $j.out
    & $dxc -T $j.profile -E $j.entry -Fo $o $in
    if ($LASTEXITCODE -ne 0) { throw "dxc failed for $($j.src):$($j.entry)" }
    Write-Host ("compiled {0}:{1} -> {2}" -f $j.src, $j.entry, $j.out)
}
Write-Host "Done."
