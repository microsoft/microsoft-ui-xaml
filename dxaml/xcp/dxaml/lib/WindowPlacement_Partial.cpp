// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacement_Partial.h"
#include "WindowShowOptions_Partial.h"
#include "WindowPlacementStore.h"

using namespace DirectUI;
namespace placement = DirectUI::WindowPlacementPersistence;
using RectInt32 = ABI::Windows::Graphics::RectInt32;

REFERENCE_ELEMENT_NAME_IMPL(RectInt32, L"Windows.Graphics.RectInt32");
REFERENCE_ELEMENT_NAME_IMPL(GUID, L"Guid");

namespace
{
    placement::Rect ToSnapshotRect(RectInt32 value)
    {
        return {value.X, value.Y, value.Width, value.Height};
    }

    RectInt32 ToPublicRect(placement::Rect value)
    {
        return {value.X, value.Y, value.Width, value.Height};
    }

    template<typename T>
    HRESULT BoxOptional(const std::optional<T>& value, wf::IReference<T>** result)
    {
        *result = nullptr;
        if (!value) return S_OK;
        ctl::ComPtr<IInspectable> boxed;
        IFC_RETURN(PropertyValue::CreateReference<T>(*value, &boxed));
        return boxed.CopyTo(result);
    }
}

HRESULT WindowPlacementFactory::CreateInstanceImpl(
    RectInt32 normalRect, RectInt32 workArea, INT32 dpi,
    xaml::IWindowPlacement** result)
{
    placement::Snapshot value;
    value.NormalRect = ToSnapshotRect(normalRect);
    value.WorkArea = ToSnapshotRect(workArea);
    value.Dpi = dpi;
    return WindowPlacement::CreateFromSnapshot(value, result);
}

HRESULT WindowPlacementFactory::LoadForPersistPlacementIdImpl(
    HSTRING persistPlacementId,
    xaml::IWindowPlacement** result)
{
    *result = nullptr;
    UINT32 placementIdLength{};
    const auto placementIdBuffer = WindowsGetStringRawBuffer(persistPlacementId, &placementIdLength);
    if (!placementIdBuffer || placementIdLength == 0) return E_INVALIDARG;

    placement::LoadResult loaded;
    // ReadPersistPlacement owns turning a failed load into a failing HRESULT, so IFC_RETURN
    // catches it here. The Unexpected case below only remains reachable when Error succeeded.
    IFC_RETURN(placement::ReadPersistPlacement(
        reinterpret_cast<const char16_t*>(placementIdBuffer),
        placementIdLength,
        loaded));
    switch (loaded.Status)
    {
    case placement::LoadStatus::Loaded:
        return WindowPlacement::CreateFromSnapshot(loaded.Placement.Placement, result);
    case placement::LoadStatus::Missing:
    case placement::LoadStatus::Unavailable:
    case placement::LoadStatus::Invalid:
        return S_OK;
    case placement::LoadStatus::Unexpected:
        return FAILED(loaded.Error) ? loaded.Error : E_FAIL;
    default:
        return E_FAIL;
    }
}

HRESULT WindowPlacement::CreateFromSnapshot(
    const placement::Snapshot& value, xaml::IWindowPlacement** result)
{
    *result = nullptr;
    if (!placement::IsValid(value)) return E_INVALIDARG;
    ctl::ComPtr<WindowPlacement> instance;
    IFC_RETURN(ctl::make(&instance));
    if (!placement::TryCopySnapshot(value, instance->m_snapshot)) return E_OUTOFMEMORY;
    return instance.CopyTo(result);
}

HRESULT WindowPlacement::CopySnapshot(placement::Snapshot& value)
{
    auto guard = m_lock.lock_shared();
    if (!placement::TryCopySnapshot(m_snapshot, value)) return E_OUTOFMEMORY;
    // Empty identity is meaningful while editing, but not when accepting placement.
    if (value.VirtualDesktopId && InlineIsEqualGUID(*value.VirtualDesktopId, GUID_NULL))
    {
        value.VirtualDesktopId.reset();
    }
    return S_OK;
}

HRESULT WindowPlacement::get_NormalRectImpl(RectInt32* value)
{
    auto guard = m_lock.lock_shared();
    *value = ToPublicRect(m_snapshot.NormalRect);
    return S_OK;
}

HRESULT WindowPlacement::put_NormalRectImpl(RectInt32 value)
{
    auto guard = m_lock.lock_exclusive();
    m_snapshot.NormalRect = ToSnapshotRect(value);
    return S_OK;
}

HRESULT WindowPlacement::get_WorkAreaImpl(RectInt32* value)
{
    auto guard = m_lock.lock_shared();
    *value = ToPublicRect(m_snapshot.WorkArea);
    return S_OK;
}

HRESULT WindowPlacement::put_WorkAreaImpl(RectInt32 value)
{
    auto guard = m_lock.lock_exclusive();
    m_snapshot.WorkArea = ToSnapshotRect(value);
    return S_OK;
}

HRESULT WindowPlacement::get_DpiImpl(INT32* value)
{
    auto guard = m_lock.lock_shared();
    *value = m_snapshot.Dpi;
    return S_OK;
}

HRESULT WindowPlacement::put_DpiImpl(INT32 value)
{
    auto guard = m_lock.lock_exclusive();
    m_snapshot.Dpi = value;
    return S_OK;
}

HRESULT WindowPlacement::get_StateImpl(xaml::WindowPlacementState* value)
{
    auto guard = m_lock.lock_shared();
    *value = static_cast<xaml::WindowPlacementState>(m_snapshot.PlacementState);
    return S_OK;
}

HRESULT WindowPlacement::put_StateImpl(xaml::WindowPlacementState value)
{
    auto guard = m_lock.lock_exclusive();
    m_snapshot.PlacementState = static_cast<placement::State>(value);
    return S_OK;
}

HRESULT WindowPlacement::get_SnapRectImpl(wf::IReference<RectInt32>** value)
{
    std::optional<RectInt32> rect;
    {
        auto guard = m_lock.lock_shared();
        if (m_snapshot.SnapRect) rect = ToPublicRect(*m_snapshot.SnapRect);
    }
    return BoxOptional(rect, value);
}

HRESULT WindowPlacement::put_SnapRectImpl(wf::IReference<RectInt32>* value)
{
    // Read a caller-supplied interface before locking or mutating our value.
    std::optional<placement::Rect> rect;
    if (value)
    {
        RectInt32 unboxed{};
        IFC_RETURN(value->get_Value(&unboxed));
        rect = ToSnapshotRect(unboxed);
    }
    auto guard = m_lock.lock_exclusive();
    m_snapshot.SnapRect = rect;
    return S_OK;
}

HRESULT WindowPlacement::get_DisplayDeviceNameImpl(HSTRING* value)
{
    auto guard = m_lock.lock_shared();
    return WindowsCreateString(
        reinterpret_cast<const wchar_t*>(m_snapshot.DisplayDeviceName.data()),
        static_cast<UINT32>(m_snapshot.DisplayDeviceName.size()), value);
}

HRESULT WindowPlacement::put_DisplayDeviceNameImpl(HSTRING value)
{
    UINT32 length{};
    const auto text = WindowsGetStringRawBuffer(value, &length);
    std::u16string copy;
    if (!placement::TryCopyDeviceName(reinterpret_cast<const char16_t*>(text), length, copy)) return E_OUTOFMEMORY;
    auto guard = m_lock.lock_exclusive();
    m_snapshot.DisplayDeviceName = std::move(copy);
    return S_OK;
}

HRESULT WindowPlacement::get_VirtualDesktopIdImpl(wf::IReference<GUID>** value)
{
    std::optional<GUID> id;
    {
        auto guard = m_lock.lock_shared();
        id = m_snapshot.VirtualDesktopId;
    }
    return BoxOptional(id, value);
}

HRESULT WindowPlacement::put_VirtualDesktopIdImpl(wf::IReference<GUID>* value)
{
    std::optional<GUID> id;
    if (value)
    {
        GUID unboxed{};
        IFC_RETURN(value->get_Value(&unboxed));
        id = unboxed;
    }
    auto guard = m_lock.lock_exclusive();
    m_snapshot.VirtualDesktopId = id;
    return S_OK;
}

HRESULT WindowShowOptions::get_PlacementImpl(xaml::IWindowPlacement** value)
{
    auto guard = m_lock.lock_shared();
    return m_placement.CopyTo(value);
}

HRESULT WindowShowOptions::put_PlacementImpl(xaml::IWindowPlacement* value)
{
    ctl::ComPtr<xaml::IWindowPlacement> copy(value);
    {
        auto guard = m_lock.lock_exclusive();
        std::swap(m_placement, copy);
    }
    // Release the previous object outside the lock.
    return S_OK;
}

HRESULT WindowShowOptions::get_ReasonImpl(xaml::WindowShowReason* value)
{
    auto guard = m_lock.lock_shared();
    *value = m_reason;
    return S_OK;
}

HRESULT WindowShowOptions::put_ReasonImpl(xaml::WindowShowReason value)
{
    auto guard = m_lock.lock_exclusive();
    m_reason = value;
    return S_OK;
}

HRESULT WindowShowOptions::get_CascadeBehaviorImpl(xaml::WindowCascadeBehavior* value)
{
    auto guard = m_lock.lock_shared();
    *value = m_cascadeBehavior;
    return S_OK;
}

HRESULT WindowShowOptions::put_CascadeBehaviorImpl(xaml::WindowCascadeBehavior value)
{
    auto guard = m_lock.lock_exclusive();
    m_cascadeBehavior = value;
    return S_OK;
}

HRESULT WindowShowOptions::get_DoNotActivateImpl(BOOLEAN* value)
{
    auto guard = m_lock.lock_shared();
    *value = m_doNotActivate;
    return S_OK;
}

HRESULT WindowShowOptions::put_DoNotActivateImpl(BOOLEAN value)
{
    auto guard = m_lock.lock_exclusive();
    m_doNotActivate = !!value;
    return S_OK;
}

HRESULT WindowShowOptions::get_SkipInitialPlacementImpl(BOOLEAN* value)
{
    auto guard = m_lock.lock_shared();
    *value = m_skipInitialPlacement;
    return S_OK;
}

HRESULT WindowShowOptions::put_SkipInitialPlacementImpl(BOOLEAN value)
{
    auto guard = m_lock.lock_exclusive();
    m_skipInitialPlacement = !!value;
    return S_OK;
}

HRESULT WindowShowOptions::CopyInitialRequest(bool isHiddenApplication, placement::InitialRequest& result)
{
    placement::InitialRequest copy;
    ctl::ComPtr<xaml::IWindowPlacement> value;
    {
        auto guard = m_lock.lock_shared();
        copy.Reason = static_cast<INT32>(m_reason);
        copy.CascadeBehavior = static_cast<INT32>(m_cascadeBehavior);
        copy.DoNotActivate = !!m_doNotActivate;
        copy.SkipInitialPlacement = !!m_skipInitialPlacement;
        value = m_placement;
    }
    if (value)
    {
        // Sealed framework value: QI the implementation, never read its fields separately.
        ctl::ComPtr<WindowPlacement> implementation;
        IFC_RETURN(value.As(&implementation));
        copy.Placement.emplace();
        IFC_RETURN(implementation->CopySnapshot(*copy.Placement));
    }
    if (!placement::IsValidInitialRequest(copy, isHiddenApplication)) return E_INVALIDARG;
    result = std::move(copy);
    return S_OK;
}
