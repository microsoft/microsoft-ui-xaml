# Branch structure

## Table of Contents

- [main](#main)
- [liftedixp-validation](#liftedixp-validation)

## main

The `main` branch is
self-descriptive. All changes ultimately end up in this branch.

## liftedixp-validation

The
`liftedixp_validation` branch
is used to validate changes made in the InteractiveExperiences packages. That code lives in the OS repo and is ingested
into Xaml via a reference to the `Microsoft.WinAppSDK.InteractiveExperiences.TransportPackage` package in
[eng/Version.Details.xml](../eng/Version.Details.xml).
This branch exists so that people making changes to those packages can validate that they're not breaking anything in
Xaml.

The contents of this branch will exactly mirror the main branch. The validation doesn't happen in this branch, but in
automated pull requests created by the dependency bot targeting this branch. Those pull requests come with automatic pipeline runs,
which contain the full suite of Xaml tests, which is where we get the validation. The PRs themselves are never
completed.

This branch exists because of a dependency update limitation - there can only be one dependency job for each branch. The `main`
branch's dependency job updates it with release IXP packages. We can't then have a separate job that updates it
with daily IXP packages as well. Thus we created a mirror of the `main` branch for this separate job.

Note that the historical daily validation branches can be found by looking up pull requests created by
the dependency bot targeting the `liftedixp_validation` branch.
Look under the "Active" pull requests for the most recent one.
