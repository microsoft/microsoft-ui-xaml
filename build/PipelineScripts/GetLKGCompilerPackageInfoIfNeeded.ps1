# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

param
(
    [String] $SourceDirectory
)

$ErrorActionPreference = "Stop"

# If $env:LkgVcToolsName and $env:LkgVcToolsVersion are not fully specified, try to retrieve the defaults from WindowsAppSDK-GlobalVariables.yml.
if (($env:LkgVcToolsName -eq $null) -or ($env:LkgVcToolsVersion -eq $null))
{
    $Records = Get-Content $SourceDirectory\build\AzurePipelinesTemplates\WinUI-BuildVariables.yml
    For ($i = 0; $i -lt $Records.count; $i++)
    {
        if ($env:LkgVcToolsName -eq $null)
        {
            $Target1 = $Records[$i] -split "compilerOverridePackageName:"
            if (($Target1.count -gt 1) -and ($Target1[1] -ne $null))
            {
                $env:LkgVcToolsName = $Target1[1].trim(' ')
            }
        }

        if ($env:LkgVcToolsVersion -eq $null)
        {
            $Target2 = $Records[$i] -split "compilerOverrideNupkgVersion:"
            if (($Target2.count -gt 1) -and ($Target2[1] -ne $null))
            {
                $env:LkgVcToolsVersion = $Target2[1].trim(' ')
            }
        }
    }
    Write-Host "Using LKG package name and version:" $env:LkgVcToolsName $env:LkgVcToolsVersion 
}
