// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PagerControlTemplateSettings.g.h"
#include "PagerControlTemplateSettings.properties.h"

class PagerControlTemplateSettings :
    public winrt::implementation::PagerControlTemplateSettingsT<PagerControlTemplateSettings>,
    public PagerControlTemplateSettingsProperties
{
public:
    PagerControlTemplateSettings() { };
};
