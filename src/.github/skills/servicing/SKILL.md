---
name: servicing
description: Cherry-pick PRs to servicing branches with WinAppSDK containment. Use when asked to service, cherry-pick, or port changes to a release branch.
---

# Servicing a PR to a Release Branch

## Overview

Servicing means cherry-picking a PR's changes onto a release branch and wrapping them in WinAppSDK containment checks so they can be controlled via RuntimeCompatibilityOptions.

## Workflow

### 1. Gather Information

Ask the user for:
- **PR number(s)** to cherry-pick
- **Target branch** (default: `release/2.0-stable` for stable servicing)
- **Servicing bug ID** — either an existing bug ID, or confirm that a new bug should be created

For WinAppSDK 2.0 experimental servicing, use `release/2.0-experimental` as
`<target-branch>` when fetching, creating the user branch, and creating the PR.
Do not fall back to `release/2.0-stable` for an experimental request. If the
release channel is unclear, confirm the target branch with the user.

#### Creating a Servicing Bug

If a new bug needs to be created, ask the user to:

1. Create a **copy** of the template bug (do **not** copy links):
   👉 https://microsoft.visualstudio.com/OS/_workitems/edit/62294974
2. Set the title to: `[<version> servicing] <first PR title>`
   - Example: `[2.0 servicing] Fix crash in UniformGridLayout`
   - For Experimental target, use `[2.0-Experimental]` prefix 
3. Report back the new bug ID to use as the **WorkItemID** for containment.

Provide the user with the suggested title so they can paste it directly.

### 2. Create a User Branch

```powershell
git fetch origin <target-branch>
git checkout -b user/$env:USERNAME/cherrypick-<PR>-to-<version> origin/<target-branch>
```

Where `<version>` is extracted from the target branch (e.g., `release/2.0-stable` → `2.0`).

Example: `user/myusername/cherrypick-12345-to-2.0`

### 3. Cherry-Pick the Changes

The original PRs are squash-merged, so cherry-pick the **squash commit** (the single merge commit on the target branch), not the individual commits from the PR branch.

```powershell
git cherry-pick <squash-commit-sha>
```

If there are merge conflicts, resolve them and continue:
```powershell
git cherry-pick --continue
```

### 4. Push the Cherry-Pick

```powershell
git push -u origin user/$env:USERNAME/cherrypick-<PR>-to-<version>
```

### 5. Create a Pull Request (Optional)

Ask the user if they'd like to create a PR. If yes, tell the user:

> "I'll create the PR now before adding containment, so containment will appear as a separate iteration in the PR — making it easier to review."

#### Ensure `az` CLI is available

```powershell
az --version
```

If `az` is not found, ask the user for permission to install it:
```powershell
winget install Microsoft.AzureCLI
```

After installing (or if freshly installed), authenticate:
```powershell
az login
```

Also ensure the `azure-devops` extension is installed:
```powershell
az extension add --name azure-devops 2>$null
```

#### Create the draft PR

```powershell
az repos pr create `
  --organization "https://microsoft.visualstudio.com/DefaultCollection" `
  --project "WinUI" `
  --repository "microsoft-ui-xaml-lift" `
  --source-branch "user/$env:USERNAME/cherrypick-<PR>-to-<version>" `
  --target-branch "<target-branch>" `
  --title "<bug title>" `
  --description "Cherry-pick !<PR>" `
  --draft
```

Where:
- **Title** = the servicing bug title (e.g., `[2.0 servicing] Fix crash in UniformGridLayout`)
- **Target branch** = the release branch (e.g., `release/2.0-stable`)
- Always create as `--draft`

#### Update the PR description via REST API

The `az` CLI truncates long descriptions. After creating the PR, update it via the ADO REST API to set the full original PR description:

```powershell
$prId = <newly created PR ID from az output>
$org = "https://microsoft.visualstudio.com/DefaultCollection"
$project = "WinUI"
$repo = "microsoft-ui-xaml-lift"

# Get an access token from az cli
$token = az account get-access-token --resource "499b84ac-1321-427f-aa17-267ca6975798" --query accessToken -o tsv

$body = @{ description = "<original PR description>" } | ConvertTo-Json -Depth 10

curl -s -X PATCH `
  -H "Content-Type: application/json" `
  -H "Authorization: Bearer $token" `
  -d $body `
  "$org/$project/_apis/git/repositories/$repo/pullrequests/$prId`?api-version=7.0"
```

This ensures the full PR description from the original PR is preserved without truncation.

The final description should begin with `Cherry-pick !<PR>` (or `Cherry-pick !<PR1>, !<PR2>` for multiple) followed by the original PR description content.

#### Link the PR to the servicing bug

After creating and updating the PR, link it to the servicing work item:

```powershell
$artifactId = "vstfs:///CodeReview/CodeReviewId/$( (az devops project show --project WinUI --org $org --query id -o tsv) )/$prId"

$linkBody = @"
[
  {
    "op": "add",
    "path": "/relations/-",
    "value": {
      "rel": "ArtifactLink",
      "url": "$artifactId",
      "attributes": {
        "name": "Pull Request"
      }
    }
  }
]
"@

curl -s -X PATCH `
  -H "Content-Type: application/json-patch+json" `
  -H "Authorization: Bearer $token" `
  -d $linkBody `
  "$org/$project/_apis/wit/workitems/<WorkItemID>?api-version=7.0"
```

### 6. Add WinAppSDK Containment

All code changes must be wrapped in containment checks. This allows the fix to be disabled in production via RuntimeCompatibilityOptions if a regression is discovered.

This is done as a **separate commit** after the PR is created so that it appears as its own iteration in the PR, making it easier to review the cherry-pick and containment independently.

#### 6a. Add the `#define` near the top of each changed file

```cpp
#include "FrameworkUdk/Containment.h"

// Bug <WorkItemID>: <Short description of the fix>
#define WINAPPSDK_CHANGEID_<WorkItemID> <WorkItemID>
```

Where:
- `<WorkItemID>` is the bug/work item ID associated with the change

#### 6b. Wrap changed code in `IsChangeEnabled` checks

```cpp
if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_<WorkItemID>>())
{
    // New (fixed) code path
}
else
{
    // Original (pre-fix) code path — preserved for consumers that have
    // disabled the fix or required patch level
}
```

**Rules:**
- The `if` branch contains the **new/fixed** behavior
- The `else` branch contains the **original** behavior (copy-paste the old code)
- Every behavioral change must be guarded — do not leave any unguarded modifications
- If a change is purely additive (no old behavior to preserve), the `else` branch can be empty or omitted only if there's nothing to revert to

#### 6c. Reference Commit

See commit `5c63966faa6e20dfefb2df31c6bc0595e0d516fc` on the `release/2.0-stable` branch for a complete example showing:
- The `#include` and `#define` placement
- The `IsChangeEnabled` guard pattern wrapping a one-line fix
- The `else` branch preserving original behavior

### 7. Commit and Push Containment

```powershell
git add -A
git commit -m "Add WinAppSDK containment for <WorkItemID>"
git push
```

### 8. Validate the Servicing Bug

After all code changes are pushed, validate and fix up the servicing bug fields via the REST API.

First, fetch the current bug to inspect its fields:

```powershell
$bugResponse = curl -s `
  -H "Authorization: Bearer $token" `
  "$org/$project/_apis/wit/workitems/<WorkItemID>?api-version=7.0" | ConvertFrom-Json
```

Build a list of patch operations based on what needs fixing:

```powershell
$patches = @()

# Remove "WASDK-Servicing-Template" tag if present and add "WASDK-Servicing" tag
$tags = $bugResponse.fields.'System.Tags'
$tagList = if ($tags) { ($tags -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' -and $_ -ne 'WASDK-Servicing-Template' }) } else { @() }
if ('WASDK-Servicing' -notin $tagList) { $tagList += 'WASDK-Servicing' }
$newTags = $tagList -join '; '
if ($newTags -ne $tags) {
    $patches += @{ op = "replace"; path = "/fields/System.Tags"; value = $newTags }
}

# Set Product Family to "Applications"
if ($bugResponse.fields.'OSG.ProductFamily' -ne 'Applications') {
    $patches += @{ op = "add"; path = "/fields/OSG.ProductFamily"; value = "Applications" }
}

# Set Product to "WinAppSDK"
if ($bugResponse.fields.'OSG.Product' -ne 'WinAppSDK') {
    $patches += @{ op = "add"; path = "/fields/OSG.Product"; value = "WinAppSDK" }
}

# Set Release to "WinAppSDK <version>" (e.g., "WinAppSDK 2.0")
$expectedRelease = "WinAppSDK <version>"
if ($bugResponse.fields.'Microsoft.VSTS.Common.Release' -ne $expectedRelease) {
    $patches += @{ op = "add"; path = "/fields/Microsoft.VSTS.Common.Release"; value = $expectedRelease }
}

# If Area Path is under WinAppSDK, move it to WinUI
$areaPath = $bugResponse.fields.'System.AreaPath'
if ($areaPath -match 'WinAppSDK') {
    $patches += @{ op = "replace"; path = "/fields/System.AreaPath"; value = "OS\Windows Client and Services\WinPD\UISP-UI and Search Platform\SPICE\WinUI" }
}

# If Assigned To doesn't match Created By, ask the user if it should be changed
$assignedTo = $bugResponse.fields.'System.AssignedTo'.displayName
$createdBy = $bugResponse.fields.'System.CreatedBy'.uniqueName
if ($assignedTo -ne $bugResponse.fields.'System.CreatedBy'.displayName) {
    # Ask user: "The bug is assigned to '$assignedTo'. Should I change it to the creator ('$($bugResponse.fields.'System.CreatedBy'.displayName)')?"
    # If yes:
    $patches += @{ op = "replace"; path = "/fields/System.AssignedTo"; value = $createdBy }
}
```

Apply the patches if any are needed:

```powershell
if ($patches.Count -gt 0) {
    $patchBody = $patches | ConvertTo-Json -Depth 10
    # Ensure it's always an array
    if ($patches.Count -eq 1) { $patchBody = "[$patchBody]" }

    curl -s -X PATCH `
      -H "Content-Type: application/json-patch+json" `
      -H "Authorization: Bearer $token" `
      -d $patchBody `
      "$org/$project/_apis/wit/workitems/<WorkItemID>?api-version=7.0"
}
```

Where `<version>` is the version from the target branch (e.g., `release/2.0-stable` → `2.0`).

## Tips

- If cherry-picking multiple PRs, include all PR numbers in the branch name separated by hyphens: `cherrypick-111-222-to-2.0`
- Always verify the cherry-pick builds before pushing (use the `build` skill)
- The containment `#define` uses the **work item ID**, not the PR number
