// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

class ImageCache;
class ImageDecodeParams;
class ImageTaskDispatcher;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class ImageMetadataTests : public WEX::TestClass<ImageMetadataTests>
        {
        public:
            BEGIN_TEST_CLASS(ImageMetadataTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(GetMetadataViewNoData)
                TEST_METHOD_PROPERTY(L"Description", L"Get an empty metadata view")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetMetadataAfterDownload)
                TEST_METHOD_PROPERTY(L"Description", L"Get metadata after stream is available")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DownloadAddsMetadata)
                TEST_METHOD_PROPERTY(L"Description", L"When stream is available metadata should become available in the existing view")
            END_TEST_METHOD()

        private:
            xref_ptr<ImageTaskDispatcher> m_dispatcher;
            xref_ptr<ImageDecodeParams> m_decodeParams16x16;
            xref_ptr<ImageCache> MakeImageCache();            
        };

    }}
} } } }
