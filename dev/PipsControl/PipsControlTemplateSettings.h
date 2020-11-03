#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PipsControlTemplateSettings.g.h"
#include "PipsControlTemplateSettings.properties.h"

class PipsControlTemplateSettings :
    public winrt::implementation::PipsControlTemplateSettingsT<PipsControlTemplateSettings>,
    public PipsControlTemplateSettingsProperties
{
public:
    PipsControlTemplateSettings() { };
};
