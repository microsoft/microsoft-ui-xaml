Local NuGet source for the `perf/tooling` projects.

Drop `.nupkg` files here to test them without pushing to a remote feed. This folder is
referenced by `perf/tooling/NuGet.config` as the `packagestore` source (path `packagestore`,
relative to that config), so it keeps working even when the `tooling` folder is copied or
zipped on its own.

This `readme.md` is a placeholder so the otherwise-empty folder is tracked by git (git does
not track empty directories). `.nupkg` files placed here are ignored by the repo's root
`.gitignore` (`*.nupkg`); use `git add -f` if you ever need to commit one.
