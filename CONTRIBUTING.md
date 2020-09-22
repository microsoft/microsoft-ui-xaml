# Contributing to the Windows UI Library

We welcome your input and contributions to all aspects of WinUI, including bug reports, doc updates, feature proposals, code contributions and spec discussions.

This document contains general guidance. More specific guidance is included in the documents linked below.

Note that all community interactions must abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

## Issues

We use GitHub issues to track bugs and features.

For reporting security issues please see the [Security Policy](docs/SECURITY.md).

For all other bugs and general issues please [file a new issue](https://github.com/Microsoft/microsoft-ui-xaml/issues/new/choose) using the Bug Report template.

## New contributors

We mark the most straightforward issues with labels. These issues are the place to start if you are interested in contributing but new to the codebase.

* [good first issues](https://github.com/Microsoft/microsoft-ui-xaml/labels/good%20first%20issue)
* [help wanted](https://github.com/Microsoft/microsoft-ui-xaml/labels/help%20wanted)

Another great way to help is by voting and commenting on feature proposals:

* [feature request](https://github.com/Microsoft/microsoft-ui-xaml/labels/feature%20request)

## Code contribution guidelines

### Proposing new public APIs or UI

Please follow the [New Feature or API Process](docs/feature_proposal_process.md) before adding, removing, or changing public APIs or UI.  
All new public APIs, new UI, or breaking changes to existing features **must** go through that process before submitting code changes.  
You don't need to follow that process for bug fixes or other small changes.

### Contribution bar

The WinUI team accepts code changes that improve WinUI or fix bugs, as long as they follow the processes outlined below and broadly align with our [roadmap](docs/roadmap.md).

While we strive to accept all community contributions that meet the guidelines outlined here, please note that we may not merge changes that have narrowly-defined benefits due to compatibility risks and maintenance costs. We may also revert changes if they are found to be breaking.

### Code contribution process

For details see:

* [Setup and build environment](docs/developer_guide.md#Prerequisites)
* [Source code structure](docs/source_code_structure.md)
* [Contribution workflow](docs/contribution_workflow.md)
* [Coding style and conventions](docs/code_style_and_conventions.md)

### Contributor License Agreement

Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

### Copying files from other projects

The following rules must be followed for PRs that include files from another project:

* The license of the file is [permissive](https://en.wikipedia.org/wiki/Permissive_free_software_licence).
* The license of the file is left intact.
* The contribution is correctly attributed in the [3rd party notices](https://github.com/dotnet/coreclr/blob/master/THIRD-PARTY-NOTICES.TXT)
file in the repository, as needed.

## Documentation and usage samples

You can also read and contribute to the WinUI documentation here:  
https://docs.microsoft.com/uwp/toolkits/winui

You can find usage examples of the controls available in WinUI in the Xaml Controls Gallery app:  
 https://github.com/Microsoft/Xaml-Controls-Gallery/  

 which can also be installed from the Windows Store:  
 https://www.microsoft.com/p/xaml-controls-gallery/9msvh128x2zt
 
 ## Spec discussions/API reviews

Before new features will be implemented the WinUI team will do a thorough spec discussion/API review. This can range from a single new API to an entire new control featuring dozens of new APIs. Joining such a discussion/review is a great opportunity for developers to help ensuring that new WinUI APIs will look and feel natural and are a joy to work with. In addition, spec discussions are the follow-up to feature proposals and will go into much finer details than the initial proposal. As such, taking part in these discussions gives developers the chance to be involved in the complete development process of new WinUI features - from their initial high-level inception right down to specific implementation/behavior details.

These discussions will be take place in their own [repository](https://github.com/microsoft/microsoft-ui-xaml-specs). While specs for feature proposals will typically be linked to in the specific proposal on the WinUI repository, not all spec discussions/reviews will immediately be mentioned there. Thus, if you don't want to miss out on these, we recommend that you [watch the repository](https://docs.github.com/en/enterprise/2.15/user/articles/watching-and-unwatching-repositories).
