@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

pushd Tests\UnitTests\UnitTestingBin
mstest /testcontainer:UnitTests.dll %~1
popd