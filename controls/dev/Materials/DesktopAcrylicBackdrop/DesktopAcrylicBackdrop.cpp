// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "DesktopAcrylicBackdrop.h"

#include "DesktopAcrylicBackdrop.properties.cpp"

void DesktopAcrylicBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnTargetConnected(target, xamlRoot);

    auto newController = DesktopAcrylicController();
    auto systemBackdrop = this->try_as<winrt::Microsoft::UI::Xaml::Media::SystemBackdrop>();
    auto configuration = systemBackdrop.GetDefaultSystemBackdropConfiguration(target, xamlRoot);
    m_controllers.push_back(std::make_unique<ControllerEntry>(target, newController, configuration));
}

void DesktopAcrylicBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop target)
{
    __super::OnTargetDisconnected(target);

    auto entryIterator = std::find_if(
        m_controllers.begin(),
        m_controllers.end(),
        [target](const std::unique_ptr<ControllerEntry>& entry){ return entry->m_target == target; });
    // Workaround for Bug 44926194: SystemBackdrop's BaseController can fail to ensure system DQ before framework gets shutdown notification
    // OnTargetConnected could fail in IXP's BaseController when ensuring a system DispatcherQueue. If that happens
    // we'll skip adding to the list of controllers, which means we won't find anything when trying to remove it later.
    // When the bug is fixed, take out this if condition and always assert and erase.
    // We hit the failure when the app has started DQ shutdown, but Xaml hasn't received any notification yet.
    if (entryIterator != m_controllers.end())
    {
        MUX_ASSERT(entryIterator != m_controllers.end());
        m_controllers.erase(entryIterator);
    }
}

DesktopAcrylicBackdrop::ControllerEntry::ControllerEntry(ICompositionSupportsSystemBackdrop target, DesktopAcrylicController controller, SystemBackdropConfiguration configuration)
    : m_target(target)
    , m_controller(controller)
{
    controller.AddSystemBackdropTarget(target);
    controller.SetSystemBackdropConfiguration(configuration);
}

DesktopAcrylicBackdrop::ControllerEntry::~ControllerEntry()
{
    m_controller.RemoveSystemBackdropTarget(m_target);
    m_controller.Close();
    m_controller = nullptr;
}