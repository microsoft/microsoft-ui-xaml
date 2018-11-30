param(
    [Parameter(Mandatory=$true, HelpMessage="Paths to the image files that need to be parsed and added to the system DLL resources.")]
    [string]$ImagePaths,

    [Parameter(Mandatory=$true, HelpMessage="Path to the file to write the resources to.")]
    [string]$OutputPath,

    [Parameter(Mandatory=$true, HelpMessage="Path to the file to write the resource helper information to.")]
    [string]$ResourceHelperOutputPath
)

# We'll start this at 2000 to be sure that we don't collide with string resources, which start at 1000.
$currentIndex = 2000

$resourceDefinitionsString = @"
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// THIS FILE IS CREATED BY A SCRIPT!  DO NOT EDIT DIRECTLY!
// If you need to edit this file, edit GenerateSystemDllImageResources.ps1, which outputs its contents.
//

"@

$resourceHelperString = @"
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// THIS FILE IS CREATED BY A SCRIPT!  DO NOT EDIT DIRECTLY!
// If you need to edit this file, edit GenerateSystemDllImageResources.ps1, which outputs its contents.
//

#pragma once

"@

foreach ($imagePath in $ImagePaths.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries))
{
    $currentLocation = Get-Location
    Set-Location $PSScriptRoot\..\dev\dll
    $imagePath = (Resolve-Path $imagePath -Relative).Replace("\", "\\")
    Set-Location $currentLocation

    $imageName = [io.path]::GetFileNameWithoutExtension($imagePath).Replace("`"", "")

    if ($resourceDefinitionsString.Length -gt 0)
    {
        $resourceDefinitionsString += [Environment]::NewLine
    }

    $resourceDefinitionsString += "$currentIndex RCDATA `"$imagePath`""

    if ($resourceHelperString.Length -gt 0)
    {
        $resourceHelperString += [Environment]::NewLine
    }

    $resourceHelperString += "#define IR_$imageName $currentIndex"

    $currentIndex++
}

$resourceHelperString += [Environment]::NewLine

Out-File $OutputPath -InputObject $resourceDefinitionsString -Encoding ASCII
Out-File $ResourceHelperOutputPath -InputObject $resourceHelperString -Encoding ASCII