// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <namescope\inc\INameScopeDeferredElementCreator.h>

class CVisualStateGroupCollection;

namespace Jupiter {
    namespace VisualStateManager {
        class DeferredNameScopeEntry
            : public Jupiter::NameScoping::INameScopeDeferredElementCreator
        {
        public:
            explicit DeferredNameScopeEntry(xref::weakref_ptr<CVisualStateGroupCollection> collection);
            xref_ptr<CDependencyObject> TryGetOrCreateElement(_Out_ bool* shouldRetryLookup) final;
        private:
            xref::weakref_ptr<CVisualStateGroupCollection> m_collection;
        };
    }
}
