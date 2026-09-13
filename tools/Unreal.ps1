[CmdletBinding()]
param(
    [ValidateSet('Generate', 'GenerateMap', 'GenerateVillage', 'VillagePreview', 'GenerateShellMap', 'GenerateAIProfiles', 'Python', 'Build', 'Test', 'EditorTest', 'Smoke', 'CombatSmoke', 'SmugglerSmoke', 'SwampSmoke', 'ShellSmoke', 'Editor', 'Play', 'Shell', 'NetworkTest')]
    [string]$Action = 'Build',
    [string]$EngineRoot = $env:UE_ROOT,
    [int]$Seed = 1927,
    [ValidateSet('Sapper', 'Photographer', 'Medium', 'Smuggler')]
    [string]$Investigator = 'Sapper',
    [ValidateSet('Victory', 'Defeat', 'Revive')]
    [string]$Outcome = 'Victory',
    [string]$Script,
    # Overrides the automation filter for Test (default DreadMeridian.Foundation) and EditorTest (default DreadMeridian.Editor).
    [string]$Filter,
    [switch]$RenderOffscreen,
    [switch]$SwampEnemies,
    [switch]$Sandbox
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'DreadMeridian.uproject'
if (-not $EngineRoot) { $EngineRoot = 'C:\Program Files\Epic Games\UE_5.8' }
$engineBuild = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path -LiteralPath $engineBuild)) { throw 'Set UE_ROOT or -EngineRoot to Unreal Engine 5.8.2.' }
$version = Get-Content -LiteralPath $engineBuild -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8 -or $version.PatchVersion -ne 2) {
    throw 'This foundation is pinned to UE 5.8.2. Review and test an engine upgrade before changing the pin.'
}

$build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if ($Action -eq 'Generate') {
    & $build -projectfiles "-Project=$projectFile" -game -engine
    if ($LASTEXITCODE -ne 0) { throw "Project generation failed: $LASTEXITCODE" }
    exit 0
}

# Build before tests/launch so evidence is not intentionally collected from stale binaries.
& $build DreadMeridianEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed: $LASTEXITCODE" }
if ($Action -eq 'Build') { exit 0 }
if ($Action -eq 'GenerateVillage') {
    & (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $projectFile -run=pythonscript "-script=$PSScriptRoot\generate_village.py" -unattended -nop4 -nullrhi -nosound
    if ($LASTEXITCODE -ne 0) { throw 'Village map generation failed.' }
    exit 0
}

$cmdEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if ($Action -eq 'Python') {
    if (-not $Script -or -not (Test-Path -LiteralPath $Script -PathType Leaf)) { throw 'Python action requires -Script pointing to an existing Python file.' }
    $resolvedScript = (Resolve-Path -LiteralPath $Script).Path
    $renderArgs = if ($RenderOffscreen) { @('-RenderOffscreen') } else { @('-nullrhi') }
    & $cmdEditor $projectFile -run=pythonscript "-script=$resolvedScript" -unattended -nop4 -nosound @renderArgs
    if ($LASTEXITCODE -ne 0) { throw "Unreal Python script failed: $LASTEXITCODE" }
    exit 0
}
if ($Action -eq 'GenerateMap') {
    $generator = Join-Path $PSScriptRoot 'generate_sandbox.py'
    & $cmdEditor $projectFile -run=pythonscript "-script=$generator" -unattended -nop4 -nullrhi -nosound
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath (Join-Path $projectRoot 'Content\DreadMeridian\Maps\L_CombatSandbox.umap'))) {
        throw 'Sandbox map generation failed.'
    }
    exit 0
}
if ($Action -eq 'GenerateShellMap') {
    $generator = Join-Path $PSScriptRoot 'generate_shell.py'
    & $cmdEditor $projectFile -run=pythonscript "-script=$generator" -unattended -nop4 -nullrhi -nosound
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath (Join-Path $projectRoot 'Content\DreadMeridian\Maps\L_Shell.umap'))) {
        throw 'Shell map generation failed.'
    }
    exit 0
}
if ($Action -eq 'GenerateAIProfiles') {
    $generator = Join-Path $PSScriptRoot 'create_ai_profiles.py'
    & $cmdEditor $projectFile -run=pythonscript "-script=$generator" -unattended -nop4 -nullrhi -nosound
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath (Join-Path $projectRoot 'Content\DreadMeridian\AI\AIP_Gunman.uasset'))) {
        throw 'AI profile generation failed.'
    }
    exit 0
}
if ($Action -eq 'Test') {
    $testFilter = if ($Filter) { $Filter } else { 'DreadMeridian.Foundation' }
    $minimum = if ($Filter) { 1 } else { 3 }
    $report = Join-Path $projectRoot ('Saved\Automation\' + [guid]::NewGuid().ToString('N'))
    & $cmdEditor $projectFile -unattended -nop4 -nosplash -nullrhi -nosound `
        "-ExecCmds=Automation RunTests $testFilter" `
        '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report"
    if ($LASTEXITCODE -ne 0) { throw "Automation process failed: $LASTEXITCODE" }
    $result = Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
    if ($result.failed -ne 0 -or $result.succeeded -lt $minimum) { throw 'Foundation tests did not all pass.' }
    Write-Output "Foundation tests passed ($testFilter). Report: $report"
    exit 0
}
if ($Action -eq 'EditorTest') {
    $testFilter = if ($Filter) { $Filter } else { 'DreadMeridian.Editor' }
    $report = Join-Path $projectRoot ('Saved\Automation\' + [guid]::NewGuid().ToString('N'))
    # PIE tests open Slate windows, so this deliberately runs without -nullrhi.
    & $cmdEditor $projectFile -unattended -nop4 -nosplash -nosound `
        "-ExecCmds=Automation RunTests $testFilter" `
        '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report"
    if ($LASTEXITCODE -ne 0) { throw "Automation process failed: $LASTEXITCODE" }
    $result = Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
    if ($result.failed -ne 0 -or $result.succeeded -lt 1) { throw 'Editor tests did not all pass.' }
    Write-Output "Editor tests passed ($testFilter, $($result.succeeded) succeeded). Report: $report"
    exit 0
}

$revision = & git -c "safe.directory=$projectRoot" -C $projectRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot record game revision.' }
$status = & git -c "safe.directory=$projectRoot" -C $projectRoot status --porcelain
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine working tree state.' }
$trackedInputs = [string[]]@(& git -c core.quotePath=false -c "safe.directory=$projectRoot" -C $projectRoot ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate source inputs.' }
[Array]::Sort($trackedInputs, [StringComparer]::Ordinal)
$fingerprintRows = foreach ($relativePath in $trackedInputs) {
    if ($relativePath.StartsWith('"')) { throw 'Quoted Git paths need the Python provenance writer; rename this input before capture.' }
    $absolutePath = Join-Path $projectRoot $relativePath
    if (Test-Path -LiteralPath $absolutePath -PathType Leaf) {
        $hash = (Get-FileHash -LiteralPath $absolutePath -Algorithm SHA256).Hash.ToLowerInvariant()
        "$relativePath`0$hash"
    }
}
$sourceBytes = [Text.Encoding]::UTF8.GetBytes(($fingerprintRows -join "`n"))
$hasher = [Security.Cryptography.SHA256]::Create()
try { $sourceDigest = ([BitConverter]::ToString($hasher.ComputeHash($sourceBytes))).Replace('-', '').ToLowerInvariant() }
finally { $hasher.Dispose() }
$gdd = Join-Path $projectRoot 'gdd\Mythos_PvE_MOBA_Master_GDD_v0.3.md'
$gddDigest = (Get-FileHash -LiteralPath $gdd -Algorithm SHA256).Hash.ToLowerInvariant()
$map = '/Game/DreadMeridian/Maps/L_CombatSandbox'
if ($Action -eq 'VillagePreview' -or ($Action -in @('Play','Editor') -and -not $Sandbox)) { $map='/Game/DreadMeridian/Maps/L_FishingVillage' }
if ($Action -eq 'Smoke') { $map = '/Engine/Maps/Entry?game=/Script/DreadMeridian.DMGameMode' }
# The shell opens the front end; the sandbox streams in when the lobby launches.
if ($Action -in @('Shell', 'ShellSmoke')) { $map = '/Game/DreadMeridian/Maps/L_Shell' }
$launchArgs = @($projectFile, $map, '-PlaytraceCapture', "-DMInvestigator=$Investigator", "-DMSeed=$Seed",
    "-DMGameRevision=$revision", "-DMSourceDigest=$sourceDigest", "-DMGDDDigest=$gddDigest")
if ($status) { $launchArgs += '-DMDirty' }
if ($SwampEnemies -or $Action -eq 'SwampSmoke') { $launchArgs += '-DMSwampProbe' }

if ($Action -eq 'VillagePreview') {
    & $cmdEditor @launchArgs -game -RenderOffscreen -DMVillagePreview -ResX=1440 -ResY=1200 -unattended -nop4 -nosound
    if ($LASTEXITCODE -ne 0) { throw 'Village preview failed.' }
} elseif ($Action -eq 'NetworkTest') {
    & (Join-Path $PSScriptRoot 'NetworkTest.ps1') -EngineRoot $EngineRoot -LaunchArguments $launchArgs
} elseif ($Action -eq 'SwampSmoke') {
    $swampLog = Join-Path $projectRoot ('Saved\Logs\swamp-' + [guid]::NewGuid().ToString('N') + '.log')
    & $cmdEditor @launchArgs -server -unattended -nop4 -nosplash -nullrhi -nosound "-abslog=$swampLog"
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $swampLog -SimpleMatch 'DREAD_SWAMP_COMPLETE' -Quiet)) { throw "Swamp simulation did not finish: $swampLog" }
    Write-Output "Swamp simulation completed. Outcome and capture path: $swampLog"
} elseif ($Action -eq 'SmugglerSmoke') {
    $factionLog = Join-Path $projectRoot ('Saved\Logs\smuggler-soak-' + [guid]::NewGuid().ToString('N') + '.log')
    & $cmdEditor @launchArgs -server -unattended -nop4 -nosplash -nullrhi -nosound -DMSmugglerSoak "-abslog=$factionLog"
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $factionLog -SimpleMatch 'DREAD_SMUGGLER_SOAK_COMPLETE' -Quiet)) { throw "Smuggler simulation did not finish: $factionLog" }
    Write-Output "Smuggler simulation completed. Outcome and capture path: $factionLog"
} elseif ($Action -eq 'ShellSmoke') {
    # Drives the front end headlessly: main menu -> lobby -> Launch Expedition -> streamed
    # mission -> case report, using the fast combat smoke roster to reach an outcome.
    $shellLog = Join-Path $projectRoot ('Saved\Logs\shell-' + [guid]::NewGuid().ToString('N') + '.log')
    # -RenderOffscreen additionally draws the screens and writes them to Saved/Screenshots.
    $shellRender = if ($RenderOffscreen) { @('-game', '-RenderOffscreen', '-DMShellShot', '-ResX=1440', '-ResY=900') } else { @('-server', '-nullrhi') }
    & $cmdEditor @launchArgs -unattended -nop4 -nosplash -nosound @shellRender -DMShellProbe "-DMCombatSmoke=$Outcome" "-abslog=$shellLog"
    foreach ($marker in @('DREAD_SHELL_PROBE_MENU', 'DREAD_SHELL_MISSION_SHOWN', 'DREAD_SHELL_PROBE_COMPLETE', 'DREAD_COMBAT_SMOKE_PASSED')) {
        if (-not (Select-String -LiteralPath $shellLog -SimpleMatch $marker -Quiet)) { throw "Shell flow marker absent ($marker): $shellLog" }
    }
    Write-Output "Shell flow passed. Log: $shellLog"
} elseif ($Action -in @('Smoke', 'CombatSmoke')) {
    $smokeLog = Join-Path $projectRoot ('Saved\Logs\smoke-' + [guid]::NewGuid().ToString('N') + '.log')
    $testFlag = if ($Action -eq 'Smoke') { '-DMSmokeTest' } else { "-DMCombatSmoke=$Outcome" }
    $marker = if ($Action -eq 'Smoke') { 'DREAD_MERIDIAN_SMOKE_PASSED' } else { 'DREAD_COMBAT_SMOKE_PASSED' }
    & $cmdEditor @launchArgs -server -unattended -nop4 -nosplash -nullrhi -nosound $testFlag "-abslog=$smokeLog"
    if ($LASTEXITCODE -ne 0) { throw "Smoke process failed: $LASTEXITCODE" }
    if (-not (Select-String -LiteralPath $smokeLog -SimpleMatch $marker -Quiet)) {
        throw "Smoke completion marker absent: $smokeLog"
    }
    Write-Output "Smoke passed. Capture path is in $smokeLog. Use Play Trace validate-telemetry for combat; tools/validate_capture.py for the foundation."
} else {
    if ($Action -in @('Play', 'Shell')) { $launchArgs += @('-game', '-windowed', '-ResX=1280', '-ResY=800') }
    # This visible editor is the explicitly selected interactive action.
    & (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe') @launchArgs
}
