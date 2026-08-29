// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ExpanderTemplateSettings.g.h"
#include "ExpanderTemplateSettings.properties.h"

class ExpanderTemplateSettings :
    public winrt::implementation::ExpanderTemplateSettingsT<ExpanderTemplateSettings>,
    public ExpanderTemplateSettingsProperties
{
public:
    ExpanderTemplateSettings();
};
