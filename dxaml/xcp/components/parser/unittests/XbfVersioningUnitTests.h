// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minerror.h>
#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XbfVersioningUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XbfVersioningUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(VerifyTargetOSVersionOperators)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the operators for TargetOsVersion struct.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetXBFVersion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates return value of GetXBFVersion()")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetDeferredElementSerializationVersion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates return value of GetDeferredElementSerializationVersion()")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetResourceDictionarySerializationVersion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates return value of GetResourceDictionarySerializationVersion()")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetStyleSerializationVersion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates return value of GetStyleSerializationVersion()")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetVisualStateGroupCollectionSerializationVersion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates return value of GetVisualStateGroupCollectionSerializationVersion()")
        END_TEST_METHOD()
    };

} } } } }