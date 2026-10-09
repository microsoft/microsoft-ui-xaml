// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <TableViewTelemetry.h>
#include <TableViewTelemetryRegistration.h>
#include <WexTestClass.h>
#include <memory>

namespace
{
    struct PublisherState
    {
        ::IInspectable* publisher{};
        uint32_t removals{};
        uint32_t deadResolves{};
        int64_t lastToken{};
        bool failResolve{};
    };

    class WeakPublisherReference final : public ::IWeakReference
    {
    public:
        explicit WeakPublisherReference(std::shared_ptr<PublisherState> state) : m_state(std::move(state)) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
        {
            *value = nullptr;
            if (iid != __uuidof(::IUnknown) && iid != __uuidof(::IWeakReference)) { return E_NOINTERFACE; }
            *value = static_cast<::IWeakReference*>(this);
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            auto const refs = --m_refs;
            if (!refs) { delete this; }
            return refs;
        }
        HRESULT STDMETHODCALLTYPE Resolve(REFIID iid, ::IInspectable** value) override
        {
            *value = nullptr;
            if (m_state->failResolve) { return E_FAIL; }
            if (!m_state->publisher) { ++m_state->deadResolves; return S_OK; }
            return m_state->publisher->QueryInterface(iid, reinterpret_cast<void**>(value));
        }
    private:
        ULONG m_refs{ 1 };
        std::shared_ptr<PublisherState> m_state;
    };

    class Publisher final : public ::IInspectable, public ::IWeakReferenceSource
    {
    public:
        explicit Publisher(std::shared_ptr<PublisherState> state) : state(std::move(state)) { this->state->publisher = this; }
        ~Publisher() { state->publisher = nullptr; }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
        {
            *value = nullptr;
            if (iid == __uuidof(::IUnknown) || iid == __uuidof(::IInspectable)) { *value = static_cast<::IInspectable*>(this); }
            else if (iid == __uuidof(::IWeakReferenceSource)) { *value = static_cast<::IWeakReferenceSource*>(this); }
            else { return E_NOINTERFACE; }
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            auto const result = --refs;
            if (!result) { delete this; }
            return result;
        }
        HRESULT STDMETHODCALLTYPE GetIids(ULONG* count, IID** values) override { *count = 0; *values = nullptr; return S_OK; }
        HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING* value) override { *value = nullptr; return S_OK; }
        HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel* value) override { *value = BaseTrust; return S_OK; }
        HRESULT STDMETHODCALLTYPE GetWeakReference(::IWeakReference** value) override
        {
            *value = new WeakPublisherReference(state);
            return S_OK;
        }
        static HRESULT Remove(::IInspectable* publisher, ::IInspectable*, int64_t token) noexcept
        {
            auto const self = static_cast<Publisher*>(publisher);
            ++self->state->removals;
            self->state->lastToken = token;
            return S_OK;
        }
        ULONG refs{ 1 };
        std::shared_ptr<PublisherState> state;
    };

    struct InitialObserverOwner
    {
        uint32_t completions{};
        TableViewTelemetry::Registration registration;
        static void Completed(void* context) noexcept
        {
            auto& owner = *static_cast<InitialObserverOwner*>(context);
            ++owner.completions;
            owner.registration.Reset();
        }
    };
}

class TableViewTelemetryRegistration_UnitTests
{
public:
    TEST_CLASS(TableViewTelemetryRegistration_UnitTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        TestControlReporting(TestReportingRequest::Suspend);
#endif
        return true;
    }
    TEST_CLASS_CLEANUP(ClassCleanup)
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        TestControlReporting(TestReportingRequest::Resume);
#endif
        return true;
    }

    TEST_METHOD(WeakAbiRegistrationRemovesOnceWithoutRetainingPublisher)
    {
        auto state = std::make_shared<PublisherState>();
        auto publisher = new Publisher(state);
        {
            TableViewTelemetry::Registration registration;
            VERIFY_IS_TRUE(registration.Prepare(static_cast<::IInspectable*>(publisher), __uuidof(::IInspectable), nullptr, Publisher::Remove));
            VERIFY_ARE_EQUAL(1ul, publisher->refs);
            registration.Arm(0);
            TableViewTelemetry::Registration moved{ std::move(registration) };
            VERIFY_IS_FALSE(registration.IsArmed());
            registration.Reset();
            VERIFY_ARE_EQUAL(0u, state->removals);
            moved.Reset();
            moved.Reset();
            VERIFY_ARE_EQUAL(1u, state->removals);
            VERIFY_ARE_EQUAL(int64_t{ 0 }, state->lastToken);
        }
        {
            TableViewTelemetry::Registration destructorOnly;
            VERIFY_IS_TRUE(destructorOnly.Prepare(static_cast<::IInspectable*>(publisher), __uuidof(::IInspectable), nullptr, Publisher::Remove));
            destructorOnly.Arm(17);
        }
        VERIFY_ARE_EQUAL(2u, state->removals);
        VERIFY_ARE_EQUAL(int64_t{ 17 }, state->lastToken);
        publisher->Release();
    }

    TEST_METHOD(DeadAndUnavailableWeakPublishersDoNotEscapeTeardown)
    {
        auto dead = std::make_shared<PublisherState>();
        auto publisher = new Publisher(dead);
        TableViewTelemetry::Registration registration;
        VERIFY_IS_TRUE(registration.Prepare(static_cast<::IInspectable*>(publisher), __uuidof(::IInspectable), nullptr, Publisher::Remove));
        registration.Arm(23);
        publisher->Release();
        registration.Reset();
        VERIFY_ARE_EQUAL(1u, dead->deadResolves);
        VERIFY_ARE_EQUAL(0u, dead->removals);
        VERIFY_IS_FALSE(registration.IsArmed());

        auto failed = std::make_shared<PublisherState>();
        publisher = new Publisher(failed);
        VERIFY_IS_TRUE(registration.Prepare(static_cast<::IInspectable*>(publisher), __uuidof(::IInspectable), nullptr, Publisher::Remove));
        registration.Arm(29);
        failed->failResolve = true;
        registration.Reset();
        VERIFY_ARE_EQUAL(0u, failed->removals);
        VERIFY_IS_FALSE(registration.IsArmed());
        publisher->Release();
    }

    TEST_METHOD(EveryInitialTerminalSynchronouslyRevokesOwnedRegistration)
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        for (uint32_t terminal = 0; terminal != 10; ++terminal)
        {
            auto observed = std::make_shared<PublisherState>();
            auto publisher = new Publisher(observed);
            InitialObserverOwner owner;
            VERIFY_IS_TRUE(owner.registration.Prepare(static_cast<::IInspectable*>(publisher), __uuidof(::IInspectable), nullptr, Publisher::Remove));
            owner.registration.Arm(terminal);
            TableViewTelemetry::State state;
            state.initialCompleted = InitialObserverOwner::Completed;
            state.initialCompletedContext = &owner;
            TableViewTelemetry::BeginSession(state);
            TableViewTelemetry::BeginInitial(state, TableViewTelemetry::Origin::Loaded);
            VERIFY_IS_TRUE(state.initialTipPending);
            using namespace TableViewTelemetry;
            switch (terminal)
            {
            case 0: CompleteInitial(state, Result::Success); break;
            case 1: CompleteInitial(state, Result::Failure, Stage::Template, E_FAIL); break;
            case 2: IgnoreInitial(state, IgnoreReason::ApplicationFault, Stage::Source, E_FAIL, AppFaultKind::Callback); break;
            case 3: IgnoreInitial(state, IgnoreReason::UnattributedFault, Stage::Layout, E_FAIL); break;
            case 4: IgnoreInitial(state, IgnoreReason::ObservationLimit); break;
            case 5: IgnoreInitial(state, IgnoreReason::Retemplated); break;
            case 6: IgnoreInitial(state, IgnoreReason::Unloaded); break;
            case 7: CompleteInitial(state, Result::Cancelled); break;
            case 8: EndSession(state); break;
            case 9: AbandonSession(state); break;
            }
            VERIFY_IS_TRUE(state.initial == InitialState::Finished && !owner.registration.IsArmed());
            VERIFY_ARE_EQUAL(1u, observed->removals);
            VERIFY_ARE_EQUAL(1u, owner.completions);
            IgnoreInitial(state, IgnoreReason::Unloaded);
            VERIFY_ARE_EQUAL(1u, owner.completions);
            AbandonSession(state);
            publisher->Release();
        }
#else
        TableViewTelemetry::State state;
        InitialObserverOwner owner;
        state.initialCompleted = InitialObserverOwner::Completed;
        state.initialCompletedContext = &owner;
        TableViewTelemetry::BeginInitial(state, TableViewTelemetry::Origin::Loaded);
        VERIFY_IS_FALSE(state.initialTipPending);
        TableViewTelemetry::AbandonSession(state);
        VERIFY_ARE_EQUAL(0u, owner.completions);
#endif
    }
};
