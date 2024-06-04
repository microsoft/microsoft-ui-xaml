// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NameScopeTable.h>
#include "NameScopeTableEntry.h"
#include <vector_map.h>

namespace Jupiter {
    namespace NameScoping {
        // The standard template namescope table implementation, for use with UserControls and
        // instances of types that aren't backed by a contorl template.
        class StandardNameScopeTable
            : public NameScopeTable
        {
        public:
            void UnregisterName(const xstring_ptr_view& name) final;

            static void MoveEntries(StandardNameScopeTable* pSource, StandardNameScopeTable* pDestination);

        private:
            void RegisterNameImpl(const xstring_ptr_view& name, _In_ NameScopeTableEntry&& entry) final;
            xref_ptr<CDependencyObject> TryGetElementImpl(const xstring_ptr_view& name, _Out_ bool* shouldRetry) final;

            containers::vector_map<xstring_ptr, NameScopeTableEntry> m_entries;
        };
    }
}