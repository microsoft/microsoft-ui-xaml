// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "ShareableDependencyObject.h"

// Base typed SHARED reference counted object.
class CNoParentShareableDependencyObject : public CShareableDependencyObject
{
public:
    bool IsActive() const final { return m_cEnteredLive > 0; }

    void ActivateImpl() final
    {
        CDependencyObject::ActivateImpl();
        ASSERT(m_cEnteredLive == 0);
        m_cEnteredLive = 1;
    }
    void DeactivateImpl() final
    {
        CDependencyObject::DeactivateImpl();
        ASSERT(m_cEnteredLive == 1);
        m_cEnteredLive = 0;
    }

    // Participate in the managed peer tree (for lifetime management)
    // if this object has non-recoverable managed state on it.
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        if( IsCustomType() )
            return PARTICIPATES_IN_MANAGED_TREE;
        else
            return OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT Enter(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT Leave(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

    // allows skipping a call to enter
    virtual bool ShouldSharedObjectRegisterName(_In_opt_ CResourceDictionary *) const
    {
        return GetSharingCount() <= 1;
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CNoParentShareableDependencyObject>::Index;
    }

    // Classes that support cloning need to implement the cloning constructor
    //  CDerivedType(_In_ const CDerivedType& original, _Out_ HRESULT& hr) : CBaseType(original, hr)
    //  (See comments on the base cloning constructor CNoParentShareableDependencyObject for more instructions.)
    // Override Clone() using the macro DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE unless there
    //  are specific needs.  If not using the macro, make sure to hit the following
    //  important points:
    //  (1) Call cloning constructor to create the cloned instance.
    //  (2) IFCOOM() result of constructor to see if allocation failed for the new object.
    //  (3) IFC(hr) to see if allocation was successful, but ran into problems for other reasons.
    //  (4) Call ClonePropertySetField() to copy the property set fields, so that local value source
    //      of values is not lost.
    //  (5) Do your special work (including comments that explains why it can't be done in the cloning constructor)
    //  (6) If everything was successful, return the new object via ppClone.
    virtual _Check_return_ HRESULT Clone( _Out_ CNoParentShareableDependencyObject **ppClone  ) { *ppClone = NULL; RRETURN(E_NOTIMPL); }

    XUINT32 IsClone(){ return m_fIsClone; }

#if DBG
    bool HasParent(_In_ CDependencyObject *) const override
    {
        ASSERT(false);
        return false;
    }

#endif

protected:
    CNoParentShareableDependencyObject(_In_ CCoreServices *pCore)
        : CShareableDependencyObject(pCore)
        , m_fIsClone(FALSE)
        , m_cEnteredLive(0)
    {}

    // Protected constructor to help derived classes implement Clone();  Implementations
    //  need to:
    //    (1) Set defaults to make sure the class is at least in a valid
    //        state.  (Set pointers to NULL, etc.)
    //    (2) IFC(hr) in case the base constructor ran into problems.  This is so
    //        the original failure HRESULT doesn't get overwritten by (3).
    //    (3) Only then is it safe to proceed and do additional work for cloning.
    //        Work that may fail with HRESULT that gets passed to derived type's constructors.
    // Types that are trivial to clone may skip steps 2 and 3, as hr will pass
    //  straight through and there's nothing else to risk stomping over it.

    CNoParentShareableDependencyObject(_In_ const CNoParentShareableDependencyObject& original, _Out_ HRESULT& hr)
        : CShareableDependencyObject(original.GetContext())
        , m_fIsClone(TRUE)
        , m_cEnteredLive(0)
    {
        // Current code is written on the assumption that clones will not themselves be cloned.
        ASSERT(!original.m_fIsClone);

        hr = S_OK;
    }

    // Objects that fire OnConnected/OnDisconnected do so in Enter/Leave. CNoParentShareableDependencyObject overrides
    // Enter/Leave and skips the method unless this is the first enter/last leave. In those cases, we still need to fire
    // OnConnected/OnDisconnected, so we use these methods.
    virtual _Check_return_ HRESULT OnSkippedLiveEnter();
    virtual _Check_return_ HRESULT OnSkippedLiveLeave();

private:
    XUINT32 m_fIsClone : 1;
    XUINT32 m_cEnteredLive : 31;
};
