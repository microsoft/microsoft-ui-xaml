param(
    [Parameter(Mandatory=$true)] [string] $repoRoot,
    [string] $Verbosity = 'quiet'
)


if ($env:BUILDPLATFORM -eq $null)
{
    if ($env:amd64)
    {
        $buildPlatform = "x64"
    }
    elseif ($env:arm64)
    {
        $buildPlatform = "arm64"
    }
    else
    {
        $buildPlatform = "x86"
    }
}
else
{
    $buildPlatform = $env:BUILDPLATFORM
}

if (Test-Path "$repoRoot\build\PipelineScripts\GetLKGCompilerPackageInfoIfNeeded.ps1")
{
    # Trigger install of the LKG toolset by default, allowing customization.
    # Retrieve default package info if needed.
    . $repoRoot\build\PipelineScripts\GetLKGCompilerPackageInfoIfNeeded.ps1 -SourceDirectory $repoRoot
    
    # The following call skips download of the package if it already exists locally.
    . $repoRoot\build\PipelineScripts\DownloadLKGCompiler.ps1 -SourceDirectory $repoRoot
}

[System.Collections.Generic.List[string]]$mainPkgs = @()

# Regardless of the WinUI flavor we're building,
# we also always need to build a version of GenXbf needs to match the architecture of MSBuild - x64 for VS2022, x86 for all prior versions.
# So we need x86 and x64 for GenXbf no matter what.

# The others we'll pull down as needed.
$mainPkgs.Add("packages.config")
$mainPkgs.Add("packages.x86.config")
$mainPkgs.Add("packages.x64.config")

if (($buildPlatform -ne "x86") -and ($buildPlatform -ne "x64"))
{
    $mainPkgs.Add("packages.$buildPlatform.config")
}

Write-Host "Restoring packages for build platform $buildPlatform..." -NoNewline
$installed = 0
foreach ($pkg in $mainPkgs)
{
    Write-Progress "Restoring packages for build platform $buildPlatform..." -PercentComplete (100 * $installed / $mainPkgs.Count)
    nuget restore $repoRoot\$pkg -ConfigFile $repoRoot\nuget.config -PackagesDirectory $repoRoot\packages -Verbosity $Verbosity
    $installed++
}
Write-Host -ForegroundColor Green Done.
Write-Progress "Restoring packages for build platform $buildPlatform..." -Completed

. $repoRoot\build\DownloadDotNetCoreSdk.ps1
. $repoRoot\build\DownloadDotNetRuntimeInstaller.ps1

Write-Host "Restoring Maestro and ensuring authentication..."
msbuild -nologo -t:Restore $repoRoot\eng\Microsoft.MaestroRestore.csproj -v:$Verbosity -p:Configuration=Release -p:NugetInteractive=true -p:PublishReadyToRun=true

Write-Host "Restoring additional packages..."
$projectPackages = @(
    'perf\packages.config',
    'eng\BuildGenXbfForMSBuild\BuildGenXbfForMSBuild.csproj',
    'eng\Microsoft.MaestroRestore.csproj',
    'controls\dev\dll\packages.config',
    'XamlCompilerPrerequisites.sln',
    'dxaml\Microsoft.UI.Xaml.sln',
    'dxaml\xcp\tools\XbfParser\XbfParser.sln',
    'src\XamlCompiler\XamlCompiler.sln'
)

# Check if this is an OSS build, where not all files are available
if (-not (Test-Path $repoRoot\src\XamlCompiler\BuildTasks\Microsoft\Lmr\XamlTypeUniverse.cs))
{
    # Use smaller perf config when building OSS
    $projectPackages = $projectPackages | ForEach-Object { $_ -replace 'perf\\packages.config', 'perf\packages.OSS.config' }

    # Replace the full MUX sln with the one limited to the OSS-available projects
    $projectPackages = $projectPackages | ForEach-Object { $_ -replace 'dxaml\\Microsoft.UI.Xaml.sln', 'dxaml\Microsoft.UI.Xaml.OSS.sln' }

    # We don't have all necessary files to build the compiler, so also restore
    # the project which uses the public compiler
    $projectPackages += 'XamlCompilerPublic.csproj'
}

$installed = 0
foreach ($project in $projectPackages)
{
    Write-Host "Restoring $project"
    Write-Progress "Restoring additional packages..." -PercentComplete (100 * $installed / $projectPackages.Count)
    nuget restore $repoRoot\$project -ConfigFile $repoRoot\nuget.config -PackagesDirectory $repoRoot\packages -Verbosity $Verbosity
    $installed++
}
Write-Host -ForegroundColor Green Done.
Write-Progress "Restoring additional packages..." -Completed
