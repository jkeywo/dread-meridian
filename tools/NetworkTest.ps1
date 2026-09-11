[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot, [Parameter(Mandatory)][string[]]$LaunchArguments)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$runDirectory = Join-Path $projectRoot ('Saved\NetworkTests\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
# Reserve an available loopback UDP port; release immediately before Unreal binds it.
$portProbe = [Net.Sockets.UdpClient]::new([Net.IPEndPoint]::new([Net.IPAddress]::Loopback, 0))
$port = $portProbe.Client.LocalEndPoint.Port
$portProbe.Dispose()
$processes = [Collections.Generic.List[Diagnostics.Process]]::new()
function Start-TestProcess([string[]]$Arguments) {
    # Start-Process joins ArgumentList, so quote each native argument explicitly.
    $quoted = ($Arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }) -join ' '
    $process = Start-Process -FilePath $editor -ArgumentList $quoted -PassThru -WindowStyle Hidden
    $processes.Add($process)
    return $process
}
try {
    $serverLog = Join-Path $runDirectory 'server.log'
    $server = Start-TestProcess ($LaunchArguments + @('-server', '-nullrhi', '-nosound', '-unattended', '-nop4', '-DMNetworkTest', '-multihome=127.0.0.1', "-port=$port", "-abslog=$serverLog"))
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Network server exited before listening.' }
        if ([DateTime]::UtcNow -gt $deadline) { throw 'Server startup timed out.' }
        Start-Sleep -Milliseconds 200
    } until ((Test-Path $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'listening on port' -Quiet))

    $clientBase = @($LaunchArguments[0], "127.0.0.1:$port", '-game', '-nullrhi', '-nosound', '-unattended', '-nop4')
    $dropLog = Join-Path $runDirectory 'disconnect.log'
    $drop = Start-TestProcess ($clientBase + @('-DMDisconnectProbe', "-abslog=$dropLog"))
    if (-not $drop.WaitForExit(60000)) { throw 'Disconnect probe timed out.' }
    if (-not (Select-String -LiteralPath $dropLog -SimpleMatch 'DREAD_DISCONNECT_PROBE_COMPLETE' -Quiet)) { throw 'Disconnect probe did not possess an investigator.' }

    $clients = @()
    foreach ($index in 1..2) {
        $log = Join-Path $runDirectory "client-$index.log"
        $clients += Start-TestProcess ($clientBase + @('-DMNetworkProbe', "-abslog=$log"))
    }
    foreach ($client in $clients) {
        if (-not $client.WaitForExit(60000)) { throw 'Replication probe timed out.' }
        if ($client.ExitCode -ne 0) { throw "Replication probe failed: $($client.ExitCode)" }
    }
    foreach ($index in 1..2) {
        if (-not (Select-String -LiteralPath (Join-Path $runDirectory "client-$index.log") -SimpleMatch 'DREAD_NETWORK_PROBE_PASSED' -Quiet)) { throw "Client $index did not validate final replicated state." }
    }
    if (-not $server.WaitForExit(15000)) { throw 'Server did not complete network test.' }
    $captureLine = (Select-String -LiteralPath $serverLog -Pattern 'Display: Capture: (.+events.jsonl)').Matches
    if (-not $captureLine) { throw 'Network capture missing.' }
    # The writer logs paths relative to the engine executable. Resolve through the GUID instead.
    $runId = [regex]::Match($captureLine[0].Groups[1].Value, 'Playtrace[/\\]([^/\\]+)').Groups[1].Value
    $capture = Join-Path $projectRoot "Saved\Playtrace\$runId\events.jsonl"
    $events = @(Get-Content -LiteralPath $capture | ForEach-Object { $_ | ConvertFrom-Json })
    $handoffs = @($events | Where-Object { $_.event_type -eq 'control.changed' -and $_.data.control -eq 'bot' })
    if ($handoffs.Count -lt 1) { throw 'Disconnect did not hand the preserved investigator back to a bot.' }
    Write-Output "Network test passed: disconnect takeover and two client state checks. Reports: $runDirectory"
} finally {
    foreach ($process in $processes) {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}
