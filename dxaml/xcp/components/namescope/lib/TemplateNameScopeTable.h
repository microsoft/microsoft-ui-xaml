// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NameScopeTable.h>
#include <xref_ptr.h>
#include "NameScopeTableEntry.h"

class CTemplateContent;

namespace Jupiter {
    namespace NameScoping {
        // A specialized version of the NameScope table for use with control templates.
        // Names are stored in a shared vector available to the template and instances
        // only store an index-based vector of elements.
        class TemplateNameScopeTable
            : public NameScopeTable
        {
        public:
            explicit TemplateNameScopeTable(xref_ptr<CTemplateContent> nameSource);
            ~TemplateNameScopeTable();

            void UnregisterName(const xstring_ptr_view& name) final;

        private:
            xref_ptr<CDependencyObject> TryGetElementImpl(const xstring_ptr_view& strName, _Out_ bool* shouldRetry) final;
            void RegisterNameImpl(const xstring_ptr_view& name, NameScopeTableEntry&& entry) final;

            std::vector<NameScopeTableEntry> m_entries;
            xref_ptr<CTemplateContent> m_nameSource;
        };
    }
}