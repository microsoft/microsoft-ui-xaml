# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
#requires -Version 7.0

$ErrorActionPreference = 'Stop'
$source = Get-Content (Join-Path (Split-Path $PSScriptRoot -Parent) 'Models\Person.cs') -Raw
$model = [regex]::Match($source, '(?s)public sealed class Person\b.*$')
if (-not $model.Success) { throw 'Person model not found.' }
$imports = "#nullable enable`nusing System; using System.ComponentModel; using System.Globalization; using System.Runtime.CompilerServices;"
Add-Type -TypeDefinition ($imports + ' namespace DisplayValueTests { ' + $model.Value + ' }')
$originalCulture = [Globalization.CultureInfo]::CurrentCulture
try {
    foreach ($cultureName in @('en-US', 'fi-FI')) {
        $culture = [Globalization.CultureInfo]::GetCultureInfo($cultureName)
        [Globalization.CultureInfo]::CurrentCulture = $culture
        $person = [DisplayValueTests.Person]::new()
        $person.JoinDate = [DateTimeOffset]'2024-04-19T00:00:00+00:00'
        $person.ShiftStart = [TimeSpan]::new(9,15,0)
        $expectedDate = 'Join date: ' + $person.JoinDate.ToString('D', $culture)
        $expectedTime = 'Shift start: 09' + $culture.DateTimeFormat.TimeSeparator + '15'
        if ($person.JoinDateAccessibleName -ne $expectedDate -or $person.ShiftStartAccessibleName -ne $expectedTime) {
            throw "Display-only names do not expose current values with context and $cultureName formatting."
        }

        $events = [Collections.Generic.List[string]]::new()
        $handler = [ComponentModel.PropertyChangedEventHandler] {
            param($sender, $eventArgs)
            $events.Add($eventArgs.PropertyName)
        }
        $person.add_PropertyChanged($handler)
        try {
            $person.JoinDate = $person.JoinDate.AddDays(1)
            $person.ShiftStart = [TimeSpan]::new(10,30,0)
            $person.ShiftStart = [TimeSpan]::new(10,30,0)
            if (@($events | Where-Object { $_ -eq 'JoinDateAccessibleName' }).Count -ne 1 -or
                @($events | Where-Object { $_ -eq 'ShiftStartAccessibleName' }).Count -ne 1) {
                throw 'Derived names must notify once per actual bound-value change.'
            }
            if ($person.JoinDateAccessibleName -eq $expectedDate -or $person.ShiftStartAccessibleName -eq $expectedTime) {
                throw 'Accessible names retained a stale date or time snapshot.'
            }
        }
        finally { $person.remove_PropertyChanged($handler) }
    }
}
finally { [Globalization.CultureInfo]::CurrentCulture = $originalCulture }
Write-Output 'PASS: actual Person display-value names, two cultures, 24-hour time, and live derived-property notifications. Native Value providers, row aggregation and Narrator speech remain runtime checks.'
