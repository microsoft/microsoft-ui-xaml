// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

#include <xvector.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Framework {

        class WinReaderUnitTests : public WEX::TestClass<WinReaderUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(WinReaderUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

                        
            TEST_METHOD(ValidateCreation)
            TEST_METHOD(ValidateReadBasicNodeTypes)
            TEST_METHOD(ValidateGetNamespaceUri)
            TEST_METHOD(ValidateGetLocalName)
        }; 
    }
}}}}
