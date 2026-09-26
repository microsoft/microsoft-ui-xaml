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

4. **Expect test coverage for every bug fix and new feature.** Bug fixes should
   include regression tests that fail without the fix and pass with it. New
   features should have tests for their intended behavior and relevant edge
   cases. Use the repo's existing test patterns and flag missing coverage.

5. **Consider behavioral compatibility, even across major versions.** WinUI apps
   are historically very sensitive to behavioral changes that seem minor.  A semver
   major version increase can still require care when changing existing behavior.
   Apps may depend on how WinUI APIs behave, including undocumented behavior
   or dependencies they took accidentally. For example, changing the order
   in which events fire can break apps even when API signatures stay the same.
   Highlight potential compatibility risks and consider whether the change
   needs additional tests or a gradual rollout.

   For behavioral changes, features can use `XamlOptionalChanges` to let apps
   opt in to the new behavior first. These behaviors are expected to become
   the default later, with apps able to opt out. Consider this approach when
   a change could affect existing apps.
