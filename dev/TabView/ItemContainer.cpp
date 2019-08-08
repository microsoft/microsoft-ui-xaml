#include "pch.h"
#include "common.h"
#include "ItemContainer.h"

int ItemContainer::RepeatedIndex()
{
    return m_repeatedIndex;
}

void ItemContainer::RepeatedIndex(int index)
{
    m_repeatedIndex = index;
}

void ItemContainer::SelectionModel(const winrt::SelectionModel& value)
{
    m_selectionModel = value;
    m_selectionChangedRevoker = m_selectionModel.SelectionChanged(winrt::auto_revoke, { this, &ItemContainer::OnSelectionChanged });
}

void ItemContainer::OnIsSelectedPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{

}

void ItemContainer::OnPointerPressed(const winrt::PointerRoutedEventArgs& args)
{
    m_selectionModel.Select(RepeatedIndex());

    auto pointerProperties = args.GetCurrentPoint(*this).Properties();
    m_isPressed = pointerProperties.IsLeftButtonPressed() || pointerProperties.IsRightButtonPressed();

    UpdateVisualState(true);
}

void ItemContainer::OnPointerReleased(const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    UpdateVisualState(true);
}

void ItemContainer::OnPointerEntered(const winrt::PointerRoutedEventArgs& args)
{
    m_isPointerOver = true;
    UpdateVisualState(true);
}

void ItemContainer::OnPointerExited(const winrt::PointerRoutedEventArgs& args)
{
    m_isPointerOver = false;
    UpdateVisualState(true);
}

void ItemContainer::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    m_isPointerOver = false;
    UpdateVisualState(true);
}

void ItemContainer::OnPointerCaptureLost(const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    m_isPointerOver = false;
    UpdateVisualState(true);
}

void ItemContainer::OnSelectionChanged(const winrt::SelectionModel& sender, const winrt::SelectionModelSelectionChangedEventArgs& args)
{
    IsSelected(m_selectionModel.IsSelected(RepeatedIndex()).Value());
    UpdateVisualState(true);
}

void ItemContainer::UpdateVisualState(bool useTransitions)
{
    // DisabledStates and CommonStates
    auto enabledStateValue = L"Enabled";
    bool isSelected = IsSelected();
    auto selectedStateValue = L"Normal";
    if (IsEnabled())
    {
        if (isSelected)
        {
            if (m_isPressed)
            {
                selectedStateValue = L"PressedSelected";
            }
            else if (m_isPointerOver)
            {
                selectedStateValue = L"PointerOverSelected";
            }
            else
            {
                selectedStateValue = L"Selected";
            }
        }
        else if (m_isPointerOver)
        {
            if (m_isPressed)
            {
                selectedStateValue = L"Pressed";
            }
            else
            {
                selectedStateValue = L"PointerOver";
            }
        }
        else if (m_isPressed)
        {
            selectedStateValue = L"Pressed";
        }
    }
    else
    {
        enabledStateValue = L"Disabled";
        if (isSelected)
        {
            selectedStateValue = L"Selected";
        }
    }

    winrt::VisualStateManager::GoToState(*this, enabledStateValue, true);
    winrt::VisualStateManager::GoToState(*this, selectedStateValue, true);
}

void ItemContainer::OnApplyTemplate()
{

}
