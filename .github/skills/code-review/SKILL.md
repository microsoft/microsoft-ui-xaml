---
name: code-review
description: Review changes in microsoft/microsoft-ui-xaml. Use when reviewing code or pull requests in this repository.
---

# Code Review

1. **Match the coding style of the current file.** Check that changes follow
   the file's existing naming, formatting, and code patterns. Do not request
   unrelated style changes.

2. **Do not include Microsoft internal information.** Check code, comments,
   documentation, and PR content for internal-only information, such as
   internal links, confidential details, or credentials. Do not copy that
   information into review comments; identify its location without repeating it.

3. **Check open PRs for related work.** Look at open pull requests in
   `microsoft/microsoft-ui-xaml` for changes to the same files, components, or
   behavior. Inspect likely matches and mention relevant overlap, conflicts,
   or duplicate work with links to those PRs. If you cannot access the open
   PRs, state that this check could not be completed.

4. **Check behavioral compatibility, even across major versions.** A semver
   major version increase does not remove the need to protect existing apps.
   Apps may depend on how WinUI APIs behave, including undocumented behavior
   or dependencies they took accidentally. For example, changing the order
   in which events fire can break apps even when API signatures stay the same.
   Flag changes to observable behavior and explain how they could affect
   existing apps. Check that intentional behavior changes have an explicit
   compatibility assessment and tests covering the affected scenarios.
