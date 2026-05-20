// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

#include <xvector.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml {

        class XVectorUnitTests : public WEX::TestClass<XVectorUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(XVectorUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

                        
            TEST_METHOD(ValidateAdditionIteratorOperator)
            TEST_METHOD(ValidateSubtractionIteratorOperatorWithIterator)
            TEST_METHOD(ValidateSubtractionIteratorOperatorWithOffset)

            TEST_METHOD(ValidateConstAdditionIteratorOperator)
            TEST_METHOD(ValidateConstSubtractionIteratorOperatorWithIterator)
            TEST_METHOD(ValidateConstSubtractionIteratorOperatorWithOffset)

            TEST_METHOD(ValidateStdDistance)
            
        private:
            // xvector doesn't implement copy or move constructors, so we return
            // a static reference instead.
            xvector<int>& BuildTestVector();
        }; 
    }
}}}}
