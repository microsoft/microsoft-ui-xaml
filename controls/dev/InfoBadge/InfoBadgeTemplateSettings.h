#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InfoBadgeTemplateSettings.g.h"
#include "InfoBadgeTemplateSettings.properties.h"

class InfoBadgeTemplateSettings :
    public winrt::implementation::InfoBadgeTemplateSettingsT<InfoBadgeTemplateSettings>,
    public InfoBadgeTemplateSettingsProperties
{
public:
    InfoBadgeTemplateSettings();
};
