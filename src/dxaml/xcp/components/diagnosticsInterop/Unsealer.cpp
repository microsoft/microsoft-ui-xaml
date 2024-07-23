// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\Unsealer.h"
#include "Style.h"
#include "SetterBaseCollection.h"
#include "SetterBase.h"
#include "RuntimeEnabledFeatures\inc\RuntimeEnabledFeatures.h"
#include "DoPointerCast.h"

using namespace RuntimeFeatureBehavior;

namespace Diagnostics
{
    ObjectUnsealer::ObjectUnsealer(bool wasSealed, _In_ CDependencyObject* owner)
        : m_wasSealed(wasSealed)
        , m_owner(xref::weakref_ptr<CDependencyObject>(owner))
    {
        ASSERT(RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeEnabledFeature::XamlDiagnostics));
    }

    std::unique_ptr<ObjectUnsealer> ObjectUnsealer::UnsealIfRequired(_In_ CDependencyObject* obj)
    {
        auto setterBase = do_pointer_cast<CSetterBase>(obj);
        if (setterBase)
        {
            return std::make_unique<SetterUnsealer>(setterBase);
        }

        auto style = do_pointer_cast<CStyle>(obj);
        if (style)
        {
            return std::make_unique<StyleUnsealer>(style);
        }

        auto setterBaseCollection = do_pointer_cast<CSetterBaseCollection>(obj);
        if (setterBaseCollection)
        {
            return std::make_unique<SetterCollectionUnsealer>(setterBaseCollection);
        }

        return std::unique_ptr<ObjectUnsealer>();
    }

    StyleUnsealer::StyleUnsealer(_In_ CStyle* style)
        : ObjectUnsealer(style->m_bIsSealed, style)
    {
        style->m_bIsSealed = false;

        if (style->m_pBasedOn)
        {
            m_basedOnStyleResealer = std::make_unique<StyleUnsealer>(style->m_pBasedOn);
        }

        if (style->m_pBasedOnSetters)
        {
            m_basedOnSettersResealer = std::make_unique<SetterCollectionUnsealer>(style->m_pBasedOnSetters);
        }
    }

    StyleUnsealer::~StyleUnsealer()
    {
        auto style = do_pointer_cast<CStyle>(m_owner.lock_noref());
        if (style && m_wasSealed)
        {
            VERIFYHR(style->Seal());
        }
    }

    SetterCollectionUnsealer::SetterCollectionUnsealer(_In_ CSetterBaseCollection* setterCollection)
        : ObjectUnsealer(setterCollection->m_bIsSealed, setterCollection)
    {
        setterCollection->m_bIsSealed = false;
        // When unsealed via diagnostics, allow invalid Setters that don't have Setter.Value, Setter.Property, or Setter.Target set 
        // to be inserted into the collection
        setterCollection->m_allowInvalidSetter = true;
    }

    SetterCollectionUnsealer::~SetterCollectionUnsealer()
    {
        auto owner = do_pointer_cast<CSetterBaseCollection>(m_owner.lock_noref());
        if (owner && m_wasSealed)
        {
            owner->SetIsSealed();
        }
    }

    SetterUnsealer::SetterUnsealer(_In_ CSetterBase* setter)
        : ObjectUnsealer(setter->m_bIsSealed, setter)
    {
        setter->m_bIsSealed = false;
    }

    SetterUnsealer::~SetterUnsealer()
    {
        auto owner = do_pointer_cast<CSetterBase>(m_owner.lock_noref());
        if (owner && m_wasSealed)
        {
            owner->SetIsSealed();
        }
    }

}
