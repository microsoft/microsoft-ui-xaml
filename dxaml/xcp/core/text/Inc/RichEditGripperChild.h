// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichEditGripperCommon.h"

class TextServicesHost;
class ITextServices;
class ITextHost2;
class CTextBoxBase;

class CRichEditGripper;

//---------------------------------------------------------------------------
//
//  CRichEditGripperChild
//
//  Encapsulates the logic for selection grippers.
//
//---------------------------------------------------------------------------
class CRichEditGripperChild : public CCanvas
{
    CRichEditGripperChild(_In_ CCoreServices *pCore);

public:

    static const XFLOAT c_rStrokeThickness;
    static const XFLOAT c_rPoleThickness;
    static const XFLOAT c_rInternalDiameter;
    static const XFLOAT c_rPaddingDiameter;
    static const int c_nGripperWidth;
    static const int c_nGripperDescent;

    _Check_return_ HRESULT InitializeGripper(
        _In_ ITextServices *pTextServices,
        _In_ RichEditGripperCommon::GripperType gripperType,
        _In_ ITextHost2 *pTextHost,
        _In_ CTextBoxBase *pTextBoxBase);

    _Check_return_ HRESULT UpdateCenterWorldCoordinate(
        _In_ const XPOINTF& centerWorldCoordinate,
        _In_ XFLOAT lineHeight);

    _Check_return_ XFLOAT GetActualOffsetX() override;

    _Check_return_ XFLOAT GetActualOffsetY() override;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds) override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ HRESULT Show(bool bForSelection);
    _Check_return_ HRESULT Hide();

    _Check_return_ HRESULT ShowPole(RichEditGripperCommon::PoleMode ePoleMode);

    _Check_return_ HRESULT OnManipulation(bool begin, bool end, XINT32 x0, XINT32 y0, INT32 pointerId);
    _Check_return_ HRESULT OnHitTargetTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnHitTargetRightTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnHitTargetDoubleTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnPointerPressedWithBarrelButtonDown(_In_ CEventArgs* pEventArgs);
    XRECT_WH GetGripperScreenRect();

    _Check_return_ HRESULT UpdateThemeColor();

    void SetPeer(_In_ CRichEditGripper *pPeerGripper);
    CRichEditGripper* GetPeerNoRef();
    XPOINTF GetPosition();
    CTouchableRectangle *GetHitTarget();
    BOOL InActiveDrag() const { return m_bActiveDrag;}
    void SetInActiveDrag(BOOL bActiveDrag){ m_bActiveDrag = bActiveDrag;}
    _Check_return_ bool HasTextSelection();

    CRichEditGripper *m_pParentGripperNoRef = nullptr;

    _Check_return_ HRESULT OnGripperHeld(_In_ CEventArgs* pEventArgs);

private:
    void UpdateCenterLocalCoordinate(_In_ const XPOINTF& centerLocalCoordinate);
    void SetContentDirty(_In_ DirtyFlags flag);
    _Check_return_ HRESULT CheckAndRepositionShapesForPoleHeightChange(XFLOAT poleHeight);

    static const XFLOAT c_rHitTargetWidthHeight;
    static const XFLOAT c_rHitTargetVerticalOffset;
    static const XFLOAT c_rPoleHeight;
    static const XFLOAT POLE_HEIGHT_UNCHANGED;

    XFLOAT m_rPoleHeight = 0;
    XPOINTF m_centerLocalCoordinate = { 0, 0 };
    XPOINTF m_centerWorldCoordinate = { 0, 0 };    // Cache for tracing

    XPOINT m_ptLastManipClient = { 0, 0 };

    RichEditGripperCommon::PoleMode m_ePoleMode = RichEditGripperCommon::PoleMode::Hidden;

    bool m_bPoleOutOfBounds = false;
    BOOL m_bActiveDrag = FALSE;  // used to help tell when we should show the PCP pole

    RichEditGripperCommon::GripperType m_gripperType = RichEditGripperCommon::GripperType::Start;

    CEllipse *m_pEllipseInternalNoAddRef = nullptr;
    CEllipse *m_pEllipsePaddingNoAddRef = nullptr;
    CTouchableRectangle *m_pRectHitTargetNoAddRef = nullptr;
    CRectangle *m_pRectanglePoleNoAddRef = nullptr;
    CRectangle *m_pRectPcpPoleNoAddRef = nullptr;
    CRectangle *m_pRectPcpPoleBgNoAddRef = nullptr;
    CRichEditGripper *m_pPeerGripperNoAddRef = nullptr;

    ITextServices *m_pTextServices = nullptr;
    CTextBoxBase *m_pTextBoxBase = nullptr;
};

