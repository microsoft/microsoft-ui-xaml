// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <array>

#include "inc\PropertyChainEvaluator.h"
#include "inc\PropertyChainIterator.h"
#include "inc\Unsealer.h"
#include "primitiveDependencyObjects\inc\primitives.h"
#include "valueboxer\inc\CValueBoxer.h"
#include "valueboxer\inc\BoxerBuffer.h"
#include "xamlDiagnostics\inc\XamlDiagnostics.h"
#include "xstring\inc\XStringBuilder.h"
#include "MetadataAPI.h"
#include "DependencyObject.h"
#include "DiagnosticsInterop.h"
#include "ThemeResourceExpression.h"
#include "DXamlServices.h"
#include "Indexes.g.h"
#include "Enums.g.h"
#include "DOPointerCast.h"
#include "framework.h"
#include "ccontrol.h"
#include "DynamicMetadataStorage.h"
#include "style.h"
#include "xamlDiagnostics\inc\helpers.h"
#include "xamlDiagnostics\inc\RuntimeProperty.h"
#include "setter.h"
#include "HStringUtil.h"
using namespace DirectUI;

namespace Diagnostics {

    PropertyChainEvaluator::PropertyChainEvaluator(
        _In_ DirectUI::DependencyObject* dependencyObject)
        : m_do(dependencyObject->GetHandle())
    {
    }

    PropertyChainEvaluator::PropertyChainEvaluator(
        _In_ CDependencyObject* dependencyObject)
        : m_do(dependencyObject)
    {
    }

    PropertyChainIterator PropertyChainEvaluator::begin()
    {
        return PropertyChainIterator(m_do);
    }

    PropertyChainIterator PropertyChainEvaluator::end()
    {
        return PropertyChainIterator();
    }

    size_t PropertyChainEvaluator::GetMaxPropertyCount() const
    {
        DirectUI::DynamicMetadataStorageInstanceWithLock storage;
        return static_cast<size_t>(storage->GetNextAvailablePropertyIndex());
    }

    size_t PropertyChainEvaluator::GetFoundSourceCount() const
    {
        // We should only be calling this before StealSources.
        ASSERT(m_sources.size() > 0u);
        return m_sources.size();
    }

    std::vector<PropertySource> PropertyChainEvaluator::StealSources()
    {
        return std::move(m_sources);
    }

    _Check_return_ HRESULT PropertyChainEvaluator::Evaluate(_In_ const PropertyChainData& data, _Out_ EvaluatedValue& evaluatedValue)
    {
        PropertyChainEvaluator::ValueAndSource valAndSource;
        IFC_RETURN(GetValueAndSource(data, valAndSource));

        KnownPropertyIndex reportedIndex = data.Index;
        if (reportedIndex == KnownPropertyIndex::Setter_Value)
        {
            // Fix Setter.Value so the DP we are looking for is actually the one associated with Setter.Value
            reportedIndex = DiagnosticsInterop::GetSetterPropertyIndex(checked_cast<CSetter>(m_do));
        }
        const auto reportedProperty = MetadataAPI::GetPropertyBaseByIndex(reportedIndex);

        auto bindingExpr = DiagnosticsInterop::TryGetBindingExpression(m_do->GetDXamlPeer(), data.Source, data.Index);

        // Get the reported value, certain types we can convert automatically to strings.
        // For others, we may want to report a different object (such as GridLength or Bindings)

        ReportedValue reportedValue;
        IFC_RETURN(GetReportedValue(valAndSource.first, reportedProperty, bindingExpr, valAndSource.second, reportedValue));

        const auto originalProperty = MetadataAPI::GetPropertyBaseByIndex(data.Index);
        unsigned int sourceIndex;
        IFC_RETURN(GetSourceIndex(valAndSource.second, data.Source, &sourceIndex));
        wil::unique_propertychainvalue propertyValueChain;
        IFC_RETURN(GetPropertyData(originalProperty, reportedValue, sourceIndex, propertyValueChain));

        m_previousIndex = data.Index;

        evaluatedValue = EvaluatedValue(reportedValue.Value, std::move(propertyValueChain));
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::EvaluateBinding(_In_ const PropertyChainData& data, _Out_ EvaluatedValue& evaluatedValue)
    {
        PropertyChainEvaluator::ValueAndSource valAndSource;
        IFC_RETURN(GetValueAndSource(data, valAndSource));

        auto property = MetadataAPI::GetPropertyBaseByIndex(data.Index);

        ASSERT(DiagnosticsInterop::TryGetBindingExpression(m_do->GetDXamlPeer(), data.Source, data.Index));

        // Pass nullptr for the binding expression so we use the evaluated value
        ReportedValue reportedValue;
        IFC_RETURN(GetReportedValue(valAndSource.first, property, nullptr, valAndSource.second, reportedValue));

        // The local index for the binding is always going to be 0
        const unsigned int localIndex = 0;
        wil::unique_propertychainvalue propertyValueChain;
        IFC_RETURN(GetPropertyData(property, reportedValue, localIndex, propertyValueChain));
        SysFreeString(propertyValueChain.PropertyName);
        propertyValueChain.PropertyName = SysAllocString(L"EvaluatedValue");

        // This property uses a special index so it can be distinguished in the chain
        RuntimeProperty evaluatedValueProperty(FakePropertyIndex::EvaluatedValue);
        propertyValueChain.Index = evaluatedValueProperty.GetIndex();

        evaluatedValue = EvaluatedValue(reportedValue.Value, std::move(propertyValueChain));
        return S_OK;
    }

    wil::unique_propertychainvalue PropertyChainEvaluator::GetIsBindingValid(_In_ const PropertyChainData& data)
    {
        wil::unique_propertychainvalue isValid;
        isValid.PropertyName = SysAllocString(L"IsBindingValid");
        // String type so that it will be displayed correctly.
        isValid.Type = SysAllocString(L"Windows.Foundation.String");
        isValid.ValueType = SysAllocString(L"Windows.Foundation.String");

        // This property uses a special index so it can be distinguished in the chain
        RuntimeProperty isValidProperty(FakePropertyIndex::IsBindingValid);
        isValid.Index = isValidProperty.GetIndex();

        auto bindExpr = DiagnosticsInterop::TryGetBindingExpression(m_do->GetDXamlPeer(), data.Source, data.Index);

        isValid.Value = SysAllocString(bindExpr->IsBindingValid() ? L"True" : L"False");

        auto depProp = MetadataAPI::GetPropertyBaseByIndex(data.Index);
        isValid.DeclaringType = GetStringWithFallback(depProp->GetTargetType()->GetFullName()).release();
        return isValid;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetDesiredSize(_In_ xaml::IUIElement* uiElement, _Out_ wil::unique_propertychainvalue& propValue)
    {
        wf::Size desiredSize = {};
        IFC_RETURN(uiElement->get_DesiredSize(&desiredSize));

        RuntimeProperty desiredSizeProperty(FakePropertyIndex::UIElementDesiredSize);
        propValue.Index = desiredSizeProperty.GetIndex();
        propValue.MetadataBits = MetadataBit::IsPropertyReadOnly;
        IFC_RETURN(GetSourceIndex(nullptr, BaseValueSourceUnknown, &propValue.PropertyChainIndex));
        propValue.PropertyName = SysAllocString(L"DesiredSize");

        WCHAR value[20];  // 20 is arbitrarily large enough to hold the size which is something like 1000x1000.
        value[0] = L'\0';
        IFCEXPECTRC_RETURN(swprintf_s(value, ARRAYSIZE(value), L"%gx%g", desiredSize.Width, desiredSize.Height) > 0, E_FAIL);
        propValue.Value = SysAllocString(value);
        propValue.DeclaringType = SysAllocString(L"Microsoft.UI.Xaml.UIElement");
        propValue.ValueType = SysAllocString(L"Windows.Foundation.Size");
        propValue.Type = SysAllocString(L"Windows.Foundation.Size");

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetValueAndSource(_In_ const PropertyChainData& data, _Out_ ValueAndSource& valAndSource) const
    {
        // Lazy getting of Style properties can try to set properties on the style and if it's sealed, this will fail. This is pretty obnoxious, we shouldn't need
        // this hack for very long once styles become mutable.
        auto resealIfRequired = ObjectUnsealer::UnsealIfRequired(m_do.get());
        auto property = MetadataAPI::GetPropertyBaseByIndex(data.Index);
        auto depProp = property->AsOrNull<CDependencyProperty>();

        ctl::ComPtr<IInspectable> evaluatedValue;
        // The sourceDO either points to the style, the source of the animated
        // property, m_do for local properties, or nullptr for default properties.
        xref_ptr<CDependencyObject> sourceDO;

        switch (data.Source)
        {
        case BaseValueSource::Animation:
            sourceDO = TryGetAnimatedPropertySource(m_do, depProp);
             IFC_RETURN(GetAnimatedValue(m_do, depProp, evaluatedValue));
            break;
        case BaseValueSourceStyle:
        case BaseValueSourceBuiltInStyle:
            IFC_RETURN(GetStyleValue(data.Style, depProp, evaluatedValue));
            sourceDO = data.Style;
            break;
        case BaseValueSourceLocal:
            if (depProp)
            {
                IFC_RETURN(GetLocalValue(m_do, depProp, evaluatedValue));
            }
            else
            {
                IFC_RETURN(GetEffectiveValue(m_do, property, evaluatedValue));
            }
            sourceDO = m_do;
            break;
        case BaseValueSourceUnknown:
        case BaseValueSourceDefault:
            IFC_RETURN(GetDefaultValue(m_do, depProp, evaluatedValue));
            break;
        default:
            IFC_RETURN(E_UNEXPECTED);
        }

        // The peer is used for the source handle so we need to get the peer
        ctl::ComPtr<DirectUI::DependencyObject> sourceDOPeer;
        if (sourceDO)
        {
            IFC_RETURN(DXamlServices::GetPeer(sourceDO, &sourceDOPeer));
        }

        valAndSource = std::make_pair(evaluatedValue, sourceDOPeer);
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetPropertyData(
        _In_ const CPropertyBase* property,
        _In_ ReportedValue& reportedValue,
        _In_ unsigned int sourceIndex,
        _Out_ wil::unique_propertychainvalue& propertyValue) const
    {
        propertyValue.PropertyChainIndex = sourceIndex;
        propertyValue.Index = static_cast<unsigned int>(property->GetIndex());

        // We always start by evaluating the effective value, so if the last index we evaluated is the current,
        // then it has been overridden.
        propertyValue.Overridden = m_previousIndex == property->GetIndex();
        propertyValue.MetadataBits = reportedValue.MetadataBits;
        propertyValue.Value = reportedValue.ValueString.release();

        if (DiagnosticsInterop::IsCollection(reportedValue.Value.Get()))
        {
            propertyValue.MetadataBits |= MetadataBit::IsValueCollection;
        }

        auto dependencyProperty = property->AsOrNull<CDependencyProperty>();
        if (dependencyProperty && dependencyProperty->IsExternalReadOnly())
        {
            propertyValue.MetadataBits |= MetadataBit::IsPropertyReadOnly;
        }

        const CClassInfo* pValueType = nullptr;
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(reportedValue.Value.Get(), &pValueType));

        propertyValue.ValueType = GetStringWithFallback(pValueType->GetFullName()).release();
        propertyValue.PropertyName = GetPropertyName(property).release();
        propertyValue.DeclaringType = GetStringWithFallback(property->GetTargetType()->GetFullName()).release();
        propertyValue.Type = GetStringWithFallback(property->GetPropertyType()->GetFullName()).release();

        return S_OK;
    }

    wil::unique_bstr PropertyChainEvaluator::GetPropertyName(_In_ const CPropertyBase* property)
    {
        auto depProp = property->AsOrNull<CDependencyProperty>();
        if (depProp && depProp->IsAttached())
        {
            // Use XStringBuilder, it graciously handles null strings.
            XStringBuilder attachedPropertyNameBuilder;

            // Don't fail just because we can't build the name.
            VERIFYHR(attachedPropertyNameBuilder.Initialize(depProp->GetDeclaringType()->GetName().GetCount() + /* '.' */ 1 + depProp->GetName().GetCount()));
            VERIFYHR(attachedPropertyNameBuilder.Append(depProp->GetDeclaringType()->GetName()));
            VERIFYHR(attachedPropertyNameBuilder.AppendChar(L'.'));
            VERIFYHR(attachedPropertyNameBuilder.Append(depProp->GetName()));
            return wil::make_bstr_nothrow(attachedPropertyNameBuilder.GetBuffer());
        }
        else
        {
            return GetStringWithFallback(property->GetName(), L"(unnamed)");
        }
    }

    wil::unique_bstr PropertyChainEvaluator::GetStringWithFallback(
        _In_ const xstring_ptr& string,
        _In_z_ const wchar_t* fallback)
    {
          if (!string.IsNullOrEmpty())
          {
              return wil::make_bstr_nothrow(string.GetBuffer());
          }
          else
          {
              return wil::make_bstr_nothrow(fallback);
          }
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetSourceIndex(
        _In_ const ctl::ComPtr<DirectUI::DependencyObject>& setter,
        _In_ BaseValueSource valueSource,
        unsigned int* sourceIndex)
    {
        // Ensure the sources vector has been created at this point.
        IFC_RETURN(EnsureSourcesVector());

        // Use BaseValueSourceDefault if valueSource passed in is BaseValueSourceUnknown
        if (valueSource == BaseValueSourceUnknown)
        {
            valueSource = BaseValueSourceDefault;
        }

        auto iter = std::find_if(m_sources.begin(), m_sources.end(), [&setter, valueSource](const auto& source) {
            return (source.Source == setter) && (source.SourceData.Source == valueSource);
        });

        if (iter != m_sources.end())
        {
            *sourceIndex =  wil::safe_cast_failfast<unsigned int>(iter - m_sources.begin());
        }
        else
        {
            // Sometimes we don't have animated sources, don't crash when that's the case and just report local
            ASSERT(valueSource == BaseValueSource::Animation && setter == nullptr);
            *sourceIndex = 0u;
        }

        return S_OK;
    }

    ctl::ComPtr<xaml::IDependencyObject> PropertyChainEvaluator::GetSourceAtIndex(_In_ uint32_t index)
    {
        auto& source = m_sources.at(index);
        return source.Source;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::EnsureSourcesVector()
    {
        if (m_sources.empty())
        {
            m_sources.reserve(5);  // Reserve 5 for the common sources (Local, Animation, Default, Style, Built-In Style)

            // We evaluate the sources in order that they take precedence so that they accurately display in the LPE.
            // Visual Studio depends on this order because they need to know the order that styles take precedence since
            // they can get multiple sources for BaseValueSourceStyle
            // 1. Animated values
            // 2. Local values
            // 3. Active Style
            // 4. Built-in Style
            // 5. Default

            // First iterate over animated properties
            for (const auto& prop : EnumIterator<KnownPropertyIndex>())
            {
                // TODO: for now only enumerate CDependencyProperties
                auto depProp = MetadataAPI::GetPropertyBaseByIndex(prop)->AsOrNull<CDependencyProperty>();

                if (depProp && m_do->IsAnimatedProperty(depProp))
                {
                    auto animatedSource = m_do->TryGetAnimatedPropertySource(depProp);
                    if (animatedSource)
                    {
                        IFC_RETURN(EvaluateSource(animatedSource, BaseValueSource::Animation));
                    }
                }
            }

            // Add local
            IFC_RETURN(EvaluateSource(m_do, BaseValueSourceLocal));

            // Add Active style and its style chain
            auto doAsFe = do_pointer_cast<CFrameworkElement>(m_do);
            if (doAsFe)
            {
                IFC_RETURN(EvaluateStyleChain(doAsFe->GetActiveStyle(), BaseValueSourceStyle));
            }

            // Add Built-In style and its style chain
            auto control = do_pointer_cast<CControl>(m_do);
            if (control)
            {
                xref_ptr<CStyle> style;
                IFC_RETURN(control->GetBuiltInStyle(style.ReleaseAndGetAddressOf()));
                IFC_RETURN(EvaluateStyleChain(style, BaseValueSourceBuiltInStyle));
            }

            // Add default
            IFC_RETURN(EvaluateSource(nullptr, BaseValueSourceDefault));

            m_sources.shrink_to_fit();
        }
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::EvaluateSource(const xref_ptr<CDependencyObject>& sourceDO, BaseValueSource source)
    {
        ctl::ComPtr<DirectUI::DependencyObject> peer;
        if (sourceDO)
        {
            IFC_RETURN(DirectUI::DXamlServices::GetPeer(sourceDO.get(), &peer));
        }
        wil::unique_propertychainsource sourceData;
        IFC_RETURN(GetSourceData(peer, source, sourceData));
        m_sources.push_back(PropertySource(peer, std::move(sourceData)));
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::EvaluateStyleChain(xref_ptr<CStyle> style, BaseValueSource source)
    {
        while (style)
        {
            IFC_RETURN(EvaluateSource(style, source));
            style.reset(style->m_pBasedOn);
        }
        return S_OK;
    }

    PropertyChainEvaluator::ReportedValue::ReportedValue(_In_ const ctl::ComPtr<IInspectable>& val, _In_z_ const wchar_t* valString, MetadataBit metadataBits)
        : Value(val)
        , ValueString(valString ? wil::make_bstr_failfast(valString) : nullptr)
        , MetadataBits(metadataBits)
    {
    }

    _Check_return_ HRESULT PropertyChainEvaluator::ReportedValue::Create(
        _In_ const ctl::ComPtr<IInspectable>& val,
        _In_z_ const wchar_t* valString,
        _Out_ ReportedValue& reportedValue)
    {
        reportedValue = ReportedValue(val, valString, MetadataBit::None);
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::ReportedValue::Create(
        _In_ const ctl::ComPtr<IInspectable>& val,
        _In_ bool isBinding,
        _Out_ ReportedValue& reportedValue)
    {
        reportedValue = ReportedValue(val, nullptr, XamlDiagnostics::CanConvertValueToString(val.Get()) ? MetadataBit::None : MetadataBit::IsValueHandle);
        IFC_RETURN(PropertyChainEvaluator::ValueToString(val, reportedValue.ValueString));

        if (isBinding)
        {
            reportedValue.MetadataBits |= MetadataBit::IsValueBindingExpression;
        }

        if (!val)
        {
            reportedValue.MetadataBits |= MetadataBit::IsValueNull;
        }

        return S_OK;
    }
        
    _Check_return_ HRESULT PropertyChainEvaluator::GetReportedValue(
        _In_ const ctl::ComPtr<IInspectable>& value,
        _In_ const CPropertyBase* property,
        _In_ const ctl::ComPtr<DebugTool::IDebugBindingExpression>& bindingExpr,
        _In_ const ctl::ComPtr<DirectUI::DependencyObject>& setterDO,\
        _Out_ PropertyChainEvaluator::ReportedValue& reportedValue)

    {
        // First use the binding expression if it was passed in
        if (bindingExpr)
        {
            IFC_RETURN(ReportedValue::Create(bindingExpr->GetBindingAsDO(), true, reportedValue));
            return S_OK;
        }

        // Convert the value to a string. For enums, we query the numeric value of the enum
        // and output that.
        auto propValue = value.AsOrNull<wf::IPropertyValue>();
        if (propValue && property->GetPropertyType()->IsEnum())
        {
            UINT enumValue = 0;
            wf::PropertyType valueType = wf::PropertyType_Empty;
            IFC_RETURN(propValue->get_Type(&valueType));
            bool isKnown = valueType == wf::PropertyType_UInt32 || valueType == wf::PropertyType_Int32 || valueType == wf::PropertyType_OtherType;
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(isKnown);

            if (valueType == wf::PropertyType_UInt32)
            {
                IFC_RETURN(propValue->GetUInt32(&enumValue));
            }
            else if (valueType == wf::PropertyType_Int32)
            {
                int32_t enumValueInt32 = 0;
                IFC_RETURN(propValue->GetInt32(&enumValueInt32));
                enumValue = static_cast<uint32_t>(enumValueInt32);
            }
            else if (valueType == wf::PropertyType_OtherType)
            {
                if (FAILED(GetEnumValueFromKnownWinRTBox(value.Get(), property->GetPropertyType(), &enumValue)))
                {
                    // This enum couldn't be converted to an integer, and we don't know how to properly get the
                    // value from it.
                    isKnown = false;
                }
            }

            if (isKnown)
            {
                std::array<wchar_t, 32> valueBuffer;
                swprintf_s(valueBuffer.data(), valueBuffer.size(), L"%d", enumValue);
                IFC_RETURN(ReportedValue::Create(value, valueBuffer.data(), reportedValue));
                return S_OK;
            }
            else
            {
                IFC_RETURN(ReportedValue::Create(value, L"(unknown)", reportedValue));
                return S_OK;
            }
        }

        auto expression = value.AsOrNull<DirectUI::ThemeResourceExpression>();
        auto dependencyProperty = property->AsOrNull<CDependencyProperty>();
        if (expression && dependencyProperty)
        {
            // If the value is a ThemeResourceExpression, we want to get the value of the expression (i.e. SolidColorBrush)
            // we we can then pass to the valueToString method.
            ctl::ComPtr<IInspectable> fixedValue;
            IFC_RETURN(expression->GetValue(setterDO.Get(), dependencyProperty, &fixedValue));
            IFC_RETURN(ReportedValue::Create(fixedValue, false, reportedValue));
            return S_OK;
        }

        if (propValue && dependencyProperty && dependencyProperty->IsGridLengthProperty())
        {
            // Similar to the IThemeResourceExpression case, we want to rewrap the value inside a grid length
            // object, this way the and
            // not some random double. This is helpful because we interpret "NaN" to be "Auto" in the case of Width/Height.

            // The incoming property value could be a string or double so we should get the class info for the value
            const CClassInfo* propertyInfo = nullptr;
            IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(value.Get(), &propertyInfo));

            // Now that we have the correct class info we can box the object to the correct CValue.
            CValue boxedValue;
            BoxerBuffer buffer;
            DependencyObject* pMOR = nullptr;
            IFC_RETURN(CValueBoxer::BoxObjectValue(&boxedValue, propertyInfo, value.Get(), &buffer, &pMOR));
            auto releaseMOR = wil::scope_exit([&pMOR]()
            {
                ctl::release_interface(pMOR);
            });

            // Now that the CValue we have represents either a string or double representation of the value, we can unbox to an actual GridLength object and replace
            // the value of propValue so that ConvertValueToStringOverride will know it is actually dealing with a length property
            auto lengthInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::GridLength);
            IFC_RETURN(CValueBoxer::UnboxObjectValue(&boxedValue, lengthInfo, lengthInfo->IsNullable(), __uuidof(wf::IReference<xaml::GridLength>), reinterpret_cast<void**>(propValue.ReleaseAndGetAddressOf())));
        }

        // Default case is that we don't know how to convert this to a string so we convert it to a handle
        IFC_RETURN(ReportedValue::Create(propValue ? propValue : value, false, reportedValue));
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetDefaultValue(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ const CDependencyProperty* depProp,
         _Out_ ctl::ComPtr<IInspectable>& value)
    {
        const CDependencyProperty* pUnderlyingDP = nullptr;
        if (depObj->GetBaseValueSource(depProp) == BaseValueSourceDefault)
        {
            IFC_RETURN(GetEffectiveValue(depObj, depProp, value));
        }
        else if (SUCCEEDED(MetadataAPI::GetUnderlyingDependencyProperty(depProp, &pUnderlyingDP)))
        {
            IFC_RETURN(depObj->GetDXamlPeer()->GetDefaultValueInternal(pUnderlyingDP, &value));
        }

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetEffectiveValue(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ const CPropertyBase* baseProp,
        _Out_ ctl::ComPtr<IInspectable>& value)
    {
         if (auto depProp = baseProp->AsOrNull<CDependencyProperty>())
        {
            if (auto customProperty = depProp->AsOrNull<CCustomProperty>())
            {
                IFC_RETURN(customProperty->GetXamlPropertyNoRef()->GetValue(ctl::iinspectable_cast(depObj->GetDXamlPeer()), &value));
            }
            else
            {
                // If depObj is a CApplication object, GetDXamlPeer() returns null since the CApplication peer doesn't inherit from DependencyObject
                // (even though CApplication inherits from CDependencyObject).  In that case we'll just call the GetValue on the CDependencyObject directly.

                DirectUI::DependencyObject* pPeer = depObj->GetDXamlPeer();

                if (pPeer != nullptr)
                {
                    IFC_RETURN(pPeer->GetValue(depProp, &value));
                    if (auto style = do_pointer_cast<CStyle>(depObj.get()))
                    {
                        if (depProp->GetIndex() == KnownPropertyIndex::Style_Setters)
                        {
                            style->RegisterSetterCollection();
                        }
                    }
                }
                else
                {
                    // The only case we shouldn't get a peer is if depObj is really a CApplication object.
                    ASSERT((depObj->OfTypeByIndex<KnownTypeIndex::Application>()));
                    CValue cVal;
                    IFC_RETURN(depObj->GetValue(depProp, &cVal));
                    IFC_RETURN(CValueBoxer::UnboxObjectValue(&cVal, depProp->GetPropertyType(), &value));
                }
            }
        }
        else if (auto simpleProp = baseProp->AsOrNull<CSimpleProperty>())
        {
            // TODO: add getter for simple properties
            IFC_RETURN(DiagnosticsInterop::GetValueOfSimpleProperty(simpleProp, depObj.get(), &value));
        }

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetAnimatedValue(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ const CDependencyProperty* depProp,
        _Out_ ctl::ComPtr<IInspectable>& value)
    {
 
        if (auto customProperty = depProp->AsOrNull<CCustomProperty>())
        {
            IFC_RETURN(customProperty->GetXamlPropertyNoRef()->GetValue(ctl::iinspectable_cast(depObj->GetDXamlPeer()), &value));
        }
        else
        {
            CValue animatedValue;
            IFC_RETURN(depObj->GetAnimatedValue(depProp, &animatedValue));
            IFC_RETURN(CValueBoxer::UnboxObjectValue(&animatedValue, depProp->GetPropertyType(), &value));
        }

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetLocalValue(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ const CDependencyProperty* depProp,
            _Out_ ctl::ComPtr<IInspectable>& value)
    {
        CValue localValue;
        bool hasLocalValue = false;
        bool isTemplateBound = false;

        IFC_RETURN(depObj->ReadLocalValue(depProp, &localValue, &hasLocalValue, &isTemplateBound));

        if (!hasLocalValue)
        {
            // Grid.Row and Grid.Column definitions are special and that they are always considered default
            if (depProp->GetIndex() == KnownPropertyIndex::Grid_ColumnDefinitions ||
                depProp->GetIndex() == KnownPropertyIndex::Grid_RowDefinitions)
            {
                IFC_RETURN(depObj->GetValue(depProp, &localValue));
            }
        }

        if (!localValue.IsNullOrUnset())
        {
            if (depProp->GetIndex() == KnownPropertyIndex::Setter_Value)
            {
                // workaround for Bug 18324174: property system doesn't report correct enum types, so unboxing doesn't work properly
                auto setterProperty = MetadataAPI::GetPropertyByIndex(DiagnosticsInterop::GetSetterPropertyIndex(checked_cast<CSetter>(depObj)));
                IFC_RETURN(CValueBoxer::UnboxObjectValue(&localValue, setterProperty->GetPropertyType(), &value));
            }
            else
            {
                IFC_RETURN(CValueBoxer::UnboxObjectValue(&localValue, depProp->GetPropertyType(), &value));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetStyleValue(
        _In_ const xref_ptr<CStyle>& style,
        _In_ const CDependencyProperty* depProp,
        _Out_ ctl::ComPtr<IInspectable>& value)
    {
        // Don't include based-on setters
        const bool getPropertyFromBasedOn = false;
        bool gotValue = false;

        CValue cvalue;
        IFC_RETURN(style->GetPropertyValue(depProp->GetIndex(), getPropertyFromBasedOn, &cvalue, &gotValue));
        IFCEXPECTRC_RETURN(gotValue, E_INVALIDARG);
        IFC_RETURN(CValueBoxer::UnboxPropertyObjectValue(&cvalue, depProp, &value));

        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::ValueToString(_In_ const ctl::ComPtr<IInspectable>& value, wil::unique_bstr& string)
    {
        xstring_ptr valueString;
        IFC_RETURN(XamlDiagnosticsHelpers::ValueToString(value, &valueString));

        string = wil::make_bstr_failfast(valueString.GetBuffer());
        return S_OK;
    }

     _Check_return_ HRESULT PropertyChainEvaluator::GetSourceData(
         _In_ const ctl::ComPtr<DirectUI::DependencyObject>& sourceAsDO,
         _In_ BaseValueSource valueSource,
         _Out_ wil::unique_propertychainsource& source)
     {
         IFC_RETURN(HandleMap::CreateHandle(sourceAsDO.Cast<xaml::IDependencyObject>(), &source.Handle));

         auto style = sourceAsDO.AsOrNull<xaml::IStyle>();
         if (style)
         {
             wxaml_interop::TypeName styleTargetType = {};
             IFC_RETURN(style->get_TargetType(&styleTargetType));
             source.TargetType = SysAllocString(HStringUtil::GetRawBuffer(styleTargetType.Name, nullptr));
         }
         else
         {
             source.TargetType = SysAllocString(L"");
         }

         source.Name = SysAllocString(L"");
         source.Source = valueSource;
         if (auto sourceInfo = sourceAsDO.AsOrNull<xaml::ISourceInfoPrivate>())
         {
            source.SrcInfo = XamlDiagnostics::GetSourceInfo(sourceInfo.Get()).release();
         }

         return S_OK;
     }

     xref_ptr<CDependencyObject> PropertyChainEvaluator::TryGetAnimatedPropertySource(
         _In_ CDependencyObject* pObject,
         _In_ const CDependencyProperty* pProperty)
     {
         auto source = pObject->TryGetAnimatedPropertySource(pProperty);

         // Only Hyperlinks and ListViewBaseItemChrome call SetAnimatedValue without a setter. This is ok behavior, so we'll
         // assert here that this object is one of the two. The assertion is really here to catch someone breaking the current
         // behavior of the property system and doesn't hold onto the original setter when diagnostics is enabled. If other types
         // start doing what Hyperlink and ListViewBaseItemChrome do and this assertion becomes too onerous to maintain, we can
         // remove it.
         if (source == nullptr)
         {
             // Add logging so if this shows up in tests we can see what type it is.
             LOG_INFO_EX(L"NO ANIMATED SOURCE: %s", pObject->GetClassName().GetBuffer());
             ASSERT(pObject->OfTypeByIndex<KnownTypeIndex::Hyperlink>() || pObject->OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>());
         }

         return source;
     }
}

