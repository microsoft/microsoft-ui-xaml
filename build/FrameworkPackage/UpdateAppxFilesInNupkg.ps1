<#
.SYNOPSIS
This script is used to take the MUX nuget package, unpack it and then overwrite the appx files in it with the
store-signed appx files and then re-create the nuget package.
#>
[CmdLetBinding()]
Param(
    [Parameter(Position=0,Mandatory=$true)]
    [string]$nugetPackage,
    [Parameter(Position=1,Mandatory=$true)]
    [string]$inputAppxDirectory,
    [switch]$pushAndQueueBuild)

$ErrorActionPreference = "Stop"
    
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($nugetPackage)

$flavors = @("x86", "x64", "arm", "arm64")

$flavorToAppx = @{}
$nugetLocationMapping = @{}
foreach ($flavor in $flavors)
{
    $search = "UAPSignedBinary_Microsoft.UI.Xaml.*.$flavor.appx"
    $found = Get-ChildItem $inputAppxDirectory -Filter $search
    if ($found.Length -eq 0)
    {
        Write-Error "Could not find '$search' in '$inputAppxDirectory'"
        Exit 1
    }
    $flavorToAppx[$flavor] = $found[0].FullName
    $nugetFileName = $found[0].Name.Replace("UAPSignedBinary_", "").Replace(".$flavor", "")
    $nugetLocationMapping[$flavor] = "tools\appx\$flavor\release\$nugetFileName"

    Write-Verbose "Source File: $($flavorToAppx[$flavor])"
    Write-Verbose "Dest File: $($nugetLocationMapping[$flavor])"
}

function New-TemporaryDirectory {
    $parent = [System.IO.Path]::GetTempPath()
    $name = [System.IO.Path]::GetRandomFileName()
    New-Item -ItemType Directory -Path (Join-Path $parent $name)
}

$tempDir = New-TemporaryDirectory
$nugetUnpacked = $tempDir.FullName
Write-Verbose "Nuget directory: $nugetUnpacked"

$nugetRewritten = $nugetPackage.Replace(".nupkg", ".updated.nupkg")

[Environment]::CurrentDirectory = $PSScriptRoot
$inputPath = [System.IO.Path]::GetFullPath($inputAppxDirectory)
Write-Verbose "Output path = $inputPath"

[System.IO.Compression.ZipFileExtensions]::ExtractToDirectory($archive, $nugetUnpacked)

# Remove things that are zip file metadata
Remove-Item -Force -Recurse "$nugetUnpacked\_rels"
Remove-Item -Force -Recurse "$nugetUnpacked\package"
if(Test-Path "$nugetUnpacked\.signature.p7s")
{
    Remove-Item -Force "$nugetUnpacked\.signature.p7s"
}
Remove-Item -Force "$nugetUnpacked\*Content_Types*"

foreach ($flavor in $flavors)
{
    $destFile = Join-Path $nugetUnpacked $nugetLocationMapping[$flavor]
    Write-Verbose "Copying '$($flavorToAppx[$flavor])' -> '$destFile'"
    Copy-Item $flavorToAppx[$flavor] $destFile
}

$nuspec = Join-Path $nugetUnpacked "Microsoft.UI.Xaml.nuspec"

$nuspecContent = Get-Content $nuspec -Encoding UTF8
$nuspecContent = $nuspecContent.Replace("<licenseUrl>https://aka.ms/deprecateLicenseUrl</licenseUrl>", "")
# Write-Verbose "Rewriting '$nuspec'"
Set-Content -Path $nuspec -Value $nuspecContent -Encoding UTF8

[xml]$nuspecContentXml = $nuspecContent
$version = $nuspecContentXml.GetElementsByTagName("version").'#text'
Write-Verbose "Nuget package version: $version"

Write-Host "Repacking nuget package..."

& "$PSScriptRoot\..\..\tools\NugetWrapper.cmd" pack "$nuspec" -BasePath "$nugetUnpacked" -OutputDirectory $nugetUnpacked
if ($LastExitCode -ne 0){ Throw "Failed to run nuget pack. Error Code: $LastExitCode" }

$outputFile = Get-ChildItem $nugetUnpacked -Filter "Microsoft.UI.Xaml.*.nupkg"
$outputFilePath = $outputFile.FullName

Write-Verbose "Move-Item $outputFilePath $nugetRewritten"

Move-Item -Force $outputFilePath $nugetRewritten

Write-Host "Repacked to: $nugetRewritten"

if ($pushAndQueueBuild)
{    
    & "$PSScriptRoot\..\..\tools\NugetWrapper.cmd" push -Source https://microsoft.pkgs.visualstudio.com/WinUI/_packaging/WinUI.SigningInput/nuget/v3/index.json -ApiKey az $nugetRewritten
    if ($LastExitCode -ne 0){ Throw "Failed to push updated package. Error Code: $LastExitCode" }
    

    Import-Module -Name $PSScriptRoot\..\..\tools\BuildMachineUtils.psm1 -DisableNameChecking

    function Queue-NugetSigningBuild
    {
        Param(
            [string]$version)

        $token = Get-AccessToken

        $headers = @{ 
                "Authorization" = ("Bearer {0}" -f $token);
                "Content-Type" = "application/json";
            }

        $root = @{
            "definition" = @{
                "id" = 34531
            };
            "parameters" = 
                ConvertTo-JSon (@{
                    "NameOfPackage" = "Microsoft.UI.Xaml"
                    "PackageVersion" = $version
                })
        };

        $jsonPayload = ConvertTo-JSon $root

        Write-Verbose "Payload = $jsonPayload"

        $result = Invoke-RestMethod -Method Post -Uri "https://microsoft.visualstudio.com/winui/_apis/build/builds?api-version=5.0" -Headers $headers -Body $jsonPayload

        $result
    }
    
    Write-Host "Queueing signing build"

    $result = Queue-NugetSigningBuild -version $version

    $buildUrl = $result._links.web.href

    Write-Host "Build queued at $buildUrl"
    Write-Host "Launching browser window."
    Start $buildUrl
}