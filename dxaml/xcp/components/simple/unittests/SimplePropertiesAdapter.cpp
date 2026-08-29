// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SimpleProperties.h"
#include <SimplePropertyImpl.h>
#include "SimplePropertiesAdapter.g.h"

SimpleProperty::sparsetables s_sparsetables;
SimpleProperty::changehandlerstables s_changehandlerstables;

namespace SimpleProperty
{
    template <typename declaring_type>
    sparsetables& GetSparseTablesForType()
    {
        return s_sparsetables;
    }
    
    template <typename declaring_type>
    sparsetables& GetSparseTablesForType(const_objid_t obj_id)
    {
        return s_sparsetables;
    }
    
    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType()
    {
        return s_changehandlerstables;
    }
    
    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType(const_objid_t obj_id)
    {
        return s_changehandlerstables;
    }

    size_t TranslatePIDToIndex(PID pid)
    {
        return static_cast<size_t>(pid);
    }
}