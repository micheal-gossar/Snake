param(
    [ValidateSet('Configure', 'Build', 'Run', 'BuildAndRun', 'Clean')]
    [string]$Action = 'Build',
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$preset = $Configuration.ToLowerInvariant()
$buildDir = Join-Path $root "build\$preset"

try {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw 'Install Visual Studio 2022 or newer with Desktop development with C++ and C++ CMake tools for Windows.'
    }
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $vs) {
        throw 'No Visual Studio C++ toolchain found. Install the Desktop development with C++ workload.'
    }
    $env:PATH = "$(Split-Path -Parent $vswhere);$env:PATH"
    $devCmd = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
    $environment = & $env:ComSpec /d /s /c "`"$devCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not initialize the Visual Studio x64 developer environment.'
    }
    foreach ($line in $environment) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }

    # Prefer native Windows CMake over an MSYS installation on PATH.
    $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path $cmake)) {
        throw 'Install the Visual Studio component C++ CMake tools for Windows (includes CMake and Ninja).'
    }

    function Invoke-CMake {
        & $cmake @args
        if ($LASTEXITCODE -ne 0) {
            throw "CMake failed with exit code $LASTEXITCODE."
        }
    }

    Push-Location $root
    try {
        if ($Action -in @('Build', 'BuildAndRun', 'Run')) {
            $sources = Get-ChildItem (Join-Path $root 'src') -Recurse -File |
                Where-Object { $_.Extension -in @('.c', '.cc', '.cpp', '.cxx') }
            if (-not $sources) {
                # Do not let Run or F5 launch an executable from a previous build.
                foreach ($name in @('snake.exe', 'snake.pdb', 'snake.ilk')) {
                    $output = Join-Path $buildDir "bin\$name"
                    if (Test-Path $output) {
                        Remove-Item -LiteralPath $output
                    }
                }
                if ($Action -in @('BuildAndRun', 'Run')) {
                    throw 'No application sources in src. Add an entry point before running.'
                }
            }
        }
        if ($Action -in @('Configure', 'Build', 'BuildAndRun')) {
            Invoke-CMake --preset $preset
        }
        if ($Action -in @('Build', 'BuildAndRun')) {
            Invoke-CMake --build --preset $preset --parallel
        }
        if ($Action -eq 'Clean') {
            if (Test-Path (Join-Path $buildDir 'CMakeCache.txt')) {
                Invoke-CMake --build --preset $preset --target clean
            } else {
                Write-Host "Nothing to clean for $Configuration."
            }
        }
        if ($Action -in @('Run', 'BuildAndRun')) {
            $executable = Join-Path $buildDir 'bin\snake.exe'
            if (-not (Test-Path $executable)) {
                throw "Executable not found. Build $Configuration first, or use BuildAndRun."
            }
            Push-Location (Split-Path -Parent $executable)
            try {
                & $executable
                if ($LASTEXITCODE -ne 0) {
                    throw "Application failed with exit code $LASTEXITCODE."
                }
            } finally {
                Pop-Location
            }
        }
    } finally {
        Pop-Location
    }
} catch {
    Write-Error -Message $_ -ErrorAction Continue
    exit 1
}
