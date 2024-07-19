// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <base\inc\weakref_ptr.h>

class CStyle;
class CSetterBaseCollection;
class CSetterBase;
class CDependencyObject;

namespace Diagnostics
{
    struct ObjectUnsealer
    {
        static std::unique_ptr<ObjectUnsealer> UnsealIfRequired(_In_ CDependencyObject* object);
        
        ObjectUnsealer(const ObjectUnsealer& rhs) = delete;
        ObjectUnsealer& operator=(const ObjectUnsealer& rhs) = delete;
        ObjectUnsealer&& operator=(ObjectUnsealer&& rhs) = delete;
        ObjectUnsealer(ObjectUnsealer&& rhs) = delete;
        virtual ~ObjectUnsealer() = default;

    private:
        ObjectUnsealer() = default;
    protected:
        ObjectUnsealer(bool wasSealed, _In_ CDependencyObject* owner);
        bool m_wasSealed = false;
        xref::weakref_ptr<CDependencyObject> m_owner;
    };
    
    struct SetterCollectionUnsealer final : ObjectUnsealer
    {
        SetterCollectionUnsealer(_In_ CSetterBaseCollection* setterCollection);
        ~SetterCollectionUnsealer() override;
        SetterCollectionUnsealer(SetterCollectionUnsealer&& rhs) = default;
    };

    struct SetterUnsealer final : ObjectUnsealer
    {
        SetterUnsealer(_In_ CSetterBase* setter);
        ~SetterUnsealer() override;

        SetterUnsealer(SetterUnsealer&& rhs) = default;
    };

    struct StyleUnsealer final : ObjectUnsealer
    {
        StyleUnsealer(_In_ CStyle* style);
        ~StyleUnsealer() override;

        StyleUnsealer(StyleUnsealer&& rhs) = default;

    private:
        std::unique_ptr<SetterCollectionUnsealer> m_basedOnSettersResealer;
        std::unique_ptr<StyleUnsealer> m_basedOnStyleResealer;
    };
}

