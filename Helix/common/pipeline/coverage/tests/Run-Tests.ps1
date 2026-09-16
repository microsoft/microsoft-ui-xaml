# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Runs the standalone Pester coverage regression suite and returns failure if any test fails.
# Invoke in a fresh powershell.exe or pwsh.exe process because the tests load an inert ACL type.
$ErrorActionPreference = 'Stop'
Import-Module Pester -RequiredVersion 3.4.0
$result = Invoke-Pester -Script "$PSScriptRoot\Coverage.Tests.ps1" -PassThru
if ($result.FailedCount -gt 0 -or $result.TotalCount -eq 0)
{
    exit 1
}
exit 0
