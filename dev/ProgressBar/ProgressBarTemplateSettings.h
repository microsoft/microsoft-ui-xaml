// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressBarTemplateSettings.g.h"
#include "ProgressBarTemplateSettings.properties.h"

class ProgressBarTemplateSettings :
    public winrt::implementation::ProgressBarTemplateSettingsT<ProgressBarTemplateSettings>,
    public ProgressBarTemplateSettingsProperties
{
public:
    ProgressBarTemplateSettings();
};
