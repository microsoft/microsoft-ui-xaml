// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewItemTemplateSettings.g.h"
#include "TreeViewItemTemplateSettings.properties.h"

class TreeViewItemTemplateSettings :
    public winrt::implementation::TreeViewItemTemplateSettingsT<TreeViewItemTemplateSettings>,
    public TreeViewItemTemplateSettingsProperties
{
public:
    TreeViewItemTemplateSettings();
};