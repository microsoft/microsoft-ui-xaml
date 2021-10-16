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
        [AllowEmptyString()]
        [String[]]$template
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

function ExtractName {
    param (
        [Parameter(Mandatory = $true)]
        [string]$VisualStateGroup
    )
    
    if($VisualStateGroup.Contains('x:Name="'))
    {
        $VisualStateGroup -match 'x\:Name=".*?"' | Out-Null
        $nameDelcaration = $matches[0]
        $nameDelcaration.Substring(8, $nameDelcaration.Length-9)
    }
}

function GetVisualStateNamesFromVisualStateGroup {
    param (
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string[]]$VisualStateGroup
    )

    function ExtractName {
        param (
            [Parameter(Mandatory = $true)]
            [string]$VisualStateGroup
        )
        
        if($VisualStateGroup.Contains('x:Name="'))
        {
            $VisualStateGroup -match 'x\:Name=".*?"' | Out-Null
            $nameDelcaration = $matches[0]
            $nameDelcaration.Substring(8, $nameDelcaration.Length-9)
        }
    }

    $VisualStateNames = New-Object -TypeName 'System.Collections.ArrayList'
    foreach($line in $VisualStateGroup)
    {
        if($line.Contains('<VisualState '))
        {
            $name = ExtractName($line)
            $VisualStateNames.Add($name) | Out-Null
        }
    }
    return $VisualStateNames
}

function GetNamedTemplatePartsFromTemplate {
    param (
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string[]]$template
    )

    function ExtractName {
        param (
            [Parameter(Mandatory = $true)]
            [string]$line
        )
        
        if($line.Contains('x:Name="'))
        {
            $line -match 'x\:Name=".*?"' | Out-Null
            $nameDelcaration = $matches[0]
            $nameDelcaration.Substring(8, $nameDelcaration.Length-9)
        }
    }

    $Names = New-Object -TypeName 'System.Collections.ArrayList'
    foreach($line in $template)
    {
        if($line)
        {
            if(-Not ($line.Trim().StartsWith('<VisualState')))
            {
                $name = ExtractName $line
                if($name)
                {
                    $Names.Add($name) | Out-Null
                }
            }
        }
    }
    return $Names
}

function OutputBoilerPlate {
    param (
        [Parameter(Mandatory = $true)]
        [string]$OutputFile
    )
    if(Test-Path $OutputFile -PathType Leaf)
    {
        Add-Content -Path $OutputFile -Value '// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// Generated by microsoft-ui-xaml/tools/GenerateTemplateHelpers.ps1
#include "pch.h"
#include "common.h"'
    }
    else{
        throw "OutputFile does not exist"
    }
}

function OutputEnumClass {
    param (
        [Parameter(Mandatory = $true)]
        [string]$OutputFile,
        [Parameter(Mandatory = $true)]
        [string]$EnumName,
        [Parameter(Mandatory = $true)]
        [string[]]$EnumValueNames
    )
    if(-Not (Test-Path $OutputFile -PathType Leaf))
    {
        throw "OutputFile does not exist"
    }
    if(-Not ($EnumName))
    {
        throw "EnumName must be not null or empty"
    }
    if($EnumValueNames.Count -eq 0)
    {
        throw "Need at least one EnumValueName"
    }

    Add-Content -Path $OutputFile -Value "
enum class $EnumName
{"
    for($i=0; $i -lt $EnumValueNames.Count - 1; $i++)
    {
        Add-Content -Path $OutputFile -Value "    $($EnumValueNames[$i]),"
    }
    Add-Content -Path $OutputFile -Value "    $($EnumValueNames[$EnumValueNames.Count - 1])"
    Add-Content -Path $OutputFile -Value '};'
}

function OutputNamespaceBoilerPlate {
    param (
        [Parameter(Mandatory = $true)]
        [string]$OutputFile,
        [Parameter(Mandatory = $true)]
        [string]$NamespaceName
    )
    if(-Not (Test-Path $OutputFile -PathType Leaf))
    {
        throw "OutputFile does not exist"
    }
    if(-Not ($NamespaceName))
    {
        throw "NamespaceName must be not null or empty"
    }

    Add-Content -Path $OutputFile -Value "
namespace $NamespaceName
{"
}

function OutputVisualStateGroupBlock{
    param (
        [Parameter(Mandatory = $true)]
        [string]$OutputFile,
        [Parameter(Mandatory = $true)]
        [string]$VisualStateGroupEnumName,
        [Parameter(Mandatory = $true)]
        [string[]]$EnumValueNames
    )
    if(-Not (Test-Path $OutputFile -PathType Leaf))
    {
        throw "OutputFile does not exist"
    }
    if(-Not ($VisualStateGroupEnumName))
    {
        throw "EnumName must be not null or empty"
    }
    if($EnumValueNames.Count -eq 0)
    {
        throw "Need at least one EnumValueName"
    }
    Add-Content -Path $OutputFile -Value ""
    Add-Content -Path $OutputFile -Value "#pragma region $VisualStateGroupEnumName"
    Add-Content -Path $OutputFile -Value "    static winrt::hstring ToString($VisualStateGroupEnumName state)"
    Add-Content -Path $OutputFile -Value "    {"
    Add-Content -Path $OutputFile -Value "        switch (state)"
    Add-Content -Path $OutputFile -Value "        {"
    foreach($name in $EnumValueNames)
    {
        Add-Content -Path $OutputFile -Value "        case $VisualStateGroupEnumName::$($name):"
        Add-Content -Path $OutputFile -Value "            return L`"$name`";"
    }
    Add-Content -Path $OutputFile -Value "        default:"
    Add-Content -Path $OutputFile -Value "            return L`"`";"
    Add-Content -Path $OutputFile -Value "        }"
    Add-Content -Path $OutputFile -Value "    }"
    Add-Content -Path $OutputFile -Value ""
    Add-Content -Path $OutputFile -Value "    static bool GoToState(const winrt::Control& control, $VisualStateGroupEnumName state, bool useTransitions = true)"
    Add-Content -Path $OutputFile -Value "    {"
    Add-Content -Path $OutputFile -Value "        return winrt::VisualStateManager::GoToState(control, ToString(state), useTransitions);"
    Add-Content -Path $OutputFile -Value "    }"
    Add-Content -Path $OutputFile -Value "#pragma endregion"
}

function OutputNamedTemplatePartsRegion {
    param (
        [Parameter(Mandatory = $true)]
        [string]$OutputFile,
        [Parameter(Mandatory = $true)]
        [string]$ControlName,
        [Parameter(Mandatory = $true)]
        $NamedTemplateParts
    )
    
    if(-Not (Test-Path $OutputFile -PathType Leaf))
    {
        throw "OutputFile does not exist"
    }
    if(-Not ($ControlName))
    {
        throw "ControlName must be not null or empty"
    }
    if(-Not ($NamedTemplateParts.Count -eq 0))
    {
        Add-Content -Path $OutputFile -Value ""
        Add-Content -Path $OutputFile -Value "#pragma region NamedTemplateParts"
        Add-Content -Path $OutputFile -Value "    static winrt::hstring ToString($($ControlName)NamedTemplatePart part)"
        Add-Content -Path $OutputFile -Value "    {"
        Add-Content -Path $OutputFile -Value "        switch (part)"
        Add-Content -Path $OutputFile -Value "        {"
        foreach($name in $NamedTemplateParts)
        {
            Add-Content -Path $OutputFile -Value "        case $($ControlName)NamedTemplatePart::$($name):"
            Add-Content -Path $OutputFile -Value "            return L`"$name`";"
        }
        Add-Content -Path $OutputFile -Value "        default:"
        Add-Content -Path $OutputFile -Value "            return L`"`";"
        Add-Content -Path $OutputFile -Value "        }"
        Add-Content -Path $OutputFile -Value "    }"
        Add-Content -Path $OutputFile -Value ""
        Add-Content -Path $OutputFile -Value "    template<typename WinRTReturn>"
        Add-Content -Path $OutputFile -Value "    WinRTReturn GetTemplatePart(tracker_ref<WinRTReturn>& tracker, $($ControlName)NamedTemplatePart namedTemplatePart, const winrt::IControlProtected& control)"
        Add-Content -Path $OutputFile -Value "    {"
        Add-Content -Path $OutputFile -Value "        auto const part = GetTemplateChildT<WinRTReturn>(ToString(namedTemplatePart), control);"
        Add-Content -Path $OutputFile -Value "        tracker.set(part);"
        Add-Content -Path $OutputFile -Value "        return part;"
        Add-Content -Path $OutputFile -Value "    }"
        Add-Content -Path $OutputFile -Value "#pragma endregion"
    }
}

$template = GetTemplateFromFile $inputFile $controlName
$tempFile = WriteToTempFile $template
$VisualStateGroups = @(GetVisualStateGroupsFromTemplate $tempFile)

$path = ".\" + $controlName + "TemplatePartHelpers.h"
if(Test-Path $path -PathType Leaf)
{
    Write-Host "Deleting previous $path"
    Remove-Item $path
}
$output = New-Item -ItemType File -Path $path
$outputFilePath = $output.FullName

OutputBoilerPlate $outputFilePath;

$namedTemplateParts = GetNamedTemplatePartsFromTemplate $template
OutputEnumClass $outputFilePath "$($controlName)NamedTemplatePart" $namedTemplateParts

foreach($visualStateGroup in $VisualStateGroups)
{
    $groupName = ExtractName $visualStateGroup[0]
    $stateNames = GetVisualStateNamesFromVisualStateGroup $VisualStateGroup
    OutputEnumClass $outputFilePath "$controlName$groupName" $stateNames
}

OutputNamespaceBoilerPlate $outputFilePath "$($controlName)TemplateHelpers"

foreach($visualStateGroup in $VisualStateGroups)
{
    $groupName = ExtractName $visualStateGroup[0]
    $stateNames = GetVisualStateNamesFromVisualStateGroup $VisualStateGroup
    OutputVisualStateGroupBlock $outputFilePath "$controlName$groupName" $stateNames
}

OutputNamedTemplatePartsRegion $outputFilePath $controlName $namedTemplateParts

Add-Content -Path $outputFilePath -Value "};"

Write-Host "Finished writing $path"
