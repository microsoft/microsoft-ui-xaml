# WinUI Repo Contribution Handling

The WinUI repo is intended as a place for the WinUI team to gather community feedback, discuss issues with the community, and provide insight into bug fixes that the team is working on before updates are released. Here, we'll outline how we handle feature requests and bugs that the community opens.

## Issues

Feature requests and bugs are tracked as GitHub issues.

For reporting security issues please see the [Security Policy](SECURITY.md).

For all other bugs and general issues please [file a new issue](https://github.com/Microsoft/microsoft-ui-xaml/issues/new/choose) using the Bug Report template.

Please file questions and discussions as discussions, not issues.

## Feature Proposals

Feature proposals for WinUI 3 are categorized based on the likely timeframe in which the feature team can consider them.

1. 	Short-term, less than a year
2.	Long-term, not likely in the next year

All feature proposals are automatically categorized as long-term and unlikely to be considered, although the WinUI team might change the priority based on customer and business needs. If the WinUI team does close a feature proposal, we will indicate the reason why and how it fits into our plans. Closing a proposal does not mean that the team will never consider it. We encourage the community to add comments or interact with a proposal at any time, even a closed one, to signal its importance to the team.

### Feature Planning

Our long term vision for WinUI is to be the best UX/UI native framework for Windows PCs that provides premium visuals, motion, accessibility, usability, and support for all input modalities.

The current details of what we plan long-term are shaped by evolving priorities that are subject to frequent change and are often confidential based on business and customer needs. However, we are happy to share our immediate plans for what's coming next in the next major release once the feature list is ready. Additionally, preview and experimental releases provide opportunities for the community to see what's new and what's changing on a more frequent basis.

For more info about our longer-term planning, see the [Windows App SDK feature roadmap](https://github.com/microsoft/WindowsAppSDK/blob/main/docs/roadmap.md). This page will be updated generally right after a major release, when the features for the next release are solidified enough to share.

## Bugs

### WinUI 2

The WinUI team is currently spending its time on WinUI 3. To help with this focus, any new bug filed against WinUI 2 is closed in this repo unless it's a security issue or is critical to our business priorities.

### WinUI 3

WinUI 3 bugs are triaged and prioritized based on how much the bug impacts one or more of the following criteria. If a bug has lower impact, it is less likely to be considered for fixing.

- Reliability
- Data corruption or loss
- Functionality (such as a major regression)
- Security
- Compatibility with applications
- User experience/usability
- Performance
- Compliance (such as legal compliance)
- Accessibility
- Build and deployment

Bugs filed in relation to preview and experimental releases are triaged with the same criteria as stable releases and are intended to assist in identifying and fixing those bugs before they make their way to a stable release.

Bugs filed on GitHub will be automatically mirrored to our internal bug tracking system and updates to them are automatically mirrored back to GitHub with appropriate tags. This way, the bugs we're working on and their progress are transparent. The only information we will reflect from our system externally is the state and release of the bug, not all the fine details; if a bug is resolved or closed, that's reflected on GitHub, and as bugs are worked on, the release for that fix will be reflected with a milestone on GitHub.