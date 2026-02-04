// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>
#include <IParserCoreServices.h>
#include <WexTestClass.h>

class XamlSchemaContext;
struct IErrorService;
class XamlNodeStreamCacheManager;
class ParserErrorReporter;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlNodeStreamCacheManagerUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlNodeStreamCacheManagerUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(Create)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetBinaryResourceForMsAppXUri)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetBinaryResourceForMsResourceUri)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetBinaryResourceForUnsupportedUri)
        END_TEST_METHOD()

    private:
        void GetBinaryResourceForSupportedUri(const xstring_ptr& uri, const xstring_ptr& physicalUri);
    };

} } } } }

