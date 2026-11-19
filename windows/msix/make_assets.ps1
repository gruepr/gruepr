<#
.SYNOPSIS
  Draws the Microsoft Store package images in windows/msix/Assets from gruepr's icon.

    windows/msix/make_assets.ps1

  The source is icons_new/icon-1024x1024.png, cropped to the fish and centered in a square the
  way icons.ico is, so the Start menu and taskbar show the same icon as the installed (NSIS) copy.
  Run it again if the icon changes, and commit the images it writes.

  The qualifiers in the file names are what the package's resources.pri (made by make_msix.ps1)
  picks a size by: scale-100/200 for 100% and 200% display scaling, targetsize-N for the taskbar
  and Start list (altform-unplated: drawn without a colored plate behind it). The medium Start
  tile (Square150x150Logo) has the icon at two thirds of its width, centered, as Windows' tile
  guidelines suggest.
#>
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = Join-Path $PSScriptRoot '..\..\icons_new\icon-1024x1024.png'
$outDir = Join-Path $PSScriptRoot 'Assets'
New-Item -ItemType Directory -Force $outDir | Out-Null

$icon = [System.Drawing.Bitmap]::new((Resolve-Path $source).Path)
try {
    # The fish's bounding box: every pixel that isn't (nearly) transparent.
    $rect = [System.Drawing.Rectangle]::new(0, 0, $icon.Width, $icon.Height)
    $data = $icon.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                           [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($data.Stride * $icon.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $stride = $data.Stride
    $icon.UnlockBits($data)
    $minX = $icon.Width; $minY = $icon.Height; $maxX = -1; $maxY = -1
    for ($y = 0; $y -lt $icon.Height; $y++) {
        $row = $y * $stride
        for ($x = 0; $x -lt $icon.Width; $x++) {
            if ($bytes[$row + 4 * $x + 3] -gt 8) {
                if ($x -lt $minX) { $minX = $x }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    if ($maxX -lt 0) { throw "$source is fully transparent" }

    # A square around the fish, centered on them.
    $side = [Math]::Max($maxX - $minX + 1, $maxY - $minY + 1)
    $cropX = ($minX + $maxX + 1 - $side) / 2
    $cropY = ($minY + $maxY + 1 - $side) / 2
    Write-Host ("Icon art: x {0}-{1}, y {2}-{3}; square of {4} px" -f $minX, $maxX, $minY, $maxY, $side)

    # One image: a transparent canvas x canvas square with the icon drawn size x size in its center.
    function Save-Padded([string]$name, [int]$canvas, [int]$size) {
        $image = [System.Drawing.Bitmap]::new($canvas, $canvas, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $g = [System.Drawing.Graphics]::FromImage($image)
            try {
                $g.Clear([System.Drawing.Color]::Transparent)
                $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                # Tile-flip wrapping keeps transparent edge pixels from bleeding gray into the border.
                $wrap = [System.Drawing.Imaging.ImageAttributes]::new()
                $wrap.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY)
                $off = [Math]::Floor(($canvas - $size) / 2)
                $g.DrawImage($icon, [System.Drawing.Rectangle]::new($off, $off, $size, $size),
                             [single]$cropX, [single]$cropY, [single]$side, [single]$side,
                             [System.Drawing.GraphicsUnit]::Pixel, $wrap)
                $wrap.Dispose()
            }
            finally { $g.Dispose() }
            $image.Save((Join-Path $outDir $name), [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally { $image.Dispose() }
        Write-Host "  $name"
    }

    foreach ($scale in 100, 200) {
        $f = $scale / 100
        Save-Padded ("StoreLogo.scale-{0}.png" -f $scale) (50 * $f) (50 * $f)
        Save-Padded ("Square44x44Logo.scale-{0}.png" -f $scale) (44 * $f) (44 * $f)
        Save-Padded ("Square150x150Logo.scale-{0}.png" -f $scale) (150 * $f) (100 * $f)
    }
    foreach ($size in 16, 24, 32, 48, 256) {
        Save-Padded ("Square44x44Logo.targetsize-{0}.png" -f $size) $size $size
        Save-Padded ("Square44x44Logo.targetsize-{0}_altform-unplated.png" -f $size) $size $size
    }
}
finally { $icon.Dispose() }
Write-Host "Wrote $outDir"
