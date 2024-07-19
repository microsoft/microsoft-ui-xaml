// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;
class CDependencyObject;
class ImageCopyParams;
class CREATEPARAMETERS;

class CSoftwareBitmapSource final
    : public CImageSource
{
protected:
    CSoftwareBitmapSource(_In_ CCoreServices* pCore)
        : CImageSource(pCore)
    {}

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** ppObject,
        _In_ CREATEPARAMETERS* pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSoftwareBitmapSource>::Index;
    }

    _Check_return_ HRESULT Close();

    _Check_return_ HRESULT SetBitmap(
        _In_ wrl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap,
        _In_ ICoreAsyncAction* pAsyncAction
        );

    _Check_return_ HRESULT ReloadSource(bool forceCopyToSoftwareSurface);

    _Check_return_ HRESULT ReloadReleasedSoftwareImage() override;

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager* pTextureManager,
        _In_ CWindowRenderTarget* pRenderTarget,
        _In_ SurfaceCache* pSurfaceCache
        ) override;

private:

    _Check_return_ HRESULT CloseSoftwareBitmap();

    _Check_return_ HRESULT OnSoftwareBitmapImageAvailable(
        _In_ IImageAvailableResponse* pResponse
        );

    _Check_return_ HRESULT PrepareCopyParams(
        bool forceCopyToSoftwareSurface,
        xref_ptr<ImageCopyParams>& spImageCopyParams);

    wrl::ComPtr<wgri::ISoftwareBitmap> m_spSoftwareBitmap;
    bool m_closeOnCompletion = false;
};
