// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "dxgi1_2.h"
namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    class CreateDXGIFactoryDetour;
    interface IMockDXGIAdapter;

    class D3D11DeviceInstanceUnitTests : public WEX::TestClass<D3D11DeviceInstanceUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(D3D11DeviceInstanceUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_METHOD(Creation)
        TEST_METHOD(EnsureDXGIAdapters)
        TEST_METHOD(CheckForStaleD3DDevice)
        TEST_METHOD(EnsureResources)
        TEST_METHOD(InitializeVideoDevice)
        TEST_METHOD(EnsureD2DResources)

    private:
        void CheckForStaleDeviceSubTest(_In_ CreateDXGIFactoryDetour & DXGIMock,
                                        _In_ IMockDXGIAdapter * adapter1,
                                        _In_ IMockDXGIAdapter * adapter2,
                                        _In_ IMockDXGIAdapter * newAdapter1,
                                        _In_ IMockDXGIAdapter * newAdapter2,
                                        bool deviceShouldBeStale);
        static LUID GetAdapterLuid(IDXGIAdapter* pAdapter);
    };

} } } } }
