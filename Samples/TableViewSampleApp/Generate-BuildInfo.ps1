# Generate-BuildInfo.ps1 — write BuildInfo.cs from current git state.
# Invoked by csproj _GenerateBuildInfo target. Robust against MSBuild's
# %-escape eating that broke the inline git log --pretty=%s pattern.
#
# Usage:  pwsh -File Generate-BuildInfo.ps1 -OutFile <path> -ProjectDir <path>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutFile,
    [Parameter(Mandatory)][string]$ProjectDir
)

$ErrorActionPreference = 'Stop'

function Run-Git {
    param([Parameter(Mandatory)][string[]]$Args)
    try {
        $out = & git -C $ProjectDir @Args 2>&1
        if ($LASTEXITCODE -ne 0) { return '(unknown)' }
        return ($out | Out-String).Trim()
    } catch { return '(unknown)' }
}

$sha       = Run-Git -Args @('rev-parse','HEAD')
$shaShort  = if ($sha.Length -ge 10) { $sha.Substring(0,10) } else { $sha }
$subject   = Run-Git -Args @('log','-1','--format=%s','HEAD')
$timestamp = Run-Git -Args @('log','-1','--format=%cI','HEAD')
$branch    = Run-Git -Args @('rev-parse','--abbrev-ref','HEAD')

$subjectEscaped = $subject -replace '\\','\\' -replace '"','\"'
$buildTime = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
$flavor    = "${env:Configuration}|${env:Platform}|${env:RuntimeIdentifier}"
if ([string]::IsNullOrWhiteSpace($flavor) -or $flavor -eq '||') { $flavor = '(unknown)' }

$content = @"
// Auto-rewritten by Generate-BuildInfo.ps1 on every build. Committed copy is a placeholder.
namespace TableViewSampleApp;
internal static class BuildInfo
{
    public static string CommitSha       = "$sha";
    public static string CommitShaShort  = "$shaShort";
    public static string CommitSubject   = "$subjectEscaped";
    public static string CommitTimestamp = "$timestamp";
    public static string Branch          = "$branch";
    public static string BuildTimestamp  = "$buildTime";
    public static string BuildFlavor     = "$flavor";
}
"@

$dir = Split-Path $OutFile -Parent
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
[System.IO.File]::WriteAllText($OutFile, $content, [System.Text.UTF8Encoding]::new($false))
Write-Host "BuildInfo: $shaShort on $branch - $subject"
