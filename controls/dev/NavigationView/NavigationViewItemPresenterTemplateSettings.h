// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemPresenterTemplateSettings.g.h"
#include "NavigationViewItemPresenterTemplateSettings.properties.h"

class NavigationViewItemPresenterTemplateSettings :
    public winrt::implementation::NavigationViewItemPresenterTemplateSettingsT<NavigationViewItemPresenterTemplateSettings>,
    public NavigationViewItemPresenterTemplateSettingsProperties
{
};
