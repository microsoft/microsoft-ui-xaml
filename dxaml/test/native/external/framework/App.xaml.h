// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "App.g.h"
#include "Behaviors.h"
#include "ChildControl.h"
#include "ControlTemplates.CustomControl.h"
#include "Converter.h"
#include "CustomObject.h"
#include "CustomUserControl.h"
#include "ControlWithAttachedProperty.h"
#include "DataBinding.CustomControl.h"
#include "DataItem.h"
#include "DataSource.h"
#include "DataStructureHolder.h"
#include "DefaultStyleControl.h"
#include "DefaultStyleResourceUriControl.h"
#include "InpcDataSource.h"
#include "NamingCustomControl.h"
#include "NamingUserControl.h"
#include "Styles.CustomControl.h"
#include "TemplateCache.ChildControl.h"
#include "TemplateCache.Converter.h"

namespace CustomTypes
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public ref class App sealed
    {
    public:
        App() {}
        virtual void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ args) override {}

    private:
        void OnSuspending(Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ e) {}
    };
}
