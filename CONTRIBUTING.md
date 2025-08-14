# Contributing to the Windows UI Library
Welcome to the Windows UI Library (WinUI) repository. The WinUI repo is intended as a place for the WinUI team to gather community feedback, discuss issues with the community, and provide insight into bug fixes that the team is working on before updates are released. We welcome your input and contributions to all aspects of WinUI, including bug reports, doc updates, feature proposals, and API spec discussions.

This document contains general guidance. More specific guidance is included in the documents linked below and within the repository’s [Wiki](https://github.com/microsoft/microsoft-ui-xaml/wiki) pages.

Note this repository is not ready for open source collaboration at this time; this work is currently in progress. You can track the WinUI team’s progress towards open source collaboration in the [Phased Rollout to Open Source Collaboration](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) discussion.

Note that all community interactions must abide by the [Code of Conduct](CODE_OF_CONDUCT.md). We strive to be respectful & empathetic toward each other, and we extend this to our community as well. Our conduct guidelines help to ensure that discourse and feedback follows this same pattern of respect & empathy, and they also make it clear that non-constructive or hostile feedback is unacceptable. By following these guidelines, we can all enjoy the mutual respect that each WinUI team member, and each community member, deserve.

## Documentation and Samples
The [WinUI](https://aka.ms/winui3) documentation is a great resource for learning more about WinUI development. Within these docs, you can find information on how to create new WinUI applications, the full set of WinUI APIs and controls, and more.

You’re welcome to propose changes to our documentation through the same link.

You can find usage examples of the controls available in WinUI in the [WinUI 3 Gallery app](https://apps.microsoft.com/detail/9p3jfpwwdzrc). The source code for WinUI3 Gallery is available on GitHub at [microsoft/WinUI-Gallery]( https://github.com/Microsoft/WinUI-Gallery/).

## Filing New Issues
If you have a general question on how to use WinUI and are not sure it’s a bug, ask in the [Q&A](https://github.com/microsoft/microsoft-ui-xaml/discussions/categories/q-a) discussion forum.

If you have an idea or feature request, post in the [Ideas](https://github.com/microsoft/microsoft-ui-xaml/discussions/categories/ideas) discussion forum.

If you have an issue which you think is a bug, please follow the [Bug Report](https://github.com/microsoft/microsoft-ui-xaml/issues/new?template=bug_report.yaml) issue template and provide a stand-alone minimal repo project.

If you are reporting a security issue, please see the [Security Policy](SECURITY.md).

For more information on how issues are handled from the community, see our [contribution handling](docs/contribution_handling.md) guidelines.

### Proposing New Public APIs or UI
Please follow the [New Feature or API Process](docs/feature_proposal_process.md) before adding, removing, or changing public APIs or UI. Note: The WinUI team will only accept feature proposals for WinUI3.

All new public APIs, new UI, or breaking changes to existing features must go through that process before submitting code changes.

### Raising Feedback
When experiencing a pain-point, it can be natural to focus on the negatives and resolving things from the perspective of the negative. However, telling the team what you'd like to *prevent* is less helpful and actionable than telling the team what you'd like to *achieve*. 

Your feedback is most effective when it is a constructive call to action on the team, and is clear and detailed – especially on the "why" so that we can make sure whatever it is that we arrive at together appropriately focuses on your goal and your intended outcome from start to finish. 


**Examples of constructively phrased feedback:**

Instead of:

- The state of the platform is disappointing. I am not going to consider WinUI until my trust has been earned.

Try this:
- Deprecation of past frameworks has left me burned. The following would go a long way in earning my trust because my company is trying to launch an app next year and will need it supported for at least 5 more: 
    - OSS
    - High-confidence 5-year roadmap
    - Guaranteed support timeline

    This feedback provides clear actionable items that the WinUI team can take into consideration and address. 

Here are some areas where your feedback can have a large impact:

#### Features
It's very helpful to specify the "why" behind your request, even if it seems obvious. Comparisons can help, but shouldn't replace explanation. 
- Ex: "I need `<`feature`>` so that I can achieve `<`goal`>`. Otherwise, I have to `<`alternative`>`."

There are also usually great opportunities to have feature-related impact in our [spec repo](https://github.com/microsoft/microsoft-ui-xaml-specs/tree/master). Here, you can give direct feedback and help shape the new features and APIs that we're currently building. 

#### Future/Roadmap
We strive to be transparent about our [product roadmap](https://aka.ms/winui3/feature-roadmap). Great examples of feedback to help us achieve this are:
- "I'd like to know the roadmap through `<`timeframe`>`. Otherwise, I can't do `<`goal`>`. "
- "The roadmap `<`certain aspect - e.g., changes too much, not detailed enough, etc.`>`prevents me from being able to do `<`goal`>`. If you would instead `<`proposed alternative`>`, that would result in `<`clear benefit`>`."

#### Ecosystem
It's helpful to be complete in explaining what another solution achieves for you and why it is important to you. 
- Ex:"I'd like WinUI 3 to work with `<`framework, language, library, technology, etc.`>`. Without this, I can't do `<`goal`>`."

#### Prioritization within WinUI
There is often trade-off required when adjusting priorities - please be clear in comparing what is more important vs. less important to you, and the reason why. 
- Ex: "I think `<`specific area or feature set`>` should be prioritized before `<`other specific area or feature set`>` so that I can achieve `<`goal`>`. Without this, I have to `<`alternative`>`. "  

#### Engagement
Engagement includes feedback about learning materials and resources. It is always helpful to know where you prefer to be engaged and how you prefer to learn. 
- "I would like to hear more about `<`specific topic`>` in `<`resource`>` so that I can achieve `<`goal`>`."
- "`<`Resource`>` could be more inclusive and accessible if you considered `<`alternative`>` because `<`benefit`>`. Have you considered making these changes?"
   
Many of the resources we provide are also open source themselves! It can sometimes be more effective to leave feedback on the more specific repo. This includes:
- [documentation](https://github.com/MicrosoftDocs/windows-uwp)
- [website (`gh-pages` branch of this repo)](https://github.com/microsoft/microsoft-ui-xaml/tree/gh-pages)
- [Xaml Controls Gallery (sample app)](https://github.com/microsoft/Xaml-Controls-Gallery/tree/master)

#### Team Process
Team process includes feedback about our repo and overall communication. Suggestions for improvement and proposed alternatives are very helpful here. 
- Ex: "`<`specific aspect of team process - e.g., response frequency, format`>` of the repo is making it hard for me to achieve `<`goal`>`. Please consider doing `<`alternative`>` instead because that would help `<`action`>`" 

## Code Contribution Guidelines
Note this repository is not ready for open source collaboration at this time; this work is currently in progress. You can track the WinUI team’s progress towards open source collaboration in the [Phased Rollout to Open Source Collaboration](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) discussion.

WinUI will be taking a phased approach to opening up the repo:
**Phase 1: Increased Mirror Frequency**
After the WASDK 1.8 release (end of August), we’ll begin more frequent mirroring of internal commits to GitHub to increase transparency and show progress.
**Phase 2: 3rd Party Devs Build Locally**
External developers will be able to clone and build the repo locally, with documentation to guide setup and dependencies.
**Phase 3: 3rd Party Devs Contribute & Run Tests**
Contributors will be able to submit PRs and run tests locally. We’re working to untangle private dependencies and make test infrastructure publicly accessible.
**Phase 4: GitHub as Center of Gravity**
GitHub becomes the primary place for development, issue tracking, and community engagement. Internal mirrors will be phased out.

### [WIP] New Contributors
Contributions from the community are greatly appreciated. We mark the most straightforward issues with labels. These issues are the best place to start if you are interested in contributing but are new to the codebase.

- [good first issues](https://github.com/Microsoft/microsoft-ui-xaml/labels/good first issue)
- [help wanted](https://github.com/orgs/microsoft/projects/1868/views/12)

Another great way to help is by up-voting and commenting on feature proposals:
- [feature request](https://github.com/Microsoft/microsoft-ui-xaml/labels/feature request)

### [WIP] Code Contribution Process
We use and recommend the following workflow:
1. Create an issue for your work. 
    - You can skip this step for trivial changes or if there is an existing issue for the bug/feature.
    - If your change adds or changes public APIs or UI, first follow the [New Feature or API Process](docs/feature_proposal_process.md).
    - Clearly state that you are going to take on implementing it, if that's the case. You can request that the issue be assigned to you. Note: The issue filer and the implementer don't have to be the same person.
1. Create a personal fork of the repository on GitHub (if you don't already have one).
1. Create a branch off of main (git checkout -b mybranch). 
    - Name the branch so that it clearly communicates your intentions (i.e. /user/janedoe/fix-datepicker).
    - Branches are useful since they isolate your changes from incoming changes from upstream. They also enable you to create multiple PRs from the same fork.
1. Make and commit your changes. 
    - Please follow our [Commit Messages](#commit-messages) guidance.
1. Add new tests corresponding to your change, if applicable.
1. Build the repository with your changes. 
    - Make sure that the builds are clean.
    - Make sure that the tests are all passing, including your new tests.
1. Create a pull request (PR) against this repository's main branch. 
    - Push your changes to your fork on GitHub (if you haven't already).
    - Note: It is okay for your PR to include a large number of commits. Once your change is accepted, you will be asked to squash your commits into one or some appropriately small number of commits before your PR is merged.
    - Note: It is okay to create your PR as "[WIP]" before the implementation is done. This can be useful if you'd like to start the feedback process while you're still working on the implementation. State that this is the case in the initial PR comment.

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
Each pull request to main must pass the following checks: [WinUI-Public-MUX-PR](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=21)

This pipeline builds your change and runs automated tests. These tests should match what you're able to run with local automated testing using Test Explorer. It also creates a NuGet package to match your change.

The license/cla check confirms that you have completed the [CLA](https://cla.microsoft.com/).

Pull requests from a fork will not automatically trigger all of these checks. A member of the WinUI team can trigger the Azure Pipeline checks by commenting /azp run on the PR. The Azure Pipelines bot will then trigger the build.

In order to have PRs automatically merge once all checks have passed (including optional checks), maintainers can apply the [auto merge](https://github.com/Microsoft/microsoft-ui-xaml/labels/auto merge) label. It will take effect after an 8 hour delay.

### Other Pipelines
Unlike the above checks these are not required for all PRs, but you may see them on some PRs: [WinUI-Public-MUX-CI](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=20)

This pipeline extends [WinUI-Public-MUX-PR](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=21) to validate more platforms, adding Debug and ARM. It is run after your changes are merged to main.
s