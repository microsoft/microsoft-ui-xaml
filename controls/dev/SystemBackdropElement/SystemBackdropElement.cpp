// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "SystemBackdropElement.h"

SystemBackdropElement::SystemBackdropElement()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SystemBackdropElement);
}

SystemBackdropElement::~SystemBackdropElement()
{
    ReleaseCompositionResources();
}

void SystemBackdropElement::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto property = args.Property();
    if (property == s_SystemBackdropProperty && args.NewValue() != args.OldValue())
    {
        m_systemBackdrop = args.NewValue().try_as<winrt::SystemBackdrop>();
        if (m_systemBackdrop)
        {
            throw winrt::hresult_not_implemented(
                L"SystemBackdropElement is unavailable when WinUI uses the system composition stack.");
        }
    }
    else if (property == s_CornerRadiusProperty)
    {
        if (auto value = args.NewValue().try_as<winrt::IReference<winrt::CornerRadius>>())
        {
            m_cornerRadius = value.Value();
        }
        else
        {
            m_cornerRadius = {};
        }
    }
}

winrt::Size SystemBackdropElement::ArrangeOverride(winrt::Size const& finalSize)
{
    m_lastArrangedSize = __super::ArrangeOverride(finalSize);
    UpdatePlacementVisual();
    return m_lastArrangedSize;
}

void SystemBackdropElement::UpdatePlacementVisual()
{
}

void SystemBackdropElement::ReleaseCompositionResources()
{
    m_systemBackdrop = nullptr;
}
