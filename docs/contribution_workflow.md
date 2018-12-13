# Contribution Workflow

You can contribute to WinUI with issues and PRs. Simply filing issues for 
problems you encounter is a great way to contribute. Contributing 
implementations is greatly appreciated.

## Suggested Workflow

We use and recommend the following workflow:
1. Create an issue for your work. 
    * You can skip this step for trivial changes.
    * Reuse an existing issue on the topic, if there is one.
    * Identify stakeholders and get agreement on the direction of your proposed
     change.
    * If your change adds or changes public API or UI, first follow
    the [New Feature or API Process](feature_proposal_process.md).
    * Clearly state that you are going to take on implementing it, if that's 
    the case. You can request that the issue be assigned to you. Note: The 
    issue filer and the implementer don't have to be the same person.
2. Create a personal fork of the repository on GitHub (if you don't already 
have one).
3. Create a branch off of master (`git checkout -b mybranch`). 
    * Name the branch so that it clearly communicates your intentions, such as 
    user/your-github-handle/issue-name.
    * Branches are useful since they isolate your changes from incoming changes 
    from upstream. They also enable you to create multiple PRs from the same 
    fork.
4. Make and commit your changes. 
    * Please follow our [Commit Messages](contribution_workflow.md#Commit%20Messages) 
    guidance.
5. Add [new tests](developer_guide.md#Testing) corresponding to your change, if applicable.
6. Build the repository with your changes. 
    * Make sure that the builds are clean.
    * Make sure that the [tests](developer_guide.md#Testing) are all passing, including your new tests.
7. Create a pull request (PR) against the upstream repository's master branch. 
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
* **DO** follow our [coding style](developer_guide.md#Code%20style%20and%20conventions).
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

Each pull request must pass the following checks.

#### WinUI_build_OS

This check essentially makes sure that your change actually builds.

One snag you might hit is a failure in `PeformDEPControlsPort.cmd`. This 
process validates the compatibility of your change with the port to the Windows
build system and [Windows.UI.Xaml.Controls](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls).
Unfortunately this process cannot be run locally without authentication to the 
Windows build, so if you run into a problem with this validation step you may 
need the help of a Microsoft employee. You may be able to look at similar code 
and its use of `BUILD_WINDOWS` to figure out what you need to do, but feel free 
to @ mention Microsoft team members to ask for help.

#### WinUI-Public-MUX-PR

This check runs automated tests on your change. These tests should match what 
you're able to run with local automated testing using Test Explorer.

#### license/cla

This check confirms that you have completed the [CLA](https://cla.microsoft.com).

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

Also do your best to factor commits appropriately, not too large with unrelated 
things in the same commit, and not too small with the same small change applied 
N times in N different commits.