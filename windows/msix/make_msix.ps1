<#
.SYNOPSIS
  Packs the deployed Windows release folder into an MSIX package for the Microsoft Store.

    windows/msix/make_msix.ps1 -Stage release -Manifest AppxManifest.xml -Out gruepr.msix

  -Stage is the folder after windeployqt and the runtime DLLs are copied in: the same files the
  NSIS installer packs. -Manifest is the AppxManifest.xml that qmake writes into the build folder
  from windows/msix/AppxManifest.xml.in, which it does only for an app store build (qmake with
  CONFIG+=appstore, as the build-windows-store CI job runs it). The package is those files plus the manifest, the Store
  images in windows/msix/Assets (drawn by make_assets.ps1) and a resources.pri that indexes them.
  It is unsigned: the Store signs packages after upload. Uses makepri and makeappx from the
  newest Windows SDK installed.
#>
param(
    [Parameter(Mandatory = $true)][string]$Stage,
    [Parameter(Mandatory = $true)][string]$Manifest,
    [Parameter(Mandatory = $true)][string]$Out
)
$ErrorActionPreference = 'Stop'

function Invoke-Tool([string]$exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$(Split-Path -Leaf $exe) failed with exit code $LASTEXITCODE" }
}

$Stage = (Resolve-Path $Stage).Path
$Manifest = (Resolve-Path $Manifest).Path
$Out = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Out)
$assets = Join-Path $PSScriptRoot 'Assets'
if (-not (Test-Path (Join-Path $Stage 'gruepr.exe'))) {
    throw "$Stage has no gruepr.exe: build and deploy into it first"
}

# The newest Windows SDK that has both tools.
$kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
$sdk = Get-ChildItem $kits -Directory |
    Where-Object { $_.Name -match '^10\.[0-9.]+$' -and
                   (Test-Path (Join-Path $_.FullName 'x64\makeappx.exe')) -and
                   (Test-Path (Join-Path $_.FullName 'x64\makepri.exe')) } |
    Sort-Object { [version]$_.Name } | Select-Object -Last 1
if (-not $sdk) { throw "No Windows SDK with makeappx.exe and makepri.exe under $kits" }
$makeappx = Join-Path $sdk.FullName 'x64\makeappx.exe'
$makepri = Join-Path $sdk.FullName 'x64\makepri.exe'
Write-Host "Windows SDK $($sdk.Name)"

$work = Join-Path ([IO.Path]::GetTempPath()) ('gruepr-msix-' + [guid]::NewGuid().ToString('N'))
$root = Join-Path $work 'package'
$priRoot = Join-Path $work 'pri'
try {
    # Package contents: the deployed files, the manifest and the Store images.
    New-Item -ItemType Directory $root, $priRoot | Out-Null
    Copy-Item (Join-Path $Stage '*') $root -Recurse
    Copy-Item $Manifest (Join-Path $root 'AppxManifest.xml')
    Copy-Item $assets (Join-Path $root 'Assets') -Recurse

    # resources.pri lets the manifest's Assets\Square44x44Logo.png etc. resolve to the scale-
    # and targetsize-qualified files. It is indexed from a folder with only the manifest and
    # the images, so the DLLs and plugins aren't listed as resources.
    Copy-Item $Manifest (Join-Path $priRoot 'AppxManifest.xml')
    Copy-Item $assets (Join-Path $priRoot 'Assets') -Recurse
    $priConfig = Join-Path $work 'priconfig.xml'
    Invoke-Tool $makepri @('createconfig', '/cf', $priConfig, '/dq', 'en-US', '/pv', '10.0.0', '/o')
    # The default config splits the resources into one .pri per scale and language, for
    # bundles; a single package reads only resources.pri, so everything goes into that one.
    $cfg = New-Object System.Xml.XmlDocument
    $cfg.Load($priConfig)
    $split = $cfg.DocumentElement.SelectSingleNode('packaging')
    if ($split) { [void]$cfg.DocumentElement.RemoveChild($split) }
    $cfg.Save($priConfig)
    Invoke-Tool $makepri @('new', '/pr', $priRoot, '/cf', $priConfig,
                           '/mn', (Join-Path $priRoot 'AppxManifest.xml'),
                           '/of', (Join-Path $root 'resources.pri'), '/o')

    Invoke-Tool $makeappx @('pack', '/d', $root, '/p', $Out, '/o')
    Write-Host "Wrote $Out"
}
finally {
    Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
}
