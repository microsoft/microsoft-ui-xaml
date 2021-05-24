# Contribution Workflow

You can contribute to WinUI with issues and PRs. Simply filing issues for 
problems you encounter is a great way to contribute. Contributing 
implementations is greatly appreciated.

Good issues to work on are issues tagged with [help wanted](https://github.com/microsoft/microsoft-ui-xaml/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22) or [good first issue](https://github.com/microsoft/microsoft-ui-xaml/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22).

## Suggested Workflow

We use and recommend the following workflow:
1. Create an issue for your work. 
    * You can skip this step for trivial changes.
    * Reuse an existing issue on the topic, if there is one.
    * If your change adds or changes public API or UI, first follow
    the [New Feature or API Process](feature_proposal_process.md).
    * Clearly state that you are going to take on implementing it, if that's 
    the case. You can request that the issue be assigned to you. Note: The 
    issue filer and the implementer don't have to be the same person.
2. Create a personal fork of the repository on GitHub (if you don't already 
have one).
3. Create a branch off of main (`git checkout -b mybranch`). 
    * Name the branch so that it clearly communicates your intentions.
    * Branches are useful since they isolate your changes from incoming changes 
    from upstream. They also enable you to create multiple PRs from the same 
    fork.
4. Make and commit your changes. 
    * Please follow our [Commit Messages](contribution_workflow.md#Commit-Messages) 
    guidance.
5. Add [new tests](developer_guide.md#Testing) corresponding to your change, if applicable.
6. Build the repository with your changes. 
    * Make sure that the builds are clean.
    * Make sure that the [tests](developer_guide.md#Testing) are all passing, including your new 
    tests.
7. Create a pull request (PR) against the upstream repository's main branch. 
    * Push your changes to your fork on GitHub (if you haven't already).
    - Note: It is okay for your PR to include a large number of commits. Once 
    your change is accepted, you will be asked to squash your commits into one 
    or some appropriately small number of commits before your PR is merged.
    - Note: It is okay to create your PR as "[WIP]" on the upstream repo before 
    the implementation is done. This can be useful if you'd like to start the 
    feedback process while you're still working on the implementation. State 
    that this is the case in the initial PR comment.

## DOs and DON'Ts

Please do:
* **DO** follow our [coding style](code_style_and_conventions.md).
* **DO** give priority to the current style of the project or file you're 
changing even if it diverges from the general guidelines.
* **DO** include tests when adding new features. When fixing bugs, start with 
adding a test that highlights how the current behavior is broken.
* **DO** keep the discussions focused. When a new or related topic comes up 
it's often better to create new issue than to side track the discussion.
* **DO** blog and tweet (or whatever) about your contributions, frequently!

Please do not:
* **DON'T** make PRs for style changes.
* **DON'T** surprise us with big pull requests. Instead, file an issue and 
start a discussion so we can agree on a direction before you invest a large 
amount of time.
* **DON'T** commit code that you didn't write (attesting this is part of the 
[CLA](https://cla.microsoft.com)). If you find code that you think is a good 
fit to add to WinUI, file an issue and start a discussion before proceeding.
* **DON'T** submit PRs that alter licensing related files or headers. If you 
believe there's a problem with them, file an issue and we'll be happy to 
discuss it.
* **DON'T** add or change public API or UI without filing an issue and 
discussing it first: see the [New Feature or API Process](feature_proposal_process.md).

## Checks

Each pull request to `main` must pass the following checks:

###### [WinUI-Public-MUX-PR](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=21)

This pipeline builds your change and runs automated tests.
These tests should match what you're able to run with local automated testing using Test Explorer.
It also creates a NuGet package to match your change.

###### license/cla

This check confirms that you have completed the [CLA](https://cla.microsoft.com).


Pull requests from a fork will not automatically trigger all of these checks. A member of the WinUI 
team can trigger the Azure Pipeline checks by commenting `/azp run` on the PR. The Azure Pipelines
bot will then trigger the build.

In order to have PRs automatically merge once all checks have passed (including optional 
checks), maintainers can apply the [auto merge](https://github.com/Microsoft/microsoft-ui-xaml/labels/auto%20merge) 
label. It will take effect after an 8 hour delay, [more info here (internal link)](https://microsoft.sharepoint.com/teams/FabricBot/SitePages/AutoMerge,-Bot-Templates-and.aspx).

### Other Pipelines

Unlike the above checks these are not required for all PRs, but you may see them on some PRs so we 
define them here:

#### [WinUI-Public-MUX-CI](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=20)

This pipeline extends [WinUI-Public-MUX-PR](https://dev.azure.com/ms/microsoft-ui-xaml/_build?definitionId=21) 
to validate more platforms, adding Debug and ARM. It is run after your changes are merged to 
main.

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