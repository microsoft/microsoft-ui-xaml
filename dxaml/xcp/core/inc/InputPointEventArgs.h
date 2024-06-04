// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Input point event args implementation that is used as a common base for Drag
//  event args and mouse event args

#pragma once
#include "RoutedEventArgs.h"

class CInputPointEventArgs : public CRoutedEventArgs
{
public:
    CInputPointEventArgs(_In_ CCoreServices* pCore)
    {
        m_pCore = pCore;
        m_ptGlobal.x = 0;
        m_ptGlobal.y = 0;
        m_pointerDeviceType = DirectUI::PointerDeviceType::Touch;
    }

    // Destructor
    ~CInputPointEventArgs() override
    {
        m_ptGlobal.x = 0;
        m_ptGlobal.y = 0;
    }

    _Check_return_ HRESULT GetPosition(
        _In_opt_ CUIElement* pRelativeTo,
        _Out_ wf::Point* pRelativePoint);

    _Check_return_ HRESULT GetRelativePosition(
        _In_opt_ CUIElement* pRelativeTo,
        _Inout_ wf::Point* pRelativePoint);

    _Check_return_ HRESULT ConvertGlobalPointToRelativePoint(
        _In_opt_ CUIElement* pRelativeTo,
        _Inout_ wf::Point* pRelativePoint);

    DirectUI::PointerDeviceType GetPointerDeviceType(_In_ XPointerInputType pointerInputType)
    {
        DirectUI::PointerDeviceType pointerDeviceType = DirectUI::PointerDeviceType::Touch;

        if (pointerInputType == XcpPointerInputTypeTouch)
        {
            pointerDeviceType = DirectUI::PointerDeviceType::Touch;
        }
        else if (pointerInputType == XcpPointerInputTypePen)
        {
            pointerDeviceType = DirectUI::PointerDeviceType::Pen;
        }
        else if (pointerInputType == XcpPointerInputTypeMouse)
        {
            pointerDeviceType = DirectUI::PointerDeviceType::Mouse;
        }
        else
        {
            ASSERT(FALSE);
        }

        return pointerDeviceType;
    }

    _Check_return_ HRESULT get_PointerDeviceType(_Out_ DirectUI::PointerDeviceType* pValue)
    {
        *pValue = m_pointerDeviceType;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_PointerDeviceType(_In_ DirectUI::PointerDeviceType value)
    {
        m_pointerDeviceType = value;
        RRETURN(S_OK);
    }

    XPOINTF GetGlobalPoint() const
    {
        return m_ptGlobal;
    }

    void SetGlobalPoint(XPOINTF ptGlobal)
    {
        m_ptGlobal = ptGlobal;
    }

    void SetGlobalPoint(wf::Point ptGlobal)
    {
        m_ptGlobal.x = ptGlobal.X;
        m_ptGlobal.y = ptGlobal.Y;
    }

    DirectUI::PointerDeviceType m_pointerDeviceType;

protected:
    HRESULT TransformPointToElement(_In_opt_ CUIElement * pUIElement, _Inout_ wf::Point* ppt);
    CCoreServices* m_pCore;
    // The mouse coordinate in DIP values in relation to the window and not the screen.  
    // This is exposed only to other internal code (specifically CInputServices)
    XPOINTF m_ptGlobal;
};

