# Generates placeholder normal maps for sprites (run by hand, like
# compile_shaders.ps1 — NOT part of the game build). Height = alpha * (0.5 +
# 0.5*luminance): alpha shapes the silhouette (rounded body for sprites with
# transparency), luminance adds drawn detail (planks/rivets on opaque tiles).
# Central-difference gradient -> tangent-space normal.
#
# Convention: normal.x = -dH/dx (image x == world +X); normal.y = +dH/dy_image
# (image rows go down = world -Y, so the world-up component is +dH/dy_image).
# The shader rotates normal.xy by the sprite rotation. If lighting looks
# inverted in the rotation test, flip the $ny sign here (one place).
#
# NOTE: PowerShell variable names are case-insensitive, so array names must not
# collide with the loop scalars ($w/$h) — hence $heights/$alphas.

Add-Type -AssemblyName System.Drawing

$dir = Join-Path (Split-Path -Parent $PSScriptRoot) "assets\sprites"
$strength = 6.0
$pairs = @(
    @{ src = "ship_sheet.png"; out = "ship_sheet_normal.png" },
    @{ src = "wall.png";       out = "wall_normal.png" },
    @{ src = "crate.png";      out = "crate_normal.png" }
)

function Clamp255([double]$v) { return [int][Math]::Max(0, [Math]::Min(255, [Math]::Round($v))) }

foreach ($pair in $pairs) {
    $src = New-Object System.Drawing.Bitmap (Join-Path $dir $pair.src)
    $w = [int]$src.Width
    $imgH = [int]$src.Height
    $count = $w * $imgH
    $heights = [double[]]::new($count)
    $alphas = [int[]]::new($count)

    for ($y = 0; $y -lt $imgH; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $px = $src.GetPixel($x, $y)
            $idx = $y * $w + $x
            $alphas[$idx] = $px.A
            $alphaNorm = $px.A / 255.0
            $lum = (0.299 * $px.R + 0.587 * $px.G + 0.114 * $px.B) / 255.0
            $heights[$idx] = $alphaNorm * (0.5 + 0.5 * $lum)
        }
    }

    $out = New-Object System.Drawing.Bitmap($w, $imgH, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $imgH; $y++) {
        $ym = [Math]::Max(0, $y - 1); $yp = [Math]::Min($imgH - 1, $y + 1)
        for ($x = 0; $x -lt $w; $x++) {
            $xm = [Math]::Max(0, $x - 1); $xp = [Math]::Min($w - 1, $x + 1)
            $dx = $heights[$y * $w + $xp] - $heights[$y * $w + $xm]
            $dy = $heights[$yp * $w + $x] - $heights[$ym * $w + $x]
            $nx = -$dx * $strength
            $ny = $dy * $strength
            $nz = 1.0
            $len = [Math]::Sqrt($nx * $nx + $ny * $ny + $nz * $nz)
            $nx /= $len; $ny /= $len; $nz /= $len
            $r = Clamp255(($nx * 0.5 + 0.5) * 255.0)
            $g = Clamp255(($ny * 0.5 + 0.5) * 255.0)
            $b = Clamp255(($nz * 0.5 + 0.5) * 255.0)
            $alpha = $alphas[$y * $w + $x]
            $color = [System.Drawing.Color]::FromArgb($alpha, $r, $g, $b)
            $out.SetPixel($x, $y, $color)
        }
    }
    $out.Save((Join-Path $dir $pair.out), [System.Drawing.Imaging.ImageFormat]::Png)
    $src.Dispose(); $out.Dispose()
    Write-Host ("generated {0} -> {1} ({2}x{3})" -f $pair.src, $pair.out, $w, $imgH)
}
Write-Host "Done."
