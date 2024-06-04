// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "base\inc\wilhelper.h"
#include "XamlOM.WinUI.h"
#include <vector>
#include "style.h"
#include "DependencyObject.h"

namespace DirectUI {
    class DependencyObject;
}

namespace DebugTool {
    struct IDebugBindingExpression;
}

namespace Diagnostics {
    class PropertyChainIterator;
    struct PropertyChainData;

    struct EvaluatedValue
    {
        EvaluatedValue() {}

        explicit EvaluatedValue(
            _In_ const ctl::ComPtr<IInspectable>& value,
            _In_ wil::unique_propertychainvalue&& data)
            : Value(value)
            , PropertyData(std::move(data))
        {
        }
        ctl::ComPtr<IInspectable> Value;
        wil::unique_propertychainvalue PropertyData;
    };

    struct PropertySource
    {
        explicit PropertySource(
            _In_ const ctl::ComPtr<DirectUI::DependencyObject>& source,
            _In_ wil::unique_propertychainsource&& data)
            : Source(source)
            , SourceData(std::move(data))
        {
        }
        ctl::ComPtr<DirectUI::DependencyObject> Source;
        wil::unique_propertychainsource SourceData;
    };

    // PropertyChainEvaluator has an exception based implementations. Usage of this type should be
    // wrapped around a try-catch handler. It is meant to be used at the API boundary for XamlDiagnostics
    // for the GetPropertyValuesChain call
    class PropertyChainEvaluator final
    {
    public:
        explicit PropertyChainEvaluator(
            _In_ DirectUI::DependencyObject* dependencyObject);

        explicit PropertyChainEvaluator(
            _In_ CDependencyObject* dependencyObject);

        _Check_return_ HRESULT Evaluate(_In_ const PropertyChainData& data, _Out_ EvaluatedValue& evaluatedValue);
        _Check_return_ HRESULT EvaluateBinding(_In_ const PropertyChainData& data, _Out_ EvaluatedValue& evaluatedValue);
        wil::unique_propertychainvalue GetIsBindingValid(_In_ const PropertyChainData& data);
        _Check_return_ HRESULT GetDesiredSize(_In_ xaml::IUIElement* uiElement, _Out_ wil::unique_propertychainvalue& desiredSize);

        size_t GetMaxPropertyCount() const;
        size_t GetFoundSourceCount() const;

        PropertyChainIterator begin();
        PropertyChainIterator end();

        PropertyChainEvaluator(PropertyChainEvaluator&&) = default;
        PropertyChainEvaluator(const PropertyChainEvaluator&) = delete;
        PropertyChainEvaluator& operator=(const PropertyChainEvaluator&) = delete;

        std::vector<PropertySource> StealSources();

        ctl::ComPtr<xaml::IDependencyObject> GetSourceAtIndex(
            _In_ uint32_t indexs);

        static _Check_return_ HRESULT GetDefaultValue(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ const CDependencyProperty* depProp,
            _Out_ ctl::ComPtr<IInspectable>& value);

        static _Check_return_ HRESULT GetEffectiveValue(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ const CPropertyBase* baseProp,
            _Out_ ctl::ComPtr<IInspectable>& value);

        static _Check_return_ HRESULT GetAnimatedValue(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ const CDependencyProperty* depProp,
            _Out_ ctl::ComPtr<IInspectable>& value);

        static _Check_return_ HRESULT GetLocalValue(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ const CDependencyProperty* depProp,
            _Out_ ctl::ComPtr<IInspectable>& value);

        static _Check_return_ HRESULT GetStyleValue(
            _In_ const xref_ptr<CStyle>& style,
            _In_ const CDependencyProperty* depProp,
            _Out_ ctl::ComPtr<IInspectable>& value);

        static xref_ptr<CDependencyObject> TryGetAnimatedPropertySource(
            _In_ CDependencyObject* pObject,
            _In_ const CDependencyProperty* pProperty);

        static _Check_return_ HRESULT ValueToString(_In_ const ctl::ComPtr<IInspectable>& val, wil::unique_bstr& string);

    private:

        struct ReportedValue
        {
        public:
            ReportedValue(_In_ const ctl::ComPtr<IInspectable>& val, _In_z_ const wchar_t* valString, MetadataBit metadataBits);

        public:
            ReportedValue() : MetadataBits(MetadataBit::None) {}
 
            static _Check_return_ HRESULT Create(
                _In_ const ctl::ComPtr<IInspectable>& val,
                _In_ bool isBinding,
                _Out_ ReportedValue& reportedValue);
            static _Check_return_ HRESULT Create(
                _In_ const ctl::ComPtr<IInspectable>& val, 
                _In_z_ const wchar_t* valString,
                _Out_ ReportedValue& reportedValue);

            ctl::ComPtr<IInspectable>   Value;
            wil::unique_bstr            ValueString;
            long long                   MetadataBits;
        };

        using ValueAndSource = std::pair<ctl::ComPtr<IInspectable>, ctl::ComPtr<DirectUI::DependencyObject>>;
        _Check_return_ HRESULT GetValueAndSource(_In_ const PropertyChainData& data, _Out_ ValueAndSource& valAndSource) const;

        _Check_return_ HRESULT GetPropertyData(
            _In_ const CPropertyBase* property,
            _In_ ReportedValue& reportedValue,
            _In_ unsigned int sourceIndex,
             _Out_ wil::unique_propertychainvalue& propertyValue) const;

        // Gets the index for the setter.
        _Check_return_ HRESULT GetSourceIndex(
            _In_ const ctl::ComPtr<DirectUI::DependencyObject>& pDO,
            _In_ BaseValueSource source,
            _Out_ unsigned int* sourceIndex);

        // Ensures the sources vector has been created. This has to be created before we evalute properties
        // because the order matters
        _Check_return_ HRESULT EnsureSourcesVector();
        _Check_return_ HRESULT EvaluateSource(const xref_ptr<CDependencyObject>& sourceDo, BaseValueSource source);
        _Check_return_ HRESULT EvaluateStyleChain(xref_ptr<CStyle> style, BaseValueSource source);
        // Returns the string value and the metadata bits associated with the
        // value
        static _Check_return_ HRESULT GetReportedValue(
            _In_ const ctl::ComPtr<IInspectable>& value,
            _In_ const CPropertyBase* property,
            _In_ const ctl::ComPtr<DebugTool::IDebugBindingExpression>& bindingExpr,
            _In_ const ctl::ComPtr<DirectUI::DependencyObject>& setterDO,
            _Out_ ReportedValue& reportedValue);

        static _Check_return_ HRESULT GetSourceData(
            _In_ const ctl::ComPtr<DirectUI::DependencyObject>& pDO,
            _In_ BaseValueSource valueSource, 
            _Out_ wil::unique_propertychainsource& source);

        static wil::unique_bstr GetPropertyName(_In_ const CPropertyBase* depProp);

        static wil::unique_bstr GetStringWithFallback(
            _In_ const xstring_ptr& string,
            _In_z_ const wchar_t* fallback = L"");

    private:
        // Source vector that get's populated as we evalutate properties. We have to keep the sources alive while
        // we are populating so that they can be stored in the handle map.
        std::vector<PropertySource>     m_sources;

        xref_ptr<CDependencyObject>     m_do;
        KnownPropertyIndex              m_previousIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    };
}

