// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <MaterialHelper.h>
#ifdef TWOPANEVIEW_INCLUDED
#include <DisplayRegionHelper.h>
#endif
#ifdef REPEATER_INCLUDED
#include <ItemsRepeater.common.h>
#endif

// Adds objects to CoreApplicationView.Properties so that they get destroyed accordingly and prevent potential deadlocks.
class LifetimeHandler : 
    public winrt::implements<LifetimeHandler, winrt::IInspectable>
{
private:

    // NoRef pointer to this thread's LifetimeHandler. 
    // We let CoreApplicationView.Properties manage LifetimeHandler's lifetime.
    // This is intended to handle all com_ptrs and winrt objects stored in the TLS.
    // See Bug 14612819: Disconnecting UiaWindowNotifier blocks the loader lock, deadlocking ShellExperienceHost.
    // Also: 15063898: Fix unsafe TLS usage in SplitPanel and ItemsRepeater controls
    static thread_local LifetimeHandler* s_tlsInstanceNoRef;
    static LifetimeHandler& Instance();

#ifdef REPEATER_INCLUDED
    com_ptr<CachedVisualTreeHelpers> m_cachedVisualTreeHelpers;
#endif
    com_ptr<MaterialHelper> m_materialHelper;
#ifdef TWOPANEVIEW_INCLUDED
    com_ptr<DisplayRegionHelper> m_displayRegionHelper;
#endif
public:
    LifetimeHandler() = default;
    ~LifetimeHandler();

#ifdef REPEATER_INCLUDED
    static com_ptr<CachedVisualTreeHelpers> GetCachedVisualTreeHelpersInstance();
#endif

    // MaterialHelper has been factored into common Base and different conditionally compiled derived types for MUX / WUXC
    // to support consumption of private API's such as MaterialProperties exclusively in WUXC.
    static com_ptr<MaterialHelper> GetMaterialHelperInstance();
    static com_ptr<MaterialHelper> TryGetMaterialHelperInstance();

#ifdef TWOPANEVIEW_INCLUDED
    static com_ptr<DisplayRegionHelper> GetDisplayRegionHelperInstance();
#endif
};

