<#
.SYNOPSIS
    Empaqueta los binarios VST3 y la documentación para distribución final en dist/.
#>
[CmdletBinding()]
param ()

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $RepoRoot

$SourceVst3 = "$RepoRoot\build-vst3\PluginCheq_artefacts\Release\VST3\PluginCheq.vst3"
$DistDir = "$RepoRoot\dist"
$DistVst3 = "$DistDir\PluginCheq.vst3"

if (-not (Test-Path $SourceVst3)) {
    Write-Error "El bundle VST3 compilado no existe en $SourceVst3. Ejecuta scripts/build.ps1 primero."
}

Write-Host "==> [PluginCheq Package] Sincronizando bundle VST3 hacia $DistVst3..." -ForegroundColor Cyan
if (-not (Test-Path $DistVst3)) {
    New-Item -ItemType Directory -Force -Path $DistVst3 | Out-Null
}

Copy-Item -Recurse -Force "$SourceVst3\*" "$DistVst3\"
Copy-Item -Force "$RepoRoot\docs\MANUAL_DE_USUARIO.md" "$DistDir\MANUAL_DE_USUARIO.md"

Write-Host "==> [PluginCheq Package] Paquete listo para distribución en dist/." -ForegroundColor Green
