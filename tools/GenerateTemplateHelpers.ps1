Param(
    [Parameter(Mandatory = $true)]
    [String]$inputFile,
    [Parameter(Mandatory = $true)]
    [String]$controlName
)

# $input = Get-Item -Path $inputFile
# $output = New-Item -ItemType File -Path (".\" + $ControlName + "TemplatePartHelpers.h")
# $outputFilePath = $output.FullName

# '// Copyright (c) Microsoft Corporation. All rights reserved.
# // Licensed under the MIT License. See LICENSE in the project root for license information.
# #include "pch.h"
# #include "common.h"' | Out-File $outputFilePath

function GetTemplateFromFile {
    param (
        [Parameter(Mandatory = $true)]
        [String]$inputFile,
        [Parameter(Mandatory = $true)]
        [String]$controlName
    )
    $file = Get-Item $inputFile
    $templateStartLine = (Select-String -Path $file -Pattern ('<ControlTemplate TargetType="local:' + $controlName) -List).LineNumber;
    if($templateStartLine -lt 1)
    {
        throw "File doesn't contain control template"
    }

    $templateEndLine = (Select-String -Path $file -Pattern ('</ControlTemplate>') -List).LineNumber;
    if($templateEndLine -lt 1)
    {
        throw "Template is malformed"
    }

    return Get-Content $file | Select-Object -First $templateEndLine | Select-Object -Last ($templateEndLine - $templateStartLine + 1)
}

function New-TemporaryDirectory {
    $parent = [System.IO.Path]::GetTempPath()
    $name = [System.IO.Path]::GetRandomFileName()
    New-Item -ItemType Directory -Path (Join-Path $parent $name)
}

function WriteToTempFile {
    param (
        [Parameter(Mandatory = $true)]
        [String]$template
    )
    
    $tempDir = New-TemporaryDirectory
    $tempFilePath = Join-Path $tempDir.FullName "template.xaml"
    Set-Content $tempFilePath -value $template
    return Get-Item $tempFilePath
}

function GetVisualStateGroupsFromTemplate {
    param (
        [Parameter(Mandatory = $true)]
        [string]$templateFile
    )
    
    $template = Get-Item $templateFile
    $VisualStateGroupStartLines = (Select-String -Path $template -Pattern ('<VisualStateGroup ')).LineNumber;
    $VisualStateGroupEndLines = (Select-String -Path $template -Pattern ('</VisualStateGroup>')).LineNumber;

    if($VisualStateGroupStartLines.count -ne $VisualStateGroupEndLines.count)
    {
        throw "unequal number of open and close visual state groups"
    }

    $VisualStateGroups = New-Object -TypeName 'System.Collections.ArrayList'
    for($i=0; $i -lt $VisualStateGroupStartLines.count; $i++)
    {
        $VisualStateGroupEndLine = $VisualStateGroupEndLines[$i];
        $VisualStateGroupStartLine = $VisualStateGroupStartLines[$i];
        $visualStateGroup = Get-Content $template | Select-Object -First $VisualStateGroupEndLine | Select-Object -Last ($VisualStateGroupEndLine - $VisualStateGroupStartLine + 1)
        $VisualStateGroups.add($visualStateGroup) | Out-Null
    }
    return $visualStateGroups
}

#Write-Host $inputFile
#GetTemplateFromFile $inputFile $ControlName