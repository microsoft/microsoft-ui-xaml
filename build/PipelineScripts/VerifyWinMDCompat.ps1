[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$WinMDPath = "${env:BuildArtifactsDir}\packaging\${env:Configuration}\lib\uap10.0",
    [Parameter(Mandatory = $false)]
    [string]$PackagesDirectory = "${env:RepoRoot}\packages",
    [Parameter(Mandatory = $false)]
    [string]$WinAppSDKWinUIVersion = "",
    [Parameter(Mandatory = $false)]
    [bool]$isRelease = $false
)

function Get-LatestVersion([string]$NugetPackageName, [bool]$PublicOnly = $false)
{
    # BUILD_BUILDID will only be defined if we're in the context of a lab build -
    # in that case we know we'll have already installed the latest version of the nuget packages.
    if ($env:BUILD_BUILDID)
    {
        Write-Host "Pipeline run: Checking just for latest installed package..."
        return ([regex]"$NugetPackageName\s+(\S+)").Matches((& nuget list -Source $PackagesDirectory $NugetPackageName)).Groups[1].Value
    }
    else
    {
        if ($PublicOnly)
        {
            # Searching only Nuget.org ensures that we find the latest *public* release
            # Use the 'packageid' search term query to force an exact match
            $nugetCommandArguments = "list", "packageid:$NugetPackageName", "-Source", "https://api.nuget.org/v3/index.json"
        }
        else
        {
            # Azure Artifacts feeds don't understand the 'packageid' search term query
            $nugetCommandArguments = "list", "$NugetPackageName"
        }

        return ([regex]"$NugetPackageName\s+(\S+)").Matches((& nuget $nugetCommandArguments)).Groups[1].Value
    }
}

function Write-LabError([string]$message)
{
    # BUILD_BUILDID will only be defined if we're in the context of a lab build -
    # in that case we want to use the AzDO error syntax rather than using Write-Error.
    if ($env:BUILD_BUILDID)
    {
        $message.Split([Environment]::NewLine, [System.StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { Write-Host "##[error]$_" }
    }
    else
    {
        Write-Error $message
    }
}

if (-not (Test-Path $WinMDPath))
{
    Write-LabError "WinMD path $WinMDPath does not exist."
    exit 1
}

if ($WinAppSDKWinUIVersion)
{
    $winAppSDKWinUIVersionToUse = $WinAppSDKWinUIVersion
    Write-Host "Specific WinUI version specified for comparison: $winAppSDKWinUIVersionToUse"
}
else
{
    Write-Host "Searching for latest public WinUI version"
    $winAppSDKWinUIVersionToUse = Get-LatestVersion "Microsoft.WindowsAppSDK.WinUI" -PublicOnly $true
    Write-Host "Latest WinUI version found for comparison: $winAppSDKWinUIVersionToUse"
}

# During a lab build the package should have already been installed as a pre-step
if (-not $env:BUILD_BUILDID)
{
    Write-Host "Using `nuget install` to ensure Microsoft.WindowsAppSDK.WinUI package exists..."
    & nuget install Microsoft.WindowsAppSDK.WinUI -OutputDirectory $PackagesDirectory -NonInteractive -Version $winAppSDKWinUIVersionToUse
    $nugetResultCode = $?
}

$winAppSdkPath = [System.IO.Path]::Combine($PackagesDirectory, "Microsoft.WindowsAppSDK.WinUI", $winAppSDKWinUIVersionToUse)
if (-not (Test-Path $winAppSdkPath))
{
    $winAppSdkPath = [System.IO.Path]::Combine($PackagesDirectory, "Microsoft.WindowsAppSDK.WinUI.$winAppSDKWinUIVersionToUse")
}

if (-not (Test-Path $winAppSdkPath))
{
    if (-not $nugetResultCode)
    {
        Write-LabError "Nuget install failed."
        exit 1
    }
    else
    {
        Write-LabError "Couldn't find Microsoft.WindowsAppSDK.WinUI path even after nuget install."
        exit 1
    }
}

# During a lab build the package should have already been installed as a pre-step
if (-not $env:BUILD_BUILDID)
{
    Write-Host "Using `nuget install` to ensure WinMDVersionVerifyLcr package exists..."
    & nuget install WinMDVersionVerifyLcr -OutputDirectory $PackagesDirectory -NonInteractive
    $nugetResultCode = $?
}

$WinMDVersionVerifyLcrVersionToUse = Get-LatestVersion "WinMDVersionVerifyLcr"
$winMDVersionVerifyLcrPath = [System.IO.Path]::Combine($PackagesDirectory, "WinMDVersionVerifyLcr", $WinMDVersionVerifyLcrVersionToUse, "WinMDVersionVerifyLcr.exe")
if (-not (Test-Path $winMDVersionVerifyLcrPath))
{
    $winMDVersionVerifyLcrPath = [System.IO.Path]::Combine($PackagesDirectory, "WinMDVersionVerifyLcr.$WinMDVersionVerifyLcrVersionToUse", "WinMDVersionVerifyLcr.exe")
}

if (-not (Test-Path $winMDVersionVerifyLcrPath))
{
    if (-not $nugetResultCode)
    {
        Write-LabError "Nuget install failed."
        exit 1
    }
    else
    {
        Write-LabError "Couldn't find WinMDVersionVerifyLcr.exe even after nuget install."
        exit 1
    }
}

$winAppSdkWinMDPath = "$WinAppSdkPath\metadata"

# We want to compare only the WinMDs we build as part of WinUI 3, so we'll copy
# the WinMDs from our build and from the WinAppSDK package to temp folders.
$oldDirectory = "${env:TEMP}\WinAppSDKWinMDs"
$newDirectory = "${env:TEMP}\LocalWinMDs"

if (-not (Test-Path $oldDirectory))
{
    New-Item $oldDirectory -ItemType Directory | Out-Null
}
else
{
    Remove-Item "$oldDirectory\*.winmd"
}

if (-not (Test-Path $newDirectory))
{
    New-Item $newDirectory -ItemType Directory | Out-Null
}
else
{
    Remove-Item "$newDirectory\*.winmd"
}

Write-Host

# We'll only copy the WinMDs common to both, since otherwise that will give us false errors
# when a type in one WinMD file doesn't have a corresponding WinMD file in the other folder.
$builtWinMDFiles = (Get-ChildItem "$WinMDPath\*.winmd")
$winAppSdkWinMDFiles = (Get-ChildItem "$winAppSdkWinMDPath\*.winmd")
[System.Collections.Generic.List[string]]$commonWinMDFiles = @()

foreach ($winmdFile in $builtWinMDFiles)
{
    $fileExistsInBothDirectories = $false

    foreach ($winmdFileCompare in $winAppSdkWinMDFiles)
    {
        if ([string]::Compare($winmdFile.Name, $winmdFileCompare.Name, $true) -eq 0)
        {
            $fileExistsInBothDirectories = $true
            break
        }
    }

    if ($fileExistsInBothDirectories)
    {
        Write-Host "Detected common WinMD file: $($winmdFile.Name)"
        $commonWinMDFiles.Add($winmdFile.Name)
    }
}

foreach ($winmdFile in $commonWinMDFiles)
{
    $oldWinMDPath = [System.IO.Path]::Combine($winAppSdkWinMDPath, $winmdFile)
    $newWinMDPath = [System.IO.Path]::Combine($WinMDPath, $winmdFile)

    Write-Host

    Write-Host "Copying $oldWinMDPath to $oldDirectory..."
    Copy-Item -Path $oldWinMDPath -Destination $oldDirectory
    Write-Host "Copying $newWinMDPath to $newDirectory..."
    Copy-Item -Path $newWinMDPath -Destination $newDirectory
}

if ($isRelease)
{
    $exceptionsFileName = "WinMDCompatExceptions_release.txt"
}
else
{
    $exceptionsFileName = "WinMDCompatExceptions_prerelease.txt"
}

$exceptionsFile = [System.IO.Path]::Combine($PSScriptRoot, $exceptionsFileName)

$params = @($oldDirectory, $newDirectory, "/v", "/e:$exceptionsFile")

Write-Host "Last exist code $LastExitCode"
Write-Host
Write-Host "Calling: $winMDVersionVerifyLcrPath $params"

$outputFile = Join-Path $env:TEMP "winmdverify.out.txt"
& $winMDVersionVerifyLcrPath $params 2>&1 > $outputFile

# The error code from WinMDVersionVerifyLcrPath.exe, if positive, is set to the number of errors that were detected.
if ($LastExitCode -gt 0)
{
    $errors = Get-Content $outputFile -Raw
    Write-Host "----- Output file content: -----"
    Write-Host "$errors"
    Write-Host "--------------------------------"
    Write-LabError "$LastExitCode WinMD compat error$(if ($LastExitCode -eq 1) { " was" } else { "s were" }) detected:$([Environment]::NewLine)$([Environment]::NewLine)$errors"

    exit 1
}
# If negative, the error code indicates a problem running the executable.
elseif ($LastExitCode -lt 0)
{
    $errors = Get-Content $outputFile -Raw
    Write-Host "----- Output file content: -----"
    Write-Host "$errors"
    Write-Host "--------------------------------"
    Write-LabError "Error retrieving WinMD compat errors. Exit code was $LastExitCode. Error output:$([Environment]::NewLine)$([Environment]::NewLine)$errors"

    exit 1
}
# If zero, the error code indicates that the executable ran successfully and found no compat errors.
else
{
    Write-Host "No WinMD compat errors were detected."
    exit 0
}
