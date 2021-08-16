// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TitleBarTemplateSettings.g.h"
#include "TitleBarTemplateSettings.properties.h"

class TitleBarTemplateSettings :
    public winrt::implementation::TitleBarTemplateSettingsT<TitleBarTemplateSettings>,
    public TitleBarTemplateSettingsProperties
{
public:
    TitleBarTemplateSettings();
};
