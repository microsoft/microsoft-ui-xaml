# Description

We generate PGO database NuGet package which is versioned based on product release version and branch name/time stamp of the code that was used for instrumentation and training.  In CI/release builds an initialization step enumerates all available versions, filters out those for other releases and branches.  Given a list of applicable versions, it will find the one that is closest (BEFORE) the time-stamp of the last commit or a fork-point from instrumented branch.  That package version will be installed and version references will be updated.  The PGO branch is determined by variable $pgoBranch in tools/MUXPGODatabase/config.ps1.  It will need to be updated if a forked branch should be PGO'd.

## Scenarios

For the purpose of illustration, let’s assume the following is a chronological list of check-ins to two branches (master and release/2.4).  Some of them have had instrumentation/training run done on them and have generated PGO NuGets (version numbers in parentheses).  To simplify, let’s assume that release major and minor versions are the same for all check-in as they merely act as filters for what versions are considered to be available.

    1b27fd5f -- master      --
    7b303f74 -- master      --
    930ff585 -- master      -- 2.4.2001312227-master
    63948a75 -- master      --
    0d379b51 -- master      --
    f23f1fad -- master      -- 2.4.2001312205-master
    bcf9adaa -- master      --
    6ef44a23 -- master      --
    310bc133 -- release/2.4 --
    80a4ab55 -- release/2.4 -- 2.4.2001312054-release_2_4
    18b956f6 -- release/2.4 --
    4abd4d54 -- master      -- 2.4.2001312033-master
    d150eae0 -- master      -- 2.4.2001312028-master

### Optimizing on PGO’d branch

If we are building on master (which in this example is PGO’d), the version picked will be the one that has the same major and minor versions AND branch name and is the same or is right before the SHA being built.

E.g.

    1b27fd5f -- 2.4.2001312227-master
    f23f1fad -- 2.4.2001312205-master
    bcf9adaa -- 2.4.2001312033-master

### Optimizing release branch

A branch which will be PGO’d requires a slightly different handling.  Let’s say release/2.4 forked from master on commit 4abd4d54.  Initially, it will be configured to track master and 18b956f6 will be optimized with 2.4.2001312033-master.  When the configuration is changed to start tracking release/2.4 (change branch name $pgoBranch in tools/MUXPGODatabase/config.ps1 script), it will start tracking its own branch.

E.g.

    18b956f6 -- if tracking master -> 2.4.2001312033-master,
                if tracking release/2.4 -> ERROR (no database exists)
    310bc133 -- 2.4.2001312054-release_2_4

### Optimizing topic branch

Assuming topic branch will not have a training run done, it can still use database from branch it was forked from.  Let’s say we have a branch which was forked from master on 4abd4d54.  If we don’t change which branch it’s tracking, it will keep using 2.4.2001312033-master.  Merging master on f23f1fad into topic branch, will change used database to 2.4.2001312205-master.
