param(
    [Parameter(Mandatory=$true, HelpMessage="Paths to the RESW files that need to be parsed and added to the system DLL resources.")]
    [string]$ReswPaths,

    [Parameter(Mandatory=$true, HelpMessage="Path to the file to write the resources to.")]
    [string]$OutputPath,

    [Parameter(Mandatory=$true, HelpMessage="Path to the file to write the resource helper information to.")]
    [string]$ResourceHelperOutputPath
)

[System.Collections.ArrayList]$reswDataObjectsToInclude = @()

$groupIndex = 0;

foreach ($reswPath in $ReswPaths.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries))
{
    # We only want to grab the en-US strings, since those are the inputs to our localization.
    if (-not $reswPath.ToLower().Contains("en-us"))
    {
        continue
    }

    $reswWithoutComments = ([regex]"<!--.*?-->").Replace((Get-Content $reswPath), "")
    $reswDataElements = ([regex]"<data.+?>.+?</data>").Matches($reswWithoutComments)
        
    $numInGroup = 0
    foreach ($reswDataElement in $reswDataElements)
    {
        # Need to split large groups because of #define nesting limit (somewhere around 50)
        if ($numInGroup -gt 50)
        {
            $numInGroup = 0
            $groupIndex++
        }

        $reswData = $reswDataElement.Groups[0].Value
        $nameMatch = ([regex]"name=`"(.+?)`"").Match($reswData)
        $valueMatch = ([regex]"<value>(.+?)</value>").Match($reswData)
        $commentMatch = ([regex]"<comment>(.+?)</comment>").Match($reswData)

        $propertyDictionary = 
        @{"Name"="$($nameMatch.Groups[1].Value)";
            "Value"="$($valueMatch.Groups[1].Value)";
            "Comment"="$($commentMatch.Groups[1].Value)"
            "GroupIndex"=$groupIndex}

        $reswDataObject = New-Object -TypeName PSObject -Property $propertyDictionary

        $reswDataObjectsToInclude.Add($reswDataObject) 2>&1> $null

        $numInGroup++
    }

    # Every file gets its own group
    $groupIndex++
}

$resourceDefinitionsString = @"
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// THIS FILE IS CREATED BY A SCRIPT!  DO NOT EDIT DIRECTLY!
// If you need to edit this file, edit GenerateSystemDllStringResources.ps1, which outputs its contents.
//

#include "$(Split-Path -Leaf $ResourceHelperOutputPath)"

"@

$resourceHelperString = @"
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// THIS FILE IS CREATED BY A SCRIPT!  DO NOT EDIT DIRECTLY!
// If you need to edit this file, edit GenerateSystemDllImageResources.ps1, which outputs its contents.
//

#pragma once

"@

$lastResourceId = "SR_FIRST"

# We'll start this at 1000 in case we need to add hard-coded resource strings for some reason.

$groupIndex = -1 # Start at -1 so we always emit the first group
$groupStart = 1000
$groupIncrement = 100

foreach ($reswDataObject in $reswDataObjectsToInclude)
{
    if ($reswDataObject.GroupIndex -ne $groupIndex)
    {
        $groupIndex = $reswDataObject.GroupIndex
        $groupIdBegin = $groupStart + ($groupIncrement * $groupIndex)
        $lastResourceId = "SR_GROUP$($groupIndex)"
        $resourceHelperString += [Environment]::NewLine + "#define $lastResourceId $($groupIdBegin)" + [Environment]::NewLine
    }

    $currentResourceId = "SR_$($reswDataObject.Name)"

    $resourceDefinitionsString += "$currentResourceId `"$($reswDataObject.Value)`""
        
    if ($reswDataObject.Comment.Length -gt 0)
    {
        $resourceDefinitionsString += " // $($reswDataObject.Comment)"
    }

    $resourceDefinitionsString += [Environment]::NewLine

    $resourceHelperString += "#define $($currentResourceId)  ($($lastResourceId) + 1) " + [Environment]::NewLine
    $lastResourceId = $currentResourceId
}

Out-File $OutputPath -InputObject $resourceDefinitionsString -Encoding ASCII
Out-File $ResourceHelperOutputPath -InputObject $resourceHelperString -Encoding ASCII