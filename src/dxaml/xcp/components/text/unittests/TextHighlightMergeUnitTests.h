// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Text {

        class TextHighlightMergeUnitTests : public WEX::TestClass<TextHighlightMergeUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(TextHighlightMergeUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(NonOverlapped)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that regions do not overlap.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(OverlapNextPartial)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region partially overlaps the next adjacent region.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(OverlapPrevPartial)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region partially overlaps the previous adjacent region.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(OverlapNextFull)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region fully overlaps the next adjacent region.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(OverlapEqualFull)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region start matches the adjacent region and fully overlaps.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(OverlapEqualPartial)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region start matches the adjacent region and partially overlaps.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(OverlapEqualMultiple)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region start matches the adjacent region and overlaps multiple.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(OverlapInner)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region overlaps entirely within another region.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(Overlap1)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region barely overlaps with with neighboring regions.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(OverlapMultiple)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the case that a region overlaps multiple regions.")
            END_TEST_METHOD()
        };
    }
} } } }
