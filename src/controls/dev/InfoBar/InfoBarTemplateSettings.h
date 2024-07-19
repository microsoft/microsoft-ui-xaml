// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InfoBarTemplateSettings.g.h"
#include "InfoBarTemplateSettings.properties.h"

class InfoBarTemplateSettings :
    public winrt::implementation::InfoBarTemplateSettingsT<InfoBarTemplateSettings>,
    public InfoBarTemplateSettingsProperties
{
public:
    InfoBarTemplateSettings();
};
