// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeProperty.h"
#include "DXamlServices.h"
#include "Style.h"
#include "MetadataAPI.h"
#include "RuntimeObject.h"
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "DOPointerCast.h"

namespace Diagnostics
{
    constexpr uint8_t FakePropertyCount = static_cast<uint8_t>(FakePropertyIndex::RenderingSwitch) + 1;
    std::array<uint32_t, FakePropertyCount> GetAllFakePropertyIndices();

    std::array<uint32_t, FakePropertyCount> GetAllFakePropertyIndices()
    {
        // Add fake property indices to this array, so any new ones added get automatic validation
        static std::array<uint32_t, FakePropertyCount> indices = {
            MAXINT,    // HandinVisual
            MAXINT - 1, // HandoutVisual
            MAXINT - 2, // EvaluatedValue
            MAXINT - 3, // IsBindingValid
            MAXINT - 4, // DesiredSize
            UINT_MAX   // RenderingSwitch
        };

        return indices;
    }

    uint32_t GetFakePropertyIndex(FakePropertyIndex index)
    {
        return GetAllFakePropertyIndices()[static_cast<size_t>(index)];
    }

    RuntimeProperty::RuntimeProperty(KnownPropertyIndex knownIndex)
        : RuntimeProperty(static_cast<uint32_t>(knownIndex))
    {
    }

    RuntimeProperty::RuntimeProperty(FakePropertyIndex fakeIndex)
        : RuntimeProperty(GetFakePropertyIndex(fakeIndex))
    {
    }

    RuntimeProperty::RuntimeProperty(uint32_t index)
        : m_index(index)
        , m_baseValueSource(BaseValueSourceLocal)
        , m_propertyChainIndex(0) // Local properties always point to the first source in the chain
    {
    }

    RuntimeProperty::RuntimeProperty(KnownPropertyIndex knownIndex, BaseValueSource source, uint32_t propertyChainIndex)
        : m_index(static_cast<uint32_t>(knownIndex))
        , m_baseValueSource(source)
        , m_propertyChainIndex(propertyChainIndex)
    {
    }

    bool RuntimeProperty::operator<(const RuntimeProperty& rhs) const
    {
        if (m_index == rhs.m_index)
        {
            // Both point to the same property, compare value sources
            if (m_baseValueSource == rhs.m_baseValueSource)
            {
                // Both point to style of same property, compare base off sources.
                return m_propertyChainIndex < rhs.m_propertyChainIndex;
            }
            return m_baseValueSource < rhs.m_baseValueSource;
        }
        return m_index < rhs.m_index;
    }

    bool RuntimeProperty::operator>(const RuntimeProperty& rhs) const
    {
        return !operator<(rhs);
    }

    bool RuntimeProperty::operator==(const RuntimeProperty& rhs) const
    {
        return m_index == rhs.m_index &&
               m_baseValueSource == rhs.m_baseValueSource &&
               m_propertyChainIndex == rhs.m_propertyChainIndex;
    }

    bool RuntimeProperty::operator!=(const RuntimeProperty& rhs) const
    {
        return !operator==(rhs);
    }

    BaseValueSource RuntimeProperty::GetValueSource() const
    {
        return m_baseValueSource;
    }

    uint32_t RuntimeProperty::GetIndex() const
    {
        return m_index;
    }

    uint32_t RuntimeProperty::GetPropertyChainIndex() const
    {
        return m_propertyChainIndex;
    }
    
    bool RuntimeProperty::IsFakeProperty() const
    {
        auto knownIndex = static_cast<KnownPropertyIndex>(m_index);
        bool isValid = DirectUI::MetadataAPI::IsKnownIndex(knownIndex) || DirectUI::MetadataAPI::IsCustomIndex(knownIndex);
        if (!isValid)
        {
            const auto fakeIndices = Diagnostics::GetAllFakePropertyIndices();
            if (std::find(fakeIndices.begin(), fakeIndices.end(), m_index) != fakeIndices.end())
            {
                return true;
            }
        }
        return false;
    }

    bool RuntimeProperty::IsParentProperty() const
    {
        return static_cast<KnownPropertyIndex>(m_index) == KnownPropertyIndex::FrameworkElement_Parent;
    }

    bool RuntimeProperty::IsRenderingSwitch() const
    {
        return GetIndex() == GetFakePropertyIndex(FakePropertyIndex::RenderingSwitch);
    }

}
