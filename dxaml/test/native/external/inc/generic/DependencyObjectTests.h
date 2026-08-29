// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    template<typename TClassUnderTest>
    class DependencyObjectTests
    {
    public:
        static void CanInstantiate()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&] ()
            {
                // Validate instantiation from code.
                auto obj = ref new TClassUnderTest();
                VERIFY_IS_NOT_NULL(obj);

                // Validate instantiation from Xaml.
                auto xamlSnippet = L"<" + GetClassName<TClassUnderTest>() + L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'/>";
                auto objFromParser = dynamic_cast<TClassUnderTest^>(xaml_markup::XamlReader::Load(xamlSnippet));
                VERIFY_IS_NOT_NULL(objFromParser);
            });
        }

    }; // class DependencyObjectTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
