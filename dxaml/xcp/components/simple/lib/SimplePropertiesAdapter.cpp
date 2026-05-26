// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <SimpleProperties.h>
#include <SimplePropertyImpl.h>
#include "StaticAssertFalse.h"

// Include all dependencies here

#include "UIElement.h"
#include "LinearGradientBrush.h"
#include "PropertyTransitions.h"
#include "EnumDefs.g.h"

#include "SimplePropertiesAdapter.g.h"
#include <DXamlServices.h>
#include "corep.h"

namespace SimpleProperty
{
    static changehandlerstables& GetHandlersTables(CCoreServices* core)
    {
        return core->GetSimplePropertyHandlersTables();
    }

    template <typename declaring_type>
    sparsetables& GetSparseTablesForTypeImpl(CCoreServices* core, std::true_type)
    {
        return core->GetSimplePropertySparseTables();
    }

    template <typename declaring_type>
    sparsetables& GetSparseTablesForTypeImpl(CCoreServices* core, std::false_type)
    {
        static_assert_false("GetSparseTablesForTypeImpl overload needs to be implemented for <declaring_type>.");
    }

    template <typename declaring_type>
    sparsetables& GetSparseTablesForType()
    {
        return GetSparseTablesForTypeImpl<declaring_type>(
            DirectUI::DXamlServices::GetHandle(),
            std::is_base_of<CDependencyObject, declaring_type>());
    }

    template <typename declaring_type>
    sparsetables& GetSparseTablesForType(const_objid_t obj_id)
    {
        return GetSparseTablesForTypeImpl<declaring_type>(
            MapObjIdToInstance<declaring_type>(obj_id)->GetContext(),
            std::is_base_of<CDependencyObject, declaring_type>());
    }

    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForTypeImpl(CCoreServices* core, std::true_type)
    {
        return core->GetSimplePropertyHandlersTables();
    }

    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForTypeImpl(CCoreServices* core, std::false_type)
    {
        static_assert_false("GetHandlersTablesForTypeImpl overload needs to be implemented for <declaring_type>.");
    }

    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType()
    {
        return GetHandlersTablesForTypeImpl<declaring_type>(
            DirectUI::DXamlServices::GetHandle(),
            std::is_base_of<CDependencyObject, declaring_type>());
    }

    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType(const_objid_t obj_id)
    {
        return GetHandlersTablesForTypeImpl<declaring_type>(
            MapObjIdToInstance<declaring_type>(obj_id)->GetContext(),
            std::is_base_of<CDependencyObject, declaring_type>());
    }

    size_t TranslatePIDToIndex(PID pid)
    {
        return static_cast<size_t>(pid) - KnownDependencyPropertyCount;
    }
}