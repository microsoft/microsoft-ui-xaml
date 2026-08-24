# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# Every field we want to set a value on when creating a new work item needs to be its own JSON dictionary
# with a bunch of boilerplate stuff in addition to the field name and value themselves.
# To avoid a ton of duplication, we'll provide a helper method to set up that dictionary.
function Get-WorkItemParameter
{
    Param(
        [Parameter(Position=0, Mandatory = $true)]
        [string]$FieldName,
        [Parameter(Position=1, Mandatory = $true)]
        $FieldValue,
        [Parameter(Position=2)]
        [ValidateSet("add","replace")]
        [string]$Operation = "add"
    )

    return @{
        "op"=$Operation
        "path"="/fields/$FieldName"
        "from"=$null
        "value"=$FieldValue
    }
}

# Serializes an object to a UTF-8-encoded byte[] suitable for the -Body parameter of
# Invoke-RestMethod when calling Azure DevOps REST APIs.
#
# Why a byte[] instead of just `ConvertTo-Json | Invoke-RestMethod -Body`:
# Under Windows PowerShell 5.1 (the mirror pipeline agent runtime: `powershell.exe`, not
# `pwsh`), Invoke-RestMethod's string-body code path transmits the JSON using the system's
# ANSI code page (Windows-1252 on the en-US Windows Server image) -- regardless of whether
# the Content-Type header includes `; charset=utf-8`. ADO decodes the request body as UTF-8,
# so any non-ASCII character (e.g. U+00A5 yen, U+20A9 won in GitHub issue
# microsoft/microsoft-ui-xaml#11092) becomes an invalid UTF-8 byte sequence. JSON parsing
# fails server-side and the API returns the generic
# `VssPropertyValidationException: You must pass a valid patch document in the body of the request.`
#
# Passing a byte[] bypasses IRM's broken string-to-bytes step -- the bytes are transmitted
# verbatim, so we control the encoding end-to-end.
function ConvertTo-AzureDevOpsRequestBody
{
    Param(
        [Parameter(Position=0, Mandatory = $true)]
        $InputObject,
        [Parameter()]
        [int]$Depth = 2
    )

    $json = ConvertTo-Json $InputObject -Depth $Depth
    return ,([System.Text.Encoding]::UTF8.GetBytes($json))
}

# It also requires some doing to convert from the markdown description in GitHub to the HTML description in Azure DevOps.
function Get-AzureDevOpsDescription
{
    Param(
        $GitHubIssue,
        $GitHubRestApiMarkdownHeaders,
        $GitHubRestApiRawMarkdownHeaders
    )

    $issueBody = $GitHubIssue.body

    # GitHub provides a handy-dandy REST API you can call to convert from GitHub markdown to HTML:
    #
    #   https://docs.github.com/en/rest/reference/markdown#render-an-arbitrary-markdown-document
    #
    # This is useful because markdown is used in GitHub descriptions while Azure DevOps expects HTML.

    # If the issue has any HTML, we might fail to parse GitHub Flavored Markdown (GFM),
    # so if we hit such an issue, we'll fall back to regular Markdown, and then to raw text.
    try
    {
        $markdownRequestBody = ConvertTo-Json @{
            "text"=$issueBody
            "mode"="gfm"
            "context"="microsoft/microsoft-ui-xaml"
        }

        $markdownAsHtml = Invoke-RestMethod "https://api.github.com/markdown" -Method Post -Body $markdownRequestBody -Headers $GitHubRestApiMarkdownHeaders
    }
    catch
    {
        try
        {
            # If we failed to parse GFM, try regular Markdown instead.
            $markdownRequestBody = ConvertTo-Json @{
                "accept"="application/vnd.github.v3+json"
                "text"=$issueBody
                "mode"="markdown"
            }

            $markdownAsHtml = Invoke-RestMethod "https://api.github.com/markdown" -Method Post -Body $markdownRequestBody -Headers $GitHubRestApiMarkdownHeaders
        }
        catch
        {
            # If even that failed, just send to the raw API.
            $markdownAsHtml = Invoke-RestMethod "https://api.github.com/markdown/raw" -Method Post -Body $issueBody -Headers $GitHubRestApiRawMarkdownHeaders
        }
    }

    $issueDescription = @"
<p>This Azure DevOps work item is mirrored from this GitHub issue: <a href="$($GitHubIssue.html_url)">$($GitHubIssue.html_url)</a></p>
<p>Created by $($GitHubIssue.user.login)</p>
<p><b>Description:</b></p>
$markdownAsHtml
"@

    return $issueDescription
}

# An Azure DevOps work item is open as long as it is not in any of the below states.
function Get-IsAzureDevOpsWorkItemOpen
{
    Param($AzureDevOpsWorkItem)

    return $AzureDevOpsWorkItem.fields.'System.State' -ne "Cut" -and
        $AzureDevOpsWorkItem.fields.'System.State' -ne "Completed" -and
        $AzureDevOpsWorkItem.fields.'System.State' -ne "Resolved" -and
        $AzureDevOpsWorkItem.fields.'System.State' -ne "Closed"
}

function Get-AzureDevOpsResolvedReason
{
    Param($GitHubIssue)

    # We'll first try to get the resolved reason from issue labels.
    if ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-ByDesign" }))
    {
        return "By Design"
    }
    elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-Duplicate" }))
    {
        return "Duplicate"
    }
    elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-External" }))
    {
        return "External"
    }
    elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-Fixed" }))
    {
        return "Fixed"
    }
    elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-NotRepro" }))
    {
        return "Not Repro"
    }
    elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.Name -eq "closed-Won'tFix" }))
    {
        return "Won't Fix"
    }

    # If we didn't find any closed labels, we'll infer the answer from the state reason.
    elseif ($GitHubIssue.state_reason -eq "not_planned")
    {
        return "Won't Fix"
    }
    else
    {
        return "Fixed"
    }
}

function Get-GitHubIssueResolvedLabel
{
    Param($AzureDevOpsWorkItem)

    # If we have a resolved reason, we'll derive the resolved label from that.
    switch ($AzureDevOpsWorkItem.fields.'Microsoft.VSTS.Common.ResolvedReason')
    {
        "By Design" { return "closed-ByDesign" }
        "Duplicate" { return "closed-Duplicate" }
        "External" { return "closed-External" }
        "Fixed" { return "closed-Fixed" }
        "Not Repro" { return "closed-NotRepro" }
        "Won't Fix" { return "closed-Won'tFix" }
    }

    # Otherwise, we'll infer it from the state of the work item.
    switch ($AzureDevOpsWorkItem.fields.'System.State')
    {
        "Completed" { return "closed-Fixed" }
        "Resolved" { return "closed-Fixed" }
        "Closed" { return "closed-Fixed" }
        "Cut" { return "closed-Won'tFix" }
    }

    # If none of the above worked, we'll just assume fixed.
    return "closed-Fixed"
}

function Get-AzureDevOpsClosureAndReason
{
    Param($GitHubIssue,
        $AzureDevOpsWorkItem)

    if ($GitHubIssue.state_reason -eq "completed")
    {
        if ($azureDevOpsWorkItem.fields.'System.WorkItemType' -eq "Bug")
        {
            return [PSCustomObject]@{
                Closure = "Resolved"
                Reason  = "Fixed"
            }
        }
        else
        {
            return [PSCustomObject]@{
                Closure = "Completed"
                Reason  = $null
            }
        }
    }
    else
    {
        if ($azureDevOpsWorkItem.fields.'System.WorkItemType' -eq "Bug")
        {
            if ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.name -eq "closed-ByDesign" }))
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason  = "By Design"
                }
            }
            elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.name -eq "closed-Duplicate" }))
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason  = "Duplicate"
                }
            }
            elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.name -eq "closed-External" }))
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason   = "External"
                }
            }
            elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.name -eq "closed-Fixed" }))
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason  = "Fixed"
                }
            }
            elseif ([System.Linq.Enumerable]::Any($GitHubIssue.labels, [Func[object,bool]] { param($label) $label.name -eq "closed-NotRepro" }))
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason  = "Not Repro"
                }
            }
            else
            {
                return [PSCustomObject]@{
                    Closure = "Resolved"
                    Reason  = "Won't Fix"
                }
            }
        }
        else
        {
            return [PSCustomObject]@{
                Closure = "Cut"
                Reason  = $null
            }
        }
    }
}

function Get-AzureDevOpsAreaPath
{
    Param(
        $gitHubIssue,
        $baseAreaPath,
        $controlsAreaPath,
        $inputAreaPath,
        $ioAreaPath,
        $lifetimeAreaPath,
        $markupAreaPath,
        $mrtAreaPath,
        $renderingAreaPath)
    
    $areaPath = $null

    $baseAreaLabels = @(
        "area-External"
    )

    $controlsAreaLabels = @(
        "area-Accessibility",
        "area-AccessKeys",
        "area-AnimatedIcon",
        "area-AnimatedVisualPlayer",
        "area-Animations",
        "area-AutoSuggestBox",
        "area-Breadcrumb",
        "area-Button",
        "area-ColorPicker",
        "area-ComboBox",
        "area-CommandBarFlyout",
        "area-Commanding",
        "area-CoreFramework",
        "area-DateTimePickers",
        "area-Density",
        "area-Dialogs",
        "area-EffectiveViewport",
        "area-Expander",
        "area-Flyouts",
        "area-FocusManager",
        "area-Hyperlink",
        "area-HyperlinkButton",
        "area-ImageIcon",
        "area-InfoBadge",
        "area-InfoBar",
        "area-InkCanvas",
        "area-InkToolBar",
        "area-InputValidation",
        "area-ItemsRepeater",
        "area-ItemsView",
        "area-KeyboardAccelerators",
        "area-Layouts",
        "area-Lists",
        "area-Menus",
        "area-Navigation",
        "area-NavigationView",
        "area-NumberBox",
        "area-Pager",
        "area-ParallaxView",
        "area-PasswordBox",
        "area-Performance",
        "area-PersonPicture",
        "area-PipsPager",
        "area-Pivot",
        "area-Popup",
        "area-Progress",
        "area-PullToRefresh",
        "area-RadialGradientBrush",
        "area-RadioButtons",
        "area-RatingControl",
        "area-RepeatButton",
        "area-ScrollBar",
        "area-Scrolling",
        "area-SelectionModel",
        "area-SemanticZoom",
        "area-Slider",
        "area-SplitButton",
        "area-SplitView",
        "area-Styling",
        "area-SwipeControl",
        "area-TabView",
        "area-TeachingTip",
        "area-TestInfrastructure",
        "area-TextBox",
        "area-TitleBar",
        "area-ToggleSwitch",
        "area-ToolTip",
        "area-Transitions",
        "area-TreeView",
        "area-TwoPaneView",
        "area-VSM",
        "area-XYFocus"
    )

    $inputAreaLabels = @(
        "area-Mouse",
        "area-Pointer"
    )

    $ioAreaLabels = @(
        "area-App activation",
        "area-AppWindow",
        "area-Islands",
        "area-Windowing"
    )

    $markupAreaLabels = @(
        "area-Application",
        "area-Binding",
        "area-ErrorHandling",
        "area-NugetPackage",
        "area-Parser",
        "area-ProjectSystem",
        "area-Tooling",
        "area-Unpackaged",
        "area-XamlCompiler",
        "area-XamlWindow"
    )

    $mrtAreaLabels = @(
        "area-MRT"
    )

    $lifetimeAreaLabels = @(
        "area-Lifetime"
    )

    $renderingAreaLabels = @(
        "area-Icon",
        "area-Images",
        "area-Materials",
        "area-MediaElement",
        "area-MediaPlayerElement",
        "area-Shadows",
        "area-Shapes",
        "area-TextBlocks",
        "area-WebView"
    )

    $labelsToAreaPathDictionary = @{
        $baseAreaLabels = $baseAreaPath
        $controlsAreaLabels = $controlsAreaPath
        $ioAreaLabels = $ioAreaPath
        $markupAreaLabels = $markupAreaPath
        $mrtAreaLabels = $mrtAreaPath
        $inputAreaLabels = $inputAreaPath
        $lifetimeAreaLabels = $lifetimeAreaPath
        $renderingAreaLabels = $renderingAreaPath
    }

    # We'll first look through the labels to find an area label.  If we find one,
    # we'll use that as the basis for the Azure DevOps area path.
    # Since a GitHub issue can have multiple labels and there's no way to tell
    # which one should win, in the case where an issue has multiple area labels,
    # we'll just use the first one we find.
    foreach ($label in $gitHubIssue.labels)
    {
        foreach ($labelListAreaPathPair in $labelsToAreaPathDictionary.GetEnumerator())
        {
            if ($labelListAreaPathPair.Key.Contains($label.name))
            {
                return $labelListAreaPathPair.Value
            }
        }
    }

    # If we didn't find an area label, we'll next consider team labels, and use those if one is found.
    $teamLabelToAreaPathDictionary = @{
        "team-Controls" = $controlsAreaPath
        "team-Markup" = $markupAreaPath
        "team-Reach" = $lifetimeAreaPath
        "team-Rendering" = $renderingAreaPath
    }

    foreach ($label in $gitHubIssue.labels)
    {
        foreach ($teamLabel in $teamLabelToAreaPathDictionary.Keys)
        {
            if ($label.name -eq $teamLabel)
            {
                return $teamLabelToAreaPathDictionary[$teamLabel]
            }
        }
    }

    # If we found no label that maps to an Azure DevOps area path, then we'll default to the controls path,
    # since the area path has to have some value.
    return $controlsAreaPath
}

# We know that the list of GitHub issues is sorted, so we'll use a custom binary search
# since FirstOrDefault() performs a linear search instead.
function Get-GitHubIssueByNumber
{
    Param(
        $GitHubIssues,
        [int]$IssueNumber)

    $lowIndex = 0
    $highIndex = $GitHubIssues.Count

    while ($lowIndex -le $highIndex)
    {
        [int]$midIndex = [Math]::Floor($LowIndex + ($HighIndex - $LowIndex) / 2)
        $issueAtIndex = $GitHubIssues[$midIndex]
        [int]$numberAtIndex = $issueAtIndex.number

        if ($numberAtIndex -eq $IssueNumber)
        {
            return $issueAtIndex
        }
        elseif ($numberAtIndex -gt $IssueNumber)
        {
            $highIndex = $midIndex - 1
        }
        else
        {
            $lowIndex = $midIndex + 1
        }
    }

    return $null
}

# We'll sum the total number of comments and the number of reactions associated with an issue
# to get a value that we'll use as the engagement 
function Get-GitHubIssueEngagementCount
{
    Param(
        $GitHubIssue,
        $GitHubRestApiHeaders)

    $engagementCount = $GitHubIssue.reactions.total_count
    $comments = Invoke-RestMethod $GitHubIssue.comments_url -Headers $GitHubRestApiHeaders
    $engagementCount += $comments.Count

    foreach ($comment in $comments)
    {
        $engagementCount += $comment.reactions.total_count
    }

    return $engagementCount
}