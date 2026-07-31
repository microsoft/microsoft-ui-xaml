# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
    Find an installed appx package

.DESCRIPTION
    Find an appx package by matching a string against it's package family name and
    its install location. The app name usually shows up in one of those properties.
    For example, an app you create in VS will deploy the app to the solution
    directory, so you can use the solution name.

    For example, this finds an app under development in VS in
    <userpath>\repos\MySampleApp

        find-appx mysample

#>

param(
    [Parameter(Mandatory=$true)]
    [string]$searchString)

$searchString = $searchString.ToLower()

get-appxpackage | ? {($_.PackageFullName.ToLower().Contains($searchString)) -or ($_.InstallLocation -ne $null -and $_.InstallLocation.ToLower().Contains($searchString))}

