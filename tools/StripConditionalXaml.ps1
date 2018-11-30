# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
Param(
    [Parameter(Mandatory = $true)] 
    [ValidateNotNullOrEmpty()]
    [string]$ThemeResourceInputFile,

    [string]$ThemeResourceOutputFile,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [ValidateSet('RS1', 'RS2', 'RS3', 'RS4', 'RS5', '19H1', 'OS')]
    [String]$ForVersion
)

# exit whenever see a exception in the script
trap 
{ 
  write-output $_ 
  exit 1 
} 

$ForVersion = $ForVersion.ToUpper()
$ApiContractMapping = @{RS1=3; RS2=4; RS3=5; RS4=6; RS5=7; '19H1'=8}  # OS always max+1
$ApiContractMaxVersion = ($ApiContractMapping.Values | measure -Maximum).Maximum
$ApiContractMapping['OS'] = $ApiContractMaxVersion + 1

function Get-ApiContractVersion
{
    Param($key)
    if ($ApiContractMapping.ContainsKey($key))
    {
        return $ApiContractMapping[$key]
    }
    Write-Error "Can't find Key: " + $key 
    eixt 2
}


function Build-ApiContractPresentPrefix
{
    Param($apiVersion)
    return [string]::Format('contract{0}Present', $apiVersion)
}

function Build-ApiContractNotPresentPrefix
{
    Param($apiVersion)
    return [string]::Format('contract{0}NotPresent', $apiVersion)
}

function Build-MuxOnlyPrefix
{
    return 'MUXOnly';
}


# xmlDocument.Load/Save converted &#xE0E5 to uncode character and it's hard to recover it to the orignal string
# replace the "escape characters" in the form &#xE0E5 in loading and convert them back in saving
function Get-XmlFile
{
    Param($file)
    return (Get-Content $file -Raw).Replace("&", "&amp;")
}

function Save-XmlFile
{
    Param($file,$content) 
    $content.Replace("&amp;","&") |Set-Content $file
}

function Remove-ElementsOrPropertiesRelatedToNamespacePrefixes
{
    Param($content, $prefixesToRemove)
    foreach ($prefixToRemove in $prefixesToRemove)
    {
    # xmlns:contract5Present="http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"
        $content = $content -replace "(?m)xmlns:$prefixToRemove=`"[^`"]+`"", ""

    #   <Rectangle x:Name="colorRectangle" Width="200" Height="200"
    #      contract5NotPresent:Fill="{x:Bind ((SolidColorBrush)((FrameworkElement)colorComboBox.SelectedItem).Tag), Mode=OneWay}">
    #
        $content = $content -replace "(?m)\s$prefixToRemove\:\w+?=`"[^`"]+`"", ""

    #    <contract5Present:ColorPicker x:Name="colorPicker"
    #                                  Grid.Column="1"
    #                                  VerticalAlignment="Center"/>
        $content = $content -replace "(?m)<$prefixToRemove\:[^>]*/>",""

    #     <contract5NotPresent:ComboBox x:Name="colorComboBox"
    #                                  PlaceholderText="Pick a color"
    #                                  Grid.Column="1"
    #                                  VerticalAlignment="Center">
    #    </contract5NotPresent:ComboBox>
        $content = $content -replace "(?m)<$prefixToRemove\:[.\S\s]*?</$prefixToRemove\:[^>]*>","" 
    }
    return $content
}

function Remove-NamespacePrefixes
{
    Param($content, $prefixesToRemove)
    foreach ($prefixToRemove in $prefixesToRemove)
    {
    # xmlns:contract5Present="http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"
        $content = $content -replace "(?m)xmlns:$prefixToRemove=`"[^`"]+`"", ""

    #    <contract5NotPresent:ComboBox
    #       ...
    #    </contract5NotPresent:ComboBox>
        $content = $content -replace "$prefixToRemove\:", ""
    }

    return $content
}

function Main
{
    Write-Host "StripConditionalXaml $ThemeResourceInputFile for $ForVersion"
    if (!(Test-Path $ThemeResourceInputFile)) 
    {
        Write-Error "File $ThemeResourceInputFile doesn't exist"
        exit 1
    }

    if (!$ThemeResourceOutputFile)
    {
        $ThemeResourceOutputFile = $ThemeResourceInputFile
    }

    if (Test-Path $ThemeResourceOutputFile) 
    {
        Write-Host "File $ThemeResourceOutputFile is overwritten"
    }

    $curVersion = Get-ApiContractVersion $ForVersion

    # generate prefixes which nodes or properties will be removed 
    $prefixesForNodesOrPropertiesToBeRemoved = New-Object System.Collections.ArrayList
    # ContractPresent for release > cur
    for ($i = $curVersion + 1; $i -le $ApiContractMaxVersion; $i++)
    {
        $prefixesForNodesOrPropertiesToBeRemoved.Add((Build-ApiContractPresentPrefix $i)) | Out-Null;
    }

    # NotContractPresent for release <= cur
    for ($i = 3; $i -le $curVersion; $i++)
    {
        $prefixesForNodesOrPropertiesToBeRemoved.Add((Build-ApiContractNotPresentPrefix $i)) | Out-Null;
    }

    # generate prefixes which will be replaced with empty
    $prefixesToBeRemoved = New-Object System.Collections.ArrayList
    # ContractPresent for release <= cur 
    for ($i = 3; $i -le $curVersion; $i++)
    {
        $prefixesToBeRemoved.Add((Build-ApiContractPresentPrefix $i)) | Out-Null;
    }

    # NotContractPresent for release > cur
    for ($i = $curVersion + 1; $i -le $ApiContractMaxVersion; $i++)
    {
        $prefixesToBeRemoved.Add((Build-ApiContractNotPresentPrefix $i)) | Out-Null;
    }

    # In Os side, MUXOnly
    $MUXOnlyNamespacePrefix = Build-MuxOnlyPrefix
    if ("OS" -ne $ForVersion)
    {
         $prefixesToBeRemoved.Add($MUXOnlyNamespacePrefix) | Out-Null;
    }
    else
    {
        $prefixesForNodesOrPropertiesToBeRemoved.Add($MUXOnlyNamespacePrefix) | Out-Null;
    }

    Write-Host "Nodes or properties related to below prefixes will be removed" 
    write-Host -Separator " " "  " $prefixesForNodesOrPropertiesToBeRemoved
    Write-Host "Below prefixes will be removed"
    write-Host  -Separator " " "  " $prefixesToBeRemoved

    $content = Get-XmlFile $ThemeResourceInputFile 
    $content = Remove-ElementsOrPropertiesRelatedToNamespacePrefixes $content $prefixesForNodesOrPropertiesToBeRemoved
    $content = Remove-NamespacePrefixes $content $prefixesToBeRemoved

    Save-XmlFile $ThemeResourceOutputFile $content
}

Main