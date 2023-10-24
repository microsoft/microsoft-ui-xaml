// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

// A DO that allows multiple association
class CShareableDependencyObject : public CDependencyObject
{
protected:
    CShareableDependencyObject(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    // CDependencyObject overrides

    bool DoesAllowMultipleAssociation() const final
    {
        return true;
    }

    // Mark as associated/unassociated with an owner. An association owner must be provided when
    // a CShareableDependencyObject is associated, but need not be provided when unassociated.
    void SetAssociated(_In_ bool isAssociated, _In_opt_ CDependencyObject *pAssociationOwner) final
    {
        CDependencyObject::SetAssociated(isAssociated, isAssociated ? pAssociationOwner : nullptr);

        if (isAssociated)
        {
            m_cShares++;

            // Association owner must be provided when associated
            ASSERT(pAssociationOwner);
        }
        else if (m_cShares > 0)
        {
            m_cShares--;
        }

        IGNOREHR(OnMultipleAssociationChange(isAssociated ? pAssociationOwner : nullptr));
    }

    bool IsAssociated() const final
    {
        return m_cShares > 0;
    }

    XUINT32 GetSharingCount() const
    {
        return m_cShares;
    }

private:
    // Number of associations
    unsigned int m_cShares = 0;
};
