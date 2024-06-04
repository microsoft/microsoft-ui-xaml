// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class TextServicesHost;
class ITextServices;
class ITextHost2;
class CTextBoxBase;

#include "RichEditGripperChild.h"
#include "RichEditGripperCommon.h"

//---------------------------------------------------------------------------
//
//  CRichEditGripper
//
//  Encapsulates the logic for selection grippers.
//
//---------------------------------------------------------------------------
class CRichEditGripper final : public CXcpObjectBase<>
{
    CRichEditGripper() = default;

public:
    _Check_return_ HRESULT InitializeGripper(
        _In_ ITextServices *pTextServices,
        _In_ RichEditGripperCommon::GripperType gripperType,
        _In_ ITextHost2 *pTextHost,
        _In_ CTextBoxBase *pTextBoxBase);

    void SetPopup(_In_ CPopup *pPopup);
    CPopup* GetPopup() const { return m_pPopup; }

    _Check_return_ HRESULT UpdateCenterWorldCoordinate(
        _In_ const XPOINTF& centerWorldCoordinate,
        _In_ XFLOAT lineHeight);

    _Check_return_ XFLOAT GetActualOffsetX();

    _Check_return_ XFLOAT GetActualOffsetY();

    static _Check_return_ HRESULT Create(
        _Outptr_ CRichEditGripper **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ HRESULT Show(bool bForSelection);
    _Check_return_ HRESULT Hide();

    _Check_return_ HRESULT ShowPole(RichEditGripperCommon::PoleMode ePoleMode);

    _Check_return_ HRESULT OnManipulation(bool begin, bool end, XINT32 x0, XINT32 y0, INT32 pointerId);
    _Check_return_ HRESULT OnHitTargetTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnHitTargetRightTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnHitTargetDoubleTapped(_In_ CEventArgs* pEventArgs);
    XRECT_WH GetGripperScreenRect();

    _Check_return_ bool HasTextSelection();

    _Check_return_ HRESULT UpdateThemeColor();

    XPOINTF GetPosition();
    CTouchableRectangle* GetHitTarget();

    xref_ptr<CRichEditGripperChild> m_pPopupChild;

private:

    xref_ptr<CPopup> m_pPopup;
};

