// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <StandardNameScopeTable.h>

#include <CDependencyObject.h>
#include "NameScopeTableEntry.h"

namespace Jupiter {
    namespace NameScoping {
        xref_ptr<CDependencyObject> StandardNameScopeTable::TryGetElementImpl(const xstring_ptr_view& name, _Out_ bool* shouldRetry)
        {
            auto entry = m_entries.find(name);
            if (entry != m_entries.end())
            {
                return entry->second.TryGetElement(shouldRetry);
            }
            else
            {
                return nullptr;
            }
        }

        void StandardNameScopeTable::UnregisterName(const xstring_ptr_view& name)
        {
            auto entry = m_entries.find(name);
            if (entry != m_entries.end())
            {
                m_entries.erase(entry);
            }
        }

        void StandardNameScopeTable::RegisterNameImpl(const xstring_ptr_view& name, NameScopeTableEntry&& entry)
        {
            // Avoid doing the key search twice by using lower_bound instead of find, and reusing the iterator as a hint if needed
            auto iter = m_entries.lower_bound(name);
            if (iter != m_entries.end() && iter->first == name)
            {
                iter->second = std::move(entry);
            }
            else
            {
                xstring_ptr promotedName;
                VERIFYHR(name.Promote(&promotedName));
                m_entries.emplace_hint(iter, std::move(promotedName), std::move(entry));
            }
        }

        /* static */ void StandardNameScopeTable::MoveEntries(StandardNameScopeTable* pSource, StandardNameScopeTable* pDestination)
        {
            for (auto entry : pSource->m_entries)
            {
                pDestination->RegisterNameImpl(entry.first, std::move(entry.second));
            }
            pSource->m_entries.clear();
        }
    }
}