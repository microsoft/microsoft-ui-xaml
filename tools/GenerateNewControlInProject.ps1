[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)]
    [string]$controlName,
    [string]$projectName
)

function FindAndReplaceInFile
{
    Param($file, $regex, $replace)

    $content = Get-Content $file -Raw

    [regex]$pattern = $regex
    $content = $pattern.replace($content, $replace, 1)

    Set-Content -Path $file -Value $content
}

function AddAttribute
{
    Param([xml]$xml, $element, $attrName, $attrValue)

    $attribute = $xml.CreateAttribute($attrName)
    $attribute.Value = $attrValue;
    $element.Attributes.Append($attribute)
}

$toolsDir = Split-Path -Path $MyInvocation.MyCommand.Path;
$muxControlsDir = Split-Path $toolsDir -Parent
$controlDir = $muxControlsDir + "\dev\$projectName"

# TODO: check if project directory exists

Copy-Item "$toolsDir\GenerateNewControlProjectFiles\NEWCONTROL.cpp" "$controlDir\$controlName.cpp"
Copy-Item "$toolsDir\GenerateNewControlProjectFiles\NEWCONTROL.h"   "$controlDir\$controlName.h"

# Replace NEWCONTROL with $controlName in file contents
$files = "$controlDir\$controlName.cpp", "$controlDir\$controlName.h"
foreach ($file in $files)
{
    $file
    (Get-Content $file) |
    Foreach-Object { $_ -replace "NEWCONTROL", $controlName } |
    Set-Content $file
}

# TODO: Add to idl file

# Add project to ProjectName.vcxitems
$controlProject = $controlDir + "\$projectName.vcxitems";
[xml]$xml = Get-Content $controlProject
foreach ($group in $xml.Project.ItemGroup)
{
    if ($group.ChildNodes.item(0).Name.Equals("ClInclude"))
    {
        $include = $xml.CreateElement("ClInclude", $xml.Project.NamespaceURI);
        AddAttribute $xml $include "Include" "`$(MSBuildThisFileDirectory)$controlName.h"
        $group.AppendChild($include);        
    }
    elseif ($group.ChildNodes.item(0).Name.Equals("ClCompile"))
    {
        $include = $xml.CreateElement("ClCompile", $xml.Project.NamespaceURI);
        AddAttribute $xml $include "Include" "`$(MSBuildThisFileDirectory)$controlName.cpp"
        $group.AppendChild($include);    
        
        $include = $xml.CreateElement("ClCompile", $xml.Project.NamespaceURI);
        AddAttribute $xml $include "Include" "`$(MSBuildThisFileDirectory)..\Generated\$controlName.properties.cpp"
        $group.AppendChild($include);        
    }
}
$xml.Save($controlProject)

# Add header file to XamlMetadataProviderGenerated.tt
FindAndReplaceInFile ($muxControlsDir + "\dev\dll\XamlMetadataProviderGenerated.tt") "#endif" @"
#include "$controlName.h"
#endif
"@

# Add new profiler id to RuntimeProfiler.h
FindAndReplaceInFile ($muxControlsDir + "\dev\Telemetry\RuntimeProfiler.h") "(\s*ProfId_Size\s*})" @"

        ProfId_$controlName,`$1
"@
