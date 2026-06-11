// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "precomp.h"
#include "XamlOM.WinUI.h"
enum class KnownPropertyIndex : uint16_t;

namespace Diagnostics
{
    struct PropertyChainData;
    class RuntimeObject;
    enum class FakePropertyIndex : uint32_t
    {
        HandinVisual = 0,
        HandoutVisual,
        EvaluatedValue,
        IsBindingValid,
        UIElementDesiredSize,
        // Magic value which ensures we are indeed being called to enable/disable rendering. This needs to be the last
        RenderingSwitch
    };

    class RuntimeProperty final
    {
    public:
        RuntimeProperty(KnownPropertyIndex knownIndex);
        RuntimeProperty(FakePropertyIndex fakeIndex);
        RuntimeProperty(KnownPropertyIndex knownIndex, BaseValueSource source, uint32_t propertyChainIndex);
        RuntimeProperty(uint32_t unknownIndex);

        bool operator<(const RuntimeProperty& rhs) const;
        bool operator>(const RuntimeProperty& rhs) const;
        bool operator==(const RuntimeProperty& rhs) const;
        bool operator!=(const RuntimeProperty& rhs) const;

        BaseValueSource GetValueSource() const;
        uint32_t GetIndex() const;
        uint32_t GetPropertyChainIndex() const;

        bool IsFakeProperty() const;
        bool IsRenderingSwitch() const;
        bool IsParentProperty() const;

    private:

        uint32_t m_index;
        uint32_t m_propertyChainIndex : 4;
        BaseValueSource m_baseValueSource : 4;
    };
}