#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PipsPagerTemplateSettings.g.h"
#include "PipsPagerTemplateSettings.properties.h"

class PipsPagerTemplateSettings :
    public winrt::implementation::PipsPagerTemplateSettingsT<PipsPagerTemplateSettings>,
    public PipsPagerTemplateSettingsProperties
{
};
