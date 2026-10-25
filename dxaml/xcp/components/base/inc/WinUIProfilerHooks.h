// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#pragma push_macro("Trace")
#undef Trace
#include "WinUIProfilerTracing.h"
#pragma pop_macro("Trace")

namespace WinUIProfiler::Hooks
{
    // Process PAL lifetime, not an individual CoreServices/WindowsXamlManager lifetime.
    void InitializeRuntime() noexcept;
    void ShutdownRuntime(bool processTerminating = false) noexcept;
#ifdef WINUI_PROFILER_STANDALONE
    void HoldRegistryForTerminationTest(HANDLE ready) noexcept;
    void HoldEligibilityForTerminationTest(HANDLE ready, HANDLE resume) noexcept;
    void GetGraphicsRegistryCountsForTest(size_t& devices, size_t& surfaces) noexcept;
    void ArmGraphicsEmissionFenceForTest(Event definition, HANDLE ready, HANDLE resume) noexcept;
    size_t LayoutStackCountForTest() noexcept;
#endif
    bool IdentitiesEnabled() noexcept;
    bool TypesEnabled() noexcept;
    void Born(Domain domain, const void* object) noexcept;
    void Destroyed(Domain domain, const void* object) noexcept;
    void ResolveCoreType(const void* core, uint64_t typeId, const wchar_t* name, bool uiElement) noexcept;
    void Associate(const void* core, const void* peer, bool linked) noexcept;
    bool FramesEnabled() noexcept;
    bool TreesEnabled() noexcept;
    bool PropertiesEnabled() noexcept;
    bool CompositionEnabled() noexcept;
    void Work(const void* root, const void* dispatcher, bool compositionWork = false) noexcept;
    void ForgetRoot(const void* root) noexcept;
    void CompleteBatch(const void* dispatcher, HRESULT result) noexcept;
    void Commit(const void* dispatcher, HRESULT result, bool primary) noexcept;
    void Membership(const void* root, const void* dispatcher, const void* parent, const void* child,
        uint8_t relationship, int32_t order, bool connected, uint8_t membership) noexcept;
    void Property(const void* root, const void* dispatcher, const void* object, uint64_t propertyId,
        const wchar_t* declaringType, const wchar_t* name, const Value& previous, const Value& current) noexcept;
    void Link(const void* from, Domain fromDomain, const void* to, Domain toDomain, uint8_t kind, bool linked) noexcept;
    void LinkVisual(const void* node, IUnknown* visual) noexcept;
    // The caller's existing strong slot must remain alive throughout this call.
    // Null retires an observed slot, not the visual's actual COM lifetime.
    void VisualSlot(const void* owner, Domain domain, uint32_t slot, IUnknown* visual, const void* group = nullptr) noexcept;
    void ReleaseVisualSlotGroup(const void* group) noexcept;
    void ReleaseVisualOwner(const void* owner, Domain domain) noexcept;
    void VisualProperty(IUnknown* visual, uint8_t property, double value) noexcept;
    void VisualMembership(IUnknown* parent, IUnknown* child, int32_t order, bool connected) noexcept;
    void Layout(const void* root, const void* dispatcher, const void* element, uint8_t kind, bool begin) noexcept;
    class RootScope
    {
    public:
        RootScope(const void* root, const void* dispatcher, bool hasPendingWork = true) noexcept;
        ~RootScope() noexcept;
    private:
        const void* m_previous = nullptr;
        const void* m_previousDispatcher = nullptr;
        bool m_active = false;
    };
    class BoundaryScope
    {
    public:
        explicit BoundaryScope(uint8_t kind) noexcept;
        ~BoundaryScope() noexcept;
        void Result(HRESULT value) noexcept { m_result = {true, value}; }
    private:
        uint64_t m_id = 0;
        uint8_t m_kind;
        Cause m_cause;
        Optional<int32_t> m_result;
    };
}
