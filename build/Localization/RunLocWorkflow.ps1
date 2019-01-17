[CmdLetBinding()]
param(
    [string]$BuildNumber,
    [switch]$CopyBackOnly
)

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

$rootPath = & $vswhere -Latest -requires Microsoft.Component.MSBuild -property InstallationPath
$MSBuildPath="$rootPath\MSBuild\15.0\Bin\MSBuild.exe"

if (-not (Test-Path $MSBuildPath))
{
    Write-Host "Could not find msbuild.exe in '$VSDIR'"
    Exit 1
}

function Usage {
    Write-Host @"
RunLocWorkflow.cmd ^<BUILDNUMBER^>

e.g. if today's date is 12/02/2016 I might do: RunLocWorkflow 20161202-manual

see readme.md for more details.
"@
}

Write-Verbose "CopyBackOnly $CopyBackOnly"

if (-not $BuildNumber) {
    Usage
    Exit 1
}

if (-not $CopyBackOnly)
{
    Write-Host "Restoring nuget packages ..." -ForegroundColor Green
    \\edge-svcs\nuget\v4\nuget.exe restore Settings\packages.config -packagesdirectory ..\..\packages -configfile nuget.config

    if ($lastexitcode -ne 0) {
	    Write-Host "##vso[task.logissue type=error;] Nuget package restore failed with exit code $lastexitcode"
	    Exit 1
    }

    & "$MSBuildPath" LocalizationAutomation.proj /fl "/p:EnableLocalization=true;TdBuildGitBranchId=master;BUILD_BUILDNUMBER=$BuildNumber"


    if ($lastexitcode -ne 0) {
	    Write-Host "##vso[task.logissue type=error;] MSBuild failed with exit code $lastexitcode"
	    Exit 1
    }
}

#
# Now copy back into the source tree the resw files.
#

Write-Host "Copying localized files back into the source tree" -ForegroundColor Green
[xml]$locConfigXml = Get-Content Settings\LocConfig.xml

$locOutput = "obj\Release\LocOutput"

$languages = get-childitem obj\Release\LocOutput | Where-Object { $_.Name -match "-" } | % { $_.Name }

foreach ($language in $languages)
{
    Write-Verbose "Current language: $language"
    foreach ($file in $locConfigXml.Modules.Module.File)
    {
        $destFilePath = $file.path -ireplace "%LocRoot%\\",""
        $destFilePath = $destFilePath -ireplace "en-us",$language

        $fileName = Split-Path -Leaf $destFilePath

        $destFileLocation = Split-Path -Parent $destFilePath

        $sourceLocation = "$locOutput\$language\bin\$($file.location)\$fileName"
        Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

        if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
        $output = Copy-Item $sourceLocation $destFileLocation
    }
}
Write-Host 

Write-Host "Done!" -ForegroundColor Green