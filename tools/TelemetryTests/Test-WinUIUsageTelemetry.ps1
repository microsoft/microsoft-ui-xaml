# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$RuntimeDirectory,
    [Parameter(Mandatory)]
    [string]$ExpectedXamlDllPath,
    [string]$OutputDirectory = (Join-Path ([IO.Path]::GetTempPath()) "WinUIUsageTelemetry-$([guid]::NewGuid().ToString('N'))")
)

$ErrorActionPreference = 'Stop'
$RuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$expectedHash = (Get-FileHash -LiteralPath $ExpectedXamlDllPath -Algorithm SHA256).Hash
if ((Get-FileHash -LiteralPath "$RuntimeDirectory\Microsoft.UI.Xaml.dll" -Algorithm SHA256).Hash -ne $expectedHash) {
    throw 'The test app layout does not contain the expected locally built Microsoft.UI.Xaml.dll.'
}
$expectedDirectory = Split-Path -Parent (Resolve-Path -LiteralPath $ExpectedXamlDllPath).Path
$controlsHash = (Get-FileHash -LiteralPath "$expectedDirectory\Microsoft.UI.Xaml.Controls.dll" -Algorithm SHA256).Hash
$tabularHash = (Get-FileHash -LiteralPath "$expectedDirectory\Microsoft.UI.Xaml.Controls.Tabular.dll" -Algorithm SHA256).Hash
if ((Get-FileHash -LiteralPath "$RuntimeDirectory\Microsoft.UI.Xaml.Controls.dll" -Algorithm SHA256).Hash -ne $controlsHash -or
    (Get-FileHash -LiteralPath "$RuntimeDirectory\Microsoft.UI.Xaml.Controls.Tabular.dll" -Algorithm SHA256).Hash -ne $tabularHash) {
    throw 'The test app layout does not contain the controls binaries from the expected local build.'
}
$catalogPath = Join-Path $PSScriptRoot 'ProfilerControls.csv'
$catalog = @(Import-Csv -LiteralPath $catalogPath)
$mt = (Get-Command mt.exe -ErrorAction Stop).Source
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$hostDirectory = Join-Path $OutputDirectory 'host'
$manifest = Join-Path $OutputDirectory 'app.manifest'
& $mt -nologo "-inputresource:$RuntimeDirectory\MUXControlsTestApp.exe;#1" "-out:$manifest"
if ($LASTEXITCODE -ne 0) { throw 'Could not extract test app activation registrations.' }
& dotnet build "$PSScriptRoot\WinUIUsageTelemetryProbe.csproj" --verbosity quiet --output $hostDirectory `
    "-p:RuntimeDirectory=$RuntimeDirectory" "-p:ApplicationManifest=$manifest" `
    "-p:BaseIntermediateOutputPath=$OutputDirectory\obj\"
if ($LASTEXITCODE -ne 0) { throw 'Telemetry probe build failed.' }

$sessionName = "WinUIUsageTelemetry-$([guid]::NewGuid().ToString('N'))"
$providers = Join-Path $OutputDirectory 'providers.txt'
@(
    '{2dc72f6e-e4d1-5f58-3245-09a4243799dd} 0x400000000000 5'
    '{21e0ae07-56a7-55b5-12f9-011e6bc08cca} 0x400000000000 5'
    '{80a0b321-304c-576e-c0f8-c593c3e7cf86} 0x400000000000 5'
) | Set-Content -LiteralPath $providers -Encoding ascii
$etl = Join-Path $OutputDirectory 'events.etl'
$xmlPath = Join-Path $OutputDirectory 'events.xml'
$probeName = 'WinUIUsageTelemetryProbe'
$deployed = @()
$sessionStarted = $false
$cases = @()
try {
    foreach ($extension in @('exe', 'dll', 'deps.json', 'runtimeconfig.json')) {
        if (Test-Path -LiteralPath "$RuntimeDirectory\$probeName.$extension") {
            throw "Refusing to overwrite an existing probe: $RuntimeDirectory\$probeName.$extension"
        }
    }
    foreach ($extension in @('exe', 'dll', 'deps.json', 'runtimeconfig.json')) {
        $source = "$hostDirectory\$probeName.$extension"
        if ($extension -in @('deps.json', 'runtimeconfig.json')) {
            # Reuse the test app's self-contained .NET runtime and native dependency layout.
            $source = "$RuntimeDirectory\MUXControlsTestApp.$extension"
        }
        $destination = "$RuntimeDirectory\$probeName.$extension"
        Copy-Item -LiteralPath $source -Destination $destination
        $deployed += $destination
    }
    & logman create trace $sessionName -pf $providers -o $etl -ets
    if ($LASTEXITCODE -ne 0) { throw 'ETW capture failed to start. Run with ETW session permissions.' }
    $sessionStarted = $true
    foreach ($name in @('load-only', 'launch-1', 'launch-2', 'control-usage')) {
        $stdout = Join-Path $OutputDirectory "$name.stdout.txt"
        $stderr = Join-Path $OutputDirectory "$name.stderr.txt"
        $options = @{
            FilePath = "$RuntimeDirectory\$probeName.exe"
            WorkingDirectory = $RuntimeDirectory
            RedirectStandardOutput = $stdout
            RedirectStandardError = $stderr
            PassThru = $true
        }
        if ($name -eq 'load-only') { $options.ArgumentList = '--load-only' }
        if ($name -eq 'control-usage') { $options.ArgumentList = "--controls `"$catalogPath`"" }
        $process = Start-Process @options
        if (-not $process.WaitForExit(60000)) {
            Stop-Process -Id $process.Id
            throw "$name timed out. See $stdout and $stderr."
        }
        if ($process.ExitCode -ne 0) { throw "$name failed with exit code $($process.ExitCode). See $stderr." }
        $log = Get-Content -LiteralPath $stdout -Raw
        if ($log -notmatch [regex]::Escape("MODULE $RuntimeDirectory\Microsoft.UI.Xaml.dll $expectedHash")) {
            throw "$name did not load the expected local XAML binary."
        }
        if ($name -eq 'control-usage') {
            if ($log -notmatch [regex]::Escape("MODULE $RuntimeDirectory\Microsoft.UI.Xaml.Controls.dll $controlsHash") -or
                $log -notmatch [regex]::Escape("MODULE $RuntimeDirectory\Microsoft.UI.Xaml.Controls.Tabular.dll $tabularHash") -or
                $log -notmatch "PASS: constructed $($catalog.Count) catalog entries twice") {
                throw 'The per-control workload or binary identity check failed.'
            }
        }
        elseif ($name -ne 'load-only') {
            if ($log -notmatch [regex]::Escape("MODULE $RuntimeDirectory\Microsoft.UI.Xaml.Controls.dll $controlsHash") -or
                @([regex]::Matches($log, 'PASS: nested XAML managers on UI thread')).Count -ne 4 -or
                $log -notmatch 'PASS: three windows, four additional UI threads') {
                throw "$name did not complete the required multi-window/multi-thread workload."
            }
        }
        $cases += [pscustomobject]@{ Name = $name; ProcessId = $process.Id }
    }
}
finally {
    if ($sessionStarted) {
        & logman stop $sessionName -ets
        if ($LASTEXITCODE -ne 0) { Write-Error "Could not stop ETW session $sessionName." -ErrorAction Continue }
    }
    foreach ($file in $deployed) { Remove-Item -LiteralPath $file }
}
& tracerpt $etl -o $xmlPath -of XML -y
if ($LASTEXITCODE -ne 0) { throw 'ETW decoding failed.' }
[xml]$trace = Get-Content -LiteralPath $xmlPath -Raw
$events = @($trace.Events.Event)
$lossFields = @($events.EventData.Data | Where-Object { $_.Name -in @('EventsLost', 'BuffersLost') })
if ($lossFields.Count -ne 2 -or @($lossFields | Where-Object { [long]$_.'#text' -ne 0 }).Count -ne 0) {
    throw 'Trace loss metadata is missing or events/buffers were lost.'
}
foreach ($case in $cases) {
    $processEvents = @($events | Where-Object { [int]$_.System.Execution.ProcessID -eq $case.ProcessId })
    $startup = @($processEvents | Where-Object {
        $_.System.Provider.Name -eq 'Microsoft.UI.Xaml' -and $_.RenderingInfo.Task -eq 'WinUIAppStartup'
    })
    $expectedCount = if ($case.Name -eq 'load-only') { 0 } else { 1 }
    if ($startup.Count -ne $expectedCount) { throw "$($case.Name): expected $expectedCount startup events; got $($startup.Count)." }
    if ($expectedCount -eq 0) { continue }
    $fields = @{}
    foreach ($field in $startup[0].EventData.Data) { $fields[$field.Name] = $field.'#text' }
    if ($fields.Count -ne 2 -or [string]::IsNullOrWhiteSpace($fields.BinaryVersion) -or
        [long]$fields.PartA_PrivTags -ne 16777216 -or
        $startup[0].System.Keywords -ne '0x400000000000' -or [int]$startup[0].System.Level -ne 4) {
        throw "$($case.Name): incorrect startup payload, privacy tag, keyword, or level."
    }
    foreach ($provider in @('Microsoft.UI.Xaml', 'Microsoft.UI.Xaml.Controls')) {
        $usage = @($processEvents | Where-Object {
            $_.System.Provider.Name -eq $provider -and $_.RenderingInfo.Task -eq 'RuntimeProfiler'
        })
        $counts = ($usage.EventData.Data | Where-Object Name -eq 'ApiCounts' | ForEach-Object { $_.'#text' }) -join ','
        if ($counts -notmatch ':\d*[1-9]\d*') { throw "$($case.Name): missing nonzero $provider control usage." }
        if ($provider -eq 'Microsoft.UI.Xaml.Controls' -and $case.Name -ne 'control-usage') {
            # Stable profiler IDs: NavigationView = 1, TabView = 27.
            foreach ($typeId in @(1, 27)) {
                $count = [regex]::Match($counts, "\[$typeId\|9999\]:(\d+)")
                if (-not $count.Success -or [int]$count.Groups[1].Value -lt 3) {
                    throw "$($case.Name): missing NavigationView or TabView constructor counts."
                }
            }
        }
    }
}
$controlProcessId = ($cases | Where-Object Name -eq 'control-usage').ProcessId
$controlEvents = @($events | Where-Object {
    [int]$_.System.Execution.ProcessID -eq $controlProcessId -and $_.RenderingInfo.Task -eq 'RuntimeProfiler'
})
$controlResults = @()
foreach ($providerGroup in $catalog | Group-Object Provider) {
    $usageEvents = @($controlEvents | Where-Object { $_.System.Provider.Name -eq $providerGroup.Name })
    if ($providerGroup.Name -ne 'Microsoft.UI.Xaml' -and $usageEvents.Count -ne 2) {
        throw "$($providerGroup.Name): expected two nonempty flushes and no duplicate empty-flush events; got $($usageEvents.Count)."
    }
    $batches = @()
    foreach ($usageEvent in $usageEvents) {
        $fields = @{}
        foreach ($field in $usageEvent.EventData.Data) { $fields[$field.Name] = $field.'#text' }
        if ([string]::IsNullOrWhiteSpace($fields.BinaryVersion) -or [int]$fields.ProfileGroupId -ne 0 -or
            [int]$fields.OnSuspend -ne 1 -or [int]$fields.Overflow -ne 0 -or
            [long]$fields.PartA_PrivTags -ne 16777216 -or $usageEvent.System.Keywords -ne '0x400000000000' -or
            ($providerGroup.Name -ne 'Microsoft.UI.Xaml' -and [int]$fields.StringOverflow -ne 0)) {
            throw "$($providerGroup.Name): invalid RuntimeProfiler metadata or counter overflow."
        }
        $counts = @{}
        foreach ($entry in [regex]::Matches($fields.ApiCounts, '\[(\d+)\|9999\]:(\d+)')) {
            $id = $entry.Groups[1].Value
            $counts[$id] = [long]$counts[$id] + [long]$entry.Groups[2].Value
        }
        $batches += ,$counts
    }
    foreach ($typeGroup in $providerGroup.Group | Group-Object TypeId) {
        $observed = 0L
        foreach ($batch in $batches) {
            $hits = [long]$batch[$typeGroup.Name]
            $observed += $hits
            if ($providerGroup.Name -ne 'Microsoft.UI.Xaml' -and $hits -lt $typeGroup.Count) {
                throw "$($providerGroup.Name): profiler ID $($typeGroup.Name) is missing from a construction batch."
            }
        }
        $expected = 2 * $typeGroup.Count
        if ($observed -lt $expected) {
            throw "$($providerGroup.Name): profiler ID $($typeGroup.Name) expected at least $expected hits; got $observed."
        }
        $controlResults += [pscustomobject]@{
            Provider = $providerGroup.Name
            TypeId = [int]$typeGroup.Name
            Classes = $typeGroup.Group.TypeName -join ', '
            MinimumExpected = $expected
            Observed = $observed
        }
    }
}
$controlResults | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'control-results.csv') -NoTypeInformation
$cases | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'processes.json') -Encoding utf8
Write-Output "PASS: startup boundaries, $($catalog.Count) control scenarios / $($controlResults.Count) constructor counters, repeated construction and empty-flush reset. Evidence: $OutputDirectory"
