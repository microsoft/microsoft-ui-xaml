// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace OfferableHeap {

        class OfferableMemoryUnitTests : public WEX::TestClass<OfferableMemoryUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(OfferableMemoryUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(MultipleAllocationsWrite)
                TEST_METHOD_PROPERTY(L"Description", L"Tests allocating multiple items from the offerable heap and that no crashes occur when filled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(OfferReclaim)
                TEST_METHOD_PROPERTY(L"Description", L"Tests offering up memory and reclaiming it immediately.")
            END_TEST_METHOD()
        
        private:
            NTSTATUS WriteOnEachPage(_In_ LPVOID BaseAddress, _In_ ULONG NumberOfPages, _In_ ULONG PageSize, _In_ ULONG Value);
            bool CheckValueOnEachPage(_In_ LPVOID BaseAddress, _In_ ULONG NumberOfPages, _In_ ULONG PageSize, _In_ ULONG ExpectedValue);
        };
    }}
} } } }
