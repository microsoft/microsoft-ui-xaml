# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [string]$CoverageToolPath
)

$ErrorActionPreference = 'Stop'

if ($CoverageToolPath)
{
    return (Get-Item -LiteralPath $CoverageToolPath).FullName
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$tool = & $vswhere -latest -products * -find 'Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe' |
    Select-Object -First 1
if (-not $tool)
{
    throw 'Microsoft.CodeCoverage.Console.exe is required. Use a Visual Studio image with C++ code coverage support.'
}

# dotnet-coverage alone does not support native instrumentation.
return $tool
