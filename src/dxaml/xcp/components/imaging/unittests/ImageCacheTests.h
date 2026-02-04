// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

class ImageCache;
class ImageDecodeParams;
class ImageTaskDispatcher;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class ImageCacheTests : public WEX::TestClass<ImageCacheTests>
        {
        public:
            BEGIN_TEST_CLASS(ImageCacheTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(DecodeOne)
                TEST_METHOD_PROPERTY(L"Description", L"Make sure a single decode request can complete")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodeTwo)
                TEST_METHOD_PROPERTY(L"Description", L"Issue two similar decodes")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodeTwoCancelFirst)
                TEST_METHOD_PROPERTY(L"Description", L"Issue two similar decodes then cancel the first and wait for the second one")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReleaseRequestInCallback)
                TEST_METHOD_PROPERTY(L"Description", L"Release the completed request from the callback handler")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RedecodeOtherRelease)
                TEST_METHOD_PROPERTY(L"Description", L"Repeat the second decode after the first request was released")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RedecodeOtherDisconnect)
                TEST_METHOD_PROPERTY(L"Description", L"Repeat the second decode after the first request was disconnected")
            END_TEST_METHOD()

        private:
            xref_ptr<ImageTaskDispatcher> m_dispatcher;
            xref_ptr<ImageDecodeParams> m_decodeParams16x16;
            xref_ptr<ImageCache> MakeImageCache();
        };
    }}
} } } }
