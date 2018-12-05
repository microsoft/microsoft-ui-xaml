// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemTemplateSettings.g.h"
#include "NavigationViewItemTemplateSettings.properties.h"

class NavigationViewItemTemplateSettings :
	public winrt::implementation::NavigationViewItemTemplateSettingsT<NavigationViewItemTemplateSettings>,
	public NavigationViewItemTemplateSettingsProperties
{
};