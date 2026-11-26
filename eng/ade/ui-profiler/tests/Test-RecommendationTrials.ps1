# Offline preparation guards and byte-exact native snapshots; no ADE sessions, builds or captures.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$scriptPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\scripts\Start-RecommendationTrials.ps1'))
$tokens = $null
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
# Load only the production helper definitions, never the entry point or its real SDK preflight.
foreach ($function in $ast.FindAll({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
    Invoke-Expression $function.Extent.Text
}
$investigationPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\scripts\Start-Investigation.ps1'))
$investigationAst = [Management.Automation.Language.Parser]::ParseFile($investigationPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
foreach ($function in $investigationAst.FindAll({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
    Invoke-Expression $function.Extent.Text
}
function Assert-Rejected([scriptblock]$Action, [string]$Message) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true }
    if (!$rejected) { throw $Message }
}
function Write-FixtureFile([string]$Path, [string]$Text) {
    New-Item -ItemType Directory -Force (Split-Path $Path -Parent) | Out-Null
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}
function Get-FixtureReference([string]$Path) { @{ path = $Path; sha256 = Get-Sha256 $Path } }

$root = Join-Path (Get-Location).Path ('.recommendation-trials-tests-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $root | Out-Null
try {
    $recommendations = @{
        schema = 'ui-profiler/recommendations/v2'; summary = 'Synthetic independent layers.'
        coverage = @{ application = 'App source.'; control = 'Control source.'; framework = 'Framework source.' }
        runtimeSourceParity = 'Old runtime is discovery only.'; risks = @('Synthetic evidence.')
        recommendations = @(
            @{ id = 'app'; level = 'application'; title = 'App'; verdict = 'propose'; summary = 'Synthetic.'
                files = @(@{ path = 'Page.xaml'; change = 'A scoped change.' })
                observations = @('Fixture.'); hypotheses = @('Fixture.'); validation = @('Fixture.'); risks = @('Fixture.') },
            @{ id = 'control'; level = 'control'; title = 'Control'; verdict = 'recommend'; summary = 'Synthetic.'
                files = @(@{ path = 'controls\dev\Control.cpp'; change = 'A scoped change.' })
                observations = @('Fixture.'); hypotheses = @('Fixture.'); validation = @('Fixture.'); risks = @('Fixture.') },
            @{ id = 'framework'; level = 'framework'; title = 'Framework'; verdict = 'insufficient-evidence'; summary = 'Synthetic.'
                files = @(); observations = @('Fixture.'); hypotheses = @('Fixture.'); validation = @('Fixture.'); risks = @('Fixture.') }
        )
    }
    $appFiles = @('Page.xaml', 'Other.cs')
    $nativeFiles = @('controls/dev/Control.cpp', 'dxaml/xcp/Layout.cpp')
    $legacyMetadata = Get-PreparationReportMetadata $recommendations
    if ($legacyMetadata.ContainsKey('detailedAnalysis') -or
        (Get-PreparationLayerNotes $legacyMetadata) -match 'Detailed layer analysis') {
        throw 'A legacy v2 import fabricated detailed evidence.'
    }
    $currentRecommendations = $recommendations | ConvertTo-Json -Depth 40 | ConvertFrom-Json
    $currentRecommendations.schema = 'ui-profiler/recommendations/v3'
    $details = [ordered]@{}
    foreach ($level in @('application', 'control', 'framework')) {
        $details[$level] = [ordered]@{
            time = "$level observed elapsed time with source/evidence references."
            cpu = "$level observed CPU work, distinct from elapsed UI ticks."
            memory = "$level measured memory and attribution limits."
            modelAnalysis = "$level source-grounded model and unproven hypotheses."
            unverified = @("$level source/runtime parity remains unverified.")
            validation = @("$level one baseline run and one candidate run, no automatic repetition.")
        }
    }
    $currentRecommendations | Add-Member -NotePropertyName detailedAnalysis -NotePropertyValue $details
    $currentSelection = @(Get-PreparationSelection $currentRecommendations @() $appFiles $nativeFiles)
    if (($currentSelection.id -join ',') -cne 'app,control') {
        throw 'Current v3 imports changed independent actionability or omitted a native layer.'
    }
    Assert-Rejected { Get-PreparationSelection $currentRecommendations @('framework') $appFiles $nativeFiles } `
        'The v3 detailed analysis incorrectly made insufficient evidence actionable.'
    Assert-Rejected { Get-PreparationSelection $currentRecommendations @('control', 'CONTROL') $appFiles $nativeFiles } `
        'The v3 import bypassed duplicate-id rejection.'
    $currentMetadata = Get-PreparationReportMetadata $currentRecommendations
    $metadataPath = Join-Path $root 'current-report-metadata.json'
    Write-JsonFile $metadataPath $currentMetadata
    $savedMetadata = Read-JsonFile $metadataPath
    if (($savedMetadata.detailedAnalysis | ConvertTo-Json -Depth 40 -Compress) -cne
        ($currentRecommendations.detailedAnalysis | ConvertTo-Json -Depth 40 -Compress)) {
        throw 'Manifest metadata dropped or reshaped the original expert detailed analysis.'
    }
    $currentNotes = Get-PreparationLayerNotes $currentMetadata
    foreach ($level in @('application', 'control', 'framework')) {
        foreach ($field in @('time', 'cpu', 'memory', 'modelAnalysis', 'unverified', 'validation')) {
            foreach ($value in @($details[$level][$field])) {
                if (!$currentNotes.Contains($value)) { throw "Proposal notes omitted $level/$field detailed evidence." }
            }
        }
    }
    $currentRecommendations.schema = 'ui-profiler/recommendations/v4'
    Assert-Rejected { Get-PreparationSelection $currentRecommendations @() $appFiles $nativeFiles } `
        'An unsupported future recommendation schema was silently imported.'
    $selection = @(Get-PreparationSelection $recommendations @() $appFiles $nativeFiles)
    if (($selection.id -join ',') -cne 'app,control') { throw 'Default selection must retain all actionable layers, not insufficient evidence.' }
    $selection = @(Get-PreparationSelection $recommendations @('control', 'app') $appFiles $nativeFiles)
    if (($selection.id -join ',') -cne 'control,app') { throw 'Independent native/application selection lost the requested order.' }
    foreach ($ids in @(@('control', 'CONTROL'), @('framework'), @('unknown'), @('APP'))) {
        Assert-Rejected { Get-PreparationSelection $recommendations $ids $appFiles $nativeFiles } 'Invalid selection was accepted.'
    }
    $notCreated = Join-Path $root 'never-created'
    Assert-Rejected { & $scriptPath -Directory $notCreated -SourceInvestigation (Join-Path $root 'missing') -RecommendationId @('app', 'APP') } `
        'The entry point accepted duplicate ids.'
    if (Test-Path -LiteralPath $notCreated) { throw 'Rejected selection created output.' }
    $publicationError = $null
    try { & $scriptPath -Directory $notCreated -PublishDraftPullRequests } catch { $publicationError = $_.Exception.Message }
    if ($publicationError -notmatch 'publication requires exact approval and a verified measured comparison' -or
        (Test-Path -LiteralPath $notCreated)) {
        throw 'Prepare accepted draft publication or created output before rejecting it.'
    }
    $prepareError = $null
    try { & $scriptPath -Directory $notCreated -PublishDraftPullRequests:$false } catch { $prepareError = $_.Exception.Message }
    if ($prepareError -notmatch 'Prepare requires -SourceInvestigation') {
        throw 'Explicitly disabling draft publication changed the normal Prepare input guards.'
    }
    $originalPath = $recommendations.recommendations[1].files[0].path
    foreach ($path in @('..\controls\dev\Control.cpp', 'dxaml\xcp\Layout.cpp',
        'controls\dev\test\Control.cpp', 'controls\dev\Control.vcxproj', 'controls\dev\Missing.cpp')) {
        $recommendations.recommendations[1].files[0].path = $path
        Assert-Rejected { Get-PreparationSelection $recommendations @('control') $appFiles $nativeFiles } `
            'An escaping, wrong-layer, absent or non-source scope was accepted.'
    }
    $recommendations.recommendations[1].files[0].path = $originalPath
    foreach ($suffix in @('cpp', 'h', 'hpp', 'inl', 'idl', 'xaml', 'cs')) {
        $path = "controls/dev/Control.$suffix"
        $recommendations.recommendations[1].files[0].path = $path
        $accepted = @(Get-PreparationSelection $recommendations @('control') $appFiles @($nativeFiles + $path))
        if ($accepted.Count -ne 1) { throw "Supported native source suffix was rejected: $suffix" }
    }
    foreach ($suffix in @('cxx', 'cc', 'c', 'hxx')) {
        $path = "controls/dev/Control.$suffix"
        $recommendations.recommendations[1].files[0].path = $path
        Assert-Rejected { Get-PreparationSelection $recommendations @('control') $appFiles @($nativeFiles + $path) } `
            "Unsupported native source suffix was accepted: $suffix"
    }
    $recommendations.recommendations[1].files[0].path = $originalPath
    $recommendations.recommendations[1].files += @(@{ path = $originalPath.Replace('\', '/'); change = 'Duplicate.' })
    Assert-Rejected { Get-PreparationSelection $recommendations @('control') $appFiles $nativeFiles } 'Duplicate file aliases were accepted.'
    $recommendations.recommendations[1].files = @($recommendations.recommendations[1].files[0])

    $repository = Join-Path $root 'original'
    Write-FixtureFile (Join-Path $repository '.gitignore') "ignored.cpp`n"
    Write-FixtureFile (Join-Path $repository '.gitattributes') "* text=auto`n"
    Write-FixtureFile (Join-Path $repository 'ignored.cpp') "tracked despite ignore`r`n"
    Write-FixtureFile (Join-Path $repository 'controls\dev\Control.cpp') "original`r`n"
    Write-FixtureFile (Join-Path $repository 'dxaml\xcp\Layout.cpp') "layout`n"
    $longRelative = 'controls/dev/' + (('long-native-source-' * 4) + '/') * 3 + 'LongGeneratedSource.cpp'
    $longPath = Join-Path $repository $longRelative.Replace('/', '\')
    if ($longPath.Length -le 260) { throw 'The native path regression fixture must exceed MAX_PATH.' }
    Write-FixtureFile $longPath "byte-exact long native source`r`n"
    & git -C $repository -c init.templateDir= init --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Fixture git init failed.' }
    & git -C $repository config --local core.longpaths false
    & git -C $repository -c core.longpaths=true -c core.autocrlf=false add --force --all 2>$null
    if ($LASTEXITCODE -ne 0) { throw 'Fixture git add failed.' }
    $hooks = Join-Path $root 'disabled-hooks'
    New-Item -ItemType Directory $hooks | Out-Null
    & git -C $repository -c user.name=Copilot -c user.email=223556219+Copilot@users.noreply.github.com `
        -c core.longpaths=true -c "core.hooksPath=$hooks" -c commit.gpgsign=false commit --quiet -m 'Synthetic native snapshot fixture' `
        -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Fixture commit failed.' }
    Write-FixtureFile (Join-Path $repository 'controls\dev\Control.cpp') "dirty harness source`r`n"
    Write-FixtureFile (Join-Path $repository 'untracked ü.cpp') "untracked current source`n"
    $longUntrackedRelative = $longRelative.Replace('LongGeneratedSource.cpp', 'UntrackedGeneratedSource.cpp')
    Write-FixtureFile (Join-Path $repository $longUntrackedRelative.Replace('/', '\')) "byte-exact long untracked source`n"
    $files = @(Get-PreparationNativeRepositoryFiles $repository)
    if ($longUntrackedRelative -cnotin $files) { throw 'Long untracked native files were silently omitted during source enumeration.' }
    $hash = Get-SourceHash $repository $files
    $revision = git -C $repository rev-parse HEAD
    $beforeStatus = @(git -C $repository -c core.longpaths=true -c core.quotepath=false status --porcelain --untracked-files=all)
    $snapshot = Join-Path $root 'snapshot'
    New-NativeSourceSnapshot $repository $snapshot $files $revision $hash
    if ((Get-SourceHash $snapshot) -cne $hash -or
        !(Test-Path -LiteralPath (Join-Path $snapshot 'ignored.cpp')) -or
        !(Test-Path -LiteralPath (Join-Path $snapshot 'untracked ü.cpp')) -or
        !(Test-Path -LiteralPath (Join-Path $snapshot $longRelative.Replace('/', '\'))) -or
        !(Test-Path -LiteralPath (Join-Path $snapshot $longUntrackedRelative.Replace('/', '\'))) -or
        (git -C $snapshot config --local --get core.longpaths) -cne 'true' -or
        (git -C $repository config --local --get core.longpaths) -cne 'false') {
        throw 'Snapshot lost dirty, tracked-ignored or untracked source bytes.'
    }
    $afterStatus = @(git -C $repository -c core.longpaths=true -c core.quotepath=false status --porcelain --untracked-files=all)
    if (($beforeStatus -join "`n") -cne ($afterStatus -join "`n")) { throw 'Snapshotting mutated the original index or worktree.' }
    Assert-Rejected { New-NativeSourceSnapshot $repository $snapshot $files $revision $hash } 'A snapshot was silently overwritten.'
    Write-FixtureFile (Join-Path $repository 'extra.cpp') 'changed file set'
    Assert-Rejected { Assert-PreparationSource $repository $revision $hash } 'A changed original file set was not detected.'
    Remove-Item -LiteralPath (Join-Path $repository 'extra.cpp')

    $sampleRevision = $revision
    & git -C $repository update-index --add --cacheinfo "160000,$sampleRevision,Samples/WinUIGallery"
    if ($LASTEXITCODE -ne 0) { throw 'Fixture sample Gitlink creation failed.' }
    New-Item -ItemType Directory -Force (Join-Path $repository 'Samples\WinUIGallery') | Out-Null
    & git -C $repository -c user.name=Copilot -c user.email=223556219+Copilot@users.noreply.github.com `
        -c core.longpaths=true -c "core.hooksPath=$hooks" -c commit.gpgsign=false commit --quiet -m 'Synthetic excluded sample submodule fixture' `
        -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Fixture sample Gitlink commit failed.' }
    $revision = git -C $repository rev-parse HEAD
    $files = @(Get-PreparationNativeRepositoryFiles $repository)
    $sampleReferences = @(Get-NativeSampleSubmodules $repository)
    if ('Samples/WinUIGallery' -in $files -or $sampleReferences.Count -ne 1 -or
        $sampleReferences[0].path -cne 'Samples/WinUIGallery' -or
        $sampleReferences[0].revision -cne $sampleRevision -or (Get-SourceHash $repository $files) -cne $hash) {
        throw 'Only the sample Gitlink must be excluded, with its exact frozen path and revision retained.'
    }
    $beforeStatus = @(git -C $repository -c core.longpaths=true -c core.quotepath=false status --porcelain --untracked-files=all)
    $snapshot = Join-Path $root 'sample-snapshot'
    New-NativeSourceSnapshot $repository $snapshot $files $revision $hash $sampleReferences
    $afterStatus = @(git -C $repository -c core.longpaths=true -c core.quotepath=false status --porcelain --untracked-files=all)
    if ((Test-Path -LiteralPath (Join-Path $snapshot 'Samples\WinUIGallery')) -or
        (Get-SourceHash $snapshot) -cne $hash -or
        ($beforeStatus -join "`n") -cne ($afterStatus -join "`n")) {
        throw 'Sample exclusion changed source bytes, copied a Gitlink, or mutated the original checkout.'
    }
    & git -C $repository update-index --cacheinfo "160000,$revision,Samples/WinUIGallery"
    Assert-Rejected { Assert-PreparationSource $repository $revision $hash $sampleReferences } `
        'An excluded sample Gitlink revision change escaped the preparation guard.'
    & git -C $repository update-index --cacheinfo "160000,$sampleRevision,Samples/WinUIGallery"
    & git -C $repository update-index --add --cacheinfo "160000,$sampleRevision,Build/Runtime"
    Assert-Rejected { Get-NativeRepositoryFiles $repository } 'A runtime/build-input Gitlink was silently excluded.'
    & git -C $repository update-index --force-remove -- Build/Runtime
    Assert-PreparationSource $repository $revision $hash $sampleReferences

    $gitForwarder = Join-Path $root 'git-forwarder.ps1'
    Write-FixtureFile $gitForwarder '& git @args; exit $LASTEXITCODE'
    $launcherDirectory = Join-Path $root 'scoped-launcher'
    New-PreparationAdeLauncher $gitForwarder (Get-Sha256 $gitForwarder) $launcherDirectory
    $launcher = Join-Path $launcherDirectory 'ade.ps1'
    $gitEnvironmentNames = @('GIT_CONFIG_COUNT', 'GIT_CONFIG_KEY_0', 'GIT_CONFIG_VALUE_0', 'GIT_CONFIG_KEY_1', 'GIT_CONFIG_VALUE_1')
    $previousGitEnvironment = @{}
    foreach ($name in $gitEnvironmentNames) { $previousGitEnvironment[$name] = [Environment]::GetEnvironmentVariable($name) }
    try {
        $inheritedGitEnvironment = @{
            GIT_CONFIG_COUNT = '1'; GIT_CONFIG_KEY_0 = 'core.longpaths'; GIT_CONFIG_VALUE_0 = 'false'
            GIT_CONFIG_KEY_1 = 'unused-existing-key'; GIT_CONFIG_VALUE_1 = 'preserve-this-value'
        }
        foreach ($name in $gitEnvironmentNames) {
            [Environment]::SetEnvironmentVariable($name, $inheritedGitEnvironment[$name])
        }
        # ADE's installed Propose copies files, then runs Git init/add/commit; no ADE command is executed here.
        $workcopy = Join-Path $root 'downstream-workcopy'
        Copy-ApplicationSource $snapshot $workcopy $files
        & $launcher -C $workcopy init --quiet
        if ($LASTEXITCODE -ne 0) { throw 'Scoped downstream workcopy initialization failed.' }
        & $launcher -C $workcopy -c core.autocrlf=false add --force --all 2>$null
        if ($LASTEXITCODE -ne 0) { throw 'Scoped downstream Git could not index long native source paths.' }
        & $launcher -C $workcopy -c user.name=Copilot -c user.email=223556219+Copilot@users.noreply.github.com `
            -c "core.hooksPath=$hooks" -c commit.gpgsign=false commit --quiet -m 'Synthetic downstream long-path workcopy fixture' `
            -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
        if ($LASTEXITCODE -ne 0) { throw 'Scoped downstream workcopy commit failed.' }
        $workcopyStatus = @(& $launcher -C $workcopy status --porcelain --untracked-files=all)
        if ($LASTEXITCODE -ne 0 -or $workcopyStatus.Count -or (Get-SourceHash $workcopy $files) -cne $hash -or
            !(Test-Path -LiteralPath (Join-Path $workcopy $longRelative.Replace('/', '\')))) {
            throw 'The downstream native workcopy dropped long paths, changed source bytes, or was not clean.'
        }
        foreach ($name in $gitEnvironmentNames) {
            if ([Environment]::GetEnvironmentVariable($name) -cne $inheritedGitEnvironment[$name]) {
                throw "The scoped ADE launcher leaked Git environment changes: $name"
            }
        }
    } finally {
        foreach ($name in $gitEnvironmentNames) {
            [Environment]::SetEnvironmentVariable($name, $previousGitEnvironment[$name])
        }
    }

    $app = Join-Path $root 'app'
    Write-FixtureFile (Join-Path $app 'Page.xaml') 'application fixture'
    & git -C $app -c init.templateDir= init --quiet
    & git -C $app add --all
    & git -C $app -c user.name=Copilot -c user.email=223556219+Copilot@users.noreply.github.com `
        -c "core.hooksPath=$hooks" -c commit.gpgsign=false commit --quiet -m 'Synthetic immutable application fixture' `
        -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Fixture app commit failed.' }
    $appBaselineSource = Join-Path $root 'baseline-source'
    Copy-ApplicationSource $app $appBaselineSource @('Page.xaml')
    $reportDirectory = Join-Path $root 'report'
    Write-FixtureFile (Join-Path $reportDirectory 'expert\recommendation.md') 'Actual synthetic recommendation.'
    $recommendationPath = Join-Path $reportDirectory 'expert\recommendation.json'
    Write-JsonFile $recommendationPath $recommendations
    $requestPath = Join-Path $root 'request.json'
    $request = @{
        schema = 'ui-profiler/request/v2'; knowledgeRepository = $repository; applicationRepository = $app
        scenario = 'Original immutable scenario.'; allowMeasurements = $true; strategy = 'independent'
        candidatePolicy = 'one-per-approved-recommendation'
    }
    Write-JsonFile $requestPath $request
    foreach ($name in @('scenario.json', 'scenario.ps1', 'contracts.md', 'analysis.json', 'measurement.json',
        'old-context.json', 'profiler\cli.exe', 'mcp\native.dll', 'mcp-source\source.cs')) {
        Write-FixtureFile (Join-Path $root $name) "immutable synthetic $name"
    }
    $databasePath = Join-Path $root 'recording.sqlite'
    Write-FixtureFile $databasePath 'Synthetic original recording.'
    Write-JsonFile (Join-Path $root 'measurement.json') @{ database = Get-FixtureReference $databasePath }
    $source = [pscustomobject]@{
        schema = 'ui-profiler/investigation/v3'; directory = $root; strategy = 'independent'; githubAccount = 'original-account'
        originalRepository = $app; originalSourceSha256 = Get-SourceHash $app
        originalRevision = (git -C $app rev-parse HEAD)
        request = Get-FixtureReference $requestPath
        contracts = Get-FixtureReference (Join-Path $root 'contracts.md')
        module = Get-FixtureReference "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1"
        baseline = Get-FixtureReference (Join-Path $root 'measurement.json')
        analysis = Get-FixtureReference (Join-Path $root 'analysis.json')
        scenarioPath = Join-Path $root 'scenario.json'; scenarioSha256 = Get-Sha256 (Join-Path $root 'scenario.json')
        scenarioScript = Join-Path $root 'scenario.ps1'; scenarioScriptSha256 = Get-Sha256 (Join-Path $root 'scenario.ps1')
        profilerCli = Join-Path $root 'profiler\cli.exe'; profilerSha256 = Get-ProfilerHash (Join-Path $root 'profiler\cli.exe')
        adeLauncher = $scriptPath; adeLauncherSha256 = Get-Sha256 $scriptPath
        nativeMcp = @{
            path = Join-Path $root 'mcp\native.dll'; sha256 = Get-Sha256 (Join-Path $root 'mcp\native.dll')
            deploymentSha256 = Get-ProfilerHash (Join-Path $root 'mcp\native.dll')
            source = Join-Path $root 'mcp-source'; files = @('source.cs')
            sourceSha256 = Get-SourceHash (Join-Path $root 'mcp-source') @('source.cs')
        }
    }
    $expert = [pscustomobject]@{
        agent = 'winui-expert'; measurementSha256 = $source.baseline.sha256; analysisSha256 = $source.analysis.sha256
        scenarioSha256 = $source.scenarioSha256; sourceRepository = $repository
        structuredRecommendation = Get-FixtureReference $recommendationPath
        recommendationSha256 = Get-Sha256 (Join-Path $reportDirectory 'expert\recommendation.md')
        mcpQueries = @(@{ server = 'winui-profiler' }, @{ server = 'ui-profiler-evidence' })
        investigationLevels = @('application', 'control', 'framework')
        sourceCoverage = @{
            application = @(Get-FixtureReference (Join-Path $appBaselineSource 'Page.xaml'))
            control = @(Get-FixtureReference (Join-Path $repository 'controls\dev\Control.cpp'))
            framework = @(Get-FixtureReference (Join-Path $repository 'dxaml\xcp\Layout.cpp'))
        }
    }
    $provenancePath = Join-Path $reportDirectory 'expert\provenance.json'
    Write-JsonFile $provenancePath $expert
    $layered = [pscustomobject]@{
        schema = 'ui-profiler/read-only-investigation/v1'; directory = $reportDirectory
        sourceInvestigation = Get-FixtureReference (Join-Path $root 'old-context.json')
        recommendations = Get-FixtureReference $recommendationPath
        provenance = Get-FixtureReference $provenancePath
        originalRevision = $source.originalRevision; baseline = $source.baseline; analysis = $source.analysis
        trials = @(); report = $null
    }
    $report = Write-RecommendationReport $layered
    $layered.report = @{ path = $report.path; sha256 = $report.sha256 }
    Assert-PreparationBindings $layered $source $expert $request
    $trialDirectory = Join-Path $root 'prepared-trial'
    $pack = Join-Path $trialDirectory 'pack'
    foreach ($name in @('ProfilerHarness.psm1', 'NativeTrials.psm1', 'NativeRuntime.psm1', 'Measure-NativeTrial.ps1')) {
        $destination = Join-Path $pack "tools\$name"
        New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "..\pack\tools\$name") -Destination $destination
    }
    Write-FixtureFile (Join-Path $pack 'knowledge\harness-contracts.md') 'Current frozen trial contracts.'
    Write-FixtureFile (Join-Path $trialDirectory 'scripts\ade.ps1') 'Pinned trial launcher fixture.'
    $legacyTrial = New-PreparationTrialContext $source $layered (Get-FixtureReference (Join-Path $root 'old-context.json')) `
        $recommendations.recommendations[0] $root $trialDirectory $pack ''
    if ($legacyTrial.ContainsKey('targetControl') -or $legacyTrial.ContainsKey('controlAnalysis') -or
        (Get-PreparationControlMetadata $layered).Count -ne 0) {
        throw 'A legacy investigation acquired an inferred/default control target or analysis.'
    }
    $controlAnalysisPath = Join-Path $reportDirectory 'control-analysis.json'
    Write-JsonFile $controlAnalysisPath @{ targetControl = 'FixtureControl'; limitations = @('Synthetic capture-local evidence.') }
    $layered | Add-Member -NotePropertyName targetControl -NotePropertyValue 'FixtureControl'
    $layered | Add-Member -NotePropertyName controlAnalysis -NotePropertyValue (Get-FixtureReference $controlAnalysisPath)
    $controlMetadata = Get-PreparationControlMetadata $layered
    $metadataPath = Join-Path $root 'control-metadata.json'
    Write-JsonFile $metadataPath $controlMetadata
    $savedMetadata = Read-JsonFile $metadataPath
    if ($savedMetadata.targetControl -cne 'FixtureControl' -or
        $savedMetadata.controlAnalysis.path -cne $layered.controlAnalysis.path -or
        $savedMetadata.controlAnalysis.sha256 -cne $layered.controlAnalysis.sha256) {
        throw 'Manifest metadata lost the explicitly selected generic control or its frozen analysis reference.'
    }
    $layeredPath = Join-Path $reportDirectory 'context.json'
    Write-JsonFile $layeredPath $layered
    $layeredReference = Get-FixtureReference $layeredPath
    $nativeSnapshot = @{
        originalRepository = $repository; originalRevision = $revision; originalSourceSha256 = $hash
        source = $snapshot; files = $files; applicationSource = $appBaselineSource
        excludedSampleSubmodules = $sampleReferences
    }
    $frameworkEntry = $recommendations.recommendations[1] | ConvertTo-Json -Depth 20 | ConvertFrom-Json
    $frameworkEntry.id = 'framework-action'
    $frameworkEntry.level = 'framework'
    $frameworkEntry.files = @(@{ path = 'dxaml/xcp/Layout.cpp'; change = 'Scoped framework fixture.' })
    foreach ($entry in @($recommendations.recommendations[1], $frameworkEntry)) {
        $trial = New-PreparationTrialContext $source $layered $layeredReference $entry `
            $root $trialDirectory $pack 'selected-account' $nativeSnapshot
        $trialContextPath = Join-Path $pack 'knowledge\trial-context.json'
        Write-JsonFile $trialContextPath $trial
        $trial = Read-JsonFile $trialContextPath
        if ($trial.schema -cne 'ui-profiler/native-trial/v1' -or $trial.strategy -cne 'independent' -or
            $trial.originalRepository -cne $app -or $trial.originalRevision -cne $source.originalRevision -or
            $trial.originalSourceSha256 -cne $source.originalSourceSha256 -or $trial.recommendationId -cne $entry.id -or
            $trial.native.level -cne $entry.level -or $trial.native.originalRepository -cne $repository -or
            $trial.native.originalRevision -cne $revision -or $trial.native.originalSourceSha256 -cne $hash -or
            $trial.native.source -cne $snapshot -or $trial.native.sourceSha256 -cne $hash -or
            ($trial.native.files -join "`0") -cne ($files -join "`0") -or
            @($trial.native.excludedSampleSubmodules).Count -ne 1 -or
            $trial.native.excludedSampleSubmodules[0].path -cne 'Samples/WinUIGallery' -or
            $trial.native.excludedSampleSubmodules[0].revision -cne $sampleRevision -or
            $trial.native.applicationSource -cne $appBaselineSource -or
            $trial.native.applicationSourceSha256 -cne $source.originalSourceSha256 -or
            ($trial.native.allowedPaths -join "`0") -cne ($entry.files.path -join "`0") -or
            ($trial.allowedPaths -join "`0") -cne ($entry.files.path -join "`0")) {
            throw 'Generated native trial lost source identity, original app identity, layer or approved scope.'
        }
        if ($trial.native.authorization.baselineCaptures -ne 1 -or $trial.native.authorization.candidateCaptures -ne 1 -or
            $trial.native.authorization.nativeRuntimeChanges -cne $true -or
            $trial.native.comparisonPolicy -notmatch 'Source-matched freshly built native baseline' -or
            $trial.native.comparisonPolicy -notmatch 'old recording is discovery evidence only' -or
            $trial.baseline.sha256 -cne $source.baseline.sha256 -or
            $trial.request.sha256 -cne $source.request.sha256 -or $trial.githubAccount -cne 'selected-account') {
            throw 'Generated native trial lost its explicit new pair budget, discovery-only baseline or inherited request.'
        }
        foreach ($reference in @($trial.native.implementation.module, $trial.native.implementation.runtime,
            $trial.native.implementation.measure, $trial.module, $trial.contracts, $trial.sourceInvestigation,
            $trial.originalInvestigation, $trial.recommendations, $trial.expert, $trial.controlAnalysis)) {
            Assert-FileHash $reference.path $reference.sha256
        }
        if ($trial.native.implementation.module.path -cne (Join-Path $pack 'tools\NativeTrials.psm1') -or
            $trial.native.implementation.runtime.path -cne (Join-Path $pack 'tools\NativeRuntime.psm1') -or
            $trial.native.implementation.measure.path -cne (Join-Path $pack 'tools\Measure-NativeTrial.ps1') -or
            $trial.module.path -cne (Join-Path $pack 'tools\ProfilerHarness.psm1') -or
            $trial.sourceInvestigation.sha256 -cne $layeredReference.sha256 -or
            $trial.recommendations.sha256 -cne $layered.recommendations.sha256 -or
            $trial.expert.sha256 -cne $layered.provenance.sha256 -or $source.githubAccount -cne 'original-account' -or
            $trial.targetControl -cne 'FixtureControl' -or $trial.controlAnalysis.path -cne $layered.controlAnalysis.path -or
            $trial.controlAnalysis.sha256 -cne $layered.controlAnalysis.sha256) {
            throw 'Generated native context confused implementation modules, layered provenance or account isolation.'
        }
        Assert-FileHash $trial.adeLauncher $trial.adeLauncherSha256
    }
    Assert-Rejected {
        New-PreparationTrialContext $source $layered $layeredReference $frameworkEntry $root $trialDirectory $pack ''
    } 'A native trial was generated without a private native source snapshot.'
    $applicationTrial = New-PreparationTrialContext $source $layered $layeredReference $recommendations.recommendations[0] `
        $root $trialDirectory $pack ''
    if ($applicationTrial.schema -cne 'ui-profiler/trial/v1' -or $applicationTrial.ContainsKey('native') -or
        $applicationTrial.githubAccount -cne 'original-account' -or $applicationTrial.targetControl -cne 'FixtureControl' -or
        $applicationTrial.controlAnalysis.sha256 -cne $layered.controlAnalysis.sha256) {
        throw 'Application trial generation inherited a native capture budget or lost the original account.'
    }
    $legacyKnowledge = Resolve-InvestigationKnowledgeSource $request ''
    $legacyPreparedKnowledge = Get-PreparationKnowledgeSource $layered $request
    if ($legacyKnowledge.path -cne $repository -or $legacyPreparedKnowledge.path -cne $repository) {
        throw 'A legacy investigation without an override changed its source checkout.'
    }
    $overrideKnowledge = Resolve-InvestigationKnowledgeSource $request $snapshot
    Assert-InvestigationKnowledgeSource $overrideKnowledge
    $changedKnowledge = $overrideKnowledge | ConvertTo-Json -Depth 60 | ConvertFrom-Json
    $changedKnowledge.sourceSha256 = 'f' * 64
    Assert-Rejected { Assert-InvestigationKnowledgeSource $changedKnowledge } 'A changed expert hypothesis source escaped the read-only seal.'
    Assert-Rejected { Resolve-InvestigationKnowledgeSource $request (Join-Path $snapshot 'controls') } `
        'A nested source directory was misrepresented as the hypothesis repository root.'
    if ($overrideKnowledge.path -cne $snapshot -or $overrideKnowledge.revision -ceq $legacyKnowledge.revision) {
        throw 'The explicit investigation source override was ignored.'
    }
    $overrideLayered = $layered | ConvertTo-Json -Depth 60 | ConvertFrom-Json
    $overrideLayered | Add-Member -NotePropertyName knowledgeRepository -NotePropertyValue $overrideKnowledge
    $overrideExpert = $expert | ConvertTo-Json -Depth 60 | ConvertFrom-Json
    $overrideExpert.sourceRepository = $snapshot
    $overrideExpert.sourceCoverage.control = @(Get-FixtureReference (Join-Path $snapshot 'controls\dev\Control.cpp'))
    $overrideExpert.sourceCoverage.framework = @(Get-FixtureReference (Join-Path $snapshot 'dxaml\xcp\Layout.cpp'))
    $overrideProvenancePath = Join-Path $reportDirectory 'expert\override-provenance.json'
    Write-JsonFile $overrideProvenancePath $overrideExpert
    $overrideLayered.provenance = Get-FixtureReference $overrideProvenancePath
    $preparedKnowledge = Get-PreparationKnowledgeSource $overrideLayered $request
    Assert-PreparationBindings $overrideLayered $source $overrideExpert $request
    $overrideExpert.sourceRepository = $repository
    Assert-Rejected { Assert-PreparationBindings $overrideLayered $source $overrideExpert $request } `
        'An old expert recommendation was rebound to an uninspected override checkout.'
    $overrideExpert.sourceRepository = $snapshot
    $overrideExpert.sourceCoverage.control = @($expert.sourceCoverage.control)
    Assert-Rejected { Assert-PreparationBindings $overrideLayered $source $overrideExpert $request } `
        'Same-byte native coverage outside the explicitly inspected checkout was accepted.'
    $overrideExpert.sourceCoverage.control = @(Get-FixtureReference (Join-Path $snapshot 'controls\dev\Control.cpp'))
    $overrideExpert.sourceCoverage.control[0].sha256 = 'f' * 64
    Assert-Rejected { Assert-PreparationBindings $overrideLayered $source $overrideExpert $request } `
        'A substituted expert source hash was accepted.'
    $overrideExpert.sourceCoverage.control = @(Get-FixtureReference (Join-Path $snapshot 'controls\dev\Control.cpp'))
    $overrideLayered.knowledgeRepository.sourceSha256 = 'f' * 64
    Assert-Rejected { Get-PreparationKnowledgeSource $overrideLayered $request } 'Changed hypothesis source metadata was accepted.'
    $overrideLayered.knowledgeRepository.sourceSha256 = $preparedKnowledge.sourceSha256
    Assert-PreparationBindings $overrideLayered $source $overrideExpert $request

    $derivedDirectory = Join-Path $root 'derived-request'
    New-Item -ItemType Directory $derivedDirectory | Out-Null
    $legacyRequestReference = New-PreparationRequestBinding $request $source.request $legacyKnowledge $derivedDirectory
    if ($legacyRequestReference.path -cne $source.request.path -or (Test-Path -LiteralPath (Join-Path $derivedDirectory 'request.json'))) {
        throw 'Same-root preparation unnecessarily rewrote the frozen original request.'
    }
    $derivedRequestReference = New-PreparationRequestBinding $request $source.request $preparedKnowledge $derivedDirectory
    $derivedRequest = Read-JsonFile $derivedRequestReference.path
    if ($derivedRequest.knowledgeRepository -cne $snapshot -or $derivedRequest.sourceRequest.path -cne $source.request.path -or
        $derivedRequest.sourceRequest.sha256 -cne $source.request.sha256 -or $derivedRequest.allowMeasurements -ne $true -or
        $derivedRequest.scenario -cne $request.scenario -or $derivedRequest.applicationRepository -cne $app -or
        $derivedRequest.hypothesisSource.sourceSha256 -cne $preparedKnowledge.sourceSha256 -or
        $request.knowledgeRepository -cne $repository -or (Read-JsonFile $source.request.path).knowledgeRepository -cne $repository) {
        throw 'The derived request lost exact lineage, changed discovery consent/scenario, or mutated the original request.'
    }
    Assert-FileHash $source.request.path $source.request.sha256
    Assert-FileHash $layered.sourceInvestigation.path $layered.sourceInvestigation.sha256
    $overrideSnapshotIdentity = @{
        originalRepository = $preparedKnowledge.path; originalRevision = $preparedKnowledge.revision
        originalSourceSha256 = $preparedKnowledge.sourceSha256; source = (Join-Path $root 'snapshot')
        files = $preparedKnowledge.files; excludedSampleSubmodules = $preparedKnowledge.excludedSampleSubmodules
        applicationSource = $appBaselineSource
    }
    $overrideTrial = New-PreparationTrialContext $source $overrideLayered $layeredReference $recommendations.recommendations[1] `
        $root $trialDirectory $pack '' $overrideSnapshotIdentity $derivedRequestReference $preparedKnowledge
    if ($overrideTrial.request.path -cne $derivedRequestReference.path -or
        $overrideTrial.originalRequest.sha256 -cne $source.request.sha256 -or
        $overrideTrial.native.originalRepository -cne $snapshot -or
        $overrideTrial.native.originalRevision -cne $preparedKnowledge.revision -or
        $overrideTrial.originalRepository -cne $app -or $overrideTrial.originalRevision -cne $source.originalRevision) {
        throw 'An override native trial mixed the new hypothesis baseline with the original application identity.'
    }

    $supplementDirectory = Join-Path $root 'supplement'
    $supplementInput = Join-Path $supplementDirectory 'discovery-data.json'
    Write-FixtureFile $supplementInput 'Synthetic discovery-only data.'
    $supplementPath = Join-Path $supplementDirectory 'manifest.json'
    Write-JsonFile $supplementPath @{
        schema = 'ui-profiler/supplemental-evidence/v1'; provider = 'MUX WinUIProfiler'; captures = 0
        originalMeasurement = $source.baseline; originalDatabase = Get-FixtureReference $databasePath
        inputs = @(Get-FixtureReference $supplementInput)
    }
    $supplementBinding = Get-SupplementalEvidenceBinding $supplementPath $source.baseline.path (Read-JsonFile $source.baseline.path)
    $supplementReference = @{ path = $supplementBinding.path; sha256 = $supplementBinding.sha256 }
    $supplementLayered = $layered | ConvertTo-Json -Depth 60 | ConvertFrom-Json
    $supplementLayered | Add-Member -NotePropertyName supplementalEvidence -NotePropertyValue $supplementReference
    $supplementExpert = $expert | ConvertTo-Json -Depth 60 | ConvertFrom-Json
    $supplementExpert | Add-Member -NotePropertyName supplementalEvidence -NotePropertyValue $supplementReference
    $supplementProvenancePath = Join-Path $reportDirectory 'expert\supplement-provenance.json'
    Write-JsonFile $supplementProvenancePath $supplementExpert
    $supplementLayered.provenance = Get-FixtureReference $supplementProvenancePath
    Assert-PreparationBindings $supplementLayered $source $supplementExpert $request
    $supplementTrial = New-PreparationTrialContext $source $supplementLayered $layeredReference $recommendations.recommendations[0] `
        $root $trialDirectory $pack ''
    if ($supplementTrial.supplementalEvidence.sha256 -cne $supplementBinding.sha256) {
        throw 'The trial dropped its optional supplemental discovery reference.'
    }
    $supplementExpert.supplementalEvidence = @{ path = $supplementPath; sha256 = ('f' * 64) }
    Assert-Rejected { Assert-PreparationBindings $supplementLayered $source $supplementExpert $request } `
        'Expert/report supplemental evidence disagreement was accepted.'
    $supplementExpert.supplementalEvidence = $supplementReference
    Write-FixtureFile $supplementInput 'Changed supplemental data.'
    Assert-Rejected { Assert-PreparationBindings $supplementLayered $source $supplementExpert $request } `
        'Changed supplemental data escaped its frozen discovery binding.'
    Write-FixtureFile $supplementInput 'Synthetic discovery-only data.'
    $expert.analysisSha256 = 'f' * 64
    Assert-Rejected { Assert-PreparationBindings $layered $source $expert $request } 'Detached expert provenance was accepted.'
    $expert.analysisSha256 = $source.analysis.sha256
    Write-FixtureFile $source.scenarioPath 'mutated scenario'
    Assert-Rejected { Assert-PreparationBindings $layered $source $expert $request } 'A changed scenario was accepted.'
    Write-FixtureFile $source.scenarioPath 'immutable synthetic scenario.json'
    Write-FixtureFile (Join-Path $root 'mcp\extra.dll') 'unexpected deployment change'
    Assert-Rejected { Assert-PreparationBindings $layered $source $expert $request } 'A changed MCP deployment closure was accepted.'
    Remove-Item -LiteralPath (Join-Path $root 'mcp\extra.dll')
    Write-FixtureFile (Join-Path $repository 'controls\dev\Control.cpp') 'changed expert-inspected source'
    Assert-Rejected { Assert-PreparationBindings $layered $source $expert $request } 'A changed expert-inspected native source was accepted.'
    Write-Output 'Recommendation preparation checks passed: explicit source overrides, strict source/coverage hashes, derived request lineage, supplemental discovery binding, legacy defaults, long paths, independent native/app contexts.'
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force
}
