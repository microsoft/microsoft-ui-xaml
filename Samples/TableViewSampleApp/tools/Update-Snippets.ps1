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
  without one (About, Home, Settings) has no generated snippet. A snippet whose first
  line is "// snippet:manual" or "<!-- snippet:manual -->" is the opt-out: it is never generated
  or checked (SamplePresenter does not display that marker line).

  Each region is dedented. Elision (C# only):
    * whole statements that narrate or drive the page UI are dropped (SetLastAction*, RefreshReadouts,
      Announce*, Populate*, Seed*, Set*Enabled, *Button.IsEnabled =, *Text.Text =; see
      $narrationStatement), paren-balanced across lines; parentheses inside string and character
      literals and comments are not counted;
    * a block left with no statement by that elision gets a "// ..." line, so no empty
      "if (...) { }" is shown;
    * an expression-bodied member whose body is narration ("=> SetLastAction(...)") cannot be
      elided and fails the run: keep it outside the region or mark it "// snippet:skip";
    * any line ending in "// snippet:skip" (C#) or "<!-- snippet:skip -->" (XAML) is dropped;
    * a C# line ending in "// snippet:as <code>" is shown as <code> (same indent), for a condition that
      only serves the page's narration.
  Trimming, so a snippet shows what an app needs (as the WinUI Gallery samples do):
    * XAML: AutomationProperties.AutomationId and HelpText are dropped; page layout (Grid.*, Margin,
      Min/MaxHeight, Horizontal/VerticalAlignment, HorizontalContentAlignment, IsEnabled) is dropped from
      the region's top-level elements; a HeaderToolTip or ToolTipService.ToolTip that is a sentence (page
      narration) is dropped; {StaticResource Col*} widths from App.xaml become literals; comments longer
      than two lines are dropped; a TableView start tag has one attribute per line, other tags only when
      longer than 110 characters;
    * C#: // ---- banners, /// doc comments, comment blocks of three or more lines and sentence-style
      HeaderToolTip strings are dropped; guard terms that only protect the page ($pageGuardTerm) are
      removed from "if (...) { return; }", with the comment above the guard; "if (!Call(...)) { <elided> }"
      becomes "Call(...);"; using (BeginBulkUpdate()) is unwrapped; an if (without a call in its
      condition) or a method left with an elided body, and a variable nothing reads any more, are removed.
  Regions are joined with a blank line.
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

# Statements that narrate or drive the page's own UI: Last action, readouts, pickers, sliders and
# button enabling.
$narrationStatement = '^(SetLastAction\w*\(|RefreshReadouts\(\);|Announce\w*\(|Populate\w*\(|Seed\w*\(|Set\w*Enabled\(|Update\w*(Buttons?|Enabled|Readouts?)\w*\(|\w+Button\.IsEnabled\s*=|\w*(Text|Description|Readout|Label)\.Text\s*=)'

function Remove-Narration([string[]]$lines, [string]$where) {
    $out = New-Object System.Collections.Generic.List[string]
    $i = 0
    while ($i -lt $lines.Count) {
        $trim = $lines[$i].Trim()
        if ($trim -match '=>\s*(SetLastAction|RefreshReadouts)\(') {
            throw "${where}: an expression-bodied narration ('=> $($Matches[1])(...)') cannot be elided from a snippet; keep it outside the region or mark the line // snippet:skip"
        }
        if ($trim -match $narrationStatement) {
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

# ---- Trimming: a snippet shows what an app needs, not the page's test and narration plumbing. ----

# Column widths are App.xaml resources on the pages; a snippet shows the literal value.
$columnWidths = @{}
$appXaml = Join-Path $ProjectDir 'App.xaml'
if (Test-Path $appXaml) {
    foreach ($m in [regex]::Matches([System.IO.File]::ReadAllText($appXaml), '<x:Double\s+x:Key="(?<key>Col\w+)"\s*>(?<value>[^<]+)</x:Double>')) {
        $columnWidths[$m.Groups['key'].Value] = $m.Groups['value'].Value.Trim()
    }
}

$xamlDropAlways = @('AutomationProperties.AutomationId', 'AutomationProperties.HelpText')
# Page layout, dropped from the region's top-level elements only (cell templates keep theirs).
$xamlDropTopLevel = @('Grid.Row', 'Grid.Column', 'Grid.RowSpan', 'Grid.ColumnSpan', 'MinHeight', 'MaxHeight',
    'VerticalAlignment', 'HorizontalAlignment', 'HorizontalContentAlignment', 'Margin', 'IsEnabled')
# Tooltips that narrate the page (a sentence) rather than label the UI.
$xamlNarrationAttributes = @('HeaderToolTip', 'ToolTipService.ToolTip')
$commentMark = [string][char]1

function Format-XamlTag([string]$name, $attributes, [string]$close, [string]$indent) {
    if ($attributes.Count -eq 0) { return "<$name$close" }
    $single = "<$name" + (($attributes | ForEach-Object { " $($_.Name)=$($_.Value)" }) -join '') + $close
    $isTable = $name -match '(^|:)TableView$'
    $multi = ($null -ne $indent) -and (($isTable -and $attributes.Count -gt 1) -or ($indent.Length + $single.Length) -gt 110)
    if (-not $multi) { return $single }
    return "<$name" + (($attributes | ForEach-Object { "`n$indent    $($_.Name)=$($_.Value)" }) -join '') + $close
}

function Format-XamlRegion([string[]]$lines) {
    $text = $lines -join "`n"
    $pattern = '(?<comment><!--[\s\S]*?-->)|(?<end></[^>]+>)|<(?<name>[A-Za-z_][\w:.]*)(?<attrs>(?:\s+[\w:.]+\s*=\s*(?:"[^"]*"|''[^'']*''))*)\s*(?<close>/?>)'
    $sb = New-Object System.Text.StringBuilder
    $depth = 0
    $pos = 0
    foreach ($m in [regex]::Matches($text, $pattern)) {
        [void]$sb.Append($text.Substring($pos, $m.Index - $pos))
        $pos = $m.Index + $m.Length
        if ($m.Groups['comment'].Success) {
            # Keep a short why; drop essays.
            if (($m.Value -split "`n").Count -le 2) { [void]$sb.Append($m.Value) } else { [void]$sb.Append($commentMark) }
            continue
        }
        if ($m.Groups['end'].Success) {
            $depth--
            [void]$sb.Append($m.Value)
            continue
        }

        $attributes = New-Object System.Collections.Generic.List[object]
        foreach ($a in [regex]::Matches($m.Groups['attrs'].Value, '(?<n>[\w:.]+)\s*=\s*(?<v>"[^"]*"|''[^'']*'')')) {
            $n = $a.Groups['n'].Value
            $v = $a.Groups['v'].Value
            $inner = $v.Substring(1, $v.Length - 2)
            if ($xamlDropAlways -contains $n) { continue }
            if ($depth -eq 0 -and $xamlDropTopLevel -contains $n) { continue }
            if ($xamlNarrationAttributes -contains $n -and -not $inner.StartsWith('{') -and ($inner.EndsWith('.') -or $inner.Contains('. '))) { continue }
            $r = [regex]::Match($inner, '^\{(?:StaticResource|ThemeResource)\s+(?<key>\w+)\}$')
            if ($r.Success -and $columnWidths.ContainsKey($r.Groups['key'].Value)) { $v = '"' + $columnWidths[$r.Groups['key'].Value] + '"' }
            $attributes.Add([pscustomobject]@{ Name = $n; Value = $v })
        }

        $lineStart = $text.LastIndexOf("`n", [Math]::Max(0, $m.Index - 1)) + 1
        if ($m.Index -eq 0) { $lineStart = 0 }
        $lead = $text.Substring($lineStart, $m.Index - $lineStart)
        $indent = if ($lead.Trim() -eq '') { $lead } else { $null }
        [void]$sb.Append((Format-XamlTag $m.Groups['name'].Value $attributes $m.Groups['close'].Value $indent))
        if ($m.Groups['close'].Value -eq '>') { $depth++ }
    }
    [void]$sb.Append($text.Substring($pos))

    $out = New-Object System.Collections.Generic.List[string]
    foreach ($l in ($sb.ToString() -split "`n")) {
        if ($l.Contains($commentMark)) {
            if ($l.Replace($commentMark, '').Trim() -eq '') { continue }
            $l = $l.Replace($commentMark, '')
        }
        $out.Add($l)
    }
    return , $out.ToArray()
}

# A guard term that only protects the page: a control or field not created yet, not loaded yet, or a
# page-driven update in progress.
$pageGuardTerm = '^(([A-Z]\w*|_\w+)(\.\w+)*\s+is\s+null|!IsLoaded|IsBulkUpdating|!?_ready|_(syncing|suppress|updating|applying|isResorting|refreshQueued)\w*)$'

function Split-TopLevelOr([string]$condition) {
    $parts = New-Object System.Collections.Generic.List[string]
    $depth = 0
    $start = 0
    for ($k = 0; $k -lt $condition.Length; $k++) {
        $c = $condition[$k]
        if ($c -eq '(' -or $c -eq '{' -or $c -eq '[') { $depth++ }
        elseif ($c -eq ')' -or $c -eq '}' -or $c -eq ']') { $depth-- }
        elseif ($depth -eq 0 -and $c -eq '|' -and $k + 1 -lt $condition.Length -and $condition[$k + 1] -eq '|') {
            $parts.Add($condition.Substring($start, $k - $start))
            $start = $k + 2
            $k++
        }
    }
    $parts.Add($condition.Substring($start))
    return , $parts.ToArray()
}

# Banners, doc comments, comment blocks of three or more lines, and guards that only protect the page.
function Remove-CSharpNoise([string[]]$lines) {
    $kept = New-Object System.Collections.Generic.List[string]
    $i = 0
    while ($i -lt $lines.Count) {
        $trim = $lines[$i].Trim()
        if ($trim -match '^//\s*-{2,}' -or $trim.StartsWith('///')) { $i++; continue }
        $tip = [regex]::Match($trim, '^(HeaderToolTip|ToolTip)\s*=\s*"(?<s>[^"]*)",?$')
        if ($tip.Success -and ($tip.Groups['s'].Value.EndsWith('.') -or $tip.Groups['s'].Value.Contains('. '))) { $i++; continue }
        if ($trim.StartsWith('//') -and $trim -ne "// $ellipsis") {
            $j = $i
            while ($j -lt $lines.Count -and $lines[$j].Trim().StartsWith('//') -and $lines[$j].Trim() -ne "// $ellipsis") { $j++ }
            if ($j - $i -ge 3) { $i = $j; continue }
            for ($k = $i; $k -lt $j; $k++) { $kept.Add($lines[$k]) }
            $i = $j
            continue
        }
        $g = [regex]::Match($lines[$i], '^(?<indent>\s*)if \((?<c>.*)\)$')
        if ($g.Success -and $i + 3 -lt $lines.Count -and $lines[$i + 1].Trim() -eq '{' -and $lines[$i + 2].Trim() -eq 'return;' -and $lines[$i + 3].Trim() -eq '}') {
            $terms = Split-TopLevelOr $g.Groups['c'].Value
            $rest = @($terms | Where-Object { $_.Trim() -notmatch $pageGuardTerm })
            # The comment above a guard explains the page terms, so it goes with them.
            if ($rest.Count -eq 0) {
                while ($kept.Count -gt 0 -and $kept[$kept.Count - 1].Trim().StartsWith('//')) { $kept.RemoveAt($kept.Count - 1) }
                $i += 4
                if ($i -lt $lines.Count -and $lines[$i].Trim() -eq '') { $i++ }
                continue
            }
            if ($rest.Count -lt $terms.Count) {
                while ($kept.Count -gt 0 -and $kept[$kept.Count - 1].Trim().StartsWith('//')) { $kept.RemoveAt($kept.Count - 1) }
                $kept.Add("$($g.Groups['indent'].Value)if ($(($rest | ForEach-Object { $_.Trim() }) -join ' || '))")
                $i++
                continue
            }
        }
        $kept.Add($lines[$i])
        $i++
    }
    return , $kept.ToArray()
}

# "if (!Call(...)) { <elided> }" becomes "Call(...);": the call is the API, the branch was narration.
function Expand-EmptyGuards([string[]]$lines) {
    $out = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $m = [regex]::Match($lines[$i], '^(?<indent>\s*)if \(!(?<call>[\w.]+\(.*\))\)$')
        if ($m.Success -and $i + 3 -lt $lines.Count -and $lines[$i + 1].Trim() -eq '{' -and $lines[$i + 2].Trim() -eq "// $ellipsis" -and $lines[$i + 3].Trim() -eq '}') {
            $out.Add("$($m.Groups['indent'].Value)$($m.Groups['call'].Value);")
            $i += 3
            continue
        }
        $out.Add($lines[$i])
    }
    return , $out.ToArray()
}

function Remove-PrecedingComments($list, [int]$index) {
    while ($index -gt 0 -and $list[$index - 1].Trim().StartsWith('//') -and $list[$index - 1].Trim() -ne "// $ellipsis") {
        $list.RemoveAt($index - 1)
        $index--
    }
    return $index
}

# What elision leaves behind: an if or a method with nothing in it, a page bulk-update scope, a
# variable only the narration read.
function Remove-EmptyCode([string[]]$lines) {
    $list = New-Object System.Collections.Generic.List[string]
    foreach ($l in $lines) { $list.Add($l) }
    $changed = $true
    while ($changed) {
        $changed = $false
        for ($i = 0; $i -lt $list.Count -and -not $changed; $i++) {
            $trim = $list[$i].Trim()
            # A body of nothing but comments (an elided "// ..." included) is empty.
            $end = -1
            if ($i + 2 -lt $list.Count -and $list[$i + 1].Trim() -eq '{') {
                $k = $i + 2
                while ($k -lt $list.Count -and ($list[$k].Trim().StartsWith('//') -or $list[$k].Trim() -eq '')) { $k++ }
                if ($k -lt $list.Count -and $list[$k].Trim() -eq '}') { $end = $k }
            }
            $c = [regex]::Match($trim, '^if \((?<c>.*)\)$')
            $noElse = $end -ge 0 -and ($end + 1 -ge $list.Count -or -not $list[$end + 1].Trim().StartsWith('else'))
            $isMethod = $trim -match '^(private|protected|public|internal)\b[^=]*\)$'
            if ($end -ge 0 -and (($c.Success -and $c.Groups['c'].Value -notmatch '\w\(' -and $noElse) -or $isMethod)) {
                for ($k = $i; $k -le $end; $k++) { $list.RemoveAt($i) }
                [void](Remove-PrecedingComments $list $i)
                $changed = $true
                continue
            }
            if ($trim -eq 'using (BeginBulkUpdate())' -and $i + 1 -lt $list.Count -and $list[$i + 1].Trim() -eq '{') {
                $indent = $list[$i].Substring(0, $list[$i].Length - $list[$i].TrimStart().Length)
                $close = -1
                for ($k = $i + 2; $k -lt $list.Count; $k++) { if ($list[$k] -eq "$indent}") { $close = $k; break } }
                if ($close -gt 0) {
                    for ($k = $i + 2; $k -lt $close; $k++) { if ($list[$k].StartsWith("$indent    ")) { $list[$k] = $indent + $list[$k].Substring($indent.Length + 4) } }
                    $list.RemoveAt($close)
                    $list.RemoveAt($i + 1)
                    $list.RemoveAt($i)
                    $changed = $true
                    continue
                }
            }
            $v = [regex]::Match($list[$i], '^(?<indent>\s*)var (?<name>\w+) = (?<expr>.+);$')
            if ($v.Success) {
                $name = $v.Groups['name'].Value
                $used = $false
                $depth = 0
                for ($k = $i + 1; $k -lt $list.Count; $k++) {
                    if ($list[$k] -match "\b$name\b") { $used = $true; break }
                    $depth += ([regex]::Matches($list[$k], '\{')).Count - ([regex]::Matches($list[$k], '\}')).Count
                    if ($depth -lt 0) { break }
                }
                if (-not $used) {
                    $expr = $v.Groups['expr'].Value
                    if ($expr -match '^[\w.]+\(.*\)$') { $list[$i] = $v.Groups['indent'].Value + $expr + ';' } else { $list.RemoveAt($i) }
                    $changed = $true
                    continue
                }
            }
        }
    }
    return , $list.ToArray()
}
function Format-Region($region) {
    $lines = @($region.Lines | Where-Object { $_ -notmatch '//\s*snippet:skip\s*$' -and $_ -notmatch '<!--\s*snippet:skip\s*-->\s*$' })
    $lines = @($lines | ForEach-Object { $m = [regex]::Match($_, '^(?<indent>\s*)\S.*?//\s*snippet:as\s+(?<code>.+?)\s*$'); if ($m.Success) { $m.Groups['indent'].Value + $m.Groups['code'].Value } else { $_ } })
    if ($region.IsXaml) { $lines = Format-XamlRegion $lines }
    else {
        $lines = Remove-CSharpNoise $lines
        $lines = Remove-Narration $lines "$($region.File):$($region.Line)"
    }

    $lines = @($lines | ForEach-Object { $_.TrimEnd() })
    if (-not $region.IsXaml) { $lines = Remove-EmptyCode (Expand-EmptyGuards $lines) }
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
    $parts = New-Object System.Collections.Generic.List[string]
    $first = $true
    foreach ($r in $regions) {
        $body = Format-Region $r
        if ($body.Count -eq 0) { continue }
        if (-not $first) { $parts.Add('') }
        $first = $false
        if ($r.Tags.Count -gt 0) {
            $rel = $r.File.Substring($ProjectDir.Length + 1)
            $parts.Add($(if ($isXaml) { "<!-- From $rel -->" } else { "// From $rel" }))
        }
        foreach ($l in $body) { $parts.Add($l) }
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
