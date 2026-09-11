[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Capture,
    [string]$PlaytraceRoot = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'playtrace')
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$python = Join-Path $PlaytraceRoot '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $python)) { throw 'Run uv sync --extra dev in Play Trace first, or set -PlaytraceRoot.' }
$capturePath = (Resolve-Path -LiteralPath $Capture).Path
$snapshot = Join-Path $projectRoot '.playtrace\snapshot.json'
$database = Join-Path $projectRoot '.playtrace\playtrace.db'
& $python -m playtrace snapshot $projectRoot --local --db $database --output $snapshot | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Local snapshot failed.' }
& $python -m playtrace record-experiment $projectRoot --snapshot $snapshot `
    --hypothesis (Join-Path $projectRoot 'design\experiments\combat-capture-hypothesis.json') `
    --telemetry $capturePath --relation context `
    --rationale 'Combat infrastructure observation; omitted GDD systems remain untested.'
if ($LASTEXITCODE -ne 0) { throw 'Telemetry import failed.' }
