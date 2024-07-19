// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSplitViewTemplateSettings final : public CDependencyObject
{
private:
    CSplitViewTemplateSettings(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

public:
    DECLARE_CREATE(CSplitViewTemplateSettings);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSplitViewTemplateSettings>::Index;
    }
};
