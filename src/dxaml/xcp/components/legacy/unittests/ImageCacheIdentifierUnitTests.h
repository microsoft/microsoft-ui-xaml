// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"
#include <ImageCacheIdentifier.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml {

        class ImageCacheIdentifierUnitTests
        {
        public:
            BEGIN_TEST_CLASS(ImageCacheIdentifierUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

                        
            TEST_METHOD(ValidateSimpleUrl)
            TEST_METHOD(ValidateLongUrl)
            TEST_METHOD(ValidateInvalidationId)
            
        private:
        }; 
    }
}}}}
