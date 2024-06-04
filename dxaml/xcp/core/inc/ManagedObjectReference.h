// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NoParentShareableDependencyObject.h>
#include "EnumDefs.g.h"
//
//  CManagedObjectReference
//
//  This is a proxy class used to keep a reference to a pure managed
//  object (one that isn't backed by a CDependencyObject).
//  It doesn't keep track of its parent, but can be shared.
//

class CManagedObjectReference final : public CNoParentShareableDependencyObject
{
public:
    CManagedObjectReference(_In_ CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
    {
        m_nativeValue.SetNull();
    }

    XCP_FORCEINLINE bool IsMarkupExtension()
    {
        return m_markupExtensionType == DirectUI::MarkupExtensionType::Extension || IsBindingExtension();
    }

    XCP_FORCEINLINE bool IsBindingExtension()
    {
        return m_markupExtensionType == DirectUI::MarkupExtensionType::Binding;
    }

public:

// Creation method

    DECLARE_CREATE(CManagedObjectReference);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CManagedObjectReference>::Index;
    }

    //override
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Our managed peer always has state by definition, so we always
        // have to participate in the managed tree.

        return PARTICIPATES_IN_MANAGED_TREE;
    }

public:
    // Public members
    DirectUI::MarkupExtensionType m_markupExtensionType = DirectUI::MarkupExtensionType::None;
    CValue m_nativeValue;
};
