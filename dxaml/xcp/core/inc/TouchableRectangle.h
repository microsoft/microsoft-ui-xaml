// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//      Definition of a Rectangle shape class that is used as the
//      hit target for the CRichEditGripper class.
//  Synopsis:
//      Class that inherits from CRectangle in order to get access to certain
//      touch events for use with CRichEditGripper.

class CRichEditGripper;

class CTouchableRectangle : public CRectangle
{
public:
    DECLARE_CREATE(CTouchableRectangle);

    static XPOINT ConvertToIntPoint(_In_ const XPOINTF& coordinate);

    _Check_return_ HRESULT OnPointerPressed(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerMoved(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerReleased(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnRightTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnDoubleTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnHolding(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT ReleaseTouchRectPointerCapture(_Out_opt_ bool *pbReleased, _Out_opt_ XINT32 *pPointerId);
    _Check_return_ HRESULT GetPointerCaptured(_Out_opt_ bool *pbCaptured);
    _Check_return_ HRESULT DoFinalManipulation(_In_ XPOINT ptGlobal, _In_ bool captureReleased, _In_ INT32 pointerId);

protected:

    CTouchableRectangle(_In_ CCoreServices *pCore)
        : CRectangle(pCore)
    {
    }

    XPOINT m_ptGlobLast;
    XINT32  m_uiCapturePointerId = -1;
    bool m_hasPointerCapture = false;
    CEventArgs* m_pPendingDoubleTapArgs = nullptr;
    CEventArgs* m_pPendingTapArgs = nullptr;

private:

    CRichEditGripper* GetParentGripper() const;
};

