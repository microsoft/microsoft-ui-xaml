[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)]
    [string]$controlName
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
$controlDir = $muxControlsDir + "\dev\$controlName"

$newDir = New-Item $controlDir -ItemType Directory
if (!$newDir)
{
    Write-Error "Control directory could not be created."
    exit 1
}

$from = $toolsDir + "\GenerateNewControlProjectFiles\*"
Copy-Item $from $controlDir -Recurse

# Replace NEWCONTROL with $controlName in file names
Get-ChildItem -Path $controlDir -Filter "*NEWCONTROL*" -Recurse | Rename-Item -NewName {$_.name -replace 'NEWCONTROL', $controlName }

# Replace NEWCONTROL with $controlName in file contents
$files = Get-ChildItem -Path $controlDir -Recurse -File
foreach ($file in $files)
{
    (Get-Content $file.PSPath) |
    Foreach-Object { $_ -replace "NEWCONTROL", $controlName } |
    Set-Content $file.PSPath
}

# Add project to MUX.vcxproj
$muxProject = $muxControlsDir + "\dev\dll\Microsoft.UI.Xaml.vcxproj";
[xml]$xml = Get-Content $muxProject
foreach ($group in $xml.Project.ImportGroup)
{
    if ($group.Attributes['Label'].Value.Equals("Shared"))
    {
        $import = $xml.CreateElement("Import", $xml.Project.NamespaceURI);
        AddAttribute $xml $import "Project" "..\$controlName\$controlName.vcxitems"
        AddAttribute $xml $import "Label" "Shared"
        AddAttribute $xml $import "Condition" "`$(BuildLeanMuxForTheStoreApp) != 'true'"
        $group.AppendChild($import);        
    }
}
$xml.Save($muxProject)

# Add tests to MUXControls.Test.Shared.targets
$testProject = $muxControlsDir + "\test\MUXControls.Test\MUXControls.Test.Shared.targets";
[xml]$xml = Get-Content $testProject
$import = $xml.CreateElement("Import", $xml.Project.NamespaceURI);
AddAttribute $xml $import "Project" "`$(MSBuildThisFileDirectory)\..\..\dev\$controlName\InteractionTests\$($controlName)_InteractionTests.projitems"
AddAttribute $xml $import "Label" "Shared"
AddAttribute $xml $import "Condition" "`$(BuildLeanMuxForTheStoreApp) != 'true'"
$xml.Project.AppendChild($import);
$xml.Save($testProject)

# Add test page to MUXControlsTestApp.Shared.targets
$testAppProject = $muxControlsDir + "\test\MUXControlsTestApp\MUXControlsTestApp.Shared.targets";
[xml]$xml = Get-Content $testAppProject
$import = $xml.CreateElement("Import", $xml.Project.NamespaceURI);
AddAttribute $xml $import "Project" "`$(MSBuildThisFileDirectory)\..\..\dev\$controlName\TestUI\$($controlName)_TestUI.projitems"
AddAttribute $xml $import "Label" "Shared"
AddAttribute $xml $import "Condition" "`$(BuildLeanMuxForTheStoreApp) != 'true'"
$xml.Project.AppendChild($import);
$xml.Save($testAppProject)

# Add .idl to main idl file
FindAndReplaceInFile ($muxControlsDir + "\idl\Microsoft.UI.Xaml.idl") "(#ifndef BUILD_LEAN_MUX_FOR_THE_STORE_APP)([.\S\s]*?)(#endif)" @"
`$1`$2#include <$controlName\$controlName.idl>
`$3
"@

# Add header file to XamlMetadataProviderGenerated.tt
FindAndReplaceInFile ($muxControlsDir + "\dev\dll\XamlMetadataProviderGenerated.tt") "#endif" @"
#include "$controlName.h"
#endif
"@

# Add new profiler id to RuntimeProfiler.h
FindAndReplaceInFile ($muxControlsDir + "\dev\Telemetry\RuntimeProfiler.h") "(\s*ProfId_Size\s*})" @"

        ProfId_$controlName,`$1
"@

# Add page to TestInventory.cs
FindAndReplaceInFile ($muxControlsDir + "\test\MUXControlsTestApp\TestInventory.cs") "#endif" @"
            Tests.Add(new TestDeclaration("$controlName Tests", typeof($($controlName)Page)));
#endif
"@
