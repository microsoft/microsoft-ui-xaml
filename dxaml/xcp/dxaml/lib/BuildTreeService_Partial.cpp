// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BuildTreeService.g.h"
#include <UIThreadScheduler.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

#define ACTIVE 0
#define SUSPENDED 1

_Check_return_ HRESULT BuildTreeService::RegisterWork(_In_ ITreeBuilder* pTreeBuildingElement)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ITreeBuilder> spTreeBuildingElement = pTreeBuildingElement;
    ctl::WeakRefPtr wrContainer;

    IFC(ctl::AsWeak(ctl::as_iinspectable(pTreeBuildingElement), &wrContainer));

#if DBG
    {
        // let's check the validity of the registered boolean
        BOOLEAN registered = FALSE;

        IFC(pTreeBuildingElement->get_IsRegisteredForCallbacks(&registered));
        ASSERT(!registered, L"don't register when you are already registered, this is costly");

        // let's really check this
        for (unsigned i = 0; i < m_workers.size(); ++i)
        {
            auto it = std::find(begin(m_workers[i]), end(m_workers[i]), wrContainer);
            ASSERT(it == end(m_workers[i]), L"the registered boolean was not true, but we did find the worker in the list!!");
        }
    }
#endif

    m_workers[ACTIVE].push_back(wrContainer);

    IFC(pTreeBuildingElement->put_IsRegisteredForCallbacks(TRUE));

    {
        auto pCore = DXamlCore::GetCurrent()->GetHandle();
        if (pCore->HasBuildTreeWorkEvents())
        {
            IFC(pCore->SetHasBuildTreeWorksEventSignaledStatus(TRUE /*bSignaled*/));
        }

        // Request another UI thread frame to handle the new build tree work
        pCore->RequestAdditionalFrame(RequestFrameReason::BuildTreeServiceWork);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT BuildTreeService::BuildTrees(_Out_ bool* pWorkLeft)
{
    HRESULT hr = S_OK;
    // workers are in a vector. we will process a worker until our policy says otherwise.
    // that policy is decided by the worker. It is keeping its own curated list of work that it will
    // perform until it decides that enough is enough.
    // Since we do not expect multiple workers ever, and work is temporarily, we just pop from the back (fast) and
    // perform work until the worker yields. When it yields he can report whether there is more work to be done.
    // If not, we continue to the next worker, otherwise we insert the worker again, but this time at index 0 and stop
    // working. This allows another worker to get a chance next tick.
    // Summary: This service is relying on the worker to indicate when too much work has been done

    std::vector<ctl::WeakRefPtr> workersToBeSuspended;
    bool isHasBuildTreeWorksEventSignaled = false;   // read as " is'HasBuildTreeWorksEvent'Signaled "

    // 1. check the workers in active queue, try to perform BuildTree on them
    while (!m_workers[ACTIVE].empty())
    {
        ctl::ComPtr<ITreeBuilder> spWorker;
        ctl::WeakRefPtr weakWorker = m_workers[ACTIVE].back();
        BOOLEAN workerHasWorkLeft = FALSE;
        BOOLEAN workerReRegistered = FALSE;
        BOOLEAN isBuildTreeSuspended = FALSE;
        IFC(weakWorker.As<ITreeBuilder>(&spWorker));

        // note the below is not in a IFCSTL for perf reasons
        // this is believed to be safe
        m_workers[ACTIVE].pop_back();   // it's gone

        if (!spWorker)
        {
            // well, it was a weak ref, so it can be gone now.
            continue;
        }

        // check if this worker can run buildtree now.
        IFC(spWorker->IsBuildTreeSuspended(&isBuildTreeSuspended));

        if (isBuildTreeSuspended)
        {
            workersToBeSuspended.push_back(weakWorker);
            continue;
        }

        IFC(spWorker->put_IsRegisteredForCallbacks(FALSE));

        IFC(spWorker->BuildTree(&workerHasWorkLeft));

        if (workerHasWorkLeft)
        {
            // worker could have reregistered
            IFC(spWorker->get_IsRegisteredForCallbacks(&workerReRegistered));

            if (!workerReRegistered)
            {
                // returning him to the pool, but at the beginning, so that
                // another worker will get a chance in the next tick
                m_workers[ACTIVE].insert(m_workers[ACTIVE].begin(), weakWorker);
                IFC(spWorker->put_IsRegisteredForCallbacks(TRUE));
            }

            // the worker yielded but has more work to do, this is a signal for us to stop
            // note how that also limits the cost of the relatively slow insert call above.
            break;
        }
    }


    // 2. if there is no worker left in active queue, let's check if any suspended workers that not longer suspended.
    for (unsigned i = m_workers[SUSPENDED].size(); i > 0; --i)
    {
        ctl::ComPtr<ITreeBuilder> spWorker;
        ctl::WeakRefPtr weakWorker = m_workers[SUSPENDED][i - 1];
        BOOLEAN isBuildTreeSuspended = FALSE;
        IFC(weakWorker.As<ITreeBuilder>(&spWorker));

        if (!spWorker)
        {
            // well, it was a weak ref, so it can be gone now.
            m_workers[SUSPENDED].erase(m_workers[SUSPENDED].begin() + i - 1);
            continue;
        }

        IFC(spWorker->IsBuildTreeSuspended(&isBuildTreeSuspended));

        if (isBuildTreeSuspended)
        {
            continue;
        }

        // remove it from suspended queue and put it back to active queue.
        m_workers[SUSPENDED].erase(m_workers[SUSPENDED].begin() + i - 1);
        // put it at the end so it will be picked up first in next tick.
        m_workers[ACTIVE].push_back(weakWorker);

        // once we move a suspended worker back to active queue, we should tell
        // the test framework that we are not in idle state.
        if (!isHasBuildTreeWorksEventSignaled)
        {
            auto pCore = DXamlCore::GetCurrent()->GetHandle();
            if (pCore->HasBuildTreeWorkEvents())
            {
                IFC(pCore->SetHasBuildTreeWorksEventSignaledStatus(TRUE /*bSignaled*/));
            }
            isHasBuildTreeWorksEventSignaled = true;
        }

    }

    // 3. move all workers that are suspended in this tick to the suspended queue.
    for (auto& worker : workersToBeSuspended)
    {
        m_workers[SUSPENDED].push_back(worker);
    }

    // requests additional UI tick only when active queue is not empty.
    *pWorkLeft = !m_workers[ACTIVE].empty();
    if (!*pWorkLeft)
    {
        auto pCore = DXamlCore::GetCurrent()->GetHandle();
        if (pCore->HasBuildTreeWorkEvents())
        {
            IFC(pCore->SetHasBuildTreeWorksEventSignaledStatus(FALSE /*bSingaled*/));
            IFC(pCore->SetBuildTreeServiceDrainedEvent());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT BuildTreeService::ClearWork()
{
    HRESULT hr = S_OK;

    for (unsigned i = 0; i < m_workers.size(); ++i)
    {
        while (!m_workers[i].empty())
        {
            ctl::ComPtr<ITreeBuilder> spWorker;
            ctl::WeakRefPtr weakWorker = m_workers[i].back();
            IFC(weakWorker.As<ITreeBuilder>(&spWorker));

            // note the below is not in a IFCSTL for perf reasons
            // this is believed to be safe
            m_workers[i].pop_back();   // it's gone

            if (spWorker)
            {
                IFC(spWorker->put_IsRegisteredForCallbacks(FALSE));

                IFC(spWorker->ShutDownDeferredWork());
            }
        }
    }


Cleanup:
    RRETURN(hr);
}
