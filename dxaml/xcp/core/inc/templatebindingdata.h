// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XcpList.h>

class CDependencyProperty;

struct TemplateBindingData
{
    TemplateBindingData()
        : m_pTemplateBindings(nullptr)
    {}

    ~TemplateBindingData()
    {
        if (m_pTemplateBindings != NULL)
        {
            // Empty the list but delete nothing
            m_pTemplateBindings->Clean(FALSE);
            delete m_pTemplateBindings;
        }
    }

    // We use a WeakRef to the TemplatedParent because it is not feasible to clean all the nested children when the parent is released.
    xref::weakref_ptr<CDependencyObject> m_pTemplatedParentWeakRef;

    // This list contains the properties that have been template bound
    CXcpList<const CDependencyProperty> *m_pTemplateBindings;
};