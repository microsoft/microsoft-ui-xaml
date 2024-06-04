// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NameScopeTable.h>
#include <NameScopeTableEntry.h>

#include <CDependencyObject.h>

namespace Jupiter {
    namespace NameScoping {
        void NameScopeTable::RegisterName(const xstring_ptr_view& name, xref_ptr<CDependencyObject> element)
        {
            RegisterNameImpl(name, NameScopeTableEntry(element));
        }
        
        void NameScopeTable::RegisterName(const xstring_ptr_view& name, xref::weakref_ptr<CDependencyObject> weakElement)
        {
            RegisterNameImpl(name, NameScopeTableEntry(weakElement));
        }

        void NameScopeTable::RegisterName(const xstring_ptr_view& name, std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator)
        {
            RegisterNameImpl(name, NameScopeTableEntry(deferredCreator));
        }

        xref_ptr<CDependencyObject> NameScopeTable::TryGetElement(const xstring_ptr_view& name)
        {
            // "\0" is an invalid element name.
            if (name.IsNullOrEmpty()) return nullptr;

            bool shouldRetry = false;
            do
            {
                // Note that there's some reentrancy here that one should be mindful of- the act of asking for
                // an element in the deferred case can cause elements to be faulted in, registering themselves
                // with the NameScopeTable.
                auto result = TryGetElementImpl(name, &shouldRetry);
                if (result) 
                {
                    ASSERT(!shouldRetry);
                    return result;
                }
            } 
            while (shouldRetry);

            return nullptr;
        }
    }
}