# Shared build, capture, evidence-integrity, and comparison helpers.
#Requires -Version 7
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-JsonFile([string]$Path) {
    Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
}

function Write-JsonFile([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 60), [Text.UTF8Encoding]::new($false))
}

function Get-Sha256([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-FileHash([string]$Path, [string]$Expected) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Sha256 $Path) -ne $Expected) {
        throw "Evidence or input changed: $Path"
    }
}

function Get-RepositoryFiles([string]$RepositoryPath) {
    $files = @(git -C $RepositoryPath -c core.quotepath=false ls-files --cached --others --exclude-standard)
    if ($LASTEXITCODE -ne 0 -or $files.Count -eq 0) { throw 'An indexed Git source repository is required.' }
    $files
}

function Get-SourceHash([string]$RepositoryPath, [string[]]$Files = @()) {
    if ($Files.Count -eq 0) { $Files = @(Get-RepositoryFiles $RepositoryPath) }
    $root = [IO.Path]::GetFullPath($RepositoryPath).TrimEnd('\') + '\'
    $hash = [Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
    try {
        foreach ($relative in ($Files | Sort-Object -CaseSensitive)) {
            $path = [IO.Path]::GetFullPath((Join-Path $root $relative.Replace('/', '\')))
            if (!$path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw 'Source path escapes the repository.' }
            if ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Source links require a separate adapter: $relative"
            }
            $hash.AppendData([Text.Encoding]::UTF8.GetBytes("$relative`0$(Get-Sha256 $path)`n"))
        }
        [Convert]::ToHexString($hash.GetHashAndReset()).ToLowerInvariant()
    } finally { $hash.Dispose() }
}

function Get-ProfilerHash([string]$ProfilerCli) {
    $root = (Split-Path $ProfilerCli -Parent).TrimEnd('\') + '\'
    $files = @([IO.Directory]::EnumerateFiles($root, '*', [IO.SearchOption]::AllDirectories) |
        ForEach-Object { [IO.Path]::GetRelativePath($root, $_).Replace('\', '/') })
    Get-SourceHash $root $files
}

function Copy-ApplicationSource([string]$RepositoryPath, [string]$Destination, [string[]]$Files) {
    if (Test-Path -LiteralPath $Destination) { throw "Source-copy destination already exists: $Destination" }
    foreach ($relative in $Files) {
        $source = Join-Path $RepositoryPath $relative.Replace('/', '\')
        $target = Join-Path $Destination $relative.Replace('/', '\')
        New-Item -ItemType Directory -Force (Split-Path $target -Parent) | Out-Null
        Copy-Item -LiteralPath $source -Destination $target
    }
}

function New-LoggedProcess([string]$Executable, [string[]]$Arguments, [string]$Directory) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $Executable
    $info.WorkingDirectory = $Directory
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.RedirectStandardInput = $true
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    if (!$process.Start()) { throw "Could not start $Executable" }
    $process
}

function Invoke-LoggedProcess([string]$Executable, [string[]]$Arguments, [string]$Directory,
    [string]$LogPath, [int]$TimeoutSeconds = 900) {
    $process = New-LoggedProcess $Executable $Arguments $Directory
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.Close()
    try {
        if (!$process.WaitForExit($TimeoutSeconds * 1000)) {
            Stop-Process -Id $process.Id -ErrorAction SilentlyContinue
            throw "Timed out: $Executable. See $LogPath"
        }
        $text = $stdout.GetAwaiter().GetResult()
        $errors = $stderr.GetAwaiter().GetResult()
        [IO.File]::WriteAllText($LogPath, "$text`n$errors")
        if ($process.ExitCode -ne 0) { throw "Exit $($process.ExitCode): $Executable. See $LogPath`n$errors" }
        $text
    } finally { $process.Dispose() }
}

function Build-Application($Context, [string]$RepositoryPath, [string]$OutputDirectory) {
    New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
    $project = Join-Path $RepositoryPath $Context.scenario.build.project
    $arguments = @('publish', $project, '--nologo', '-v:minimal',
        '-c', $Context.scenario.build.configuration, '-r', 'win-x64',
        '-p:Platform=x64', '-p:UseStandalone=true', '-p:SelfContained=true',
        '-p:WindowsAppSdkSelfContained=true', '-p:PublishSingleFile=false',
        '-p:PublishTrimmed=false', '-p:PublishReadyToRun=false',
        '-p:AppxPackageSigningEnabled=false', '-o', $OutputDirectory)
    $text = Invoke-LoggedProcess $Context.dotnet $arguments $RepositoryPath (Join-Path $OutputDirectory 'build.log')
    Write-Host $text
    $propertyArguments = @('msbuild', $project, '--nologo', '-getProperty:TargetDir',
        "-p:Configuration=$($Context.scenario.build.configuration)", '-p:RuntimeIdentifier=win-x64') +
        @($arguments | Where-Object { $_.StartsWith('-p:') })
    $directoryLog = Join-Path $OutputDirectory 'target-directory.log'
    $buildDirectory = (Invoke-LoggedProcess $Context.dotnet $propertyArguments $RepositoryPath $directoryLog).Trim()
    $root = [IO.Path]::GetFullPath($RepositoryPath).TrimEnd('\') + '\'
    if (![IO.Path]::GetFullPath($buildDirectory).StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The application build output must remain inside its isolated source copy.'
    }
    # Unpackaged publishing can omit the application's PRI and compiled XAML.
    Copy-Deployment $buildDirectory $OutputDirectory
    $executable = Join-Path $OutputDirectory $Context.scenario.build.executable
    if (!(Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Build did not produce $executable" }
    $executable
}

function Copy-Deployment([string]$BuildDirectory, [string]$OutputDirectory) {
    $source = [IO.Path]::GetFullPath($BuildDirectory).TrimEnd('\') + '\'
    $destination = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\') + '\'
    if ($destination.StartsWith($source, [StringComparison]::OrdinalIgnoreCase) -or
        $source.StartsWith($destination, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Deployment source and destination must not overlap.'
    }
    foreach ($file in [IO.Directory]::EnumerateFiles($source, '*', [IO.SearchOption]::AllDirectories)) {
        $target = Join-Path $destination ([IO.Path]::GetRelativePath($source, $file))
        New-Item -ItemType Directory -Force (Split-Path $target -Parent) | Out-Null
        Copy-Item -LiteralPath $file -Destination $target -Force
    }
}

function Assert-InteractiveDesktop {
    if (![Environment]::UserInteractive) { throw 'UI measurement requires an interactive desktop.' }
    if (!('ProfilerDesktop' -as [type])) {
        Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ProfilerDesktop {
    [DllImport("user32.dll")] public static extern IntPtr OpenInputDesktop(uint flags, bool inherit, uint access);
    [DllImport("user32.dll")] public static extern bool CloseDesktop(IntPtr desktop);
}
'@
    }
    $desktop = [ProfilerDesktop]::OpenInputDesktop(0, $false, 1)
    if ($desktop -eq [IntPtr]::Zero) { throw 'Unlock the interactive desktop before measuring UI actions.' }
    [void][ProfilerDesktop]::CloseDesktop($desktop)
}

function Assert-MinimalCandidate([string]$RepositoryPath, [string[]]$ProtectedPaths = @()) {
    $changes = @(git -C $RepositoryPath -c core.quotepath=false diff --name-status ade-baseline)
    if ($LASTEXITCODE -ne 0) { throw 'Candidate is missing its ADE source baseline.' }
    $newFiles = @(git -C $RepositoryPath ls-files --others --exclude-standard)
    if ($LASTEXITCODE -ne 0 -or $newFiles.Count -gt 0 -or $changes.Count -eq 0 -or $changes.Count -gt 2 -or
        @($changes | Where-Object { $_ -notmatch '^M\t.+\.(cs|xaml)$' }).Count -gt 0) {
        throw 'The candidate must modify only one or two existing C# or XAML files.'
    }
    foreach ($change in $changes) {
        $relative = ($change -split "`t", 2)[1].Replace('\', '/')
        foreach ($protected in $ProtectedPaths) {
            $prefix = $protected.Replace('\', '/').TrimEnd('/')
            if ($relative.Equals($prefix, [StringComparison]::OrdinalIgnoreCase) -or
                $relative.StartsWith("$prefix/", [StringComparison]::OrdinalIgnoreCase)) {
                throw "Protected candidate path: $relative"
            }
        }
    }
}

function Assert-ScopedCandidate([string]$RepositoryPath, [string[]]$AllowedPaths) {
    if (!$AllowedPaths -or @($AllowedPaths | Select-Object -Unique).Count -ne $AllowedPaths.Count) {
        throw 'A unique, explicitly approved candidate file scope is required.'
    }
    $allowed = @($AllowedPaths | ForEach-Object {
        $path = $_.Replace('\', '/')
        if ([IO.Path]::IsPathRooted($_) -or $path -match '(^|/)\.\.(/|$)|:|[*?]' -or
            $path -notmatch '\.(cs|xaml)$') { throw 'Invalid approved source-file scope.' }
        $path
    })
    $changes = @(git -C $RepositoryPath -c core.quotepath=false diff --name-status ade-baseline)
    if ($LASTEXITCODE -ne 0) { throw 'Candidate is missing its ADE source baseline.' }
    $newFiles = @(git -C $RepositoryPath ls-files --others --exclude-standard)
    if ($LASTEXITCODE -ne 0 -or $newFiles.Count -gt 0 -or !$changes) {
        throw 'The candidate must modify explicitly approved existing files; no additions or empty fixes.'
    }
    foreach ($change in $changes) {
        if ($change -notmatch '^M\t(.+)$' -or $Matches[1].Replace('\', '/') -notin $allowed) {
            throw "Candidate change is outside its approved file scope: $change"
        }
    }
}

function Assert-TrialContext($Context) {
    if ($Context.schema -ne 'ui-profiler/trial/v1' -or $Context.strategy -ne 'independent') {
        throw 'A supported independent recommendation trial is required.'
    }
    Assert-FileHash $Context.request.path $Context.request.sha256
    Assert-FileHash $Context.contracts.path $Context.contracts.sha256
    Assert-FileHash $Context.recommendations.path $Context.recommendations.sha256
    Assert-FileHash $Context.baseline.path $Context.baseline.sha256
    Assert-FileHash $Context.module.path $Context.module.sha256
    Assert-FileHash $Context.nativeMcp.path $Context.nativeMcp.sha256
    $request = Read-JsonFile $Context.request.path
    if (!$request.allowMeasurements -or $request.strategy -ne 'independent' -or
        $request.candidatePolicy -ne 'one-per-approved-recommendation') {
        throw 'The original request did not authorize independent recommendation measurements.'
    }
    if ((Get-ProfilerHash $Context.nativeMcp.path) -ne $Context.nativeMcp.deploymentSha256 -or
        (Get-SourceHash $Context.nativeMcp.source $Context.nativeMcp.files) -ne $Context.nativeMcp.sourceSha256) {
        throw 'The shared frozen native MCP implementation changed.'
    }
    $report = Read-JsonFile $Context.recommendations.path
    $entry = @($report.recommendations | Where-Object id -eq $Context.recommendationId)
    if ($entry.Count -ne 1 -or $entry[0].verdict -ne 'propose' -or
        (ConvertTo-Json @($entry[0].files | ForEach-Object path) -Compress) -cne
        (ConvertTo-Json @($Context.allowedPaths) -Compress)) {
        throw 'The trial does not match its frozen recommendation and approved file scope.'
    }
    Assert-FrozenScenario $Context
    Assert-OriginalSource $Context
}

function Assert-TrialReference($Context, $Reference) {
    if ($Reference.id -cnotmatch '^[a-z0-9][a-z0-9-]{0,79}$') { throw 'Invalid independent recommendation id.' }
    $known = @($Context.trials | Where-Object id -CEQ $Reference.id)
    if ($known.Count -ne 1) { throw 'The investigation must contain one reference per prepared trial.' }
    $directory = Join-Path (Join-Path $Context.directory 'trials') $Reference.id
    $paths = @(
        @($Reference.directory, $directory),
        @($Reference.context.path, (Join-Path $directory 'pack\knowledge\trial-context.json')),
        @($Reference.proposal.path, (Join-Path $directory 'ade-run.json'))
    )
    foreach ($pair in $paths) {
        if ([IO.Path]::GetFullPath($pair[0]) -ne [IO.Path]::GetFullPath($pair[1])) {
            throw 'The trial reference is outside its recommendation directory.'
        }
    }
    Assert-FileHash $Reference.context.path $Reference.context.sha256
    Assert-FileHash $Reference.proposal.path $Reference.proposal.sha256
    $trial = Read-JsonFile $Reference.context.path
    $proposal = Read-JsonFile $Reference.proposal.path
    if ($trial.schema -ne 'ui-profiler/trial/v1' -or $trial.strategy -ne 'independent' -or
        $trial.recommendationId -cne $Reference.id -or
        [IO.Path]::GetFullPath($trial.directory) -ne [IO.Path]::GetFullPath($directory) -or
        [IO.Path]::GetFullPath($trial.investigationDirectory) -ne [IO.Path]::GetFullPath($Context.directory) -or
        $trial.originalRepository -ne $Context.originalRepository -or
        $trial.originalRevision -ne $Context.originalRevision -or
        $trial.originalSourceSha256 -ne $Context.originalSourceSha256) {
        throw 'The trial context is not bound to this recommendation and original source.'
    }
    foreach ($name in @('request', 'contracts', 'recommendations', 'baseline')) {
        if ($trial.$name.path -ne $Context.$name.path -or $trial.$name.sha256 -ne $Context.$name.sha256) {
            throw "The trial uses different shared inputs: $name"
        }
    }
    if ($proposal.status -ne 'AwaitingApproval' -or $proposal.runId -cne $Reference.runId -or
        $Reference.proposalSha256 -cnotmatch '^[a-f0-9]{64}$' -or
        $proposal.proposalSha256 -cne $Reference.proposalSha256 -or
        [string]::IsNullOrWhiteSpace($Reference.runId) -or
        $Reference.runId -match '[/\\:]' -or $Reference.runId -in @('.', '..')) {
        throw 'The trial ADE run does not match its frozen proposal.'
    }
    $trial
}

function Assert-ComparisonBaseline($Context, $Comparison) {
    if ($Comparison.schema -ne 'ui-profiler/comparison/v2' -or $Comparison.baseline.sha256 -ne $Context.baseline.sha256) {
        throw 'The trial comparison does not use the shared original baseline.'
    }
    foreach ($side in @('baseline', 'candidate')) {
        Assert-FileHash $Comparison.$side.path $Comparison.$side.sha256
        Assert-Measurement (Read-JsonFile $Comparison.$side.path)
    }
}

function Write-RecommendationReport($Context) {
    Assert-FileHash $Context.recommendations.path $Context.recommendations.sha256
    $recommendations = Read-JsonFile $Context.recommendations.path
    $rows = @(
        foreach ($entry in $recommendations.recommendations) {
            $trial = @($Context.trials | Where-Object id -eq $entry.id) | Select-Object -First 1
            $state = if ($entry.verdict -eq 'propose') { 'pending' } else { 'insufficient-evidence' }
            $outcome = $null
            $outcomeReference = $null
            $summaryMetrics = @()
            if ($trial) {
                [void](Assert-TrialReference $Context $trial)
                $state = 'awaiting-approval'
                $outcomePath = Join-Path $trial.directory 'outcome.json'
                if (Test-Path -LiteralPath $outcomePath -PathType Leaf) {
                    $outcome = Read-JsonFile $outcomePath
                    $outcomeReference = @{ path = $outcomePath; sha256 = Get-Sha256 $outcomePath }
                    if ($outcome.schema -ne 'ui-profiler/trial-outcome/v1' -or $outcome.recommendationId -ne $entry.id -or
                        $outcome.baselineSha256 -ne $Context.baseline.sha256 -or
                        $outcome.contextSha256 -ne $trial.context.sha256 -or
                        $outcome.proposalSha256 -ne $trial.proposalSha256 -or
                        $outcome.adeRun -cne $trial.runId -or
                        $outcome.state -notin @('verified-comparison', 'failed', 'blocked')) {
                        throw 'The trial outcome is not bound to this recommendation and shared baseline.'
                    }
                    if ($outcome.state -eq 'verified-comparison') {
                        if (!$outcome.adeChecksPassed -or !$outcome.chainVerified -or !$outcome.originalRepositoryUnchanged) {
                            throw 'A trial cannot report success without all required checks.'
                        }
                        Assert-FileHash $outcome.comparison.path $outcome.comparison.sha256
                        $comparison = Read-JsonFile $outcome.comparison.path
                        Assert-ComparisonBaseline $Context $comparison
                        $summaryMetrics = @($comparison.metrics | Where-Object primary)
                    }
                    $state = $outcome.state
                }
            }
            @{
                id = $entry.id; title = $entry.title; state = $state; verdict = $entry.verdict
                summary = $entry.summary; files = @($entry.files | ForEach-Object path)
                observations = $entry.observations; hypotheses = $entry.hypotheses; risks = $entry.risks
                trial = $trial; outcome = $outcome; outcomeReference = $outcomeReference
                summaryMetrics = $summaryMetrics
            }
        }
    )
    $report = @{
        schema = 'ui-profiler/recommendation-report/v1'; strategy = 'independent'
        baseline = $Context.baseline; sourceRevision = $Context.originalRevision
        recommendations = $Context.recommendations; rows = $rows
        summary = $recommendations.summary; risks = $recommendations.risks
        interpretation = 'Each candidate starts from original source and is compared with the same frozen baseline. Results are observational; variants are not combined.'
    }
    $path = Join-Path $Context.directory 'recommendations-report.json'
    Write-JsonFile $path $report
    $markdown = "# Independent recommendation trials`n`n$($recommendations.summary)`n`n$($report.interpretation)`n`n"
    $markdown += "Baseline: $($Context.baseline.path) (SHA-256 $($Context.baseline.sha256)).`n`n"
    $markdown += "| Recommendation | Status | Scoped files | Comparison |`n|---|---|---:|---|`n"
    foreach ($row in $rows) {
        $comparison = if ($row.outcome -and $row.outcome.state -eq 'verified-comparison') {
            "$($row.outcome.comparison.path) - $($row.outcome.counts | ConvertTo-Json -Compress)"
        } else { 'Not measured; no improvement claimed.' }
        $markdown += "| $($row.id): $($row.title.Replace('|', '\|')) | $($row.state) | $($row.files.Count) | $comparison |`n"
    }
    foreach ($row in $rows) {
        $markdown += "`n## $($row.id): $($row.title)`n`n$($row.summary)`n`n"
        if ($row.trial) { $markdown += "ADE run: $($row.trial.runId). Proposal SHA-256: $($row.trial.proposalSha256).`n`n" }
        if ($row.summaryMetrics.Count) {
            $markdown += "| Family / scope / metric | Unit | Original baseline | Candidate | Delta | Status / change |`n|---|---|---:|---:|---:|---|`n"
            foreach ($metric in $row.summaryMetrics) {
                $values = @('baseline', 'candidate', 'delta') | ForEach-Object {
                    if ($null -eq $metric.$_) { 'unavailable' } else { [string]$metric.$_ }
                }
                $label = "$($metric.family) / $($metric.scope) / $($metric.metric)".Replace('|', '\|')
                $markdown += "| $label | $($metric.unit) | $($values[0]) | $($values[1]) | $($values[2]) | $($metric.status) / $($metric.change) |`n"
            }
            $markdown += "`nFull dimensions, CSV, provenance and unavailable states: $($row.outcome.comparison.path).`n`n"
        }
        foreach ($section in @('files', 'observations', 'hypotheses', 'risks')) {
            $markdown += "### $section`n`n"
            foreach ($item in $row[$section]) { $markdown += "- $item`n" }
        }
        if ($row.outcome -and $row.outcome.state -ne 'verified-comparison') {
            $markdown += "`nTrial stopped: $($row.outcome.error)`n"
        }
    }
    [IO.File]::WriteAllText((Join-Path $Context.directory 'recommendations-report.md'), $markdown)
    @{ path = $path; sha256 = Get-Sha256 $path; rows = $rows }
}

function Assert-RecommendationReport($Context) {
    Assert-FileHash $Context.report.path $Context.report.sha256
    $report = Read-JsonFile $Context.report.path
    if ($report.schema -ne 'ui-profiler/recommendation-report/v1' -or
        $report.strategy -ne 'independent' -or $report.baseline.sha256 -ne $Context.baseline.sha256 -or
        $report.recommendations.sha256 -ne $Context.recommendations.sha256) {
        throw 'The recommendation report is not bound to this investigation.'
    }
    foreach ($row in $report.rows) {
        if ($row.trial) {
            [void](Assert-TrialReference $Context $row.trial)
        }
        if ($row.outcomeReference) { Assert-FileHash $row.outcomeReference.path $row.outcomeReference.sha256 }
        if ($row.outcome -and $row.outcome.state -eq 'verified-comparison') {
            Assert-FileHash $row.outcome.comparison.path $row.outcome.comparison.sha256
            Assert-ComparisonBaseline $Context (Read-JsonFile $row.outcome.comparison.path)
        }
    }
    $report
}

function Assert-RunDirectory([string]$Directory, [string[]]$InputRoots) {
    if (![IO.Path]::IsPathFullyQualified($Directory)) { throw 'Use an absolute investigation directory.' }
    if ($Directory -notmatch '^[A-Za-z]:\\' -or $Directory -match '~[0-9]') {
        throw 'Use a normal local drive path, not UNC/device paths or DOS short-name aliases.'
    }
    $target = [IO.Path]::GetFullPath($Directory).TrimEnd('\') + '\'
    if ($target -eq [IO.Path]::GetPathRoot($target)) { throw 'A drive root is not an investigation directory.' }
    foreach ($inputRoot in $InputRoots) {
        if ($inputRoot -notmatch '^[A-Za-z]:\\' -or $inputRoot -match '~[0-9]') {
            throw 'Input roots must use normal local drive paths, not aliases.'
        }
        $inputPath = [IO.Path]::GetFullPath($inputRoot).TrimEnd('\') + '\'
        if ($target.StartsWith($inputPath, [StringComparison]::OrdinalIgnoreCase) -or
            $inputPath.StartsWith($target, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Investigation output must not overlap any source, expert, SDK or profiler input.'
        }
    }
    $ancestor = $target.TrimEnd('\')
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and
            ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'Investigation output may not traverse a filesystem link.'
        }
        $ancestor = Split-Path $ancestor -Parent
    }
}

function Assert-OriginalSource($Context) {
    if ((Get-SourceHash $Context.originalRepository) -ne $Context.originalSourceSha256 -or
        (git -C $Context.originalRepository rev-parse HEAD) -ne $Context.originalRevision) {
        throw 'The original application source changed; the investigation cannot report success.'
    }
}

function Assert-InvestigationState($Context, [string[]]$Allowed) {
    if ($Context.state -notin $Allowed) {
        throw "Operation requires state $($Allowed -join ', '); actual state is $($Context.state)."
    }
}

function Assert-FrozenScenario($Context) {
    if (!$Context.scenarioPath -or !$Context.scenarioSha256) { throw 'Freeze a validated scenario before measuring.' }
    Assert-FileHash $Context.scenarioPath $Context.scenarioSha256
    Assert-FileHash $Context.scenarioScript $Context.scenarioScriptSha256
    if ($null -ne $Context.PSObject.Properties['request']) {
        Assert-FileHash $Context.request.path $Context.request.sha256
    }
}

function New-ProfilerMcpConfiguration([string]$KnowledgeRepository, [string]$Dotnet, [string]$AnalysisPath = '',
    [string]$EvidenceServerPath = (Join-Path $PSScriptRoot 'Evidence-Mcp.py'), [string]$NativeAssembly = '') {
    $launcher = Join-Path $KnowledgeRepository 'tools\WinUIProfiler\scripts\Start-McpServer.ps1'
    if (!(Test-Path -LiteralPath $launcher -PathType Leaf)) { throw 'The MUX WinUIProfiler MCP launcher is required.' }
    $configuration = @{
        mcpServers = @{
            'winui-profiler' = @{
                type = 'local'; command = if ($NativeAssembly) { $Dotnet } else { 'powershell.exe' }
                args = @(if ($NativeAssembly) { $NativeAssembly } else {
                    @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File', $launcher)
                })
                env = @{ DOTNET_ROOT = Split-Path $Dotnet -Parent; PATH = "$(Split-Path $Dotnet -Parent);$env:PATH" }
                tools = @('*'); timeout = 600000
            }
        }
    }
    if ($AnalysisPath) {
        $configuration.mcpServers['ui-profiler-evidence'] = @{
            type = 'local'; command = (Get-Command python -ErrorAction Stop).Path
            args = @($EvidenceServerPath, '--analysis', $AnalysisPath)
            tools = @('*'); timeout = 600000
        }
    }
    $configuration
}

function Build-ProfilerMcp($Context, [string]$KnowledgeRepository) {
    $prefix = 'tools/WinUIProfiler/'
    $files = @(git -C $KnowledgeRepository ls-files -- 'tools/WinUIProfiler/WinUIProfiler.Core/*' `
        'tools/WinUIProfiler/WinUIProfiler.Mcp/*' 'tools/WinUIProfiler/Directory.Build.*' `
        'tools/WinUIProfiler/Directory.Packages.props' 'tools/WinUIProfiler/global.json' 'tools/WinUIProfiler/NuGet.Config')
    if ($LASTEXITCODE -ne 0 -or !$files -or !($files | Where-Object { $_ -like '*WinUIProfiler.Mcp.csproj' })) {
        throw 'The tracked MUX WinUIProfiler MCP/Core source and build inputs are required.'
    }
    $source = Join-Path $Context.directory 'profiler-mcp-source'
    if (Test-Path -LiteralPath $source) { throw 'Partial MCP tooling exists; do not overwrite it or claim readiness.' }
    $relativeFiles = @($files | ForEach-Object { $_.Substring($prefix.Length).Replace('/', '\') })
    foreach ($file in $relativeFiles) {
        $destination = Join-Path $source $file
        New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
        Copy-Item -LiteralPath (Join-Path (Join-Path $KnowledgeRepository 'tools\WinUIProfiler') $file) -Destination $destination
    }
    $codeFiles = @($relativeFiles | Where-Object { $_ -notlike '*packages.lock.json' })
    $sourceHash = Get-SourceHash $source $codeFiles
    $output = Join-Path $Context.directory 'pack\mcp\winui-profiler'
    # Restore only the private source copy: CLI/MCP RIDs may differ in the repository's lock files.
    $arguments = @('publish', (Join-Path $source 'WinUIProfiler.Mcp\WinUIProfiler.Mcp.csproj'),
        '--configuration', 'Release', '--artifacts-path', (Join-Path $Context.directory 'profiler-mcp-build'),
        '--output', $output, '-p:RestoreLockedMode=false', '-p:UseSharedCompilation=false', '-nodeReuse:false', '-nologo')
    [void](Invoke-LoggedProcess $Context.dotnet $arguments $Context.directory `
        (Join-Path $Context.directory 'profiler-mcp-build.log') 600)
    if ((Get-SourceHash $source $codeFiles) -ne $sourceHash) { throw 'The private MCP build changed profiler source.' }
    $assembly = Join-Path $output 'WinUIProfiler.Mcp.dll'
    if (!(Test-Path -LiteralPath $assembly -PathType Leaf)) { throw 'The original WinUIProfiler MCP did not publish.' }
    @{ path = $assembly; sha256 = Get-Sha256 $assembly
        deploymentSha256 = Get-ProfilerHash $assembly
        source = $source; sourceSha256 = $sourceHash; files = $codeFiles
        implementation = 'Unchanged MUX WinUIProfiler.Mcp/Core source, privately published; not a replacement server.' }
}

function Invoke-AdeRuntime([string[]]$Arguments, $Context, [switch]$Probe) {
    Assert-FileHash $Context.adeLauncher $Context.adeLauncherSha256
    $saved = @{}
    foreach ($name in @('COPILOT_CLI_PATH', 'COPILOT_CLI_RESOLVED_DIST_DIR', 'COPILOT_HOME',
        'COPILOT_AUTO_UPDATE', 'COPILOT_GITHUB_TOKEN')) {
        $saved[$name] = [Environment]::GetEnvironmentVariable($name)
    }
    try {
        if ($Context.adeCopilotCli) { $env:COPILOT_CLI_PATH = $Context.adeCopilotCli }
        $env:COPILOT_CLI_RESOLVED_DIST_DIR = $null
        $env:COPILOT_HOME = Join-Path $Context.directory 'ade-client'
        $env:COPILOT_AUTO_UPDATE = 'false'
        if ($Context.githubAccount) {
            $token = gh auth token --hostname github.com --user $Context.githubAccount
            if ($LASTEXITCODE -ne 0) { throw 'The selected existing GitHub credential is unavailable.' }
            $env:COPILOT_GITHUB_TOKEN = $token.Trim()
            $token = $null
        } elseif (!$env:COPILOT_GITHUB_TOKEN -and !$env:GH_TOKEN -and !$env:GITHUB_TOKEN) {
            throw 'Supply an existing licensed GitHub account or an existing token in the environment.'
        }
        if ($Probe) {
            $runtime = if ($env:ADE_RUNTIME_DIR) { $env:ADE_RUNTIME_DIR } else {
                Join-Path $env:LOCALAPPDATA 'ade\runtime\0.1.0'
            }
            Add-Type -Path (Join-Path $runtime 'GitHub.Copilot.SDK.dll')
            $options = [GitHub.Copilot.CopilotClientOptions]::new()
            $options.UseLoggedInUser = $true
            $options.WorkingDirectory = $Context.originalRepository
            $client = [GitHub.Copilot.CopilotClient]::new($options)
            $stop = [Threading.CancellationTokenSource]::new(60000)
            try {
                [void]$client.StartAsync($stop.Token).GetAwaiter().GetResult()
                $auth = $client.GetAuthStatusAsync($stop.Token).GetAwaiter().GetResult()
                if (!$auth.IsAuthenticated) { throw 'ADE provider is not authenticated.' }
            } finally {
                [void]$client.DisposeAsync().AsTask().GetAwaiter().GetResult()
                $stop.Dispose()
            }
        } else {
            $output = @(& $Context.adeLauncher @Arguments)
            if ($LASTEXITCODE -ne 0) { throw "ADE failed: $($output -join [Environment]::NewLine)" }
            $line = $output | Where-Object { $_ -like 'ADE-RESULT:*' } | Select-Object -Last 1
            if (!$line) { throw 'ADE returned no structured result.' }
            $result = $line.Substring(12) | ConvertFrom-Json
            if (!$result.ok) { throw "ADE stopped: $($result.stopReason): $($result.message)" }
            $result
        }
    } finally {
        foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
    }
}

function Reserve-Capture([string]$Directory, [string]$SourceHash) {
    New-Item -ItemType Directory -Force $Directory | Out-Null
    $path = Join-Path $Directory 'capture-budget.json'
    try {
        $stream = [IO.File]::Open($path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    } catch [IO.IOException] {
        throw "The one-capture budget is already spent or reserved: $Directory. No automatic retry."
    }
    $stream.Dispose()
    Write-JsonFile $path @{ state = 'reserved'; sourceSha256 = $SourceHash; startedUtc = [DateTime]::UtcNow.ToString('O') }
}

function Get-TickStatistics([object[]]$Frames) {
    $durations = @($Frames | Where-Object { $null -ne $_.TickMilliseconds } |
        ForEach-Object {
            $value = [double]$_.TickMilliseconds
            if (![double]::IsFinite($value) -or $value -lt 0) { throw 'Invalid tick duration.' }
            $value
        } | Sort-Object)
    if ($durations.Count -eq 0) { throw 'No complete UI ticks in the scenario window; no performance conclusion is possible.' }
    $sum = ($durations | Measure-Object -Sum).Sum
    @{
        completedTicks = $durations.Count
        totalTickMs = $sum
        meanTickMs = $sum / $durations.Count
        p95TickMs = $durations[[int][Math]::Ceiling(0.95 * $durations.Count) - 1]
        maxTickMs = $durations[-1]
    }
}

function Assert-Measurement($Measurement) {
    if ($Measurement.schema -notin @('ui-profiler/measurement/v1', 'ui-profiler/measurement/v2') -or $Measurement.state -ne 'complete') {
        throw 'A completed, versioned measurement is required.'
    }
    Assert-FileHash $Measurement.database.path $Measurement.database.sha256
    Assert-FileHash $Measurement.actions.path $Measurement.actions.sha256
    if ($Measurement.schema -eq 'ui-profiler/measurement/v2') {
        if (@($Measurement.evidence).Count -eq 0) { throw 'Measurement v2 requires its secondary evidence hashes.' }
        foreach ($evidence in $Measurement.evidence) { Assert-FileHash $evidence.path $evidence.sha256 }
    }
    if ($Measurement.health.lostByEtw -ne 0 -or $Measurement.health.droppedByProfiler -ne 0 -or
        $null -ne $Measurement.health.completeFromSeq -or !$Measurement.health.ended) {
        throw 'Lost, dropped, trimmed, or unfinished recordings cannot support this comparison.'
    }
    if ($Measurement.metrics.completedTicks -le 0) { throw 'Measurement has no complete scenario ticks.' }
    foreach ($name in @('scenarioElapsedMs', 'p95TickMs', 'meanTickMs', 'totalTickMs', 'maxTickMs')) {
        if ($null -eq $Measurement.metrics.$name -or ![double]::IsFinite([double]$Measurement.metrics.$name) -or
            [double]$Measurement.metrics.$name -lt 0) { throw "Missing or invalid metric: $name" }
    }
    $actions = Read-JsonFile $Measurement.actions.path
    if ($actions.state -ne 'complete' -or @($actions.steps).Count -eq 0 -or
        @($actions.steps | Where-Object { $_.state -ne 'passed' }).Count -ne 0) { throw 'Scenario assertions did not pass.' }
}

function Compare-TimingMeasurements($Baseline, $Candidate) {
    Assert-Measurement $Baseline
    Assert-Measurement $Candidate
    foreach ($name in @('scenarioSha256', 'profilerSha256', 'frameworkSha256')) {
        if ($Baseline.$name -ne $Candidate.$name) { throw "Incomparable inputs: $name" }
    }
    $beforeActions = Read-JsonFile $Baseline.actions.path
    $afterActions = Read-JsonFile $Candidate.actions.path
    foreach ($name in @('dpi', 'width', 'height')) {
        if ($beforeActions.window.$name -ne $afterActions.window.$name) { throw "Different window conditions: $name" }
    }
    $beforeSteps = $beforeActions.steps | Select-Object id, action, from, to | ConvertTo-Json -Depth 10 -Compress
    $afterSteps = $afterActions.steps | Select-Object id, action, from, to | ConvertTo-Json -Depth 10 -Compress
    if ($beforeSteps -ne $afterSteps) { throw 'Different actions or expansion transitions.' }
    $rows = foreach ($name in @('scenarioElapsedMs', 'p95TickMs', 'meanTickMs', 'totalTickMs', 'maxTickMs', 'completedTicks')) {
        $before = [double]$Baseline.metrics.$name
        $after = [double]$Candidate.metrics.$name
        [pscustomobject]@{
            metric = $name
            baseline = $before
            candidate = $after
            delta = $after - $before
            reductionPercent = if ($before -eq 0) { $null } else { 100 * ($before - $after) / $before }
        }
    }
    @{
        schema = 'ui-profiler/comparison/v1'
        interpretation = 'One observed baseline/candidate pair, not statistical evidence. Tick times are elapsed UI-thread time, not CPU time. Scenario elapsed time includes automation waits.'
        metrics = @($rows)
        baseline = $Baseline.database
        candidate = $Candidate.database
    }
}

function Export-MeasurementAnalysis([string]$MeasurementPath, [string]$Directory, [string]$ProfilerCli) {
    $measurement = Read-JsonFile $MeasurementPath
    Assert-Measurement $measurement
    $analyzer = Join-Path $PSScriptRoot 'Analyze-Profiler.py'
    $python = (Get-Command python -ErrorAction Stop).Path
    $reportPath = Join-Path $Directory 'analysis.json'
    if (Test-Path -LiteralPath $reportPath) {
        $report = Read-JsonFile $reportPath
        if ($report.measurement.sha256 -ne (Get-Sha256 $MeasurementPath)) { throw 'Cached analysis belongs to different evidence.' }
        $manifestPath = Join-Path $Directory 'manifest.json'
        $manifest = Read-JsonFile $manifestPath
        $code = @($manifest.inputs | Where-Object origin -eq analyzer)
        if ($code.Count -ne 1 -or $code[0].sha256 -ne (Get-Sha256 $analyzer)) {
            throw 'Analysis implementation changed; choose a new derived-output directory.'
        }
        $arguments = @($analyzer, 'verify', '--manifest', $manifestPath)
        [void](Invoke-LoggedProcess $python $arguments $Directory (Join-Path $Directory 'verify.log') 120)
        return @{ path = $reportPath; sha256 = Get-Sha256 $reportPath; manifestPath = $manifestPath }
    }
    if (Test-Path -LiteralPath $Directory) { throw 'Partial analysis output exists; choose a new directory instead of overwriting evidence.' }
    New-Item -ItemType Directory $Directory | Out-Null
    $exporter = Join-Path $PSScriptRoot 'Export-ProfilerEvidence.ps1'
    $queries = Join-Path $Directory 'queries'
    $arguments = @('-NoProfile', '-NonInteractive', '-File', $exporter, '-MeasurementPath', $MeasurementPath,
        '-ProfilerCli', $ProfilerCli, '-Directory', $queries)
    [void](Invoke-LoggedProcess (Get-Command pwsh -ErrorAction Stop).Path $arguments $Directory (Join-Path $Directory 'export.log') 300)
    $arguments = @($analyzer, 'analyze', '--measurement', $MeasurementPath,
        '--engine', (Join-Path $queries 'engine.json'), '--directory', $Directory)
    [void](Invoke-LoggedProcess $python $arguments $Directory (Join-Path $Directory 'analyze.log') 300)
    @{ path = $reportPath; sha256 = Get-Sha256 $reportPath; manifestPath = Join-Path $Directory 'manifest.json' }
}

function Compare-Measurements([string]$BaselinePath, [string]$CandidatePath, [string]$Directory, [string]$ProfilerCli) {
    $baseline = Read-JsonFile $BaselinePath
    $candidate = Read-JsonFile $CandidatePath
    [void](Compare-TimingMeasurements $baseline $candidate)
    if ((Get-ProfilerHash $ProfilerCli) -ne $baseline.profilerSha256) { throw 'Use the exact recorded profiler payload.' }
    $analyzer = Join-Path $PSScriptRoot 'Analyze-Profiler.py'
    $python = (Get-Command python -ErrorAction Stop).Path
    $reportPath = Join-Path $Directory 'comparison.json'
    if (Test-Path -LiteralPath $reportPath) {
        $report = Read-JsonFile $reportPath
        if ($report.baseline.sha256 -ne (Get-Sha256 $BaselinePath) -or
            $report.candidate.sha256 -ne (Get-Sha256 $CandidatePath)) { throw 'Cached comparison belongs to a different pair.' }
        $arguments = @($analyzer, 'verify', '--manifest', (Join-Path $Directory 'manifest.json'))
        [void](Invoke-LoggedProcess $python $arguments $Directory (Join-Path $Directory 'verify.log') 120)
        return $report
    }
    if (Test-Path -LiteralPath $Directory) { throw 'Use a new comparison directory; derived reports never overwrite prior results.' }
    New-Item -ItemType Directory $Directory | Out-Null
    $before = Export-MeasurementAnalysis $BaselinePath (Join-Path $Directory 'baseline') $ProfilerCli
    $after = Export-MeasurementAnalysis $CandidatePath (Join-Path $Directory 'candidate') $ProfilerCli
    [void](Invoke-LoggedProcess $python @($analyzer, 'compare', '--baseline', $before.path, '--candidate', $after.path,
        '--directory', $Directory) $Directory (Join-Path $Directory 'compare.log') 300)
    Read-JsonFile $reportPath
}

function Invoke-ProfilerQuery([string]$Cli, [string[]]$Arguments, [string]$Directory, [string]$Name) {
    $path = Join-Path $Directory "$Name.json"
    $text = Invoke-LoggedProcess $Cli ($Arguments + '--json') $Directory (Join-Path $Directory "$Name.log") 120
    [IO.File]::WriteAllText($path, $text)
    $text | ConvertFrom-Json
}

function Close-OwnedApplication($Identity, [string]$Executable) {
    if ($null -eq $Identity) { return }
    $target = Get-Process -Id $Identity.processId -ErrorAction SilentlyContinue
    if (!$target) { return }
    try {
        if ($target.MainModule.FileName -ne $Executable -or
            $target.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $Identity.creationTimeFileTime) {
            throw 'Refusing to close a different process instance.'
        }
        [void]$target.CloseMainWindow()
        if (!$target.WaitForExit(5000)) { Stop-Process -Id $target.Id }
    } finally { $target.Dispose() }
}

function Receive-ProfilerOutput($Pump) {
    while ($null -ne $Pump.pending -and $Pump.pending.IsCompleted) {
        $line = $Pump.pending.GetAwaiter().GetResult()
        if ($null -eq $line) { $Pump.pending = $null; break }
        $Pump.writer.WriteLine($line)
        $Pump.writer.Flush()
        $item = $line | ConvertFrom-Json
        $Pump.items.Add($item)
        if ($item.type -eq 'status' -and $null -ne $item.data.PSObject.Properties['scope']) {
            $Pump.ready = $item.data
        }
        if ($item.type -eq 'summary') { $Pump.summary = $item.data }
        $Pump.pending = $Pump.process.StandardOutput.ReadLineAsync()
    }
}

function Request-ProfilerSnapshot($Pump, [int]$TimeoutSeconds = 30) {
    $count = @($Pump.items | Where-Object type -eq snapshot).Count
    $Pump.process.StandardInput.WriteLine('snapshot')
    $Pump.process.StandardInput.Flush()
    $timeout = [Diagnostics.Stopwatch]::StartNew()
    while (@($Pump.items | Where-Object type -eq snapshot).Count -le $count) {
        Receive-ProfilerOutput $Pump
        if ($Pump.process.HasExited -or $timeout.Elapsed.TotalSeconds -gt $TimeoutSeconds) {
            throw 'The existing recording could not retain its end-of-scenario snapshot.'
        }
        Start-Sleep -Milliseconds 20
    }
}

function Invoke-Measurement($Context, [string]$RepositoryPath, [string]$Role, [string[]]$SourceFiles = @()) {
    if ($Role -notin @('baseline', 'candidate')) { throw 'Unknown capture role.' }
    Assert-FrozenScenario $Context
    if ((Get-ProfilerHash $Context.profilerCli) -ne $Context.profilerSha256) { throw 'Profiler payload changed.' }
    $sourceHash = Get-SourceHash $RepositoryPath $SourceFiles
    $directory = Join-Path $Context.directory $Role
    $reportPath = Join-Path $directory 'measurement.json'
    if (Test-Path -LiteralPath $reportPath) {
        $cached = Read-JsonFile $reportPath
        Assert-Measurement $cached
        if ($cached.sourceSha256 -ne $sourceHash) { throw 'This capture was for a different source revision; the budget cannot be reused.' }
        foreach ($name in @('scenarioSha256', 'profilerSha256')) {
            if ($cached.$name -ne $Context.$name) { throw "Cached evidence belongs to different inputs: $name" }
        }
        return $cached
    }
    Assert-InteractiveDesktop
    $executable = Build-Application $Context $RepositoryPath (Join-Path $directory 'app')
    if ((Get-SourceHash $RepositoryPath $SourceFiles) -ne $sourceHash) {
        throw 'The build changed source files; resolve that before capturing.'
    }
    $framework = Join-Path (Split-Path $executable -Parent) 'Microsoft.ui.xaml.dll'
    $frameworkHash = Get-Sha256 $framework
    Reserve-Capture $directory $sourceHash
    $process = $null
    $automation = $null
    $identity = $null
    $pump = $null
    $errors = $null
    try {
        $arguments = @('live', '--exe', $executable, '--working-directory', (Split-Path $executable -Parent),
            '--trust-local-binaries', '--session-directory', $directory, '--snapshot-directory', $directory,
            '--interactive', '--duration', '120', '--json')
        $process = New-LoggedProcess $Context.profilerCli $arguments $directory
        $errors = $process.StandardError.ReadToEndAsync()
        $pump = @{
            process = $process
            pending = $process.StandardOutput.ReadLineAsync()
            writer = [IO.StreamWriter]::new((Join-Path $directory 'live.ndjson'), $false, [Text.UTF8Encoding]::new($false))
            items = [Collections.Generic.List[object]]::new()
            ready = $null
            summary = $null
        }
        $timeout = [Diagnostics.Stopwatch]::StartNew()
        while ($null -eq $pump.ready) {
            Receive-ProfilerOutput $pump
            if ($process.HasExited) {
                Receive-ProfilerOutput $pump
                $reason = if ($null -ne $pump.summary -and $null -ne $pump.summary.PSObject.Properties['error']) {
                    $pump.summary.error
                } else { 'No completed startup status was received.' }
                throw "Profiler startup failed: $reason. See $directory\live.ndjson."
            }
            if ($timeout.Elapsed.TotalSeconds -gt 90) { throw 'Profiler failed to become ready within 90 seconds.' }
            Start-Sleep -Milliseconds 20
        }
        $identity = $pump.ready.scope.process
        if ($pump.ready.etwState -ne 'available' -or $pump.ready.recordingState -ne 'available') {
            throw 'ETW or the SQLite recording is unavailable.'
        }
        $target = Get-Process -Id $identity.processId
        try {
            if ($target.MainModule.FileName -ne $executable -or
                $target.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $identity.creationTimeFileTime) {
                throw 'The profiler did not launch the task-owned executable.'
            }
        } finally { $target.Dispose() }
        $actionPath = Join-Path $directory 'actions.json'
        $automation = New-LoggedProcess 'powershell.exe' @('-NoProfile', '-NonInteractive', '-File', $Context.scenarioScript,
            '-ProcessId', [string]$identity.processId, '-CreationTimeFileTime', [string]$identity.creationTimeFileTime,
            '-ScenarioPath', $Context.scenarioPath, '-OutputPath', $actionPath, '-ExpectedExecutable', $executable) $directory
        $automationOut = $automation.StandardOutput.ReadToEndAsync()
        $automationErrors = $automation.StandardError.ReadToEndAsync()
        $automation.StandardInput.Close()
        $timeout.Restart()
        while (!$automation.HasExited) {
            Receive-ProfilerOutput $pump
            if ($process.HasExited -or $timeout.Elapsed.TotalSeconds -gt 90) { throw 'Scenario or recording stopped before verification.' }
            Start-Sleep -Milliseconds 20
        }
        [IO.File]::WriteAllText((Join-Path $directory 'automation.log'),
            $automationOut.GetAwaiter().GetResult() + "`n" + $automationErrors.GetAwaiter().GetResult())
        if ($automation.ExitCode -ne 0) { throw "Scenario failed. See $directory\automation.log" }
        Request-ProfilerSnapshot $pump
        $process.StandardInput.WriteLine('stop')
        $process.StandardInput.Flush()
        $timeout.Restart()
        while (!$process.HasExited) {
            Receive-ProfilerOutput $pump
            if ($timeout.Elapsed.TotalSeconds -gt 30) { throw 'Profiler did not finalize its recording.' }
            Start-Sleep -Milliseconds 20
        }
        while ($null -ne $pump.pending) {
            [void]$pump.pending.Wait(1000)
            Receive-ProfilerOutput $pump
        }
        [IO.File]::WriteAllText((Join-Path $directory 'live.stderr.log'), $errors.GetAwaiter().GetResult())
        if ($process.ExitCode -ne 0 -or $null -eq $pump.summary -or $pump.summary.state -ne 'completed') {
            throw "Incomplete recording. See $directory\live.stderr.log"
        }
        $database = $pump.summary.recordingPath
        $info = Invoke-ProfilerQuery $Context.profilerCli @('info', $database) $directory 'info'
        $frames = @(Invoke-ProfilerQuery $Context.profilerCli @('frames', $database) $directory 'frames')
        $anchor = @(Invoke-ProfilerQuery $Context.profilerCli @('dump', $database, '--max', '1') $directory 'clock-anchor')[0]
        [void](Invoke-ProfilerQuery $Context.profilerCli @('interesting', $database) $directory 'interesting')
        [void](Invoke-ProfilerQuery $Context.profilerCli @('cpu', $database) $directory 'cpu')
        [void](Invoke-ProfilerQuery $Context.profilerCli @('launch', $database) $directory 'launch')
        [void](Invoke-ProfilerQuery $Context.profilerCli @('summary', $database) $directory 'summary')
        [void](Invoke-ProfilerQuery $Context.profilerCli @('objects', $database) $directory 'objects')
        [void](Invoke-ProfilerQuery $Context.profilerCli @('frames', $database, '--list') $directory 'frame-list')
        $actions = Read-JsonFile $actionPath
        $start = $anchor.RelativeMilliseconds + 1000.0 * ($actions.startedQpc - $anchor.TimestampQpc) / $actions.qpcFrequency
        $end = $anchor.RelativeMilliseconds + 1000.0 * ($actions.endedQpc - $anchor.TimestampQpc) / $actions.qpcFrequency
        $selected = @($frames | Where-Object {
            $_.ThreadId -eq $actions.window.threadId -and $null -ne $_.StartMilliseconds -and $null -ne $_.EndMilliseconds -and
            $_.StartMilliseconds -ge $start -and $_.EndMilliseconds -le $end
        })
        $metrics = Get-TickStatistics $selected
        $metrics.scenarioElapsedMs = 1000.0 * ($actions.endedQpc - $actions.startedQpc) / $actions.qpcFrequency
        $slowest = @($selected | Sort-Object TickMilliseconds -Descending | Select-Object -First 5)
        foreach ($frame in $slowest) {
            $queryArguments = @('events', $database, '--frame', [string]$frame.FrameId, '--count', '100')
            [void](Invoke-ProfilerQuery $Context.profilerCli $queryArguments $directory "tick-$($frame.FrameId)")
        }
        $pump.writer.Flush()
        $evidence = @()
        $queryNames = @('info', 'frames', 'clock-anchor', 'interesting', 'cpu', 'launch', 'summary', 'objects', 'frame-list') +
            @($slowest | ForEach-Object { "tick-$($_.FrameId)" })
        foreach ($name in $queryNames) {
            $path = Join-Path $directory "$name.json"
            $evidence += @{ kind = 'query'; name = $name; path = $path; sha256 = Get-Sha256 $path }
        }
        foreach ($path in $pump.summary.snapshotPaths) {
            $evidence += @{ kind = 'snapshot'; path = $path; sha256 = Get-Sha256 $path }
        }
        $livePath = Join-Path $directory 'live.ndjson'
        $evidence += @{ kind = 'native-stream'; path = $livePath; sha256 = Get-Sha256 $livePath }
        $measurement = [pscustomobject]@{
            schema = 'ui-profiler/measurement/v2'
            state = 'complete'
            role = $Role
            sourceSha256 = $sourceHash
            scenarioSha256 = $Context.scenarioSha256
            profilerSha256 = $Context.profilerSha256
            frameworkSha256 = $frameworkHash
            executableSha256 = Get-Sha256 $executable
            database = @{ path = $database; sha256 = Get-Sha256 $database }
            actions = @{ path = $actionPath; sha256 = Get-Sha256 $actionPath }
            evidence = $evidence
            health = @{
                lostByEtw = $info.LostByEtw
                droppedByProfiler = $info.DroppedByProfiler
                completeFromSeq = $info.CompleteFromSeq
                ended = $null -ne $info.EndedUtc
            }
            metrics = $metrics
            windowMilliseconds = @{ start = $start; end = $end }
            slowestTicks = $slowest
            sampledMetrics = @($pump.items | Where-Object { $_.type -eq 'metrics' } | ForEach-Object { $_.data })
            queries = @('info', 'frames', 'dump --max 1', 'interesting', 'cpu', 'launch', 'summary', 'objects',
                'frames --list', 'events --frame ID --count 100')
        }
        Assert-Measurement $measurement
        Write-JsonFile $reportPath $measurement
        Write-JsonFile (Join-Path $directory 'capture-budget.json') @{ state = 'complete'; sourceSha256 = $sourceHash }
        $measurement
    } catch {
        Write-JsonFile (Join-Path $directory 'capture-budget.json') @{ state = 'failed'; sourceSha256 = $sourceHash; error = $_.Exception.Message }
        throw
    } finally {
        if ($null -ne $automation) {
            if (!$automation.HasExited) { Stop-Process -Id $automation.Id -ErrorAction SilentlyContinue }
            $automation.Dispose()
        }
        if ($null -ne $process) {
            if (!$process.HasExited) {
                try { $process.StandardInput.WriteLine('stop') } catch { }
                if (!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
            }
            if ($null -ne $pump) {
                $drain = [Diagnostics.Stopwatch]::StartNew()
                while ($null -ne $pump.pending -and $drain.Elapsed.TotalSeconds -lt 5) {
                    [void]$pump.pending.Wait(100)
                    Receive-ProfilerOutput $pump
                }
                if ($null -eq $identity -and $null -ne $pump.ready) { $identity = $pump.ready.scope.process }
            }
            if ($null -ne $errors -and $errors.Wait(5000)) {
                [IO.File]::WriteAllText((Join-Path $directory 'live.stderr.log'), $errors.GetAwaiter().GetResult())
            }
            $process.Dispose()
        }
        if ($null -ne $pump) { $pump.writer.Dispose() }
        Close-OwnedApplication $identity $executable
    }
}

Export-ModuleMember -Function Read-JsonFile, Write-JsonFile, Get-Sha256, Assert-FileHash, Get-RepositoryFiles,
    Get-SourceHash, Get-ProfilerHash, Copy-ApplicationSource, New-LoggedProcess, Invoke-LoggedProcess,
    Build-Application, Copy-Deployment, Reserve-Capture, Get-TickStatistics, Assert-Measurement,
    Compare-TimingMeasurements, Compare-Measurements, Export-MeasurementAnalysis, Invoke-ProfilerQuery,
    Receive-ProfilerOutput, Request-ProfilerSnapshot, Invoke-Measurement, Assert-MinimalCandidate, Assert-InteractiveDesktop,
    Assert-RunDirectory, Assert-OriginalSource, Assert-InvestigationState, Assert-FrozenScenario,
    New-ProfilerMcpConfiguration, Build-ProfilerMcp, Invoke-AdeRuntime, Close-OwnedApplication,
    Assert-ScopedCandidate, Assert-TrialContext, Assert-TrialReference, Write-RecommendationReport, Assert-RecommendationReport
