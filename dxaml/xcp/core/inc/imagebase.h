// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef __IMAGE_BASE_H__
#define __IMAGE_BASE_H__

class CRectangle;
class CImageBrush;
class CImageBrushReportCallback;
class CBitmapImage;
class CImageSource;

//------------------------------------------------------------------------
//
//  Class:  CImageBase
//
//  Synopsis:
//      Base class for CImage
//
//------------------------------------------------------------------------
class CImageBase : public CMediaBase
{
protected:
    CImageBase(_In_ CCoreServices *pCore)
        : CMediaBase(pCore)
        , m_pImageReportCallback(NULL)
        , m_pImageSource(NULL)
        , m_ninegridPrivate()
    {
    }

   ~CImageBase() override;

public:
    bool HasNinegrid() const
    {
        // If we have any ninegrid margin, it should affect rendering.
        return m_ninegridPrivate.left > 0.0f
            || m_ninegridPrivate.right > 0.0f
            || m_ninegridPrivate.top > 0.0f
            || m_ninegridPrivate.bottom > 0.0f;
    }

    XTHICKNESS GetNinegrid() const
    {
        ASSERT(HasNinegrid());

        // Negative values are clamped to zero.
        XTHICKNESS clampedNinegrid;
        clampedNinegrid.left = MAX(m_ninegridPrivate.left, 0.0f);
        clampedNinegrid.right = MAX(m_ninegridPrivate.right, 0.0f);
        clampedNinegrid.top = MAX(m_ninegridPrivate.top, 0.0f);
        clampedNinegrid.bottom = MAX(m_ninegridPrivate.bottom, 0.0f);

        return clampedNinegrid;
    }

    // CDependencyObject overrides
    _Check_return_ HRESULT InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner) override;

    // CMediaBase Overrides
    bool HasValidMediaSource( ) final;
    _Check_return_ HRESULT EnsureBrush() override;

    // IImageReportCallback
    virtual _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode);
    _Check_return_ HRESULT FireDownloadProgress(_In_ XFLOAT idownloadProgress);
    virtual _Check_return_ HRESULT FireImageOpened();

protected:
    void CloseMedia() final;
    _Check_return_ HRESULT GetNaturalBounds(_Inout_ XRECTF& pNaturalBounds) final;
    CAutomationPeer* OnCreateAutomationPeerImpl() final;
    _Check_return_ HRESULT UpdateMeasure();
    CImageBrushReportCallback *m_pImageReportCallback;

public:
    CImageSource *m_pImageSource;
    XTHICKNESS m_ninegridPrivate;
};

class CImageBrushReportCallback final : public IImageReportCallback
{
public:
    static HRESULT Create(_In_ CImageBase* pImage, _Out_ CImageBrushReportCallback** ppCallback);

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode) override;
    _Check_return_ HRESULT FireDownloadProgress(_In_ XFLOAT iProgress) override;
    _Check_return_ HRESULT FireImageOpened() override;

    void Disable() override;

private:
    friend xref_ptr<CImageBrushReportCallback> make_xref<CImageBrushReportCallback>();
    CImageBrushReportCallback() = default;
    ~CImageBrushReportCallback() = default;

    CImageBase  *m_pImage   = nullptr;
    XUINT32      m_cRef     = 1;
};
#endif // #ifndef __IMAGE_BASE_H__
