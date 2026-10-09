# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
  Generates the Source-expander snippets (Snippets\<Tag>.xaml.txt / <Tag>.cs.txt) from marked
  regions in the page files, so a snippet is the page's feature code and cannot drift from it.

.DESCRIPTION
  Markers (one or more regions per file, not nested):
    XAML  <!-- snippet -->        ...  <!-- /snippet -->
    C#    // <snippet>            ...  // </snippet>
  Unnamed regions in Pages\<Tag>Page.xaml / Pages\<Tag>Page.xaml.cs belong to <Tag>. A named region
  (<!-- snippet Groups Sort --> or // <snippet Groups Sort>) in ANY project file is appended to the
  listed Tags' snippets, after the page's own regions, under a "From <file>" line; this is how the
  Groups snippet shows the real GroupBy call that lives in Controls\ShapingOptions.xaml.cs.
  *.Status.cs and shared files contribute only through named regions.

  A Tag is generated only when its own page files contain at least one unnamed region; a page
  without one (About, Home, Settings, Hierarchy) has no generated snippet. A snippet whose first
  line is "// snippet:manual" or "<!-- snippet:manual -->" is the opt-out: it is never generated
  or checked (SamplePresenter does not display that marker line).

  Each region is dedented; regions are joined with a "..." separator line. Elision (C# only):
    * whole statements starting with SetLastAction( or RefreshReadouts(); are dropped (narration
      and readouts are not the feature), paren-balanced across lines; parentheses inside string
      and character literals and comments are not counted;
    * a block left with no statement by that elision gets a "// ..." line, so no empty
      "if (...) { }" is shown;
    * an expression-bodied member whose body is narration ("=> SetLastAction(...)") cannot be
      elided and fails the run: keep it outside the region or mark it "// snippet:skip";
    * any line ending in "// snippet:skip" (C#) or "<!-- snippet:skip -->" (XAML) is dropped.
  Keep every region readable on its own: declare (or take as parameters) the values it uses.
  Output: UTF-8 (no BOM), CRLF.

.PARAMETER Check
  Do not write. Fail (exit 1) when a generated snippet differs from the file on disk, or when a page
  names a SourceSnippet / AdditionalSnippet / Snippet file that does not exist. The build runs this
  in its _CheckTableViewSnippets target (incremental; an error in CI, a warning locally).

.PARAMETER Tags
  Only generate (or check) these Tags, e.g. -Tags Sort,Groups. Other snippets are left untouched,
  so parallel work on other pages is not rewritten. Default: every generated Tag.

.EXAMPLE
  powershell -File tools\Update-Snippets.ps1                    # regenerate
  powershell -File tools\Update-Snippets.ps1 -Tags Sort,Groups  # regenerate two pages only
  powershell -File tools\Update-Snippets.ps1 -Check             # what the build runs
  (Windows PowerShell 5.1 and PowerShell 7 both work.)
#>
[CmdletBinding()]
param(
    [switch]$Check,
    [string]$ProjectDir,
    [string[]]$Tags
)

# Windows PowerShell 5.1 runs this from the build, so keep the file ASCII-only.
$ErrorActionPreference = 'Stop'
if (-not $ProjectDir) { $ProjectDir = Split-Path -Parent $PSScriptRoot }
$ProjectDir = (Resolve-Path $ProjectDir).Path.TrimEnd('\')
$ellipsis = [string][char]0x2026

$csStart = '^\s*//\s*<snippet(?:\s+(?<tags>[\w ]+?))?\s*>\s*$'
$csEnd = '^\s*//\s*</snippet>\s*$'
$xamlStart = '^\s*<!--\s*snippet(?:\s+(?<tags>[\w ]+?))?\s*-->\s*$'
$xamlEnd = '^\s*<!--\s*/snippet\s*-->\s*$'

function Get-Regions([string]$path) {
    $isXaml = $path -like '*.xaml'
    $start = if ($isXaml) { $xamlStart } else { $csStart }
    $end = if ($isXaml) { $xamlEnd } else { $csEnd }
    $lines = [System.IO.File]::ReadAllText($path) -split "`r?`n"
    $regions = @()
    $current = $null
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($null -eq $current) {
            $m = [regex]::Match($line, $start)
            if ($m.Success) {
                $tags = @()
                if ($m.Groups['tags'].Success) { $tags = @($m.Groups['tags'].Value -split '\s+' | Where-Object { $_ }) }
                $current = [pscustomobject]@{ File = $path; Tags = $tags; Lines = New-Object System.Collections.Generic.List[string]; IsXaml = $isXaml; Line = $i + 1 }
            }
        }
        elseif ([regex]::IsMatch($line, $end)) {
            $regions += $current
            $current = $null
        }
        elseif ([regex]::IsMatch($line, $start)) {
            throw "${path}:$($i + 1): nested snippet region"
        }
        else {
            $current.Lines.Add($line)
        }
    }
    if ($null -ne $current) { throw "${path}:$($current.Line): snippet region is not closed" }
    return , $regions
}

# The code of a C# line with string / char literals and the trailing // comment blanked out, so
# parentheses and braces can be counted. $state carries an open verbatim or raw string to the next line.
function Get-CodeText([string]$line, [ref]$state) {
    $sb = New-Object System.Text.StringBuilder
    $i = 0
    while ($i -lt $line.Length) {
        $c = $line[$i]
        if ($state.Value -eq 'verbatim') {
            if ($c -eq '"') {
                if ($i + 1 -lt $line.Length -and $line[$i + 1] -eq '"') { $i += 2; continue }
                $state.Value = $null
            }
            $i++; continue
        }
        if ($state.Value -eq 'raw') {
            if ($line.Substring($i).StartsWith('"""')) { $state.Value = $null; $i += 3; continue }
            $i++; continue
        }
        if ($c -eq '/' -and $i + 1 -lt $line.Length -and $line[$i + 1] -eq '/') { break }
        if ($line.Substring($i).StartsWith('"""')) { $state.Value = 'raw'; $i += 3; continue }
        if ($c -eq '@' -and $i + 1 -lt $line.Length -and $line[$i + 1] -eq '"') { $state.Value = 'verbatim'; $i += 2; continue }
        if (($c -eq '$' -or $c -eq '@') -and $i + 2 -lt $line.Length -and ($line[$i + 1] -eq '@' -or $line[$i + 1] -eq '$') -and $line[$i + 2] -eq '"') {
            $state.Value = 'verbatim'; $i += 3; continue
        }
        if ($c -eq '"' -or $c -eq "'") {
            # Regular (or $-interpolated) string / char literal: skip to its closing quote.
            $i++
            while ($i -lt $line.Length -and $line[$i] -ne $c) {
                if ($line[$i] -eq '\') { $i++ }
                $i++
            }
            $i++; continue
        }
        [void]$sb.Append($c)
        $i++
    }
    return $sb.ToString()
}

function Remove-Narration([string[]]$lines, [string]$where) {
    $out = New-Object System.Collections.Generic.List[string]
    $i = 0
    while ($i -lt $lines.Count) {
        $trim = $lines[$i].Trim()
        if ($trim -match '=>\s*(SetLastAction|RefreshReadouts)\(') {
            throw "${where}: an expression-bodied narration ('=> $($Matches[1])(...)') cannot be elided from a snippet; keep it outside the region or mark the line // snippet:skip"
        }
        if ($trim.StartsWith('SetLastAction(') -or $trim.StartsWith('RefreshReadouts();')) {
            # Skip until the statement's parentheses balance and it ends with ';'.
            $indent = $lines[$i].Substring(0, $lines[$i].Length - $lines[$i].TrimStart().Length)
            $depth = 0
            $state = $null
            while ($i -lt $lines.Count) {
                $code = Get-CodeText $lines[$i] ([ref]$state)
                $depth += ([regex]::Matches($code, '\(')).Count - ([regex]::Matches($code, '\)')).Count
                $i++
                if ($depth -le 0 -and $null -eq $state -and $code.TrimEnd().EndsWith(';')) { break }
            }
            $out.Add("$indent$([char]0)")
            continue
        }
        $out.Add($lines[$i])
        $i++
    }

    # Keep a "// ..." where an elided statement leaves its block with no code; drop the other marks.
    $result = New-Object System.Collections.Generic.List[string]
    for ($k = 0; $k -lt $out.Count; $k++) {
        $line = $out[$k]
        if (-not $line.EndsWith([string][char]0)) { $result.Add($line); continue }
        $prev = $null
        for ($p = $k - 1; $p -ge 0; $p--) {
            $t = $out[$p].Trim()
            if ($t -eq '' -or $t.StartsWith('//') -or $t -eq [string][char]0) { continue }
            $prev = $t; break
        }
        $next = $null
        for ($n = $k + 1; $n -lt $out.Count; $n++) {
            $t = $out[$n].Trim()
            if ($t -eq '' -or $t.StartsWith('//') -or $t -eq [string][char]0) { continue }
            $next = $t; break
        }
        $opensBlock = ($null -ne $prev) -and $prev.EndsWith('{')
        $closesBlock = ($null -ne $next) -and $next.StartsWith('}')
        $alreadyMarked = $result.Count -gt 0 -and $result[$result.Count - 1].Trim() -eq "// $ellipsis"
        if ($opensBlock -and $closesBlock -and -not $alreadyMarked) {
            $result.Add($line.Substring(0, $line.Length - 1) + "// $ellipsis")
        }
    }
    return , $result.ToArray()
}

function Format-Region($region) {
    $lines = @($region.Lines | Where-Object { $_ -notmatch '//\s*snippet:skip\s*$' -and $_ -notmatch '<!--\s*snippet:skip\s*-->\s*$' })
    if (-not $region.IsXaml) { $lines = Remove-Narration $lines "$($region.File):$($region.Line)" }

    $lines = @($lines | ForEach-Object { $_.TrimEnd() })
    $indent = ($lines | Where-Object { $_ -ne '' } | ForEach-Object { $_.Length - $_.TrimStart().Length } | Measure-Object -Minimum).Minimum
    if ($null -eq $indent) { $indent = 0 }
    $lines = @($lines | ForEach-Object { if ($_.Length -ge $indent) { $_.Substring($indent) } else { $_.TrimStart() } })

    # One blank line at most; none right after an opening brace or before a closing one.
    $clean = New-Object System.Collections.Generic.List[string]
    foreach ($l in $lines) {
        if ($l -eq '') {
            if ($clean.Count -eq 0 -or $clean[$clean.Count - 1] -eq '' -or $clean[$clean.Count - 1].Trim() -eq '{') { continue }
        }
        elseif ($l.Trim().StartsWith('}') -and $clean.Count -gt 0 -and $clean[$clean.Count - 1] -eq '') {
            $clean.RemoveAt($clean.Count - 1)
        }
        $clean.Add($l)
    }
    while ($clean.Count -gt 0 -and $clean[$clean.Count - 1] -eq '') { $clean.RemoveAt($clean.Count - 1) }
    return , $clean.ToArray()
}

function Join-Snippet($regions, [bool]$isXaml) {
    $sep = if ($isXaml) { "<!-- $ellipsis -->" } else { "// $ellipsis" }
    $parts = New-Object System.Collections.Generic.List[string]
    $first = $true
    foreach ($r in $regions) {
        if (-not $first) { $parts.Add(''); $parts.Add($sep); $parts.Add('') }
        $first = $false
        if ($r.Tags.Count -gt 0) {
            $rel = $r.File.Substring($ProjectDir.Length + 1)
            $parts.Add($(if ($isXaml) { "<!-- From $rel -->" } else { "// From $rel" }))
        }
        foreach ($l in (Format-Region $r)) { $parts.Add($l) }
    }
    return (($parts -join "`r`n") + "`r`n")
}

$pagesDir = Join-Path $ProjectDir 'Pages'
$snippetDir = Join-Path $ProjectDir 'Snippets'
$allFiles = Get-ChildItem -Path $ProjectDir -Recurse -File -Include '*.xaml', '*.cs' |
    Where-Object { $_.FullName -notmatch '\\(bin|obj)\\' } |
    Sort-Object FullName

$own = @{}
$named = New-Object System.Collections.Generic.List[object]
foreach ($file in $allFiles) {
    $text = [System.IO.File]::ReadAllText($file.FullName)
    if ($text -notmatch '(<!--\s*/?snippet[\s>-])|(//\s*</?snippet[\s>])') { continue }
    $regions = Get-Regions $file.FullName
    foreach ($r in $regions) {
        if ($r.Tags.Count -gt 0) { $named.Add($r); continue }
        $m = [regex]::Match($file.Name, '^(?<tag>\w+)Page\.xaml(?<cs>\.cs)?$')
        if ($file.DirectoryName -ne $pagesDir -or -not $m.Success) {
            throw "$($file.FullName):$($r.Line): an unnamed snippet region is only allowed in Pages\<Tag>Page.xaml(.cs); name the Tag (// <snippet Tag>)"
        }
        $tag = $m.Groups['tag'].Value
        if (-not $own.ContainsKey($tag)) { $own[$tag] = @{ xaml = @(); cs = @() } }
        $kind = if ($m.Groups['cs'].Success) { 'cs' } else { 'xaml' }
        $own[$tag][$kind] += $r
    }
}

$selected = @($own.Keys | Sort-Object)
if ($Tags) {
    $Tags = @($Tags | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $unknown = @($Tags | Where-Object { -not $own.ContainsKey($_) })
    if ($unknown.Count -gt 0) {
        throw "Unknown or unmarked Tag(s): $($unknown -join ', '). Generated Tags: $($selected -join ', ')"
    }
    $selected = @($selected | Where-Object { $Tags -contains $_ })
}

$stale = @()
$written = @()
foreach ($tag in $selected) {
    foreach ($kind in 'xaml', 'cs') {
        $wantXaml = $kind -eq 'xaml'
        $regions = @($own[$tag][$kind]) + @($named | Where-Object { $_.Tags -contains $tag -and $_.IsXaml -eq $wantXaml })
        if ($regions.Count -eq 0) { continue }
        $target = Join-Path $snippetDir "$tag.$kind.txt"
        if (Test-Path $target) {
            $firstLine = Get-Content -Path $target -TotalCount 1
            if ($firstLine -match 'snippet:manual') { continue }
        }
        $content = Join-Snippet $regions $wantXaml
        $existing = if (Test-Path $target) { [System.IO.File]::ReadAllText($target) } else { $null }
        $same = ($null -ne $existing) -and (($existing -replace "`r`n", "`n") -ceq ($content -replace "`r`n", "`n"))
        if ($same) { continue }
        if ($Check) {
            $stale += "Snippets\$tag.$kind.txt"
        }
        else {
            [System.IO.File]::WriteAllText($target, $content, (New-Object System.Text.UTF8Encoding($false)))
            $written += "Snippets\$tag.$kind.txt"
        }
    }
}

# Every snippet a page names must exist (Snippet="Sort" names both Sort.xaml.txt and Sort.cs.txt).
function Get-MissingSnippets {
    $present = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    if (Test-Path $snippetDir) {
        Get-ChildItem -Path $snippetDir -Filter '*.txt' -File | ForEach-Object { [void]$present.Add($_.Name) }
    }

    $declared = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    Get-ChildItem -Path $pagesDir -Filter '*.xaml' -File | ForEach-Object {
        $text = [System.IO.File]::ReadAllText($_.FullName)
        [regex]::Matches($text, '(?:SourceSnippet|AdditionalSnippet)="([^"]+)"') | ForEach-Object { [void]$declared.Add($_.Groups[1].Value) }
        [regex]::Matches($text, '\bSnippet="([^"]+)"') | ForEach-Object {
            [void]$declared.Add("$($_.Groups[1].Value).xaml.txt")
            [void]$declared.Add("$($_.Groups[1].Value).cs.txt")
        }
    }

    return , @($declared | Where-Object { -not $present.Contains($_) })
}

if ($Check) {
    $missing = Get-MissingSnippets
    # One line per problem and exit 1 (not throw), so the build can quote the message verbatim.
    $problems = @()
    if ($missing.Count -gt 0) {
        $problems += "Missing TableView sample snippet file(s): $($missing -join ', '). Add them, or run tools\Update-Snippets.ps1 for pages with // <snippet> regions."
    }
    if ($stale.Count -gt 0) {
        $problems += "Stale TableView sample snippet(s): $($stale -join ', '). Run tools\Update-Snippets.ps1 and include the result."
    }
    if ($problems.Count -gt 0) {
        $problems | ForEach-Object { Write-Host $_ }
        exit 1
    }
    Write-Host "Snippets: up to date ($($selected.Count) generated page(s))."
}
elseif ($written.Count -gt 0) {
    Write-Host "Snippets written: $($written -join ', ')"
}
else {
    Write-Host 'Snippets: up to date.'
}
