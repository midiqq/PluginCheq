<#
.SYNOPSIS
    Compila el target de producción PluginCheq (VST3) y la suite de pruebas Catch2 en modo Release.
#>
[CmdletBinding()]
param (
    [string]$Config = "Release",
    [int]$Jobs = 8
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $RepoRoot

Write-Host "==> [PluginCheq Build] Configurando CMake en $Config..." -ForegroundColor Cyan
if (-not (Test-Path "$RepoRoot\build-vst3\CMakeCache.txt")) {
    cmake -B build-vst3 -G "Visual Studio 17 2022" -A x64
}

Write-Host "==> [PluginCheq Build] Compilando VST3 y Tests..." -ForegroundColor Cyan
cmake --build build-vst3 --config $Config -j $Jobs

Write-Host "==> [PluginCheq Build] Compilación finalizada con éxito." -ForegroundColor Green
