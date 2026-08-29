# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# Removes installed Windows App Runtime framework packages.
# Only removes packages which can be removed by user actions and not system installed packages like CBS, Inbox.

$allApps = Get-AppxPackage -AllUsers
 
$appsToRemove = @()
foreach($app in $allApps)
{
    foreach($dep in $app.Dependencies)
    {
        if(($dep.Name.Contains("WindowsAppRuntime") -or $dep.Name.Contains("WinAppRuntime")) `
                                                        -and -not $dep.Name.Contains("CBS") `
                                                        -and -not $dep.Name.Contains("Inbox"))
        {
            $appsToRemove += $app
        }
    }
}

foreach($app in $appsToRemove)
{
    Write-Host "Apps To Remove: $($app.Name)"
    Remove-AppxPackage $app -AllUsers
}


foreach($app in (Get-AppxPackage -Name *Win*AppRuntime* -AllUsers))
{
    if(-not $app.Name.Contains("CBS") -and -not $app.Name.Contains("Inbox"))
    {
        Write-Host "Framework Packages To Remove: $($app.Name)"
        Remove-AppxPackage $app -AllUsers
    }
}