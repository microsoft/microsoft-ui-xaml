// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;

namespace Jupiter {
    namespace NameScoping {
        class NameScopeTableEntry;
        class INameScopeDeferredElementCreator;

        class NameScopeTable
        {
        public:
            // Registers a name in the NameScope table, overriding any previous entry values if
            // present.
            void RegisterName(const xstring_ptr_view& name, xref_ptr<CDependencyObject> element);
            void RegisterName(const xstring_ptr_view& name, xref::weakref_ptr<CDependencyObject> weakElement);
            void RegisterName(const xstring_ptr_view& name, std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator);

            virtual void UnregisterName(const xstring_ptr_view& name) = 0;

            xref_ptr<CDependencyObject> TryGetElement(const xstring_ptr_view& name);

            virtual ~NameScopeTable() {}

        private:
            virtual xref_ptr<CDependencyObject> TryGetElementImpl(const xstring_ptr_view& name, _Out_ bool* shouldRetry) = 0;
            virtual void RegisterNameImpl(const xstring_ptr_view& name, NameScopeTableEntry&& entry) = 0;
        };
    }
}