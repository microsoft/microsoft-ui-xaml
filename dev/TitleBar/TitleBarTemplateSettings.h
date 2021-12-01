// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TitleBarTemplateSettings.g.h"
#include "TitleBarTemplateSettings.properties.h"

static constexpr winrt::GridLength c_titleColumnGridLengthDefault{ 1, winrt::GridUnitType::Auto };
static constexpr winrt::GridLength c_customColumnGridLengthDefault{ 1, winrt::GridUnitType::Star };

class TitleBarTemplateSettings :
    public winrt::implementation::TitleBarTemplateSettingsT<TitleBarTemplateSettings>,
    public TitleBarTemplateSettingsProperties
{
public:
    TitleBarTemplateSettings();
};
