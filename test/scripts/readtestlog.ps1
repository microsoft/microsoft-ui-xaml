# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Run this on a test log from the lab to get some information about which tests
# are getting re-run and how long each test is taking.

param (
    [string]$logpath
)

$name = ''
$lastStart = $null
$lastEnd = $null
$lasttest = ''

$currentWorkItem = ''
$workItemStart = $null
$workItemEnd = $null
$longtests = @()
$failedtests = @()

function get-color {
    param (
        $seconds
    )
    if ($seconds -lt 1) { return "darkgray" } 
    if ($seconds -lt 10) { return "white" } 
    if ($seconds -lt 20) { return "yellow" } 
    return "red"
}

get-content $logpath |% {

    $line = $_

    if ($line -match " Running (\S*) work item$") {
        $currentWorkItem = $Matches[1]
        Write-Host "===== Work item $currentWorkItem" -ForegroundColor Cyan
        $workItemStart = (Get-Date $line.Split(" ")[0])
    }
    if ($line -match " WorkItemEndTime: ") {
        $workItemEnd = (Get-Date $line.Split(" ")[0])
        $elapsed = ($workItemEnd-$workItemStart).TotalSeconds
        Write-Host "===== $currentWorkItem $elapsed" -ForegroundColor Cyan
        Write-Host ""

        if ($elapsed -gt 300) {
            $longtests += "$currentWorkItem,$elapsed"
        }
    }


    if ($line -match " StartGroup: (\S*)$") {
        $lastStart = (Get-Date $line.Split(" ")[0])

        if ($lastend)
        {
            $elapsed = ($lastStart-$lastEnd).TotalSeconds
            if ($elapsed -gt 1) {
                Write-Host "----,Gap (after $lasttest),$elapsed" -ForegroundColor (get-color $elapsed)
            }
        }
    }
    if ($line -match " EndGroup: (\S*) (\S*)$") {
        $testname = $Matches[1]
        $passed = $Matches[2].Contains("Passed") ? "pass" : "FAIL"
        $lastEnd = (Get-Date $line.Split(" ")[0])
        $elapsed = ($lastEnd-$lastStart).TotalSeconds
        Write-Host "$passed,$testname,$elapsed" -ForegroundColor (get-color $elapsed)
        $lasttest = $testname
        if ($passed -ne "pass") {
            $failedtests += "$currentWorkItem / $testname"
        }
    }
}

Write-Host ""
if ($longtests.Length -gt 0) {
    Write-Host "Long-running tests:"
    $longtests | ft
}

if ($failedtests.Length -gt 0) {
    Write-Host ""
    Write-Host "Failed tests:"
    $failedtests | ft
}