// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include "FeatureFlags.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Commanding {

#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
    class CommandingContainerIntegrationTests : public WEX::TestClass<CommandingContainerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CommandingContainerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"d96ffe50-c538-42ef-918b-517ad455f1e0;65d77a94-832e-4d74-af02-4e3b3f5180cc;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateCommandingContainerProvidesCommandTarget)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandingContainer provides the correct command target when a button is clicked that is a peer to a target.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateCommandingContainerProvidesListCommandTarget)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandingContainer provides the correct list command target when a command is invoked on a list item.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateCommandingContainerTracksContext)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandingContainer tracks context-changed notifications and can respond to them.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()
    };
#endif
} } } } } }
