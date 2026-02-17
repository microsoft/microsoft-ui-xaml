# Contributing to the Windows UI Library
Welcome to the Windows UI Library (WinUI) repository. The WinUI repo is intended as a place for the WinUI team to gather community feedback, discuss issues with the community, and provide insight into bug fixes that the team is working on before updates are released. We welcome your input and contributions to all aspects of WinUI, including bug reports, doc updates, feature proposals, and API spec discussions.

This document contains general guidance. More specific guidance is included in the documents linked below and within the repository's [Wiki](https://github.com/microsoft/microsoft-ui-xaml/wiki) pages.

Note this repository is not ready for open source collaboration at this time; this work is currently in progress. You can track the WinUI team's progress towards open source collaboration in the [Phased Rollout to Open Source Collaboration](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) discussion.

Note that all community interactions must abide by the [Code of Conduct](CODE_OF_CONDUCT.md). We strive to be respectful & empathetic toward each other, and we extend this to our community as well. Our conduct guidelines help to ensure that discourse and feedback follows this same pattern of respect & empathy, and they also make it clear that non-constructive or hostile feedback is unacceptable. By following these guidelines, we can all enjoy the mutual respect that each WinUI team member, and each community member, deserve.

## Documentation and Samples
The [WinUI](https://aka.ms/winui3) documentation is a great resource for learning more about WinUI development. Within these docs, you can find information on how to create new WinUI applications, the full set of WinUI APIs and controls, and more.

You're welcome to propose changes to our documentation through the same link.

You can find usage examples of the controls available in WinUI in the [WinUI 3 Gallery app](https://apps.microsoft.com/detail/9p3jfpwwdzrc). The source code for WinUI3 Gallery is available on GitHub at [microsoft/WinUI-Gallery]( https://github.com/Microsoft/WinUI-Gallery/).

## Filing New Issues
If you have a general question on how to use WinUI and are not sure it's a bug, ask in the [Q&A](https://github.com/microsoft/microsoft-ui-xaml/discussions/categories/q-a) discussion forum.

If you have an idea or feature request, post in the [Ideas](https://github.com/microsoft/microsoft-ui-xaml/discussions/categories/ideas) discussion forum.

If you have an issue which you think is a bug, please follow the [Bug Report](https://github.com/microsoft/microsoft-ui-xaml/issues/new?template=bug_report.yaml) issue template and provide a stand-alone minimal repo project.

If you are reporting a security issue, please see the [Security Policy](SECURITY.md).

For more information on how issues are handled from the community, see our [contribution handling](docs/contribution_handling.md) guidelines.

### Proposing New Public APIs or UI
Please follow the [New Feature or API Process](docs/feature_proposal_process.md) before adding, removing, or changing public APIs or UI. Note: The WinUI team will only accept feature proposals for WinUI3.

All new public APIs, new UI, or breaking changes to existing features must go through that process before submitting code changes.

## Code Contribution Guidelines
Note this repository is not ready for open source collaboration at this time; this work is currently in progress. You can track the WinUI team's progress towards open source collaboration in the [Phased Rollout to Open Source Collaboration](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) discussion.

WinUI will be taking a phased approach to opening up the repo:

| Phase | Description |
|--|--|
|**Phase 1: Increased Mirror Frequency** | After the WASDK 1.8 release (end of September), we'll begin more frequent mirroring of internal commits to GitHub to increase transparency and show progress. |
|**Phase 2: 3rd Party Devs Build Locally** | External developers will be able to clone and build the repo locally, with documentation to guide setup and dependencies. |
| **Phase 3: 3rd Party Devs Contribute & Run Tests** | Contributors will be able to submit PRs and run tests locally. We're working to untangle private dependencies and make test infrastructure publicly accessible. |
| **Phase 4: GitHub as Center of Gravity** | GitHub becomes the primary place for development, issue tracking, and community engagement. Internal mirrors will be phased out. |

### [WIP] New Contributors
Contributions from the community are greatly appreciated. We mark the most straightforward issues with labels. These issues are the best place to start if you are interested in contributing but are new to the codebase.

- [good first issues](https://github.com/Microsoft/microsoft-ui-xaml/issues?q=state%3Aopen%20label%3A%22good%20first%20issue%22)
- [help wanted](https://github.com/orgs/microsoft/projects/1868/views/12)

Another great way to help is by up-voting and commenting on feature proposals:
- [feature proposals](https://github.com/Microsoft/microsoft-ui-xaml/labels/feature%20proposal)

### [WIP] Code Contribution Process
Content will be added to this section once the reposority is ready for open source contribution. 

### Contribution Bar
The WinUI team accepts code changes that improve WinUI or fix bugs, as long as they follow the processes outlined below and broadly align with our [roadmap](docs/roadmap.md).

While we strive to accept all community contributions that meet the guidelines outlined here, please note that we may not merge changes that have narrowly-defined benefits due to compatibility risks and maintenance costs. We may also revert changes if they are found to be breaking.

### Contributor License Agreement
Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

### Copying files from other projects
The following rules must be followed for PRs that include files from another project:
- The license of the file is [permissive](https://en.wikipedia.org/wiki/Permissive_free_software_licence).
- The license of the file is left intact.
- The contribution is correctly attributed in the [3rd party notices](https://github.com/dotnet/coreclr/blob/master/THIRD-PARTY-NOTICES.TXT) file in the repository, as needed.

## Commit Messages
Please format commit messages as follows (based on [A Note About Git Commit Messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)):

```
Summarize change in 50 characters or less

Provide more detail after the first line. Leave one blank line below the 
summary and wrap all lines at 72 characters or less.

If the change fixes an issue, leave another blank line after the final 
paragraph and indicate which issue is fixed in the specific format below.

Fix #42
```

## Checks
Each pull request to main must pass checks within Azure DevOps. These pipelines build your change and run automated tests.

The license/CLA check confirms that you have completed the [CLA](https://cla.microsoft.com/).

Pull requests from a fork will not automatically trigger all of these checks. A member of the WinUI team can trigger the Azure Pipeline checks by commenting `/azp run` on the PR. The Azure Pipelines bot will then trigger the build.

In order to have PRs automatically merge once all checks have passed (including optional checks), maintainers can apply the [auto merge](https://github.com/microsoft/microsoft-ui-xaml/labels/auto%20merge) label. It will take effect after an 8 hour delay.
