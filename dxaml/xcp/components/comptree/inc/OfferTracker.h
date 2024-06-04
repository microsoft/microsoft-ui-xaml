// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <memory>

struct IDCompositionSurfaceFactoryPartner3;

// Simple class to track Offer/Reclaim in a thread-safe manner.
// One instance of this class will be created and shared between objects.
// The lifetime of this object is managed completely by the UI thread, but the
// writing/reading of offered state can be done across threads.
class OfferTracker : public CXcpObjectBase<IObject>
{
public:
    OfferTracker() = default;
    
    // Non-copyable
    OfferTracker(const OfferTracker&) = delete;
    OfferTracker& operator=(const OfferTracker&) = delete;

    class UnofferRevoker
    {
    public:
        ~UnofferRevoker();

    private:
        friend class OfferTracker;
        UnofferRevoker(OfferTracker &offerTracker, _In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory);
        OfferTracker &m_offerTracker;
        IDCompositionSurfaceFactoryPartner3 *m_surfaceFactory;
    };

    _Check_return_ HRESULT Unoffer(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory, _Out_ std::unique_ptr<UnofferRevoker> *unofferRevoker);

    _Check_return_ HRESULT OfferResources(_In_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector);
    _Check_return_ HRESULT OfferSurfaceFactory(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory);

    _Check_return_ HRESULT ReclaimResources(_Out_ BOOL* discarded);
    _Check_return_ HRESULT ReclaimSurfaceFactory(_In_ IDCompositionSurfaceFactoryPartner3* surfaceFactory, _Out_ BOOL* discarded);
    bool IsOffered();
    void Reset();
    std::vector<IDCompositionSurfaceFactoryPartner3*>::iterator FindOfferedSurfaceFactory(IDCompositionSurfaceFactoryPartner3* surfaceFactory);
    void DeleteReleasedSurfaceFactoryFromList(IDCompositionSurfaceFactoryPartner3* surfaceFactory);
    int getNumOfOfferedSurfaceFactories() { return static_cast<int>(offeredSurfaceFactories.size()); }

private:
    wil::critical_section m_cs;
    wil::cs_leave_scope_exit m_csRevoker;
    BOOL m_discarded = false;
    BOOL m_offered = false;
    std::vector<IDCompositionSurfaceFactoryPartner3*> offeredSurfaceFactories;
};

