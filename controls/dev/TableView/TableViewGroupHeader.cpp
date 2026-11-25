// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewGroupHeader.h"
#include "TableView.h"
#include "TableViewFocusHelpers.h"
#include "TableViewGroupHeaderToggleRequestedEventArgs.h"
#include "TableViewGroupInfo.h"
#include "TableViewGroupHeaderAutomationPeer.h"
#include "TableViewAutomationHelpers.h"

static constexpr std::wstring_view s_GridLineBorderPartName{ L"PART_GridLineBorder"sv };
static constexpr std::wstring_view s_ExpanderGutterPartName{ L"PART_ExpanderGutter"sv };

TableViewGroupHeader::TableViewGroupHeader()
{
    SetDefaultStyleKey(this);
    RegisterPropertyChangedCallback(winrt::Control::PaddingProperty(),
        [weakThis = get_weak()](auto const&, auto const&)
        {
            if (auto header = weakThis.get())
            {
                header->RefreshContentAlignment();
            }
        });
    RegisterPropertyChangedCallback(winrt::UIElement::UseLayoutRoundingProperty(),
        [weakThis = get_weak()](auto const&, auto const&)
        {
            if (auto header = weakThis.get())
            {
                header->RefreshContentAlignment();
            }
        });
    RegisterPropertyChangedCallback(winrt::Control::BorderThicknessProperty(),
        [weakThis = get_weak()](auto const&, auto const&)
        {
            if (auto header = weakThis.get())
            {
                header->RefreshGridLines();
            }
        });

    RegisterPropertyChangedCallback(winrt::UIElement::ContextFlyoutProperty(),
        [weakThis = get_weak()](auto&&, auto&&)
        {
            if (auto const strongThis = weakThis.get())
            {
                strongThis->m_contextFlyoutOpeningRevoker = TableViewDetails::HookContextFlyoutSuppression(*strongThis);
            }
        });
}

namespace
{
    class TableViewGroupBindingAutomationPeer :
        public TableViewBindingAutomationPeerT<TableViewGroupBindingAutomationPeer, winrt::IExpandCollapseProvider, winrt::IGridItemProvider, winrt::IScrollItemProvider>
    {
    public:
        explicit TableViewGroupBindingAutomationPeer(winrt::TableViewGroupHeader const& header) : m_header(winrt::make_weak(header)) {}

        // Served here so a held pattern object follows the group, not whichever group the header shows later.
        winrt::IInspectable GetPatternCore(winrt::PatternInterface const& pattern)
        {
            auto const peer = HeaderPeer();
            return pattern == winrt::PatternInterface::ExpandCollapse || pattern == winrt::PatternInterface::GridItem ||
                    pattern == winrt::PatternInterface::ScrollItem
                ? winrt::IInspectable{ *this } : peer.GetPattern(pattern);
        }
        winrt::hstring GetNameCore() { return HeaderPeer().GetName(); }
        int32_t GetCultureCore() { return HeaderPeer().GetCulture(); }
        winrt::hstring GetClassNameCore() { return HeaderPeer().GetClassName(); }
        winrt::AutomationControlType GetAutomationControlTypeCore() { return HeaderPeer().GetAutomationControlType(); }
        winrt::hstring GetLocalizedControlTypeCore() { return HeaderPeer().GetLocalizedControlType(); }
        bool IsEnabledCore() { return HeaderPeer().IsEnabled(); }
        void SetFocusCore() { HeaderPeer().SetFocus(); }
        void ShowContextMenuCore() { HeaderPeer().ShowContextMenu(); }
        winrt::IVector<winrt::AutomationPeer> GetChildrenCore() { return HeaderPeer().GetChildren(); }
        // Children are reached through this element so they parent to it, not to the pooled header's peer.
        winrt::IInspectable NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction)
        {
            using Direction = winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection;
            auto const peer = HeaderPeer();
            return direction == Direction::FirstChild || direction == Direction::LastChild
                ? __super::NavigateCore(direction) : peer.Navigate(direction);
        }

        winrt::ExpandCollapseState ExpandCollapseState() { return HeaderPeer().as<winrt::IExpandCollapseProvider>().ExpandCollapseState(); }
        void Expand() { HeaderPeer().as<winrt::IExpandCollapseProvider>().Expand(); }
        void Collapse() { HeaderPeer().as<winrt::IExpandCollapseProvider>().Collapse(); }

        int32_t Row() { return HeaderPeer().as<winrt::IGridItemProvider>().Row(); }
        int32_t Column() { return HeaderPeer().as<winrt::IGridItemProvider>().Column(); }
        int32_t RowSpan() { return HeaderPeer().as<winrt::IGridItemProvider>().RowSpan(); }
        int32_t ColumnSpan() { return HeaderPeer().as<winrt::IGridItemProvider>().ColumnSpan(); }
        winrt::IRawElementProviderSimple ContainingGrid() { return HeaderPeer().as<winrt::IGridItemProvider>().ContainingGrid(); }

        void ScrollIntoView()
        {
            HeaderPeer();
            winrt::BringIntoViewOptions options;
            options.AnimationDesired(false);
            m_header.get().StartBringIntoView(options);
        }

        void Retarget(winrt::TableViewGroupHeader const& header) { m_header = winrt::make_weak(header); }

        template <typename Forward, typename T>
        T ForwardToContainerPeer(Forward&& forward, T const&)
        {
            return static_cast<T>(forward(HeaderPeer()));
        }

        winrt::UIElement TryBoundContainer() { return TryBoundHeader().first; }

    private:
        winrt::AutomationPeer HeaderPeer()
        {
            auto const bound = TryBoundHeader();
            if (!bound.first)
            {
                ThrowElementNotAvailable();
            }
            return bound.second;
        }

        std::pair<winrt::TableViewGroupHeader, winrt::AutomationPeer> TryBoundHeader()
        {
            auto const header = m_header.get();
            auto const table = header ? winrt::get_self<TableViewGroupHeader>(header)->GetOwningTableView() : nullptr;
            auto const repeater = table ? winrt::get_self<TableView>(table)->GetRowsRepeaterInternal() : nullptr;
            auto const peer = repeater ? winrt::FrameworkElementAutomationPeer::FromElement(header) : nullptr;
            if (!IsEventsSourceOf(peer) || repeater.GetElementIndex(header) < 0)
            {
                return { nullptr, nullptr };
            }
            return { header, peer };
        }

        winrt::weak_ref<winrt::TableViewGroupHeader> m_header{ nullptr };
    };
}

winrt::AutomationPeer TableViewGroupHeader::OnCreateAutomationPeer()
{
    auto const peer = winrt::make<TableViewGroupHeaderAutomationPeer>(*this);
    SyncGroupAutomationPeerInternal(peer, true, GetGroupIdentity());
    return peer;
}

void TableViewGroupHeader::SyncGroupAutomationPeerInternal(
    winrt::AutomationPeer const& headerPeer, bool newBinding, winrt::hstring const& groupIdentity)
{
    // An EventsSource the app installed wins; only replace or clear the binding this header installed.
    if (auto const current = headerPeer.EventsSource(); current && current != m_groupBinding.get())
    {
        return;
    }

    auto const owner = GetOwningTableView();
    if (!m_groupEntry.get())
    {
        headerPeer.EventsSource(nullptr);
    }
    else if (!groupIdentity.empty() && owner)
    {
        auto* const tableView = winrt::get_self<TableView>(owner);
        auto binding = tableView->GetGroupAutomationElementInternal(groupIdentity);
        if (binding)
        {
            winrt::get_self<TableViewGroupBindingAutomationPeer>(binding.as<winrt::IExpandCollapseProvider>())->Retarget(*this);
        }
        else
        {
            binding = winrt::make<TableViewGroupBindingAutomationPeer>(*this);
            tableView->SetGroupAutomationElementInternal(groupIdentity, binding);
        }
        m_groupBinding = winrt::make_weak(binding);
        if (headerPeer.EventsSource() != binding)
        {
            headerPeer.EventsSource(binding);
        }
    }
    else if (newBinding)
    {
        auto const binding = winrt::make<TableViewGroupBindingAutomationPeer>(*this);
        m_groupBinding = winrt::make_weak(binding);
        headerPeer.EventsSource(binding);
    }
    else if (!headerPeer.EventsSource())
    {
        auto binding = m_groupBinding.get();
        if (!binding)
        {
            binding = winrt::make<TableViewGroupBindingAutomationPeer>(*this);
            m_groupBinding = winrt::make_weak(binding);
        }
        headerPeer.EventsSource(binding);
    }
}

bool TableViewGroupHeader::HasContentAutomationPeersInternal()
{
    std::vector<winrt::DependencyObject> pending;
    if (auto const root = ContentTemplateRoot())
    {
        pending.push_back(root);
    }
    while (!pending.empty())
    {
        auto const element = std::move(pending.back());
        pending.pop_back();
        if (auto const uie = element.try_as<winrt::UIElement>(); uie && winrt::FrameworkElementAutomationPeer::FromElement(uie))
        {
            return true;
        }
        const int32_t count = winrt::VisualTreeHelper::GetChildrenCount(element);
        for (int32_t i = 0; i < count; ++i)
        {
            pending.push_back(winrt::VisualTreeHelper::GetChild(element, i));
        }
    }
    return false;
}

bool TableViewGroupHeader::IsExpanderPartInternal(winrt::DependencyObject element)
{
    auto const gutter = m_expanderGutter.get();
    winrt::DependencyObject const self = *this;
    for (; gutter && element && element != self; element = winrt::VisualTreeHelper::GetParent(element))
    {
        if (element == gutter)
        {
            return true;
        }
    }
    return false;
}

winrt::Size TableViewGroupHeader::MeasureOverride(winrt::Size const& availableSize)
{
    RefreshDensity();
    auto const desired = MeasureBand(availableSize);

    TrackDefaultCount(ContentTemplateRoot());
    if (!IsEnabled())
    {
        UpdateDefaultCountOpacity();
    }
    return desired;
}

winrt::Size TableViewGroupHeader::MeasureBand(winrt::Size const& availableSize)
{
    // Same sum TableViewCellsPanel uses for a row, evaluated at the same point in the layout
    // pass, so the band and the rows can never disagree.
    double columnsTotal = 0.0;
    if (auto const owner = GetOwningTableView())
    {
        if (auto const columns = owner.Columns())
        {
            for (auto const& column : columns)
            {
                if (column && column.Visibility() == winrt::Visibility::Visible)
                {
                    columnsTotal += std::max(0.0, column.ActualWidth());
                }
            }
        }
    }

    if (columnsTotal <= 0.0)
    {
        // Before any column has resolved (first pass, or no owner yet) fall back to the natural
        // measure rather than reporting zero, which would collapse the band.
        return __super::MeasureOverride(availableSize);
    }

    const auto width = static_cast<float>(columnsTotal);
    auto const desired = __super::MeasureOverride({ width, availableSize.Height });
    return { width, desired.Height };
}

void TableViewGroupHeader::SetOwningTableViewInternal(winrt::TableView const& owner)
{
    // Weak: the TableView owns this container through the repeater, so a strong ref here would
    // be a cycle. Mirrors TableViewRow::SetOwningTableViewInternal.
    m_owningTableView = owner ? winrt::make_weak(owner) : nullptr;
    m_inspectedContentRoot = nullptr;
    RefreshGridLines();
}

winrt::TableView TableViewGroupHeader::GetOwningTableView() const
{
    return m_owningTableView.get();
}

void TableViewGroupHeader::RefreshDensity()
{
    if (auto const owner = GetOwningTableView())
    {
        auto* const ownerImpl = winrt::get_self<TableView>(owner);
        auto const style = ownerImpl->GetGroupHeaderStyle(*this);
        if (!m_densityStyleInspected || style != m_densityStyle.get())
        {
            m_styleSetsMinHeight = TableView::StyleSetsProperty(style, winrt::FrameworkElement::MinHeightProperty());
            m_styleSetsPadding = TableView::StyleSetsProperty(style, winrt::Control::PaddingProperty());
            m_styleSetsFontSize = TableView::StyleSetsProperty(style, winrt::Control::FontSizeProperty());
            m_densityStyle.set(style);
            m_densityStyleInspected = true;
        }
        const auto update = [this](winrt::DependencyProperty const& property, auto value, auto& appliedBinding, bool styleSetsProperty)
        {
            using Value = std::decay_t<decltype(value)>;
            auto const local = ReadLocalValue(property);
            auto const expression = GetBindingExpression(property);
            const bool ownsLocal = expression && expression.ParentBinding() == appliedBinding.get();

            // Binding identity distinguishes app overrides even when the effective value is unchanged.
            if (styleSetsProperty)
            {
                if (ownsLocal)
                {
                    ClearValue(property);
                }
                appliedBinding.set(nullptr);
            }
            else if (local == winrt::DependencyProperty::UnsetValue() || ownsLocal)
            {
                if (!ownsLocal || winrt::unbox_value<Value>(appliedBinding.get().Source()) != value)
                {
                    winrt::Binding binding;
                    binding.Source(box_value(value));
                    binding.Mode(winrt::BindingMode::OneWay);
                    appliedBinding.set(binding);
                    SetBinding(property, binding);
                }
            }
            else
            {
                appliedBinding.set(nullptr);
            }
        };
        update(winrt::FrameworkElement::MinHeightProperty(), ownerImpl->GetDensityGroupHeaderMinHeight(), m_densityMinHeightBinding, m_styleSetsMinHeight);
        update(winrt::Control::PaddingProperty(), ownerImpl->GetDensityGroupHeaderPadding(), m_densityPaddingBinding, m_styleSetsPadding);
        update(winrt::Control::FontSizeProperty(), ownerImpl->GetCellFontSize(), m_densityFontSizeBinding, m_styleSetsFontSize);
    }
    RefreshContentAlignment();
}

void TableViewGroupHeader::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    m_isEnabledChangedRevoker.revoke();
    m_templateRootContextRequestedRevoker.revoke();
    m_gridLineBorder.set(GetTemplateChild(hstring{ s_GridLineBorderPartName }).try_as<winrt::Border>());
    m_rowChrome.set(GetTemplateChild(L"PART_RowChrome").try_as<winrt::FrameworkElement>());
    m_contentHost.set(GetTemplateChild(L"PART_ContentHost").try_as<winrt::Border>());
    m_contentPresenter.set(GetTemplateChild(L"PART_ContentPresenter").try_as<winrt::ContentPresenter>());
    m_expanderGutter.set(GetTemplateChild(hstring{ s_ExpanderGutterPartName }).try_as<winrt::Border>());

    // A group header's ContextFlyout (set through a TableViewGroupHeader Style) stays closed while
    // a cell is editing.
    m_templateRootContextRequestedRevoker = TableViewDetails::HookTemplateRootContextRequested(*this,
        [weakThis = get_weak()](auto&&, winrt::ContextRequestedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                if (auto const owner = strongThis->GetOwningTableView())
                {
                    winrt::get_self<TableView>(owner)->SuppressContextRequestWhileEditing(args);
                }
            }
        });

    // Keep CommonStates in sync with IsEnabled so Disabled activates when a consumer toggles it
    // at runtime, not only when it happens to be false at template time.
    m_isEnabledChangedRevoker = IsEnabledChanged(
        winrt::auto_revoke,
        [weakThis = get_weak()](auto const&, auto const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->UpdateVisualStates(true /* useTransitions */);
                strongThis->UpdateDefaultCountOpacity();
            }
        });

    UpdateVisualStates(false /* useTransitions */);
    RefreshGridLines();
}

void TableViewGroupHeader::UpdateDefaultCountOpacity()
{
    // The disabled brush already dims the default count; its Style opacity would dim it twice.
    if (IsEnabled())
    {
        if (auto const count = m_defaultCount.get())
        {
            count.ClearValue(winrt::UIElement::OpacityProperty());
        }
        return;
    }

    TrackDefaultCount(ContentTemplateRoot());
    if (auto const count = m_defaultCount.get())
    {
        count.Opacity(1.0);
    }
}

void TableViewGroupHeader::TrackDefaultCount(winrt::UIElement const& root)
{
    auto const contentTemplate = ContentTemplate();
    if (root == m_inspectedContentRoot.get() && contentTemplate == m_inspectedContentTemplate.get())
    {
        return;
    }

    if (auto const previous = m_defaultCount.get())
    {
        previous.ClearValue(winrt::UIElement::OpacityProperty());
        m_defaultCount.set(nullptr);
    }
    m_inspectedContentRoot = root ? winrt::make_weak(root) : nullptr;
    m_inspectedContentTemplate.set(contentTemplate);

    auto const rootElement = root.try_as<winrt::FrameworkElement>();
    if (rootElement && rootElement.Name() == L"TableViewDefaultGroupHeaderContentRoot"
        && TableView::IsDefaultGroupHeaderContentTemplate(*this))
    {
        m_defaultCount.set(rootElement.FindName(L"TableViewGroupHeaderItemCountText").try_as<winrt::TextBlock>());
    }
    RefreshContentAlignment();
}

void TableViewGroupHeader::SetTerminalBottomGridLineSuppression(bool suppress)
{
    // Re-applies on every call rather than returning early on an unchanged flag: the overlay is
    // also derived from BorderThickness, so a same-value push re-asserts it against the current
    // thickness.
    m_suppressBottomGridLine = suppress;
    RefreshGridLines();
}

void TableViewGroupHeader::RefreshGridLines()
{
    if (auto gridLineBorder = m_gridLineBorder.get())
    {
        auto thickness = BorderThickness();
        auto const owner = GetOwningTableView();
        if (m_suppressBottomGridLine ||
            (owner && owner.GridLinesVisibility() == winrt::TableViewGridLinesVisibility::None))
        {
            thickness.Bottom = 0.0;
        }
        if (gridLineBorder.BorderThickness() != thickness)
        {
            gridLineBorder.BorderThickness(thickness);
        }
        TableViewDetails::SetRowChromeSeparatorInsets(m_rowChrome.get(), thickness);
    }
    RefreshContentAlignment();
}

void TableViewGroupHeader::RefreshContentAlignment()
{
    if (auto const content = m_contentHost.get())
    {
        auto const insets = TableViewDetails::GetRowChromeSeparatorInsets(m_rowChrome.get());
        const double offset = (insets.Top - insets.Bottom) / 2.0;
        const auto controlPadding = Padding();
        const auto padding = TableViewDetails::OffsetContentPadding(controlPadding, offset);
        if (!m_contentPresenter.get() || !m_expanderGutter.get())
        {
            if (content.Padding() != padding)
            {
                content.Padding(padding);
            }
            return;
        }
        const winrt::Thickness horizontalPadding{ padding.Left, 0, padding.Right, 0 };
        if (content.Padding() != horizontalPadding)
        {
            content.Padding(horizontalPadding);
        }
        if (auto const gutter = m_expanderGutter.get())
        {
            const winrt::Thickness gutterPadding{ 0, padding.Top, 0, padding.Bottom };
            if (gutter.Padding() != gutterPadding)
            {
                gutter.Padding(gutterPadding);
            }
        }
        if (auto const presenter = m_contentPresenter.get())
        {
            winrt::Thickness textPadding{ 0, controlPadding.Top, 0, controlPadding.Bottom };
            auto const paddingBinding = GetBindingExpression(winrt::Control::PaddingProperty());
            if (auto const owner = GetOwningTableView();
                owner && m_defaultCount.get() && !m_styleSetsPadding &&
                paddingBinding && paddingBinding.ParentBinding() == m_densityPaddingBinding.get())
            {
                textPadding = winrt::get_self<TableView>(owner)->GetDensityGroupHeaderTextPadding();
            }
            textPadding = TableViewDetails::OffsetContentPadding(textPadding, offset);
            if (presenter.Padding() != textPadding)
            {
                presenter.Padding(textPadding);
            }
        }
    }
}

void TableViewGroupHeader::OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent)
{
    __super::OnContentChanged(oldContent, newContent);

    // PrepareGroupHeaderElement assigns the projection, then pushes expansion via the DPs. A
    // projection swapped in later must not be left reporting stale expansion, so seed it here.
    SyncExpansionToContent();
}

void TableViewGroupHeader::SyncExpansionToContent()
{
    if (auto const info = Content().try_as<winrt::TableViewGroupInfo>())
    {
        winrt::get_self<TableViewGroupInfo>(info)->SetExpansionInternal(IsExpandable(), IsExpanded());
    }
}

void TableViewGroupHeader::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto const property = args.Property();

    if (property == s_IsExpandedProperty || property == s_IsExpandableProperty)
    {
        // The DPs are authoritative: they drive the VisualStates and are mirrored onto the
        // projection here, so there is a single write path for expansion state.
        SyncExpansionToContent();
        UpdateVisualStates(true /* useTransitions */);
    }
}

void TableViewGroupHeader::RaiseExpandCollapseStateChanged(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState)
{
    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        return;
    }

    // Deferred: the header is not reachable from the window yet during a repeater re-prepare.
    auto const queue = DispatcherQueue();
    if (!queue)
    {
        return;
    }
    // Bound to the current group's element so a header recycled while queued does not raise it.
    queue.TryEnqueue([weakThis = get_weak(), groupBinding = m_groupBinding, oldState, newState]()
    {
        auto const strongThis = weakThis.get();
        auto const binding = groupBinding.get();
        if (!strongThis || !binding || !strongThis->m_groupEntry.get())
        {
            return;
        }

        auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*strongThis);
        auto const headerPeer = peer ? peer.try_as<winrt::TableViewGroupHeaderAutomationPeer>() : nullptr;
        if (!headerPeer || headerPeer.EventsSource() != binding)
        {
            return;
        }

        auto const self = winrt::get_self<TableViewGroupHeaderAutomationPeer>(headerPeer);
        if (self->ExpandCollapseState() == newState)
        {
            self->RaiseExpandCollapseAutomationEvent(oldState, newState);
        }
    });
}

void TableViewGroupHeader::UpdateVisualStates(bool useTransitions)
{
    // Glyph, band chrome and chevron visibility are all declared in the ControlTemplate and
    // selected by state -- no visual property is assigned from here.
    winrt::VisualStateManager::GoToState(
        *this,
        IsExpandable() ? L"Expandable" : L"NotExpandable",
        useTransitions);

    winrt::VisualStateManager::GoToState(
        *this,
        IsExpanded() ? L"Expanded" : L"Collapsed",
        useTransitions);

    winrt::VisualStateManager::GoToState(
        *this,
        !IsEnabled() ? L"Disabled" : (m_isPressed ? L"Pressed" : (m_isPointerOver ? L"PointerOver" : L"Normal")),
        useTransitions);
}

void TableViewGroupHeader::RequestToggle()
{
    if (!IsExpandable())
    {
        return;
    }

    auto const info = Content().try_as<winrt::TableViewGroupInfo>();
    auto const args = winrt::make<TableViewGroupHeaderToggleRequestedEventArgs>(info ? info.Key() : nullptr);

    // Raised inline, deliberately. The owner resolves the target group synchronously and
    // defers only the structural reshape, so nothing is destroyed while XAML is still
    // unwinding pointer dispatch.
    //
    // Posting HERE is what created the bug it appeared to prevent: by the time a queued raise
    // ran, a scroll could have recycled this header onto another group, so the toggle hit
    // the wrong one -- or the header had left the viewport and it was dropped. Resolution has to
    // happen while the gesture's row index is still current, which means the raise cannot be
    // deferred; only the reshape can.
    //
    // Strong self across the raise keeps THIS object alive if a handler releases it mid-invoke.
    // It does not protect XAML's in-flight pointer dispatch -- that safety comes from the owner
    // deferring the reshape, and from RequestToggle being the last statement in
    // OnPointerReleased so nothing here touches a member after the raise.
    auto const strongThis = get_strong();
    auto const self = strongThis.as<winrt::TableViewGroupHeader>();

    m_toggleRequestedEventSource(self, args);
}

void TableViewGroupHeader::RequestExpansion(bool expand)
{
    if (!IsExpandable())
    {
        return;
    }

    // Mirror the ExpandCollapse peer: pass the direction through to the owner unresolved (the
    // mutation is idempotent and applied on a later turn) and resolve through the stored owner
    // rather than an ancestor walk, so a header that has been recycled/unparented still works.
    if (auto const owner = GetOwningTableView())
    {
        auto const self = get_strong().as<winrt::TableViewGroupHeader>();
        winrt::get_self<TableView>(owner)->SetGroupExpansion(self, expand);
    }
}

void TableViewGroupHeader::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (!args.Handled() && IsExpandable())
    {
        const auto key = args.Key();
        const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;

        switch (key)
        {
        case winrt::VirtualKey::Enter:
        case winrt::VirtualKey::Space:
            args.Handled(true);
            m_gestureFocusState = winrt::FocusState::Keyboard;
            RequestToggle();
            break;

        case winrt::VirtualKey::Right:
            // Right expands in LTR, collapses in RTL -- the TreeViewItem / Expander convention.
            args.Handled(true);
            m_gestureFocusState = winrt::FocusState::Keyboard;
            RequestExpansion(!isRtl);
            break;

        case winrt::VirtualKey::Left:
            args.Handled(true);
            m_gestureFocusState = winrt::FocusState::Keyboard;
            RequestExpansion(isRtl);
            break;

        default:
            break;
        }
    }

    __super::OnKeyDown(args);
}

void TableViewGroupHeader::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);
    // Only the pressing pointer may cancel the press (as TableViewRow): another contact or the mouse
    // leaving must not drop an in-flight tap.
    if (m_isPressed && args.Pointer().PointerId() != m_pressedPointerId)
    {
        return;
    }
    m_isPointerOver = false;
    m_isPressed = false;
    m_pressedGroupIdentity.clear();
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::ResetPointerStateInternal()
{
    // Hover is left alone: it describes where the pointer is, not which group it acted on.
    m_pressedGroupIdentity.clear();
    if (m_isPressed)
    {
        m_isPressed = false;
        UpdateVisualStates(false /* useTransitions */);
    }
}

winrt::hstring TableViewGroupHeader::GetGroupIdentity()
{
    if (auto const owner = GetOwningTableView())
    {
        return winrt::get_self<TableView>(owner)->GetContainerIdentityInternal(*this);
    }
    return {};
}

void TableViewGroupHeader::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerPressed(args);

    // Left-button only, matching TableViewRow. Pointer events fire for secondary and middle
    // buttons too, and the matching release would reach RequestToggle -- so without this a
    // right-click on the band would toggle the group and swallow the context-menu gesture.
    // Despite the name, IsLeftButtonPressed covers the primary action regardless of device.
    if (!args.GetCurrentPoint(*this).Properties().IsLeftButtonPressed())
    {
        return;
    }

    m_isPressed = true;
    m_pressedPointerId = args.Pointer().PointerId();

    // A reshape between press and release can recycle this container onto another group.
    m_pressedGroupIdentity = GetGroupIdentity();

    // The band owns the gesture: mark it handled so a press on a group header never reaches the
    // row's selection handling.
    if (IsExpandable())
    {
        args.Handled(true);
    }

    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);

    const bool wasPressed = m_isPressed;
    auto const pressedIdentity = m_pressedGroupIdentity;
    m_isPressed = false;
    m_pressedGroupIdentity.clear();
    UpdateVisualStates(true /* useTransitions */);

    // Toggle on release-inside, the standard click semantic: a press that drags off the band
    // does not activate.
    if (!wasPressed || !m_isPointerOver || pressedIdentity != GetGroupIdentity())
    {
        return;
    }

    // Focus before the toggle so the toggle captures it for the restore after the reshape.
    // Focus on release, not press: leaving an open editor commits it and can reshape mid-gesture.
    if (Focus(winrt::FocusState::Pointer))
    {
        if (auto const owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->SetCellCursorActiveInternal(false);
        }
    }

    // The editor commit inside Focus() can reshape and recycle this header onto another group.
    if (pressedIdentity != GetGroupIdentity())
    {
        if (IsExpandable())
        {
            args.Handled(true);
        }
        return;
    }

    if (IsExpandable())
    {
        args.Handled(true);
        m_gestureFocusState = winrt::FocusState::Pointer;
        RequestToggle();
    }
}

void TableViewGroupHeader::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCaptureLost(args);
    if (m_isPressed && args.Pointer().PointerId() != m_pressedPointerId)
    {
        return;
    }
    m_isPressed = false;
    m_isPointerOver = false;
    m_pressedGroupIdentity.clear();
    UpdateVisualStates(true /* useTransitions */);
}
