// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;
class CDependencyObject;
class ImageCopyParams;
class CREATEPARAMETERS;
struct IImageAvailableResponse;
struct ID2D1DeviceContext5;

class CSvgImageSource final
    : public CImageSource
{
public:

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** ppObject,
        _In_ CREATEPARAMETERS* pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSvgImageSource>::Index;
    }

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager* pTextureManager,
        _In_ CWindowRenderTarget* pRenderTarget,
        _In_ SurfaceCache* pSurfaceCache
        ) override;

    _Check_return_ HRESULT GetID2D1SvgDocument(
        _In_ ID2D1DeviceContext5* pD2DDeviceContextNoRef,
        _Out_ wrl::ComPtr<ID2D1SvgDocument>& d2dSvgDocument,
        _Outptr_ uint32_t* width,
        _Outptr_ uint32_t* height
        ) final;

    _Check_return_ HRESULT GetTitle(_Outptr_ HSTRING* output) final;
    _Check_return_ HRESULT GetDescription(_Outptr_ HSTRING* output) final;

    // Exposed metadata for codegen
    XFLOAT m_rasterizeWidth;
    XFLOAT m_rasterizeHeight;

protected:

    CSvgImageSource(_In_ CCoreServices* pCore);

    XUINT32 GetDecodeWidth() override;
    XUINT32 GetDecodeHeight() override;

    _Check_return_ HRESULT OnDownloadImageAvailableImpl(
        _In_ IImageAvailableResponse* pResponse
        ) override;

    _Check_return_ HRESULT PrepareDecode(bool retainPlaybackState) override;

    _Check_return_ HRESULT GetSvgDocumentString(
        _In_z_ const wchar_t* elementId,
        _Outptr_ HSTRING* output);

    static _Check_return_ HRESULT FindSvgElement(
        _In_ ID2D1SvgDocument* svgDocument,
        _In_z_ const wchar_t* elementId,
        _Out_ ctl::ComPtr<ID2D1SvgElement>& foundElement);
};
