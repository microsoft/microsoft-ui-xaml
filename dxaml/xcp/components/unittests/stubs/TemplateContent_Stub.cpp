// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TemplateContent.h>
#include <SavedContext.h>

xref_ptr<CDependencyObject> CTemplateContent::GetSavedEventRoot() const
{
    return nullptr;
}


std::shared_ptr<XamlSavedContext> CTemplateContent::GetSavedContext() const
{
    return nullptr;
}