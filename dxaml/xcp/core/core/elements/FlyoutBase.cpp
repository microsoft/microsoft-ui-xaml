// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// FlyoutBase is DependencyObject and x:Name registration for
// framework objects is performed on UIElement level, flyout needs to
// have an object in core which will override EnterImpl method to register
// its children names.
// Alternative approach of moving the registration call to CDependencyObject
// was rejected because of its negative effect on performance.

#include "precomp.h"
#include <MenuFlyout.h>

CFlyoutBase::CFlyoutBase(_In_ CCoreServices *pCore)
    : CDependencyObject(pCore)
{}

_Check_return_ HRESULT CFlyoutBase::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    if(HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::DependencyObject_EnterImpl(
            this,
            pNamescopeOwner,
            params.fIsLive,
            params.fSkipNameRegistration,
            params.fCoercedIsEnabled,
            params.fUseLayoutRounding
        ));
    }

    return S_OK;
}

_Check_return_ HRESULT CFlyoutBase::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    if(HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::DependencyObject_LeaveImpl(
            this,
            pNamescopeOwner,
            params.fIsLive,
            params.fSkipNameRegistration,
            params.fCoercedIsEnabled,
            params.fVisualTreeBeingReset
        ));
    }

    return S_OK;
}

xref_ptr<CXamlIslandRoot> CFlyoutBase::GetIslandContext()
{
    if (auto visualTree = VisualTree::GetForElementNoRef(this))
    {
        return xref_ptr<CXamlIslandRoot>(do_pointer_cast<CXamlIslandRoot>(visualTree->GetRootElementNoRef()));
    }
    return xref_ptr<CXamlIslandRoot>(nullptr);
}

_Check_return_ HRESULT CFlyoutBase::OnPlacementUpdated(_In_ const MajorPlacementMode majorPlacementMode)
{
    auto iter = m_placementUpdatedSubscribers.begin();
    while(iter != m_placementUpdatedSubscribers.end())
    {
        auto& subscriber = *iter;
        xref_ptr<CDependencyObject> pObject = subscriber.pTarget.lock();
        if (pObject)
        {
            IFC_RETURN(subscriber.callbackFunc(pObject, majorPlacementMode));
            ++iter;
        }
        else
        {
            iter = m_placementUpdatedSubscribers.erase(iter);
        }
    }

    return S_OK;
}

void CFlyoutBase::AddPlacementUpdatedSubscriber(_In_ CDependencyObject *pTarget,
    const std::function<HRESULT(CDependencyObject *, MajorPlacementMode)>& callbackFunc)
{
    m_placementUpdatedSubscribers.push_back(OnPlacementUpdatedSubscriber(xref::get_weakref(pTarget), callbackFunc));
}

_Check_return_ HRESULT CFlyoutBase::RemovePlacementUpdatedSubscriber(_In_ CDependencyObject *pTarget)
{
    auto targetPos = std::find_if(m_placementUpdatedSubscribers.begin(), m_placementUpdatedSubscribers.end(),
        [pTarget](const auto& subscriber)
        {
            xref_ptr<CDependencyObject> pObject = subscriber.pTarget.lock();
            return pObject && pTarget == pObject.get();
        });
    if (targetPos != m_placementUpdatedSubscribers.end())
    {
        m_placementUpdatedSubscribers.erase(targetPos);
        return S_OK;
    }
    return E_NOTFOUND;
}