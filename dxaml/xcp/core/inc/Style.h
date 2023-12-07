// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NoParentShareableDependencyObject.h>
#include <ICustomWriterRuntimeDataReceiver.h>

class OptimizedStyle;
class StyleCustomRuntimeData;
struct SetterCollectionUnsealer;

namespace Diagnostics
{
    struct StyleUnsealer;
}

class CStyle final
    : public CNoParentShareableDependencyObject
    , public ICustomWriterRuntimeDataReceiver
{
    friend class OptimizedStyle;
    friend struct Diagnostics::StyleUnsealer;
private:
    CStyle(_In_ CCoreServices *pCore);

   _Check_return_ HRESULT ValidateBasedOnTargetType();

public:
    // Creation method
    static _Check_return_ HRESULT Create(
            _Outptr_ CDependencyObject **ppObject,
            _In_ CREATEPARAMETERS *pCreate
        );

    ~CStyle() override;

    _Check_return_ HRESULT Seal();

    // RefreshSetterCollection is used exclusively by XamlDiagnostics to modify final Styles
    _Check_return_ HRESULT RefreshSetterCollection();

    _Check_return_ HRESULT GetTargetTypeName(_Out_ xstring_ptr* pstrFullName);

    unsigned int GetSetterCount();

    _Check_return_ HRESULT GetPropertyAtSetterIndex(_In_ unsigned int setterIndex, _Out_ KnownPropertyIndex* propIndex);

    _Check_return_ HRESULT NotifySetterApplied(_In_ unsigned int setterIndex);

    _Check_return_ HRESULT HasPropertySetter(_In_ const KnownPropertyIndex propIndex, _Out_ bool* hasProperty);

    // Default property getter. Will try to get the property from based on style if no setter on
    // this style for the property.
    _Check_return_ HRESULT GetPropertyValue(
        _In_ KnownPropertyIndex uPropertyId,
        _Out_ CValue *pValue,
        _Out_ bool* gotValue);

    _Check_return_ HRESULT GetPropertyValue(
        _In_ KnownPropertyIndex uPropertyId,
        _In_ bool getFromBasedOn,
        _Out_ CValue *pValue,
        _Out_ bool* gotValue);

    CStyle* GetBasedOnStyleNoRef() const
    {
        return m_pBasedOn;
    }

    // override
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    void ReleaseOverride() final;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;
    _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty *pdp, _Inout_ CValue *pValue) override;

    _Check_return_ HRESULT SetCustomWriterRuntimeData(
            std::shared_ptr<CustomWriterRuntimeData> data,
            std::unique_ptr<CustomWriterRuntimeContext> context) override;

    _Check_return_ CSetterBaseCollection* GetSetterCollection() const;

    _Check_return_ HRESULT CheckForBasedOnCircularReferences();

    void RegisterSetterCollection();

    _Check_return_ HRESULT NotifyMutableSetterValueChanged(_In_ CSetter* const sender);

// CDependencyObject overrides
protected:
    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;
    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) override;

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStyle>::Index;
    };

    const CClassInfo* GetTargetType() const;

    bool HasMutableSetters(bool considerBasedOnStyle = true) const;
    void SubscribeToMutableSetters();
    void UnsubscribeFromMutableSetters();

private:
    // Used for optimizing style's setters
    _Check_return_ HRESULT EnsureOptimizedStyle();
    _Check_return_ HRESULT FaultInOptimizedStyle();
    _Check_return_ HRESULT CreateMergedBasedOnSetterCollection();

public:
    CSetterBaseCollection *m_pSetters = nullptr;
    CBasedOnSetterCollection *m_pBasedOnSetters = nullptr; // holds the union set of self-setters and basedon setters
    CStyle* m_pBasedOn = nullptr; // Holds Style.BasedOn value

private:
    std::unique_ptr<OptimizedStyle> m_optimizedStyle;

public:
    // remove when Types are resolved
    KnownTypeIndex m_targetTypeIndex = KnownTypeIndex::UnknownType;
    bool m_bIsSealed = false;

private:
    // indicates if style is in a basedon circular ref. 0 if it is not, 1 if there is a cycle in its basedon hierarchy
    // but it is not part of that cycle, 2 if it is part of basedon circular reference.
    std::uint8_t m_cBasedOnCircularRefCount = 0;
};
