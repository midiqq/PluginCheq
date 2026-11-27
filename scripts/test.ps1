<#
.SYNOPSIS
    Ejecuta la suite completa de 13 pruebas unitarias y de estrés en Catch2 v3.
#>
[CmdletBinding()]
param (
    [string]$Config = "Release",
    [string]$TagFilter = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $RepoRoot

$TestExe = "$RepoRoot\build-vst3\Release\dsp_tests.exe"
if (-not (Test-Path $TestExe)) {
    $TestExe = "$RepoRoot\build-vst3\tests\Release\dsp_tests.exe"
}

if (-not (Test-Path $TestExe)) {
    Write-Error "No se encontró el ejecutable de pruebas ($TestExe). Ejecuta scripts/build.ps1 primero."
}

Write-Host "==> [PluginCheq QA] Ejecutando suite de pruebas unitarias Catch2..." -ForegroundColor Cyan
if ($TagFilter -ne "") {
    & $TestExe "$TagFilter"
} else {
    & $TestExe
}
