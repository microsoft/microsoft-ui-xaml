// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TemplateNameScopeTable.h>

#include <CDependencyObject.h>
#include <TemplateContent.h>
#include "NameScopeTableEntry.h"

namespace Jupiter {
    namespace NameScoping {

        TemplateNameScopeTable::TemplateNameScopeTable(xref_ptr<CTemplateContent> nameSource)
            : m_nameSource(std::move(nameSource))
        {}

        TemplateNameScopeTable::~TemplateNameScopeTable()
        {}

        xref_ptr<CDependencyObject> TemplateNameScopeTable::TryGetElementImpl(const xstring_ptr_view& name, _Out_ bool* shouldRetry)
        {
            size_t idx = m_nameSource->TryGetNameIndex(name);
            if (idx != static_cast<size_t>(-1) && m_entries.size() > idx)
            {
                return m_entries[idx].TryGetElement(shouldRetry);
            }
            else
            {
                return xref_ptr<CDependencyObject>();
            }
        }

        void TemplateNameScopeTable::UnregisterName(const xstring_ptr_view& name)
        {
            size_t idx = m_nameSource->TryGetNameIndex(name);
            if (idx != static_cast<size_t>(-1) && m_entries.size() > idx)
            {
                m_entries[idx] = NameScopeTableEntry();
            }
        }

        void TemplateNameScopeTable::RegisterNameImpl(const xstring_ptr_view& name, NameScopeTableEntry&& entry)
        {
            size_t idx = m_nameSource->GetOrCreateNameIndex(name);
            if (idx == m_entries.size())
            {
                m_entries.push_back(std::move(entry));
            }
            else if (idx > m_entries.size())
            {
                m_entries.resize(idx + 1);
                m_entries[idx] = std::move(entry);
            }
            else
            {
                m_entries[idx] = std::move(entry);
            }
        }
    }
}