// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Expander.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "ExpanderAutomationPeer.h"
#include "Utils.h"
#include "winnls.h"
#include "ExpanderCollapsedEventArgs.h"
#include "ExpanderExpandingEventArgs.h"

static constexpr auto c_toggleButton = L"ExpanderHeader"sv;

Expander::Expander()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Expander);

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer Expander::OnCreateAutomationPeer()
{
    return winrt::make<ExpanderAutomationPeer>(*this);
}

void Expander::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };
    auto toggleButton = GetTemplateChildT<winrt::ToggleButton>(c_toggleButton, *this);
    GettingFocus({ this, &Expander::OnExpanderGettingFocus });
    LosingFocus({ this, &Expander::OnExpanderLosingFocus });
    //OnGotFocus
    // We will do 2 things with the toggle button's peer:
    // 1. Set the events source of the toggle button peer to
    // the expander's automation peer. This is is because we
    // don't want to announce the toggle button's on/off property
    // changes, but the expander's expander/collapse property changes
    // (or on the events source that's set, if it's set) and
    // 2. Set the expander's automation properties name to the
    // toggleButton's in case the expander doesn't have one.
    if (winrt::AutomationPeer toggleButtonPeer = winrt::FrameworkElementAutomationPeer::FromElement(toggleButton))
    {
        // 1. Set the events source of the toggle button peer to the expander's.
        if (winrt::AutomationPeer expanderPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
        {
            auto expanderEventsSource = expanderPeer.EventsSource() != nullptr ?
                                            expanderPeer.EventsSource() : 
                                             expanderPeer;
            toggleButtonPeer.EventsSource(expanderEventsSource);
        }

        // 2. If the expander doesn't have any AutomationProperties.Name set,
        // we will try setting one based on the header. This is how
        // WPF's expanders work.
        if (winrt::AutomationProperties::GetName(*this).empty()
            && !toggleButtonPeer.GetNameCore().empty())
        {
            winrt::AutomationProperties::SetName(*this, toggleButtonPeer.GetNameCore());
        }
    }

    // Check if it's expanded
    if (ExpanderProperties::IsExpanded())
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", false);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", false);
        this->RaiseCollapsedEvent(*this);
    }

    UpdateExpandDirection(false);
}


void Expander::RaiseExpandingEvent(const winrt::Expander& container)
{
    auto eventArgs = winrt::make_self<ExpanderExpandingEventArgs>(*this);
    eventArgs->ExpandingContent(container.Content());
    m_expandingEventSource(*this, *eventArgs);
}

void Expander::RaiseCollapsedEvent(const winrt::Expander& container)
{
    auto eventArgs = winrt::make_self<ExpanderCollapsedEventArgs>(*this);
    eventArgs->CollapsedContent(container.Content());
    m_collapsedEventSource(*this, *eventArgs);
}

void Expander::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    bool shouldExpand = unbox_value<bool>(args.NewValue());

    if (shouldExpand)
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
        this->RaiseCollapsedEvent(*this);
    }

    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        auto expanderPeer = peer.as<ExpanderAutomationPeer>();
        expanderPeer->RaiseExpandCollapseAutomationEvent(
            shouldExpand ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed
        );
    }
}

void Expander::OnExpandDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateExpandDirection(true);
}

void Expander::OnExpanderGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args)
{
    auto toggleButton = GetTemplateChildT<winrt::ToggleButton>(c_toggleButton, *this);
    if (args.Direction() == winrt::FocusNavigationDirection::None)
    {
        
        auto nextElement = args.NewFocusedElement();
        if (auto expander = nextElement.try_as<winrt::Expander>())
        {
            
            if (winrt::AutomationPeer expanderPeer = winrt::FrameworkElementAutomationPeer::FromElement(expander))
            {
                auto children = expanderPeer.GetChildrenCore();

                for (auto peer : children)
                {
                    if (peer.GetAutomationId() == L"ExpanderToggleButton")
                    {
                        auto toggleButtonPeer = peer.as<winrt::FrameworkElementAutomationPeer>();
                        // Since the EventsSource of the toggle button
                        // is the same as the expander's, we need to
                        // redirect the focus of the expander and base it on the toggle button's.
                        args.TrySetNewFocusedElement(toggleButtonPeer.Owner());
                        return;
                    }
                }
            }


        }
        return;
    }
    if (args.NewFocusedElement() == *this && args.OldFocusedElement() == toggleButton)
    {
        auto direction = args.Direction();
      
        auto nextElement = winrt::FocusManager::FindNextFocusableElement(direction);
        //options.ExclusionRect(rect);
        //findElementOptions.SearchRoot(nullptr);
        //auto nextElement = winrt::FocusManager::FindNextElement(args.Direction(), options);
        if (auto expander = nextElement.try_as<winrt::Expander>())
        {
            if (expander == *this)
            {
                if (winrt::AutomationPeer expanderPeer = winrt::FrameworkElementAutomationPeer::FromElement(expander))
                {

                    auto previousSibling = expanderPeer.Navigate(winrt::AutomationNavigationDirection::PreviousSibling);
                    if (auto previousSiblingPeer = previousSibling.try_as<winrt::FrameworkElementAutomationPeer>())
                    {
                        args.TrySetNewFocusedElement(previousSiblingPeer.Owner());
                    }
                    else if (previousSibling == nullptr)
                    {
                        winrt::Rect rect;
                        rect.X = expander.Translation().x;
                        rect.Y = expander.Translation().y;
                        const winrt::FindNextElementOptions options;
                        options.ExclusionRect(rect);
                        options.SearchRoot(nullptr);
                        auto nextElement = winrt::FocusManager::FindNextElement(winrt::FocusNavigationDirection::Up, options);
                        if (nextElement != nullptr)
                            args.TrySetNewFocusedElement(nextElement);
                        return;
                    }
                    args.Cancel();
                    return;
                }
            }
            else
            {
                args.TrySetNewFocusedElement(expander);
            }
            
            
        }
        args.TrySetNewFocusedElement(nextElement);
        return;
  
    }
    else if (args.NewFocusedElement() == *this && args.OldFocusedElement() != toggleButton)
    {
        args.TrySetNewFocusedElement(toggleButton);
        return;
    }
    

    
}

void Expander::OnExpanderLosingFocus(const winrt::IInspectable& sender, const winrt::LosingFocusEventArgs& args)
{
    return;
    winrt::UIElement elem(nullptr);
    auto toggleButton = GetTemplateChildT<winrt::ToggleButton>(c_toggleButton, *this);

    if (IsExpanded())
    {
        if (args.OldFocusedElement() == toggleButton)
        {
            // Try focusing child of toggle button
            if (winrt::AutomationPeer toggleButtonPeer = winrt::FrameworkElementAutomationPeer::FromElement(toggleButton))
            {
                auto el = toggleButtonPeer.Navigate(winrt::AutomationNavigationDirection::FirstChild);

                if (el != nullptr)
                {
                    //args.TrySetNewFocusedElement(el.as<winrt::FrameworkElementAutomationPeer>().Owner());
                }
            }
            auto contentX = (this->Content()).as<winrt::UIElement>();
            if (winrt::AutomationPeer contentXPeer = winrt::FrameworkElementAutomationPeer::FromElement(contentX))
                args.TrySetNewFocusedElement(contentX);

            // If that doesn't work, try focusing expanded content

            // finally, if that didnt work, fallback to default behavior
        }
        return;
    }
    if (args.Direction() != winrt::FocusNavigationDirection::None) {
        elem = winrt::FocusManager::FindNextFocusableElement(args.Direction());
        if (auto e = elem.try_as<winrt::Button>())
        {
            return;
        }
        if (auto e = elem.try_as<winrt::Expander>())
        {
            return;
        }
    }


    //args.TrySetNewFocusedElement();
    return;
}

void Expander::UpdateExpandDirection(bool useTransitions)
{
    const auto direction = ExpandDirection();
    switch (direction)
    {
    case winrt::ExpandDirection::Down:
        winrt::VisualStateManager::GoToState(*this, L"Down", useTransitions);
        break;
    case winrt::ExpandDirection::Up:
        winrt::VisualStateManager::GoToState(*this, L"Up", useTransitions);
        break;
    case winrt::ExpandDirection::Left:
        winrt::VisualStateManager::GoToState(*this, L"Left", useTransitions);
        break;
    case winrt::ExpandDirection::Right:
        winrt::VisualStateManager::GoToState(*this, L"Right", useTransitions);
        break;
    }
}
