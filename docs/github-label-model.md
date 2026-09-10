# GitHub label model

This proposal supports issue [#11319](https://github.com/microsoft/microsoft-ui-xaml/issues/11319). It is based on the public repository label list fetched on 2026-08-02 and the label references in `.github/policies/resourceManagement.yml` and `.github/ISSUE_TEMPLATE/*.yaml`.

## Goals

- Make labels useful for triage, backlog review, scrum discussion, and public contributor communication.
- Keep label names and descriptions understandable without internal team knowledge.
- Preserve labels that are currently used by issue templates or automation until those dependencies are migrated.
- Prefer a smaller, consistently described label set over many overlapping labels.

## Current inventory

The repository currently has 188 public labels.

| Purpose | Current shape | Count | Notes |
| --- | --- | ---: | --- |
| Area | `area-*` | 122 | Largest label family; many descriptions are empty. |
| Type | `bug`, `feature proposal`, `documentation`, `discussion`, `question`, `dependencies`, `announcement`, `spec issue`, `breaking change` | 9 | Issue templates already set `bug` or `feature proposal`. |
| Status and workflow | `needs-*`, `closed-*`, `duplicate`, `future`, `working on it`, `no-recent-activity`, `auto merge`, `fix-released`, `release note` | 16 | Several labels are automation-sensitive. |
| Priority and impact | `Blocking`, `Crash`, `Regression`, `nice to have`, `good first issue`, `help wanted`, accessibility severity/impact labels | 12 | Naming is mixed between impact, severity, priority, and contributor suitability. |
| Product and platform | `product-*`, `appModel-*`, `.NET MAUI`, `react-native-windows`, `wct`, `wpf-vs-winui-mismatch`, `needs-winui-3` | 10 | Useful, but some names mix product, framework, and migration concepts. |
| Tracking | `team-*`, `group-*`, release labels, `WinUI OSS`, `community-call`, `transfer-repo`, proposal/spec labels | 15 | Some labels expose internal routing rather than public ownership. |
| Accessibility-specific | `accessibility`, `area-Accessibility`, `A11y*` | 12 | Duplicates area, scenario, severity, standard, and test taxonomy. |

125 labels have no public description. 14 labels contain spaces, and 3 labels contain non-ASCII characters. Those are not inherently wrong, but they make filtering and automation harder to keep consistent.

## Labels used by automation

Do not delete or rename these labels until the GitOps resource-management rules are updated in the same change:

- `auto merge`
- `declined`
- `needs-assignee-attention`
- `needs-author-feedback`
- `needs-triage`
- `no-recent-activity`
- `working on it`

Do not delete or rename these labels until issue templates are updated in the same change:

- `bug`
- `feature proposal`
- `needs-triage`

## Recommended target model

Use these label families as the long-term model.

| Family | Format | Purpose | Examples |
| --- | --- | --- | --- |
| Area | `area-*` | Component, feature area, or repository area that owns investigation. | `area-Button`, `area-Windowing`, `area-TestInfrastructure` |
| Type | existing plain type labels | What kind of work item this is. | `bug`, `feature proposal`, `documentation`, `question`, `discussion` |
| Status | `needs-*`, `blocked`, `working on it`, resolution labels | Current workflow state. | `needs-triage`, `needs-repro`, `needs-author-feedback`, `duplicate` |
| Impact | lowercase public impact labels | User-visible severity or release risk. | `crash`, `regression`, `blocking`, `breaking change` |
| Product | `product-*` | Product generation or platform surface. | `product-winui2`, `product-winui3` |
| Contributor | existing GitHub community labels | Good external contribution signals. | `good first issue`, `help wanted`, `community-contribution` |
| Release | release-specific labels | Release notes or servicing tracking. | `release note`, `fix-released`, `needs-cherrypicktorelease` |
| Tracking | narrowly scoped project labels | Temporary initiative tracking. | `WinUI OSS` |

## Proposed cleanup

### Keep

Keep these labels and add or normalize descriptions where missing:

- Type labels: `bug`, `feature proposal`, `documentation`, `discussion`, `question`, `dependencies`, `announcement`, `spec issue`, `breaking change`.
- Core workflow labels: `needs-triage`, `needs-repro`, `needs-author-feedback`, `needs-assignee-attention`, `no-recent-activity`, `working on it`, `duplicate`.
- Contributor labels: `good first issue`, `help wanted`, `community-contribution`.
- Release labels: `release note`, `fix-released`, `needs-cherrypicktorelease`.
- Product labels: `product-winui2`, `product-winui3`, `appModel-UWP`, `appModel-win32`.
- Most `area-*` labels that map to active controls, platform areas, tooling, infrastructure, or docs.

### Rename

These labels should be renamed for clarity. GitHub preserves label assignments during a rename, but automation and documentation references must still be updated.

| Current label | Proposed label | Reason |
| --- | --- | --- |
| `area-SystemBackdropEement` | `area-SystemBackdropElement` | Fix typo. |
| `area-NugetPackage` | `area-NuGetPackage` | Match NuGet product casing. |
| `area-wapproj` | `area-WapProj` or `area-PackagingProject` | Use public-friendly casing/name. |
| `Blocking` | `blocking` | Normalize impact label casing. |
| `Crash` | `crash` | Normalize impact label casing. |
| `Regression` | `regression` | Normalize impact label casing. |
| `closed-Won'tFix` | `resolution-wont-fix` | Avoid status/resolution ambiguity and apostrophe in filters. |
| `closed-Fixed` | `resolution-fixed` | Make this a resolution label, not a closed-state label. |
| `needs-review` with emoji suffix | `needs-review` | Remove emoji from the public label name. |
| `community-contribution` with emoji prefix | `community-contribution` | Remove emoji from the public label name. |

### Consolidate

These groups overlap and should be reduced after maintainers confirm which labels are still used in current triage.

| Current labels | Recommendation |
| --- | --- |
| `accessibility`, `area-Accessibility`, `A11yHighimpact`, `A11yMediumImpact`, `A11yLowImpact`, `A11ySev1`, `A11ySev2`, `A11ySev3`, `A11yMAS`, `A11yWCAG`, `A11yTest_CT`, `A11yTest_Feature` | Keep `area-Accessibility`; keep one severity family such as `a11y-sev1`, `a11y-sev2`, `a11y-sev3`; move standards/test metadata to issue fields or comments unless actively used for tracking. |
| `area-Design`, `area-DesignDiscussion`, `area-UIDesign`, `area-WindowsDesign`, `team-Design` | Keep public area labels for design subjects; retire internal team routing from public labels where possible. |
| `area-Icon`, `area-ImageIcon`, `area-Images` | Keep distinct labels only if triage owners are different; otherwise document each label's scope. |
| `area-Scrolling`, `area-ScrollBar` | Keep both only if issues are routed differently between scrolling infrastructure and ScrollBar control bugs. |
| `area-MediaElement`, `area-MediaPlayerElement` | Keep both if both controls remain supported; otherwise merge into the active supported surface. |
| `group-blurry`, `group-language`, `wpf-vs-winui-mismatch` | Keep only if these are active cross-area review buckets; otherwise replace with area/product/type labels. |

### Retire or make private-process-neutral

Labels that primarily encode internal ownership should be reviewed before the repository moves deeper into public triage:

- `team-CompInput`
- `team-Controls`
- `team-Design`
- `team-Markup`
- `team-Reach`
- `team-Rendering`

Preferred public alternatives are area labels, project-board fields, or GitHub assignees/teams. If a team label is still needed, its description should explain the public routing meaning, not internal organization.

## Baseline descriptions

Use complete descriptions for every label kept in the public set. Suggested descriptions for high-value labels:

| Label | Description |
| --- | --- |
| `needs-triage` | Needs maintainer review to classify area, priority, and next action. |
| `needs-repro` | Needs a minimal reproduction project, sample, or reliable reproduction steps. |
| `needs-author-feedback` | Waiting for the issue author to provide requested information. |
| `needs-assignee-attention` | Assigned issue needs follow-up from the assignee. |
| `no-recent-activity` | Automatically marked inactive after requested author feedback was not provided. |
| `working on it` | A contributor or maintainer is actively working on this item. |
| `auto merge` | Pull request will be merged automatically after required checks pass. |
| `release note` | Pull request should be considered for release notes. |
| `fix-released` | Fix is available in a public experimental, preview, stable, or servicing release. |
| `product-winui2` | Applies to WinUI 2 for UWP apps. |
| `product-winui3` | Applies to WinUI 3 for Windows App SDK apps. |
| `appModel-UWP` | Specific to UWP app model behavior. |
| `appModel-win32` | Specific to Win32 desktop app model behavior. |
| `area-External` | Not owned by this repository; tracked here for visibility or routing. |

## Implementation plan

1. Export current labels and usage counts before making changes.
2. Update labels with missing descriptions first. This has no workflow risk.
3. Rename typo, casing, and emoji labels in one batch.
4. Update `.github/policies/resourceManagement.yml`, issue templates, saved project views, and bot configuration for any renamed workflow labels.
5. Review accessibility and design labels with maintainers, then merge or retire overlapping labels.
6. Add a recurring label review to the repository maintenance checklist so temporary tracking labels do not become permanent taxonomy.

## Follow-up tracking

Track these separately from the label cleanup itself:

- Project-board filters or views that reference renamed labels.
- Bot commands or slash-command behavior that applies labels.
- Existing issues and pull requests that need relabeling after consolidation.
- Documentation that tells contributors which labels maintainers apply and which labels contributors should not set manually.
