#include "pch.h"
#include "common.h"
#include "TeachingTip.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "TeachingTipClosingEventArgs.h"
#include "TeachingTipClosedEventArgs.h"
#include "TeachingTipTestHooks.h"
#include "TeachingTipAutomationPeer.h"
#include "../Collections/Vector.h"

TeachingTip::TeachingTip()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TeachingTip);
    SetDefaultStyleKey(this);
    EnsureProperties();
    SetValue(s_TemplateSettingsProperty, winrt::make<::TeachingTipTemplateSettings>());
}

winrt::AutomationPeer TeachingTip::OnCreateAutomationPeer()
{
    return winrt::make<TeachingTipAutomationPeer>(*this);
}

void TeachingTip::OnApplyTemplate()
{
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

    if (auto&& closeButton = m_closeButton.get())
    {
        m_closeButtonClickedRevoker = closeButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnCloseButtonClicked });
    }
    if (auto&& alternateCloseButton = m_alternateCloseButton.get())
    {
        winrt::AutomationProperties::SetName(alternateCloseButton, ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipAlternateCloseButtonName));
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

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopEdgeAlignedRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), height - lastRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopEdgeAlignedLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, firstRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomEdgeAlignedRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), firstRowHeight, 0.0f });
            tailEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomEdgeAlignedLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - lastColumnWidth,  height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f,  height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"LeftEdgeAlignedTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ width - lastColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"LeftEdgeAlignedBottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth, height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicHeroContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"RightEdgeAlignedTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            tailOcclusionGrid.CenterPoint({ firstColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            tailEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicHeroContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"RightEdgeAlignedBottom"sv, false);
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
        popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
        popup.HorizontalOffset((((m_currentTargetBounds.X * 2.0f) + m_currentTargetBounds.Width - tipWidth) / 2.0f));
        break;

    case winrt::TeachingTipPlacementMode::Bottom:
        popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset((((m_currentTargetBounds.X * 2.0f) + m_currentTargetBounds.Width - tipWidth) / 2.0f));
        break;

    case winrt::TeachingTipPlacementMode::Left:
        popup.VerticalOffset(((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height - tipHeight) / 2.0f);
        popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::Right:
        popup.VerticalOffset(((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height - tipHeight) / 2.0f);
        popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + static_cast<float>(offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
        popup.HorizontalOffset(((((m_currentTargetBounds.X  * 2.0f) + m_currentTargetBounds.Width) / 2.0f) - MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
        popup.HorizontalOffset(((((m_currentTargetBounds.X  * 2.0f) + m_currentTargetBounds.Width) / 2.0f) - tipWidth + MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset(((((m_currentTargetBounds.X * 2.0f) + m_currentTargetBounds.Width) / 2.0f) - MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + static_cast<float>(offset.Bottom));
        popup.HorizontalOffset(((((m_currentTargetBounds.X * 2.0f) + m_currentTargetBounds.Width) / 2.0f) - tipWidth + MinimumTipEdgeToTailCenter()));
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        popup.VerticalOffset((((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height) / 2.0f) - tipHeight + MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        popup.VerticalOffset((((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height) / 2.0f) - MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        popup.VerticalOffset((((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height) / 2.0f) - tipHeight + MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + static_cast<float>(offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        popup.VerticalOffset((((m_currentTargetBounds.Y * 2.0f) + m_currentTargetBounds.Height) / 2.0f) - MinimumTipEdgeToTailCenter());
        popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + static_cast<float>(offset.Right));
        break;

    default:
        MUX_FAIL_FAST();
    }

    return tipDoesNotFit;
}

bool TeachingTip::PositionUntargetedPopup()
{
    auto windowBounds = m_useTestWindowBounds ? m_testWindowBounds : winrt::Window::Current().CoreWindow().Bounds();

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
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Top:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Left:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::Right:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
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
    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        templateSettings->TopRightHighlightMargin(BottomEdgeAlignedLeftPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomEdgeAlignedLeftPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        templateSettings->TopRightHighlightMargin(BottomEdgeAlignedRightPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomEdgeAlignedRightPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Auto:
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

        m_currentBounds = this->TransformToVisual(nullptr).TransformBounds({
            0.0,
            0.0,
            static_cast<float>(this->ActualWidth()),
            static_cast<float>(this->ActualHeight())
            });

        if (auto&& target = m_target.get())
        {
            SetViewportChangedEvent();
            m_currentTargetBounds = target.TransformToVisual(nullptr).TransformBounds({
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
}

void TeachingTip::OnPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&)
{
    m_lightDismissIndicatorPopup.get().IsOpen(false);
    m_popup.get().Child(nullptr);
    auto myArgs = winrt::make_self<TeachingTipClosedEventArgs>();
    myArgs->Reason(m_lastCloseReason);
    m_closedEventSource(*this, *myArgs);
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
            m_currentTargetBounds = target.TransformToVisual(nullptr).TransformBounds({
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

            if (newTargetBounds != m_currentTargetBounds || newCurrentBounds != m_currentBounds)
            {
                m_currentBounds = newCurrentBounds;
                m_currentTargetBounds = newTargetBounds;
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
    auto placement = PreferredPlacement();
    // Because we do not have access to APIs to give us details about multi monitor scenarios we do not have the ability to correctly
    // Place the tip in scenarios where we have an out of root bounds tip. Since this is the case we have decided to do no special
    // calculations and return the provided value or top if auto was set. This behavior can be removed via the
    // SetReturnTopForOutOfWindowBounds test hook.
    if (!ShouldConstrainToRootBounds() && m_returnTopForOutOfWindowPlacement)
    {
        if (placement == winrt::TeachingTipPlacementMode::Auto)
        {
            return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Top, false);
        }
        return std::tuple<winrt::TeachingTipPlacementMode, bool>(placement, false);
    }

    if (IsOpen() && m_currentEffectiveTipPlacementMode != winrt::TeachingTipPlacementMode::Auto)
    {
        return std::tuple<winrt::TeachingTipPlacementMode, bool>(m_currentEffectiveTipPlacementMode, false);
    }

    if (m_target)
    {
        bool shouldConstrainToRootBounds = ShouldConstrainToRootBounds();

        // These variables will track which positions the tip will fit in. They all start true and are
        // flipped to false when we find a display condition that is not met.
        bool topCenterAvailable = true;
        bool topLeftAvailable = true;
        bool topRightAvailable = true;
        bool bottomCenterAvailable = true;
        bool bottomLeftAvailable = true;
        bool bottomRightAvailable = true;
        bool rightCenterAvailable = true;
        bool rightTopAvailable = true;
        bool rightBottomAvailable = true;
        bool leftCenterAvailable = true;
        bool leftTopAvailable = true;
        bool leftBottomAvailable = true;

        auto&& tailOcclusionGrid = m_tailOcclusionGrid.get();
        double contentHeight = tailOcclusionGrid.ActualHeight();
        double contentWidth = tailOcclusionGrid.ActualWidth();
        double tipHeight = contentHeight + TailShortSideLength();
        double tipWidth = contentWidth + TailShortSideLength();

        // We try to avoid having the tail touch the HeroContent so rule out positions where this would be required
        if (HeroContent())
        {
            if (m_heroContentBorder.get().ActualHeight() > m_nonHeroContentRootGrid.get().ActualHeight() - TailLongSideActualLength())
            {
                leftCenterAvailable = false;
                rightCenterAvailable = false;
            }

            switch (HeroContentPlacement())
            {
            case winrt::TeachingTipHeroContentPlacementMode::Bottom:
                topCenterAvailable = false;
                topRightAvailable = false;
                topLeftAvailable = false;
                rightTopAvailable = false;
                leftTopAvailable = false;
                break;
            case winrt::TeachingTipHeroContentPlacementMode::Top:
                bottomCenterAvailable = false;
                bottomLeftAvailable = false;
                bottomRightAvailable = false;
                rightBottomAvailable = false;
                leftBottomAvailable = false;
                break;
            }
        }

        // When ShouldConstrainToRootBounds is false screenSpaceAroundTarget = windowSpaceAroundTarget
        // We have to separate them because there are checks which care about windowSpaceAroundTarget event when
        // ShouldConstrainToRootBounds is true.
        auto [targetBounds, windowSpaceAroundTarget, screenSpaceAroundTarget] = DetermineSpaceAroundTarget();

        // If the edge of the target isn't in the window.
        if (windowSpaceAroundTarget.Left < 0)
        {
            leftBottomAvailable = false;
            leftCenterAvailable = false;
            leftTopAvailable = false;
        }
        // If the right edge of the target isn't in the window.
        if (windowSpaceAroundTarget.Right < 0)
        {
            rightBottomAvailable = false;
            rightCenterAvailable = false;
            rightTopAvailable = false;
        }
        // If the top edge of the target isn't in the window.
        if (windowSpaceAroundTarget.Top < 0)
        {
            topLeftAvailable = false;
            topCenterAvailable = false;
            topRightAvailable = false;
        }
        // If the bottom edge of the target isn't in the window
        if (windowSpaceAroundTarget.Bottom < 0)
        {
            bottomLeftAvailable = false;
            bottomCenterAvailable = false;
            bottomRightAvailable = false;
        }

        // If the horizontal midpoint is out of the window.
        if (windowSpaceAroundTarget.Left < -targetBounds.Width / 2 ||
            windowSpaceAroundTarget.Right < -targetBounds.Width / 2)
        {
            topLeftAvailable = false;
            topCenterAvailable = false;
            topRightAvailable = false;
            bottomLeftAvailable = false;
            bottomCenterAvailable = false;
            bottomRightAvailable = false;
        }

        // If the vertical midpoint is out of the window.
        if (windowSpaceAroundTarget.Top < -targetBounds.Height / 2 ||
            windowSpaceAroundTarget.Bottom < -targetBounds.Height / 2)
        {
            leftBottomAvailable = false;
            leftCenterAvailable = false;
            leftTopAvailable = false;
            rightBottomAvailable = false;
            rightCenterAvailable = false;
            rightTopAvailable = false;
        }

        // If the tip is too tall to fit between the top of the target and the top edge of the window or screen.
        if (tipHeight > screenSpaceAroundTarget.Top)
        {
            topCenterAvailable = false;
            topRightAvailable = false;
            topLeftAvailable = false;
        }
        // If the tip is too tall to fit between the center of the target and the top edge of the window.
        if (contentHeight - MinimumTipEdgeToTailCenter() > screenSpaceAroundTarget.Top + (targetBounds.Height / 2.0f))
        {
            rightTopAvailable = false;
            leftTopAvailable = false;
        }
        // If the tip is too tall to fit in the window when the tail is centered vertically on the target and the tip.
        if (contentHeight / 2.0f > screenSpaceAroundTarget.Top + (targetBounds.Height / 2.0f) ||
            contentHeight / 2.0f > screenSpaceAroundTarget.Bottom + (targetBounds.Height / 2.0f))
        {
            rightCenterAvailable = false;
            leftCenterAvailable = false;
        }
        // If the tip is too tall to fit between the center of the target and the bottom edge of the window.
        if (contentHeight - MinimumTipEdgeToTailCenter() > screenSpaceAroundTarget.Bottom + (targetBounds.Height / 2.0f))
        {
            rightBottomAvailable = false;
            leftBottomAvailable = false;
        }
        // If the tip is too tall to fit between the bottom of the target and the bottom edge of the window.
        if (tipHeight > screenSpaceAroundTarget.Bottom)
        {
            bottomCenterAvailable = false;
            bottomLeftAvailable = false;
            bottomRightAvailable = false;
        }

        // If the tip is too wide to fit between the left edge of the target and the left edge of the window.
        if (tipWidth > screenSpaceAroundTarget.Left)
        {
            leftCenterAvailable = false;
            leftTopAvailable = false;
            leftBottomAvailable = false;
        }
        // If the tip is too wide to fit between the center of the target and the left edge of the window.
        if (contentWidth - MinimumTipEdgeToTailCenter() > screenSpaceAroundTarget.Left + (targetBounds.Width / 2.0f))
        {
            topLeftAvailable = false;
            bottomLeftAvailable = false;
        }
        // If the tip is too wide to fit in the window when the tail is centered horizontally on the target and the tip.
        if (contentWidth / 2.0f > screenSpaceAroundTarget.Left + (targetBounds.Width / 2.0f) ||
            contentWidth / 2.0f > screenSpaceAroundTarget.Right + (targetBounds.Width / 2.0f))
        {
            topCenterAvailable = false;
            bottomCenterAvailable = false;
        }
        // If the tip is too wide to fit between the center of the target and the right edge of the window.
        if (contentWidth - MinimumTipEdgeToTailCenter() > screenSpaceAroundTarget.Right + (targetBounds.Width / 2.0f))
        {
            topRightAvailable = false;
            bottomRightAvailable = false;
        }
        // If the tip is too wide to fit between the right edge of the target and the right edge of the window.
        if (tipWidth > screenSpaceAroundTarget.Right)
        {
            rightCenterAvailable = false;
            rightTopAvailable = false;
            rightBottomAvailable = false;
        }

        auto priorities = GetPlacementFallbackOrder(placement);

        for each (winrt::TeachingTipPlacementMode mode in priorities)
        {
            if (mode == winrt::TeachingTipPlacementMode::Top && topCenterAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Top, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::Bottom && bottomCenterAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Bottom, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::Left && leftCenterAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Left, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::Right && rightCenterAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Right, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft && topLeftAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::TopEdgeAlignedRight && topRightAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::TopEdgeAlignedRight, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft && bottomLeftAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight && bottomRightAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop && leftTopAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom && leftBottomAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::RightEdgeAlignedTop && rightTopAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::RightEdgeAlignedTop, false);
            }
            if (mode == winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom && rightBottomAvailable)
            {
                return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom, false);
            }
        }
    }
    // The teaching tip wont fit anywhere, set tipDoesNotFit to indicate that we should not open.
    return std::tuple<winrt::TeachingTipPlacementMode, bool>(winrt::TeachingTipPlacementMode::Top, true);
}

std::tuple<winrt::Rect, winrt::Thickness, winrt::Thickness> TeachingTip::DetermineSpaceAroundTarget()
{
    auto shouldConstrainToRootBounds = ShouldConstrainToRootBounds();
    // We use the current window/XamlRoot bounds to determine how much space is between the bottom and left edge of the target
    // and the bottom and left edge of the window/XamlRoot.
    auto windowBounds = m_testWindowBounds;
    if (!m_useTestWindowBounds)
    {
        windowBounds = winrt::Window::Current().CoreWindow().Bounds();
    }

    // We use the screen's size to determine how much space is available between the bottom and right edges of the window/root and the screen
    // This is only necessary if we are displaying in a popup which is not constrained to the root bounds.
    auto screenBounds = m_testScreenBounds;

    if (!m_useTestScreenBounds && !shouldConstrainToRootBounds)
    {
        auto displayInfo = winrt::DisplayInformation::GetForCurrentView();
        auto scaleFactor = displayInfo.RawPixelsPerViewPixel();
        screenBounds = winrt::Rect(0.0f,
            0.0f,
            displayInfo.ScreenHeightInRawPixels() / static_cast<float>(scaleFactor),
            displayInfo.ScreenWidthInRawPixels() / static_cast<float>(scaleFactor));
    }


    auto targetBounds = m_currentTargetBounds;
    if (m_useTestWindowBounds)
    {
        targetBounds.X -= windowBounds.X;
        targetBounds.Y -= windowBounds.Y;
    }
    if (m_useTestScreenBounds)
    {
        targetBounds.X -= screenBounds.X;
        targetBounds.Y -= screenBounds.Y;
    }

    winrt::Thickness windowSpaceAroundTarget{ targetBounds.X,
                                              targetBounds.Y,
                                              windowBounds.Width - (targetBounds.X + targetBounds.Width),
                                              windowBounds.Height - (targetBounds.Y + targetBounds.Height) };

    if (!shouldConstrainToRootBounds)
    {
        winrt::Thickness screenSpaceAroundTarget{ targetBounds.X + windowBounds.X,
                                                  targetBounds.Y + windowBounds.Y,
                                                  screenBounds.Width - (windowBounds.X + targetBounds.X + targetBounds.Width),
                                                  screenBounds.Height - (windowBounds.Y + targetBounds.Y + targetBounds.Height) };

        return std::tuple<winrt::Rect, winrt::Thickness, winrt::Thickness>(targetBounds, windowSpaceAroundTarget, screenSpaceAroundTarget);
    }
    else
    {
        return std::tuple<winrt::Rect, winrt::Thickness, winrt::Thickness>(targetBounds, windowSpaceAroundTarget, windowSpaceAroundTarget);
    }
}

std::list<winrt::TeachingTipPlacementMode> TeachingTip::GetPlacementFallbackOrder(winrt::TeachingTipPlacementMode preferredPlacement)
{
    auto priority1 = winrt::TeachingTipPlacementMode::Top;
    auto priority2 = winrt::TeachingTipPlacementMode::Bottom;
    auto priority3 = winrt::TeachingTipPlacementMode::Left;
    auto priority4 = winrt::TeachingTipPlacementMode::Right;
    auto priority5 = winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft;
    auto priority6 = winrt::TeachingTipPlacementMode::TopEdgeAlignedRight;
    auto priority7 = winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft;
    auto priority8 = winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight;
    auto priority9 = winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop;
    auto priority10 = winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom;
    auto priority11 = winrt::TeachingTipPlacementMode::RightEdgeAlignedTop;
    auto priority12 = winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom;


    if (isPlacementBottom(preferredPlacement))
    {
        // Swap to bottom > top
        std::swap(priority1, priority2);
        std::swap(priority5, priority7);
        std::swap(priority6, priority8);
    }
    else if (isPlacementLeft(preferredPlacement))
    {
        // swap to lateral > vertical
        std::swap(priority1, priority3);
        std::swap(priority2, priority4);
        std::swap(priority5, priority9);
        std::swap(priority6, priority10);
        std::swap(priority7, priority11);
        std::swap(priority8, priority12);
    }
    else if (isPlacementRight(preferredPlacement))
    {
        // swap to lateral > vertical
        std::swap(priority1, priority3);
        std::swap(priority2, priority4);
        std::swap(priority5, priority9);
        std::swap(priority6, priority10);
        std::swap(priority7, priority11);
        std::swap(priority8, priority12);

        // swap to right > left
        std::swap(priority1, priority2);
        std::swap(priority5, priority7);
        std::swap(priority6, priority8);
    }

    auto priorityList = std::list<winrt::TeachingTipPlacementMode>();
    priorityList.push_back(priority1);
    priorityList.push_back(priority2);
    priorityList.push_back(priority3);
    priorityList.push_back(priority4);
    priorityList.push_back(priority5);
    priorityList.push_back(priority6);
    priorityList.push_back(priority7);
    priorityList.push_back(priority8);
    priorityList.push_back(priority9);
    priorityList.push_back(priority10);
    priorityList.push_back(priority11);
    priorityList.push_back(priority12);

    //Switch the preferred placement to first.
    uint32_t index = 0;
    priorityList.remove(preferredPlacement);
    priorityList.insert(priorityList.begin(), preferredPlacement);

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
    m_testWindowBounds = testWindowBounds;
}

void TeachingTip::SetUseTestScreenBounds(bool useTestScreenBounds)
{
    m_useTestScreenBounds = useTestScreenBounds;
}

void TeachingTip::SetTestScreenBounds(const winrt::Rect& testScreenBounds)
{
    m_testScreenBounds = testScreenBounds;
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
