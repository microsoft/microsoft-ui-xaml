// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>

class CPointer final : public CDependencyObject
{
protected:
    CPointer(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    // Creation method
    DECLARE_CREATE(CPointer);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointer>::Index;
    }

    XUINT32 GetPointerId()
    {
        return m_uiPointerId;
    }

    DirectUI::PointerDeviceType GetPointerDeviceType()
    {
        return m_pointerDeviceType;
    }

    static DirectUI::PointerDeviceType ToPointerDeviceType(XPointerInputType pointerInputType)
    {
        if (pointerInputType == XcpPointerInputTypeTouch)
        {
            return DirectUI::PointerDeviceType::Touch;
        }
        else if (pointerInputType == XcpPointerInputTypePen)
        {
            return DirectUI::PointerDeviceType::Pen;
        }
        else if (pointerInputType == XcpPointerInputTypeMouse)
        {
            return DirectUI::PointerDeviceType::Mouse;
        }
        ASSERT(FALSE);
        return DirectUI::PointerDeviceType::Mouse;
    }

    _Check_return_ HRESULT
    SetPointerFromPointerInfo(const PointerInfo& pointerInfo)
    {
        // Keep in sync with MatchesPointerInfo
        m_uiPointerId = pointerInfo.m_pointerId;
        m_bInContact = pointerInfo.m_bInContact;
        m_bInRange = pointerInfo.m_bInRange;
        m_bLeftButtonPressed = pointerInfo.m_bLeftButtonPressed;
        m_bRightButtonPressed = pointerInfo.m_bRightButtonPressed;
        m_bMiddleButtonPressed = pointerInfo.m_bMiddleButtonPressed;
        m_bBarrelButtonPressed = pointerInfo.m_bBarrelButtonPressed;
        m_pointerDeviceType = ToPointerDeviceType(pointerInfo.m_pointerInputType);

        return S_OK; //RRETURN_REMOVAL
    }

    bool MatchesPointerInfo(const PointerInfo& pointerInfo) const
    {
        // Keep in sync with SetPointerFromPointerInfo
        return
            m_uiPointerId == pointerInfo.m_pointerId
            && m_bInContact == pointerInfo.m_bInContact
            && m_bInRange == pointerInfo.m_bInRange
            && m_bLeftButtonPressed == pointerInfo.m_bLeftButtonPressed
            && m_bRightButtonPressed == pointerInfo.m_bRightButtonPressed
            && m_bMiddleButtonPressed == pointerInfo.m_bMiddleButtonPressed
            && m_bBarrelButtonPressed == pointerInfo.m_bBarrelButtonPressed
            && m_pointerDeviceType == ToPointerDeviceType(pointerInfo.m_pointerInputType);
    }

    _Check_return_ HRESULT
    SetPointer(_In_ CPointer* pPointer)
    {
        IFCPTR_RETURN(pPointer);

        m_uiPointerId = pPointer->m_uiPointerId;
        m_bInContact = pPointer->m_bInContact;
        m_bInRange = pPointer->m_bInRange;
        m_bLeftButtonPressed = pPointer->m_bLeftButtonPressed;
        m_bRightButtonPressed = pPointer->m_bRightButtonPressed;
        m_bMiddleButtonPressed = pPointer->m_bMiddleButtonPressed;
        m_bBarrelButtonPressed = pPointer->m_bBarrelButtonPressed;

        m_pointerDeviceType = pPointer->m_pointerDeviceType;

        return S_OK;
    }

public:
    // CPointer fields
    XINT32                          m_uiPointerId           = 0;
    DirectUI::PointerDeviceType     m_pointerDeviceType     = DirectUI::PointerDeviceType::Touch;
    bool                            m_bInContact            = false;
    bool                            m_bInRange              = false;
    bool                            m_bLeftButtonPressed    = false;
    bool                            m_bRightButtonPressed   = false;
    bool                            m_bMiddleButtonPressed  = false;
    bool                            m_bBarrelButtonPressed  = false;
};
