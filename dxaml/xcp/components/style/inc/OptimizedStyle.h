// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerInterfaces.h"
#include <bit_vector.h>

enum class KnownPropertyIndex : UINT16;
class CStyle;
class CSetter;
class CValue;
class CCollection;
class CSetterBaseCollection;
class CDependencyProperty;
class CDependencyObject;
class CDependencyPropertyProxy;
class CCoreServices;
class StyleCustomRuntimeData;
class CustomWriterRuntimeContext;

namespace Theming {
    enum class Theme : uint8_t;
}

class OptimizedStyle
{
private:
    OptimizedStyle(_In_ CStyle* const style);

    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT Initialize(
        _In_ std::shared_ptr<StyleCustomRuntimeData> runtimeData,
        _In_ const std::unique_ptr<CustomWriterRuntimeContext>& runtimeContext);
public:

    static _Check_return_ HRESULT Create(_In_ CStyle* const style, _Out_ std::unique_ptr<OptimizedStyle>* pValue)
    {
        std::unique_ptr<OptimizedStyle> opStyle(new OptimizedStyle(style));
        IFC_RETURN(opStyle->Initialize());
        *pValue = std::move(opStyle);
        return S_OK;
    }

    static _Check_return_ HRESULT Create(
        _In_ CStyle* const style,
        _In_ std::shared_ptr<StyleCustomRuntimeData> runtimeData,
        _In_ const std::unique_ptr<CustomWriterRuntimeContext>& runtimeContext,
        _Out_ std::unique_ptr<OptimizedStyle>* pValue)
    {
        std::unique_ptr<OptimizedStyle> opStyle(new OptimizedStyle(style));
        IFC_RETURN(opStyle->Initialize(runtimeData, runtimeContext));
        *pValue = std::move(opStyle);
        return S_OK;
    }

    ~OptimizedStyle();

    _Check_return_ HRESULT FaultInOwnedSetters(_In_ const xref_ptr<CSetterBaseCollection>& setters) const;

    // Returns the count of the style's setters.
    unsigned int GetSetterCount() const
    {
        return static_cast<unsigned int>(m_properties.size());
    }

    // Returns the KnownPropertyIndex at the given setter index.
    KnownPropertyIndex GetPropertyAtSetterIndex(_In_ const unsigned int setterIndex) const
    {
        ASSERT(m_properties.size() > setterIndex);
        return m_properties[setterIndex];
    }

    bool HasMutableSetters() const
    {
        return !m_mutableSetters.empty();
    }

    _Check_return_ HRESULT NotifySetterApplied(_In_ unsigned int setterIndex);

    bool HasPropertySetter(_In_ const KnownPropertyIndex propertyId) const;

    _Check_return_ HRESULT GetPropertyValue(_In_ KnownPropertyIndex propertyId, _Outptr_opt_ CValue** ppValue)
    {
        IFC_RETURN(GetPropertyValue(propertyId, true /*getFromBasedOn*/, ppValue));
        return S_OK;
    }

    _Check_return_ HRESULT GetPropertyValue(_In_ KnownPropertyIndex propertyId, _In_ bool getFromBasedOn, _Outptr_opt_ CValue** ppValue);

    _Check_return_ HRESULT NotifyThemeChanged(_In_ Theming::Theme theme, _In_ bool forceRefresh = false) const;

    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer);

    _Check_return_ HRESULT Seal();

    // Used by CStyle to notify its associated OptimizedStyle that an optimized mutable Setter's Value has changed
    _Check_return_ HRESULT NotifyMutableSetterValueChanged(_In_ CSetter* const sender);

    void SubscribeToMutableSetters() const;
    void UnsubscribeFromMutableSetters() const;

    static _Check_return_ HRESULT ResolveValue(_In_ CCoreServices* coreServices, _In_ const CDependencyProperty* depProp, _Inout_ CValue& val, _Out_ bool* createdObject);

private:
    struct MutableSetterAndIndex
    {
        MutableSetterAndIndex(xref_ptr<CSetter> setter, std::size_t index)
            : MutableSetter(setter)
            , Index(index)
        {
        }

        xref_ptr<CSetter> MutableSetter;
        std::size_t Index;
    };

private:
    CStyle* const m_style;
    KnownTypeIndex m_targetType;
    bool m_isSealed;

    std::vector<KnownPropertyIndex> m_properties;
    std::vector<CValue> m_values;
    std::vector<MutableSetterAndIndex> m_mutableSetters;
    containers::bit_vector m_resolvedStates;
    containers::bit_vector m_peggedStates;

    size_t m_basedOnBegin;
    CValue m_latestFoundBasedOnValue;

    _Check_return_ HRESULT OptimizeSetterCollection(_In_ CSetterBaseCollection* setters, bool isBasedOn);
    _Check_return_ HRESULT AddSetterInfo(_In_ KnownPropertyIndex propertyId, _In_ CValue&& value);
    void AddBasedOnSetterInfo(_In_ KnownPropertyIndex propertyId);
    _Check_return_ HRESULT AddPeerRefIfNecessary(_In_opt_ CDependencyObject* depObj, _Out_ bool* pResult) const;
};
