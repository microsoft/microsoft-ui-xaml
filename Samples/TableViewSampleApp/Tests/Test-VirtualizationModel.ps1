# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
#requires -Version 7.0

$ErrorActionPreference = 'Stop'
$source = Get-Content (Join-Path (Split-Path $PSScriptRoot -Parent) 'Pages\VirtualizationPage.xaml.cs') -Raw
$model = [regex]::Match($source, '(?s)public sealed class NumberedPerson\b.*$')
if (-not $model.Success) { throw 'NumberedPerson model not found.' }
Add-Type -TypeDefinition ("#nullable enable`nusing System.ComponentModel; namespace SampleModelTests { " + $model.Value + " }")
$row = [SampleModelTests.NumberedPerson]::new()
$row.Id = 123
$row.Role = 'Before'
$events = [Collections.Generic.List[string]]::new()
$handler = [ComponentModel.PropertyChangedEventHandler] {
    param($sender, $eventArgs)
    $events.Add($eventArgs.PropertyName)
}
$row.add_PropertyChanged($handler)
try {
    $row.Role = 'Updated role 1'
    $row.Role = 'Updated role 1'
    if ($events.Count -ne 1 -or $events[0] -ne 'Role') {
        throw 'Role must notify once per actual value change, not for an identical assignment.'
    }
    if ($row.Id -ne 123 -or $row.Role -ne 'Updated role 1') {
        throw 'Role update must preserve the row ID and actual new value.'
    }
}
finally { $row.remove_PropertyChanged($handler) }
Write-Output 'PASS: actual NumberedPerson Role notification/value/identity contract. UI mutation and recycling still require runtime validation.'
