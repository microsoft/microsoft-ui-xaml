
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

param
(
    [String] $SourceDirectory
)

$ErrorActionPreference = "Stop"

# Feed source for compiler package restore.
# The LKG compiler package is expected to come from the LKG compiler feed; require it explicitly.
if (-not $env:WINUILKGCOMPILERFEEDURI) {
    throw "WINUILKGCOMPILERFEEDURI must be set to the LKG compiler feed URI for this script to run."
}

$feedSource = $env:WINUILKGCOMPILERFEEDURI

nuget install $env:LkgVcToolsName -version $env:LkgVcToolsVersion -source $feedSource -ConfigFile "$repoRoot\nuget.config"

