// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef MEDIA_BASE_H
#define MEDIA_BASE_H

class CRectangle;
class CEventArgs;
class CXamlCompositionBrush;
class MediaCompositorTreeNode;

enum class PrimaryStretchDirection
{
    Unknown,
    Vertical,
    Horizontal
};


//------------------------------------------------------------------------
//
//  Class:  CMediaBase
//
//  Synopsis:
//      Base class for all media related tags ( image, mediaelement )
//
//------------------------------------------------------------------------
class CMediaBase : public CFrameworkElement
{
protected:
    CMediaBase(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
        , m_fRegisteredForUpdate(FALSE)
    {}

    ~CMediaBase() override;

public:
    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMediaBase>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peers of derived classes have state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT UpdateState() override;

    // CFrameWorkElement overrides
    _Check_return_ XFLOAT GetActualWidth() override;
    _Check_return_ XFLOAT GetActualHeight() override;

    // CMediaBase methods
    void FireMediaEvent(
            _In_ EventHandle hEvent,
            _Inout_opt_ CEventArgs **ppArgs = NULL);

    _Check_return_ CImageBrush *GetBackgroundBrush()
    {
        return m_pBackgroundBrush;
    }

    virtual _Check_return_ HRESULT EnsureBrush();

    _Check_return_ HRESULT GetVisibleImageSourceBounds(
        _In_ const XRECTF_RB *pWindowBounds,
        _Out_ XRECTF *pBounds
        );

    // Media elements do not inherit flow direction
    _Check_return_ HRESULT PullInheritedTextFormatting() final;

    bool IsRightToLeft() final;

    _Check_return_ HRESULT UpdateInternalSize(
        _Out_ XFLOAT &rActualWidth,
        _Out_ XFLOAT &rActualHeight,
        _Out_ DirectUI::AlignmentX &alignmentX,
        _Out_ DirectUI::AlignmentY &alignmentY);

protected:
    // MediaBase methods
    virtual _Check_return_ HRESULT EnsureMedia();
    _Check_return_ HRESULT EnsureGeometry();
    virtual void CloseMedia();

    _Check_return_ HRESULT ComputeScaleFactor(
        _In_ XSIZEF *pRenderBounds,
        _Inout_ XSIZEF& scaleFactor,
        _In_ XFLOAT naturalWidth,
        _In_ XFLOAT naturalHeight,
        _In_ PrimaryStretchDirection preferPrimaryStretchDirection,
        _Out_opt_ PrimaryStretchDirection* actualStretchDirection);

    virtual _Check_return_ HRESULT GetNaturalBounds(_Inout_ XRECTF& pNaturalBounds);
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, _Inout_ XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, _Inout_ XSIZEF& newFinalSize) override;

    virtual bool HasValidMediaSource();
    virtual bool ShouldCreateBackgroundBrush();

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT MeasureArrangeHelper(
        _In_ XSIZEF availableSize,
        _Inout_ XSIZEF &desiredSize,
        _In_ PrimaryStretchDirection preferPrimaryStretchDirection,
        _Out_opt_ PrimaryStretchDirection* actualStretchDirection);

private:
    _Check_return_ HRESULT RegisterForUpdate();
    _Check_return_ HRESULT UnregisterForUpdate();

public:
// CMediaBase fields

    xstring_ptr             m_strSource;
    DirectUI::Stretch       m_Stretch                       = DirectUI::Stretch::Uniform;

protected:
    CImageBrush*            m_pBackgroundBrush              = nullptr;
    bool                    m_containerRequiresMeasure      = false;

private:
    CRectangle*             m_pBackgroundRect               = nullptr;

    // Actual width & height.
    XFLOAT                  m_eActualWidth                  = static_cast<XFLOAT>(XDOUBLE_NAN);
    XFLOAT                  m_eActualHeight                 = static_cast<XFLOAT>(XDOUBLE_NAN);

    bool                   m_fRegisteredForUpdate : 1;

    PrimaryStretchDirection m_lastMeasureStretchDirection = PrimaryStretchDirection::Unknown;

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) final;
};

#endif
