// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>
#include <layoutelement.h>

class CPanelEx : public CLayoutElement
{
protected:
    CPanelEx(_In_ CCoreServices *core);
    ~CPanelEx() override = default;

public:
    DECLARE_CREATE(CPanelEx);

    KnownTypeIndex GetTypeIndex() const final
    {
        return DependencyObjectTraits<CPanelEx>::Index;
    }

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
protected:
    KnownPropertyIndex GetWidthProperty() const override
    {
        return KnownPropertyIndex::PanelEx_Width;
    }

    KnownPropertyIndex GetHeightProperty() const override
    {
        return KnownPropertyIndex::PanelEx_Height;
    }
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
};
