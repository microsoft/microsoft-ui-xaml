// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DeferredNameScopeEntry.h>
#include <VisualStateGroupCollection.h>
#include <corep.h>
#include <FxCallbacks.h>

namespace Jupiter {
    namespace VisualStateManager {
        DeferredNameScopeEntry::DeferredNameScopeEntry(xref::weakref_ptr<CVisualStateGroupCollection> collection)
            : m_collection(collection)
        {}

        xref_ptr<CDependencyObject> DeferredNameScopeEntry::TryGetOrCreateElement(_Out_ bool* shouldRetryLookup)
        {
            auto collection = m_collection.lock();
            *shouldRetryLookup = false;
            if (collection && collection->IsOptimizedGroupCollection())
            {
                bool isDXamlCoreShuttingDown = false;
                IFCFAILFAST(FxCallbacks::FrameworkCallbacks_IsDXamlCoreShuttingDown(&isDXamlCoreShuttingDown));

                // If the Xaml view is being closed and DXamlCore is being shutdown,
                // do not create the entire VisualStateCollection so DeferredElement 
                // is added to the namescope, only to look up the deferred element in 
                // that namescope in order to destroy it.  'shouldRetryLookup' should
                // remain 'false' so this call isn't repeated.
                
                // If the core is not shutting down, 'fault in' the VisualStateCollection
                // so this element is realized and available in the namescope for lookup.
                if(!isDXamlCoreShuttingDown)
                {
                    IFCFAILFAST(collection->EnsureFaultedIn());
                    *shouldRetryLookup = true;
                }
            }
            return nullptr;
        }
    }
}
