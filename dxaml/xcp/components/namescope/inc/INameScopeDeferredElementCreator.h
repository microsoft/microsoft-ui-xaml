// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Jupiter { 
    namespace NameScoping {
        // This is an interface to be implemented by any class that defers the creation of
        // elements but wishes for those elements to be able to partipate in FindName lookups.
        class INameScopeDeferredElementCreator
        {
        public:
            // The deferred element creator interface supports two modes of operation- either it will return
            // the instance directly or it will do some amount of work that will cause the required
            // elements to be created and registered in the same namescope table. In the second case
            // we return null and set shouldRetry to true to indicate we should perform the lookup
            // again.
            virtual xref_ptr<CDependencyObject> TryGetOrCreateElement(_Out_ bool* shouldRetryLookup) = 0;
            virtual ~INameScopeDeferredElementCreator() {}
        };
    } 
}