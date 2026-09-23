# Builds and runs the PC unit tests for src/core (MSYS2 ucrt64 g++).
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"
& C:\msys64\usr\bin\make.exe -C (Join-Path $root "tests") run
if ($LASTEXITCODE -ne 0) { throw "Core tests failed" }
