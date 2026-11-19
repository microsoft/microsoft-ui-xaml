// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    class SelectionTests
    {
        BEGIN_TEST_CLASS(SelectionTests)
        END_TEST_CLASS()

        TEST_METHOD(ParsesEveryModeName);
        TEST_METHOD(ParsingIsCaseInsensitive);
        TEST_METHOD(RejectsUnknownOrEmptyText);
        TEST_METHOD(ModeNamesRoundTrip);
        TEST_METHOD(UnsetRequestSelectsComposition);
        TEST_METHOD(InvalidRequestFallsBackToComposition);
        TEST_METHOD(UnavailableModeFallsBackWithReason);
        TEST_METHOD(AvailableModeIsHonored);
        TEST_METHOD(CompositionIsAlwaysAvailable);
        TEST_METHOD(ReadsEnvironmentVariable);
        TEST_METHOD(ProviderIdentityIsStable);
    };
}
