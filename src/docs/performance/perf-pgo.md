# Perf PGO

## Table of Contents

- [Description](#description)
  - [Scenarios](#scenarios)
    - [Optimizing on PGO’d branch](#optimizing-on-pgod-branch)
    - [Optimizing release branch](#optimizing-release-branch)
    - [Optimizing topic branch](#optimizing-topic-branch)

# Description

We need to provide a PGO database for CI and release builds.  PGO database is not going to be built
on every build but we still would like to be able to optimize all builds.  In this solution we
generate PGO database NuGet package which is versioned based on product release version and
branch name/time stamp of the code that was used for instrumentation and training.  In CI/release
builds an initialization step enumerates all available versions, filters out those for other
releases and branches.  Given a list of applicable versions, it will find the one that is closest
(BEFORE) the time-stamp of the last commit or a fork-point from instrumented branch.  That package
version will be installed and version references will be updated.

The PGO branch is determined by variable `PGOBranch` in the [`perf\pgo\Microsoft.WinUI.PGO.props`](../../perf/pgo/Microsoft.WinUI.PGO.props) file.

## Scenarios

For the purpose of illustration, let’s assume the following is a chronological list of check-ins to
two branches (main and release/3.0).  Some of them have had instrumentation/training run done on
them and have generated PGO NuGets (version numbers in parentheses).  To simplify, let's assume
that release major and minor versions are the same for all check-in as they merely act as filters
for what versions are considered to be available.

    1b27fd5f -- main        --
    7b303f74 -- main        --
    930ff585 -- main        -- 3.0.2001312227-main
    63948a75 -- main        --
    0d379b51 -- main        --
    f23f1fad -- main        -- 3.0.2001312205-main
    bcf9adaa -- main        --
    6ef44a23 -- main        --
    310bc133 -- release/3.0 --
    80a4ab55 -- release/3.0 -- 3.0.2001312054-release_3_0
    18b956f6 -- release/3.0 --
    4abd4d54 -- main        -- 3.0.2001312033-main
    d150eae0 -- main        -- 3.0.2001312028-main

### Optimizing on PGO’d branch

If we are building on main (which in this example is PGO'd), the version picked will be the one
that has the same major and minor versions AND branch name and is the same or is right before the
SHA being built.

E.g.

    1b27fd5f -- 3.0.2001312227-main
    f23f1fad -- 3.0.2001312205-main
    bcf9adaa -- 3.0.2001312033-main

### Optimizing release branch

A branch which will be PGO’d requires a slightly different handling.  Let’s say release/3.0 forked
from main on commit 4abd4d54.  Initially, it will be configured to track main and 18b956f6 will
be optimized with 3.0.2001312033-main.  When the configuration is changed to start tracking
release/3.0 (change branch name $pgoBranch in perf/pgo/config.ps1 script), it will start tracking
its own branch.

E.g.

    18b956f6 -- if tracking main -> 3.0.2001312033-main,
                if tracking release/3.0 -> ERROR (no database exists)
    310bc133 -- 3.0.2001312054-release_3_0

### Optimizing topic branch

Assuming topic branch will not have a training run done, it can still use database from branch it
was forked from.  Let's say we have a branch which was forked from main on 4abd4d54.  If we don't
change which branch it's tracking, it will keep using 3.0.2001312033-main.  Merging main on
f23f1fad into topic branch, will change used database to 3.0.2001312205-main.