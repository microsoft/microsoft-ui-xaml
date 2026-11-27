// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewRowFlattener.h"
#include "RowMetadataProvider.h"
#include "GroupedEntry.h"
#include "RuntimeProfiler.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"
#include "TVDiag.h"

#include <algorithm>
#include <cwchar>
#include <limits>
#include <string>
#include <utility>

#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;

    TableViewRowItemKeySelector MakeIdentitySelector()
    {
        return [](winrt::IInspectable const& item) -> winrt::hstring
        {
            return TableViewRowFlattener::ObjectIdentity(item);
        };
    }

    uintptr_t CanonicalAddress(winrt::IInspectable const& value)
    {
        if (!value)
        {
            return 0;
        }
        return reinterpret_cast<uintptr_t>(winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>()));
    }

    // The group's key value when its Group object exposes one as a "Key" property (the IGrouping<K,T>
    // idiom, and CollectionViewSource's computed groups), else the Group object itself.
    winrt::IInspectable GroupKey(winrt::IInspectable const& groupObject)
    {
        auto const group = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
        if (!group)
        {
            return groupObject;
        }
        auto key = group.Group();
        if (!key)
        {
            return group;
        }
        if (auto const provider = key.try_as<winrt::Microsoft::UI::Xaml::Data::ICustomPropertyProvider>())
        {
            try
            {
                if (auto const property = provider.GetCustomProperty(L"Key"); property && property.CanRead())
                {
                    if (auto value = property.GetValue(key))
                    {
                        return value;
                    }
                }
            }
            catch (...)
            {
            }
        }
        return key;
    }

    std::optional<winrt::hstring> ScalarIdentity(winrt::IInspectable const& value)
    {
        using winrt::Windows::Foundation::PropertyType;
        auto const scalar = value.try_as<winrt::Windows::Foundation::IPropertyValue>();
        if (!scalar)
        {
            return std::nullopt;
        }

        auto const type = scalar.Type();
        std::wstring text;
        switch (type)
        {
        case PropertyType::String: text = scalar.GetString(); break;
        case PropertyType::Boolean: text = scalar.GetBoolean() ? L"1" : L"0"; break;
        case PropertyType::Char16: text = std::to_wstring(static_cast<uint32_t>(scalar.GetChar16())); break;
        case PropertyType::UInt8: text = std::to_wstring(scalar.GetUInt8()); break;
        case PropertyType::Int16: text = std::to_wstring(scalar.GetInt16()); break;
        case PropertyType::UInt16: text = std::to_wstring(scalar.GetUInt16()); break;
        case PropertyType::Int32: text = std::to_wstring(scalar.GetInt32()); break;
        case PropertyType::UInt32: text = std::to_wstring(scalar.GetUInt32()); break;
        case PropertyType::Int64: text = std::to_wstring(scalar.GetInt64()); break;
        case PropertyType::UInt64: text = std::to_wstring(scalar.GetUInt64()); break;
        case PropertyType::Single: text = std::to_wstring(scalar.GetSingle()); break;
        case PropertyType::Double: text = std::to_wstring(scalar.GetDouble()); break;
        case PropertyType::DateTime: text = std::to_wstring(scalar.GetDateTime().time_since_epoch().count()); break;
        case PropertyType::TimeSpan: text = std::to_wstring(scalar.GetTimeSpan().count()); break;
        case PropertyType::Guid:
        {
            wchar_t buffer[40]{};
            if (::StringFromGUID2(scalar.GetGuid(), buffer, ARRAYSIZE(buffer)) == 0)
            {
                return std::nullopt;
            }
            text = buffer;
            break;
        }
        default:
            return std::nullopt;
        }

        // The type is part of the identity, so 1 and "1" stay distinct groups.
        return winrt::hstring{ L"cvgroup:v" + std::to_wstring(static_cast<int>(type)) + L":" + text };
    }
}

TableViewRowFlattener::TableViewRowFlattener(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view) :
    m_view(view)
{
    if (!view)
    {
        throw winrt::hresult_invalid_argument(L"view cannot be null.");
    }

    if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        m_uiQueue = winrt::make_weak(queue);
    }

    m_groupRowsView = winrt::ItemsSourceView{ m_groupRows };

    m_groupExpansion.SetChangedHandler(
        [this](ShapingHelpers::RowExpansionModel::Change const& change)
        {
            OnGroupExpansionChanged(change);
        });
}

TableViewRowFlattener::~TableViewRowFlattener()
{
    if (m_viewNotify && m_viewPropertyChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_viewNotify.PropertyChanged(m_viewPropertyChangedToken); });
    }
    if (m_viewGroups && m_viewGroupsChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_viewGroups.VectorChanged(m_viewGroupsChangedToken); });
    }
    UnsubscribeFromGroupItems();
}

winrt::hstring TableViewRowFlattener::ObjectIdentity(winrt::IInspectable const& item)
{
    if (!item)
    {
        return {};
    }
    // The canonical IUnknown is the COM identity; boxed copies of one value are distinct rows.
    auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
    wchar_t buffer[2 + (sizeof(uintptr_t) * 2) + 1]{};
    swprintf_s(buffer, L"0x%zx", reinterpret_cast<size_t>(winrt::get_abi(unknown)));
    return winrt::hstring{ buffer };
}

void TableViewRowFlattener::AssertOnUiThread() const
{
    MUX_ASSERT(!m_uiQueue.get() || m_uiQueue.get().HasThreadAccess());
}

void TableViewRowFlattener::Start()
{
    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    m_isShapingView = static_cast<bool>(m_view.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewShaping>());
    AttachViewGroupsNow();

    // A shaping view keeps its identity when it starts or stops grouping (WPF): CollectionGroups turns
    // non-null or null and PropertyChanged("CollectionGroups") is raised before the view's Reset.
    if (auto const notify = m_view.try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>())
    {
        m_viewNotify = notify;
        m_viewPropertyChangedToken = notify.PropertyChanged(
            [weakThis](winrt::IInspectable const&, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.lock(); strongThis && args.PropertyName() == L"CollectionGroups")
                {
                    strongThis->AttachViewGroups();
                }
            });
    }

    Update();
}

bool TableViewRowFlattener::IsBusyWithGroups() const noexcept
{
    return m_attachingViewGroups || m_readingGroups || m_groupRowsInFlight || m_updating;
}

void TableViewRowFlattener::AttachViewGroups()
{
    // Deferred while app code (a GroupItems/Key getter, a group enumerator, a row handler) runs inside a walk of
    // m_groups or the rows: replacing them now would free what the outer loop is reading.
    m_attachViewGroupsPending = true;
    RunPendingAttachViewGroups();
}

void TableViewRowFlattener::RunPendingAttachViewGroups()
{
    if (!m_attachViewGroupsPending || m_runningPendingAttach || IsBusyWithGroups())
    {
        return;
    }

    // A loop, not recursion: an app that swaps its groups again while they are read leaves the flag set again.
    auto const keepAlive = weak_from_this().lock();
    if (!keepAlive)
    {
        return;
    }
    m_runningPendingAttach = true;
    auto const done = wil::scope_exit([this]() noexcept { m_runningPendingAttach = false; });
    int reattaches = 0;
    while (m_attachViewGroupsPending && !IsBusyWithGroups() && WithinReattachCap(reattaches))
    {
        ++reattaches;
        if (AttachViewGroupsNow())
        {
            Update();
        }
    }
}

bool TableViewRowFlattener::WithinReattachCap(int reattaches) noexcept
{
    if (reattaches < c_maxReattachesPerLoop)
    {
        return true;
    }
    // Cleared, not left set: a request still pending would be retried by the next caller and spin again.
    m_attachViewGroupsPending = false;
    TVDiag::LogRetailF(L"[TableView] The ICollectionView changed CollectionGroups on each of %d consecutive reads; keeping the groups read last.",
        c_maxReattachesPerLoop);
    return false;
}

bool TableViewRowFlattener::AttachViewGroupsNow()
{
    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    m_attachViewGroupsPending = false;
    m_attachingViewGroups = true;
    auto const done = wil::scope_exit([this]() noexcept { m_attachingViewGroups = false; });
    try
    {
        auto const groups = m_view.CollectionGroups();
        if (groups == m_viewGroups)
        {
            return false;
        }

        if (m_viewGroups && m_viewGroupsChangedToken.value)
        {
            ShapingHelpers::SafeRevokeWith([&] { m_viewGroups.VectorChanged(m_viewGroupsChangedToken); });
        }
        m_viewGroups = nullptr;
        m_viewGroupsChangedToken = {};
        m_groups.clear();

        if (groups)
        {
            m_viewGroups = groups;
            ReadViewGroups();
            m_viewGroupsChangedToken = groups.VectorChanged(
                [weakThis](winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const&,
                    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.lock())
                    {
                        strongThis->OnViewGroupsChanged(args.CollectionChange());
                    }
                });
        }
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] The ICollectionView's groups could not be read (HRESULT 0x%08X); showing its items ungrouped.",
            static_cast<unsigned int>(winrt::to_hresult()));
        m_viewGroups = nullptr;
        m_viewGroupsChangedToken = {};
        m_groups.clear();
    }
    return true;
}

void TableViewRowFlattener::Update()
{
    // A request raised while the rows are being published (an app handler of the change) runs once
    // the publication unwinds, against the declaration in force then.
    if (m_updating)
    {
        m_updatePending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    {
        m_updating = true;
        auto const done = wil::scope_exit([this]() noexcept { m_updating = false; });
        int reattaches = 0;
        do
        {
            m_updatePending = false;
            // A CollectionGroups change raised by app code during the last pass: attached here, between passes.
            if (CanAttachViewGroupsInUpdate() && WithinReattachCap(reattaches))
            {
                ++reattaches;
                AttachViewGroupsNow();
            }
            UpdateCore();
        } while (m_updatePending || (CanAttachViewGroupsInUpdate() && WithinReattachCap(reattaches)));
    }

    RaiseRowsChanged();
    RunPendingAttachViewGroups();
}

bool TableViewRowFlattener::CanAttachViewGroupsInUpdate() const noexcept
{
    return m_attachViewGroupsPending && !m_attachingViewGroups && !m_readingGroups && !m_groupRowsInFlight;
}

void TableViewRowFlattener::SetKind(Kind kind, winrt::ItemsSourceView const& rows)
{
    if (m_kind != kind)
    {
        m_rowMetadataStale = true;
    }
    m_itemsSourceView = rows;
    m_kind = kind;
}

void TableViewRowFlattener::UpdateCore()
{
    if (!m_viewGroups)
    {
        DetachGroupRows();
        if (!m_flatView)
        {
            m_flatView = winrt::ItemsSourceView{ m_view };
        }
        // The view itself: its fine-grained changes reach the repeater as they are.
        SetKind(Kind::Flat, m_flatView);
    }
    else if (m_kind != Kind::Grouped)
    {
        DetachGroupRows();
        AttachGroupRows();
        SetKind(Kind::Grouped, m_groupRowsView);
    }
}

TableViewRowMetadataProvider TableViewRowFlattener::RowMetadata()
{
    if (m_kind != Kind::Grouped)
    {
        return nullptr;
    }
    if (!m_rowMetadataStale)
    {
        if (auto existing = m_rowMetadata.lock())
        {
            return existing;
        }
    }

    auto const created = tabularPrimitives::RowMetadataProvider::CreateForGroupedRows(shared_from_this(), MakeIdentitySelector());
    m_rowMetadata = created;
    m_rowMetadataStale = false;
    return created;
}

void TableViewRowFlattener::RaiseRowsChanged() const
{
    if (auto const handler = m_rowsChanged)
    {
        handler();
    }
}

std::optional<winrt::hstring> TableViewRowFlattener::TryGetGroupIdentityForKey(winrt::IInspectable const& key) const
{
    if (!m_viewGroups)
    {
        return std::nullopt;
    }
    return IdentityForKey(key);
}

std::vector<TableViewRowFlattener::GroupInfo> TableViewRowFlattener::CurrentGroups() const
{
    std::vector<GroupInfo> groups;
    if (m_kind == Kind::Grouped)
    {
        for (auto const& group : m_groups)
        {
            groups.push_back({ group.Identity, group.Key });
        }
    }
    return groups;
}

uint32_t TableViewRowFlattener::SourceRowCount() const
{
    return m_view.Size();
}

// --- The view's groups ----------------------------------------------------------------------------

// A view may recreate its group objects on every recompute (CollectionViewSource shaping does), so a
// group is identified by its key value rather than by the group object's address.
winrt::hstring TableViewRowFlattener::IdentityForKey(winrt::IInspectable const& key)
{
    if (!key)
    {
        return L"cvgroup:null";
    }
    if (auto identity = ScalarIdentity(key))
    {
        return *identity;
    }

    // A computed group exposes its key as a "Key" property (the IGrouping<K,T> idiom).
    if (auto const provider = key.try_as<winrt::Microsoft::UI::Xaml::Data::ICustomPropertyProvider>())
    {
        try
        {
            if (auto const property = provider.GetCustomProperty(L"Key"); property && property.CanRead())
            {
                auto const value = property.GetValue(key);
                if (!value)
                {
                    return L"cvgroup:null";
                }
                if (auto identity = ScalarIdentity(value))
                {
                    return *identity;
                }
                wchar_t buffer[48];
                swprintf_s(buffer, L"cvgroup:k%p", reinterpret_cast<void*>(CanonicalAddress(value)));
                return winrt::hstring{ buffer };
            }
        }
        catch (...)
        {
        }
    }

    wchar_t buffer[48];
    swprintf_s(buffer, L"cvgroup:%p", reinterpret_cast<void*>(CanonicalAddress(key)));
    return winrt::hstring{ buffer };
}

void TableViewRowFlattener::OnViewGroupsChanged(winrt::Windows::Foundation::Collections::CollectionChange change)
{
    // A shaping view rebuilds its groups as one Reset raised once they are complete, so it is read now.
    if (m_isShapingView && change == winrt::Windows::Foundation::Collections::CollectionChange::Reset)
    {
        ReadViewGroupsNow();
        return;
    }

    // Other views replace their groups in several notifications (an IsSourceGrouped CollectionViewSource
    // clears and refills them), and a shaping view adds or removes a group before its items follow. Read
    // them once the view has finished, so the grouped rows never see a partial group set: that would drop
    // the expansion state of the groups missing from it.
    if (m_readGroupsQueued)
    {
        return;
    }
    if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
        m_readGroupsQueued = queue.TryEnqueue([weakThis]()
        {
            if (auto const strongThis = weakThis.lock())
            {
                strongThis->m_readGroupsQueued = false;
                strongThis->ReadViewGroupsNow();
            }
        });
        if (m_readGroupsQueued)
        {
            return;
        }
    }
    ReadViewGroupsNow();
}

void TableViewRowFlattener::ReadViewGroupsNow() noexcept
{
    try
    {
        ReadViewGroups();
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] Reading the view's CollectionGroups failed (HRESULT 0x%08X).",
            static_cast<unsigned int>(winrt::to_hresult()));
    }
}

void TableViewRowFlattener::ReadViewGroups()
{
    // A change raised while the groups are being replaced is applied once they are.
    if (m_readingGroups)
    {
        m_readGroupsPending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    bool changed = false;
    do
    {
        m_readingGroups = true;
        m_readGroupsPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_readingGroups = false; });

        std::vector<ViewGroup> groups;
        std::unordered_set<winrt::hstring> identities;
        uint32_t const groupCount = m_viewGroups.Size();
        groups.reserve(groupCount);
        for (uint32_t g = 0; g < groupCount; ++g)
        {
            auto const groupObject = m_viewGroups.GetAt(g);
            if (!groupObject)
            {
                continue;
            }

            ViewGroup group;
            group.Key = GroupKey(groupObject);
            group.Identity = IdentityForKey(group.Key);
            // Two groups with one key stay two groups.
            for (uint32_t suffix = 2; identities.contains(group.Identity); ++suffix)
            {
                group.Identity = IdentityForKey(group.Key) + L"#" + winrt::to_hstring(suffix);
            }
            identities.insert(group.Identity);

            if (auto const viewGroup = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
            {
                group.Items = viewGroup.GroupItems();
                if (auto const inner = viewGroup.Group())
                {
                    group.Inner = inner.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
                }
            }
            if (!group.Items)
            {
                group.Items = groupObject.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>();
            }
            if (!group.Items)
            {
                // A group that is only enumerable is read once; the next CollectionGroups change re-reads it.
                group.Snapshot = ShapingHelpers::EnumerateInspectableItems(groupObject);
            }
            groups.push_back(std::move(group));
        }

        // The same groups over the same item collections need no Reset.
        bool same = groups.size() == m_groups.size();
        for (size_t i = 0; same && i < groups.size(); ++i)
        {
            same = groups[i].Identity == m_groups[i].Identity && groups[i].Items && groups[i].Items == m_groups[i].Items;
        }

        m_groups = std::move(groups);
        if (!same)
        {
            if (m_groupRowsAttached)
            {
                RebuildGroupRows();
            }
            changed = true;
        }
    } while (m_readGroupsPending);

    if (changed && m_kind == Kind::Grouped && !m_updating)
    {
        RaiseRowsChanged();
    }
    RunPendingAttachViewGroups();
}

// --- Grouped rows ---------------------------------------------------------------------------------

winrt::hstring TableViewRowFlattener::GroupIntentKey(winrt::hstring const& groupIdentity)
{
    return L"identity:" + groupIdentity;
}

void TableViewRowFlattener::AttachGroupRows()
{
    if (!std::exchange(m_groupRowsProfiled, true))
    {
        __RP_Marker_ClassById(RuntimeProfiler::ProfId_GroupedSourceAdapter);
    }
    m_groupRowsAttached = true;
    RebuildGroupRows();
}

void TableViewRowFlattener::DetachGroupRows()
{
    UnsubscribeFromGroupItems();
    m_groupRowsAttached = false;
}

std::vector<TableViewRowFlattener::GroupSlice> TableViewRowFlattener::ReadGroupSlices() const
{
    std::vector<GroupSlice> slices;
    if (m_groupRowsAttached)
    {
        // A copy: the group enumerators and GroupItems/Group/Key getters below are app code, which can replace m_groups
        // (a re-read of the view's groups). That re-read schedules its own rebuild of the rows.
        auto const groups = m_groups;
        slices.reserve(groups.size());
        for (auto const& group : groups)
        {
            GroupSlice slice{ group.Identity, group.Key };
            if (group.Items)
            {
                slice.Items = ShapingHelpers::EnumerateInspectableItems(group.Items);
                slice.Observed = group.Items;
            }
            else
            {
                slice.Items = group.Snapshot;
            }
            if (group.Inner)
            {
                AppendSubgroupSlices(group.Inner, std::move(slice), slices);
            }
            else
            {
                slices.push_back(std::move(slice));
            }
        }
    }
    return slices;
}

// A multi-level view (WPF: nested CollectionViewGroups): a group whose GroupItems are groups is followed by its
// subgroups' slices, one level deeper, depth first. An explicit stack, not recursion: the groups are app objects, and
// one that contains itself (or a cycle of them) is shown once, as is anything deeper than c_maxGroupDepth.
void TableViewRowFlattener::AppendSubgroupSlices(
    winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup const& group,
    GroupSlice&& slice,
    std::vector<GroupSlice>& slices)
{
    using winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup;
    static constexpr size_t c_maxGroupDepth{ 64 };

    struct Frame
    {
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Children{ nullptr };
        uint32_t Count{ 0 };
        uint32_t Next{ 0 };
        winrt::hstring Identity;
        int32_t Level{ 0 };
        std::unordered_set<winrt::hstring> Identities;
    };
    std::vector<Frame> stack;
    // Holds each visited group for the walk, so an app vector that hands out a new group object per GetAt cannot free one
    // and reuse its address for a different subgroup (which would then be skipped as visited).
    std::unordered_map<uintptr_t, ICollectionViewGroup> visited;

    // Appends the slice; a group whose items are groups also opens a frame for its subgroups.
    auto const open = [&stack, &visited, &slices](ICollectionViewGroup const& current, GroupSlice&& currentSlice)
    {
        visited.emplace(CanonicalAddress(current), current);
        auto const children = current.GroupItems();
        auto const firstChild = (children && children.Size() > 0) ? children.GetAt(0) : nullptr;
        if (!firstChild || !firstChild.try_as<ICollectionViewGroup>() || stack.size() >= c_maxGroupDepth)
        {
            slices.push_back(std::move(currentSlice));
            return;
        }

        Frame frame;
        frame.Children = children;
        frame.Count = children.Size();
        frame.Identity = currentSlice.Identity;
        frame.Level = currentSlice.Level;
        currentSlice.IsBottomLevel = false;
        slices.push_back(std::move(currentSlice));
        stack.push_back(std::move(frame));
    };

    open(group, std::move(slice));
    while (!stack.empty())
    {
        auto& frame = stack.back();
        if (frame.Next >= frame.Count)
        {
            stack.pop_back();
            continue;
        }

        auto const subgroup = frame.Children.GetAt(frame.Next++).try_as<ICollectionViewGroup>();
        if (!subgroup || visited.contains(CanonicalAddress(subgroup)))
        {
            continue;
        }

        GroupSlice child;
        child.Key = GroupKey(subgroup);
        auto const childIdentity = frame.Identity + L"/" + IdentityForKey(child.Key);
        child.Identity = childIdentity;
        for (uint32_t suffix = 2; frame.Identities.contains(child.Identity); ++suffix)
        {
            child.Identity = childIdentity + L"#" + winrt::to_hstring(suffix);
        }
        frame.Identities.insert(child.Identity);
        child.Level = frame.Level + 1;
        // Not observed: every change below a top-level group reaches its leaves last, and those are observed.
        if (auto const leaves = subgroup.Group().try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
        {
            child.Items = ShapingHelpers::EnumerateInspectableItems(leaves);
        }
        // May grow the stack: frame is not used after this.
        open(subgroup, std::move(child));
    }
}

std::vector<winrt::IInspectable> TableViewRowFlattener::ReadGroupItems(winrt::hstring const& groupIdentity) const
{
    if (m_groupRowsAttached)
    {
        for (auto const& group : m_groups)
        {
            if (group.Identity == groupIdentity)
            {
                // Copied before the enumeration (app code) can replace m_groups.
                auto const items = group.Items;
                return items ? ShapingHelpers::EnumerateInspectableItems(items) : group.Snapshot;
            }
        }
    }
    return {};
}

void TableViewRowFlattener::RebuildGroupRows()
{
    AssertOnUiThread();

    if (m_groupRowsInFlight)
    {
        // A change notification raised synchronously inside ReplaceAll re-entered: run once after unwind.
        m_groupRowsPending = true;
        return;
    }

    bool runPending = false;
    do
    {
        m_groupRowsInFlight = true;
        m_groupRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_groupRowsInFlight = false;
            runPending = m_groupRowsPending;
            m_groupRowsPending = false;
        });

        UnsubscribeFromGroupItems();

        // One Reset for the whole set: per-item Adds during a known full rebuild cost one layout pass each.
        std::vector<winrt::IInspectable> built;
        std::unordered_set<winrt::hstring> liveKeys;
        std::vector<GroupInfo> liveGroups;
        // Slices deeper than a collapsed group's level are hidden under it.
        int32_t collapsedLevel = (std::numeric_limits<int32_t>::max)();
        bool hasSubgroupRows = false;

        for (auto& slice : ReadGroupSlices())
        {
            auto const intentKey = GroupIntentKey(slice.Identity);
            liveKeys.insert(intentKey);
            liveGroups.push_back({ slice.Identity, slice.Key });
            hasSubgroupRows = hasSubgroupRows || slice.Level > 0;

            const bool isExpanded = m_groupExpansion.IsExpanded(intentKey);
            if (slice.Level <= collapsedLevel)
            {
                collapsedLevel = isExpanded ? (std::numeric_limits<int32_t>::max)() : slice.Level;

                // An empty group keeps its header: it stays targetable.
                const int32_t groupItemCount = static_cast<int32_t>(slice.Items.size());
                built.push_back(winrt::make<GroupedEntry>(slice.Identity, slice.Key, groupItemCount, isExpanded, slice.Level));
                if (isExpanded && slice.IsBottomLevel)
                {
                    built.insert(built.end(), slice.Items.begin(), slice.Items.end());
                }
            }

            // Any change inside a view group rebuilds the rows.
            if (slice.Observed)
            {
                GroupItemsSubscription subscription{ slice.Observed };
                subscription.Token = slice.Observed.VectorChanged(
                    [weakThis = weak_from_this()](auto&&, auto&&)
                    {
                        if (auto strongThis = weakThis.lock())
                        {
                            strongThis->RebuildGroupRows();
                        }
                    });
                m_groupItemsSubscriptions.push_back(std::move(subscription));
            }
        }

        m_liveGroups = std::move(liveGroups);
        m_hasSubgroupRows = hasSubgroupRows;
        // Silent: pruning a dead key changes no live key's state.
        m_groupExpansion.RetainOnly(liveKeys);

        m_groupRows.ReplaceAll(built);
    } while (runPending);

    RunPendingAttachViewGroups();
}

void TableViewRowFlattener::UnsubscribeFromGroupItems()
{
    // Moved out first: a revoke runs app code (the vector's remove_VectorChanged), which can re-enter.
    auto subscriptions = std::exchange(m_groupItemsSubscriptions, {});
    for (auto& subscription : subscriptions)
    {
        if (subscription.Items && subscription.Token.value)
        {
            ShapingHelpers::SafeRevokeWith([&] { subscription.Items.VectorChanged(subscription.Token); });
        }
    }
}

bool TableViewRowFlattener::IsGroupExpanded(winrt::hstring const& groupIdentity) const
{
    return m_groupExpansion.IsExpanded(GroupIntentKey(groupIdentity));
}

void TableViewRowFlattener::SetGroupExpanded(winrt::hstring const& groupIdentity, bool isExpanded)
{
    if (groupIdentity.empty())
    {
        return;
    }

    auto const key = GroupIntentKey(groupIdentity);
    if (m_groupExpansion.IsExpanded(key) != isExpanded)
    {
        // Through the model, so a per-group toggle and ExpandAll share one path (OnGroupExpansionChanged).
        m_groupExpansion.SetExpanded(key, isExpanded);
    }
}

void TableViewRowFlattener::ExpandAllGroups()
{
    m_groupExpansion.SetAllExpanded(true);
}

void TableViewRowFlattener::CollapseAllGroups()
{
    m_groupExpansion.SetAllExpanded(false);
}

TableViewRowFlattener::GroupInfo const* TableViewRowFlattener::TryGetLiveGroup(winrt::hstring const& groupIdentity) const
{
    if (groupIdentity.empty())
    {
        return nullptr;
    }
    auto const found = std::find_if(m_liveGroups.begin(), m_liveGroups.end(),
        [&](GroupInfo const& group) { return group.Identity == groupIdentity; });
    return found != m_liveGroups.end() ? &*found : nullptr;
}

void TableViewRowFlattener::OnGroupExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertOnUiThread();

    if (m_groupRowsInFlight)
    {
        // The model already holds the new intent; the outer operation rebuilds once afterward.
        m_groupRowsPending = true;
        return;
    }

    // Only a single-group toggle can splice; a baseline move or a batch rebuilds into one Reset.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        RebuildGroupRows();
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_groupRowsInFlight = true;
        m_groupRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_groupRowsInFlight = false;
            runPending = m_groupRowsPending;
            m_groupRowsPending = false;
        });

        applied = TryApplyGroupExpansionSplice(change.Keys.front(), change.IsExpanded);
    }

    if (runPending || !applied)
    {
        RebuildGroupRows();
    }
    else
    {
        RunPendingAttachViewGroups();
    }
}

bool TableViewRowFlattener::TryApplyGroupExpansionSplice(winrt::hstring const& intentKey, bool expand)
{
    if (m_hasSubgroupRows)
    {
        return false;
    }

    const uint32_t count = m_groupRows.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const entry = TryGetGroupedEntry(m_groupRows.GetAt(i));
        if (!entry || GroupIntentKey(entry->Identity()) != intentKey)
        {
            continue;
        }

        if (entry->IsExpanded() == expand)
        {
            return true;
        }

        const int32_t groupItemCount = entry->GroupItemCount();

        // GroupedEntry is immutable: one Replace refreshes the chevron without touching any other row.
        m_groupRows.SetAt(i, winrt::make<GroupedEntry>(entry->Identity(), entry->Key(), groupItemCount, expand));

        if (expand)
        {
            // A count that disagrees means the group changed without a notification we processed.
            auto const items = ReadGroupItems(entry->Identity());
            if (static_cast<int32_t>(items.size()) != groupItemCount)
            {
                return false;
            }

            uint32_t insertAt = i + 1;
            for (auto const& item : items)
            {
                m_groupRows.InsertAt(insertAt++, item);
            }
        }
        else
        {
            // The group's rows run from the header to the next header (or the end).
            const uint32_t firstData = i + 1;
            for (int32_t removed = 0; removed < groupItemCount; ++removed)
            {
                if (firstData >= m_groupRows.Size() || TryGetGroupedEntry(m_groupRows.GetAt(firstData)))
                {
                    return false;
                }
                m_groupRows.RemoveAt(firstData);
            }
        }

        return true;
    }

    // The header is not in the rows (mid-reshape); a rebuild reconciles.
    return false;
}
