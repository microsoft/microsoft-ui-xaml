// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressRingTemplateSettings.g.h"
#include "ProgressRingTemplateSettings.properties.h"

class ProgressRingTemplateSettings :
    public winrt::implementation::ProgressRingTemplateSettingsT<ProgressRingTemplateSettings>,
    public ProgressRingTemplateSettingsProperties
{
public:
    ProgressRingTemplateSettings();
};
