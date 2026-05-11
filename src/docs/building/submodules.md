# Developing with Submodules

The WinUI repo references the following git submodule:

* Samples\WinUIGallery from github, for validating the WinUI build

## Overview

A submodule is an independent git repo, consumed as a dependency by a host repo. The relationship between host and submodule is strongly versioned by commit-id. Submodule support was a feature added onto core git functionality, and retains something of a bolted-on feel. So an explicit understanding of submodule operations is important. But the basic commands are fairly straightforward.

## Initializing

Whenever a host repo is updated, all submodules must also be updated to ensure that corresponding sources are in sync. This applies when initially cloning, when pulling remote changes, when switching branches, etc.

Submodule updates can be included in host repo git operations:

```shell
>git clone --recursive https://github.com/microsoft/microsoft-ui-xaml.git
>git checkout --recurse-submodules main
>git pull --recurse-submodules
```

Submodules can also be updated/initialized explicitly (e.g., if an operation above omits recursion):

```shell
>git submodule update --init
```

The **init.cmd** script initializes and updates submodules in case the developer forgets to.  

## Status

### Host Status

For each submodule, the host repo records its remote URL and relative folder (via ``.gitmodules``). And the current commit of the host repo also records a commit-id for the submodule.

If the host's submodule commit-id is out of sync with the submodule's **HEAD** pointer, the host repo will report "new commits" for the submodule:

```shell
>git status
    ...
    modified:   Samples/WinUIGallery (new commits)
```

This can also be seen with a leading '+' sign in the submodule status:

```shell
>git submodule status
+6e236c641d62238ebf5a04d5ea27cb630482ac3f Samples/WinUIGallery
```

This state is not typically a sign of uncommitted submodule modifications. Rather, it's the result of a git clone, pull, or checkout that updates the submodule commit-id, but without updating the submodule itself, as described above. Again, a 'git submodule update' will resolve.

If there are uncommitted changes in a submodule, the host repo will report that the submodule has "modified content":

```shell
>git status
    ...
    modified:   Samples/WinUIGallery (modified content)
```

One downside of submodules is that it's not immediately obvious from the status above what the modified content is. To see that, it's necessary to change into the submodule directory (again, a separate repo) and run a 'git status'.

It's possible to configure the host repo to display status recursively, to include submodules, via:

```shell
>git config --global status.submoduleSummary true
```

However, this can slow down git status significantly, so isn't recommended.

### Submodule Status

Typically, a submodule will report "Detached HEAD" for git status. This simply means that the HEAD is not pointing to a working branch but just a raw commit. Again, the relationship between host and submodule is based on a specific commit, not a branch (which can evolve). To develop in the submodule, a topic branch must be created (see below).

Depending on your git command history, the submodule may report "HEAD detached **at** commit-id" or "HEAD detached **from** commit-id", and the commit-id may vary, as it references the point at which the submodule was detached and not its current state. This can be a bit confusing, so it's best to ignore the "HEAD detached" commit-id and rely on the host's submodule status and/or the submodule's 'git log' status. The important thing is to be aware of uncommitted submodule modifications.  

## Developing

Once all submodules have been updated, development in the WinUI repo itself proceeds as usual. Changes are committed to a topic branch and pushed to a remote.

When coordinated changes are required between the host repo and a submodule, the typical sequence is to modify the submodule, commit changes to a topic branch, and then point the host to that commit.

For example:

```shell
>cd Samples\WinUIGallery
>git checkout -b user/username/gallery_update
>git commit -a -m"My changes to gallery
>git push
>cd ..
>git checkout -b user/username/xaml_update
>git commit -a -m"Reference my changes to gallery
>git push
```

Note that whether a developer chooses to merge a submodule topic branch to main is irrelevant to the host repo. FIs and RIs can be done periodically, to pick up fixes and stay in sync. But for the host repo, only the commit-id matters.

For a deeper dive into submodules, see also [Mastering Git submodules](https://medium.com/@porteneuve/mastering-git-submodules-34c65e940407).
