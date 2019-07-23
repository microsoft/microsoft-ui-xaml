// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <common.h>
#include <pch.h>

#include "LifetimeHandler.h"

/* static */
thread_local LifetimeHandler* LifetimeHandler::s_tlsInstanceNoRef;

LifetimeHandler::~LifetimeHandler()
{
    s_tlsInstanceNoRef = nullptr;
}

LifetimeHandler& LifetimeHandler::Instance()
{
    if (s_tlsInstanceNoRef == nullptr)
    {
        bool managedByCoreApplicationView = false;
        com_ptr<LifetimeHandler> strongInstance = winrt::make_self<LifetimeHandler>();
        s_tlsInstanceNoRef = strongInstance.get();

        if (!SharedHelpers::IsInDesignMode() && SharedHelpers::IsRS2OrHigher())
        {
            try
            {
                if (auto currentView = winrt::CoreApplication::GetCurrentView()) // Might be null in XamlPresenter scenarios
                {
                    auto propertyName = MUXCONTROLSROOT_NAMESPACE_STR L".Controls.LifetimeHandler";
                    auto properties = currentView.Properties();
                    properties.Insert(propertyName, *strongInstance);
                    managedByCoreApplicationView = true;
                }
            }
            catch (winrt::hresult_error)
            {
                // CoreApplicationView might throw in XamlPresenter scenarios.
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
    if (!Instance().m_materialHelper)
    {
        Instance().m_materialHelper = winrt::make_self<MaterialHelper>();
    }

    return Instance().m_materialHelper;
}

/* static */
com_ptr<MaterialHelper> LifetimeHandler::TryGetMaterialHelperInstance()
{
    return Instance().m_materialHelper;
}
