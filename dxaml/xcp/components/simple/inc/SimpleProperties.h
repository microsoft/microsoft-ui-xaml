// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>
#include <Operators.h>
#include "StaticAssertFalse.h"

namespace SimpleProperty
{
    using PID = KnownPropertyIndex;
    using TID = KnownTypeIndex;
}

#include <SimpleProperty.h>
#include "SimplePropertiesMetadata.g.h"

namespace SimpleProperty
{
    namespace details
    {
        template <typename declaring_type, typename idtype>
        declaring_type* MapObjIdToInstanceImpl(idtype obj_id, std::true_type)
        {
            return reinterpret_cast<declaring_type*>(obj_id);
        }

        template <typename declaring_type, typename idtype>
        declaring_type* MapObjIdToInstanceImpl(idtype obj_id, std::false_type)
        {
            static_assert_false("MapObjIdToInstanceImpl overload needs to be implemented for <declaring_type>.");
        }
    }

    template <typename declaring_type>
    declaring_type* MapObjIdToInstance(objid_t obj_id)
    {
        return details::MapObjIdToInstanceImpl<declaring_type>(
            obj_id,
            std::is_base_of<CDependencyObject, declaring_type>());
    }

    template <typename declaring_type>
    const declaring_type* MapObjIdToInstance(const_objid_t obj_id)
    {
        return details::MapObjIdToInstanceImpl<const declaring_type>(
            obj_id,
            std::is_base_of<CDependencyObject, declaring_type>());
    }
}