#include "pch.h"
#include "common.h"
#include "TeachingTip.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "TeachingTipClosingEventArgs.h"
#include "TeachingTipClosedEventArgs.h"
#include "TeachingTipTestHooks.h"
#include "TeachingTipAutomationPeer.h"
#include "../ResourceHelper/Utils.h"
#include <enum_array.h>

TeachingTip::TeachingTip()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TeachingTip);
    SetDefaultStyleKey(this);
    EnsureProperties();
    m_automationNameChangedRevoker = RegisterPropertyChanged(*this, winrt::AutomationProperties::NameProperty(), { this, &TeachingTip::OnAutomationNameChanged });
    m_automationIdChangedRevoker = RegisterPropertyChanged(*this, winrt::AutomationProperties::AutomationIdProperty(), { this, &TeachingTip::OnAutomationIdChanged });
    SetValue(s_TemplateSettingsProperty, winrt::make<::TeachingTipTemplateSettings>());
}

TeachingTip::~TeachingTip()
{
    m_previouslyFocusedElement.set(nullptr);
}

winrt::AutomationPeer TeachingTip::OnCreateAutomationPeer()
{
    return winrt::make<TeachingTipAutomationPeer>(*this);
}

void TeachingTip::OnApplyTemplate()
{
    m_acceleratorKeyActivatedRevoker.revoke();
    m_effectiveViewportChangedRevoker.revoke();
    m_contentSizeChangedRevoker.revoke();
    m_closeButtonClickedRevoker.revoke();
    m_alternateCloseButtonClickedRevoker.revoke();
    m_actionButtonClickedRevoker.revoke();

    winrt::IControlProtected controlProtected{ *this };

    m_container.set(GetTemplateChildT<winrt::Border>(s_containerName, controlProtected));
    m_rootElement.set(m_container.get().Child());
    m_tailOcclusionGrid.set(GetTemplateChildT<winrt::Grid>(s_tailOcclusionGridName, controlProtected));
    m_contentRootGrid.set(GetTemplateChildT<winrt::Grid>(s_contentRootGridName, controlProtected));
    m_nonHeroContentRootGrid.set(GetTemplateChildT<winrt::Grid>(s_nonHeroContentRootGridName, controlProtected));
    m_heroContentBorder.set(GetTemplateChildT<winrt::Border>(s_heroContentBorderName, controlProtected));
    m_iconBorder.set(GetTemplateChildT<winrt::Border>(s_iconBorderName, controlProtected));
    m_actionButton.set(GetTemplateChildT<winrt::Button>(s_actionButtonName, controlProtected));
    m_alternateCloseButton.set(GetTemplateChildT<winrt::Button>(s_alternateCloseButtonName, controlProtected));
    m_closeButton.set(GetTemplateChildT<winrt::Button>(s_closeButtonName, controlProtected));
    m_tailEdgeBorder.set(GetTemplateChildT<winrt::Grid>(s_tailEdgeBorderName, controlProtected));
    m_tailPolygon.set(GetTemplateChildT<winrt::Polygon>(s_tailPolygonName, controlProtected));

    m_acceleratorKeyActivatedRevoker = Dispatcher().AcceleratorKeyActivated(winrt::auto_revoke, { this, &TeachingTip::OnF6AcceleratorKeyClicked });

    if (auto && container = m_container.get())
    {
        container.Child(nullptr);
    }

    if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
    {
        m_contentSizeChangedRevoker = tailOcclusionGrid.SizeChanged(winrt::auto_revoke, {
            [this](auto const&, auto const&)
            {
                UpdateSizeBasedTemplateSettings();
                // Reset the currentEffectivePlacementMode so that the tail will be updated for the new size as well.
                m_currentEffectiveTipPlacementMode = winrt::TeachingTipPlacementMode::Auto;
                TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
                if (IsOpen())
                {
                    PositionPopup();
                }
                {
                    auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
                    if (auto&& expandAnimation = m_expandAnimation.get())
                    {
                        expandAnimation.SetScalarParameter(L"Width", static_cast<float>(tailOcclusionGrid.ActualWidth()));
                        expandAnimation.SetScalarParameter(L"Height", static_cast<float>(tailOcclusionGrid.ActualHeight()));
                    }
                    if (auto&& contractAnimation = m_contractAnimation.get())
                    {
                        contractAnimation.SetScalarParameter(L"Width", static_cast<float>(tailOcclusionGrid.ActualWidth()));
                        contractAnimation.SetScalarParameter(L"Height", static_cast<float>(tailOcclusionGrid.ActualHeight()));
                    }
                }
            }
        });
    }

    if (auto && contentRootGrid = m_contentRootGrid.get())
    {
        winrt::AutomationProperties::SetLocalizedLandmarkType(contentRootGrid, ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipCustomLandmarkName));
    }

    if (auto&& closeButton = m_closeButton.get())
    {
        m_closeButtonClickedRevoker = closeButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnCloseButtonClicked });
    }
    if (auto&& alternateCloseButton = m_alternateCloseButton.get())
    {
        winrt::AutomationProperties::SetName(alternateCloseButton, ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipAlternateCloseButtonName));
        winrt::ToolTip tooltip = winrt::ToolTip();
        tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipAlternateCloseButtonTooltip)));
        winrt::ToolTipService::SetToolTip(alternateCloseButton, tooltip);
        m_alternateCloseButtonClickedRevoker = alternateCloseButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnCloseButtonClicked });
    }
    if (auto&& actionButton = m_actionButton.get())
    {
        m_actionButtonClickedRevoker = actionButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnActionButtonClicked });
    }
    UpdateButtonsState();

    OnIconSourceChanged();

    EstablishShadows();

    m_isTemplateApplied = true;
}

void TeachingTip::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsOpenProperty)
    {
        OnIsOpenChanged();
    }
    else if (property == s_TargetProperty)
    {
        OnTargetChanged();
    }
    else if (property == s_ActionButtonContentProperty ||
        property == s_CloseButtonContentProperty)
    {
        UpdateButtonsState();
    }
    else if (property == s_PlacementMarginProperty)
    {
        OnPlacementMarginChanged();
    }
    else if (property == s_IsLightDismissEnabledProperty)
    {
        OnIsLightDismissEnabledChanged();
    }
    else if (property == s_ShouldConstrainToRootBoundsProperty)
    {
        OnShouldConstrainToRootBoundsChanged();
    }
    else if (property == s_TailVisibilityProperty)
    {
        OnTailVisibilityChanged();
    }
    else if (property == s_PreferredPlacementProperty)
    {
        if (IsOpen())
        {
            PositionPopup();
        }
    }
    else if (property == s_HeroContentPlacementProperty)
    {
        OnHeroContentPlacementChanged();
    }
    else if (property == s_IconSourceProperty)
    {
        OnIconSourceChanged();
    }
    else if (property == s_TitleProperty)
    {
        SetPopupAutomationProperties();
    }

}

void TeachingTip::OnContentChanged(const winrt::IInspectable& oldContent, const winrt::IInspectable& newContent)
{
    if (newContent)
    {
        winrt::VisualStateManager::GoToState(*this, L"Content"sv, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoContent"sv, false);
    }
}

void TeachingTip::SetPopupAutomationProperties()
{
    if (auto && popup = m_popup.get())
    {
        auto name = winrt::AutomationProperties::GetName(*this);
        if (name.empty())
        {
            name = Title();
        }
        winrt::AutomationProperties::SetName(popup, name);

        winrt::AutomationProperties::SetAutomationId(popup, winrt::AutomationProperties::GetAutomationId(*this));
    }
}

// Playing a closing animation when the Teaching Tip is closed via light dismiss requires this work around.
// This is because there is no event that occurs when a popup is closing due to light dismiss so we have no way to intercept
// the close and play our animation first. To work around this we've created a second popup which has no content and sits
// underneath the teaching tip and is put into light dismiss mode instead of the primary popup. Then when this popup closes
// due to light dismiss we know we are supposed to close the primary popup as well. To ensure that this popup does not block
// interaction to the primary popup we need to make sure that the LightDismissIndicatorPopup is always opened first, so that
// it is Z ordered underneath the primary popup.
void TeachingTip::CreateLightDismissIndicatorPopup()
{
    if (!m_lightDismissIndicatorPopup)
    {
        auto popup = winrt::Popup();
        // A Popup needs contents to open, so set a child that doesn't do anything.
        auto grid = winrt::Grid();
        popup.Child(grid);

        m_lightDismissIndicatorPopup.set(popup);
    }
}

bool TeachingTip::UpdateTail()
{
    // An effective placement of auto indicates that no tail should be shown.
    auto [placement, tipDoesNotFit] = DetermineEffectivePlacement();
    m_currentEffectiveTailPlacementMode = placement;
    auto&& tailVisibility = TailVisibility();
    if (tailVisibility == winrt::TeachingTipTailVisibility::Collapsed || (!m_target && tailVisibility != winrt::TeachingTipTailVisibility::Visible))
    {
        m_currentEffectiveTailPlacementMode = winrt::TeachingTipPlacementMode::Auto;
    }

    if (placement != m_currentEffectiveTipPlacementMode)
    {
        m_currentEffectiveTipPlacementMode = placement;
        TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    }

    auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
    auto&& tailEdgeBorder = m_tailEdgeBorder.get();

    float height = static_cast<float>(tailOcclusionGrid.ActualHeight());
    float width = static_cast<float>(tailOcclusionGrid.ActualWidth());

    auto columnDefinitions = tailOcclusionGrid.ColumnDefinitions();
    auto rowDefinitions = tailOcclusionGrid.RowDefinitions();

    float firstColumnWidth = columnDefinitions.Size() > 0 ? static_cast<float>(columnDefinitions.GetAt(0).ActualWidth()) : 0.0f;
    float secondColumnWidth = columnDefinitions.Size() > 1 ? static_cast<float>(columnDefinitions.GetAt(1).ActualWidth()) : 0.0f;
    float nextToLastColumnWidth = columnDefinitions.Size() > 1 ? static_cast<float>(columnDefinitions.GetAt(columnDefinitions.Size() - 2).ActualWidth()) : 0.0f;
    float lastColumnWidth = columnDefinitions.Size() > 0 ? static_cast<float>(columnDefinitions.GetAt(columnDefinitions.Size() - 1).ActualWidth()) : 0.0f;

    float firstRowHeight = rowDefinitions.Size() > 0 ? static_cast<float>(rowDefinitions.GetAt(0).ActualHeight()) : 0.0f;
    float secondRowHeight = rowDefinitions.Size() > 1 ? static_cast<float>(rowDefinitions.GetAt(1).ActualHeight()) : 0.0f;
    float nextToLastRowHeight = rowDefinitions.Size() > 1 ? static_cast<float>(rowDefinitions.GetAt(rowDefinitions.Size() - 2).ActualHeight()) : 0.0f;
    float lastRowHeight = rowDefinitions.Size() > 0 ? static_cast<float>(rowDefinitions.GetAt(rowDefinitions.Size() - 1).ActualHeight()) : 0.0f;

    UpdateSizeBasedTemplateSettings();

    switch (m_currentEffectiveTailPlacementMode)
    {
    // An effective placement of auto means the tip should not display a tail.
    case winrt::TeachingTipPlacementMode::Auto:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width / 2, height / 2, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Untargeted"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Top:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width / 2, height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ (width / 2) - firstColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Top"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Bottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width / 2, firstRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ (width / 2) - firstColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"Bottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Left:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - lastColumnWidth, (height / 2), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, (height / 2) - firstRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Left"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Right:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth, height / 2, 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, (height / 2) - firstRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Right"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::TopRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::TopLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, firstRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), firstRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - lastColumnWidth,  height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f,  height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"LeftTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - lastColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"LeftBottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth, height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"RightTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"RightBottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Center:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width / 2, height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ (width / 2) - firstColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Center"sv, false);
        break;

    default:
        break;
    }

    return tipDoesNotFit;
}

void TeachingTip::PositionPopup()
{
    bool tipDoesNotFit = false;
    if (m_target)
    {
        tipDoesNotFit = PositionTargetedPopup();
    }
    else
    {
        tipDoesNotFit = PositionUntargetedPopup();
    }
    if (tipDoesNotFit)
    {
        IsOpen(false);
    }

    TeachingTipTestHooks::NotifyOffsetChanged(*this);
}

bool TeachingTip::PositionTargetedPopup()
{
    bool tipDoesNotFit = UpdateTail();
    auto offset = PlacementMargin();

    auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
    double tipHeight = tailOcclusionGrid.ActualHeight();
    double tipWidth = tailOcclusionGrid.ActualWidth();

    auto&& popup = m_popup.get();
    // Depending on the effective placement mode of the tip we use a combination of the tip's size, the target's position within the app, the target's
    // size, and the target offset property to determine the appropriate vertical and horizontal offsets of the popup that the tip is contained in.
    switch (m_currentEffectiveTipPlacementMode)
    {
    case winrt::TeachingTipPlacementMode::Top:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y - tipHeight - offset.Top);
        popup.HorizontalOffset((((m_currentTargetBoundsInCoreWindowSpace.X * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width - tipWidth) / 2.0f));
        break;

    case winrt::TeachingTipPlacementMode::Bottom:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y + m_currentTargetBoundsInCoreWindowSpace.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset((((m_currentTargetBoundsInCoreWindowSpace.X * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width - tipWidth) / 2.0f));
        break;

    case winrt::TeachingTipPlacementMode::Left:
        popup.VerticalOffset(((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height - tipHeight) / 2.0f);
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::Right:
        popup.VerticalOffset(((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height - tipHeight) / 2.0f);
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X + m_currentTargetBoundsInCoreWindowSpace.Width + static_cast<float>(offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopRight:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y - tipHeight - offset.Top);
        popup.HorizontalOffset(((((m_currentTargetBoundsInCoreWindowSpace.X  * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width) / 2.0f) - MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::TopLeft:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y - tipHeight - offset.Top);
        popup.HorizontalOffset(((((m_currentTargetBoundsInCoreWindowSpace.X  * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width) / 2.0f) - tipWidth + MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::BottomRight:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y + m_currentTargetBoundsInCoreWindowSpace.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset(((((m_currentTargetBoundsInCoreWindowSpace.X * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width) / 2.0f) - MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::BottomLeft:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y + m_currentTargetBoundsInCoreWindowSpace.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset(((((m_currentTargetBoundsInCoreWindowSpace.X * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width) / 2.0f) - tipWidth + MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::LeftTop:
        popup.VerticalOffset((((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height) / 2.0f) - tipHeight + MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::LeftBottom:
        popup.VerticalOffset((((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height) / 2.0f) - MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::RightTop:
        popup.VerticalOffset((((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height) / 2.0f) - tipHeight + MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X + m_currentTargetBoundsInCoreWindowSpace.Width + static_cast<float>(offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::RightBottom:
        popup.VerticalOffset((((m_currentTargetBoundsInCoreWindowSpace.Y * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Height) / 2.0f) - MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBoundsInCoreWindowSpace.X + m_currentTargetBoundsInCoreWindowSpace.Width + static_cast<float>(offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Center:
        popup.VerticalOffset(m_currentTargetBoundsInCoreWindowSpace.Y + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f) - tipHeight - offset.Top);
        popup.HorizontalOffset((((m_currentTargetBoundsInCoreWindowSpace.X * 2.0f) + m_currentTargetBoundsInCoreWindowSpace.Width - tipWidth) / 2.0f));
        break;

    default:
        MUX_FAIL_FAST();
    }
    return tipDoesNotFit;
}

bool TeachingTip::PositionUntargetedPopup()
{
    auto windowBoundsInCoreWindowSpace = GetEffectiveWindowBoundsInCoreWindowSpace(GetWindowBounds());

    auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
    double finalTipHeight = tailOcclusionGrid.ActualHeight();
    double finalTipWidth = tailOcclusionGrid.ActualWidth();

    bool tipDoesNotFit = UpdateTail();

    auto offset = PlacementMargin();

    // Depending on the effective placement mode of the tip we use a combination of the tip's size, the window's size, and the target
    // offset property to determine the appropriate vertical and horizontal offsets of the popup that the tip is contained in.
    auto&& popup = m_popup.get();
    switch (PreferredPlacement())
    {
    case winrt::TeachingTipPlacementMode::Auto:
    case winrt::TeachingTipPlacementMode::Bottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.X, windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Top:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.Y, offset.Top));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.X, windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Left:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.Y, windowBoundsInCoreWindowSpace.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.X, offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::Right:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.Y, windowBoundsInCoreWindowSpace.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopRight:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.Y, offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopLeft:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.Y, offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.X, offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::BottomRight:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::BottomLeft:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.X, offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.Y, offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.X, offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.X, offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::RightTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(windowBoundsInCoreWindowSpace.Y, offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::RightBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Center:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.Y, windowBoundsInCoreWindowSpace.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBoundsInCoreWindowSpace.X, windowBoundsInCoreWindowSpace.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    default:
        MUX_FAIL_FAST();
    }

    return tipDoesNotFit;
}

void TeachingTip::UpdateSizeBasedTemplateSettings()
{
    auto templateSettings = winrt::get_self<::TeachingTipTemplateSettings>(TemplateSettings());
    auto&& contentRootGrid = m_contentRootGrid.get();
    auto width = contentRootGrid.ActualWidth();
    auto height = contentRootGrid.ActualHeight();
    auto floatWidth = static_cast<float>(width);
    auto floatHeight = static_cast<float>(height);
    switch (m_currentEffectiveTailPlacementMode)
    {
    case winrt::TeachingTipPlacementMode::Top:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Bottom:
        templateSettings->TopRightHighlightMargin(BottomPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Left:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Right:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::TopLeft:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::TopRight:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomLeft:
        templateSettings->TopRightHighlightMargin(BottomLeftPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomLeftPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomRight:
        templateSettings->TopRightHighlightMargin(BottomRightPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomRightPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Auto:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Center:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    }
}

void TeachingTip::UpdateButtonsState()
{
    auto actionContent = ActionButtonContent();
    auto closeContent = CloseButtonContent();
    bool isLightDismiss = IsLightDismissEnabled();
    if (actionContent && closeContent)
    {
        winrt::VisualStateManager::GoToState(*this, L"BothButtonsVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
    }
    else if (actionContent && isLightDismiss)
    {
        winrt::VisualStateManager::GoToState(*this, L"ActionButtonVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
    }
    else if (actionContent)
    {
        winrt::VisualStateManager::GoToState(*this, L"ActionButtonVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"HeaderCloseButton"sv, false);
    }
    else if (closeContent)
    {
        winrt::VisualStateManager::GoToState(*this, L"CloseButtonVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
    }
    else if (isLightDismiss)
    {
        winrt::VisualStateManager::GoToState(*this, L"NoButtonsVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoButtonsVisible"sv, false);
        winrt::VisualStateManager::GoToState(*this, L"HeaderCloseButton"sv, false);
    }
}

void TeachingTip::UpdateDynamicHeroContentPlacementToTop()
{
    if (HeroContentPlacement() == winrt::TeachingTipHeroContentPlacementMode::Auto)
    {
        winrt::VisualStateManager::GoToState(*this, L"HeroContentTop"sv, false);
        if (m_currentHeroContentEffectivePlacementMode != winrt::TeachingTipHeroContentPlacementMode::Top)
        {
            m_currentHeroContentEffectivePlacementMode = winrt::TeachingTipHeroContentPlacementMode::Top;
            TeachingTipTestHooks::NotifyEffectiveHeroContentPlacementChanged(*this);
        }
    }
}

void TeachingTip::UpdateDynamicHeroContentPlacementToBottom()
{
    if (HeroContentPlacement() == winrt::TeachingTipHeroContentPlacementMode::Auto)
    {
        winrt::VisualStateManager::GoToState(*this, L"HeroContentBottom"sv, false);
        if (m_currentHeroContentEffectivePlacementMode != winrt::TeachingTipHeroContentPlacementMode::Bottom)
        {
            m_currentHeroContentEffectivePlacementMode = winrt::TeachingTipHeroContentPlacementMode::Bottom;
            TeachingTipTestHooks::NotifyEffectiveHeroContentPlacementChanged(*this);
        }
    }
}

void TeachingTip::OnIsOpenChanged()
{
    if (IsOpen())
    {
        //Reset the close reason to the default value of programmatic.
        m_lastCloseReason = winrt::TeachingTipCloseReason::Programmatic;

        m_currentBoundsInCoreWindowSpace = this->TransformToVisual(nullptr).TransformBounds({
            0.0,
            0.0,
            static_cast<float>(this->ActualWidth()),
            static_cast<float>(this->ActualHeight())
            });

        if (auto&& target = m_target.get())
        {
            SetViewportChangedEvent();
            m_currentTargetBoundsInCoreWindowSpace = target.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualWidth()),
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualHeight())
                });
        }

        if (!m_lightDismissIndicatorPopup)
        {
            CreateLightDismissIndicatorPopup();
        }
        OnIsLightDismissEnabledChanged();

        if (!m_contractAnimation)
        {
            CreateContractAnimation();
        }
        if (!m_expandAnimation)
        {
            CreateExpandAnimation();
        }

        // We are about to begin the process of trying to open the teaching tip, so notify that we are no longer idle.
        if (m_isIdle)
        {
            m_isIdle = false;
            TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
        }

        if (!m_isTemplateApplied)
        {
            this->ApplyTemplate();
        }

        if (!m_popup || m_createNewPopupOnOpen)
        {
            auto popup = winrt::Popup();
            m_popupOpenedRevoker = popup.Opened(winrt::auto_revoke, { this, &TeachingTip::OnPopupOpened });
            m_popupClosedRevoker = popup.Closed(winrt::auto_revoke, { this, &TeachingTip::OnPopupClosed });
            popup.ShouldConstrainToRootBounds(ShouldConstrainToRootBounds());
            m_popup.set(popup);
            SetPopupAutomationProperties();
            m_createNewPopupOnOpen = false;
        }

        // If the tip is not going to open because it does not fit we need to make sure that
        // the open, closing, closed life cycle still fires so that we don't cause apps to leak
        // that depend on this sequence.
        auto [ignored, tipDoesNotFit] = DetermineEffectivePlacement();
        if (tipDoesNotFit)
        {
            RaiseClosingEvent(false);
            auto closedArgs = winrt::make_self<TeachingTipClosedEventArgs>();
            closedArgs->Reason(m_lastCloseReason);
            m_closedEventSource(*this, *closedArgs);
            IsOpen(false);
        }
        else
        {
            auto&& popup = m_popup.get();
            if (!popup.IsOpen())
            {
                popup.Child(m_rootElement.get());
                m_lightDismissIndicatorPopup.get().IsOpen(true);
                popup.IsOpen(true);
            }
            else
            {
                // We have become Open but our popup was already open. This can happen when a close is canceled by the closing event, so make sure the idle status is correct.
                if (!m_isIdle && !m_isExpandAnimationPlaying && !m_isContractAnimationPlaying)
                {
                    m_isIdle = true;
                    TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
                }
            }
        }
    }
    else
    {
        if (auto&& popup = m_popup.get())
        {
            if (popup.IsOpen())
            {
                // We are about to begin the process of trying to close the teaching tip, so notify that we are no longer idle.
                if (m_isIdle)
                {
                    m_isIdle = false;
                    TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
                }
                RaiseClosingEvent(true);
            }
            else
            {
                // We have become not Open but our popup was already not open. Lets make sure the idle status is correct.
                if (!m_isIdle && !m_isExpandAnimationPlaying && !m_isContractAnimationPlaying)
                {
                    m_isIdle = true;
                    TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
                }
            }
        }

        m_currentEffectiveTipPlacementMode = winrt::TeachingTipPlacementMode::Auto;
        TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    }
    TeachingTipTestHooks::NotifyOpenedStatusChanged(*this);
}

void TeachingTip::OnTailVisibilityChanged()
{
    UpdateTail();
}

void TeachingTip::OnIconSourceChanged()
{
    auto templateSettings = winrt::get_self<::TeachingTipTemplateSettings>(TemplateSettings());
    if (auto source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, L"Icon"sv, false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, L"NoIcon"sv, false);
    }
}

void TeachingTip::OnPlacementMarginChanged()
{
    if (IsOpen())
    {
        PositionPopup();
    }
}

void TeachingTip::OnIsLightDismissEnabledChanged()
{
    if (IsLightDismissEnabled())
    {
        winrt::VisualStateManager::GoToState(*this, L"LightDismiss"sv, false);
        if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
        {
            lightDismissIndicatorPopup.IsLightDismissEnabled(true);
            m_lightDismissIndicatorPopupClosedRevoker = lightDismissIndicatorPopup.Closed(winrt::auto_revoke, { this, &TeachingTip::OnLightDismissIndicatorPopupClosed });
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NormalDismiss"sv, false);
        if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
        {
            lightDismissIndicatorPopup.IsLightDismissEnabled(false);
        }
        m_lightDismissIndicatorPopupClosedRevoker.revoke();
    }
    UpdateButtonsState();
}

void TeachingTip::OnShouldConstrainToRootBoundsChanged()
{
    // ShouldConstrainToRootBounds is a property that can only be set on a popup before it is opened.
    // If we have opened the tip's popup and then this property changes we will need to discard the old popup
    // and replace it with a new popup.  This variable indicates this state.
    m_createNewPopupOnOpen = true;
}

void TeachingTip::OnHeroContentPlacementChanged()
{
    switch (HeroContentPlacement())
    {
    case winrt::TeachingTipHeroContentPlacementMode::Auto:
        break;
    case winrt::TeachingTipHeroContentPlacementMode::Top:
        winrt::VisualStateManager::GoToState(*this, L"HeroContentTop"sv, false);
        if (m_currentHeroContentEffectivePlacementMode != winrt::TeachingTipHeroContentPlacementMode::Top)
        {
            m_currentHeroContentEffectivePlacementMode = winrt::TeachingTipHeroContentPlacementMode::Top;
            TeachingTipTestHooks::NotifyEffectiveHeroContentPlacementChanged(*this);
        }
        break;
    case winrt::TeachingTipHeroContentPlacementMode::Bottom:
        winrt::VisualStateManager::GoToState(*this, L"HeroContentBottom"sv, false);
        if (m_currentHeroContentEffectivePlacementMode != winrt::TeachingTipHeroContentPlacementMode::Bottom)
        {
            m_currentHeroContentEffectivePlacementMode = winrt::TeachingTipHeroContentPlacementMode::Bottom;
            TeachingTipTestHooks::NotifyEffectiveHeroContentPlacementChanged(*this);
        }
        break;
    }

    // Setting m_currentEffectiveTipPlacementMode to auto ensures that the next time position popup is called we'll rerun the DetermineEffectivePlacement
    // algorithm. If we did not do this and the popup was opened the algorithm would maintain the current effective placement mode, which we don't want
    // since the hero content placement contributes to the choice of tip placement mode.
    m_currentEffectiveTipPlacementMode = winrt::TeachingTipPlacementMode::Auto;
    TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    if (IsOpen())
    {
        PositionPopup();
    }
}

void TeachingTip::OnF6AcceleratorKeyClicked(const winrt::CoreDispatcher&, const winrt::AcceleratorKeyEventArgs& args)
{
    if (args.VirtualKey() == winrt::VirtualKey::F6 && args.EventType() == winrt::CoreAcceleratorKeyEventType::KeyDown)
    {
        //  Logging usage telemetry
        if (m_hasF6BeenInvoked)
        {
            __RP_Marker_ClassMemberById(RuntimeProfiler::ProfId_TeachingTip, RuntimeProfiler::ProfMemberId_TeachingTip_F6AccessKey_SubsequentInvocation);
        }
        else
        {
            __RP_Marker_ClassMemberById(RuntimeProfiler::ProfId_TeachingTip, RuntimeProfiler::ProfMemberId_TeachingTip_F6AccessKey_FirstInvocation);
            m_hasF6BeenInvoked = true;
        }

        auto const focusFromContent = [this, args]()
        {
            auto parent = winrt::FocusManager::GetFocusedElement();
            auto const rootElement = m_rootElement.get();
            while (parent)
            {
                if (auto parentAsUIElement = parent.try_as<winrt::UIElement>())
                {
                    if (parentAsUIElement == rootElement)
                    {
                        return true;
                    }
                }
                parent = winrt::VisualTreeHelper::GetParent(parent.try_as<winrt::DependencyObject>());
            }
            return false;
        }();

        if (focusFromContent)
        {
            if (auto&& previouslyFocusedElement = m_previouslyFocusedElement.get())
            {
                previouslyFocusedElement.Focus(winrt::FocusState::Keyboard);
            }
        }
        else
        {
            const winrt::Button f6Button = [this]() -> winrt::Button
            {
                auto firstButton = m_closeButton.get();
                auto secondButton = m_alternateCloseButton.get();
                if (CloseButtonContent())
                {
                    std::swap(firstButton, secondButton);
                }
                if (firstButton && firstButton.Visibility() == winrt::Visibility::Visible)
                {
                    return firstButton;
                }
                else if (secondButton && secondButton.Visibility() == winrt::Visibility::Visible)
                {
                    return secondButton;
                }
                return nullptr;
            }();

            if (f6Button)
            {
                m_closeButtonGettingFocusFromF6Revoker = f6Button.GettingFocus(winrt::auto_revoke, { this, &TeachingTip::OnCloseButtonGettingFocusFromF6 });
                f6Button.Focus(winrt::FocusState::Keyboard);
            }
        }
    }
}

void TeachingTip::OnCloseButtonGettingFocusFromF6(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args)
{
    m_previouslyFocusedElement.set(args.OldFocusedElement().try_as<winrt::Control>());
    m_closeButtonGettingFocusFromF6Revoker.revoke();
}

void TeachingTip::OnAutomationNameChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    SetPopupAutomationProperties();
}

void TeachingTip::OnAutomationIdChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    SetPopupAutomationProperties();
}

void TeachingTip::OnCloseButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    m_closeButtonClickEventSource(*this, nullptr);
    m_lastCloseReason = winrt::TeachingTipCloseReason::CloseButton;
    IsOpen(false);
}

void TeachingTip::OnActionButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    m_actionButtonClickEventSource(*this, nullptr);
}

void TeachingTip::OnPopupOpened(const winrt::IInspectable&, const winrt::IInspectable&)
{
    StartExpandToOpen();

    if (auto teachingTipPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::TeachingTipAutomationPeer>())
    {
        auto appName = []()
        {
            if (auto && package = winrt::ApplicationModel::Package::Current())
            {
                return package.DisplayName();
            }
            return winrt::hstring{};
        }();

        auto notificationString = [this, appName]()
        {
            if (!appName.empty())
            {
                return StringUtil::FormatString(
                    ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipNotification),
                    appName.data(),
                    winrt::AutomationProperties::GetName(m_popup.get()).data());
            }
            else
            {
                return StringUtil::FormatString(
                    ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipNotificationWithoutAppName),
                    winrt::AutomationProperties::GetName(m_popup.get()).data());
            }
        }();

        winrt::get_self<TeachingTipAutomationPeer>(teachingTipPeer)->RaiseWindowOpenedEvent(notificationString);
    }
}

void TeachingTip::OnPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&)
{
    m_lightDismissIndicatorPopup.get().IsOpen(false);
    m_popup.get().Child(nullptr);
    auto myArgs = winrt::make_self<TeachingTipClosedEventArgs>();
    myArgs->Reason(m_lastCloseReason);
    m_closedEventSource(*this, *myArgs);

    //If we were closed by the close button and we have tracked a previously focused element because F6 was used
    //To give the tip focus, then we return focus when the popup closes.
    if (m_lastCloseReason == winrt::TeachingTipCloseReason::CloseButton)
    {
        if (auto&& perviouslyFocusedElement = m_previouslyFocusedElement.get())
        {
            perviouslyFocusedElement.Focus(winrt::FocusState::Keyboard);
        }
    }
    m_previouslyFocusedElement.set(nullptr);

    if (auto teachingTipPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::TeachingTipAutomationPeer>())
    {
        winrt::get_self<TeachingTipAutomationPeer>(teachingTipPeer)->RaiseWindowClosedEvent();
    }
}

void TeachingTip::OnLightDismissIndicatorPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (IsOpen())
    {
        m_lastCloseReason = winrt::TeachingTipCloseReason::LightDismiss;
    }
    IsOpen(false);
}

void TeachingTip::OnTailOcclusionGridLoaded(const winrt::IInspectable&, const winrt::IInspectable&)
{
    StartExpandToOpen();
    m_tailOcclusionGridLoadedRevoker.revoke();
}


void TeachingTip::RaiseClosingEvent(bool attachDeferralCompletedHandler)
{
    auto args = winrt::make_self<TeachingTipClosingEventArgs>();
    args->Reason(m_lastCloseReason);

    if (attachDeferralCompletedHandler)
    {
        com_ptr<TeachingTip> strongThis = get_strong();
        winrt::DeferralCompletedHandler instance{ [strongThis, args]()
            {
                strongThis->CheckThread();
                if (!args->Cancel())
                {
                    strongThis->ClosePopupWithAnimationIfAvailable();
                }
                else
                {
                    // The developer has changed the Cancel property to true, indicating that they wish to Cancel the
                    // closing of this tip, so we need to revert the IsOpen property to true.
                    strongThis->IsOpen(true);
                }
            }
        };

        args->SetDeferral(instance);

        args->IncrementDeferralCount();
        m_closingEventSource(*this, *args);
        args->DecrementDeferralCount();
    }
    else
    {
        m_closingEventSource(*this, *args);
    }
}

void TeachingTip::ClosePopupWithAnimationIfAvailable()
{
    if (m_popup && m_popup.get().IsOpen())
    {
        if (SharedHelpers::IsRS5OrHigher())
        {
            StartContractToClose();
        }
        else
        {
            ClosePopup();
        }

        // Under normal circumstances we would have launched an animation just now, if we did not then we should make sure
        // that the idle state is correct.
        if (!m_isContractAnimationPlaying && !m_isIdle && !m_isExpandAnimationPlaying)
        {
            m_isIdle = true;
            TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
        }
    }
}

void TeachingTip::ClosePopup()
{
    if (auto&& popup = m_popup.get())
    {
        popup.IsOpen(false);
    }
    if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
    {
        lightDismissIndicatorPopup.IsOpen(false);
    }
    if (auto && tailOcclusionGrid = m_tailOcclusionGrid.get())
    {
        if (SharedHelpers::IsRS5OrHigher())
        {
            // A previous close animation may have left the rootGrid's scale at a very small value and if this teaching tip
            // is shown again then its text would be rasterized at this small scale and blown up ~20x. To fix this we have to
            // reset the scale after the popup has closed so that if the teaching tip is re-shown the render pass does not use the
            // small scale.
            tailOcclusionGrid.Scale({ 1.0f,1.0f,1.0f });
        }
    }
}

void TeachingTip::OnTargetChanged()
{
    m_targetLayoutUpdatedRevoker.revoke();
    m_targetEffectiveViewportChangedRevoker.revoke();

    m_target.set(Target());

    if (IsOpen())
    {
        if (auto&& target = m_target.get())
        {
            m_currentTargetBoundsInCoreWindowSpace = target.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(target.ActualWidth()),
                static_cast<float>(target.ActualHeight())
            });
        }
        SetViewportChangedEvent();
        PositionPopup();
    }
}

void TeachingTip::SetViewportChangedEvent()
{
    if (m_tipFollowsTarget)
    {
        if (auto targetAsFE = m_target.get())
        {
            // EffectiveViewPortChanged is only available on RS5 and higher.
            if (SharedHelpers::IsRS5OrHigher())
            {
                m_targetEffectiveViewportChangedRevoker = targetAsFE.EffectiveViewportChanged(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
                m_effectiveViewportChangedRevoker = this->EffectiveViewportChanged(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
            }
            else
            {
                m_targetLayoutUpdatedRevoker = targetAsFE.LayoutUpdated(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
            }
        }
    }
}

void TeachingTip::RevokeViewportChangedEvent()
{
    m_targetEffectiveViewportChangedRevoker.revoke();
    m_effectiveViewportChangedRevoker.revoke();
    m_targetLayoutUpdatedRevoker.revoke();
}

void TeachingTip::TargetLayoutUpdated(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (IsOpen())
    {
        if (auto&& target = m_target.get())
        {
            auto newTargetBounds = target.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualWidth()),
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualHeight())
            });

            auto newCurrentBounds = this->TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(this->ActualWidth()),
                static_cast<float>(this->ActualHeight())
            });

            if (newTargetBounds != m_currentTargetBoundsInCoreWindowSpace || newCurrentBounds != m_currentBoundsInCoreWindowSpace)
            {
                m_currentBoundsInCoreWindowSpace = newCurrentBounds;
                m_currentTargetBoundsInCoreWindowSpace = newTargetBounds;
                PositionPopup();
            }
        }
    }
}

void TeachingTip::CreateExpandAnimation()
{
    auto compositor = winrt::Window::Current().Compositor();
    if (!m_expandEasingFunction)
    {
        m_expandEasingFunction.set(compositor.CreateCubicBezierEasingFunction(s_expandAnimationEasingCurveControlPoint1, s_expandAnimationEasingCurveControlPoint2));
    }
    auto expandAnimation = compositor.CreateVector3KeyFrameAnimation();
    if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
    {
        expandAnimation.SetScalarParameter(L"Width", static_cast<float>(tailOcclusionGrid.ActualWidth()));
        expandAnimation.SetScalarParameter(L"Height", static_cast<float>(tailOcclusionGrid.ActualHeight()));
    }
    else
    {
        expandAnimation.SetScalarParameter(L"Width", s_defaultTipHeightAndWidth);
        expandAnimation.SetScalarParameter(L"Height", s_defaultTipHeightAndWidth);
    }

    auto&& expandEasingFunction = m_expandEasingFunction.get();
    expandAnimation.InsertExpressionKeyFrame(0.0f, L"Vector3(Min(0.01, 20.0 / Width), Min(0.01, 20.0 / Height), 1.0)");
    expandAnimation.InsertKeyFrame(1.0f, { 1.0f, 1.0f, 1.0f }, expandEasingFunction);
    expandAnimation.Duration(m_expandAnimationDuration);
    expandAnimation.Target(s_scaleTargetName);
    m_expandAnimation.set(expandAnimation);

    auto expandElevationAnimation = compositor.CreateVector3KeyFrameAnimation();
    expandElevationAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(this.Target.Translation.X, this.Target.Translation.Y, contentElevation)", expandEasingFunction);
    expandElevationAnimation.SetScalarParameter(L"contentElevation", m_contentElevation);
    expandElevationAnimation.Duration(m_expandAnimationDuration);
    expandElevationAnimation.Target(s_translationTargetName);
    m_expandElevationAnimation.set(expandElevationAnimation);
}

void TeachingTip::CreateContractAnimation()
{
    auto compositor = winrt::Window::Current().Compositor();
    if (!m_contractEasingFunction)
    {
        m_contractEasingFunction.set(compositor.CreateCubicBezierEasingFunction(s_contractAnimationEasingCurveControlPoint1, s_contractAnimationEasingCurveControlPoint2));
    }

    auto contractAnimation = compositor.CreateVector3KeyFrameAnimation();
    if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
    {
        contractAnimation.SetScalarParameter(L"Width", static_cast<float>(tailOcclusionGrid.ActualWidth()));
        contractAnimation.SetScalarParameter(L"Height", static_cast<float>(tailOcclusionGrid.ActualHeight()));
    }
    else
    {
        contractAnimation.SetScalarParameter(L"Width", s_defaultTipHeightAndWidth);
        contractAnimation.SetScalarParameter(L"Height", s_defaultTipHeightAndWidth);
    }

    auto&& contractEasingFunction = m_contractEasingFunction.get();
    contractAnimation.InsertKeyFrame(0.0f, { 1.0f, 1.0f, 1.0f });
    contractAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(20.0 / Width, 20.0 / Height, 1.0)", contractEasingFunction);
    contractAnimation.Duration(m_contractAnimationDuration);
    contractAnimation.Target(s_scaleTargetName);
    m_contractAnimation.set(contractAnimation);

    auto contractElevationAnimation = compositor.CreateVector3KeyFrameAnimation();
    contractElevationAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(this.Target.Translation.X, this.Target.Translation.Y, 0.0f)", contractEasingFunction);
    contractElevationAnimation.Duration(m_contractAnimationDuration);
    contractElevationAnimation.Target(s_translationTargetName);
    m_contractElevationAnimation.set(contractElevationAnimation);
}

void TeachingTip::StartExpandToOpen()
{
    // The contract and expand animations currently use facade's which were not available pre-RS5.
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (!m_expandAnimation)
        {
            CreateExpandAnimation();
        }
        auto scopedBatch = winrt::Window::Current().Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        auto&& expandAnimation = m_expandAnimation.get();
        if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
        {
            tailOcclusionGrid.StartAnimation(expandAnimation);
            m_isExpandAnimationPlaying = true;
        }
        if (auto&& contentRootGrid = m_contentRootGrid.get())
        {
            contentRootGrid.StartAnimation(m_expandElevationAnimation.get());
            m_isExpandAnimationPlaying = true;
        }
        if (auto&& tailEdgeBorder = m_tailEdgeBorder.get())
        {
            tailEdgeBorder.StartAnimation(expandAnimation);
            m_isExpandAnimationPlaying = true;
        }
        scopedBatch.End();

        auto strongThis = get_strong();
        scopedBatch.Completed([strongThis](auto, auto)
        {
            strongThis->m_isExpandAnimationPlaying = false;
            if (!strongThis->m_isContractAnimationPlaying && !strongThis->m_isIdle)
            {
                strongThis->m_isIdle = true;
                TeachingTipTestHooks::NotifyIdleStatusChanged(*strongThis);
            }
        });
    }

    // Under normal circumstances we would have launched an animation just now, if we did not then we should make sure that the idle state is correct
    if (!m_isExpandAnimationPlaying && !m_isIdle && !m_isContractAnimationPlaying)
    {
        m_isIdle = true;
        TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
    }
}

void TeachingTip::StartContractToClose()
{
    // The contract and expand animations currently use facade's which were not available pre RS5.
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (!m_contractAnimation)
        {
            CreateContractAnimation();
        }

        auto scopedBatch = winrt::Window::Current().Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        auto&& contractAnimation = m_contractAnimation.get();
        if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
        {
            tailOcclusionGrid.StartAnimation(contractAnimation);
            m_isContractAnimationPlaying = true;
        }
        if (auto&& contentRootGrid = m_contentRootGrid.get())
        {
            contentRootGrid.StartAnimation(m_contractElevationAnimation.get());
            m_isContractAnimationPlaying = true;
        }
        if (auto&& tailEdgeBorder = m_tailEdgeBorder.get())
        {
            tailEdgeBorder.StartAnimation(contractAnimation);
            m_isContractAnimationPlaying = true;
        }
        scopedBatch.End();

        auto strongThis = get_strong();
        scopedBatch.Completed([strongThis](auto, auto)
        {
            strongThis->m_isContractAnimationPlaying = false;
            strongThis->ClosePopup();
            if (!strongThis->m_isExpandAnimationPlaying && !strongThis->m_isIdle)
            {
                strongThis->m_isIdle = true;
                TeachingTipTestHooks::NotifyIdleStatusChanged(*strongThis);
            }
        });
    }
}

std::tuple<winrt::TeachingTipPlacementMode, bool> TeachingTip::DetermineEffectivePlacement()
{
    // Because we do not have access to APIs to give us details about multi monitor scenarios we do not have the ability to correctly
    // Place the tip in scenarios where we have an out of root bounds tip. Since this is the case we have decided to do no special
    // calculations and return the provided value or top if auto was set. This behavior can be removed via the
    // SetReturnTopForOutOfWindowBounds test hook.
    if (!ShouldConstrainToRootBounds() && m_returnTopForOutOfWindowPlacement)
    {
        auto placement = PreferredPlacement();
        if (placement == winrt::TeachingTipPlacementMode::Auto)
        {
            return std::make_tuple(winrt::TeachingTipPlacementMode::Top, false);
        }
        return std::make_tuple(placement, false);
    }

    if (IsOpen() && m_currentEffectiveTipPlacementMode != winrt::TeachingTipPlacementMode::Auto)
    {
        return std::make_tuple(m_currentEffectiveTipPlacementMode, false);
    }

    auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
    double contentHeight = tailOcclusionGrid.ActualHeight();
    double contentWidth = tailOcclusionGrid.ActualWidth();

    if (m_target)
    {
        return DetermineEffectivePlacementTargeted(contentHeight, contentWidth);
    }
    else 
    {
        return DetermineEffectivePlacementUntargeted(contentHeight, contentWidth);
    }
}

std::tuple<winrt::TeachingTipPlacementMode, bool> TeachingTip::DetermineEffectivePlacementTargeted(double contentHeight, double contentWidth)
{
    // These variables will track which positions the tip will fit in. They all start true and are
    // flipped to false when we find a display condition that is not met.
    enum_array <winrt::TeachingTipPlacementMode, bool, 14> availability;
    availability[winrt::TeachingTipPlacementMode::Auto] = false;
    availability[winrt::TeachingTipPlacementMode::Top] = true;
    availability[winrt::TeachingTipPlacementMode::Bottom] = true;
    availability[winrt::TeachingTipPlacementMode::Right] = true;
    availability[winrt::TeachingTipPlacementMode::Left] = true;
    availability[winrt::TeachingTipPlacementMode::TopLeft] = true;
    availability[winrt::TeachingTipPlacementMode::TopRight] = true;
    availability[winrt::TeachingTipPlacementMode::BottomLeft] = true;
    availability[winrt::TeachingTipPlacementMode::BottomRight] = true;
    availability[winrt::TeachingTipPlacementMode::LeftTop] = true;
    availability[winrt::TeachingTipPlacementMode::LeftBottom] = true;
    availability[winrt::TeachingTipPlacementMode::RightTop] = true;
    availability[winrt::TeachingTipPlacementMode::RightBottom] = true;
    availability[winrt::TeachingTipPlacementMode::Center] = true;

    double tipHeight = contentHeight + TailShortSideLength();
    double tipWidth = contentWidth + TailShortSideLength();

    // We try to avoid having the tail touch the HeroContent so rule out positions where this would be required
    if (HeroContent())
    {
        if (m_heroContentBorder.get().ActualHeight() > m_nonHeroContentRootGrid.get().ActualHeight() - TailLongSideActualLength())
        {
            availability[winrt::TeachingTipPlacementMode::Left] = false;
            availability[winrt::TeachingTipPlacementMode::Right] = false;
        }

        switch (HeroContentPlacement())
        {
        case winrt::TeachingTipHeroContentPlacementMode::Bottom:
            availability[winrt::TeachingTipPlacementMode::Top] = false;
            availability[winrt::TeachingTipPlacementMode::TopRight] = false;
            availability[winrt::TeachingTipPlacementMode::TopLeft] = false;
            availability[winrt::TeachingTipPlacementMode::RightTop] = false;
            availability[winrt::TeachingTipPlacementMode::LeftTop] = false;
            availability[winrt::TeachingTipPlacementMode::Center] = false;
            break;
        case winrt::TeachingTipHeroContentPlacementMode::Top:
            availability[winrt::TeachingTipPlacementMode::Bottom] = false;
            availability[winrt::TeachingTipPlacementMode::BottomLeft] = false;
            availability[winrt::TeachingTipPlacementMode::BottomRight] = false;
            availability[winrt::TeachingTipPlacementMode::RightBottom] = false;
            availability[winrt::TeachingTipPlacementMode::LeftBottom] = false;
            break;
        }
    }

    // When ShouldConstrainToRootBounds is true clippedTargetBounds == availableBoundsAroundTarget
    // We have to separate them because there are checks which care about both.
    auto [clippedTargetBounds, availableBoundsAroundTarget] = DetermineSpaceAroundTarget();

    // If the edge of the target isn't in the window.
    if (clippedTargetBounds.Left < 0)
    {
        availability[winrt::TeachingTipPlacementMode::LeftBottom] = false;
        availability[winrt::TeachingTipPlacementMode::Left] = false;
        availability[winrt::TeachingTipPlacementMode::LeftTop] = false;
    }
    // If the right edge of the target isn't in the window.
    if (clippedTargetBounds.Right < 0)
    {
        availability[winrt::TeachingTipPlacementMode::RightBottom] = false;
        availability[winrt::TeachingTipPlacementMode::Right] = false;
        availability[winrt::TeachingTipPlacementMode::RightTop] = false;
    }
    // If the top edge of the target isn't in the window.
    if (clippedTargetBounds.Top < 0)
    {
        availability[winrt::TeachingTipPlacementMode::TopLeft] = false;
        availability[winrt::TeachingTipPlacementMode::Top] = false;
        availability[winrt::TeachingTipPlacementMode::TopRight] = false;
    }
    // If the bottom edge of the target isn't in the window
    if (clippedTargetBounds.Bottom < 0)
    {
        availability[winrt::TeachingTipPlacementMode::BottomLeft] = false;
        availability[winrt::TeachingTipPlacementMode::Bottom] = false;
        availability[winrt::TeachingTipPlacementMode::BottomRight] = false;
    }

    // If the horizontal midpoint is out of the window.
    if (clippedTargetBounds.Left < -m_currentTargetBoundsInCoreWindowSpace.Width / 2 ||
        clippedTargetBounds.Right < -m_currentTargetBoundsInCoreWindowSpace.Width / 2)
    {
        availability[winrt::TeachingTipPlacementMode::TopLeft] = false;
        availability[winrt::TeachingTipPlacementMode::Top] = false;
        availability[winrt::TeachingTipPlacementMode::TopRight] = false;
        availability[winrt::TeachingTipPlacementMode::BottomLeft] = false;
        availability[winrt::TeachingTipPlacementMode::Bottom] = false;
        availability[winrt::TeachingTipPlacementMode::BottomRight] = false;
        availability[winrt::TeachingTipPlacementMode::Center] = false;
    }

    // If the vertical midpoint is out of the window.
    if (clippedTargetBounds.Top < -m_currentTargetBoundsInCoreWindowSpace.Height / 2 ||
        clippedTargetBounds.Bottom < -m_currentTargetBoundsInCoreWindowSpace.Height / 2)
    {
        availability[winrt::TeachingTipPlacementMode::LeftBottom] = false;
        availability[winrt::TeachingTipPlacementMode::Left] = false;
        availability[winrt::TeachingTipPlacementMode::LeftTop] = false;
        availability[winrt::TeachingTipPlacementMode::RightBottom] = false;
        availability[winrt::TeachingTipPlacementMode::Right] = false;
        availability[winrt::TeachingTipPlacementMode::RightTop] = false;
        availability[winrt::TeachingTipPlacementMode::Center] = false;
    }

    // If the tip is too tall to fit between the top of the target and the top edge of the window or screen.
    if (tipHeight > availableBoundsAroundTarget.Top)
    {
        availability[winrt::TeachingTipPlacementMode::Top] = false;
        availability[winrt::TeachingTipPlacementMode::TopRight] = false;
        availability[winrt::TeachingTipPlacementMode::TopLeft] = false;
    }
    // If the total tip is too tall to fit between the center of the target and the top of the window.
    if (tipHeight > availableBoundsAroundTarget.Top + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::Center] = false;
    }
    // If the tip is too tall to fit between the center of the target and the top edge of the window.
    if (contentHeight - MinimumTipEdgeToTailCenter() > availableBoundsAroundTarget.Top + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::RightTop] = false;
        availability[winrt::TeachingTipPlacementMode::LeftTop] = false;
    }
    // If the tip is too tall to fit in the window when the tail is centered vertically on the target and the tip.
    if (contentHeight / 2.0f > availableBoundsAroundTarget.Top + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f) ||
        contentHeight / 2.0f > availableBoundsAroundTarget.Bottom + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::Right] = false;
        availability[winrt::TeachingTipPlacementMode::Left] = false;
    }
    // If the tip is too tall to fit between the center of the target and the bottom edge of the window.
    if (contentHeight - MinimumTipEdgeToTailCenter() > availableBoundsAroundTarget.Bottom + (m_currentTargetBoundsInCoreWindowSpace.Height / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::RightBottom] = false;
        availability[winrt::TeachingTipPlacementMode::LeftBottom] = false;
    }
    // If the tip is too tall to fit between the bottom of the target and the bottom edge of the window.
    if (tipHeight > availableBoundsAroundTarget.Bottom)
    {
        availability[winrt::TeachingTipPlacementMode::Bottom] = false;
        availability[winrt::TeachingTipPlacementMode::BottomLeft] = false;
        availability[winrt::TeachingTipPlacementMode::BottomRight] = false;
    }

    // If the tip is too wide to fit between the left edge of the target and the left edge of the window.
    if (tipWidth > availableBoundsAroundTarget.Left)
    {
        availability[winrt::TeachingTipPlacementMode::Left] = false;
        availability[winrt::TeachingTipPlacementMode::LeftTop] = false;
        availability[winrt::TeachingTipPlacementMode::LeftBottom] = false;
    }
    // If the tip is too wide to fit between the center of the target and the left edge of the window.
    if (contentWidth - MinimumTipEdgeToTailCenter() > availableBoundsAroundTarget.Left + (m_currentTargetBoundsInCoreWindowSpace.Width / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::TopLeft] = false;
        availability[winrt::TeachingTipPlacementMode::BottomLeft] = false;
    }
    // If the tip is too wide to fit in the window when the tail is centered horizontally on the target and the tip.
    if (contentWidth / 2.0f > availableBoundsAroundTarget.Left + (m_currentTargetBoundsInCoreWindowSpace.Width / 2.0f) ||
        contentWidth / 2.0f > availableBoundsAroundTarget.Right + (m_currentTargetBoundsInCoreWindowSpace.Width / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::Top] = false;
        availability[winrt::TeachingTipPlacementMode::Bottom] = false;
        availability[winrt::TeachingTipPlacementMode::Center] = false;
    }
    // If the tip is too wide to fit between the center of the target and the right edge of the window.
    if (contentWidth - MinimumTipEdgeToTailCenter() > availableBoundsAroundTarget.Right + (m_currentTargetBoundsInCoreWindowSpace.Width / 2.0f))
    {
        availability[winrt::TeachingTipPlacementMode::TopRight] = false;
        availability[winrt::TeachingTipPlacementMode::BottomRight] = false;
    }
    // If the tip is too wide to fit between the right edge of the target and the right edge of the window.
    if (tipWidth > availableBoundsAroundTarget.Right)
    {
        availability[winrt::TeachingTipPlacementMode::Right] = false;
        availability[winrt::TeachingTipPlacementMode::RightTop] = false;
        availability[winrt::TeachingTipPlacementMode::RightBottom] = false;
    }

    auto priorities = GetPlacementFallbackOrder(PreferredPlacement());

    for (auto mode : priorities)
    {
        if (availability[mode])
        {
            return std::make_tuple(mode, false);
        }
    }
    // The teaching tip wont fit anywhere, set tipDoesNotFit to indicate that we should not open.
    return std::make_tuple(winrt::TeachingTipPlacementMode::Top, true);
}

std::tuple<winrt::TeachingTipPlacementMode, bool> TeachingTip::DetermineEffectivePlacementUntargeted(double contentHeight, double contentWidth)
{
    auto const windowBounds = GetWindowBounds();
    if (ShouldConstrainToRootBounds())
    {
        auto const screenBoundsInCoreWindowSpace = GetEffectiveScreenBoundsInCoreWindowSpace(windowBounds);
        if (screenBoundsInCoreWindowSpace.Height < contentHeight && screenBoundsInCoreWindowSpace.Width < contentWidth)
        {
            return std::make_tuple(winrt::TeachingTipPlacementMode::Bottom, false);
        }
    }
    else
    {
        auto const windowBoundsInCoreWindowSpace = GetEffectiveWindowBoundsInCoreWindowSpace(windowBounds);
        if (windowBoundsInCoreWindowSpace.Height < contentHeight && windowBoundsInCoreWindowSpace.Width < contentWidth)
        {
            return std::make_tuple(winrt::TeachingTipPlacementMode::Bottom, false);
        }
    }

    // The teaching tip doesn't fit in the window/screen set tipDoesNotFit to indicate that we should not open.
    return std::make_tuple(winrt::TeachingTipPlacementMode::Top, true);
}

std::tuple<winrt::Thickness, winrt::Thickness> TeachingTip::DetermineSpaceAroundTarget()
{
    auto const shouldConstrainToRootBounds = ShouldConstrainToRootBounds();

    auto const [windowBoundsInCoreWindowSpace, screenBoundsInCoreWindowSpace] = [this]()
    {
        auto const windowBounds = GetWindowBounds();
        return std::make_tuple(GetEffectiveWindowBoundsInCoreWindowSpace(windowBounds),
                               GetEffectiveScreenBoundsInCoreWindowSpace(windowBounds));
    }();


    const winrt::Thickness windowSpaceAroundTarget{
        // Target.Left - Window.Left
        m_currentTargetBoundsInCoreWindowSpace.X - /* 0 except with test window bounds */ windowBoundsInCoreWindowSpace.X,
        // Target.Top - Window.Top
        m_currentTargetBoundsInCoreWindowSpace.Y - /* 0 except with test window bounds */ windowBoundsInCoreWindowSpace.Y,
        // Window.Right - Target.Right
        (windowBoundsInCoreWindowSpace.X + windowBoundsInCoreWindowSpace.Width) - (m_currentTargetBoundsInCoreWindowSpace.X + m_currentTargetBoundsInCoreWindowSpace.Width),
        // Screen.Right - Target.Right
        (windowBoundsInCoreWindowSpace.Y + windowBoundsInCoreWindowSpace.Height) - (m_currentTargetBoundsInCoreWindowSpace.Y + m_currentTargetBoundsInCoreWindowSpace.Height) };


    const winrt::Thickness screenSpaceAroundTarget = [this, screenBoundsInCoreWindowSpace, windowSpaceAroundTarget]()
    {
        if (!ShouldConstrainToRootBounds())
        {
            return winrt::Thickness{
                // Target.Left - Screen.Left
                m_currentTargetBoundsInCoreWindowSpace.X - screenBoundsInCoreWindowSpace.X,
                // Target.Top - Screen.Top
                m_currentTargetBoundsInCoreWindowSpace.Y - screenBoundsInCoreWindowSpace.Y,
                // Screen.Right - Target.Right
                (screenBoundsInCoreWindowSpace.X + screenBoundsInCoreWindowSpace.Width) - (m_currentTargetBoundsInCoreWindowSpace.X + m_currentTargetBoundsInCoreWindowSpace.Width),
                // Screen.Bottom - Target.Bottom
                (screenBoundsInCoreWindowSpace.Y + screenBoundsInCoreWindowSpace.Height) - (m_currentTargetBoundsInCoreWindowSpace.Y + m_currentTargetBoundsInCoreWindowSpace.Height) };
        }
        return windowSpaceAroundTarget;
    }();

    return std::make_tuple(windowSpaceAroundTarget, screenSpaceAroundTarget);
}

winrt::Rect TeachingTip::GetEffectiveWindowBoundsInCoreWindowSpace(const winrt::Rect& windowBounds)
{
    if (m_useTestWindowBounds)
    {
        return m_testWindowBoundsInCoreWindowSpace;
    }
    else
    {
        return winrt::Rect{ 0, 0, windowBounds.Width, windowBounds.Height };
    }
    
}

winrt::Rect TeachingTip::GetEffectiveScreenBoundsInCoreWindowSpace(const winrt::Rect& windowBounds)
{
    if (!m_useTestScreenBounds && !ShouldConstrainToRootBounds())
    {
        MUX_ASSERT_MSG(!m_returnTopForOutOfWindowPlacement, "When returnTopForOutOfWindowPlacement is true we will never need to get the screen bounds");
        auto displayInfo = winrt::DisplayInformation::GetForCurrentView();
        auto scaleFactor = displayInfo.RawPixelsPerViewPixel();
        return winrt::Rect(-windowBounds.X,
            -windowBounds.Y,
            displayInfo.ScreenHeightInRawPixels() / static_cast<float>(scaleFactor),
            displayInfo.ScreenWidthInRawPixels() / static_cast<float>(scaleFactor));
    }
    return m_testScreenBoundsInCoreWindowSpace;
}

winrt::Rect TeachingTip::GetWindowBounds()
{
#ifdef USE_INSIDER_SDK
    if (winrt::IUIElement10 uiElement10 = *this)
    {
        if (auto xamlRoot = uiElement10.XamlRoot())
        {
            return winrt::Rect{ 0, 0, xamlRoot.Size().Width, xamlRoot.Size().Height };
        }
    }
#endif // USE_INSIDER_SDK
    return winrt::Window::Current().CoreWindow().Bounds();
}

std::array<winrt::TeachingTipPlacementMode, 13> TeachingTip::GetPlacementFallbackOrder(winrt::TeachingTipPlacementMode preferredPlacement)
{
    auto priorityList = std::array<winrt::TeachingTipPlacementMode, 13>();
    priorityList[0] = winrt::TeachingTipPlacementMode::Top;
    priorityList[1] = winrt::TeachingTipPlacementMode::Bottom;
    priorityList[2] = winrt::TeachingTipPlacementMode::Left;
    priorityList[3] = winrt::TeachingTipPlacementMode::Right;
    priorityList[4] = winrt::TeachingTipPlacementMode::TopLeft;
    priorityList[5] = winrt::TeachingTipPlacementMode::TopRight;
    priorityList[6] = winrt::TeachingTipPlacementMode::BottomLeft;
    priorityList[7] = winrt::TeachingTipPlacementMode::BottomRight;
    priorityList[8] = winrt::TeachingTipPlacementMode::LeftTop;
    priorityList[9] = winrt::TeachingTipPlacementMode::LeftBottom;
    priorityList[10] = winrt::TeachingTipPlacementMode::RightTop;
    priorityList[11] = winrt::TeachingTipPlacementMode::RightBottom;
    priorityList[12] = winrt::TeachingTipPlacementMode::Center;


    if (IsPlacementBottom(preferredPlacement))
    {
        // Swap to bottom > top
        std::swap(priorityList[0], priorityList[1]);
        std::swap(priorityList[4], priorityList[6]);
        std::swap(priorityList[5], priorityList[7]);
    }
    else if (IsPlacementLeft(preferredPlacement))
    {
        // swap to lateral > vertical
        std::swap(priorityList[0], priorityList[2]);
        std::swap(priorityList[1], priorityList[3]);
        std::swap(priorityList[4], priorityList[8]);
        std::swap(priorityList[5], priorityList[9]);
        std::swap(priorityList[6], priorityList[10]);
        std::swap(priorityList[7], priorityList[11]);
    }
    else if (IsPlacementRight(preferredPlacement))
    {
        // swap to lateral > vertical
        std::swap(priorityList[0], priorityList[2]);
        std::swap(priorityList[1], priorityList[3]);
        std::swap(priorityList[4], priorityList[8]);
        std::swap(priorityList[5], priorityList[9]);
        std::swap(priorityList[6], priorityList[10]);
        std::swap(priorityList[7], priorityList[11]);

        // swap to right > left
        std::swap(priorityList[0], priorityList[1]);
        std::swap(priorityList[4], priorityList[6]);
        std::swap(priorityList[5], priorityList[7]);
    }

    //Switch the preferred placement to first.
    auto pivot = std::find_if(priorityList.begin(),
        priorityList.end(),
        [preferredPlacement](const winrt::TeachingTipPlacementMode mode) -> bool {
            return mode == preferredPlacement;
        });
    if (pivot != priorityList.end()) {
        std::rotate(priorityList.begin(), pivot, pivot + 1);
    }

    return priorityList;
}


void TeachingTip::EstablishShadows()
{
#ifdef USE_INSIDER_SDK
#ifdef TAIL_SHADOW
#ifdef _DEBUG
    if (winrt::IUIElement10 tailPolygon_uiElement10 = m_tailPolygon.get())
    {
        if (m_tipShadow)
        {
            if (!tailPolygon_uiElement10.Shadow())
            {
                // This facilitates an experiment around faking a proper tail shadow, shadows are expensive though so we don't want it present for release builds.
                auto tailShadow = winrt::Windows::UI::Xaml::Media::ThemeShadow{};
                tailShadow.Receivers().Append(m_target.get());
                tailPolygon_uiElement10.Shadow(tailShadow);
                auto&& tailPolygon = m_tailPolygon.get();
                auto&& tailPolygonTranslation = tailPolygon.Translation();
                tailPolygon.Translation({ tailPolygonTranslation.x, tailPolygonTranslation.y, m_tailElevation });
            }
        }
        else
        {
            tailPolygon_uiElement10.Shadow(nullptr);
        }
    }
#endif
#endif
    if (winrt::IUIElement10 m_contentRootGrid_uiElement10 = m_contentRootGrid.get())
    {
        if (m_tipShouldHaveShadow)
        {
            if (!m_contentRootGrid_uiElement10.Shadow())
            {
                m_contentRootGrid_uiElement10.Shadow(winrt::ThemeShadow{});
                auto contentRootGrid = m_contentRootGrid.get();
                auto contentRootGridTranslation = contentRootGrid.Translation();
                contentRootGrid.Translation({ contentRootGridTranslation.x, contentRootGridTranslation.y, m_contentElevation });
            }
        }
        else
        {
            m_contentRootGrid_uiElement10.Shadow(nullptr);
        }
    }
#endif
}

void TeachingTip::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::get_self<TeachingTip>(sender.as<winrt::TeachingTip>())->OnPropertyChanged(args);
}

////////////////
// Test Hooks //
////////////////
void TeachingTip::SetExpandEasingFunction(const winrt::CompositionEasingFunction& easingFunction)
{
    m_expandEasingFunction.set(easingFunction);
    CreateExpandAnimation();
}

void TeachingTip::SetContractEasingFunction(const winrt::CompositionEasingFunction& easingFunction)
{
    m_contractEasingFunction.set(easingFunction);
    CreateContractAnimation();
}

void TeachingTip::SetTipShouldHaveShadow(bool tipShouldHaveShadow)
{
    if (m_tipShouldHaveShadow != tipShouldHaveShadow)
    {
        m_tipShouldHaveShadow = tipShouldHaveShadow;
        EstablishShadows();
    }
}

void TeachingTip::SetContentElevation(float elevation)
{
    m_contentElevation = elevation;
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (auto&& tailOcclusionGrid = m_tailOcclusionGrid.get())
        {
            auto tailOcclusionGridTranslation = tailOcclusionGrid.Translation();
            m_contentRootGrid.get().Translation({ tailOcclusionGridTranslation.x, tailOcclusionGridTranslation.y, m_contentElevation });
        }
        if (m_expandElevationAnimation)
        {
            m_expandElevationAnimation.get().SetScalarParameter(L"contentElevation", m_contentElevation);
        }
    }
}

void TeachingTip::SetTailElevation(float elevation)
{
    m_tailElevation = elevation;
    if (SharedHelpers::IsRS5OrHigher() && m_tailPolygon)
    {
        if (auto && tailPolygon = m_tailPolygon.get())
        {
            auto tailPolygonTranslation = tailPolygon.Translation();
            tailPolygon.Translation({ tailPolygonTranslation.x, tailPolygonTranslation.y, m_tailElevation });
        }
    }
}

void TeachingTip::SetUseTestWindowBounds(bool useTestWindowBounds)
{
    m_useTestWindowBounds = useTestWindowBounds;
}

void TeachingTip::SetTestWindowBounds(const winrt::Rect& testWindowBounds)
{
    m_testWindowBoundsInCoreWindowSpace = testWindowBounds;
}

void TeachingTip::SetUseTestScreenBounds(bool useTestScreenBounds)
{
    m_useTestScreenBounds = useTestScreenBounds;
}

void TeachingTip::SetTestScreenBounds(const winrt::Rect& testScreenBounds)
{
    m_testScreenBoundsInCoreWindowSpace = testScreenBounds;
}

void TeachingTip::SetTipFollowsTarget(bool tipFollowsTarget)
{
    if (m_tipFollowsTarget != tipFollowsTarget)
    {
        m_tipFollowsTarget = tipFollowsTarget;
        if (tipFollowsTarget)
        {
            SetViewportChangedEvent();
        }
        else
        {
            RevokeViewportChangedEvent();
        }
    }
}

void TeachingTip::SetReturnTopForOutOfWindowPlacement(bool returnTopForOutOfWindowPlacement)
{
    m_returnTopForOutOfWindowPlacement = returnTopForOutOfWindowPlacement;
}

void TeachingTip::SetExpandAnimationDuration(const winrt::TimeSpan& expandAnimationDuration)
{
    m_expandAnimationDuration = expandAnimationDuration;
    if (auto&& expandAnimation = m_expandAnimation.get())
    {
        expandAnimation.Duration(m_expandAnimationDuration);
    }
    if (auto&& expandElevationAnimation = m_expandElevationAnimation.get())
    {
        expandElevationAnimation.Duration(m_expandAnimationDuration);
    }
}

void TeachingTip::SetContractAnimationDuration(const winrt::TimeSpan& contractAnimationDuration)
{
    m_contractAnimationDuration = contractAnimationDuration;
    if (auto&& contractAnimation = m_contractAnimation.get())
    {
        contractAnimation.Duration(m_contractAnimationDuration);
    }
    if (auto&& contractElevationAnimation = m_contractElevationAnimation.get())
    {
        contractElevationAnimation.Duration(m_contractAnimationDuration);
    }
}

bool TeachingTip::GetIsIdle()
{
    return m_isIdle;
}

winrt::TeachingTipPlacementMode TeachingTip::GetEffectivePlacement()
{
    return m_currentEffectiveTipPlacementMode;
}

winrt::TeachingTipHeroContentPlacementMode TeachingTip::GetEffectiveHeroContentPlacement()
{
    return m_currentHeroContentEffectivePlacementMode;
}

double TeachingTip::GetHorizontalOffset()
{
    if (auto&& popup = m_popup.get())
    {
        return popup.HorizontalOffset();
    }
    return 0.0;
}

double TeachingTip::GetVerticalOffset()
{
    if (auto&& popup = m_popup.get())
    {
        return popup.VerticalOffset();
    }
    return 0.0;
}
