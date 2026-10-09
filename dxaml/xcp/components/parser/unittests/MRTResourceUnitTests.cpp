// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MRTResourceUnitTests.h"

#include <MRTResource.h>
#include <algorithm>
#include <array>
#include <xref_ptr.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    void MRTResourceUnitTests::EmbeddedDataMemoryOutlivesResource()
    {
        constexpr std::array<BYTE, 8> expectedBytes = { 0x58, 0x42, 0x46, 0x00, 0x12, 0x34, 0x56, 0x78 };

        xref_ptr<CMRTResource> resource;
        VERIFY_SUCCEEDED(CMRTResource::CreateEmbeddedDataForTest(
            expectedBytes.data(),
            static_cast<std::uint32_t>(expectedBytes.size()),
            resource.ReleaseAndGetAddressOf()));

        const BYTE* resourceBuffer = resource->GetEmbeddedDataBufferForTest();
        xref_ptr<IPALMemory> memory;
        VERIFY_SUCCEEDED(resource->Load(memory.ReleaseAndGetAddressOf()));

        VERIFY_ARE_EQUAL(expectedBytes.size(), static_cast<size_t>(memory->GetSize()));
        VERIFY_IS_TRUE(resourceBuffer != memory->GetAddress());

        resource.reset();

        VERIFY_IS_TRUE(std::equal(
            expectedBytes.begin(),
            expectedBytes.end(),
            static_cast<const BYTE*>(memory->GetAddress())));
    }

} } } } }
