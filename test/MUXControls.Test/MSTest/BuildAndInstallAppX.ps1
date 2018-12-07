[CmdletBinding()]
param(
    [Parameter(Position=0,mandatory=$true)]
    [String]$ProjectName,
    [Parameter(Position=1,mandatory=$true)]
    [String]$Architecture,
    [Parameter(Position=2,mandatory=$true)]
    [String]$PackageName,
    [Parameter(Position=3,mandatory=$true)]
    [String]$PackageFullName
)

$appxRegKey = "HKLM:\Software\Policies\Microsoft\Windows\AppX"
$sideloadingRegName = "AllowAllTrustedApps"

Write-Host ""

if (-not (Test-Path $appxRegKey) -or -not (Get-ItemProperty -Path $appxRegKey -Name $sideloadingRegName -ErrorAction Ignore) -or (Get-ItemPropertyValue -Path $appxRegKey -Name $sideloadingRegName) -ne 1)
{
    Write-Host "Enabling sideloading of apps..." -ForegroundColor Magenta
    Write-Host ""

    $commands = @(
        "if (-not (Test-Path $appxRegKey)) { New-Item -Path $appxRegKey }",
        "New-ItemProperty -Path $appxRegKey -Name $sideloadingRegName -Value 1 -PropertyType DWORD -Force")
    $argumentList = "-ExecutionPolicy Unrestricted -Command `"& { $($commands -join "; ") }`""

    Write-Host "powershell $argumentList"
    Write-Host ""
    Start-Process -FilePath powershell.exe -ArgumentList $argumentList -Verb RunAs
}

# The configuration is present in the directory structure, so we'll get it from there.
$currentDirectory = (Get-Item $MyInvocation.MyCommand.Definition).Directory
$baseDirectory = $currentDirectory.Parent.Parent

$configuration = $baseDirectory.Name
$baseDirectoryName = $baseDirectory.FullName
$rootDirectoryName = $baseDirectory.Parent.Parent.FullName

Write-Host "Test AppX does not exist or is not up-to-date. Running build to generate." -ForegroundColor Magenta
Write-Host ""
$testProjectFile = Get-Item "$rootDirectoryName\test\$($ProjectName)\MSTest\$($ProjectName).csproj"
$buildCmd = "$PSScriptRoot\BuildAppX.cmd $($testProjectFile.FullName) $Architecture $configuration"
Write-Host $buildCmd
Invoke-Expression $buildCmd
Write-Host ""
Write-Host "Rebuild complete. Uninstalling current app if it exists..." -ForegroundColor Magenta
Write-Host ""

if (Get-AppxPackage -Name $PackageName)
{
    Remove-AppxPackage -Package $PackageFullName
}

& $PSScriptRoot\InstallAppX.ps1 -AppXDir "$baseDirectoryName\$Architecture\$($ProjectName)\AppPackages\$($ProjectName)_Test"