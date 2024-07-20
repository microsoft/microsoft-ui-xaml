// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Windows specific text core services.

#pragma once

#include "DynamicArray.h"
#include <weakref_ptr.h>
#include <vector>

class SystemMemoryBits;
class SharedWicBitmap;
struct ID2D1Factory1;
struct ID2D1DeviceContext;
struct ID2D1SolidColorBrush;
struct IDWriteFactory2;
struct IDWriteFactory3;
struct IDWriteFactory4;
class DWriteFontFace;
class CUIElement;

//---------------------------------------------------------------------------
//
//  WinTextCore
//
//  Windows specific text core services.
//
//---------------------------------------------------------------------------
class WinTextCore : public CXcpObjectBase<IObject>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _Outptr_ WinTextCore **ppWinTextCore
        );

    _Check_return_ HRESULT GetD2D1Factory(
        _Outptr_ ID2D1Factory1 **ppD2DFactory
        );

    _Check_return_ HRESULT GetSharedD2D1DeviceContext(
        _Outptr_ ID2D1DeviceContext **ppSharedD2DDeviceContext
        );

    _Check_return_ HRESULT GetSharedD2DSolidColorBrush(
        _Outptr_ ID2D1SolidColorBrush **ppSharedD2DSolidColorBrush
        );

    _Check_return_ HRESULT GetAtlasDeviceContext(
        _In_ SystemMemoryBits *pTextureAtlas,
        _Outptr_ ID2D1DeviceContext **ppAtlasD2DDeviceContext
        );

    _Check_return_ HRESULT FlushTextRealizations();

    void ReleaseDeviceDependentResources();

    _Check_return_ HRESULT GetDWriteFactory(
        _COM_Outptr_ IDWriteFactory2 **ppDWriteFactory
        );

    _Check_return_ HRESULT GetDWriteFactory(
        _COM_Outptr_ IDWriteFactory3 **ppDWriteFactory
        );

    _Check_return_ HRESULT GetDWriteFactory(
        _COM_Outptr_ IDWriteFactory4 **ppDWriteFactory
        );

    void GetFontFaceId(
        _In_  DWriteFontFace *pFontFace,
        _Out_ XUINT32 *pFontId
        );

    _Check_return_ const DWriteFontFace* GetFontFaceNoAddRef (_In_ XUINT32 fontFaceId) const;

    _Check_return_ HRESULT DelayInvalidation(_In_ CUIElement *pTextBoxView);
    void ProcessInvalidation(_In_ CUIElement *pTextBoxViewOriginatingCall);

    void ApplyLogicalDpiSettings(
        _In_ ID2D1DeviceContext *pDeviceContext
        );
    HRESULT ConfigureNumberSubstitution();

    _Check_return_ HRESULT SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection);

private:
    // Private ctor.
    WinTextCore(_In_ CCoreServices *pCore);
    ~WinTextCore() override;

    _Check_return_ HRESULT EnsureSharedD2DResources();

    _Check_return_ HRESULT CreateSharedD2DResources();

    // Provides mapping between IPALSurface and corresponding ID2D1DeviceContext.
    struct AtlasDeviceContext
    {
        SystemMemoryBits *AtlasSystemMemory;
        SharedWicBitmap *WicBitmap;
        ID2D1DeviceContext *DeviceContext;
    };

    CCoreServices *m_pCore;
    ID2D1Factory1 *m_pD2DFactory;
    ID2D1DeviceContext *m_pSharedD2DDeviceContext;
    ID2D1SolidColorBrush *m_pSharedD2DSolidColorBrush;
    DynamicArray<AtlasDeviceContext> m_atlasToDeviceContextMap;
    std::vector<xref::weakref_ptr<CUIElement>> m_invalidTextBoxViews;

    // static table mapping IProvideFontInfo fontId <--> DWriteFontFace, shared by multiple text cores
    // !!! For thread safety, only read access is allowed through GetFontFaceNoAddRef()
    static std::vector<xref_ptr<DWriteFontFace>> *s_pFontFaceTable;
    // number of text cores alive
    static UINT32 s_coreCount;
    // static srw lock for thread safe accessing s_pFontFaceTable
    static wil::srwlock s_fontTableLock;
};
