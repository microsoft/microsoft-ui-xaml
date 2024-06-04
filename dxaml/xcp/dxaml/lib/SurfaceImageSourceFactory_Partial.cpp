// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SurfaceImageSource.g.h"
#include "DCompSurfaceFactoryManager.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

SurfaceImageSourceFactory::SurfaceImageSourceFactory()
{
}

SurfaceImageSourceFactory::~SurfaceImageSourceFactory()
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Flushes all GPU work for all SIS surfaces created with the given device.
//      This includes GPU work done by DComp on the app's behalf (gutters).
//
//-------------------------------------------------------------------------
HRESULT SurfaceImageSourceFactory::FlushAllSurfacesWithDevice(_In_ IUnknown *pDevice)
{
    RRETURN(DCompSurfaceFactoryManager::Instance()->FlushAllSurfaceFactoriesWithDevice(pDevice));
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      QI implementation
//
//-------------------------------------------------------------------------
HRESULT SurfaceImageSourceFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ISurfaceImageSourceManagerNative)))
    {
        *ppObject = static_cast<ISurfaceImageSourceManagerNative *>(this);
    }
    else
    {
        RRETURN(__super::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Creates a new SurfaceImageSource with the specified dimensions and opacity
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT SurfaceImageSourceFactoryGenerated::CreateInstanceWithDimensionsAndOpacityImpl(
    _In_ INT pixelWidth,
    _In_ INT pixelHeight,
    _In_ BOOLEAN isOpaque,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_imaging::ISurfaceImageSource** ppInstance)
{
    HRESULT hr = S_OK;
    ISurfaceImageSource* pInstance = NULL;
    IInspectable* pInner = NULL;
    SurfaceImageSource* pSurfaceImageSourceNoRef = NULL;

    IFCPTR(ppInstance);

    IFC(CheckActivationAllowed());

    if (pixelWidth < 0 || pixelHeight < 0)
    {
        IFC(E_INVALIDARG);
    }

    IFCCATASTROPHIC(pOuter == NULL || ppInner != NULL);

    // Create the SurfaceImageSource.
    IFC(__super::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    pSurfaceImageSourceNoRef = static_cast<SurfaceImageSource*>(pInstance);

    // Call into core for initialization.
    IFC(CoreImports::SurfaceImageSource_Initialize(
        static_cast<CSurfaceImageSource*>(pSurfaceImageSourceNoRef->GetHandle()),
        pixelWidth,
        pixelHeight,
        !!isOpaque));

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Creates a new VirtualSurfaceImageSource with the specified dimensions.   Assumed to
// contain transparent content
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT SurfaceImageSourceFactoryGenerated::CreateInstanceWithDimensionsImpl(
    _In_ INT pixelWidth,
    _In_ INT pixelHeight,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_imaging::ISurfaceImageSource** ppInstance)
{
    RRETURN(CreateInstanceWithDimensionsAndOpacity(pixelWidth, pixelHeight, FALSE, pOuter, ppInner, ppInstance));
}




