// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MicaBackdrop.h"

void MicaBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnTargetConnected(target, xamlRoot);

    auto newController = MicaController();
    newController.Kind(Kind());
    auto systemBackdrop = this->try_as<winrt::Microsoft::UI::Xaml::Media::SystemBackdrop>();
    auto configuration = systemBackdrop.GetDefaultSystemBackdropConfiguration(target, xamlRoot);
    m_controllers.push_back(std::make_unique<ControllerEntry>(target, newController, configuration));
}

void MicaBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop target)
{
    __super::OnTargetDisconnected(target);

    auto entryIterator = std::find_if(
        m_controllers.begin(),
        m_controllers.end(),
        [target](const std::unique_ptr<ControllerEntry>& entry){ return entry->m_target == target; });
    MUX_ASSERT(entryIterator != m_controllers.end());
    m_controllers.erase(entryIterator);
}

void MicaBackdrop::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    if (property == s_KindProperty)
    {
        // If no controller exists, Kind will get set as part of OnTargetConnected
        for (const std::unique_ptr<ControllerEntry>& entry : m_controllers)
        {
            entry->m_controller.Kind(Kind());
        }
    }
}

MicaBackdrop::ControllerEntry::ControllerEntry(ICompositionSupportsSystemBackdrop target, MicaController controller, SystemBackdropConfiguration configuration)
    : m_target(target)
    , m_controller(controller)
{
    controller.AddSystemBackdropTarget(target);
    controller.SetSystemBackdropConfiguration(configuration);
}

MicaBackdrop::ControllerEntry::~ControllerEntry()
{
    m_controller.RemoveSystemBackdropTarget(m_target);
    m_controller.Close();
    m_controller = nullptr;
}