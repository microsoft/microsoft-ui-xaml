// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>
#include "d2d1_2.h"
#include "CreateMockDetourBase.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    // D2D1Device Mock
    struct __declspec(uuid("640AC04A-6135-4D95-A137-310F3526C3BE")) IMockD2D1Device : public ID2D1Device
    {
        // Put Mock specific functions here
    };

    // D2D1Factory Mock
    struct __declspec(uuid("401D86E9-6DAE-400E-B508-BBDF7F3EB545")) IMockD2D1Factory : public ID2D1Factory1
    {
        // Put Mock specific functions here
    };

    HRESULT CreateMockD2D1Factory(_Out_ IMockD2D1Factory ** device);

    //  CreateD2D1FactoryDetour - RAII object to create a detour for the D2D1CreateFactory function so we can return a mock
    typedef HRESULT(WINAPI D2D1CREATEFACTORY)(
        _In_      D2D1_FACTORY_TYPE factoryType,
        _In_      REFIID riid,
        _In_opt_  const D2D1_FACTORY_OPTIONS *pFactoryOptions,
        _Out_     void **ppIFactory
        );

    class CreateD2D1FactoryDetour : public CreateMockDetourBase<typename D2D1CREATEFACTORY>
    {
    public:
        CreateD2D1FactoryDetour() : CreateMockDetourBase(L"d2d1.dll", "D2D1CreateFactory")
        {
            SetDetour([](D2D1_FACTORY_TYPE factoryType, REFIID riid, const D2D1_FACTORY_OPTIONS *pFactoryOptions, void **ppIFactory) -> HRESULT
            {
                Microsoft::WRL::ComPtr<IMockD2D1Factory> factory;
                IFC_RETURN(CreateMockD2D1Factory(&factory));
                return factory.CopyTo(riid, ppIFactory);
            });
        }
    };

} } } } }
