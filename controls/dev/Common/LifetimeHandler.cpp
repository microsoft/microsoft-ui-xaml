// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>

#include "LifetimeHandler.h"


/* static */
thread_local LifetimeHandler* LifetimeHandler::s_tlsInstanceNoRef;

LifetimeHandler::~LifetimeHandler()
{
    s_tlsInstanceNoRef = nullptr;
}

LifetimeHandler& LifetimeHandler::Instance()
{
    if (!s_tlsInstanceNoRef)
    {
        bool managedByCoreApplicationView = false;
        com_ptr<LifetimeHandler> strongInstance = winrt::make_self<LifetimeHandler>();
        s_tlsInstanceNoRef = strongInstance.get();

        if (!SharedHelpers::IsInDesignMode())
        {
            if (auto currentView = SharedHelpers::TryGetCurrentCoreApplicationView()) // Might be null in XamlPresenter scenarios
            {
                auto propertyName = MUXCONTROLSROOT_NAMESPACE_STR L".Controls.LifetimeHandler";
                auto properties = currentView.Properties();
                properties.Insert(propertyName, *strongInstance);
                managedByCoreApplicationView = true;
            }
        }

        if (!managedByCoreApplicationView)
        {
            // Keep alive indefinitely - the best we can do in this case
            strongInstance.detach();
        }
    }

    return *s_tlsInstanceNoRef;
}

#ifdef REPEATER_INCLUDED
/* static */
com_ptr<CachedVisualTreeHelpers> LifetimeHandler::GetCachedVisualTreeHelpersInstance()
{
    if (!Instance().m_cachedVisualTreeHelpers)
    {
        Instance().m_cachedVisualTreeHelpers = winrt::make_self<CachedVisualTreeHelpers>();
    }

    return Instance().m_cachedVisualTreeHelpers;
}
#endif

#ifdef TWOPANEVIEW_INCLUDED
/* static */
com_ptr<DisplayRegionHelper> LifetimeHandler::GetDisplayRegionHelperInstance()
{
    if (!Instance().m_displayRegionHelper)
    {
        Instance().m_displayRegionHelper = winrt::make_self<DisplayRegionHelper>();
    }

    return Instance().m_displayRegionHelper;
}
#endif

/* static */
com_ptr<MaterialHelper> LifetimeHandler::GetMaterialHelperInstance()
{
    if (const auto instance = TryGetMaterialHelperInstance())
    {
        return instance;
    }
    
    Instance().m_materialHelper = winrt::make_self<MaterialHelper>(); 

    return Instance().m_materialHelper;
}

/* static */
com_ptr<MaterialHelper> LifetimeHandler::TryGetMaterialHelperInstance()
{
    const auto instance = Instance().m_materialHelper;
    if (instance && instance->m_acrylicCompositor)
    {
        // Make sure that MaterialHelper's Compositor is the same Compositor XAML's using. If XAML shuts down and starts
        // back up on the same thread, it will have closed the previous Compositor and created a new one.  Make sure
        // we're not using comp objects based on a stale/closed Compositor.
        winrt::Compositor xamlCompositor {nullptr};        

        // Calling winrt::CompositionTarget::GetCompositorForCurrentThread() can result in a noisy exception if this
        // call is happening after the XAML core has already shutdown. Avoid this by calling the ABI method, using
        // relevant winrt::impl types for the guid and interface definition.
        auto compositionTarget = winrt::get_activation_factory<winrt::CompositionTarget>();
        auto guid = winrt::impl::guid_v<winrt::Microsoft::UI::Xaml::Media::ICompositionTargetStatics>;
        com_ptr<winrt::impl::abi<winrt::Microsoft::UI::Xaml::Media::ICompositionTargetStatics>::type> compositionTargetStatics;
        auto hr = compositionTarget.as<IUnknown>()->QueryInterface(guid, compositionTargetStatics.put_void());
        if (SUCCEEDED(hr))
        {
            auto ignorehr = compositionTargetStatics->GetCompositorForCurrentThread(winrt::put_abi(xamlCompositor));
        }

        if (instance->m_acrylicCompositor != xamlCompositor)
        {
            ClearMaterialHelperInstance();
            return nullptr;
        }
    }

    return instance;
}

/* static */
void LifetimeHandler::ClearMaterialHelperInstance()
{
    Instance().m_materialHelper = nullptr;
}
