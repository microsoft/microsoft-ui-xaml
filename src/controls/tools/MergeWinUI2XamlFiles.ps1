Param(
    [Parameter(Mandatory = $true)] 
    [string]$WinUI2RepoRoot,
    [string]$BuildArch = "x86",
    [string]$BuildType = "chk"
)

$xamlFilesPattern = "controls\dev\*.xaml"

# This file does git operations to XAML files, so we'll want to make sure that no changes to those files currently exist.
$gitXamlStatus = & git status -s $xamlFilesPattern

if ($gitXamlStatus)
{
    Write-Error @"
Uncommitted edits of XAML files detected:

$gitXamlStatus

Please `"git commit $xamlFilesPattern`" before running this script.
"@

    exit 1
}

if (-not $env:VSINSTALLDIR)
{
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msBuildInstallPath = & $vswhere -products Microsoft.VisualStudio.Product.BuildTools -property InstallationPath -prerelease -latest -version 16.0

    if (-not $msBuildInstallPath)
    {
        $msBuildInstallPath = & $vswhere -requires Microsoft.Component.MSBuild -property InstallationPath -prerelease -latest -version 16.0
    }

    $currentLocation = Get-Location
    & $msBuildInstallPath\Common7\Tools\Launch-VsDevShell.ps1 -Latest
    Set-Location $currentLocation
}

if ($env:RepoRoot)
{
    $repoRoot = $env:RepoRoot
}
else
{
    $repoRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..")
}

if ($env:BuildOutputRoot)
{
    $buildOutputRoot = $env:BuildOutputRoot
}
else
{
    $buildOutputRoot = "$repoRoot\BuildOutput\obj"
}

if ($env:_BuildArch)
{
    $BuildArch = $env:_BuildArch
}

if ($env:_BuildType)
{
    $BuildType = $env:_BuildType
}

$outDir = "$BuildOutputRoot\$BuildArch$BuildType\controls\tools\"
[System.Collections.Generic.List[string]]$xamlFilesHandled = [System.Collections.Generic.List[string]]::new()

Get-ChildItem "$repoRoot\controls\dev" -Filter "*.vcxitems" -Recurse | ForEach-Object {
    $pathToProject = $_.DirectoryName
    $xamlFiles = Get-ChildItem "$pathToProject\*.xaml" | Sort-Object -Property @{Expression = {$_.Name.Length}; Descending = $True}

    foreach ($xamlFile in $xamlFiles)
    {
        Write-Host "Merging $xamlFile..."
        $winUI2XamlPath = "$($xamlFile.Directory.FullName.Replace("$repoRoot\controls", $WinUI2RepoRoot))\"

        if ($xamlFile.Name.Contains("themeresources"))
        {
            $winUI2XamlPath = "$winUI2XamlPath$($xamlFile.Name)".ToLower().Replace("themeresources", "*themeresources")
        }
        elseif ($xamlFile.Name.Contains("_v1"))
        {
            continue
        }
        else
        {
            $winUI2XamlPath = "$winUI2XamlPath$([System.IO.Path]::GetFileNameWithoutExtension($xamlFile.Name))*.xaml"
        }
        
        function Remove-OsVersion
        {
            Param(
                [Parameter(Mandatory = $true)] 
                [string]$XamlFileName
            )

            $XamlFileName = $XamlFileName.Replace("_rs1", "")
            $XamlFileName = $XamlFileName.Replace("_rs2", "")
            $XamlFileName = $XamlFileName.Replace("_rs3", "")
            $XamlFileName = $XamlFileName.Replace("_rs4", "")
            $XamlFileName = $XamlFileName.Replace("_rs5", "")
            $XamlFileName = $XamlFileName.Replace("_19h1", "")
            $XamlFileName = $XamlFileName.Replace("_21h1", "")

            return $XamlFileName
        }

        # We want to make sure we merge from lower versions into higher versions,
        # so we'll enforce that sort order explicitly.
        function Get-XamlFileNameSortOrder
        {
            Param(
                [Parameter(Mandatory = $true)] 
                [string]$XamlFileName
            )

            $XamlFileName = $XamlFileName.ToLower()

            if ($XamlFileName.Contains("rs1"))
            {
                return 0
            }
            elseif ($XamlFileName.Contains("rs2"))
            {
                return 1
            }
            elseif ($XamlFileName.Contains("rs3"))
            {
                return 2
            }
            elseif ($XamlFileName.Contains("rs4"))
            {
                return 3
            }
            elseif ($XamlFileName.Contains("rs5"))
            {
                return 4
            }
            elseif ($XamlFileName.Contains("19h1"))
            {
                return 5
            }
            elseif ($XamlFileName.Contains("21h1"))
            {
                return 6
            }
            else
            {
                return 7
            }
        }
        
        [System.Collections.Generic.List[string]]$winUI2XamlFiles = @()
        Get-ChildItem (Remove-OsVersion $winUI2XamlPath) | Where-Object { -not $xamlFilesHandled.Contains($_.FullName.ToLower()) } | Sort-Object -Property @{Expression = { Get-XamlFileNameSortOrder $_.Name }; Descending = $False} | ForEach-Object { $winUI2XamlFiles.Add($_.FullName.ToLower()) }

        if (-not $xamlFile.Name.Contains("themeresources"))
        {
            $themeResourcesFiles = $winUI2XamlFiles | Where-Object { $_.Contains("themeresources") }
            $themeResourcesFiles | ForEach-Object { $winUI2XamlFiles.Remove($_) | Out-Null }
        }

        if (-not $xamlFile.Name.Contains("_v1"))
        {
            $v1Files = $winUI2XamlFiles | Where-Object { $_.Contains("_v1") }
            $v1Files | ForEach-Object { $winUI2XamlFiles.Remove($_) | Out-Null }
        }

        if ($winUI2XamlFiles.Count -gt 0)
        {
            $xamlFilesString = $winUI2XamlFiles -join ";"
            $outputXamlFileName = Remove-OsVersion $xamlFile.Name
            $outputXamlFilePath = "$pathToProject\$outputXamlFileName"
            $msBuildCommand = "msbuild `"$PSScriptRoot\MergeWinUI2XamlFiles.msbuildproj`" /p:XamlFiles=`"$xamlFilesString`" /p:OutputDirectory=`"$pathToProject`" /p:OutputXamlFileName=`"$outputXamlFileName`""

            Write-Host "Merging XAML files $xamlFilesString into $outputXamlFileName..."
            Write-Host $msBuildCommand

            # In order to pass a quote into a command line parameter, we need to escape it by wrapping it in quotes.
            Invoke-Expression $msBuildCommand.Replace("`"", "`"`"`"")

            # In the lifted XAML repo, we no longer need contract references since both MUX and MUXC are in the same place,
            # so we'll go ahead and remove all of those references from XAML files.
            Write-Host "Removing contract references from $outputXamlFileName..."

            $reader = [System.IO.StreamReader]::new($outputXamlFilePath, $true)

            $contents = $reader.ReadToEnd()
            $encoding = $reader.CurrentEncoding
            $reader.Close()

            # We only want to write to the file if the contents have actually changed, so we'll save off the original contents.
            $originalContents = $contents

            # We'll first add the copyright notice to the contents, since it'll have been removed by the XML parsing.
            $contents = @"
<!-- Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project root for license information. -->
$contents
"@

            # Sometimes regexes won't remove everything if the removal of one thing conflicted with another,
            # so we'll do a replace in a while loop for as long as a match is found.
            function Replace-AllInstances
            {
                Param(
                    [Parameter(Mandatory = $true)] 
                    [string]$StringContents,
                    [Parameter(Mandatory = $true)] 
                    [string]$MatchString,
                    [string]$ReplacementString = ""
                )

                while ($StringContents -imatch $MatchString)
                {
                    $StringContents = $StringContents -ireplace $MatchString, $ReplacementString
                }

                return $StringContents
            }

            # Now we'll remove every instance of "contractXPresent:", and entirely remove anything prepended with "contractXNotPresent:".
            $contents = Replace-AllInstances $contents "\s*?<\w*?contract(\w+?)NotPresent:(\S*?)[^>]*?>(?:\s|\S)*?<\w*?\/\w*?contract\1NotPresent:\2>"
            $contents = Replace-AllInstances $contents "\s*?</?\w*?contract\w+?NotPresent:.*?>"
            $contents = Replace-AllInstances $contents "<(/?)\w*?contract\w+?Present:(.*?)>" "<`$1`$2>"
            $contents = Replace-AllInstances $contents "\s*?\w*?contract\w+?NotPresent:.*?=`".*?`""
            $contents = Replace-AllInstances $contents "\w*?contract\w+?Present:(.*?=`".*?`")" "`$1"

            # Next, we'll remove all of the XML namespace defines for contracts.
            $contents = Replace-AllInstances $contents "\s*?xmlns:\w*?contract\w+Present=`".*?`""

            # Finally, we'll remove types and properties not present in WinUI 3.
            $contents = Replace-AllInstances $contents "\s*?BackgroundSource=`".*?`""
            $contents = Replace-AllInstances $contents "\s*?ContentLinkForegroundColor=`".*?`""
            $contents = Replace-AllInstances $contents "\s*?ContentLinkBackgroundColor=`".*?`""
            $contents = Replace-AllInstances $contents "\s*?<\s*?Setter\s*?Property\s*?=\s*?`"ContentLinkForegroundColor`" Value=`"[^`"]+`"\s*?/>"
            $contents = Replace-AllInstances $contents "\s*?<\s*?Setter\s*?Property\s*?=\s*?`"ContentLinkBackgroundColor`" Value=`"[^`"]+`"\s*?/>"
            $contents = Replace-AllInstances $contents "<\s*Setter\s*Target=`"(.*?media:RevealBrush.State[^`"]*?)`"\s+Value=`"([^`"]+?)`"\s*/>" "<revealBrushPresent:Setter Target=`"`$1`" Value=`"`$2`" />"

            if ($contents.Contains("<revealBrushPresent") -and -not $contents.Contains("xmlns:revealBrushPresent"))
            {
                # Don't use Replace-AllInstances because this will also match the replaced string, and we only need/want to do this once.
                $contents = $contents -ireplace "<(ResourceDictionary[^>]*?xmlns=`"http:\/\/schemas.microsoft.com\/winfx\/2006\/xaml\/presentation`"[^>]*?)>", "<`$1 xmlns:revealBrushPresent=`"http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypePresent(Microsoft.UI.Xaml.Media.RevealBrush)`">"
            }

            if ($originalContents -ne $contents)
            {
                [System.IO.File]::WriteAllText($outputXamlFilePath, $contents, $encoding)
            }

            Write-Host ""
        }

        $xamlFilesHandled.AddRange($winUI2XamlFiles)
    }
}

# There are some WinUI 3 specific changes made to XAML files, which this merging will overwrite.
# To account for that, we'll additionally cherry-pick a Git change that reapplies those changes.
Write-Host "Committing XAML changes..."
& git commit $xamlFilesPattern -m "Merging XAML files from WinUI 2 repo."

Write-Host "Applying WinUI 3 specific changes to XAML files..."
& git cherry-pick bdcbad56f1c9f853eb80dd513d1a25a2588ec6eb