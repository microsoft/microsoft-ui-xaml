// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MicaBackdrop.h"
#include "SystemCompositionEngine.h"

namespace
{
    // The DWM material that matches a MicaKind. MicaKind::Base is the material an app's main window uses, and
    // MicaKind::BaseAlt is the flatter variant behind a tab strip. A kind we don't know about has no equivalent,
    // and the caller keeps using the lifted controller rather than silently drawing the wrong material.
    std::optional<DWM_SYSTEMBACKDROP_TYPE> ToDwmBackdropType(MicaKind kind)
    {
        switch (kind)
        {
        case MicaKind::Base:
            return DWMSBT_MAINWINDOW;
        case MicaKind::BaseAlt:
            return DWMSBT_TABBEDWINDOW;
        default:
            return std::nullopt;
        }
    }
}

void MicaBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnTargetConnected(target, xamlRoot);

    auto systemBackdrop = this->try_as<winrt::Microsoft::UI::Xaml::Media::SystemBackdrop>();
    auto configuration = systemBackdrop.GetDefaultSystemBackdropConfiguration(target, xamlRoot);

    // Fork on the composition engine. MicaController renders the material into the target through the lifted
    // compositor, so in a process running on the system composition engine it has nothing to render into and
    // Mica would silently not appear at all. DWM draws the same material for a window directly, which is the
    // recipe the system provides for every other window, so that is what a Xaml Window falls back to. Targets
    // that don't cover a Xaml Window can't be expressed as a window attribute and keep the controller.
    if (SystemCompositionEngine::IsEnabledForProcess())
    {
        if (const auto dwmBackdropType = ToDwmBackdropType(Kind()))
        {
            if (auto dwmBackdrop = DwmSystemBackdrop::TryApplyToXamlWindow(
                    systemBackdrop, target, xamlRoot, *dwmBackdropType, IsDarkTheme(configuration)))
            {
                m_targets.push_back(std::make_unique<TargetEntry>(target, std::move(dwmBackdrop)));
                return;
            }
        }
    }

    auto newController = MicaController();
    newController.Kind(Kind());
    m_targets.push_back(std::make_unique<TargetEntry>(target, newController, configuration));
}

// The theme is the only part of the configuration a DWM-drawn backdrop can act on: DWM owns the material, so
// input activation and the high contrast colors are already its own business. Xaml only ever leaves the theme
// at Default before it has resolved one, which is the same light default DWM would pick on its own.
bool MicaBackdrop::IsDarkTheme(const SystemBackdropConfiguration& configuration)
{
    return configuration && configuration.Theme() == SystemBackdropTheme::Dark;
}

void MicaBackdrop::OnDefaultSystemBackdropConfigurationChanged(
    ICompositionSupportsSystemBackdrop target,
    winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnDefaultSystemBackdropConfigurationChanged(target, xamlRoot);

    auto entryIterator = std::find_if(
        m_targets.begin(),
        m_targets.end(),
        [target](const std::unique_ptr<TargetEntry>& entry){ return entry->Target() == target; });

    if (entryIterator == m_targets.end())
    {
        return;
    }

    auto systemBackdrop = this->try_as<winrt::Microsoft::UI::Xaml::Media::SystemBackdrop>();
    (*entryIterator)->UpdateDarkMode(IsDarkTheme(systemBackdrop.GetDefaultSystemBackdropConfiguration(target, xamlRoot)));
}

void MicaBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop target)
{
    __super::OnTargetDisconnected(target);

    auto entryIterator = std::find_if(
        m_targets.begin(),
        m_targets.end(),
        [target](const std::unique_ptr<TargetEntry>& entry){ return entry->Target() == target; });

    // Workaround for Bug 44926194: SystemBackdrop's BaseController can fail to ensure system DQ before framework
    // gets shutdown notification. OnTargetConnected could fail in IXP's BaseController when ensuring a system
    // DispatcherQueue. If that happens we'll skip adding to the list of targets, which means we won't find
    // anything when trying to remove it later. When the bug is fixed, take out this if condition and always
    // assert and erase. We hit the failure when the app has started DQ shutdown, but Xaml hasn't received any
    // notification yet.
    if (entryIterator != m_targets.end())
    {
        m_targets.erase(entryIterator);
    }
}

void MicaBackdrop::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    if (property == s_KindProperty)
    {
        const auto kind = Kind();
        const auto dwmBackdropType = ToDwmBackdropType(kind);

        // If no target is connected, Kind will get applied as part of OnTargetConnected
        for (const std::unique_ptr<TargetEntry>& entry : m_targets)
        {
            entry->UpdateKind(kind, dwmBackdropType);
        }
    }
}

MicaBackdrop::TargetEntry::TargetEntry(ICompositionSupportsSystemBackdrop target, MicaController controller, SystemBackdropConfiguration configuration)
    : m_target(target)
    , m_controller(controller)
{
    controller.AddSystemBackdropTarget(target);
    controller.SetSystemBackdropConfiguration(configuration);
}

MicaBackdrop::TargetEntry::TargetEntry(ICompositionSupportsSystemBackdrop target, std::unique_ptr<DwmSystemBackdrop> dwmBackdrop)
    : m_target(target)
    , m_dwmBackdrop(std::move(dwmBackdrop))
{
}

MicaBackdrop::TargetEntry::~TargetEntry()
{
    if (m_controller)
    {
        m_controller.RemoveSystemBackdropTarget(m_target);
        m_controller.Close();
        m_controller = nullptr;
    }
}

void MicaBackdrop::TargetEntry::UpdateKind(MicaKind kind, std::optional<DWM_SYSTEMBACKDROP_TYPE> dwmBackdropType)
{
    if (m_controller)
    {
        m_controller.Kind(kind);
    }
    else if (dwmBackdropType)
    {
        m_dwmBackdrop->BackdropType(*dwmBackdropType);
    }

    // A kind with no DWM equivalent leaves the window on the material it already has, for the same reason
    // OnTargetConnected doesn't take the DWM path for one: drawing the wrong material is worse.
}

void MicaBackdrop::TargetEntry::UpdateDarkMode(bool useDarkMode)
{
    // A lifted controller reads the theme out of the SystemBackdropConfiguration it was given, so there is
    // nothing to forward for one.
    if (m_dwmBackdrop)
    {
        m_dwmBackdrop->DarkMode(useDarkMode);
    }
}
