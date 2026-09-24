# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# Source-contract checks; these do not replace live theme-switching and visual tests.
param(
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

[xml]$styles = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'controls\dev\TableView\TableView.xaml') -Raw
[xml]$resources = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'controls\dev\CommonStyles\TabularSurfaces_themeresources.xaml') -Raw
[xml]$imports = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'controls\Tabular.ProjectImports.targets') -Raw
$namespaces = [System.Xml.XmlNamespaceManager]::new($styles.NameTable)
$namespaces.AddNamespace('p', 'http://schemas.microsoft.com/winfx/2006/xaml/presentation')
$namespaces.AddNamespace('x', 'http://schemas.microsoft.com/winfx/2006/xaml')
$failures = [System.Collections.Generic.List[string]]::new()

function Assert-Contract([bool]$Condition, [string]$Message)
{
    if (-not $Condition)
    {
        $failures.Add($Message)
    }
}

$localTokens = $styles.SelectNodes('/p:ResourceDictionary/*[starts-with(@x:Key, "TabularSurface")]', $namespaces)
Assert-Contract ($localTokens.Count -eq 0) 'Default styles must not shadow the canonical themed TabularSurface resources.'

$resourcePage = $imports.SelectSingleNode('//*[local-name()="Page" and contains(@Include, "CommonStyles\TabularSurfaces_themeresources.xaml")]')
Assert-Contract ($null -ne $resourcePage -and $resourcePage.Type -eq 'ThemeResources') 'The Tabular binary must include the canonical theme dictionary.'

$usedKeys = @([regex]::Matches($styles.OuterXml, '\{ThemeResource (TabularSurface\w+)\}') |
    ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
foreach ($theme in @('Default', 'Light', 'HighContrast'))
{
    $dictionary = $resources.SelectSingleNode("//p:ResourceDictionary[@x:Key='$theme']", $namespaces)
    Assert-Contract ($null -ne $dictionary) "Missing $theme dictionary."
    if ($null -eq $dictionary) { continue }

    foreach ($key in $usedKeys)
    {
        $entries = $dictionary.SelectNodes("*[@x:Key='$key']", $namespaces)
        Assert-Contract ($entries.Count -eq 1) "$theme must define $key exactly once."
        if ($theme -eq 'HighContrast' -and $entries.Count -eq 1 -and $entries[0].LocalName -eq 'SolidColorBrush')
        {
            Assert-Contract ($entries[0].GetAttribute('Color') -match '^\{ThemeResource SystemColor\w+\}$') "HighContrast $key must use a system color, not a Fluent color sentinel."
        }
    }
}

$rowStyle = $styles.SelectSingleNode('//p:Style[@TargetType="controls:TableViewRow"]', $namespaces)
$stateTokens = [ordered]@{
    PointerOver = 'PointerOver'
    Pressed = 'Pressed'
    Disabled = 'Disabled'
    Selected = 'Selected'
    SelectedPointerOver = 'SelectedPointerOver'
    SelectedPressed = 'SelectedPressed'
    SelectedDisabled = 'Disabled'
}
$highContrast = $resources.SelectSingleNode('//p:ResourceDictionary[@x:Key="HighContrast"]', $namespaces)
foreach ($state in $stateTokens.Keys)
{
    $suffix = $stateTokens[$state]
    $visualState = $rowStyle.SelectSingleNode(".//p:VisualState[@x:Name='$state']", $namespaces)
    foreach ($property in @('Background', 'Foreground'))
    {
        $target = if ($property -eq 'Background') { 'PART_RootBorder' } else { 'PART_CellForegroundPresenter' }
        $key = "TabularSurfaceRow$property${suffix}Brush"
        $frame = $visualState.SelectSingleNode(".//p:ObjectAnimationUsingKeyFrames[@Storyboard.TargetName='$target' and @Storyboard.TargetProperty='$property']/p:DiscreteObjectKeyFrame", $namespaces)
        Assert-Contract ($null -ne $frame -and $frame.GetAttribute('Value') -eq "{ThemeResource $key}") "$state must apply $key to $target.$property."

        $expectedColor = if ($suffix -eq 'Disabled')
        {
            if ($property -eq 'Background') { 'SystemColorWindowColor' } else { 'SystemColorGrayTextColor' }
        }
        else
        {
            if ($property -eq 'Background') { 'SystemColorHighlightColor' } else { 'SystemColorHighlightTextColor' }
        }
        $brush = $highContrast.SelectSingleNode("*[@x:Key='$key']", $namespaces)
        Assert-Contract ($null -ne $brush -and $brush.GetAttribute('Color') -eq "{ThemeResource $expectedColor}") "$state.$property must resolve to $expectedColor in high contrast."
    }
}

$presenter = $rowStyle.SelectSingleNode('.//p:ContentPresenter[@x:Name="PART_CellForegroundPresenter"]', $namespaces)
Assert-Contract ($presenter.GetAttribute('Foreground') -eq '{TemplateBinding Foreground}') 'Normal state must restore the row foreground, including an application override.'
$normal = $rowStyle.SelectSingleNode('.//p:VisualState[@x:Name="Normal"]', $namespaces)
Assert-Contract ($normal.ChildNodes.Count -eq 0) 'Normal state must release the state animations.'

if ($failures.Count -gt 0)
{
    throw ($failures -join [Environment]::NewLine)
}
Write-Output "PASS: $($usedKeys.Count) TableView tokens in three themes; seven state color pairs; normal-state foreground restoration; canonical dictionary included."
