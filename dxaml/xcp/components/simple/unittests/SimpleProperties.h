// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Include all dependencies which are needed for property traits here.

#include "SomeClass.h"

// 

#include "sp_ids.h"
#include <SimpleProperty.h>
#include "SimplePropertiesMetadata.g.h"

namespace SimpleProperty
{
    template <typename declaring_type>
    declaring_type* MapObjIdToInstance(objid_t obj_id)
    {
        return reinterpret_cast<declaring_type*>(obj_id);
    }

    template <typename declaring_type>
    const declaring_type* MapObjIdToInstance(const_objid_t obj_id)
    {
        return reinterpret_cast<const declaring_type*>(obj_id);
    }
}