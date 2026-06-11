// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TrackerPtr.h"

namespace DirectUI
{

    //
    // To avoid completely re-implementing tracker points in MUXP, we create them in MUX
    // and provide this wrapper so that they can be used in MUXP.
    //
    class TrackerPtrWrapper
        : public xaml_hosting::ITrackerPtrWrapper
    {
    private:
        TrackerPtr<IUnknown> m_ptr;

    public:
        _Check_return_ HRESULT Set(_In_ IUnknown* pValue) override
        {
            m_ptr.Set(pValue);
            return S_OK;
        }

        IUnknown* Get() override
        {
            return m_ptr.Get();
        }

        bool TryGetSafeReference(_Outptr_result_maybenull_ IUnknown** ppValue) override
        {
            bool succeeded = false;
            ctl::ComPtr<IUnknown> spObject;

            *ppValue = nullptr;
            succeeded = m_ptr.TryGetSafeReference(&spObject);

            if (succeeded)
            {
                *ppValue = spObject.Detach();
            }

            return succeeded;
        }

        void ReferenceTrackerWalk(_In_ INT walkType) override
        {
            m_ptr.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
        }
    };
    
}
