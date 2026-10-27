// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GraphicsResourceDiagnostics.h>
#include <GraphicsResourceCounters.h>

namespace GraphicsResourceDiagnostics
{
    HRESULT GetCounterCount(_Out_ UINT32* count)
    {
        IFCPTR_RETURN(count);
        *count = GraphicsResourceCounters::ResourceKindCount;
        return S_OK;
    }

    HRESULT GetCounterSnapshot(UINT32 index, _Out_ XamlGraphicsResourceCounterSnapshot* snapshot)
    {
        IFCPTR_RETURN(snapshot);
        *snapshot = {};
        LARGE_INTEGER start{};
        IFCW32_RETURN(QueryPerformanceCounter(&start));

        GraphicsResourceCounters::Snapshot counters{};
        if (!GraphicsResourceCounters::TryGetSnapshot(index, counters))
        {
            return E_INVALIDARG;
        }

        snapshot->Size = sizeof(*snapshot);
        snapshot->Version = 1;
        IFC_RETURN(StringCchCopyW(snapshot->Name, ARRAYSIZE(snapshot->Name), counters.name));
        if (!counters.enabled)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        snapshot->Flags = XamlGraphicsResourceCounter_CreationCountAvailable
            | XamlGraphicsResourceCounter_WinUICoreObservedScope
            | XamlGraphicsResourceCounter_NonTransactional;
        if (counters.hasLifetime)
        {
            snapshot->Flags |= XamlGraphicsResourceCounter_WrapperLifetimeAvailable;
        }
        snapshot->TotalCreated = counters.created;
        snapshot->TotalDestroyed = counters.destroyed;
        snapshot->LiveCount = counters.live;
        LARGE_INTEGER end{};
        IFCW32_RETURN(QueryPerformanceCounter(&end));
        snapshot->ReadStartQpc = start.QuadPart;
        snapshot->ReadEndQpc = end.QuadPart;
        return S_OK;
    }
}
