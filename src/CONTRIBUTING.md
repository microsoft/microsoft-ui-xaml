# Contributing to the Windows UI Library

We welcome your input and contributions to all aspects of WinUI, including bug reports, doc updates, feature proposals, and API spec discussions.

This document contains general guidance. More specific guidance is included in the documents linked below.

Note that all community interactions must abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

## How we work with contributions

Contributions from the community in the form of feature requests and bugs are handled according to our [contribution handling](CONTRIBUTION_HANDLING.md) guidelines.

## New contributors

We mark the most straightforward issues with labels. These issues are the place to start if you are interested in contributing but new to the codebase.

* [good first issues](https://github.com/Microsoft/microsoft-ui-xaml/labels/good%20first%20issue)
* [help wanted](https://github.com/Microsoft/microsoft-ui-xaml/labels/help%20wanted)

Another great way to help is by voting and commenting on feature proposals:

* [feature request](https://github.com/Microsoft/microsoft-ui-xaml/labels/feature%20request)

## Code contribution guidelines

### Proposing new public APIs or UI

Please follow the [New Feature or API Process](https://github.com/microsoft/microsoft-ui-xaml/blob/main/docs/feature_proposal_process.md) before adding, removing, or changing public APIs or UI.  
All new public APIs, new UI, or breaking changes to existing features **must** go through that process before submitting code changes.  
You don't need to follow that process for bug fixes or other small changes.

### Contribution bar

The WinUI team accepts code changes that improve WinUI or fix bugs, as long as they follow the processes outlined below and broadly align with the [Windows App SDK feature roadmap](https://github.com/microsoft/WindowsAppSDK/blob/main/docs/roadmap.md).

While we strive to accept all community contributions that meet the guidelines outlined here, please note that we may not merge changes that have narrowly-defined benefits due to compatibility risks and maintenance costs. We may also revert changes if they are found to be breaking.

### Contributor License Agreement

Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

### Copying files from other projects

The following rules must be followed for PRs that include files from another project:

* The license of the file is [permissive](https://en.wikipedia.org/wiki/Permissive_free_software_licence).
* The license of the file is left intact.
* The contribution is correctly attributed in the [3rd party notices](NOTICE.md) file in the repository, as needed.

## Documentation and usage samples

You can also read and contribute to the WinUI documentation here:  
 https://learn.microsoft.com/en-us/windows/apps/winui/winui3/

You can find usage examples of the controls available in WinUI in the WinUI 3 Gallery app:  
 https://github.com/microsoft/WinUI-Gallery/

 which can also be installed from the Windows Store:  
 https://www.microsoft.com/p/winui-3-controls-gallery/9p3jfpwwdzrc
 
 ## API spec discussions

Before new features are added to WinUI, we always perform a thorough API review and spec discussion. This can range from a single new API to an entire new control featuring dozens of new APIs. Joining such a spec discussion is a great opportunity for developers to help ensuring that new WinUI APIs will look and feel natural. In addition, spec discussions are the follow-up to feature proposals and will go into much finer details than the initial proposal. As such, taking part in these discussions gives developers the chance to be involved in the complete development process of new WinUI features - from their initial high-level inception right down to specific implementation/behavior details. These discussions take place in the WInUI repository, i.e. this repository.