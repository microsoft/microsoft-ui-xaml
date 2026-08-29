// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class TextSelectionManager;
class CPopup;

class IXcpHandleInputProcessor;

//---------------------------------------------------------------------------
//
//  CTextSelectionGripper
//
//  Encapsulates the logic for selection grippers.
//
//---------------------------------------------------------------------------
class CTextSelectionGripper final : public CCanvas, public IGripper
{
    friend class JupiterTextHelper;

    CTextSelectionGripper(_In_ CCoreServices *pCore);
    ~CTextSelectionGripper() override;

public:

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ HRESULT InitializeGripper(
        _In_ TextSelectionManager *pTextSelectionManager
        );

    void SetContentDirty(_In_ DirtyFlags flag);

    _Check_return_ HRESULT UpdateCenterWorldCoordinate(_In_ const XPOINTF& centerWorldCoordinate, _In_ XFLOAT lineHeight);

    _Check_return_ HRESULT Show(bool fAnimate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextSelectionGripper>::Index;
    }

    _Check_return_ HRESULT Hide(bool fAnimate);

    _Check_return_ XFLOAT GetActualOffsetX() override;
    _Check_return_ XFLOAT GetActualOffsetY() override;

    bool IsFocusable() final
    {
        return IsActive() &&
            IsVisible() &&
            IsEnabled() &&
            AreAllAncestorsVisible();
    }

    _Check_return_ HRESULT OnPointerPressed(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerMoved(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerReleased(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnRightTapped(_In_ CEventArgs* pEventArgs) override;

    void SetOwner(
        _In_ CUIElement *pOwner,
        _In_ bool isGripperMoving = false);

    CUIElement* GetOwner(){return m_pOwner;}
    void SetOwnerTextView(_In_ ITextView *pTextView){m_pTextView = pTextView;}
    ITextView* GetOwnerTextView(){return m_pTextView;}
    XFLOAT GetGripperCenterToLineEdgeOffset();
    void SetPopupNoAddRef(_In_ CPopup *pPopupNoAddRef){m_pPopupNoAddRef = pPopupNoAddRef;}
    bool IsStartGripper() const { return m_isStartGripper; }
    void SetStartGripper(_In_ bool isStart) { m_isStartGripper = isStart; }
    XINT32 GetStrokeColor(){return m_StrokeColor;}
    _Check_return_ HRESULT ReleaseGripperPointerCapture();

    // Sentinel value for UpdateCenterWorldCoordinate line height
    static const XFLOAT POLE_HEIGHT_UNCHANGED;
    _Check_return_ HRESULT CheckAndAdjustStrokeColor();

    // IGripper implementation
    _Check_return_ HRESULT UpdateVisibility() override;

protected:

    DirectUI::ManipulationModes GetManipulationModeCore() const override
    {
        return DirectUI::ManipulationModes::None;
    }

    _Check_return_ HRESULT SetManipulationModeCore(_In_ DirectUI::ManipulationModes value) override
    {
        return E_NOTIMPL;
    }

    _Check_return_ HRESULT MeasureOverride(
        _In_  XSIZEF availableSize,
        _Out_ XSIZEF& desiredSize) override;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

    static _Check_return_ HRESULT CreateFadeStoryboard(
        _Outptr_ CStoryboard   **ppStoryboard,
        _In_ CCoreServices         *pCore,
        _In_ CDependencyObject     *pTargetObject,
        _In_ const CDependencyProperty   *pTargetProperty,
        _In_ XFLOAT                 rFullTimeInSeconds,
        _In_ bool                  fFadeInTrueFadeOutFalse
        );

    _Check_return_ HRESULT HideImmediately();

    _Check_return_ HRESULT CheckAndRepositionShapesForPoleHeightChange(
        _In_ XFLOAT rPoleHeight
        );

protected:
    static _Check_return_ HRESULT OnShowAnimationComplete(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        );

    static _Check_return_ HRESULT OnHideAnimationComplete(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        );

public:
    bool IsGripperVisible();
    bool IsAnimating() { return m_isAnimating; }
    bool HasPointerCapture() { return m_hasPointerCapture; }

private:

    // Configuration data, converted from millimeters to pixels according to current DPI
    XFLOAT m_rInternalDiameter;
    XFLOAT m_rPaddingDiameter;
    XFLOAT m_rStrokeThickness;
    XFLOAT m_rPoleThickness;
    XFLOAT m_rHitTestBoxWidth;
    XFLOAT m_rHitTestBoxHeight;
    XFLOAT m_rGripperTopToCenterHeight;

    XFLOAT m_rPoleHeight;
    XPOINTF m_centerLocalCoordinate;
    XPOINTF m_centerWorldCoordinate;    // Cache for tracing
    TextSelectionManager *m_pTextSelectionManager;
    CUIElement *m_pOwner;
    ITextView  *m_pTextView;
    CPopup *m_pPopupNoAddRef;
    CEllipse *m_pEllipseInternalNoAddRef;
    CEllipse *m_pEllipseExternalNoAddRef;
    CRectangle *m_pRectanglePoleNoAddRef;
    CStoryboard *m_pShowAnimation;
    CStoryboard *m_pHideAnimation;
    IXcpHandleInputProcessor *m_pInputProcessor;
    bool   m_hideGripper            : 1;
    bool   m_hasPointerCapture      : 1;
    bool   m_isAnimating            : 1;
    bool   m_isHideAnimation        : 1;
    bool   m_isStartGripper         : 1;
    XINT32  m_StrokeColor;
    XINT32  m_uiCapturePointerId;

    static XFLOAT ConvertToPixels(_In_ XFLOAT distanceInMillimeters);
    static XFLOAT HiMetricsPerPixel();
    static XPOINT ConvertToIntPoint(_In_ const XPOINTF& coordinate);
    bool IsGripperClipped();
    _Check_return_ HRESULT UpdateParent();
    void UpdateCenterLocalCoordinate(_In_ const XPOINTF& centerLocalCoordinate);
    _Check_return_ HRESULT ConvertToLocalCoordinate(_In_ const XPOINTF& worldCoordinate, _Out_ XPOINTF *pLocalCoordinate);
    _Check_return_ HRESULT ConvertToWorldCoordinate(_In_ const XPOINTF& localCoordinate, _Out_ XPOINTF *pWorldCoordinate);
    _Check_return_ HRESULT PointerReleasedWorker(_In_ XUINT32 uiPointerId, _Inout_opt_ bool *pfHandled);
};
