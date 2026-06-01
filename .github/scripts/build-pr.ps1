param(
    [Parameter(Mandatory)] [string]$Flavor,
    [Parameter(Mandatory)] [string]$Platform,
    [Parameter(Mandatory)] [string]$Configuration
)

$ErrorActionPreference = 'Stop'

# init.cmd's UAC fallback fails non-interactively; ensure long-path support up front.
$lp = 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem'
if ((Get-ItemProperty -Path $lp -Name 'LongPathsEnabled' -ErrorAction SilentlyContinue).LongPathsEnabled -ne 1) {
    New-ItemProperty -Path $lp -Name 'LongPathsEnabled' -Value 1 -PropertyType DWord -Force | Out-Null
}

foreach ($v in 'ACTIONS_RUNTIME_TOKEN','ACTIONS_RUNTIME_URL','ACTIONS_RESULTS_URL','ACTIONS_CACHE_URL','ACTIONS_ID_TOKEN_REQUEST_TOKEN','ACTIONS_ID_TOKEN_REQUEST_URL','GITHUB_TOKEN') {
    Remove-Item "env:$v" -ErrorAction SilentlyContinue
}

$binlogDir = "BuildOutput\binlogs"
Write-Host "flavor=$Flavor platform=$Platform configuration=$Configuration"
if (-not (Test-Path $binlogDir)) { New-Item -ItemType Directory -Force -Path $binlogDir | Out-Null }

$commonArgs = "/restore /m /ds:false"

$lmrSentinel = "src\XamlCompiler\BuildTasks\Microsoft\Lmr\XamlTypeUniverse.cs"
if (Test-Path $lmrSentinel) {
    Write-Host "LMR present - building XamlCompilerPrerequisites.sln"
    $prereqSteps = @(
        "msbuild XamlCompilerPrerequisites.sln /p:Platform=$Platform /p:Configuration=$Configuration $commonArgs /binaryLogger:$binlogDir\XamlCompilerPrerequisites.$Platform.$Configuration.binlog"
    )
} else {
    Write-Host "OSS path - using XamlCompilerPublic.csproj + BuildGenXbfForMSBuild.csproj"
    $prereqSteps = @(
        "msbuild XamlCompilerPublic.csproj                              /p:Platform=$Platform /p:Configuration=$Configuration $commonArgs /binaryLogger:$binlogDir\XamlCompilerPublic.$Platform.$Configuration.binlog",
        "msbuild eng\BuildGenXbfForMSBuild\BuildGenXbfForMSBuild.csproj /p:Platform=$Platform /p:Configuration=$Configuration $commonArgs /binaryLogger:$binlogDir\BuildGenXbfForMSBuild.$Platform.$Configuration.binlog"
    )
}

$sequence = $prereqSteps + @(
    "msbuild Microsoft.UI.Xaml-Product.sln                       /p:Platform=$Platform /p:Configuration=$Configuration $commonArgs /binaryLogger:$binlogDir\Microsoft.UI.Xaml-Product.$Platform.$Configuration.binlog",
    "msbuild controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj /p:Platform=$Platform /p:Configuration=$Configuration $commonArgs /binaryLogger:$binlogDir\Microsoft.UI.Xaml.Controls.$Platform.$Configuration.binlog",
    "pack.component.cmd"
)

$chain = ("init.cmd $Flavor /nopgo") + " && " + ($sequence -join " && ")
Write-Host $chain

cmd /c $chain
$cmdExit = $LASTEXITCODE
if ($cmdExit -ne 0) {
    Write-Host "::error::Build chain exited with code $cmdExit."
    exit $cmdExit
}
