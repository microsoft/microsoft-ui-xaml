// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a service to build trees incrementally. Currently scoped to listviewbase

#pragma once

#include "BuildTreeService.g.h"

namespace DirectUI
{
    interface ITreeBuilder;

    PARTIAL_CLASS(BuildTreeService)
    {
    public:
        // registers a worker with the service. The worker is unregister when she returns false after being called
        // to perform work
        _Check_return_ HRESULT RegisterWork(_In_ ITreeBuilder* pTreeBuildingElement);
        
        // perform work
        _Check_return_ HRESULT BuildTrees(_Out_ bool* pWorkLeft);

        // clear work (on shutdown)
        _Check_return_ HRESULT ClearWork();
        
        // returns the number of active workers
        int ActiveWorkersCount() const;

    private:
        // stores everyone that likes to be called
        // first queue: active workers,
        // second queue: suspended workers.
        std::array<std::vector<ctl::WeakRefPtr>, 2> m_workers;
    };
}
