// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      Allows caller to draw to specified bits

class CWriteableBitmap final : public CBitmapSource
{
protected:
    CWriteableBitmap(_In_ CCoreServices *pCore)
        : CBitmapSource(pCore)
    {}

    ~CWriteableBitmap() override;

private:
    _Check_return_ HRESULT InitInstance() override;

public:
    DECLARE_CREATE(CWriteableBitmap);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CWriteableBitmap>::Index;
    }

    _Check_return_ HRESULT Create(
        _In_reads_((nWidth*nHeight)) void* pvPixels,
        _In_ XINT32 nWidth,
        _In_ XINT32 nHeight);

    _Check_return_ HRESULT Invalidate();

    _Check_return_ HRESULT CopyPixels(
        _In_ void* pvPixels);


    XINT32 GetPixelWidth() const override
    {
        IPALSurface *pSoftwareSurface = m_pImageSurfaceWrapper->GetSoftwareSurface();
        return pSoftwareSurface != NULL ? pSoftwareSurface->GetWidth() : m_nWidth;
    }

    XINT32 GetPixelHeight() const override
    {
        IPALSurface *pSoftwareSurface = m_pImageSurfaceWrapper->GetSoftwareSurface();
        return pSoftwareSurface != NULL ? pSoftwareSurface->GetHeight() : m_nHeight;
    }

    _Check_return_ HRESULT OnDownloadImageAvailableImpl(
        _In_ IImageAvailableResponse* pResponse
        ) override;

protected:

    // Preserve the software surfaces if it is a writeable bitmap
    // This is because CWriteableBitmap::CopyPixels has an IFCEXPECT for the software surface and relies on it
    // Refer to bugs in TFS: 577350, 505668, 342776
    bool ShouldKeepSoftwareSurfaces() override { return true; }

private:
    void Reset();
    _Check_return_ HRESULT InitializeSurface(__in_xcount(m_nLength) XINT32* pPixels);

    void InvalidateContent()
    {
        m_pImageSurfaceWrapper->SetUpdateRequired(true);
    }

    XINT32*     m_pPixels           = nullptr;
    XUINT32     m_nWidth            = 0;        // WriteableBitmap (Surface) width.
    XUINT32     m_nHeight           = 0;        // WriteableBitmap (Surface) height.
    XUINT32     m_nLength           = 0;        // m_nWidth * m_nHeight
};
