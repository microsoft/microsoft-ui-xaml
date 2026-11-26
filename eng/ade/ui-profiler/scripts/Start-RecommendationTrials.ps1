# Prepare independent ADE proposals from an immutable three-level investigation; never approve or run them.
#Requires -Version 7
param(
    [ValidateSet('Prepare', 'Run', 'Verify', 'Report')][string]$Phase = 'Prepare',
    [Parameter(Mandatory)][string]$Directory,
    [string]$SourceInvestigation,
    [string[]]$RecommendationId = @(),
    [string]$GitHubAccount,
    [switch]$PublishDraftPullRequests
)
$ErrorActionPreference = 'Stop'
$plugin = Split-Path $PSScriptRoot -Parent
Import-Module "$plugin\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking

function Get-PreparationSelection($Recommendations, [string[]]$Ids, [string[]]$ApplicationFiles, [string[]]$NativeFiles) {
    $selectedIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($id in $Ids) {
        if (!$selectedIds.Add($id)) { throw 'Recommendation ids must be unique; no trial has been started.' }
    }
    if ($Recommendations.schema -notin @('ui-profiler/recommendations/v2', 'ui-profiler/recommendations/v3')) {
        throw 'Independent native trials require the saved three-level recommendation JSON.'
    }
    $knownIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $Recommendations.recommendations) {
        if ($entry.id -cnotmatch '^[a-z0-9][a-z0-9-]{0,79}$' -or !$knownIds.Add($entry.id) -or
            $entry.level -notin @('application', 'control', 'framework') -or
            $entry.verdict -notin @('propose', 'recommend', 'insufficient-evidence') -or
            ($entry.level -eq 'application' -and $entry.verdict -eq 'recommend') -or
            ($entry.level -ne 'application' -and $entry.verdict -eq 'propose')) {
            throw 'Invalid or duplicate recommendation in the frozen expert output.'
        }
    }
    $selected = if ($Ids.Count) {
        foreach ($id in $Ids) {
            $matches = @($Recommendations.recommendations | Where-Object id -CEQ $id)
            if ($matches.Count -ne 1) { throw "No recommendation with this exact id: $id" }
            $matches[0]
        }
    } else { @($Recommendations.recommendations | Where-Object verdict -ne 'insufficient-evidence') }
    if (!@($selected).Count) { throw 'The report contains no selected actionable recommendations.' }
    foreach ($entry in $selected) {
        if ($entry.verdict -eq 'insufficient-evidence' -or !@($entry.files).Count) {
            throw "Recommendation $($entry.id) has insufficient evidence or no actionable source scope."
        }
        $paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $repositoryFiles = if ($entry.level -eq 'application') { $ApplicationFiles } else { $NativeFiles }
        foreach ($file in $entry.files) {
            $path = $file.path.Replace('\', '/')
            if ([IO.Path]::IsPathRooted($file.path) -or $path -match '(^|/)\.\.?(/|$)|[:*?\[\]]|//|(^|/)\.git(/|$)' -or
                !$paths.Add($path) -or $path -cnotin @($repositoryFiles | ForEach-Object { $_.Replace('\', '/') })) {
                throw "Recommendation $($entry.id) escapes its existing source-file scope: $path"
            }
            if ($path -match '(^|/)(tests?|\.github)(/|$)|(^|/)(AGENTS\.md|copilot-instructions\.md)$') {
                throw 'Tests and instructions cannot be edited by a recommendation trial.'
            }
            if (($entry.level -eq 'application' -and $path -notmatch '\.(cs|xaml)$') -or
                ($entry.level -ne 'application' -and $path -notmatch '\.(cpp|h|hpp|inl|idl|xaml|cs)$') -or
                ($entry.level -eq 'control' -and !$path.StartsWith('controls/dev/', [StringComparison]::Ordinal)) -or
                ($entry.level -eq 'framework' -and !$path.StartsWith('dxaml/xcp/', [StringComparison]::Ordinal))) {
                throw "Recommendation $($entry.id) does not belong to its declared source layer."
            }
        }
    }
    $selected
}

function Get-PreparationReportMetadata($Recommendations) {
    $metadata = @{
        coverage = $Recommendations.coverage
        runtimeSourceParity = $Recommendations.runtimeSourceParity
    }
    if ($null -ne $Recommendations.detailedAnalysis) {
        $metadata.detailedAnalysis = $Recommendations.detailedAnalysis
    }
    $metadata
}

function Get-PreparationLayerNotes($Metadata) {
    $text = ($Metadata.coverage | ConvertTo-Json -Depth 30) +
        "`nOriginal runtime/source parity limitation: $($Metadata.runtimeSourceParity)"
    if ($Metadata.ContainsKey('detailedAnalysis')) {
        $text += "`n`n# Detailed layer analysis (original expert layout)`n`n" +
            ($Metadata.detailedAnalysis | ConvertTo-Json -Depth 40)
    }
    $text
}

function Get-PreparationControlMetadata($Layered) {
    $metadata = @{}
    foreach ($name in @('targetControl', 'controlAnalysis', 'supplementalEvidence')) {
        $present = if ($Layered -is [Collections.IDictionary]) { $Layered.Contains($name) }
            else { $null -ne $Layered.PSObject.Properties[$name] }
        if ($present) { $metadata[$name] = $Layered.$name }
    }
    $metadata
}

function Get-PreparationNativeRepositoryFiles([string]$Repository) {
    $previousCount = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT')
    $count = 0
    if ($null -ne $previousCount -and ($previousCount -notmatch '^\d+$' -or
        ![int]::TryParse($previousCount, [ref]$count) -or $count -eq [int]::MaxValue)) {
        throw 'Invalid inherited Git command configuration count.'
    }
    $keyName = "GIT_CONFIG_KEY_$count"
    $valueName = "GIT_CONFIG_VALUE_$count"
    $previousKey = [Environment]::GetEnvironmentVariable($keyName)
    $previousValue = [Environment]::GetEnvironmentVariable($valueName)
    try {
        [Environment]::SetEnvironmentVariable($keyName, 'core.longpaths')
        [Environment]::SetEnvironmentVariable($valueName, 'true')
        [Environment]::SetEnvironmentVariable('GIT_CONFIG_COUNT', [string]($count + 1))
        Get-NativeRepositoryFiles $Repository
    } finally {
        [Environment]::SetEnvironmentVariable('GIT_CONFIG_COUNT', $previousCount)
        [Environment]::SetEnvironmentVariable($keyName, $previousKey)
        [Environment]::SetEnvironmentVariable($valueName, $previousValue)
    }
}

function Get-PreparationKnowledgeRoot($Layered, $Request) {
    $metadata = if ($Layered -is [Collections.IDictionary]) { $Layered['knowledgeRepository'] }
        elseif ($Layered.PSObject.Properties['knowledgeRepository']) { $Layered.knowledgeRepository } else { $null }
    $root = if ($null -ne $metadata) { $metadata.path } else { $Request.knowledgeRepository }
    if ($root -isnot [string] -or ![IO.Path]::IsPathFullyQualified($root)) {
        throw 'A declared absolute hypothesis source checkout is required.'
    }
    [IO.Path]::GetFullPath($root).TrimEnd('\')
}

function Get-PreparationKnowledgeSource($Layered, $Request) {
    $root = Get-PreparationKnowledgeRoot $Layered $Request
    $gitRoot = git -C $root rev-parse --show-toplevel
    if ($LASTEXITCODE -ne 0 -or [IO.Path]::GetFullPath($gitRoot.Replace('/', '\')).TrimEnd('\') -ne $root) {
        throw 'Use the actual hypothesis Git repository root, not a nested source directory.'
    }
    $files = @(Get-PreparationNativeRepositoryFiles $root)
    $revision = git -C $root rev-parse HEAD
    if ($LASTEXITCODE -ne 0) { throw 'The hypothesis source must be a Git checkout.' }
    $current = @{
        path = $root; revision = $revision; sourceSha256 = Get-SourceHash $root $files; files = $files
        excludedSampleSubmodules = @(Get-NativeSampleSubmodules $root)
    }
    $recorded = if ($Layered -is [Collections.IDictionary]) { $Layered['knowledgeRepository'] }
        elseif ($Layered.PSObject.Properties['knowledgeRepository']) { $Layered.knowledgeRepository } else { $null }
    if ($recorded -and ($recorded.revision -cne $current.revision -or $recorded.sourceSha256 -cne $current.sourceSha256 -or
        (ConvertTo-Json @($recorded.files) -Compress) -cne (ConvertTo-Json @($current.files) -Compress) -or
        (ConvertTo-Json @($recorded.excludedSampleSubmodules | Select-Object path, revision) -Compress) -cne
        (ConvertTo-Json @($current.excludedSampleSubmodules | Select-Object path, revision) -Compress))) {
        throw 'The declared expert hypothesis source no longer matches its frozen checkout identity.'
    }
    $current
}

function New-PreparationRequestBinding($Request, $OriginalReference, $Knowledge, [string]$Directory) {
    Assert-FileHash $OriginalReference.path $OriginalReference.sha256
    if ([IO.Path]::GetFullPath($Request.knowledgeRepository).TrimEnd('\') -eq $Knowledge.path) { return $OriginalReference }
    $path = Join-Path $Directory 'request.json'
    if (Test-Path -LiteralPath $path) { throw 'Never overwrite a derived trial request.' }
    $derived = $Request | ConvertTo-Json -Depth 60 | ConvertFrom-Json -AsHashtable
    $derived.directory = $Directory
    $derived.knowledgeRepository = $Knowledge.path
    $derived.sourceRequest = @{ path = $OriginalReference.path; sha256 = $OriginalReference.sha256 }
    $derived.hypothesisSource = $Knowledge
    Write-JsonFile $path $derived
    @{ path = $path; sha256 = Get-Sha256 $path }
}

function New-PreparationAdeLauncher([string]$Source, [string]$SourceSha256, [string]$Directory) {
    Assert-FileHash $Source $SourceSha256
    if (Test-Path -LiteralPath $Directory) { throw 'Never overwrite a frozen ADE launcher directory.' }
    New-Item -ItemType Directory $Directory | Out-Null
    Copy-Item -LiteralPath $Source -Destination (Join-Path $Directory 'ade-pinned.ps1')
    # Scope Git's command configuration to ADE and its descendants, including later approved workcopy init/clone.
    $wrapper = @'
#Requires -Version 7
$ErrorActionPreference = 'Stop'
$pinned = Join-Path $PSScriptRoot 'ade-pinned.ps1'
if ((Get-FileHash -LiteralPath $pinned -Algorithm SHA256).Hash.ToLowerInvariant() -cne '__PINNED_SHA256__') {
    throw 'The frozen upstream ADE launcher changed.'
}
$previousCount = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT')
$count = 0
if ($null -ne $previousCount -and ($previousCount -notmatch '^\d+$' -or
    ![int]::TryParse($previousCount, [ref]$count) -or $count -eq [int]::MaxValue)) {
    throw 'Invalid inherited Git command configuration count.'
}
$keyName = "GIT_CONFIG_KEY_$count"
$valueName = "GIT_CONFIG_VALUE_$count"
$previousKey = [Environment]::GetEnvironmentVariable($keyName)
$previousValue = [Environment]::GetEnvironmentVariable($valueName)
try {
    [Environment]::SetEnvironmentVariable($keyName, 'core.longpaths')
    [Environment]::SetEnvironmentVariable($valueName, 'true')
    [Environment]::SetEnvironmentVariable('GIT_CONFIG_COUNT', [string]($count + 1))
    & $pinned @args
    $code = $LASTEXITCODE
} finally {
    [Environment]::SetEnvironmentVariable('GIT_CONFIG_COUNT', $previousCount)
    [Environment]::SetEnvironmentVariable($keyName, $previousKey)
    [Environment]::SetEnvironmentVariable($valueName, $previousValue)
}
exit $code
'@
    [IO.File]::WriteAllText((Join-Path $Directory 'ade.ps1'), $wrapper.Replace('__PINNED_SHA256__', $SourceSha256),
        [Text.UTF8Encoding]::new($false))
}

function New-PreparationTrialContext($Source, $Layered, $LayeredReference, $Entry,
    [string]$InvestigationDirectory, [string]$TrialDirectory, [string]$Pack, [string]$Account, $NativeSnapshot = $null,
    $RequestReference = $null, $Knowledge = $null) {
    $trial = $Source | ConvertTo-Json -Depth 60 | ConvertFrom-Json -AsHashtable
    foreach ($key in @('trials', 'report', 'preview', 'adapter', 'state')) { $trial.Remove($key) }
    $trial.schema = if ($Entry.level -eq 'application') { 'ui-profiler/trial/v1' } else { 'ui-profiler/native-trial/v1' }
    $trial.directory = $TrialDirectory
    $trial.investigationDirectory = $InvestigationDirectory
    $trial.sourceInvestigation = $LayeredReference
    $trial.originalInvestigation = $Layered.sourceInvestigation
    if ($RequestReference) {
        $trial.request = $RequestReference
        if ($RequestReference.path -ne $Source.request.path) { $trial.originalRequest = $Source.request }
    }
    if ($Knowledge) { $trial.knowledgeRepository = $Knowledge }
    $trial.contracts = @{ path = (Join-Path $Pack 'knowledge\harness-contracts.md')
        sha256 = Get-Sha256 (Join-Path $Pack 'knowledge\harness-contracts.md') }
    $trial.recommendations = $Layered.recommendations
    $trial.expert = $Layered.provenance
    $controlMetadata = Get-PreparationControlMetadata $Layered
    foreach ($key in $controlMetadata.Keys) { $trial[$key] = $controlMetadata[$key] }
    $trial.recommendationId = $Entry.id
    $trial.allowedPaths = @($Entry.files | ForEach-Object path)
    $trial.module = @{ path = (Join-Path $Pack 'tools\ProfilerHarness.psm1')
        sha256 = Get-Sha256 (Join-Path $Pack 'tools\ProfilerHarness.psm1') }
    if ($Account) { $trial.githubAccount = $Account }
    $trial.adeLauncher = Join-Path $TrialDirectory 'scripts\ade.ps1'
    $trial.adeLauncherSha256 = Get-Sha256 $trial.adeLauncher
    if ($Entry.level -ne 'application') {
        if (!$NativeSnapshot) { throw 'A native trial requires its private source snapshot identity.' }
        $trial.native = @{
            level = $Entry.level; originalRepository = $NativeSnapshot.originalRepository
            originalRevision = $NativeSnapshot.originalRevision; originalSourceSha256 = $NativeSnapshot.originalSourceSha256
            source = $NativeSnapshot.source; sourceSha256 = $NativeSnapshot.originalSourceSha256; files = @($NativeSnapshot.files)
            excludedSampleSubmodules = @($NativeSnapshot.excludedSampleSubmodules)
            applicationSource = $NativeSnapshot.applicationSource; applicationSourceSha256 = $Source.originalSourceSha256
            allowedPaths = $trial.allowedPaths
            authorization = @{ baselineCaptures = 1; candidateCaptures = 1; nativeRuntimeChanges = $true }
            comparisonPolicy = 'Source-matched freshly built native baseline versus candidate. The old recording is discovery evidence only.'
            implementation = @{
                module = @{ path = (Join-Path $Pack 'tools\NativeTrials.psm1')
                    sha256 = Get-Sha256 (Join-Path $Pack 'tools\NativeTrials.psm1') }
                runtime = @{ path = (Join-Path $Pack 'tools\NativeRuntime.psm1')
                    sha256 = Get-Sha256 (Join-Path $Pack 'tools\NativeRuntime.psm1') }
                measure = @{ path = (Join-Path $Pack 'tools\Measure-NativeTrial.ps1')
                    sha256 = Get-Sha256 (Join-Path $Pack 'tools\Measure-NativeTrial.ps1') }
            }
        }
    }
    $trial
}

function Assert-PreparationSource([string]$Repository, [string]$Revision, [string]$Sha256,
    [object[]]$ExcludedSampleSubmodules = @()) {
    $actualRevision = git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actualRevision -cne $Revision -or
        (Get-SourceHash $Repository @(Get-PreparationNativeRepositoryFiles $Repository)) -cne $Sha256) {
        throw 'The original native source changed during independent trial preparation.'
    }
    $actualExclusions = @(Get-NativeSampleSubmodules $Repository | Sort-Object path | Select-Object path, revision)
    if ((ConvertTo-Json $actualExclusions -Depth 10 -Compress) -cne
        (ConvertTo-Json @($ExcludedSampleSubmodules | Sort-Object path | Select-Object path, revision) -Depth 10 -Compress)) {
        throw 'The excluded sample submodule identities changed during independent trial preparation.'
    }
}

function New-NativeSourceSnapshot([string]$Repository, [string]$Destination, [string[]]$Files,
    [string]$Revision, [string]$Sha256, [object[]]$ExcludedSampleSubmodules = @()) {
    Assert-PreparationSource $Repository $Revision $Sha256 $ExcludedSampleSubmodules
    if (Test-Path -LiteralPath $Destination) { throw 'Never reuse a native source snapshot.' }
    foreach ($relative in $Files) {
        if ($relative.Replace('\', '/') -match '(^|/)\.git(/|$)') { throw 'Git metadata is not native source.' }
    }
    Copy-ApplicationSource $Repository $Destination $Files
    if ((Get-SourceHash $Destination $Files) -cne $Sha256) { throw 'The native source copy is not byte-identical.' }
    & git -C $Destination -c init.templateDir= -c core.longpaths=true init --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the private native source repository.' }
    & git -C $Destination config --local core.longpaths true
    if ($LASTEXITCODE -ne 0) { throw 'Could not enable long paths in the private native source repository.' }
    # Keep CRLF and filtered source bytes exact, including files already tracked despite .gitignore.
    New-Item -ItemType Directory -Force (Join-Path $Destination '.git\info') | Out-Null
    [IO.File]::WriteAllText((Join-Path $Destination '.git\info\attributes'), "* -text -filter -ident -working-tree-encoding`n")
    $pathspec = Join-Path $Destination '.git\source-pathspec'
    [IO.File]::WriteAllText($pathspec, (($Files | ForEach-Object { ":(literal)$_" }) -join "`0") + "`0",
        [Text.UTF8Encoding]::new($false))
    & git -C $Destination -c core.autocrlf=false add --force "--pathspec-from-file=$pathspec" --pathspec-file-nul
    if ($LASTEXITCODE -ne 0) { throw 'Could not index every original native source file.' }
    Remove-Item -LiteralPath $pathspec
    $hooks = Join-Path $Destination '.git\disabled-hooks'
    [void](New-Item -ItemType Directory $hooks)
    & git -C $Destination -c user.name=Copilot -c user.email=223556219+Copilot@users.noreply.github.com `
        -c "core.hooksPath=$hooks" -c commit.gpgsign=false commit --quiet -m 'Freeze original native source for an independent recommendation trial' `
        -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Could not commit the private native source snapshot.' }
    $status = @(git -C $Destination status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $status.Count -or (Get-SourceHash $Destination) -cne $Sha256) {
        throw 'The private native source repository must be clean and byte-identical to the original.'
    }
    Assert-PreparationSource $Repository $Revision $Sha256 $ExcludedSampleSubmodules
}

function Assert-PreparationBindings($Layered, $Source, $Expert, $Request) {
    foreach ($reference in @($Layered.sourceInvestigation, $Layered.recommendations, $Layered.provenance,
        $Layered.report, $Layered.baseline, $Layered.analysis, $Source.request, $Source.contracts,
        $Source.module, $Source.baseline, $Source.analysis, $Source.nativeMcp)) {
        Assert-FileHash $reference.path $reference.sha256
    }
    $controlMetadata = Get-PreparationControlMetadata $Layered
    if ($controlMetadata.ContainsKey('controlAnalysis') -and $controlMetadata.controlAnalysis) {
        Assert-FileHash $controlMetadata.controlAnalysis.path $controlMetadata.controlAnalysis.sha256
    }
    $supplemental = if ($controlMetadata.ContainsKey('supplementalEvidence')) { $controlMetadata.supplementalEvidence } else { $null }
    $expertSupplemental = if ($Expert.PSObject.Properties['supplementalEvidence']) { $Expert.supplementalEvidence } else { $null }
    if ($supplemental -or $expertSupplemental) {
        if (!$supplemental -or !$expertSupplemental -or $supplemental.path -ne $expertSupplemental.path -or
            $supplemental.sha256 -cne $expertSupplemental.sha256) {
            throw 'The source report and actual expert disagree about supplemental discovery evidence.'
        }
        Assert-FileHash $supplemental.path $supplemental.sha256
        $binding = Get-SupplementalEvidenceBinding $supplemental.path $Source.baseline.path (Read-JsonFile $Source.baseline.path)
        Assert-SupplementalEvidenceBinding $binding
    }
    $knowledgeRoot = Get-PreparationKnowledgeRoot $Layered $Request
    if ($Layered.schema -ne 'ui-profiler/read-only-investigation/v1' -or
        $Source.schema -ne 'ui-profiler/investigation/v3' -or
        $Layered.originalRevision -cne $Source.originalRevision -or
        $Layered.baseline.sha256 -cne $Source.baseline.sha256 -or $Layered.analysis.sha256 -cne $Source.analysis.sha256 -or
        $Expert.agent -ne 'winui-expert' -or $Expert.measurementSha256 -cne $Source.baseline.sha256 -or
        $Expert.analysisSha256 -cne $Source.analysis.sha256 -or $Expert.scenarioSha256 -cne $Source.scenarioSha256 -or
        [IO.Path]::GetFullPath($Expert.sourceRepository).TrimEnd('\') -ne $knowledgeRoot -or
        $Expert.structuredRecommendation.path -ne $Layered.recommendations.path -or
        $Expert.structuredRecommendation.sha256 -cne $Layered.recommendations.sha256 -or
        !@($Expert.mcpQueries | Where-Object server -eq 'winui-profiler').Count -or
        !@($Expert.mcpQueries | Where-Object server -eq 'ui-profiler-evidence').Count) {
        throw 'The expert recommendation is not bound to the original scenario, source and read-only evidence.'
    }
    Assert-FileHash $Expert.structuredRecommendation.path $Expert.structuredRecommendation.sha256
    Assert-FileHash (Join-Path (Split-Path $Layered.recommendations.path -Parent) 'recommendation.md') $Expert.recommendationSha256
    foreach ($level in @('application', 'control', 'framework')) {
        if ($level -notin $Expert.investigationLevels -or !@($Expert.sourceCoverage.$level).Count) {
            throw 'The actual expert must have inspected all three source layers.'
        }
        $coverageRoot = if ($level -eq 'application') { Join-Path $Source.directory 'baseline-source' }
            elseif ($level -eq 'control') { Join-Path $knowledgeRoot 'controls\dev' } else { Join-Path $knowledgeRoot 'dxaml\xcp' }
        $coverageRoot = [IO.Path]::GetFullPath($coverageRoot).TrimEnd('\') + '\'
        foreach ($reference in $Expert.sourceCoverage.$level) {
            if (![IO.Path]::IsPathFullyQualified($reference.path) -or
                ![IO.Path]::GetFullPath($reference.path).StartsWith($coverageRoot, [StringComparison]::OrdinalIgnoreCase)) {
                throw 'Expert source coverage is outside its declared application/control/framework checkout.'
            }
            Assert-FileHash $reference.path $reference.sha256
        }
    }
    [void](Assert-RecommendationReport $Layered)
    Assert-FrozenScenario $Source
    Assert-OriginalSource $Source
    Assert-FileHash $Source.adeLauncher $Source.adeLauncherSha256
    if ((Get-ProfilerHash $Source.profilerCli) -cne $Source.profilerSha256 -or
        (Get-ProfilerHash $Source.nativeMcp.path) -cne $Source.nativeMcp.deploymentSha256 -or
        (Get-SourceHash $Source.nativeMcp.source $Source.nativeMcp.files) -cne $Source.nativeMcp.sourceSha256) {
        throw 'The original profiler or frozen read-only MCP closure changed.'
    }
}

$Directory = [IO.Path]::GetFullPath($Directory)
$contextPath = Join-Path $Directory 'context.json'
if ($Phase -ne 'Prepare') {
    $saved = Read-JsonFile $contextPath
    if ($saved.schema -ne 'ui-profiler/recommendation-trials/v1') { throw 'A standalone recommendation-trials context is required.' }
    $expectedModule = Join-Path $Directory 'tools\NativeTrials.psm1'
    if ([IO.Path]::GetFullPath($saved.directory) -ne $Directory -or $saved.module.path -ne $expectedModule) {
        throw 'The frozen recommendation runner belongs to another output directory.'
    }
    Assert-FileHash $saved.module.path $saved.module.sha256
    Import-Module $saved.module.path -Force -DisableNameChecking
    Invoke-RecommendationTrials -Phase $Phase -Directory $Directory -RecommendationId $RecommendationId `
        -GitHubAccount $GitHubAccount -PublishDraftPullRequests:$PublishDraftPullRequests
    return
}
if ($PublishDraftPullRequests) {
    throw 'Draft publication requires exact approval and a verified measured comparison; it is not permitted during Prepare.'
}
# Reject duplicate selection before reading inputs, making directories or contacting ADE.
$unique = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($id in $RecommendationId) {
    if (!$unique.Add($id)) { throw 'Recommendation ids must be unique; no trial has been started.' }
}
if (!$SourceInvestigation) { throw 'Prepare requires -SourceInvestigation with a saved read-only three-level report.' }
if (Test-Path -LiteralPath $Directory) { throw 'Use a fresh short output directory; never reset a partial or spent trial.' }
if ($Directory.Length -gt 80) { throw 'Use a short local output directory (at most 80 characters) for native builds.' }
$layeredPath = Join-Path ([IO.Path]::GetFullPath($SourceInvestigation)) 'context.json'
$layeredReference = @{ path = $layeredPath; sha256 = Get-Sha256 $layeredPath }
$layered = Read-JsonFile $layeredPath
if ([IO.Path]::GetFullPath($layered.directory) -ne [IO.Path]::GetFullPath($SourceInvestigation)) {
    throw 'The saved report belongs to a different investigation directory.'
}
Assert-FileHash $layered.sourceInvestigation.path $layered.sourceInvestigation.sha256
$source = Read-JsonFile $layered.sourceInvestigation.path
if ([IO.Path]::GetFullPath($layered.sourceInvestigation.path) -ne (Join-Path $source.directory 'context.json')) {
    throw 'The original source context belongs to a different investigation directory.'
}
$request = Read-JsonFile $source.request.path
$expert = Read-JsonFile $layered.provenance.path
$recommendations = Read-JsonFile $layered.recommendations.path
Assert-PreparationBindings $layered $source $expert $request
$applicationFiles = @(Get-RepositoryFiles $source.originalRepository)
$applicationSource = Join-Path $source.directory 'baseline-source'
if ((Get-SourceHash $applicationSource $applicationFiles) -cne $source.originalSourceSha256) {
    throw 'The immutable application baseline source no longer matches the original measured source.'
}
$knowledge = Get-PreparationKnowledgeSource $layered $request
$nativeFiles = @($knowledge.files)
$excludedSampleSubmodules = @($knowledge.excludedSampleSubmodules)
$nativeHash = $knowledge.sourceSha256
$nativeRevision = $knowledge.revision
$selected = @(Get-PreparationSelection $recommendations $RecommendationId $applicationFiles $nativeFiles)
$inputRoots = @($plugin, $layered.directory, $source.directory, $source.originalRepository,
    $request.knowledgeRepository, $knowledge.path, $request.expertHome, (Split-Path $source.profilerCli -Parent),
    (Split-Path $source.dotnet -Parent), (Split-Path $source.adeLauncher -Parent),
    (Split-Path $source.scenarioPath -Parent), (Split-Path $source.scenarioScript -Parent),
    (Split-Path $source.analysis.path -Parent), (Split-Path $source.baseline.path -Parent),
    $source.nativeMcp.source, (Split-Path $source.nativeMcp.path -Parent))
Assert-RunDirectory $Directory $inputRoots
if ($layered.PSObject.Properties['supplementalEvidence'] -and $layered.supplementalEvidence) {
    Assert-RunDirectory $Directory @((Split-Path $layered.supplementalEvidence.path -Parent))
}
foreach ($name in @('NativeTrials.psm1', 'NativeRuntime.psm1', 'Measure-NativeTrial.ps1')) {
    if (!(Test-Path -LiteralPath (Join-Path $plugin "pack\tools\$name") -PathType Leaf)) {
        throw "The current native recommendation implementation is incomplete: $name"
    }
}
New-Item -ItemType Directory $Directory | Out-Null
foreach ($name in @('tools', 'knowledge', 'skills')) {
    Copy-Item -LiteralPath (Join-Path $plugin "pack\$name") -Destination (Join-Path $Directory $name) -Recurse
}
Copy-Item -LiteralPath (Join-Path $plugin 'pack\ade.team.json') -Destination $Directory
New-PreparationAdeLauncher $source.adeLauncher $source.adeLauncherSha256 (Join-Path $Directory 'scripts')
$probe = $source | ConvertTo-Json -Depth 60 | ConvertFrom-Json
$probe.directory = $Directory
$probe.adeLauncher = Join-Path $Directory 'scripts\ade.ps1'
$probe.adeLauncherSha256 = Get-Sha256 $probe.adeLauncher
if ($GitHubAccount) { $probe.githubAccount = $GitHubAccount }
# SDK/authentication preflight launches no coding agent and precedes the expensive native source copy.
[void](Invoke-AdeRuntime @() $probe -Probe)
$frozenContracts = Join-Path $Directory 'tools\harness-contracts.md'
Copy-Item -LiteralPath (Join-Path $plugin 'CONTRACTS.md') -Destination $frozenContracts
$requestReference = New-PreparationRequestBinding $request $source.request $knowledge $Directory
$manifest = $source | ConvertTo-Json -Depth 60 | ConvertFrom-Json -AsHashtable
foreach ($key in @('state', 'preview', 'adapter', 'report', 'expert', 'recommendations')) { $manifest.Remove($key) }
$manifest.schema = 'ui-profiler/recommendation-trials/v1'
$manifest.directory = $Directory
$manifest.sourceInvestigation = $layeredReference
$manifest.originalInvestigation = $layered.sourceInvestigation
$manifest.request = $requestReference
if ($requestReference.path -ne $source.request.path) { $manifest.originalRequest = $source.request }
$manifest.knowledgeRepository = $knowledge
$manifest.contracts = @{ path = $frozenContracts; sha256 = Get-Sha256 $frozenContracts }
$manifest.sourceContracts = $layered.contracts
$manifest.recommendations = $layered.recommendations
$manifest.expert = $layered.provenance
$controlMetadata = Get-PreparationControlMetadata $layered
foreach ($key in $controlMetadata.Keys) { $manifest[$key] = $controlMetadata[$key] }
$manifest.analysis = $source.analysis
$manifest.adeLauncher = $probe.adeLauncher
$manifest.adeLauncherSha256 = $probe.adeLauncherSha256
$manifest.module = @{ path = (Join-Path $Directory 'tools\NativeTrials.psm1')
    sha256 = Get-Sha256 (Join-Path $Directory 'tools\NativeTrials.psm1') }
$manifest.implementation = @{
    harness = @{ path = (Join-Path $Directory 'tools\ProfilerHarness.psm1')
        sha256 = Get-Sha256 (Join-Path $Directory 'tools\ProfilerHarness.psm1') }
    runtime = @{ path = (Join-Path $Directory 'tools\NativeRuntime.psm1')
        sha256 = Get-Sha256 (Join-Path $Directory 'tools\NativeRuntime.psm1') }
}
$reportMetadata = Get-PreparationReportMetadata $recommendations
foreach ($key in $reportMetadata.Keys) { $manifest[$key] = $reportMetadata[$key] }
$packFiles = @(
    foreach ($name in @('tools', 'knowledge', 'skills', 'scripts')) {
        Get-ChildItem -LiteralPath (Join-Path $Directory $name) -File -Recurse |
            ForEach-Object { [IO.Path]::GetRelativePath($Directory, $_.FullName).Replace('\', '/') }
    }
    'ade.team.json'
)
$manifest.packSnapshot = @{ directory = $Directory; files = $packFiles; sha256 = Get-SourceHash $Directory $packFiles }
$manifest.rows = @((Read-JsonFile $layered.report.path).rows)
$manifest.trials = @()
$manifest.state = 'preparing'
$manifest.captures = 0
$manifest.editsAuthorized = $false
Write-JsonFile $contextPath $manifest
foreach ($entry in $selected) {
    Assert-FileHash $layeredPath $layeredReference.sha256
    Assert-PreparationBindings $layered $source $expert $request
    Assert-PreparationSource $knowledge.path $nativeRevision $nativeHash $excludedSampleSubmodules
    $trialDirectory = Join-Path (Join-Path $Directory 'trials') $entry.id
    $pack = Join-Path $trialDirectory 'pack'
    New-Item -ItemType Directory $pack -Force | Out-Null
    foreach ($name in @('knowledge', 'skills')) {
        Copy-Item -LiteralPath (Join-Path $Directory $name) -Destination $pack -Recurse
    }
    Copy-Item -LiteralPath (Join-Path $Directory 'tools') -Destination (Join-Path $pack 'tools') -Recurse
    Copy-Item -LiteralPath (Join-Path $Directory 'ade.team.json') -Destination $pack
    Copy-Item -LiteralPath $frozenContracts -Destination (Join-Path $pack 'knowledge\harness-contracts.md')
    $launcher = Join-Path $trialDirectory 'scripts\ade.ps1'
    Copy-Item -LiteralPath (Join-Path $Directory 'scripts') -Destination (Split-Path $launcher -Parent) -Recurse
    $targetRepository = $source.originalRepository
    $files = $applicationFiles
    $checkName = 'Measure.ps1'
    $nativeSnapshot = $null
    if ($entry.level -ne 'application') {
        $targetRepository = Join-Path $trialDirectory 'native-source'
        New-NativeSourceSnapshot $knowledge.path $targetRepository $nativeFiles $nativeRevision $nativeHash $excludedSampleSubmodules
        $files = $nativeFiles
        $checkName = 'Measure-NativeTrial.ps1'
        $nativeSnapshot = @{
            originalRepository = $knowledge.path
            originalRevision = $nativeRevision; originalSourceSha256 = $nativeHash
            source = $targetRepository; files = $nativeFiles; applicationSource = $applicationSource
            excludedSampleSubmodules = $excludedSampleSubmodules
        }
    }
    $trial = New-PreparationTrialContext $source $layered $layeredReference $entry `
        $Directory $trialDirectory $pack $GitHubAccount $nativeSnapshot $requestReference $knowledge
    $allowed = @($entry.files | ForEach-Object { $_.path.Replace('\', '/') })
    $trial.protectedPaths = @('.github', 'tests', 'test') +
        @($files | Where-Object { $_.Replace('\', '/') -cnotin $allowed } | ForEach-Object { $_.Replace('\', '/') })
    $trialContextPath = Join-Path $pack 'knowledge\trial-context.json'
    Write-JsonFile $trialContextPath $trial
    $trialContextHash = Get-Sha256 $trialContextPath
    $trial = Read-JsonFile $trialContextPath
    Write-JsonFile (Join-Path $pack 'mcp.json') (New-ProfilerMcpConfiguration $request.knowledgeRepository `
        $source.dotnet $source.analysis.path (Join-Path $source.directory 'pack\tools\Evidence-Mcp.py') $source.nativeMcp.path)
    $cfg = Read-JsonFile (Join-Path $pack 'ade.team.json')
    $quotedContext = $trialContextPath.Replace("'", "''")
    $quotedCheck = (Join-Path $pack "tools\$checkName").Replace("'", "''")
    $cfg.checks = @(@{
        id = if ($entry.level -eq 'application') { 'profile-scenario' } else { 'profile-native-scenario' }
        command = "& '$quotedCheck' -RepositoryPath (Get-Location).Path -ContextPath '$quotedContext' -ContextSha256 '$trialContextHash'"
        timeoutSeconds = if ($entry.level -eq 'application') { 900 } else { 3600 }
    })
    $cfg.protectedPaths = $trial.protectedPaths
    $cfg.permissions.shell = $false
    $cfg.permissions.urls = $false
    $cfg.limits.maxRounds = 1
    $cfg.limits.checkTimeoutSeconds = if ($entry.level -eq 'application') { 900 } else { 3600 }
    Write-JsonFile (Join-Path $pack 'ade.team.json') $cfg
    $workstreams = Join-Path $trialDirectory 'workstreams'
    $name = "trial-$($entry.id)"
    $goal = "Independently test $($entry.level) recommendation $($entry.id): $($entry.title). Exact-hash human approval is required before any coding or capture."
    $ws = Invoke-AdeRuntime @('workstream', 'new', '--pack', $pack, '--name', $name,
        '--repo', $targetRepository, '--type', 'change', '--goal', $goal, '--workstreams-dir', $workstreams) $trial
    $layerNotes = Get-PreparationLayerNotes $reportMetadata
    $notes = @"
# Immutable request, scenario and discovery evidence

$($request.scenario)
Original application: $($source.originalRepository) at $($source.originalRevision).
Scenario: $($source.scenarioPath), SHA-256 $($source.scenarioSha256).
Discovery baseline: $($source.baseline.path), SHA-256 $($source.baseline.sha256).
Analysis: $($source.analysis.path), SHA-256 $($source.analysis.sha256).
Source report context: $layeredPath, SHA-256 $($layeredReference.sha256).
Actual expert provenance: $($layered.provenance.path), SHA-256 $($layered.provenance.sha256).
Inspected hypothesis source: $($knowledge.path) at $nativeRevision, source SHA-256 $nativeHash.
This checkout is hypothesis source, NOT proof of the historical recording's runtime/source parity.
Read mandatory CURRENT contracts: $($trial.contracts.path), SHA-256 $($trial.contracts.sha256).
Use only the declared unchanged read-only winui-profiler and ui-profiler-evidence MCPs.
The frozen knowledge\intent-catalog.md guides NEW request/plan metadata only. Never retrofit intent
metadata into this existing frozen scenario, change its executable steps, or expand its capture budget.

# All three investigation layers

$layerNotes

# Complete selected actual expert entry

$($entry | ConvertTo-Json -Depth 30)

# Independent coding and validation constraints

Target repository: $targetRepository. This is a fresh standalone workstream and coding session.
Do not import another trial's patch, history or outcome. Modify ONLY the listed existing source files:
$($entry.files | ForEach-Object { "- $($_.path): $($_.change)" } | Out-String)
Preserve all content, functional behavior, scenario semantics and accessibility.
Do not edit tests, instructions, dependencies, project/build configuration or harness/profiler integration.
Do not build, launch, capture, invoke another agent or access shell/URLs in the coding session.
There is exactly one coding round, only after human approval of this proposal's exact hash.
Preparation MUST NOT code, build, launch, capture or automatically approve anything.
ADE baseline proposal checks verify source identity and frozen inputs only; no baseline capture.
Application trials reuse the original app baseline and permit one separately approved candidate capture.
Native trials require the correct control/framework target and complete matching private runtime/resources.
Native comparisons MUST use a SOURCE-MATCHED freshly built native baseline and candidate from the same
native source snapshot, unchanged application baseline source and identical frozen scenario/profiler.
Only sample Git submodules under Samples/ are excluded from the native source snapshot. Their exact
frozen Gitlink paths/revisions are recorded below; no runtime/build-input submodule may be excluded:
$($excludedSampleSubmodules | ConvertTo-Json -Depth 10)
The old recording is discovery evidence ONLY, never a native comparison baseline.
Native approval must explicitly authorize TWO captures (one fresh native baseline, one candidate)
and private native runtime replacement; old app recording consent is NOT this authorization.
Each baseline/candidate side runs the selected scenario ONCE; do not introduce three-run repetitions.
The native build/private-runtime check may take up to 3600 seconds. Fail closed on missing runtime closure
or source/runtime parity; never silently fall back to the installed runtime or replace global packages.
Never combine fixes, reuse a spent capture, relax a guard, or profile concurrently with another trial.
A passing check proves comparable functional evidence, not an improvement. Report regressions and missing
dimensions honestly; a single pair is observational, not statistical significance.
UI ticks are elapsed UI-thread time, not CPU work or presented frames; automation waits remain included.
Read root and scoped source instructions before editing; current project values govern framework versions.
"@
    foreach ($instruction in @('AGENTS.md', '.github/copilot-instructions.md')) {
        if ($instruction -in @($files | ForEach-Object { $_.Replace('\', '/') })) {
            $notes += "`n# Source instructions: $instruction`n" +
                (Get-Content -LiteralPath (Join-Path $targetRepository $instruction.Replace('/', '\')) -Raw)
        }
    }
    [IO.File]::WriteAllText((Join-Path $ws.workstream 'notes.md'), $notes)
    $result = Invoke-AdeRuntime @('propose', '--pack', $pack, '--repo', $targetRepository,
        '--type', 'change', '--task', $goal, '--workstream', $name, '--workstreams-dir', $workstreams,
        '--runs-dir', (Join-Path $trialDirectory 'ade-runs')) $trial
    if ($result.status -ne 'AwaitingApproval' -or $result.proposalSha256 -cnotmatch '^[a-f0-9]{64}$' -or
        [string]::IsNullOrWhiteSpace($result.runId) -or $result.runId -match '[/\\:]' -or $result.runId -in @('.', '..')) {
        throw 'ADE did not produce an approvable exact-hash proposal.'
    }
    Assert-FileHash $layeredPath $layeredReference.sha256
    Assert-FileHash $trialContextPath $trialContextHash
    Assert-PreparationBindings $layered $source $expert $request
    Assert-PreparationSource $knowledge.path $nativeRevision $nativeHash $excludedSampleSubmodules
    $proposalPath = Join-Path $trialDirectory 'ade-run.json'
    Write-JsonFile $proposalPath $result
    $reference = @{
        id = $entry.id; level = $entry.level; directory = $trialDirectory
        context = @{ path = $trialContextPath; sha256 = $trialContextHash }
        proposal = @{ path = $proposalPath; sha256 = Get-Sha256 $proposalPath }
        runId = $result.runId; proposalSha256 = $result.proposalSha256
    }
    $manifest.trials = @($manifest.trials) + @($reference)
    foreach ($row in $manifest.rows) {
        if ($row.id -ceq $entry.id) { $row.state = 'awaiting-approval'; $row.trial = $reference }
    }
    Write-JsonFile $contextPath $manifest
}
Assert-FileHash $layeredPath $layeredReference.sha256
Assert-FileHash $layered.sourceInvestigation.path $layered.sourceInvestigation.sha256
Assert-OriginalSource $source
Assert-PreparationSource $knowledge.path $nativeRevision $nativeHash $excludedSampleSubmodules
$manifest.state = 'awaiting-approval'
Write-JsonFile $contextPath $manifest
@{ status = 'AwaitingApproval'; directory = $Directory; captures = 0; editsAuthorized = $false
    trials = $manifest.trials; rows = $manifest.rows } | ConvertTo-Json -Depth 60
