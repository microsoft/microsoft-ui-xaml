# Three-level reporting and execution-boundary checks; zero agents or captures.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$temp = Join-Path ([IO.Path]::GetTempPath()) ('ui-profiler-levels-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $temp)
try {
    $entries = foreach ($level in @('application', 'control', 'framework')) {
        @{ id = $level; level = $level; title = "$level finding"; summary = 'Synthetic source-grounded hypothesis.'
            verdict = if ($level -eq 'application') { 'propose' } else { 'recommend' }
            files = @(@{ path = if ($level -eq 'application') { 'Page.xaml' } else { "$level.cpp" }
                change = 'Synthetic fixture only.' })
            observations = @('Synthetic measurement.'); hypotheses = @('Unproven.'); risks = @('Not an optimization result.')
            validation = @('Build the correct target/runtime and compare the same scenario.') }
    }
    $recommendations = @{ schema = 'ui-profiler/recommendations/v2'; summary = 'Three independent levels.'
        coverage = @{ application = 'App usage.'; control = 'Control implementation.'; framework = 'Core engine.' }
        runtimeSourceParity = 'Unverified runtime/source match.'; risks = @('Synthetic evidence.'); recommendations = @($entries) }
    $path = Join-Path $temp 'recommendation.json'
    Write-JsonFile $path $recommendations
    $context = [pscustomobject]@{ schema = 'ui-profiler/read-only-investigation/v1'; directory = $temp
        originalRevision = 'synthetic'; baseline = @{ path = 'synthetic.sqlite'; sha256 = 'synthetic' }
        recommendations = @{ path = $path; sha256 = Get-Sha256 $path }; trials = @(); report = $null }
    $result = Write-RecommendationReport $context
    $context.report = @{ path = $result.path; sha256 = $result.sha256 }
    $report = Assert-RecommendationReport $context
    if ($report.schema -ne 'ui-profiler/recommendation-report/v2' -or
        $report.rows[0].state -ne 'pending' -or $report.rows[1].state -ne 'target-workflow-required' -or
        $report.rows[2].repository -ne 'mux' -or $report.coverage.framework -ne 'Core engine.') {
        throw 'Layer coverage, repository scope or native execution eligibility was lost.'
    }
    $markdown = Get-Content -LiteralPath (Join-Path $temp 'recommendations-report.md') -Raw
    if (!$markdown.Contains('## control investigation') -or !$markdown.Contains('## framework investigation') -or
        !$markdown.Contains('target-workflow-required') -or !$markdown.Contains('### validation')) {
        throw 'The readable report omitted a layer or confused unsupported execution with missing evidence.'
    }
    $recommendations.schema = 'ui-profiler/recommendations/v3'
    $recommendations.detailedAnalysis = @{}
    foreach ($level in @('application', 'control', 'framework')) {
        $recommendations.detailedAnalysis[$level] = @{
            question = "What costs are attributable to $level?"
            time = @('Measured nested elapsed spans, not exclusive CPU.')
            cpu = @('Exclusive CPU is not captured.')
            memory = @('Owned bytes are unknown; process totals are separate.')
            modelAnalysis = @('Source/model explanation is not a measurement.')
            unverified = @('No candidate run or improvement yet.')
            validation = @('One source-matched baseline/candidate run.')
        }
    }
    Write-JsonFile $path $recommendations
    $context.recommendations.sha256 = Get-Sha256 $path
    $result = Write-RecommendationReport $context
    $context.report = @{path=$result.path;sha256=$result.sha256}
    $report = Assert-RecommendationReport $context
    if ($report.schema -ne 'ui-profiler/recommendation-report/v3' -or
        $report.detailedAnalysis.framework.memory[0] -notlike 'Owned bytes*') {
        throw 'Deep framework/control/application layouts were lost.'
    }
    $entries[1].verdict = 'propose'
    Write-JsonFile $path $recommendations
    $context.recommendations.sha256 = Get-Sha256 $path
    $failed = $false
    try { [void](Write-RecommendationReport $context) } catch { $failed = $true }
    if (!$failed) { throw 'A native recommendation entered the app-only trial workflow.' }
    $entries[1].verdict = 'insufficient-evidence'
    $entries[1].files = @()
    Write-JsonFile $path $recommendations
    $context.recommendations.sha256 = Get-Sha256 $path
    $result = Write-RecommendationReport $context
    $report = Read-JsonFile $result.path
    if ($report.rows[1].state -ne 'insufficient-evidence' -or $report.rows[2].state -ne 'target-workflow-required') {
        throw 'Evidence confidence was conflated with target execution support.'
    }
    $legacy = @{ schema = 'ui-profiler/recommendations/v1'; summary = 'Legacy app report.'
        risks = @('Synthetic evidence.'); recommendations = @($entries[0]) }
    $legacy.recommendations[0].Remove('level')
    $legacy.recommendations[0].Remove('validation')
    Write-JsonFile $path $legacy
    $context.recommendations.sha256 = Get-Sha256 $path
    $result = Write-RecommendationReport $context
    $context.report = @{ path = $result.path; sha256 = $result.sha256 }
    $report = Assert-RecommendationReport $context
    if ($report.schema -ne 'ui-profiler/recommendation-report/v1' -or
        $report.rows[0].level -ne 'application' -or $report.rows[0].state -ne 'pending') {
        throw 'Legacy app report execution eligibility changed.'
    }
    Write-Output 'Three-level report checks passed: coverage, target scope, validation, execution/evidence distinction, native boundary and legacy v1.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
