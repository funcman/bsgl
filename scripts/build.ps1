#=====================================================================
# build.ps1 - BSGL one-click build script (PowerShell / Windows)
#
# Usage:
#   scripts\build.ps1                # Release build
#   scripts\build.ps1 -Config Debug  # Debug build
#   scripts\build.ps1 -Clean         # Clean and rebuild (can combine with -Config)
#   scripts\build.ps1 -CleanOnly     # Clean only, do not build
#=====================================================================
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string]$Arch = "x64",
    [switch]$Clean,
    [switch]$CleanOnly
)

$ErrorActionPreference = "Stop"

$Root  = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"

#--- Clean: the whole build/ directory holds every artifact -------------
function Invoke-Clean {
    Write-Host "==> Cleaning build artifacts ..."
    Remove-Item -Recurse -Force $Build -ErrorAction SilentlyContinue
}

if ($Clean -or $CleanOnly) {
    Invoke-Clean
    if ($CleanOnly) { exit 0 }
}

#--- Initialize git submodules (3rd/*) -----------------------------------
# any entry still prefixed with '-' in `git submodule status` has not
# been checked out yet
if (Test-Path (Join-Path $Root ".git")) {
    if (git -C $Root submodule status --recursive 2>$null | Select-String -Pattern '^-' -Quiet) {
        Write-Host "==> Initializing git submodules ..."
        git -C $Root submodule update --init --recursive
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

#--- Deploy the DragonBonesCPP CMake script ------------------------------
# DragonBonesCPP ships no CMake build of its own; once the submodule is
# present, copy our script in as 3rd/DragonBonesCPP/CMakeLists.txt.
$dbcDir = Join-Path $Root "3rd\DragonBonesCPP"
$dbcSrc = Join-Path $Root "scripts\DragonBonesCPP-CMakeLists.txt"
$dbcDst = Join-Path $dbcDir "CMakeLists.txt"
if (Test-Path (Join-Path $dbcDir "DragonBones\src")) {
    if ((-not (Test-Path $dbcDst)) -or
        ((Get-FileHash $dbcSrc).Hash -ne (Get-FileHash $dbcDst).Hash)) {
        Write-Host "==> Deploying 3rd\DragonBonesCPP\CMakeLists.txt ..."
        Copy-Item $dbcSrc $dbcDst -Force
    }
}

#--- Find cmake: prefer PATH, then the one bundled with Visual Studio ---
$cmake = $null

$cmd = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($cmd) {
    $cmake = $cmd.Source
} else {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -property installationPath
        if ($vsPath) {
            $candidate = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $candidate) { $cmake = $candidate }
        }
    }
}

if (-not $cmake) {
    Write-Error "cmake not found; please install CMake or Visual Studio"
}

#--- Existing cache with a different platform blocks reconfigure; wipe it --
$cache = Join-Path $Build "CMakeCache.txt"
if (Test-Path $cache) {
    $m = Select-String -Path $cache -Pattern "^CMAKE_GENERATOR_PLATFORM:INTERNAL=(.*)$"
    $cachedArch = if ($m) { $m.Matches[0].Groups[1].Value.Trim() } else { "" }
    if ($cachedArch -ne $Arch) {
        Write-Host "==> Cached platform '$cachedArch' != '$Arch', reconfiguring from scratch ..."
        Invoke-Clean
    }
}

# Let cmake pick the default generator (newest installed Visual Studio);
# only the target architecture is pinned here.
Write-Host "==> cmake: $cmake"
Write-Host "==> Configuring ($Config / $Arch) ..."
& $cmake -S $Root -B $Build -A $Arch
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building ..."
& $cmake --build $Build --config $Config --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "==> Build complete"
Write-Host "    Binaries:  $Build\bin\$Config"
Write-Host "    Libraries: $Build\lib\$Config"
