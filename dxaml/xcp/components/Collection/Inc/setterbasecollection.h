// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "DOCollection.h"

namespace Diagnostics
{
    struct SetterCollectionUnsealer;
}

class CSetterBaseCollection : public CDOCollection
{
    friend struct Diagnostics::SetterCollectionUnsealer;

protected:
    CSetterBaseCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    CSetterBaseCollection(_In_ CCoreServices *pCore, _In_ bool shouldBeParentToItems)
        : CDOCollection(pCore, shouldBeParentToItems)
    {}

   ~CSetterBaseCollection() override;

public:
// Creation method

    DECLARE_CREATE(CSetterBaseCollection);

    void SetIsSealed() { m_bIsSealed = true; }

    bool AllowsInvalidSetter() const { return m_allowInvalidSetter;}

    bool IsSetterValid(_In_ CSetter* setter) const;
    bool IsSetterValueValid(_In_ CSetter* setter) const;

    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) final;
    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject) final;

    // Children will need to be able to Find named items relative to themselves, and as such
    // need to have an ensured path to a NamescopeOwner.  This is required for Enter/Leave to
    // be processed correctly on child items.  Without it, InheritanceContext is not set and
    // bindings will fail.
    bool ShouldEnsureNameResolution() final { return true; }

    // Enter and Leave should only be processed for non-Style setters (to support bindings)
    _Check_return_ HRESULT ChildEnter(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        EnterParams params,
        bool fCanProcessEnterLeave
        ) override;


    // Enter and Leave should only be processed for non-Style setters (to support bindings)
    _Check_return_ HRESULT ChildLeave(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        LeaveParams params,
        bool fCanProcessEnterLeave
        ) override;

    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT GetSetterIndexByPropertyId(
        _In_ KnownPropertyIndex uPropertyId,
        _In_ KnownTypeIndex targetTypeIndex,
        _Out_ XUINT32& uIndex,
        _Out_ bool& bFound);

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSetterBaseCollection>::Index;
    }

    bool m_bIsSealed = false;
private:
    bool m_allowInvalidSetter = false;
};

// Internal type for storing setters. Overrides virtual to avoid parent-child relationship
// with items.
class CBasedOnSetterCollection final : public CSetterBaseCollection
{
private:
    CBasedOnSetterCollection(_In_ CCoreServices *pCore)
        : CSetterBaseCollection(pCore, false)
    {}

public:
    DECLARE_CREATE(CBasedOnSetterCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBasedOnSetterCollection>::Index;
    }
};

