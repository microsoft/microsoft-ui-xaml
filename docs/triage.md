# Issue/PR Management Information

## Overview

The WinUI team manages issues and PRs in the repo using a process we call "triage". It helps us keep issues
organized and focuses the attention of the different groups that work in our project.

### Triage labels

1. New and re-opened issues are marked with `needs-triage`.
2. Issues with `needs-assignee-attention` should be investigated by the assignee as top priority.
3. Issues with `needs-author-feedback` are waiting for the author to reply.

Because many groups are involved in WinUI we have `team-...` labels to help filter issues further for triage.

### Triage process

For each issue with `needs-triage`:
* Feature proposals:
  - Triage makes first pass to ask clarifying questions. If it's ok then:
     - Spec owner gets assigned
     - Gets added to `New proposal` in feature tracking board
     - Assigned owner is responsible for following the feature process
* Everything else:
  - If author needs to provide more info, ask in comments and add `needs-author-feedback`
  - Add a team label (`team-Controls`, `team-Framework`, etc) if missing
  - Add area tag(s)
  - Add tags for type of issue (`bug`, `test issue`, `spec issue`, `documentation`)
  - Change labels to `question` or `discussion` if appropriate
  - Consider adding `help wanted` or `good first issue` to encourage community engagement
  - Add `nice to have` for low priority issues

The temporary `needs-assignee-attention` label is intended for issues which need additional investigation, like debugging or another teams input, to determine how to route them. 

### Backlog

[Assigned issues](https://github.com/microsoft/microsoft-ui-xaml/issues/assigned/*) are being investigated or worked on. This doesn't mean they *will* be fixed soon, just that they are on the
short list for that person to investigate, and possibly fix. They may get unassigned.

[Unassigned issues](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+no%3Aassignee) are the backlog. 
     
### Triage queries

Shortcuts to triage queries for the team. Note that these include closed issues because external comments on closed issues may not be
noticed, so we have a bot rule that adds `needs-triage` to closed issues too.

* [Controls Triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Aneeds-triage+-label%3Ateam-framework+-label%3Ateam-reach+-label%3Ateam-rendering+-label%3Ateam-ink+-label%3Ateam-compinput++-label%3Ateam-markup+)
* [Framework Triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Ateam-Framework+label%3Aneeds-triage+)
* [Markup Triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Ateam-Markup+label%3Aneeds-triage+)
* [Reach Triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Ateam-Reach+label%3Aneeds-triage+)
* [Rendering Triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Ateam-Rendering+label%3Aneeds-triage+)

* [Root node triage](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=label%3Aneeds-triage+-label%3Ateam-controls+-label%3Ateam-framework+-label%3Ateam-reach+-label%3Ateam-rendering+-label%3Ateam-ink+-label%3Ateam-compinput++-label%3Ateam-markup+) -- issues not assigned to a team yet.

We also need to monitor:
* [needs-assignee-attention](https://github.com/microsoft/microsoft-ui-xaml/labels/needs-assignee-attention)
* [needs-author-feedback](https://github.com/microsoft/microsoft-ui-xaml/labels/needs-author-feedback)

### Other useful links for contributors

* [good first issue](https://github.com/microsoft/microsoft-ui-xaml/labels/good%20first%20issue)
* [help wanted](https://github.com/microsoft/microsoft-ui-xaml/labels/help%20wanted)
* [unassigned bugs in WinUI 2.x](https://github.com/microsoft/microsoft-ui-xaml/issues?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+label%3Ateam-Controls+no%3Aassignee+-label%3A%22feature+proposal%22++-label%3Aneeds-winui-3+label%3Abug+-label%3Awinui3%CE%B1)

## Bot rules

1. New and re-opened issues get `needs-triage` label added
1. `needs-triage` label is added whenever `team-...` labels change so that the new team sees the status change on the issue.
1. If `feature proposal` is added or removed it gets added/removed from the [feature tracking project board](https://github.com/microsoft/microsoft-ui-xaml/projects/4) accordingly.
1. If `declined` is added, bot adds a friendly message and closes.
1. Tags issues/PR with release announcement.
1. When moving into `Front Burner` column, gets added to the `API review` board.
1. Adds `working on it` to bugs linked as being fixed by a PR.
1. Auto-merge PRs with the `auto merge` label on them.
1. Remove `needs-triage` label when an issue is closed.
1. Remove `needs-triage` when a closed issue has a reply by someone with write access to the repo.
1. Replace `needs-author-feedback` label with `needs-assignee-attention` (if assigned) or `needs-triage` (if unassigned).


[Admin panel](https://fabric-cp.azurewebsites.net/bot/)
