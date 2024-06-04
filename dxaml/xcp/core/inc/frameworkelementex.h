// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>
#include <layoutelement.h>

class CFrameworkElementEx : public CLayoutElement
{
protected:
    CFrameworkElementEx(_In_ CCoreServices *core);
    ~CFrameworkElementEx() override = default;

public:
    DECLARE_CREATE(CFrameworkElementEx);

    KnownTypeIndex GetTypeIndex() const final
    {
        return DependencyObjectTraits<CFrameworkElementEx>::Index;
    }

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
protected:
    KnownPropertyIndex GetWidthProperty() const override
    {
        return KnownPropertyIndex::FrameworkElementEx_Width;
    }

    KnownPropertyIndex GetHeightProperty() const override
    {
        return KnownPropertyIndex::FrameworkElementEx_Height;
    }
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
};
