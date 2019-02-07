// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TeachingTipTemplateSettings.g.h"
#include "TeachingTipTemplateSettings.properties.h"

class TeachingTipTemplateSettings :
    public winrt::implementation::TeachingTipTemplateSettingsT<TeachingTipTemplateSettings>,
    public TeachingTipTemplateSettingsProperties
{
public:
    TeachingTipTemplateSettings();
};