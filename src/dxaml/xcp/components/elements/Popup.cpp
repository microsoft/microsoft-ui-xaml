// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Popup.h"
#include "ImplicitAnimations.h"

ImplicitCloseGuard::ImplicitCloseGuard(_In_ CPopup* popup)
    : m_popupNoRef(popup)
{
    m_popupNoRef->m_isImplicitClose = TRUE;
}

ImplicitCloseGuard::~ImplicitCloseGuard()
{
    m_popupNoRef->m_isImplicitClose = FALSE;
}

void CPopup::CancelHideAnimationToPrepareForShow()
{
    CancelImplicitAnimation(ImplicitAnimationType::Hide);

    // Open() cancels not only hide animations on the popup, but on its child as well.
    if (m_pChild != nullptr)
    {
        m_pChild->CancelImplicitAnimation(ImplicitAnimationType::Hide);
    }

    if (m_unloadingChild)
    {
        m_unloadingChild->CancelImplicitAnimation(ImplicitAnimationType::Hide);
    }
}

bool CPopup::IsUnloading() const
{
    return m_isUnloading;
}

CPopup* CPopup::GetPopupOfUnloadingChild(_In_ CUIElement* child)
{
    CDependencyObject* logicalParent = child->GetLogicalParentNoRef();
    if (logicalParent != nullptr && logicalParent->OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        CPopup* parentPopup = static_cast<CPopup*>(logicalParent);
        if (parentPopup->m_unloadingChild == child)
        {
            return parentPopup;
        }
    }
    return nullptr;
}

void CPopup::RemoveUnloadingChild()
{
    RemoveLogicalChild(m_unloadingChild);
    m_unloadingChild->SetAssociated(false, nullptr);
    m_unloadingChild = nullptr;
}
