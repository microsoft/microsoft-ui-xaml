// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComObject.h>
#include <WeakReferenceSource.h>
#include <TrackerPtr.h>

namespace DirectUI
{
    class WuxTracker
        : public ctl::WeakReferenceSource
    {
    public:

        void OnReferenceTrackerWalk(INT walkType) override { m_gotWalked = true; __super::OnReferenceTrackerWalk(walkType); }

        HRESULT SetTrackerReference(IInspectable* value) { SetPtrValue(m_trackerReference, value); return S_OK; }
        bool TryGetSafeReference(const ctl::Internal::ComPtrRef<ctl::ComPtr<IInspectable>>& safeReference) const
        {
            return m_trackerReference.TryGetSafeReference(safeReference);
        }

        bool GotWalked() const { return m_gotWalked; }
        void ResetGotWalked() { m_gotWalked = false; }

    private:
        TrackerPtr<IInspectable> m_trackerReference;
        bool m_gotWalked = false;
    };

    // Represents a CCW.
    class TrackerTarget
        : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
            IReferenceTrackerTarget>
    {
    public:

        // IReferenceTrackerTarget
        ULONG STDMETHODCALLTYPE AddRefFromReferenceTracker() override { return AddRef(); }
        ULONG STDMETHODCALLTYPE ReleaseFromReferenceTracker() override { return Release(); }
        HRESULT STDMETHODCALLTYPE Peg() override { ++m_pegCount; return S_OK; }
        HRESULT STDMETHODCALLTYPE Unpeg() override { ++m_unpegCount; return S_OK; }

        unsigned GetPegCount() const { return m_pegCount; }
        unsigned GetUnpegCount() const { return m_unpegCount; }

    private:
        unsigned m_pegCount = 0;
        unsigned m_unpegCount = 0;
    };

    // Represents a CCW that also implements IReferenceTrackerInternal.
    class ReferenceTrackerTarget
        : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        IReferenceTrackerTarget,
        Microsoft::WRL::ComposableBase<IInspectable >>
    {
    public:

        _Check_return_ HRESULT RuntimeClassInitialize()
        {
            ctl::ComPtr<WuxTracker> inner;
            IFC_RETURN(ctl::ComObject<WuxTracker>::CreateInstance(this, inner.GetAddressOf()));

            Microsoft::WRL::ComPtr<IInspectable> nonDelegatingInner;
            nonDelegatingInner.Attach(reinterpret_cast<IInspectable*>(static_cast<ctl::ComBase*>(inner.Get())->AsNonDelegatingInspectable()));
            IFC_RETURN(SetComposableBasePointers(nonDelegatingInner.Get()));

            // Don't release the inner object's reference, or else the object will get destroyed.
            m_inner = inner.Detach();

            return S_OK;
        }

        // IReferenceTrackerTarget
        ULONG STDMETHODCALLTYPE AddRefFromReferenceTracker() override { return AddRef(); }
        ULONG STDMETHODCALLTYPE ReleaseFromReferenceTracker() override { return Release(); }
        HRESULT STDMETHODCALLTYPE Peg() override { ++m_pegCount; return S_OK; }
        HRESULT STDMETHODCALLTYPE Unpeg() override { ++m_unpegCount; return S_OK; }

        ctl::ComPtr<WuxTracker> GetInner() const { return m_inner; }
        unsigned GetPegCount() const { return m_pegCount; }
        unsigned GetUnpegCount() const { return m_unpegCount; }

    private:
        unsigned m_pegCount = 0;
        unsigned m_unpegCount = 0;
        WuxTracker* m_inner;
    };

}