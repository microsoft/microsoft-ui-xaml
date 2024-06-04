// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NoParentShareableDependencyObject.h"

// Base typed SHARED objects that hold a list of parents.
class CMultiParentShareableDependencyObject : public CNoParentShareableDependencyObject
{

public:
    _Ret_maybenull_ CDependencyObject* GetMentor() final;

    std::size_t GetParentCount() const final;

    _Ret_notnull_ CDependencyObject* GetParentItem(std::size_t index) const final;

protected:
    CMultiParentShareableDependencyObject(_In_ CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
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

    CMultiParentShareableDependencyObject(_In_ const CMultiParentShareableDependencyObject& original, _Out_ HRESULT& hr)
        : CNoParentShareableDependencyObject(original, hr)
    {}

public:
    bool DoesAllowMultipleParents() const final
    {
        return true;
    }

    _Check_return_ HRESULT AddParent(
        _In_ CDependencyObject *pNewParent,
        bool fPublic = true,
        _In_opt_ RENDERCHANGEDPFN pfnParentRenderChangedHandler = nullptr
        ) final;

    _Check_return_ HRESULT RemoveParent(_In_ CDependencyObject *pParentToRemove) final;

    void SetParentForInheritanceContextOnly(_In_opt_ CDependencyObject *pDO) final;

    // allows skipping a call to enter
    bool ShouldSharedObjectRegisterName(_In_opt_ CResourceDictionary *pParentResourceDictionary) const final;

#if DBG
    bool HasParent(_In_ CDependencyObject *pCandidate) const override;
#endif

    CDependencyObject* GetParent() const final {return nullptr;}

protected:
    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    struct ParentAssociation
    {
        CDependencyObject *pParent;
        RENDERCHANGEDPFN pfnRenderChangedHandler;

        //
        // The only reason for these constructors is to call XcpMarkWeakPointer() - otherwise
        // they wouldn't be needed.
        //

        ParentAssociation(_In_opt_ CDependencyObject* argParent = nullptr, _In_opt_ RENDERCHANGEDPFN handlerArg = nullptr)
            : pParent(argParent)
            , pfnRenderChangedHandler(handlerArg)
        {
            XCP_WEAK(&pParent);
        }

        ParentAssociation(const ParentAssociation& toCopy)
            : ParentAssociation(toCopy.pParent, toCopy.pfnRenderChangedHandler)
        {
        }
    };

    const std::vector<ParentAssociation>& GetParentAssociation() const
    {
        return m_rgParentAssociation;
    }

private:
    std::vector<ParentAssociation> m_rgParentAssociation;

    // InheritanceContext becomes disabled anytime a DO has multiple parents except in the case
    // where the ResourceDictionary is the first parent
    bool fInheritanceContextEnabled = true;
};
