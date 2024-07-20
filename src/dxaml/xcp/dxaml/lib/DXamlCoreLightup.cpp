// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This source file is intended for light-up code, requiring compilation with a UAP
// contract later than 7 (i.e., Windows build 17763, the downlevel limit for WinAppSDK).
// To permit that, we compile without a PCH which otherwise locks in those limits, 
// and undefine the UAP contract version.  See also wrtdxamlfoundation.vcxproj.

// Undefine UAP contract version to enable latest for lightup code 
#undef WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION
#include "precomp.h"    // This is a direct (non-PCH) compile
#include "DXamlCore.h"
#include <WRLHelper.h>
#include "Callback.h"

using namespace WRLHelper;
using namespace DirectUI;

void DXamlCore::RemoveAutoHideScrollBarsChangedHandler()
{
    if (m_autoHideScrollbarsChangedToken.value != 0 && m_spUISettings)
    {
        // XAML_DIM_DOWN: This interface is only available on 19h1 and later.
        auto source = ctl::query_interface_cast<wuv::IUISettings5>(m_spUISettings.Get());
        if (source)
        {
            VERIFYHR(source->remove_AutoHideScrollBarsChanged(m_autoHideScrollbarsChangedToken));
            m_autoHideScrollbarsChangedToken.value = 0;
        }
    }

    m_regKeyWatcher.reset();
}

void DXamlCore::RegisterForChangeVisualStateOnDynamicScrollbarsSettingChanged(_In_ Control* control)
{
    if (m_autoHideScrollbarsChangedToken.value == 0)
    {
        ctl::ComPtr<wuv::IUISettings> spUISettings;
        if(SUCCEEDED(GetUISettings(spUISettings)))
        {
            auto source = ctl::query_interface_cast<wuv::IUISettings5>(spUISettings.Get());
            if (source) // interface only available on 19h1+
            {
                VERIFYHR(source->add_AutoHideScrollBarsChanged(
                    WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<wuv::UISettings*, wuv::UISettingsAutoHideScrollBarsChangedEventArgs*>>(
                        [this](wuv::IUISettings* sender, wuv::IUISettingsAutoHideScrollBarsChangedEventArgs* args) -> HRESULT
                        {
                            ctl::ComPtr<IDispatcher> spDispatcher;
                            if(SUCCEEDED(GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                            {
                                // This callback will be raised from a thread pool thread. Dispatch to UI Thread.
                                IFC_RETURN(spDispatcher->RunAsync(MakeCallback(this, &DXamlCore::OnAutoHideScrollbarsChanged)));
                            }
                            return S_OK;
                        }).Get(), &m_autoHideScrollbarsChangedToken));
            }
            else
            {
                RegisterForChangeVisualStateOnDynamicScrollbarsRegistryKeySettingChanged();
            }
        }
    }

    m_registeredControlsForSettingsChanged.emplace_back(control);
}

bool DXamlCore::ShouldUseDynamicScrollbars()
{
    if (s_dynamicScrollbarsDirty)
    {
        // Lazy initialize
        ctl::ComPtr<wuv::IUISettings> spUISettings;
        if(SUCCEEDED(DXamlCore::GetCurrent()->GetUISettings(spUISettings)))
        {
            BOOLEAN enableDynamicScrollbars = FALSE;
            auto source = ctl::query_interface_cast<wuv::IUISettings5>(spUISettings.Get());
            if (source) // interface only available on 19h1+
            {
                source->get_AutoHideScrollBars(&enableDynamicScrollbars);
                s_dynamicScrollbars = !!enableDynamicScrollbars;
            }
            else
            {
                s_dynamicScrollbars = ShouldUseDynamicScrollbars_CheckRegistryKey();
            }
        }

        s_dynamicScrollbarsDirty = false;
    }

    return s_dynamicScrollbars;
}

_Check_return_ HRESULT DXamlCore::AddAnimationsEnabledChangedHandler()
{
    if (m_animationsEnabledChangedToken.value == 0)
    {
        ctl::ComPtr<wuv::IUISettings> uiSettings;
        IFC_RETURN(GetUISettings(uiSettings));

        // IUISesttings6 is only available on 19h1 and above hence we return S_OK and do nothing if it is not available
        if (auto source = ctl::query_interface_cast<wuv::IUISettings6>(uiSettings.Get()))
        {
            IFC_RETURN(source->add_AnimationsEnabledChanged(
                WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<wuv::UISettings*, wuv::UISettingsAnimationsEnabledChangedEventArgs*>>(
                    [this](wuv::IUISettings* sender, wuv::IUISettingsAnimationsEnabledChangedEventArgs* args) -> HRESULT
                    {
                        ctl::ComPtr<IDispatcher> spDispatcher;
                        if(SUCCEEDED(GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                        {
                            // This callback will be raised from a thread pool thread. Dispatch to UI Thread.
                            IFC_RETURN(spDispatcher->RunAsync(MakeCallback(this, &DXamlCore::UpdateAnimationsEnabled)));
                        }
                        return S_OK;
                    }).Get(),
                &m_animationsEnabledChangedToken));
        }
    }

    return S_OK;
}

void DXamlCore::RemoveAnimationsEnabledChangedHandler()
{
    if (m_animationsEnabledChangedToken.value != 0 && m_spUISettings)
    {
        auto source = ctl::query_interface_cast<wuv::IUISettings6>(m_spUISettings.Get());

        VERIFYHR(source->remove_AnimationsEnabledChanged(m_animationsEnabledChangedToken));
        m_animationsEnabledChangedToken.value = 0;
    }
}
