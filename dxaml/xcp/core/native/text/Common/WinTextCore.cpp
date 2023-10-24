// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <algorithm>
#include "WinTextCore.h"
#include "TextHelpers.h"
#include "SharedWicBitmap.h"
#include "PALFontAndScriptServices.h"
#include "DWriteTypes.h"
#include "DWriteFontAndScriptServices.h"
#include "RootScale.h"
#include <FxCallbacks.h>

// Static table, lock and WinTextCore count variables, provide shared read-only access to
// mapped DWrite font faces for multiple cores of the same process
std::vector<xref_ptr<DWriteFontFace>> *WinTextCore::s_pFontFaceTable = nullptr;
UINT32 WinTextCore::s_coreCount = 0;
wil::srwlock WinTextCore::s_fontTableLock;

template class DynamicArray<WinTextCore::AtlasDeviceContext>;

typedef HRESULT (WINAPI *D2D1CreateFactoryFunc)(
    _In_ D2D1_FACTORY_TYPE factoryType,
    _In_ REFIID riid,
    _In_opt_ CONST D2D1_FACTORY_OPTIONS *pFactoryOptions,
    _Out_ void **ppIFactory
    );

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the WinTextCore class.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::Create(
    _In_ CCoreServices *pCore,
    _Outptr_ WinTextCore **ppWinTextCore
    )
{
    HRESULT hr = S_OK;
    WinTextCore *pWinTextCore = NULL;

    pWinTextCore = new WinTextCore(pCore);

    {
        // acquire exclusive lock during creation to allocate font table if s_pFontFaceTable is currently null
        auto exclusiveLock = s_fontTableLock.lock_exclusive();

        // Alloacte s_pFontFaceTable if this is the first created WinTextCore
        if (pWinTextCore->s_pFontFaceTable == nullptr)
        {
            ASSERT(s_coreCount == 0);
            pWinTextCore->s_pFontFaceTable = new std::vector<xref_ptr<DWriteFontFace>>();
            // Insert a dummy entry to avoid inserting font faces at position 0
            // we pass a font face id to RichEdit which corresponds to the entry index
            // in our vector. However, fontfaceId = 0 is special and means that font
            // resolution failed.
            pWinTextCore->s_pFontFaceTable->emplace_back(nullptr);
        }
        s_coreCount++;
    }

    *ppWinTextCore = pWinTextCore;
    RRETURN(hr);//RRETURN_REMOVAL
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the WinTextCore class.
//
//---------------------------------------------------------------------------
WinTextCore::WinTextCore(_In_ CCoreServices *pCore)
    : m_pCore(pCore)
    , m_pD2DFactory(NULL)
    , m_pSharedD2DDeviceContext(NULL)
    , m_pSharedD2DSolidColorBrush(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the WinTextCore.
//
//---------------------------------------------------------------------------
WinTextCore::~WinTextCore()
{
    ReleaseInterface(m_pD2DFactory);
    ReleaseInterface(m_pSharedD2DDeviceContext);
    ReleaseInterface(m_pSharedD2DSolidColorBrush);

    // release the font face table if this is the last core
    {
        auto exclusiveLock = s_fontTableLock.lock_exclusive();
        s_coreCount--;
        if (s_coreCount == 0)
        {
            delete s_pFontFaceTable;
            s_pFontFaceTable = nullptr;
        }
    }
}

//---------------------------------------------------------------------------
//
//  Gets cached instance of ID2D1Factory1.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::GetD2D1Factory(
    _Outptr_ ID2D1Factory1 **ppD2DFactory
    )
{
    IFC_RETURN(EnsureSharedD2DResources());
    ASSERT(m_pD2DFactory != NULL);

    *ppD2DFactory = m_pD2DFactory;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Gets cached instance of shared ID2D1DeviceContext.
//  NOTE: this ID2D1DeviceContext is not suitable for drawing.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::GetSharedD2D1DeviceContext(
    _Outptr_ ID2D1DeviceContext **ppSharedD2DDeviceContext
    )
{
    IFC_RETURN(EnsureSharedD2DResources());
    ASSERT(m_pSharedD2DDeviceContext != NULL);

    // Apply DPI settings to ID2D1DeviceContext in order to benefit from D2D rendering optimizations.
    ApplyLogicalDpiSettings(m_pSharedD2DDeviceContext);

    *ppSharedD2DDeviceContext = m_pSharedD2DDeviceContext;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Gets cached instance of shared ID2D1SolidColorBrush.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::GetSharedD2DSolidColorBrush(
    _Outptr_ ID2D1SolidColorBrush **ppSharedD2DSolidColorBrush
    )
{
    IFC_RETURN(EnsureSharedD2DResources());
    ASSERT(m_pSharedD2DSolidColorBrush != NULL);

    *ppSharedD2DSolidColorBrush = m_pSharedD2DSolidColorBrush;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Gets ID2D1DeviceContext associated with the given atlas.
//  If DeviceContext does not exist, it gets created based on the entire atlas.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::GetAtlasDeviceContext(
    _In_ SystemMemoryBits *pAtlasSystemMemory,
    _Outptr_ ID2D1DeviceContext **ppAtlasD2DDeviceContext
    )
{
    HRESULT hr = S_OK;
    ID2D1DeviceContext *pAtlasD2DDeviceContext = NULL;
    ID2D1RenderTarget *pRenderTarget = NULL;
    SharedWicBitmap *pWicBitmap = NULL;

    // Find existing DeviceContext for specified atlas.
    for (XUINT32 i = 0, collectionSize = m_atlasToDeviceContextMap.GetCount(); i < collectionSize; i++)
    {
        if (m_atlasToDeviceContextMap[i].AtlasSystemMemory == pAtlasSystemMemory)
        {
            pAtlasD2DDeviceContext = m_atlasToDeviceContextMap[i].DeviceContext;
            break;
        }
    }

    // If DeviceContext is not created yet, create one based on the entire atlas.
    if (pAtlasD2DDeviceContext == NULL)
    {
        XUINT32 atlasIndex;
        XUINT32 textureWidth;
        XUINT32 textureHeight;
        XUINT32 textureStride;
        XUINT8 *pTextureBuffer;
        D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = GetCommonD2DRenderTargetProperties();

        pAtlasSystemMemory->Lock(
            reinterpret_cast<void **>(&pTextureBuffer),
            reinterpret_cast<XINT32*>(&textureStride),
            &textureWidth,
            &textureHeight);

        IFC(SharedWicBitmap::Create(
            textureWidth,
            textureHeight,
            textureStride,
            GUID_WICPixelFormat8bppAlpha,
            pTextureBuffer,
            &pWicBitmap));

        IFC(m_pD2DFactory->CreateWicBitmapRenderTarget(
            pWicBitmap,
            &renderTargetProperties,
            &pRenderTarget));

        IFC(pRenderTarget->QueryInterface(
            __uuidof(ID2D1DeviceContext),
            reinterpret_cast<void **>(&pAtlasD2DDeviceContext)));

        atlasIndex = m_atlasToDeviceContextMap.GetCount();
        IFC(m_atlasToDeviceContextMap.SetCount(atlasIndex + 1));
        m_atlasToDeviceContextMap[atlasIndex].AtlasSystemMemory = pAtlasSystemMemory;
        m_atlasToDeviceContextMap[atlasIndex].WicBitmap = pWicBitmap;
        m_atlasToDeviceContextMap[atlasIndex].DeviceContext = pAtlasD2DDeviceContext;
        AddRefInterface(pAtlasSystemMemory);
        pWicBitmap = NULL;

        // Apply DPI settings to ID2D1DeviceContext in order to benefit from D2D rendering optimizations.
        ApplyLogicalDpiSettings(pAtlasD2DDeviceContext);

        pAtlasD2DDeviceContext->BeginDraw();
    }

    *ppAtlasD2DDeviceContext = pAtlasD2DDeviceContext;
    pAtlasD2DDeviceContext = NULL;

Cleanup:
    ReleaseInterface(pWicBitmap);
    ReleaseInterface(pRenderTarget);
    ReleaseInterface(pAtlasD2DDeviceContext);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Flush pending text realizations.
//  For each active D2DDeviceContext can EndDraw and cleanup allocated resources.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::FlushTextRealizations()
{
    HRESULT hr = S_OK;
    bool fFireEvents = false;

    if (m_atlasToDeviceContextMap.GetCount() > 0)
    {
        TraceUIThreadTextRasterizeWaitBegin();
        fFireEvents = TRUE;
    }

    for (XUINT32 i = 0, collectionSize = m_atlasToDeviceContextMap.GetCount(); i < collectionSize; i++)
    {
        AtlasDeviceContext *pAtlasDeviceContext = &m_atlasToDeviceContextMap[i];
        if (pAtlasDeviceContext->AtlasSystemMemory != NULL)
        {
            ASSERT(pAtlasDeviceContext->DeviceContext != NULL);
            ASSERT(pAtlasDeviceContext->WicBitmap != NULL);

            IFC(pAtlasDeviceContext->DeviceContext->EndDraw());
            pAtlasDeviceContext->AtlasSystemMemory->Unlock();

            ReleaseInterface(pAtlasDeviceContext->DeviceContext);
            ReleaseInterface(pAtlasDeviceContext->WicBitmap);
            ReleaseInterface(pAtlasDeviceContext->AtlasSystemMemory);
        }
    }

    m_atlasToDeviceContextMap.Clear();

Cleanup:
    if (fFireEvents)
    {
        TraceUIThreadTextRasterizeWaitEnd();
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Create the shared resources on the text core. Called from
//      initialization and from the render walk after device lost.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::CreateSharedD2DResources()
{
    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = GetCommonD2DRenderTargetProperties();

    ASSERT(m_pSharedD2DDeviceContext == NULL);
    ASSERT(m_pSharedD2DSolidColorBrush == NULL);

    //
    // Create shared ID2D1DeviceContext. This device context is not suitable for drawing,
    // since it is based on "empty" IWICBitmap.
    //
    xref_ptr<SharedWicBitmap> pSharedWicBitmap;
    IFC_RETURN(SharedWicBitmap::Create(1, 1, 4, GUID_WICPixelFormat8bppAlpha, nullptr, pSharedWicBitmap.ReleaseAndGetAddressOf()));

    wrl::ComPtr<ID2D1RenderTarget> pRenderTarget;
    IFC_RETURN(m_pD2DFactory->CreateWicBitmapRenderTarget(
        pSharedWicBitmap,
        &renderTargetProperties,
        &pRenderTarget));

    IFC_RETURN(pRenderTarget.CopyTo(&m_pSharedD2DDeviceContext));

    //
    // Create shared ID2D1SolidColorBrush.
    //
    IFC_RETURN(pRenderTarget->CreateSolidColorBrush(
        &D2D1::ColorF(D2D1::ColorF::Black),
        &D2D1::BrushProperties(),
        &m_pSharedD2DSolidColorBrush
        ));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases device-dependent D2D resources for device lost scenario.
//
//---------------------------------------------------------------------------
void
WinTextCore::ReleaseDeviceDependentResources()
{
    ReleaseInterface(m_pSharedD2DDeviceContext);
    ReleaseInterface(m_pSharedD2DSolidColorBrush);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Performs actual initialization work to load D2D, create D2D Factory,
//      and shared D2D resources.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
WinTextCore::EnsureSharedD2DResources()
{
    HRESULT hr = S_OK;

    //
    // Load d2d1.dll and create D2D1Factory once for lifetime of app
    //
    static HMODULE hD2DModule = NULL;
    if (hD2DModule == NULL)
    {
        // TODO: TEXT: Incorrect error checking, should call GetLastError. This shouldn't be loaded directly from core code, it should be in the PAL.
        IFCPTR(hD2DModule = LoadLibraryEx(L"d2d1.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
    }

    if (m_pD2DFactory == NULL)
    {
        D2D1CreateFactoryFunc pCreateFactoryFunc = NULL;
        D2D1_FACTORY_OPTIONS factoryOptions = { D2D1_DEBUG_LEVEL_NONE };

        pCreateFactoryFunc = reinterpret_cast<D2D1CreateFactoryFunc>(GetProcAddress(hD2DModule, "D2D1CreateFactory"));
        IFCPTR(pCreateFactoryFunc);

        IFC(pCreateFactoryFunc(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory1),
            &factoryOptions,
            reinterpret_cast<void **>(&m_pD2DFactory)));
    }

    // If the device dependent resources don't exist, allocate them.
    if (m_pSharedD2DDeviceContext == NULL)
    {
        ASSERT(m_pSharedD2DSolidColorBrush == NULL);

        IFC(CreateSharedD2DResources());
    }

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(m_pD2DFactory);
        FreeLibrary(hD2DModule);
        hD2DModule = NULL;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     Gets instance of IDWriteFactory2 buried in the PAL.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT WinTextCore::GetDWriteFactory(
    _COM_Outptr_ IDWriteFactory2 **ppDWriteFactory
    )
{
    // Just upcast since IDWriteFactory4 inherits from IDWriteFactory2.
    return GetDWriteFactory(reinterpret_cast<IDWriteFactory4**>(ppDWriteFactory));
}

_Check_return_ HRESULT WinTextCore::GetDWriteFactory(
    _COM_Outptr_ IDWriteFactory3 **ppDWriteFactory
    )
{
    // Just upcast since IDWriteFactory4 inherits from IDWriteFactory3.
    return GetDWriteFactory(reinterpret_cast<IDWriteFactory4**>(ppDWriteFactory));
}

_Check_return_ HRESULT WinTextCore::GetDWriteFactory(
    _COM_Outptr_ IDWriteFactory4 **ppDWriteFactory
    )
{
    CTextCore *pTextCore = nullptr;
    IFontAndScriptServices *pFontServices = nullptr;
    DWriteFontAndScriptServices *pDWriteFontServices = nullptr;

    IFC_RETURN(m_pCore->GetTextCore(&pTextCore));

    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontServices));

    pDWriteFontServices =
        static_cast<DWriteFontAndScriptServices*>(
            static_cast<PALFontAndScriptServices*>(pFontServices)->GetPALFontAndScriptServices());

    IFC_RETURN(pDWriteFontServices->GetDWriteFactory(ppDWriteFactory));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a unique identifier (an index into the array of RichEdit referenced
//      font families) matching a DWriteFontFace.  If the DWriteFontFace
//      is not already referenced, a new reference is created.
//
//---------------------------------------------------------------------------
void WinTextCore::GetFontFaceId(
    _In_  DWriteFontFace *pFontFace,
    _Out_ XUINT32 *pFontId
    )
{
    {
        // For majority cases, only read lock is needed
        auto sharedLock = s_fontTableLock.lock_shared();
        auto pos = std::find_if(s_pFontFaceTable->begin() + 1, s_pFontFaceTable->end(),
            [=](DWriteFontFace* pItem) {
            return pItem && pFontFace->Equals(pItem);
        });

        // return if the table has the font face entry already
        if (pos != s_pFontFaceTable->end())
        {
            *pFontId = static_cast<XUINT32>(pos - s_pFontFaceTable->begin());
            return;
        }
    }

    // Acquire exclusive/write lock only for inserting the new font face into the table
    auto exclusiveLock = s_fontTableLock.lock_exclusive();
    s_pFontFaceTable->emplace_back(pFontFace);
    *pFontId = static_cast<XUINT32>(s_pFontFaceTable->size()) - 1;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Given a unique identifier (an index into the array of RichEdit referenced
//      font families) returns the cached DWriteFontFace
//      DWriteFOntFace* elements in s_pFontFaceTable are readonly once in the table,
//      const DWriteFOntFace* access is required for thread safety
//---------------------------------------------------------------------------
_Check_return_ const DWriteFontFace* WinTextCore::GetFontFaceNoAddRef (_In_ XUINT32 fontFaceId) const
{
    // Acquire shared lock for read access
    auto lock = s_fontTableLock.lock_shared();
    return (s_pFontFaceTable->at(fontFaceId));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Applies DPI settings to ID2D1DeviceContext in order to benefit from
//      D2D rendering optimizations.
//
//---------------------------------------------------------------------------
void
WinTextCore::ApplyLogicalDpiSettings(
    _In_ ID2D1DeviceContext *pDeviceContext
    )
{
    // Bug 19572630: CTextCore needs to be ContentRoot aware
    const auto contentRootCoordinator = m_pCore->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot();
    const float plateauScale = RootScale::GetRasterizationScaleForContentRoot(root);
    const float dpi = plateauScale * 96.0f;

    // D2D has an optimization that can cause the text rendering mode to default to a simpler mode when DPI is high.
    // Since the DPI scaling is already present in the rendering transform, we need to tell D2D to not
    // affect actual size of text by calling SetUnitMode(D2D1_UNIT_MODE_PIXELS).
    pDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    pDeviceContext->SetDpi(dpi, dpi);
}

//------------------------------------------------------------------------
//
//  In some cases RichEdit will cause TextBoxView invalidation in the
//  ArrangeOverride call. This will cause the layout manager to exceed the
//  max iterations allowed.
//  So to workaround this issue we delay the view invalidation by
//  issuing a dispatcher call. In this method we group such TextBoxViews
//  to be batch processed instead of issuing a dispatcher call per
//  TextBoxView.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT WinTextCore::DelayInvalidation(_In_ CUIElement *pTextBoxView)
{
    if (m_invalidTextBoxViews.empty())
    {
        // protect against this elements being deleted.
        AddRefInterface(pTextBoxView);
        IFC_RETURN(FxCallbacks::TextBoxView_InvalidateView(pTextBoxView));
    }

    m_invalidTextBoxViews.emplace_back(pTextBoxView);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Invalidates the TextBoxViews queued for delayed processing.
//
//------------------------------------------------------------------------
void WinTextCore::ProcessInvalidation(_In_ CUIElement *pTextBoxViewOriginatingCall)
{
    for (auto& wr : m_invalidTextBoxViews)
    {
        auto pTextBoxView = wr.lock();
        if (pTextBoxView)
        {
            pTextBoxView->InvalidateMeasure();
        }
    }
    m_invalidTextBoxViews.clear();

    // Before posting the dispatcher call we AddRefed this element
    // to guard against it being deleted until we receive the dispatcher callback.
    // Now that we processed the callback we need to Release() this TextView.
    ReleaseInterface(pTextBoxViewOriginatingCall);
}

//------------------------------------------------------------------------
//
//  Update the shared IDWriteNumberSubstitution object stored in DWriteFontAndScriptServices
//
//------------------------------------------------------------------------
HRESULT WinTextCore::ConfigureNumberSubstitution()
{
    CTextCore *pTextCore = NULL;
    IFontAndScriptServices *pFontServices = NULL;
    DWriteFontAndScriptServices *pDWriteFontServices = NULL;

    IFC_RETURN(m_pCore->GetTextCore(&pTextCore));

    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontServices));

    pDWriteFontServices =
        static_cast<DWriteFontAndScriptServices*>(
            static_cast<PALFontAndScriptServices*>(pFontServices)->GetPALFontAndScriptServices());

    pDWriteFontServices->ClearNumberSubstitutionList();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Sets a custom font collection for core text services to use in testing.
//
//------------------------------------------------------------------------
HRESULT WinTextCore::SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection)
{
    CTextCore *pTextCore = nullptr;
    IFontAndScriptServices *pFontServices = nullptr;
    PALFontAndScriptServices* pPalFontServices = nullptr;
    DWriteFontAndScriptServices *pDWriteFontServices = nullptr;

    IFC_RETURN(m_pCore->GetTextCore(&pTextCore));
    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontServices));

    pPalFontServices = static_cast<PALFontAndScriptServices*>(pFontServices);
    pDWriteFontServices = static_cast<DWriteFontAndScriptServices*>(pPalFontServices->GetPALFontAndScriptServices());

    IFC_RETURN(pDWriteFontServices->SetSystemFontCollectionOverride(pFontCollection));
    pPalFontServices->ResetSystemFontCollection();

    return S_OK;
}


