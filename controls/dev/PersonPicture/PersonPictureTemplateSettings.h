// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PersonPictureTemplateSettings.g.h"
#include "PersonPictureTemplateSettings.properties.h"

class PersonPictureTemplateSettings :
    public winrt::implementation::PersonPictureTemplateSettingsT<PersonPictureTemplateSettings>,
    public PersonPictureTemplateSettingsProperties
{
};
