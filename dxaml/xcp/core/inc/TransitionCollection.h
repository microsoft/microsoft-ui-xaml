// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTransitionCollection final : public CDOCollection
{
protected:
    CTransitionCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTransitionCollection() // !!! FOR UNIT TESTING ONLY !!!
        : CTransitionCollection(nullptr)
    {}
#endif

    // Creation method
    DECLARE_CREATE(CTransitionCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransitionCollection>::Index;
    }

    _Check_return_ HRESULT GetApplicableTransitionsNoAddRefs(_In_ CUIElement* pTarget, _In_ DirectUI::TransitionTrigger trigger, _Inout_ xvector<CTransition*>& applicableTransitions)
    {
        for (auto transition : *this)
        {
            CTransition* pTransition = NULL;
            bool participate = false;
            IFC_RETURN(DoPointerCast(pTransition, transition));
            IFC_RETURN(pTransition->ParticipateInTransitions(pTarget, trigger, &participate));
            if (participate)
            {
                IFC_RETURN(applicableTransitions.push_back(pTransition));
            }
        }

        return S_OK;
    }

    _Check_return_ bool NeedsOwnerInfo() override
    {
        return false;
    }


    bool DoesAllowMultipleAssociation() const override
    {
        return true;
    }

    void SetAssociated(_In_ bool isAssociated, _In_opt_ CDependencyObject *pAssociationOwner) final
    {
        CDOCollection::SetAssociated(isAssociated, isAssociated ? pAssociationOwner : nullptr);

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

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Transitioncollection keeps transitions that will always have managed peers
        // so we'll have to keep these elements alive.
        // If we are in a style, this would not happen since we're setting it on Setter.Value,
        // and the managed peers of my children would get cleaned up.
        return PARTICIPATES_IN_MANAGED_TREE;
    }

private:
    // Number of DOs this object is associated with
    XUINT32 m_cShares = 0;
};
