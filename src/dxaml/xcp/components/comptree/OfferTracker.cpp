// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OfferTracker.h"
#include <dcompinternal.h>
#include <dcompprivate.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

OfferTracker::UnofferRevoker::UnofferRevoker(OfferTracker &offerTracker, _In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory)
    : m_offerTracker(offerTracker)
    , m_surfaceFactory(surfaceFactory)
{}

OfferTracker::UnofferRevoker::~UnofferRevoker()
{
    // TODO: Handle DeviceLost error that may occur here
    // Bug 12111763: DeviceLost cannot always be handled reliably for LoadedImageSurface
    IFCFAILFAST(m_surfaceFactory->OfferSurfaceResources());

    //push the current surfacefactory pointer into the
    //offered list
    m_offerTracker.offeredSurfaceFactories.push_back(m_surfaceFactory);

    m_offerTracker.m_csRevoker.reset();
}

_Check_return_ HRESULT OfferTracker::Unoffer(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory, _Out_ std::unique_ptr<UnofferRevoker> *unofferRevoker)
{
    auto lock = m_cs.lock();

    ASSERT(m_offered);

    //we first check whether the surfaceFactory to be offered is
    //in the offered list, if not, we just return
    std::vector<IDCompositionSurfaceFactoryPartner3*>::iterator positionInList = FindOfferedSurfaceFactory(surfaceFactory);

    if (positionInList == offeredSurfaceFactories.end())
    {
        return S_OK;
    }

    BOOL discarded = false;
    IFC_RETURN(surfaceFactory->ReclaimSurfaceResources(&discarded));

    m_discarded = m_discarded || discarded;

    //delete the reclaimed surfacefactory in the offered list
    offeredSurfaceFactories.erase(positionInList);

    unofferRevoker->reset(new UnofferRevoker(*this, surfaceFactory));

    // Keep locked on success
    std::swap(m_csRevoker, lock);
    return S_OK;
}

//Wrapper method to iterate through the SurfaceFactory vector, the vector contains all the SurfaceFactories
//(main and secondary ones), and it is provided by DCompTreeHost
HRESULT OfferTracker::OfferResources(_In_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector)
{
    auto guard = m_cs.lock();

    ASSERT(offeredSurfaceFactories.size() == 0);

    int cSurfaceFactory = surfaceFactoryVector->size();
    for (int iSurfaceFactory = 0; iSurfaceFactory < cSurfaceFactory; iSurfaceFactory++)
    {
        IDCompositionSurfaceFactoryPartner3 *pSurfaceFactoryPartner = surfaceFactoryVector->at(iSurfaceFactory);
        HRESULT hr = OfferSurfaceFactory(pSurfaceFactoryPartner);

        if (hr != DCOMPOSITION_ERROR_SURFACE_BEING_RENDERED)
        {
            IFC_RETURN(hr);
        }
    }

    return S_OK;
}

// Helper to perform the offer while holding the lock
HRESULT OfferTracker::OfferSurfaceFactory(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory)
{
    IFC_RETURN(surfaceFactory->OfferSurfaceResources());

    //push the current surfacefactory pointer into the
    //offered list
    offeredSurfaceFactories.push_back(surfaceFactory);

    m_offered = true;

    return S_OK;
}

//Wrapper method to iterate through the offeredSurfaceFactory, all the SurfaceFactories
//stored in the collection should be reclaimed
HRESULT OfferTracker::ReclaimResources(_Out_ BOOL* discarded)
{
    auto guard = m_cs.lock();

    std::vector<IDCompositionSurfaceFactoryPartner3*>::const_iterator cSurfaceFactory;
    for (cSurfaceFactory = offeredSurfaceFactories.begin(); cSurfaceFactory != offeredSurfaceFactories.end(); cSurfaceFactory++)
    {
        BOOL discardedNow = false;
        //we treat errors as lost pixels because we need to preserve compatibility with the previous
        //behavior on IDCompositionDesktopDevicePartner4::ReclaimSurfaceResources().
        if (SUCCEEDED(ReclaimSurfaceFactory(*cSurfaceFactory, &discardedNow)))
        {
            *discarded |= discardedNow;
        }
        else
        {
            *discarded = true;
        }
    }

    *discarded |= m_discarded;

    m_discarded = false;
    m_offered = false;

    //clear the offeredSurfaceFactories list
    offeredSurfaceFactories.clear();

    return S_OK;
}

// Helper to perform the reclaim while holding the lock
HRESULT OfferTracker::ReclaimSurfaceFactory(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory, _Out_ BOOL* discarded)
{
    IFC_RETURN(surfaceFactory->ReclaimSurfaceResources(discarded));

    return S_OK;
}

bool OfferTracker::IsOffered()
{
    auto guard = m_cs.lock();

    return m_offered;
}

void OfferTracker::Reset()
{
    auto guard = m_cs.lock();

    m_offered = false;
    m_discarded = false;

    offeredSurfaceFactories.clear();
}


//Helpper method to preform a search to the spicified SF in the offered SF list
std::vector<IDCompositionSurfaceFactoryPartner3*>::iterator OfferTracker::FindOfferedSurfaceFactory(IDCompositionSurfaceFactoryPartner3* surfaceFactory)
{
    return std::find(offeredSurfaceFactories.begin(), offeredSurfaceFactories.end(), surfaceFactory);
}

//Helper function to delete the input SF in the offered SurfaceFactory list, usually called when resources is
//released
void OfferTracker::DeleteReleasedSurfaceFactoryFromList(IDCompositionSurfaceFactoryPartner3* surfaceFactory)
{
    std::vector<IDCompositionSurfaceFactoryPartner3*>::iterator Iter = FindOfferedSurfaceFactory(surfaceFactory);

    if (Iter != offeredSurfaceFactories.end()) {
        offeredSurfaceFactories.erase(Iter);
    }
}


