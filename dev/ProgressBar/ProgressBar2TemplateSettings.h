// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressBar2TemplateSettings.g.h"
#include "ProgressBar2TemplateSettings.properties.h"

class ProgressBar2TemplateSettings :
    public winrt::implementation::ProgressBar2TemplateSettingsT<ProgressBar2TemplateSettings>,
    public ProgressBar2TemplateSettingsProperties
{
public:
    ProgressBar2TemplateSettings();
};
