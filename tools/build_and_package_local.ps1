Param(
    [string[]]$Args
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$repoRoot = Resolve-Path "$scriptDir\.."
Set-Location $repoRoot

$python = if ($env:PYTHON) { $env:PYTHON } else { 'py' }
if (-not (Get-Command $python -ErrorAction SilentlyContinue)) {
    $python = 'python'
}

if (-not (Get-Command $python -ErrorAction SilentlyContinue)) {
    Write-Error 'Python is required to run this helper. Install Python or set the PYTHON environment variable.'
    exit 1
}

$cmake = if ($env:CMAKE) { $env:CMAKE } else { 'cmake' }
if (-not (Get-Command $cmake -ErrorAction SilentlyContinue)) {
    $cmake = 'cmake.exe'
}

if (-not (Get-Command $cmake -ErrorAction SilentlyContinue)) {
    Write-Error 'CMake is required to run this helper. Install CMake or set the CMAKE environment variable to the CMake executable.'
    exit 1
}

Write-Host "Using Python: $python"
Write-Host "Using CMake: $cmake"
& $python tools/build_and_package_local.py --cmake-executable "$cmake" @Args
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
