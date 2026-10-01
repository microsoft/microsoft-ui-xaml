// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

class DependentResourceUnitTests
{
public:
    BEGIN_TEST_CLASS(DependentResourceUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(SharedOwnerSurvivesUntilLastCleanup)
    TEST_METHOD(MoveTransfersTheWholePair)
    TEST_METHOD(ReentrantResetSeesEmptyState)
    TEST_METHOD(ReentrantMovePreservesReplacement)
    TEST_METHOD(GuardUnwindsBeforeOwner)
    TEST_METHOD(EmptyAdoptionDoesNotRetainOwner)
    TEST_METHOD(NullViewCanRetainItsLock)
};
