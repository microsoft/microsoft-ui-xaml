// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CVisualState;

// Used for tracking a VisualState through multiple deferral states (optimized, faulted-in, legacy)
// There are three deferral states that VisualState may be in:
// 1) optimized: may be referenced by VisualState index
// 2) faulted-in: any VisualState indexes held before fault-in are now invalid, may be referenced by the CVisualState DO instance 
// 3) legacy: may be referenced by CVisualState DO instance. No deferred objects (essences) are ever stored or created;
// indexes are never valid. (The legacy path is only used by XamlReader.Load.)
// To hold a reference to VisualState through its lifecycle, create a token using the deferred VisualState index value,
// or get a token from a CVisualState instance with GetVisualStateToken(). Resolve this token with VisualStateManagerDataSource.TryGetVisualState()
// or pass the token to CVisualStateManager2::GoToStateOptimized directly.  
class VisualStateToken
{
    public:
        VisualStateToken();
        VisualStateToken(size_t tokenValue);
        VisualStateToken(_In_ const CVisualState* tokenValue);
        bool operator==(const VisualStateToken& t);
        bool operator!=(const VisualStateToken& t);
        bool IsEmpty() const; 

    private:
        size_t m_value;
};
