// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DiagnosticsInterop.h"
#include "FrameworkApplication.g.h"
#include "Setter.g.h"
#include "Style.g.h"
#include "DynamicMetadataStorage.h"
#include <CColor.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <functional>
#include "ElementCompositionPreview.g.h"
#include "SetterBaseCollection.g.h"
#include "XamlDiagnostics.h"
#include "collectionbase.h"
#include "CVisualStateManager2.h"
#include "VisualState.h"
#include "docollection.h"
#include "TimelineCollection.h"
#include "TimelineCollection.g.h"
#include "storyboard.h"
#include "timeline.h"
#include "Timeline_partial.h"
#include "VisualStateSetterHelper.h"
#include "ThemeResourceExpression.h"
#include "VisualTreeHelper.h"
#include "DynamicValueConverter.h"
#include "ResourceDictionary.g.h"
#include "ResourceDictionary_Partial.h"
#include "wilhelper.h"
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "deferral\inc\CustomWriterRuntimeContext.h"
#include "primitiveDependencyObjects\inc\primitives.h"
#include "Binding_Partial.h"
#include "BindingExpression_Partial.h"
#include "StateTriggerBase_Partial.h"
#include "VisualStateManagerDataSource.h"
#include <DiagnosticsInterop_SimpleProperties.g.h>
#include "XcpAllocationDebug.h"
#include "ObjectKeyFrame.h"
#include "ObjectKeyFrame.g.h"
#include "DesktopWindowXamlSource_partial.h"
#include "XamlIsland_partial.h"
#include "Window_Partial.h"
#include "RuntimeObjectCache.h"
#include "RuntimeElement.h"
#include "XamlIslandRoot_Partial.h"
#include "XamlIslandRoot.h"
#include "XamlRoot_Partial.h"
#include "CRootScrollViewer.g.h"
#include "ScrollViewer.h"
#include "FrameworkApplication_Partial.h"
#include "DebugSettings_Partial.h"
#include "XamlIslandRoot_Partial.h"
#include "Unsealer.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

#define E_UNKNOWNTYPE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, 40L)

using namespace DirectUI;
using namespace RuntimeFeatureBehavior;

namespace Diagnostics
{
    std::shared_ptr<DiagnosticsInterop> GetDiagnosticsInterop(bool create)
    {
        // We keep XamlDiagnostics alive while running leak detection, so ignore this object
#if XCP_MONITOR
        auto lock = XcpDebugStartIgnoringLeaks();
#endif
        static PROVIDE_DEPENDENCY(DiagnosticsInterop);
        static DependencyLocator::Dependency<DiagnosticsInterop> s_diagInterop;

        return s_diagInterop.Get(create);
    }

    DiagnosticsInterop::DiagnosticsInterop()
        : m_diagnostics(nullptr)
    {
        m_mainThreadId = GetCurrentThreadId();
    }

    DiagnosticsInterop::~DiagnosticsInterop()
    {
    }

    bool DiagnosticsInterop::IsEnabledForThread(DWORD threadId)
    {
        // Since diagnostics interop can live across all threads, make sure this is one where we have an actual core
        // and isn't a background thread. It's also possible someone doesn't do a runtime check to make sure diagnostics
        // is enabled so make sure we have a valid instance.
        if (!(DXamlCore::IsInitializedStatic() || DXamlCore::IsShuttingDownStatic()) || !m_diagnostics)
        {
            return false;
        }

        return GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeEnabledFeature::XamlDiagnostics);
    }

    void DiagnosticsInterop::Launch(
        _In_ const wrl::ComPtr<msy::IDispatcherQueue>& dispatcher)
    {
        // Only launch on the main thread ID.
        if (m_diagnostics && GetCurrentThreadId() == m_mainThreadId)
        {
            m_diagnostics->Launch(DebugTool::EnvironmentMap(), dispatcher.Get());
        }
    }

    void DiagnosticsInterop::Launch(
        _In_ ctl::ComPtr<IXamlDiagnostics> diagnostics,
        _In_ const DebugTool::EnvironmentMap& map)
    {
        m_diagnostics = static_cast<XamlDiagnostics*>(diagnostics.Get());
        if (m_diagnostics)
        {
            m_diagnostics->Launch(map, nullptr);
        }
    }

    void DiagnosticsInterop::SignalMutation(
        _In_ xaml::IDependencyObject* pReference,
        _In_ VisualMutationType mutationType
    )
    {
        if (m_diagnostics)
        {
           m_diagnostics->SignalMutation(pReference, mutationType);
        }
    }

    void DiagnosticsInterop::SignalRootMutation(
        _In_ IInspectable* root,
        _In_ VisualMutationType mutationType)
    {
        if (m_diagnostics && IsEnabledForThread(GetCurrentThreadId()))
        {
            m_diagnostics->SignalRootMutation(root, mutationType);
        }
    }

    std::vector<wrl::ComPtr<msy::IDispatcherQueue>> DiagnosticsInterop::GetDispatcherQueues()
    {
        std::vector<wrl::ComPtr<msy::IDispatcherQueue>> queues;
        if (auto application = DirectUI::FrameworkApplication::GetCurrentNoRef())
        {
            ctl::ComPtr<xaml::IDebugSettings> debugSettings;
            IGNOREHR(application->get_DebugSettings(&debugSettings));
            if (debugSettings)
            {
                queues = debugSettings.Cast<DebugSettings>()->GetDispatcherQueues();
            }
        }
        return queues;
    }

    Microsoft::WRL::ComPtr<msy::IDispatcherQueue> DiagnosticsInterop::GetDispatcherQueueForThreadId(DWORD threadId)
    {
        wrl::ComPtr<msy::IDispatcherQueue> queue;
        if (auto application = DirectUI::FrameworkApplication::GetCurrentNoRef())
        {
            ctl::ComPtr<xaml::IDebugSettings> debugSettings;
            IGNOREHR(application->get_DebugSettings(&debugSettings));
            if (debugSettings)
            {
                queue = debugSettings.Cast<DebugSettings>()->GetDispatcherQueueForThreadId(threadId);
            }
        }
        return queue;
    }

    void DiagnosticsInterop::OnCoreDeinitialized(DWORD coreThreadId)
    {
        // Only callback to xaml diagnostics if enabled for this thread.
        if (m_diagnostics && IsEnabledForThread(coreThreadId))
        {
            VERIFYHR(m_diagnostics->OnCoreDeinitialized(coreThreadId));
        }
    }

    xref_ptr<CDependencyObject> DiagnosticsInterop::ConvertToCore(
        _In_ IInspectable* object,
        _Out_opt_ bool* wasPeerPegged)
    {
        if (wasPeerPegged != nullptr)
        {
            *wasPeerPegged = false;
        }

        ctl::ComPtr<IInspectable> obj(object);
        auto dependencyObject = obj.AsOrNull<DirectUI::DependencyObject>();
        if (dependencyObject)
        {
            return xref_ptr<CDependencyObject>(dependencyObject->GetHandle());
        }

        auto application = obj.AsOrNull<xaml::IApplication>();
        if (application && DXamlServices::IsDXamlCoreInitialized())
        {
            return xref_ptr<CDependencyObject>(DXamlCore::GetCurrent()->GetCoreAppHandle());
        }

        // For certain property values, we have corresponding CDependencyObject types that we use at parsing (ex: x:Double, x:String, etc).
        // Ideally, visual studio could call CreateInstance with "x:Double" as the type.
        auto propValue = obj.AsOrNull<wf::IPropertyValue>();
        if (propValue)
        {
            CValue boxedValue;
            BoxerBuffer buffer;
            ctl::ComPtr<DirectUI::DependencyObject> mor;
            THROW_IF_FAILED(CValueBoxer::BoxObjectValue(&boxedValue, nullptr, propValue.Get(), &buffer, &mor));

            // Get the type info and get the class constructor for the core dependency object.
            const CClassInfo* typeInfo = nullptr;

            THROW_IF_FAILED(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(propValue.Get(), &typeInfo));

            auto constructor = typeInfo->GetCoreConstructor();
            if (constructor)
            {
                CREATEPARAMETERS params(DXamlCore::GetCurrent()->GetHandle(), boxedValue);

                xref_ptr<CDependencyObject> coreObj;
                // Don't fail if we couldn't create the core object. We'll just wrap in an ExternalObjectReferences in that case, which is
                // expected for types that we can't convert to core.
                if (SUCCEEDED(constructor(coreObj.ReleaseAndGetAddressOf(), &params)))
                {
                    return coreObj;
                }
            }
        }

        if (obj)
        {
            // Wrap other IInspectable types in ExternalObjectReferences.
            BOOLEAN wasWrapped;
            ctl::ComPtr<DependencyObject> valueDO;
            THROW_IF_FAILED(ExternalObjectReference::ConditionalWrap(obj.Get(), &valueDO, &wasWrapped));

            if (wasWrapped)
            {
                // Propagate this value to the core since it might be needed there (e.g. for resource lookup of a non-DO like Color).
                THROW_IF_FAILED(valueDO.Cast<ExternalObjectReference>()->put_NativeValue(obj.Get()));
                xref_ptr<CDependencyObject> coreObj(valueDO->GetHandle());
                coreObj->PegManagedPeer();
                // We'll cause a leak if we don't unpeg the peer
                MICROSOFT_TELEMETRY_ASSERT_DISABLED(wasPeerPegged);
                if (wasPeerPegged != nullptr)
                {
                    *wasPeerPegged = true;
                }
                return coreObj;
            }
        }
        return nullptr;
    }

    bool DiagnosticsInterop::TryGetDictionaryItem(
        _In_ xaml::IResourceDictionary* resourceDictionary,
        const xstring_ptr_view& keyAsString,
        bool isImplicitStyle,
        _COM_Outptr_result_maybenull_ IInspectable** item)
    {
        return SUCCEEDED(static_cast<DirectUI::ResourceDictionary*>(resourceDictionary)->TryGetItemCore(keyAsString, isImplicitStyle, item));
    }

    HRESULT DiagnosticsInterop::EnsureElementInCorrectNamescope(
        _In_ const ctl::ComPtr<IInspectable>& parent,
        _In_ const ctl::ComPtr<IInspectable>& child)
    {
        ctl::ComPtr<xaml::IDependencyObject> childAsDO;

        // If this element isn't a DO then return early, at least we tried...
        if (FAILED(child.As(&childAsDO)))
        {
            return S_OK;
        }

        // We'll start with seeing if this object already has a templated parent, if it does, we'll just use that. This can
        // happen if an object is created, added to the tree inside a template, and then has a property set on it. If that property
        // is the name property, CDependencyObject::SetName currently won't register the name, so we'll do it ourselves.
        auto coreChild = static_cast<CDependencyObject*>(static_cast<DependencyObject*>(childAsDO.Get())->GetHandle());

        CDependencyObject* templatedParent = coreChild->GetTemplatedParent();
        bool isTemplateNamescopeMember = coreChild->IsTemplateNamescopeMember();
        CDependencyObject* namescopeOwner = templatedParent;

        if (!templatedParent)
        {
            ASSERT(!isTemplateNamescopeMember);
            std::tie(templatedParent, isTemplateNamescopeMember) = TryFindTemplatedParent(coreChild);
            // If a templated parent was found, we'll set it on the object and use the
            // templated parent for the namescope owner
            if (isTemplateNamescopeMember && templatedParent)
            {
                IFC_RETURN(coreChild->SetTemplatedParent(templatedParent));
                coreChild->SetIsTemplateNamescopeMember(isTemplateNamescopeMember);
                namescopeOwner = templatedParent;
            }
            else
            {
                // otherwise fallback to the standard namescope owner
                namescopeOwner = coreChild->GetStandardNameScopeOwner();
            }
        }

        // Register the object with the correct namescope
        if (namescopeOwner)
        {
            IFC_RETURN(coreChild->RegisterName(namescopeOwner, isTemplateNamescopeMember));
        }

        return S_OK;
    }

    std::tuple<CDependencyObject*, bool> DiagnosticsInterop::TryFindTemplatedParent(_In_ CDependencyObject* child)
    {
        CDependencyObject* coreParent = nullptr;
        bool isTemplateNamescopeMember = false;
        auto visualState = child->OfTypeByIndex<KnownTypeIndex::VisualState>() ?
            checked_cast<CVisualState>(child) : TryFindVisualState(child);
        if (visualState != nullptr)
        {
            coreParent = CVisualStateManager2::GetGroupCollectionFromVisualState(visualState);
        }
        else if (auto uiElement = do_pointer_cast<CUIElement>(child))
        {
            coreParent = uiElement->GetLogicalParentNoRef();
        }
        else
        {
            coreParent = child->GetParentInternal(false);
        }

        CDependencyObject *templatedParent = nullptr;
        if (coreParent)
        {
            templatedParent = coreParent->GetTemplatedParent();
            if (coreParent->OfTypeByIndex<KnownTypeIndex::VisualStateGroupCollection>() && templatedParent)
            {
                // The isTemplateNamescopeMember bit doesn't propagate properly for VisualStateGroupCollection(s)
                // If the child is in a VisualState and have a templated parent, then we are a templateNamescopeMember.
                isTemplateNamescopeMember = true;
            }
            else
            {
                isTemplateNamescopeMember = coreParent->IsTemplateNamescopeMember();
            }
        }

        return std::make_tuple(templatedParent, isTemplateNamescopeMember);
    }

HRESULT
DiagnosticsInterop::CreateInstance(
    _In_ LPCWSTR typeName,
    _In_opt_ LPCWSTR value,
    _Outptr_ IInspectable** ppInstance)
{
    const CClassInfo* pType = nullptr;
    *ppInstance = nullptr;

    //If we are trying to create a DependencyProperty, handle it specially
    if (_wcsnicmp(typeName, L"Microsoft.UI.Xaml.DependencyProperty", 34) == 0)
    {
        const CPropertyBase* pProp = nullptr;
        IFC_RETURN(MetadataAPI::TryGetPropertyByFullName(xephemeral_string_ptr(value, wcslen(value)), &pProp));

        const CDependencyProperty* pDP = nullptr;
        if (pProp != nullptr)
        {
            pDP = pProp->AsOrNull<CDependencyProperty>();
        }

        //If we couldn't get the dependency property, return an error
        if (pDP == nullptr)
        {
            return E_INVALIDARG;
        }
        ctl::ComPtr<xaml::IDependencyProperty> spIDP;
        ctl::ComPtr<IInspectable> spIDPInspectable;
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(pDP->GetIndex(), &spIDP));
        IFC_RETURN(spIDP.As(&spIDPInspectable));
        *ppInstance = spIDPInspectable.Detach();
        return S_OK;
    }

    IFC_RETURN(MetadataAPI::GetClassInfoByFullName(xephemeral_string_ptr(typeName, wcslen(typeName)), &pType));

    const bool hasCustomTypeConverter = !pType->IsBuiltinType() && pType->HasTypeConverter() || pType->GetIndex() == KnownTypeIndex::IMediaPlaybackSource;
    // GetClassInfoByFullName guarantees on success that pType is not null, so we can go ahead and skip any checks.
    if (!pType->IsEnum() && pType->GetBaseType()->GetIndex() == KnownTypeIndex::UnknownType && !hasCustomTypeConverter && !pType->IsValueType())
    {
        IFC_RETURN(E_FAIL);
    }

    // This is sort of hacky but if the type is a double and the value being set is "Auto", then we will create the
    // instance through the CValueBoxer. This is to enable the scenario where a developer changes the width property
    // to "Auto" when debugging. This happens in markup because our parser has some custom logic and understanding
    // of the property being set that isn't accessible in this scenario so we have to do it for all doubles.
    if (pType->GetIndex() == KnownTypeIndex::Double && wcslen(value) == 4 && _wcsnicmp(value, L"Auto", 4) == 0)
    {
        CValue val;
        val.SetDouble(XDOUBLE_NAN);

        return CValueBoxer::UnboxObjectValue(&val, pType, ppInstance);
    }
    else if (value)
    {
        if (hasCustomTypeConverter)
        {
            if (pType->GetIndex() == KnownTypeIndex::IMediaPlaybackSource)
            {
                // There is a special case inside ActivationAPI::ActivateInstanceFromString that handles the MediaPlaybackItemConverter
                // which is used for setting MediaPlayerElement.Source
                pType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::MediaPlaybackItemConverter);
            }
            return ActivationAPI::ActivateInstanceFromString(pType, xephemeral_string_ptr(value, wcslen(value)),ppInstance);
        }
        else
        {
            wrl::ComPtr<IInspectable> spValueAsI;

            IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(value).Get(), &spValueAsI));

            if (!m_spValueConverter)
            {
                IFC_RETURN(DynamicValueConverter::CreateConverter(&m_spValueConverter));
            }
            HRESULT hr = m_spValueConverter->Convert(spValueAsI.Get(), pType, nullptr, ppInstance);

            // Most of our simple property types like Vector3, Matrix4x4, etc. aren't known or convertible in the
            // framework and will fail the previous creation attempts.
            // We attempt the above conversion first because CreateSimplePropertyType will create a DirectUI::Reference
            // that only implements IPropertyValue::Value.  So for simple property types like TimeSpans
            // (or other types we may add in the future like GUID, Point, etc.)
            // that are normally supported by IPropertyValue, calls to methods like IPropertyValue::GetTimeSpan
            // unexpectedly fail even if the stored type really is a TimeSpan when that IPropertyValue is created from CreateSimplePropertyType.
            // The above conversion makes a 'proper' IPropertyValue where those methods succeed as expected
            // for those types.
            if (FAILED(hr) && IsCreatableSimplePropertyType(pType))
            {
                return CreateSimplePropertyType(pType, xephemeral_string_ptr(value, wcslen(value)), ppInstance);
            }
#if XCP_MONITOR
            m_spValueConverter.Reset();
#endif
            return hr;
        }
    }
    else
    {
        // If no value is provided, then the caller is creating a FrameworkElement so we go through our
        // ActivationAPI to create the object
        return ActivationAPI::ActivateInstance(pType, ppInstance);
    }
}

HRESULT DiagnosticsInterop::GetName(
    _In_ IInspectable* pReference,
    _Out_ HSTRING* phName)
{
    ctl::ComPtr<xaml::IFrameworkElement> frameworkElement;

    *phName = nullptr;

    if (SUCCEEDED(pReference->QueryInterface(frameworkElement.GetAddressOf())))
    {
        return frameworkElement->get_Name(phName);
    }

    return S_OK;
}

// Returns the type display name for the given object.
HRESULT
DiagnosticsInterop::GetTypeDisplayNameFromObject(
    _In_ IInspectable* pObject,
    _Out_ HSTRING* phDisplayName)
{
    *phDisplayName = nullptr;
    ctl::ComPtr<xaml::IDependencyObject> dependencyObject;
    if (pObject && SUCCEEDED(pObject->QueryInterface(dependencyObject.GetAddressOf())))
    {
        IFC_RETURN(GetTypeNameFor(dependencyObject.Get(), phDisplayName));
    }
    else if (pObject)
    {
        xruntime_string_ptr runtimeTypeName;
        xstring_ptr typeName;

        IFC_RETURN(MetadataAPI::GetFriendlyRuntimeClassName(pObject, &typeName));
        IFC_RETURN(typeName.Promote(&runtimeTypeName));

        *phDisplayName = runtimeTypeName.DetachHSTRING();
    }

    return S_OK;
}

// Returns the type name for the given DO.
HRESULT
DiagnosticsInterop::GetTypeNameFor(
    _In_ IInspectable* reference,
    _Out_ HSTRING* typeName)
{
    xruntime_string_ptr runtimeTypeName;
    const CClassInfo* typeInfo = nullptr;

    IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(reference, &typeInfo));
    IFC_RETURN(typeInfo->GetFullName().Promote(&runtimeTypeName));

    *typeName = runtimeTypeName.DetachHSTRING();
    return S_OK;
}

HRESULT
DiagnosticsInterop::ClearPropertyValue(
    _In_ IInspectable* pReference,
    _In_ uint32_t propertyIndex)
{
    ctl::ComPtr<IInspectable> obj(pReference);

    auto clearPropertyIndex = static_cast<KnownPropertyIndex>(propertyIndex);
    const auto* prop = MetadataAPI::GetPropertyBaseByIndex(clearPropertyIndex);

    if (auto customProp = prop->AsOrNull<CCustomProperty>())
    {
        // For custom properties, we don't care if the object is a DO, we'll
        // call out to IXamlType
        IFC_RETURN(ClearCustomProperty(pReference, customProp));
    }
    else if (auto doObj = obj.AsOrNull<DirectUI::DependencyObject>())
    {
        if (auto depProp = prop->AsOrNull<CDependencyProperty>())
        {
            IFC_RETURN(ClearDependencyProperty(doObj->GetHandle(), depProp));
        }
        else if (auto simpleProp = prop->AsOrNull<CSimpleProperty>())
        {
            ClearSimpleProperty(doObj->GetHandle(), simpleProp);
        }
        else
        {
            // Property isn't a DependencyProperty or SimpleProperty, which isn't expected
            // as those are the only property types we have.
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
            return E_FAIL;
        }
    }
    else
    {
        // Clearing a non custom type on a non-DO isn't supported. Our property
        // system currently requires CDependencyObject under the covers, even
        // if the type isn't one publicly. When that changes, we can add code here
        // or remove the above check for DO.
        MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::ClearCustomProperty(
    _In_ IInspectable* reference,
    const CCustomProperty* customProp)
{
    ASSERT(customProp != nullptr);
    // For non-DOs/non-DPs, ResetValue should just create a default value of the property type
    // via ActivateInstance, and use that as the default value
    const CClassInfo* memberTypeInfo = MetadataAPI::GetClassInfoByIndex(customProp->GetPropertyTypeIndex());
    ctl::ComPtr<IInspectable> defaultValue;
    IFC_RETURN(ActivationAPI::ActivateInstance2(memberTypeInfo, &defaultValue));
    IFC_RETURN(SetPropertyValue(reference, static_cast<uint32_t>(customProp->GetIndex()), defaultValue.Get(), false /*unregisterResource*/));
    return S_OK;
}

HRESULT
DiagnosticsInterop::ClearDependencyProperty(
    _In_ CDependencyObject* obj,
    const CDependencyProperty* prop)
{
    ASSERT(!prop->Is<CCustomProperty>());
    if (PropertyChainIterator::IsValidPropertyForObject(prop, obj))
    {
        auto reseal = ObjectUnsealer::UnsealIfRequired(obj);
        if (prop->GetIndex() == KnownPropertyIndex::VisualState_Storyboard)
        {
            auto visualState = do_pointer_cast<CVisualState>(obj);
            xref_ptr<CStoryboard> storyboard;
            IFC_RETURN(visualState->GetStoryboard(storyboard.ReleaseAndGetAddressOf()));
            IFC_RETURN(CVisualStateManager2::TryRemoveStoryboardFromState(visualState, storyboard));
        }
        // If the peer has state, then there could be an EffectiveValueEntry for a Binding, we'll want
        // to detach that before. Otherwise, there's no need to create a peer if it doesn't exist.
        auto peer = obj->GetDXamlPeer();
        if (peer)
        {
            auto valueEntry = peer->TryGetEffectiveValueEntry(prop->GetIndex());
            if (valueEntry && valueEntry->IsExpression())
            {
                auto expressionInspectable = valueEntry->GetBaseValue();

                ctl::ComPtr<BindingExpressionBase> expressionBase;
                IFC_RETURN(expressionInspectable.As(&expressionBase));

                IFC_RETURN(peer->DetachExpression(prop, expressionBase.Get()));

                const auto resourceGraph = Diagnostics::GetResourceGraph();
                resourceGraph->RemoveMarkupExtensionToTarget(obj, prop->GetIndex());
            }
        }
        IFC_RETURN(obj->ClearValue(prop));

        //If the object we cleared from is a Setter, we need to do some extra work to propagate the change
        auto coreSetter = do_pointer_cast<CSetter>(obj);
        if (coreSetter)
        {
            const bool isAddOperation = false;
            IFC_RETURN(OnSetterChanged(coreSetter, isAddOperation));
        }
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::GetValueOfSimpleProperty(
    _In_ const CPropertyBase* property,
    _In_ CDependencyObject* pDO,
    _Outptr_ IInspectable** ppValue)
{
    IFC_RETURN(GetValueSimpleProperty(property, pDO, ppValue));
    return S_OK;
}

HRESULT
DiagnosticsInterop::GetValueOfCustomProperty(
    _In_ const CPropertyBase* property,
    _In_ IInspectable* pObj,
    _Outptr_ IInspectable** ppValue)
{
    // Handle a non-DO on a non-DO
    if (auto customProp = property->AsOrNull<CCustomProperty>())
    {
        IFC_RETURN(customProp->GetXamlPropertyNoRef()->GetValue(
            pObj,
            ppValue));
    }
    else
    {
        // There was a non-custom user property defined on a non-DO - shouldn't be possible
        return E_NOINTERFACE;
    }
    return S_OK;
}

HRESULT
DiagnosticsInterop::GetAllEnums(
    _Inout_ DebugTool::ICollection<DebugTool::DebugEnumInfo2>* pEnumCollection)
{
    DynamicMetadataStorageInstanceWithLock storage;
    unsigned short typeCount = static_cast<unsigned short>(storage->GetNextAvailableTypeIndex());
    for (unsigned short typeIndex = 1; typeIndex < typeCount; typeIndex++)
    {
        KnownTypeIndex type = static_cast<KnownTypeIndex>(typeIndex);
        const CClassInfo* pInfo = MetadataAPI::GetClassInfoByIndex(type);

        if (pInfo && pInfo->IsEnum() && pInfo->GetIndex() != KnownTypeIndex::Enumerated)
        {
            DebugTool::DebugEnumInfo2 debugEnumInfo;
            std::vector<std::pair<int, PCWSTR>> values;
            UINT nValueTableLength = 0;
            const XTABLE* aValueTable = nullptr;

            if (SUCCEEDED(GetEnumValueTable(type, &nValueTableLength, &aValueTable)))
            {
                for (size_t k = 0; k < nValueTableLength; k++)
                {
values.emplace_back(aValueTable[k].m_nValue, aValueTable[k].m_strStringStorage.Buffer);
                }
            }

            IFC_RETURN(debugEnumInfo.Initialize(static_cast<int>(type), pInfo->GetFullName().GetBuffer(), values));

            IFC_RETURN(pEnumCollection->Append(debugEnumInfo));
        }
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::GetSize(
    _In_ IInspectable* pCollection,
    _Out_ unsigned int* pSize)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    IFCPTR_RETURN(pCollection);

    if (SUCCEEDED(spCollection.As(&spUntypedCollection)))
    {
        return spUntypedCollection->UntypedGetSize(pSize);
    }
    else if (SUCCEEDED(spCollection.As(&spTypedCollection)))
    {
        return spTypedCollection->get_Size(pSize);
    }
    else if (SUCCEEDED(spCollection.As(&spCommandBarCollection)))
    {
        return spCommandBarCollection->get_Size(pSize);
    }
    else if (SUCCEEDED(spCollection.As(&spDOCollection)))
    {
        return spDOCollection->get_Size(pSize);
    }

    return E_UNKNOWNTYPE;
}


HRESULT
DiagnosticsInterop::GetAt(
    _In_ IInspectable* pCollection,
    _In_ unsigned int index,
    _Outptr_ IInspectable** ppInstance)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    IFCPTR_RETURN(pCollection);

    if (SUCCEEDED(spCollection.As(&spUntypedCollection)))
    {
        return spUntypedCollection->UntypedGetAt(index, ppInstance);
    }
    else if (SUCCEEDED(spCollection.As(&spTypedCollection)))
    {
        return spTypedCollection->GetAt(index, ppInstance);
    }
    else if (SUCCEEDED(spCollection.As(&spCommandBarCollection)))
    {
        return VectorGetAt(spCommandBarCollection.Get(), index, ppInstance);
    }
    else if (SUCCEEDED(spCollection.As(&spDOCollection)))
    {
        return VectorGetAt<xaml::DependencyObject, xaml::IDependencyObject>(spDOCollection.Get(), index, ppInstance);
    }

    return E_UNKNOWNTYPE;
}

bool DiagnosticsInterop::TryGetIndexOf(
    _In_ IInspectable* pCollection,
    _In_ IInspectable* item,
    size_t& index)
{
    bool found = false;
    if (item)
    {
        ctl::ComPtr<IInspectable> collection(pCollection);
        unsigned int foundIndex = 0;
        if (auto vectorInspectable = collection.AsOrNull<wfc::IVector<IInspectable*>>())
        {
            found = VectorTryGetIndexOf(vectorInspectable.Get(), item, &foundIndex);
        }
        else if (auto vectorCommandBarElement = collection.AsOrNull<wfc::IVector<xaml::Controls::ICommandBarElement*>>())
        {
            found = VectorTryGetIndexOf(vectorCommandBarElement.Get(), item, &foundIndex);
        }
        else if (auto vectorDependencyObject = collection.AsOrNull<wfc::IVector<xaml::DependencyObject*>>())
        {
            found = VectorTryGetIndexOf<xaml::DependencyObject, xaml::IDependencyObject>(vectorDependencyObject.Get(), item, &foundIndex);
        }
        else if (auto vectorUIElement = collection.AsOrNull<wfc::IVector<xaml::UIElement*>>())
        {
            found = VectorTryGetIndexOf<xaml::UIElement, xaml::IUIElement>(vectorUIElement.Get(), item, &foundIndex);
        }
        else if (auto untyped = collection.AsOrNull<IUntypedVector>())
        {
            found = UntypedTryGetIndexOf(untyped.Get(), item, &foundIndex);
        }

        if (found)
        {
            index = static_cast<size_t>(foundIndex);
        }
    }
    return found;
}

HRESULT
DiagnosticsInterop::InsertAt(
    _In_ IInspectable* pCollection,
    _In_ IInspectable* pElement,
    _In_ unsigned int index)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<xaml::ISetterBaseCollection> spSetterBaseCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    ctl::ComPtr<IInspectable> spElementInspectable(pElement);
    ctl::ComPtr<xaml::ISetter> spSetter;
    ctl::ComPtr<wfc::IVector<xaml_media::Animation::Timeline*>> spAnimationCollection;
    IFCPTR_RETURN(pCollection);
    IFCPTR_RETURN(pElement);

    //Handle SetterBaseCollections specially - if it's owned by a Style, we need to
    //walk the visual tree and update any objects using the Style or Styles based on it
    if (SUCCEEDED(spCollection.As(&spSetterBaseCollection)) && SUCCEEDED(spElementInspectable.As(&spSetter)))
    {
        HRESULT insertResult = S_OK;
        UpdateSetterCollectionWithSingleChange(spSetterBaseCollection.Get(), spSetter.Get(),
            [&index, &spSetter, &insertResult](wfc::IVector<xaml::SetterBase*>* pVector, CStyle* parentStyle)
        {
            insertResult = VectorInsertAt<xaml::SetterBase, xaml::ISetterBase>(pVector, index, spSetter.Get());

            if (parentStyle != nullptr)
            {
                auto coreSetter = do_pointer_cast<CSetter>(static_cast<Setter*>(spSetter.Get())->GetHandle());
                coreSetter->SubscribeToValueChangedNotification(parentStyle);
            }
        });
        IFC_RETURN(insertResult);

        VisualStateSetterHelper::PerformSetterOperationIfStateActive(
            do_pointer_cast<CVisualState>(static_cast<SetterBaseCollection*>(spSetterBaseCollection.Get())->GetHandle()->GetParentInternal(false)),
            do_pointer_cast<CSetter>(static_cast<Setter*>(spSetter.Get())->GetHandle()),
            VisualStateSetterHelper::SetterOperation::Set);
    }
    else if (SUCCEEDED(spCollection.As(&spAnimationCollection)))
    {
        auto coreCollection = static_cast<TimelineCollection*>(spAnimationCollection.Get())->GetHandle();
        auto storyboard = do_pointer_cast<CStoryboard>(coreCollection->GetParentInternal(false));
        bool isActive = false;
        if (auto visualState = TryFindVisualState(storyboard))
        {
            isActive = CVisualStateManager2::IsActiveVisualState(visualState);
        }
        auto modifySB = [spAnimationCollection, index, pElement]() {
            return VectorInsertAt<xaml_media::Animation::Timeline, xaml_media::Animation::ITimeline>(spAnimationCollection.Get(), index, pElement);
        };
        IFC_RETURN(StoryboardHelpers::ModifyStoryboard(storyboard, isActive, modifySB));
    }
    else if (SUCCEEDED(spCollection.As(&spUntypedCollection)))
    {
        IFC_RETURN(spUntypedCollection->UntypedInsertAt(index, pElement));
        // VectorInsertAt handles the namescope registration, but we can't use that for Untyped vectors so do it manually
        MICROSOFT_TELEMETRY_ASSERT_HR(EnsureElementInCorrectNamescope(spCollection, spElementInspectable));
    }
    else if (SUCCEEDED(spCollection.As(&spTypedCollection)))
    {
        IFC_RETURN(VectorInsertAt(spTypedCollection.Get(), index, pElement));
    }
    else if (SUCCEEDED(spCollection.As(&spCommandBarCollection)))
    {
        IFC_RETURN(VectorInsertAt(spCommandBarCollection.Get(), index, pElement));
    }
    else if (SUCCEEDED(spCollection.As(&spDOCollection)))
    {
        HRESULT hr = VectorInsertAt<xaml::DependencyObject, xaml::IDependencyObject>(spDOCollection.Get(), index, pElement);
        IFC_RETURN(hr);
    }
    else
    {
        return E_UNKNOWNTYPE;
    }

    ctl::ComPtr<wfc::IVector<xaml::StateTriggerBase*>> stateTriggerCollection;
    ctl::ComPtr<StateTriggerBase> triggerBase;
    if (SUCCEEDED(spCollection.As(&stateTriggerCollection)) && SUCCEEDED(pElement->QueryInterface(IID_PPV_ARGS(&triggerBase))))
    {
        if (auto visualState = TryFindVisualState(triggerBase->GetHandle()))
        {
            // If part of a visual state, then ensure that state triggers are initialized
            if (auto groupCollection = CVisualStateManager2::GetGroupCollectionFromVisualState(visualState))
            {
                const bool forceUpdate = true;
                MICROSOFT_TELEMETRY_ASSERT_HR(CVisualStateManager2::InitializeStateTriggers(groupCollection->GetOwningControl().get(), forceUpdate));
            }
        }
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::RemoveAt(
    _In_ IInspectable* pCollection,
    _In_ unsigned int index)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<xaml::ISetterBaseCollection> spSetterBaseCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    ctl::ComPtr<wfc::IVector<xaml_media::Animation::Timeline*>> spAnimationCollection;
    IFCPTR_RETURN(pCollection);

    //Handle SetterBaseCollections specially - if it's owned by a Style, we need to
    //walk the visual tree and update any objects using the Style or Styles based on it
    if (SUCCEEDED(spCollection.As(&spSetterBaseCollection)))
    {
        ctl::ComPtr<xaml::ISetterBase> spSetterBase;
        ctl::ComPtr<xaml::ISetter> spSetter;
        ctl::ComPtr<wfc::IVector<xaml::SetterBase*>> spSetterVector;
        IFC_RETURN(spSetterBaseCollection.As(&spSetterVector));
        IFC_RETURN(spSetterVector->GetAt(index, &spSetterBase));
        IFC_RETURN(spSetterBase.As(&spSetter));

        // Try undoing the setter. We do this before removing the setter so that the animated value is cleared.
        VisualStateSetterHelper::PerformSetterOperationIfStateActive(
            do_pointer_cast<CVisualState>(static_cast<SetterBaseCollection*>(spSetterBaseCollection.Get())->GetHandle()->GetParentInternal(false)),
            do_pointer_cast<CSetter>(static_cast<Setter*>(spSetter.Get())->GetHandle()),
            VisualStateSetterHelper::SetterOperation::Unset);

        HRESULT removeResult = S_OK;
        // Remove the setter
        UpdateSetterCollectionWithSingleChange(spSetterBaseCollection.Get(), spSetter.Get(),
            [&index, &spSetter, &removeResult](wfc::IVector<xaml::SetterBase*>* pVector, CStyle* parentStyle)
        {
            removeResult = pVector->RemoveAt(index);

            if (parentStyle != nullptr)
            {
                auto coreSetter = do_pointer_cast<CSetter>(static_cast<Setter*>(spSetter.Get())->GetHandle());
                coreSetter->UnsubscribeFromValueChangedNotification(parentStyle);
            }
        });
        IFC_RETURN(removeResult);
    }
    else if (SUCCEEDED(spCollection.As(&spAnimationCollection)))
    {
        auto coreCollection = static_cast<TimelineCollection*>(spAnimationCollection.Get())->GetHandle();
        auto storyboard = do_pointer_cast<CStoryboard>(coreCollection->GetParentInternal(false));
        bool isActive = false;
        if (auto visualState = TryFindVisualState(storyboard))
        {
            isActive = CVisualStateManager2::IsActiveVisualState(visualState);
        }
        auto modifySB = [spAnimationCollection, index](){
            return spAnimationCollection->RemoveAt(index);
        };
        IFC_RETURN(StoryboardHelpers::ModifyStoryboard(storyboard, isActive, modifySB));
    }
    else if (SUCCEEDED(spCollection.As(&spUntypedCollection)))
    {
        IFC_RETURN(spUntypedCollection->UntypedRemoveAt(index));
    }
    else if (SUCCEEDED(spCollection.As(&spTypedCollection)))
    {
        IFC_RETURN(spTypedCollection->RemoveAt(index));
    }
    else if (SUCCEEDED(spCollection.As(&spCommandBarCollection)))
    {
        IFC_RETURN(spCommandBarCollection->RemoveAt(index));
    }
    else if (SUCCEEDED(spCollection.As(&spDOCollection)))
    {
        IFC_RETURN(spDOCollection->RemoveAt(index));
    }
    else
    {
        return E_UNKNOWNTYPE;
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::Clear(
    _In_ IInspectable* pCollection)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<xaml::ISetterBaseCollection> spSetterBaseCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    IFCPTR_RETURN(pCollection);

    //Handle SetterBaseCollections specially - if it's owned by a Style, we need to
    //walk the visual tree and update any objects using the Style or Styles based on it
    if (SUCCEEDED(spCollection.As(&spSetterBaseCollection)))
    {
        HRESULT clearResult = S_OK;
        UpdateSetterCollectionWithMultipleChanges(spSetterBaseCollection.Get(),
            [&clearResult](wfc::IVector<xaml::SetterBase*>* pVector, CStyle* parentStyle)
        {
            clearResult = pVector->Clear();

            if (parentStyle != nullptr)
            {
                for (auto& setterBase : pVector)
                {
                    auto coreSetter = checked_cast<CSetter>(static_cast<Setter*>(setterBase.Get())->GetHandle());
                    coreSetter->UnsubscribeFromValueChangedNotification(parentStyle);
                }
            }
        });

        return clearResult;
    }

    if (SUCCEEDED(spCollection.As(&spUntypedCollection)))
    {
        return spUntypedCollection->UntypedClear();
    }
    else if (SUCCEEDED(spCollection.As(&spTypedCollection)))
    {
        return spTypedCollection->Clear();
    }
    else if (SUCCEEDED(spCollection.As(&spCommandBarCollection)))
    {
        return spCommandBarCollection->Clear();
    }
    else if (SUCCEEDED(spCollection.As(&spDOCollection)))
    {
        return spDOCollection->Clear();
    }

    return E_UNKNOWNTYPE;
}

HRESULT
DiagnosticsInterop::GetContentPropertyName(
    _In_ xaml::IDependencyObject* pDO,
    _Out_ LPCWSTR* pName)
{
    ctl::ComPtr<xaml::IDependencyObject> spDOAsI(pDO);
    ctl::ComPtr<DependencyObject> spDO;

    IFC_RETURN(spDOAsI.As(&spDO));

    *pName = spDO->GetHandle()->GetContentProperty()->GetName().GetBuffer();

    return S_OK;
}


HRESULT
DiagnosticsInterop::SetPropertyValue(
    _In_ IInspectable* pReference,
    _In_ uint32_t propertyIndex,
    _In_opt_ IInspectable* pValue)
{
    return SetPropertyValue(pReference, propertyIndex, pValue, true);
}

HRESULT
DiagnosticsInterop::SetPropertyValue(
    _In_ IInspectable* pReference,
    _In_ uint32_t propertyIndex,
    _In_opt_ IInspectable* pValue,
    _In_ bool unregisterResource)
{
    ctl::ComPtr<IInspectable> spReference(pReference);
    ctl::ComPtr<xaml::IDependencyObject> spDO;
    ctl::ComPtr<IInspectable> spValue(pValue);

    auto setPropertyIndex = static_cast<KnownPropertyIndex>(propertyIndex);

    if (SUCCEEDED(spReference.As(&spDO)))
    {
        bool isValueSet = false;
        auto spDOAsSetter = spDO.AsOrNull<Setter>();
        if (spDOAsSetter)
        {
            IFC_RETURN(SetPropertyOnSetter(spDOAsSetter, setPropertyIndex, pValue, &isValueSet));
        }

        auto valueBinding = spValue.AsOrNull<xaml_data::IBindingBase>();
        if (valueBinding && !isValueSet)
        {
            IFC_RETURN(SetBinding(spDO.Get(), setPropertyIndex, valueBinding.Get(), &isValueSet));
        }

        auto spDOAsStyle = spDO.AsOrNull<xaml::IStyle>();
        if (spDOAsStyle && !isValueSet)
        {
            IFC_RETURN(SetPropertyOnStyle(spDOAsStyle, setPropertyIndex, pValue, &isValueSet));
        }

        auto spTimeline = spDO.AsOrNull<xaml_animation::ITimeline>();
        if (spTimeline && !isValueSet)
        {
            IFC_RETURN(SetPropertyOnTimeline(spTimeline, setPropertyIndex, pValue, &isValueSet));
        }

        auto keyFrame = spDO.AsOrNull<xaml_animation::IObjectKeyFrame>();
        if (keyFrame && !isValueSet)
        {
            IFC_RETURN(SetPropertyOnKeyFrame(keyFrame, setPropertyIndex, pValue, &isValueSet));
        }

        if (!isValueSet)
        {
            IFC_RETURN(SetValue(spDO, setPropertyIndex, pValue));
        }

        auto pObject = (static_cast<DependencyObject*>(spDO.Get()))->GetHandle();
        if (unregisterResource)
        {
            auto resourceGraph = GetResourceGraph();
            resourceGraph->UnregisterResourceDependency(pObject, setPropertyIndex);
        }

        if (pValue)
        {
            IFC_RETURN(EnsureElementInCorrectNamescope(spDO, pValue));
        }

        // If setting the storyboard on a visual state, try adding it to the active state
        if (setPropertyIndex == KnownPropertyIndex::VisualState_Storyboard)
        {
            auto visualState = do_pointer_cast<CVisualState>(pObject);
            xref_ptr<CStoryboard> storyboard;
            IFC_RETURN(visualState->GetStoryboard(storyboard.ReleaseAndGetAddressOf()));
            IFC_RETURN(CVisualStateManager2::TryAddStoryboardToState(visualState, storyboard));
        }

        if (auto valueStyle = spValue.AsOrNull<Style>())
        {
            const auto resourceGraph = GetResourceGraph();
            const bool isImplicit = false; // Can't be implicit since it's not in a resource dictionary
            resourceGraph->AddStyleContext(checked_cast<CStyle>(valueStyle->GetHandle()), spDO.Cast<DirectUI::DependencyObject>()->GetHandle(), isImplicit);
        }
    }
    else
    {
        IFC_RETURN(SetValue(spReference, setPropertyIndex, pValue));
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::SetBinding(
    _In_ const ctl::ComPtr<xaml::IDependencyObject>& spDO,
    _In_ KnownPropertyIndex setPropertyIndex,
    _In_ xaml_data::IBindingBase* pValue,
    _Out_ bool* wasValueSet)
{
    *wasValueSet = false;

    ctl::ComPtr<xaml::IDependencyProperty> spDP;
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(setPropertyIndex, &spDP));

    IFC_RETURN(BindingOperations::SetBindingImpl(spDO.Get(), spDP.Get(), pValue));

    auto coreDO = spDO.Cast<DirectUI::DependencyObject>()->GetHandle();
    IFC_RETURN(OnValueChanged(coreDO, setPropertyIndex));

    const auto graph = GetResourceGraph();
    graph->CacheMarkupExtensionToTarget(checked_cast<CMarkupExtensionBase>(static_cast<DirectUI::Binding*>(pValue)->GetHandle()), setPropertyIndex, coreDO);

    *wasValueSet = true;

    return S_OK;
}

HRESULT
DiagnosticsInterop::SetPropertyOnSetter(
    _In_ const ctl::ComPtr<Setter>& setter,
    _In_ KnownPropertyIndex setPropertyIndex,
    _In_opt_ IInspectable* pValue,
    _Out_ bool* wasValueSet)
{
    *wasValueSet = false;
    auto coreSetter = checked_cast<CSetter>(setter.Get()->GetHandle());

    // Temporarily unseal the setter while we modify it
    SetterUnsealer sealOnExit(coreSetter);

    auto valueBinding = ctl::ComPtr<IInspectable>(pValue).AsOrNull<xaml_data::IBindingBase>();
    if (valueBinding)
    {
        ctl::ComPtr<DependencyObject> setterDO;
        IFC_RETURN(setter.As(&setterDO));
        IFC_RETURN(SetBinding(setterDO.Get(), setPropertyIndex, valueBinding.Get(), wasValueSet));
    }
    else if (setPropertyIndex == KnownPropertyIndex::Setter_Value && pValue)
    {
        IFC_RETURN(setter->put_Value(pValue));
        *wasValueSet = true;
    }
    else if (setPropertyIndex == KnownPropertyIndex::Setter_Property && pValue)
    {
        wrl::ComPtr<xaml::IDependencyProperty> spValueProperty;
        IFC_RETURN(pValue->QueryInterface<xaml::IDependencyProperty>(&spValueProperty));
        IFC_RETURN(setter->put_Property(spValueProperty.Get()));
        *wasValueSet = true;
    }
    else if (setPropertyIndex == KnownPropertyIndex::Setter_Target && pValue)
    {
        wrl::ComPtr<xaml::ITargetPropertyPath> spValueTarget;
        IFC_RETURN(pValue->QueryInterface<xaml::ITargetPropertyPath>(&spValueTarget));
        IFC_RETURN(setter->put_Target(spValueTarget.Get()));
        *wasValueSet = true;
    }

    IFC_RETURN(OnSetterChanged(coreSetter));

    return S_OK;
}

HRESULT
DiagnosticsInterop::SetPropertyOnStyle(
    _In_ const ctl::ComPtr<xaml::IStyle>& style,
    _In_ KnownPropertyIndex setPropertyIndex,
    _In_opt_ IInspectable* pValue,
    _Out_ bool* wasValueSet)
{
    *wasValueSet = false;
    // Setting a style.  Temporarily unseal the Style, its Setters collection, and Setters themselves
    // as necessary to modify it, then walk the visual tree and ensure all properties which are modified
    // by the Style are updated.  Our convention with Visual Studio is
    // for modifying Setters in a Style through the LPE, they call SetPropertyValue with pReference = the Style,
    // propertyIndex = the Setter's Property property index, and pValue = the new value for the Setter's Value.
    // We then search the Style for the matching Setter (or create it if necessary) and apply the change.
    // Visual Studio will not use this for modifying Setter Styles in Edit&Continue - whenever a Setter
    // is modified, they will clone a new Setter with the necessary changes, and go through our collection APIs
    // to remove the old Setter and put in the new one.

    auto pCStyle = checked_cast<CStyle>(static_cast<Style*>(style.Get())->GetHandle());
    std::vector<CStyle*> usedStyles;

    auto pProperty = MetadataAPI::GetPropertyByIndex(setPropertyIndex);
    auto resourceGraph = GetResourceGraph();
    // If setting a style and the property isn't directly assignable to a style, assume it is for a setter
    if (!MetadataAPI::IsAssignableFrom(pProperty->GetTargetType()->GetIndex(), KnownTypeIndex::Style))
    {
        bool rebuildSetters = false;
        IFC_RETURN(UpdateSetterForStyle(style, setPropertyIndex, pValue, &rebuildSetters));

        if (rebuildSetters)
        {
            IFC_RETURN(resourceGraph->RebuildMergedSetters(pCStyle, usedStyles));
        }
        else
        {
            resourceGraph->GetAllDependentStyles(pCStyle, usedStyles);
        }

        for (const auto & item : GetAllDependentItems(usedStyles))
        {
            CDependencyObject *pCDO = static_cast<FrameworkElement*>(item.first.Get())->GetHandle();
            IFC_RETURN(pCDO->InvalidateProperty(pProperty, ::BaseValueSourceStyle));
        }
        *wasValueSet = true;
    }
    else if (setPropertyIndex == KnownPropertyIndex::Style_BasedOn)
    {
        wrl::ComPtr<xaml::IStyle> spValueStyle;
        if (pValue)
        {
            IFC_RETURN(pValue->QueryInterface<xaml::IStyle>(&spValueStyle));
        }

        StyleUnsealer reseal(pCStyle);

        // Unlink the old BasedOn Style
        resourceGraph->UnregisterBasedOnStyleDependency(pCStyle);
        auto oldBasedOnStyle = xref_ptr<CStyle>(pCStyle->GetBasedOnStyleNoRef());

        // Link the new BasedOn Style
        IFC_RETURN(style->put_BasedOn(spValueStyle.Get()));
        resourceGraph->RegisterBasedOnStyleDependency(pCStyle);
        auto newBasedOnStyle = xref_ptr<CStyle>(pCStyle->GetBasedOnStyleNoRef());

        std::vector<CStyle*> usedStylesIgnore;
        IFC_RETURN(resourceGraph->RebuildMergedSetters(pCStyle, usedStylesIgnore));

        // Notify all listeners of the Style of the property invalidations due to the change in
        // the BasedOn Style
        IFC_RETURN(UpdateBasedOnStyleListeners(pCStyle, oldBasedOnStyle, newBasedOnStyle));
        *wasValueSet = true;
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::SetPropertyOnTimeline(
    _In_ const ctl::ComPtr<xaml_animation::ITimeline>& timeline,
    _In_ KnownPropertyIndex setPropertyIndex,
    _In_opt_ IInspectable* pValue,
    _Out_ bool* wasValueSet)
{
    *wasValueSet = true;
    auto coreTimeline = checked_cast<CTimeline>(static_cast<Timeline*>(timeline.Get())->GetHandle());

    // The timeline being modified is either the storyboard itself, or an animation inside the storyboard
    // so make sure we disable the correct one.
    auto storyboard = do_pointer_cast<CStoryboard>(coreTimeline->GetRootTimingParent());

    bool isActive = false;
    if (storyboard)
    {
        if (auto visualState = TryFindVisualState(storyboard))
        {
            isActive = CVisualStateManager2::IsActiveVisualState(visualState);
        }
    }

    auto modifySB = [timeline, setPropertyIndex, pValue](){
        return SetValue(timeline, setPropertyIndex, pValue);
    };
    IFC_RETURN(StoryboardHelpers::ModifyStoryboard(storyboard, isActive, modifySB));

    return S_OK;
}

_Check_return_ HRESULT DiagnosticsInterop::SetPropertyOnKeyFrame(
    _In_ const ctl::ComPtr<xaml_animation::IObjectKeyFrame>& keyFrame,
    _In_ KnownPropertyIndex propertyIndex,
    _In_opt_ IInspectable* pValue,
    _Out_ bool* wasValueSet)
{
    *wasValueSet = false;
    auto coreKeyFrame = checked_cast<CObjectKeyFrame>(static_cast<ObjectKeyFrame*>(keyFrame.Get())->GetHandle());

    CStoryboard* storyboard = nullptr;
    auto parent = coreKeyFrame->GetParentInternal(false /*publicParentOnly*/);
    if (parent)
    {
        storyboard = do_pointer_cast<CStoryboard>(parent->GetParentInternal(false /*publicParentOnly*/));
    }

    bool isActive = false;
    if (storyboard)
    {
        if (auto visualState = TryFindVisualState(storyboard))
        {
            isActive = CVisualStateManager2::IsActiveVisualState(visualState);
        }
    }

    auto modifySB = [keyFrame, propertyIndex, pValue](){
        return SetValue(keyFrame, propertyIndex, pValue);
    };

    IFC_RETURN(StoryboardHelpers::ModifyStoryboard(storyboard, isActive, modifySB));
    *wasValueSet = true;
    return S_OK;
}

xaml_hosting::IXamlIslandRoot* TryGetXamlIsland(_In_ IInspectable* object)
{
    ctl::ComPtr<IInspectable> spObject(object);

    if (auto xamlSource = spObject.AsOrNull<DirectUI::DesktopWindowXamlSource>())
    {
        return xamlSource->GetXamlIslandRootNoRef();
    }
    else if (auto xamlIslandSource = spObject.AsOrNull<DirectUI::XamlIsland>())
    {
        return xamlIslandSource->GetXamlIslandRootNoRef();
    }

    return nullptr;
}

HRESULT
DiagnosticsInterop::GetChildren(
    _In_ IInspectable* object,
    _Outptr_ IInspectable** children)
{
    ctl::ComPtr<IInspectable> parent(object);
    if (auto parentWindow = parent.AsOrNull<xaml::IWindow>())
    {
        std::vector<wrl::ComPtr<IInspectable>> roots;
        IFC_RETURN(GetVisualTreeRoots(roots));
        auto collection = CDependencyObjectCollection::MakeDiagnosticsRootCollection(GetCore());
        for (const auto& root : roots)
        {
            collection->push_back(ConvertToCore(root.Get()));
        }

        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(collection.get(), children));
        return S_OK;
    }
    else if (auto island = TryGetXamlIsland(parent.Get()))
    {
        auto coreIsland = checked_cast<CXamlIslandRoot>(static_cast<DirectUI::XamlIslandRoot*>(island)->GetHandle());
        auto collection = CDependencyObjectCollection::MakeDiagnosticsRootCollection(GetCore());

        if (auto mainVisualTree = coreIsland->GetVisualTreeNoRef())
        {
            collection->push_back(xref_ptr<CDependencyObject>(mainVisualTree->GetPopupRoot()));

            if (auto rsv = mainVisualTree->GetRootScrollViewer())
            {
                collection->push_back(xref_ptr<CDependencyObject>(rsv));
            }
        }
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(collection.get(), children));
    }
    else if (auto parentDO = parent.AsOrNull<xaml::IDependencyObject>())
    {
        wrl::ComPtr<xaml::IDependencyObject> childrenDO;
        IFC_RETURN(VisualTreeHelper::GetChildrenStatic(parentDO.Get(), &childrenDO));
        *children = childrenDO.Detach();
    }

    return S_OK;
}

bool DiagnosticsInterop::TryGetOwnerFromIsland(
    _In_ xaml_hosting::IXamlIslandRoot* island,
    _COM_Outptr_result_maybenull_ IInspectable** islandOwner)
{
    return static_cast<DirectUI::XamlIslandRoot*>(island)->TryGetOwner(islandOwner);
}

/* Notes for TryGetVisualTreeParent()
When we don't have a parent, it means a root is entering the tree, and we need to be careful about the roots that we hand back to VS.
    1. We don't want to expose a lot of the internal roots that 3rd party developers don't care about (especially the VisualDiagnosticsRoot).
    2. When in multi-window mode, the roots that we hand back are parented to the Window they are associated with. Otherwise, we hand back each root as a top-level root.
    3. In XamlIslandRoots, they are associated with the DesktopWindowXamlSource, or there are ways that internal customers can create XamlIslandRoots today,
       and so we'll fallback to the XamlIslandRoot if that's the case.

LVT structure is governed by the following APIs:
    - DiagnosticsInterop::GetRoots
    - DiagnosticsInterop::TryGetVisualTreeParent
    - DiagnosticsInterop::GetChildren
    There are places where some logic overlaps (see other use of TryGetOwnerFromIsland). And if any change is made, it should be followed up with
    those other methods to make sure they are consistent. If any special filtering logic needs to be done, it should be done in one of these methods (as deemed appropriate)
*/
bool DiagnosticsInterop::TryGetVisualTreeParent(
    _In_ IInspectable* child,
    _COM_Outptr_result_maybenull_ IInspectable** parent)
{
    *parent = nullptr;

    ctl::ComPtr<IInspectable> childItem(child);

    if (auto childElement = childItem.AsOrNull<xaml::IUIElement>())
    {
        ctl::ComPtr<xaml::IDependencyObject> childDO;
        IFCFAILFAST(childElement.As(&childDO));

        auto coreChild = childElement.Cast<UIElement>()->GetHandle();

        auto coreServices = DXamlCore::GetCurrent()->GetHandle();
        if (coreServices->GetInitializationType() == InitializationType::IslandsOnly)
        {
            CXamlIslandRoot* root = do_pointer_cast<CXamlIslandRoot>(coreChild);
            if (root != nullptr)
            {
                return false; // Don't signal add for islandRoot itself since it is not public
            }
            else
            {
                auto visualTree = VisualTree::GetForElementNoRef(coreChild);
                if (visualTree)
                {
                    root = do_pointer_cast<CXamlIslandRoot>(visualTree->GetRootElementNoRef());
                    if (root == nullptr)
                    {
                        return false; // Don't signal if the element is not under island tree
                    }
                }
            }
        }

        ctl::ComPtr<xaml::IDependencyObject> parentObject;
        MICROSOFT_TELEMETRY_ASSERT_HR(VisualTreeHelper::GetParentStatic(childDO.Get(), &parentObject));
        if (parentObject)
        {
            *parent = parentObject.Detach();
            return true;
        }

        // No direct parent, since VisualTreeHelper::GetParent won't return the XamlIslandRoot, see if this element is supposed to be under a xaml island.
        // If so, we want to use the DesktopWindowXamlSource as the parent. Otherwise, we'll just return the islands as we always have.

        if (auto xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(coreChild))
        {
            if (do_pointer_cast<CPopupRoot>(coreChild) ||
                do_pointer_cast<CRootScrollViewer>(coreChild))
            {
                wrl::ComPtr<xaml_hosting::IXamlIslandRoot> islandPeer;
                if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer<xaml_hosting::IXamlIslandRoot>(xamlIslandRoot, islandPeer.ReleaseAndGetAddressOf())) && islandPeer)
                {
                    ctl::ComPtr<IInspectable> islandOwner;
                    *parent = TryGetOwnerFromIsland(islandPeer.Get(), &islandOwner) ? islandOwner.Detach() : islandPeer.Detach();
                    return true;
                }
            }
        }
        else
        {
            // No parent and not under a Xaml Island, see if this element is a root object that should be parented under the Window.
            ctl::ComPtr<xaml::IUIElement> visualRoot;

            if (FAILED(GetVisualRoot(&visualRoot)))
            {
                return false;
            }

            if (do_pointer_cast<CPopupRoot>(coreChild) ||
                do_pointer_cast<CFullWindowMediaRoot>(coreChild) ||
                do_pointer_cast<CRenderTargetBitmapRoot>(coreChild) ||
                visualRoot == childElement)
            {
                // this object is an orphan, cannot be guaranteed to get a valid window using GetAssociatedWindowNoRef
                // https://microsoft.visualstudio.com/OS/_workitems/edit/37064856
                parentObject = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
                *parent = parentObject.Detach();
                return true;
            }
        }
    }

    return false;
}


HRESULT
DiagnosticsInterop::HitTest(
    _In_ IInspectable* rootElement,
    _In_ RECT rect,
    _Inout_ DebugTool::ICollection<xaml::IDependencyObject*>* pElements)
{
    wrl::ComPtr<wfc::IIterable<xaml::UIElement*>> spElements;
    wrl::ComPtr<wfc::IIterator<xaml::UIElement*>> spElementsIterator;
    wrl::ComPtr<IInspectable> spRootInspectable(rootElement);
    wrl::ComPtr<xaml::IUIElement> spRootUIElement;
    wf::Rect wfRect;
    boolean hasCurrent = false;

    wfRect.X = static_cast<float>(rect.left);
    wfRect.Y = static_cast<float>(rect.top);
    wfRect.Width = static_cast<float>(rect.right - rect.left);
    wfRect.Height = static_cast<float>(rect.bottom - rect.top);

    // Assume we're given a XamlRoot as the element, and hit test based on its content
    wrl::ComPtr<xaml::IXamlRoot> spPeerRoot;
    if (spRootInspectable.Get() != nullptr)
    {
        if (SUCCEEDED(spRootInspectable.As(&spPeerRoot)))
        {
             IFC_RETURN(spPeerRoot->get_Content(&spRootUIElement));
        }

        // If we weren't given a XamlRoot, our only other candidate would be a UIElement, so try hit-testing with it directly
        if (spRootUIElement.Get() == nullptr)
        {
            IFC_RETURN(spRootInspectable.As(&spRootUIElement));
        }
    }

    const BOOLEAN c_canHitDisabledElements = TRUE;
    const BOOLEAN c_canHitInvisibleElements = TRUE;
    IFC_RETURN(VisualTreeHelper::FindElementsInHostCoordinatesRectStatic(
        wfRect,
        spRootUIElement.Get(),
        c_canHitDisabledElements,
        c_canHitInvisibleElements,
        &spElements));

    IFC_RETURN(spElements->First(&spElementsIterator));
    IFC_RETURN(spElementsIterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        wrl::ComPtr<xaml::IUIElement> spElement;
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;

        IFC_RETURN(spElementsIterator->get_Current(&spElement));
        IFC_RETURN(spElement.As(&spElementAsDO));

        // We should only be returning elements in the live tree
        if (static_cast<UIElement*>(spElement.Get())->IsInLiveTree())
        {
            IFC_RETURN(pElements->Append(spElementAsDO.Get()));
        }

        IFC_RETURN(spElementsIterator->MoveNext(&hasCurrent));
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::GetApplication(
    _Outptr_ xaml::IApplication** ppApplication)
{
    return GetApplicationStatic(ppApplication);
}

HRESULT
DiagnosticsInterop::GetApplicationStatic(
    _Outptr_ xaml::IApplication** ppApplication)
{
    FrameworkApplication* pInstance = FrameworkApplication::GetCurrentNoRef();
    IFCEXPECT_RETURN(pInstance);

    IFC_RETURN(ctl::do_query_interface(*ppApplication, pInstance));

    return S_OK;
}

HRESULT
DiagnosticsInterop::GetDispatcherQueue(
    _Outptr_ msy::IDispatcherQueue** ppDispatcher)
{
    return DXamlCore::GetCurrent()->GetDispatcherQueue(ppDispatcher);
}

HRESULT
DiagnosticsInterop::GetPopupRoot(
    _Outptr_ xaml::IDependencyObject** ppPopupRoot)
{
    CCoreServices* pCS = GetCore();
    ctl::ComPtr<DependencyObject> spDO;

    *ppPopupRoot = nullptr;

    CPopupRoot* pPopupRoot = pCS->GetMainPopupRoot();
    if (!pPopupRoot)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pPopupRoot, &spDO));
    if (spDO)
    {
        *ppPopupRoot = static_cast<xaml::IDependencyObject*>(spDO.Detach());
    }

    return S_OK;
}

// When XamlDiagnostics queries for the roots, it is possible that the visual root
// has not been created. However, even when this is the case the MainPopupRoot,
// FullWindowMediaRoot, and (if in background task) the RenderTargetBitmapRoot will have been created.
// We will return S_OK on success regardless if roots are in the tree or not. This method must always
// return these roots, removing any will break earlier versions of XamlDiagnostics.
HRESULT
DiagnosticsInterop::GetRoots(std::vector<wrl::ComPtr<IInspectable>>& roots)
{
    static auto runtimeEnabledFeatureDetector = GetRuntimeEnabledFeatureDetector();
    // If v2 is enabled, this means that multiple windows are supported and we are enabled for all cores. We can grab
    // all the root objects cached on this thread
    // Only use the window if it's capable of having content.
    if (FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel() == AppPolicyWindowingModel_Universal)
    {
        roots.emplace_back(ctl::iinspectable_cast(DXamlCore::GetCurrent()->GetUwpWindowNoRef()));
    }

    GetXamlIslandRoots(roots);

    return S_OK;
}


// Returns the proper XamlRoot object for the input element if possible.
HRESULT DiagnosticsInterop::GetXamlRoot(
    _In_ IInspectable* element,
    _Outptr_ xaml::IXamlRoot** xamlRoot)
{
    ctl::ComPtr<IInspectable> spObject(element);

    if (auto xamlSource = spObject.AsOrNull<DirectUI::DesktopWindowXamlSource>())
    {
        ctl::ComPtr<IInspectable> spXamlRootInspectable;
        ctl::ComPtr<xaml::IXamlRoot> spXamlRoot;
        auto coreIsland = checked_cast<CXamlIslandRoot>(static_cast<DirectUI::XamlIslandRoot*>(xamlSource->GetXamlIslandRootNoRef())->GetHandle());

        spXamlRootInspectable = coreIsland->GetVisualTreeNoRef()->GetOrCreateXamlRootNoRef();
        IFC_RETURN(spXamlRootInspectable.As(&spXamlRoot));

        // GetOrCreateXamlRootNoRef didn't add a reference, so we need to Detach here
        *xamlRoot = spXamlRoot.Detach();
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT DiagnosticsInterop::GetXamlIslandRoots(std::vector<wrl::ComPtr<IInspectable>>& roots)
{
    CDOCollection* rootCollection = nullptr;
    if (auto visualTree = GetCore()->GetMainVisualTree())
    {
        if (auto xamlIslandRoots = visualTree->GetXamlIslandRootCollection())
        {
            rootCollection = xamlIslandRoots->GetChildren();
        }
    }

    if (rootCollection)
    {
        for (CDependencyObject* child : *rootCollection)
        {
            wrl::ComPtr<xaml_hosting::IXamlIslandRoot> xamlIslandRoot;
            if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(child, xamlIslandRoot.ReleaseAndGetAddressOf())) && xamlIslandRoot)
            {
                wrl::ComPtr<IInspectable> islandOwner;
                if (TryGetOwnerFromIsland(xamlIslandRoot.Get(), &islandOwner))
                {
                    roots.push_back(islandOwner);
                }
                else
                {
                    roots.push_back(xamlIslandRoot);
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DiagnosticsInterop::GetVisualTreeRoots(std::vector<wrl::ComPtr<IInspectable>>& roots)
{
    CCoreServices* pCS = GetCore();
    CPopupRoot* pPopupRoot = pCS->GetMainPopupRoot();

    if (pPopupRoot)
    {
        wrl::ComPtr<xaml::IUIElement> root;
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pPopupRoot, root.ReleaseAndGetAddressOf()));

        if (root)
        {
            roots.emplace_back(std::move(root));
        }
    }

    CFullWindowMediaRoot* pFullWindowMediaRoot = pCS->GetMainFullWindowMediaRoot();
    if (pFullWindowMediaRoot)
    {
        wrl::ComPtr<xaml::IUIElement> root;
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pFullWindowMediaRoot, root.ReleaseAndGetAddressOf()));

        if (root)
        {
            roots.emplace_back(std::move(root));
        }
    }

    // The RenderTargetBitmap roots is used for rendering elements in a background task,
    // one use case of this is for apps to render their live tiles. So while the number
    // of cases where this is used will probably be minimal, it is worthy of keeping for
    // diagnostic info.
    CRenderTargetBitmapRoot* pRenderTargetBitmapRoot = pCS->GetMainRenderTargetBitmapRoot();
    if (pRenderTargetBitmapRoot)
    {
        wrl::ComPtr<xaml::IUIElement> root;
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pRenderTargetBitmapRoot, root.ReleaseAndGetAddressOf()));

        if (root)
        {
            roots.emplace_back(std::move(root));
        }
    }

    //  We add the visual root last so that it will appear on top of the LVT.
    wrl::ComPtr<xaml::IUIElement> spVisualRoot;
    IFC_RETURN(GetVisualRoot(&spVisualRoot));
    if (spVisualRoot)
    {
        roots.emplace_back(std::move(spVisualRoot));
    }
    return S_OK;
}

HRESULT
DiagnosticsInterop::GetVisualRoot(
    _Outptr_result_maybenull_ xaml::IUIElement** ppRoot)
{
    ctl::ComPtr<DirectUI::DependencyObject> spDO;
    ctl::ComPtr<xaml::IUIElement> spDOAsI;

    CCoreServices* pCore = GetCore();
    CDependencyObject *pVisualRoot = pCore->getRootScrollViewer();

    if (!pVisualRoot)
    {
        pVisualRoot = pCore->getVisualRoot();
        if (!pVisualRoot)
        {
            *ppRoot = nullptr;
            return S_OK;
        }
    }

    IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer(pVisualRoot, pVisualRoot->GetTypeIndex(), &spDO));

    IFC_RETURN(spDO.As(&spDOAsI));
    *ppRoot = spDOAsI.Detach();

    return S_OK;
}

HRESULT
DiagnosticsInterop::GetVisualDiagnosticRoot(
    _In_ IInspectable* rootElement,
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppRoot)
{
    ctl::ComPtr<DependencyObject> spDO;
    CGrid* pGrid = nullptr;
    if (rootElement == nullptr)
    {
        // If no root was supplied, default to using the root from the main visual tree.
        // Note this should never be called with a null value for Desktop apps,
        // as the main visual tree is never actually displayed.
        CCoreServices* pCS = GetCore();
        IFC_RETURN(pCS->GetVisualDiagnosticsRoot(&pGrid));
    }
    else
    {
        // If we are given an element, it must be a XamlRoot.
        ctl::ComPtr<IInspectable> spRoot(rootElement);
        ctl::ComPtr<xaml::IXamlRoot> spPeerRoot;
        IFC_RETURN(spRoot.As(&spPeerRoot));
        ctl::ComPtr<DirectUI::XamlRoot> xamlRoot(static_cast<DirectUI::XamlRoot*>(spPeerRoot.Get()));
        pGrid = xamlRoot->GetVisualTreeNoRef()->GetVisualDiagnosticsRoot();
    }

    if (!pGrid)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pGrid, &spDO));
    if (spDO)
    {
        *ppRoot = static_cast<xaml::IDependencyObject*>(spDO.Detach());
    }

    return S_OK;
}

// This is also under discussion...basically, this is meant to "update"
// the app after the style gets updated...if we can't update the style,
// then we don't need this.
HRESULT
DiagnosticsInterop::VisualRootUpdateLayout()
{
    wrl::ComPtr<xaml::IUIElement> spVisualRoot;

    IFC_RETURN(GetVisualRoot(&spVisualRoot));

    IFC_RETURN(spVisualRoot->UpdateLayout());

    return S_OK;
}

// Converts the given property value of the specified type to a string.
HRESULT
DiagnosticsInterop::ConvertValueToString(
    _In_ wf::IPropertyValue* pPropertyValue,
    _In_ wf::PropertyType propertyType,
    _Out_ HSTRING* phstr)
{
    if (!ValueConversionHelpers::CanConvertValueToString(propertyType))
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(ValueConversionHelpers::ConvertValueToString(pPropertyValue, propertyType, phstr));

    return S_OK;
}


//When a Style is changed, we need to walk the visual tree and reapply the changed property/properties
//to objects using the changed Styles (both the directly modified Style, and any Styles that inherit from it).
//usedStyles is a list of Styles we need to check each object for, pDP is the DP of the property index
//the Setter is changing (the property index on objects that we need to re-evaluate), and multipleSettersChanged
//indicates if there is the possibility the operation changed multiple Setters' values.  If just one Setter's value
//was changed, we can just invalidate that property on objects using the changed styles - but if multiple Setters
//changed (e.g. we changed the BasedOn property of the Style), we reapply the entire Style.

void
DiagnosticsInterop::ForEachVisualTreeElement(
    _In_ const std::function< void(xaml::IFrameworkElement*) >& func)
{
    // Now, find all controls that use the changed style(s) and reapply them.
    std::queue<wrl::ComPtr<xaml::IDependencyObject>> processingQueue;
    wrl::ComPtr<xaml::IUIElement> root;

    MICROSOFT_TELEMETRY_ASSERT_HR(GetVisualRoot(&root));

    if (root)
    {
        wrl::ComPtr<xaml::IDependencyObject> rootDO;
        IFCFAILFAST(root.As(&rootDO))
        processingQueue.push(rootDO);
    }

    while (!processingQueue.empty())
    {
        auto current = std::move(processingQueue.front());
        processingQueue.pop();

        // Enqueue the children for processing.
        wrl::ComPtr<xaml::IDependencyObject> children;
        MICROSOFT_TELEMETRY_ASSERT_HR(GetChildren(current.Get(), &children));
        if (children)
        {
            wrl::ComPtr<wfc::IVector<xaml::UIElement*>> childrenAsIVector;
            unsigned int numChildren = 0;

            MICROSOFT_TELEMETRY_ASSERT_HR(children.As(&childrenAsIVector));
            if (childrenAsIVector)
            {
                MICROSOFT_TELEMETRY_ASSERT_HR(childrenAsIVector->get_Size(&numChildren));
            }
            for (size_t k = 0; k < numChildren; k++)
            {
                wrl::ComPtr<xaml::IUIElement> child;
                wrl::ComPtr<xaml::IDependencyObject> childAsDO;

                MICROSOFT_TELEMETRY_ASSERT_HR(childrenAsIVector->GetAt(k, &child));
                if (child && SUCCEEDED(child.As(&childAsDO)))
                {
                    processingQueue.push(std::move(childAsDO));
                }
            }
        }

        // Replace style, if possible.
        wrl::ComPtr<xaml::IFrameworkElement> currentAsFE;

        if (SUCCEEDED(current.As(&currentAsFE)) && currentAsFE)
        {
            func(currentAsFE.Get());
        }
    }
}

std::vector<std::pair<wrl::ComPtr<xaml::IFrameworkElement>, wrl::ComPtr<xaml::IStyle>>> DiagnosticsInterop::GetAllDependentItems(
    _In_ const std::vector<CStyle*>& usedStyles)
{
    std::vector<std::pair<wrl::ComPtr<xaml::IFrameworkElement>, wrl::ComPtr<xaml::IStyle>>> elements;
    auto getAll = [&](xaml::IFrameworkElement* element) {
        wrl::ComPtr<xaml::IStyle> spStyleOfCurrent;
        if (SUCCEEDED(element->get_Style(&spStyleOfCurrent)) && spStyleOfCurrent.Get() != nullptr)
        {
            CStyle *pCurStyle = static_cast<CStyle*>(static_cast<Style*>(spStyleOfCurrent.Get())->GetHandle());

            //We need to check if this element used any of our changed Styles (the one we modified directly and
            //Styles based on the modified one)
            for (CStyle* pChangedStyle : usedStyles)
            {
                if (pCurStyle == pChangedStyle)
                {
                    elements.push_back(std::make_pair(element, spStyleOfCurrent));
                    break;
                }
            }
        }
    };

    ForEachVisualTreeElement(getAll);

    return elements;
}

void DiagnosticsInterop::UpdateSetterCollectionWithSingleChange(
    _In_ const ctl::ComPtr<xaml::ISetterBaseCollection>& spSetterCollection,
    _In_opt_ xaml::ISetter* pSetter,
    _In_ const std::function< void(wfc::IVector<xaml::SetterBase*>*, CStyle*)>& collectionFunc)
{
    ctl::ComPtr<wfc::IVector<xaml::SetterBase*>> spSetterVector;
    IFCFAILFAST(spSetterCollection.As(&spSetterVector));

    auto pCSetterCollection = checked_cast<CSetterBaseCollection>(static_cast<SetterBaseCollection*>(spSetterCollection.Get())->GetHandle());
    auto resourceGraph = GetResourceGraph();
    auto pOwnerStyle = resourceGraph->GetOwningStyle(pCSetterCollection);
    //pOwnerStyle may be null if the setter collection is used in a visual state instead of a Style.
    SetterCollectionUnsealer resealCleanup(pCSetterCollection);
    if (pOwnerStyle != nullptr && pSetter != nullptr)
    {
        auto coreSetter = checked_cast<CSetter>(static_cast<Setter*>(pSetter)->GetHandle());
        const bool setterIsValid = IsSetterValid(coreSetter);

        // Notify the change before modifying the collection so we can remove all current setters of the style
        // and only apply the ones contained in the style after the add/remove. Only do this if the setter
        // being added/removed is currently valid. Otherwise, there is no need
        bool isStyleImplicit = false;
        auto parent = do_pointer_cast<CResourceDictionary>(resourceGraph->GetParent(pOwnerStyle, &isStyleImplicit));
        if (parent && isStyleImplicit && setterIsValid)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(parent->NotifyImplicitStyleChanged(nullptr, CResourceDictionary::StyleUpdateType::ForcedByDiagnotics));
        }
        collectionFunc(spSetterVector.Get(), pOwnerStyle);

        if (!setterIsValid)
        {
            // Don't do anything if Setter.Value is unset or we failed to get the value. This can
            // happen if the target property path is invalid, or if Setter.Value hasn't been set yet.
            // If Setter.Value was just cleared, then setterValue will still be set, it will just be null.
            return;
        }

        std::vector<CStyle*> usedStyles;
        MICROSOFT_TELEMETRY_ASSERT_HR(resourceGraph->RebuildMergedSetters(pOwnerStyle, usedStyles));
        if (parent && isStyleImplicit)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(parent->NotifyImplicitStyleChanged(pOwnerStyle, CResourceDictionary::StyleUpdateType::ForcedByDiagnotics));
        }

        pOwnerStyle->NotifyMutableSetterValueChanged(coreSetter);
    }
    else
    {
        collectionFunc(spSetterVector.Get(), pOwnerStyle);
    }
}

void DiagnosticsInterop::UpdateSetterCollectionWithMultipleChanges(
    _In_ const ctl::ComPtr<xaml::ISetterBaseCollection>& spSetterCollection,
    _In_ const std::function< void(wfc::IVector<xaml::SetterBase*>*, CStyle*)>& collectionFunc)
{
    ctl::ComPtr<wfc::IVector<xaml::SetterBase*>> spSetterVector;
    IFCFAILFAST(spSetterCollection.As(&spSetterVector));
    CSetterBaseCollection *pCSetterCollection = static_cast<CSetterBaseCollection*>(static_cast<SetterBaseCollection*>(spSetterCollection.Get())->GetHandle());
    auto resourceGraph = GetResourceGraph();
    auto pOwnerStyle = resourceGraph->GetOwningStyle(pCSetterCollection);
    SetterCollectionUnsealer resealCleanup(pCSetterCollection);

    //pOwnerStyle may be null if the setter collection is used in a visual state instead of a Style
    if (pOwnerStyle != nullptr)
    {
        // Notify the change before modifying the collection so we can remove all current setters of the style
        // and only apply the new ones
        bool isStyleImplicit = false;
        auto parent = do_pointer_cast<CResourceDictionary>(resourceGraph->GetParent(pOwnerStyle, &isStyleImplicit));

        if (parent && isStyleImplicit)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(parent->NotifyImplicitStyleChanged(nullptr, CResourceDictionary::StyleUpdateType::ForcedByDiagnotics));
        }

        // Clone the original state of the setter collection so that we can invalidate properties after the collection is modified
        std::vector<xref_ptr<CSetter>> originalSetters;
        originalSetters.reserve(pCSetterCollection->size());
        for (auto& setter : *pCSetterCollection)
        {
            originalSetters.emplace_back(do_pointer_cast<CSetter>(setter));
        }

        collectionFunc(spSetterVector.Get(), pOwnerStyle);

        std::vector<CStyle*> usedStyles;
        resourceGraph->RebuildMergedSetters(pOwnerStyle, usedStyles);
        if (parent && isStyleImplicit)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(parent->NotifyImplicitStyleChanged(pOwnerStyle, CResourceDictionary::StyleUpdateType::ForcedByDiagnotics));
        }

        // Send notifications for changed setters; this is both original and current, although we want to make sure
        // we only send a single notification per property (so if a Setter in both the original set and the current set share the same
        // Property, we only want to call NotifyMutableSetterValueChanged() for one of them)
        containers::vector_set<KnownPropertyIndex> seenProperties;
        seenProperties.reserve(originalSetters.size() + pCSetterCollection->size());

        for (auto& setter : originalSetters)
        {
            KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
            MICROSOFT_TELEMETRY_ASSERT_HR(setter->GetProperty(pOwnerStyle->GetTargetType()->GetIndex(), &propertyIndex));

            auto inserted = seenProperties.emplace(propertyIndex).second;
            if (inserted)
            {
                pOwnerStyle->NotifyMutableSetterValueChanged(setter.get());
            }
        }
        for (auto& setter : *pCSetterCollection)
        {
            auto coreSetter = do_pointer_cast<CSetter>(setter);
            KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
            MICROSOFT_TELEMETRY_ASSERT_HR(coreSetter->GetProperty(pOwnerStyle->GetTargetType()->GetIndex(), &propertyIndex));

            auto inserted = seenProperties.emplace(propertyIndex).second;
            if (inserted)
            {
                pOwnerStyle->NotifyMutableSetterValueChanged(coreSetter);
            }
        }
    }
    else
    {
        for (auto setter : *pCSetterCollection)
        {
            VisualStateSetterHelper::PerformSetterOperationIfStateActive(
                do_pointer_cast<CVisualState>(pCSetterCollection->GetParentInternal(false)),
                do_pointer_cast<CSetter>(setter),
                VisualStateSetterHelper::SetterOperation::Unset);
        }

        collectionFunc(spSetterVector.Get(), pOwnerStyle);
    }
}

//Given a dependency object and a fully qualified property name (including namespace), retrieves the
//property index for that property if it is valid for the object.
HRESULT
DiagnosticsInterop::GetPropertyIndex(
    _In_ IInspectable* pInspectable,
    _In_ LPCWSTR propertyName,
    _COM_Outptr_ unsigned int* propertyIndex)
{
    IFCPTR_RETURN(pInspectable);
    IFCPTR_RETURN(propertyIndex);

    ctl::ComPtr<IInspectable> spInspectable(pInspectable);

    *propertyIndex = 0;

    const CClassInfo* pObjType = nullptr;

    IFC_RETURN(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(pInspectable, &pObjType));

    const CPropertyBase* pProp = nullptr;

    //Initially assume we're looking for a DP, and try getting that.  If it fails, look for a non-DP property instead.
    IFC_RETURN(MetadataAPI::TryGetPropertyByFullName(XSTRING_PTR_EPHEMERAL2(propertyName, wcslen(propertyName)), &pProp));

    if (pProp == nullptr)
    {
        // Property name passed in doesn't correspond to an actual property
        return E_INVALIDARG;
    }

    // If the passed in property isn't valid for the object passed in then return E_INVALIDARG. Someone could, for example,
    // pass in "Microsoft.UI.Xaml.Controls.Control.Background" when the object passed in is a Grid and they should be passing
    // in "Microsoft.UI.Xaml.Controls.Panel.Background" instead. Don't validate against custom properties
    if (!pProp->Is<CCustomProperty>() && !MetadataAPI::IsAssignableFrom(pProp->GetTargetType()->GetIndex(), pObjType->GetIndex()))
    {
        return E_INVALIDARG;
    }

    *propertyIndex = static_cast<unsigned int>(pProp->GetIndex());

    return S_OK;
}

ctl::ComPtr<DebugTool::IDebugBindingExpression> DiagnosticsInterop::TryGetBindingExpression(
    _In_ DirectUI::DependencyObject* pDO,
    _In_ BaseValueSource valueSource,
    _In_ KnownPropertyIndex propertyIndex)
{
    EffectiveValueEntry* pValueEntry = pDO ? pDO->TryGetEffectiveValueEntry(propertyIndex) : nullptr;

    // Query the binding expression, if available.
    if (pValueEntry && pValueEntry->IsExpression() && pValueEntry->GetBaseValueSource() == valueSource)
    {
        ctl::ComPtr<IInspectable> spExpressionValue = pValueEntry->GetBaseValue();

        // We want to ignore if this fails since the QI can fail if this isn't a BindingExpression.
        return spExpressionValue.AsOrNull<DebugTool::IDebugBindingExpression>();
    }

    return nullptr;
}

CCoreServices*
DiagnosticsInterop::GetCore()
{
    CCoreServices* pCS = DXamlCore::GetCurrent()->GetHandle();
    ASSERT(pCS != nullptr);

    return pCS;
}

bool
DiagnosticsInterop::IsCollection(
    _In_ IInspectable* pCollection)
{
    ctl::ComPtr<IInspectable> spCollection(pCollection);
    ctl::ComPtr<IUntypedVector> spUntypedCollection;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spTypedCollection;
    ctl::ComPtr<wfc::IVector<xaml::Controls::ICommandBarElement*>> spCommandBarCollection;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spDOCollection;
    ctl::ComPtr<xaml::IResourceDictionary> resourceDictionary;
    bool isDictionary = SUCCEEDED(spCollection.As(&resourceDictionary));
    // CTL ComPtrs don't check for null, so we'll do it once and return false if the collection is null.
    // Also return false for resource dictionaries, we don't return them in GetPropertyValuesChain anyways,
    // but iterating through ResourceDictionaries is a crash waiting to happen.
    if (pCollection == nullptr || isDictionary)
    {
        return false;
    }
    bool isUntyped = SUCCEEDED(spCollection.As(&spUntypedCollection));
    bool isInspectable = SUCCEEDED(spCollection.As(&spTypedCollection));
    bool isCommandBar = SUCCEEDED(spCollection.As(&spCommandBarCollection));
    bool isDO = SUCCEEDED(spCollection.As(&spDOCollection));

    return isUntyped || isInspectable || isCommandBar || isDO;
}

template<class T, class U> HRESULT
DiagnosticsInterop::VectorGetAt(
    _In_ wfc::IVector<T*>* pVector,
    _In_ unsigned int index,
    _COM_Outptr_result_maybenull_ IInspectable** ppInstance)
{
    *ppInstance = nullptr;
    ctl::ComPtr<U> spVectorElement;
    IFC_RETURN(pVector->GetAt(index, &spVectorElement));
    IFC_RETURN(spVectorElement.CopyTo(ppInstance));
    return S_OK;
}

template<class T, class U> HRESULT
DiagnosticsInterop::VectorInsertAt(
    _In_ wfc::IVector<T*>* pVector,
    _In_ unsigned int index,
    _In_ IInspectable* pInstance)
{
    ctl::ComPtr<U> spVectorElement;
    ctl::ComPtr<IInspectable> spVectorElementInspectable(pInstance);
    IFC_RETURN(spVectorElementInspectable.As(&spVectorElement));
    IFC_RETURN(pVector->InsertAt(index, spVectorElement.Get()));
    // We have to make sure to add the item to the namescope before resuming the storyboard in case there are any
    // target names that need to be resolved
    MICROSOFT_TELEMETRY_ASSERT_HR(EnsureElementInCorrectNamescope(pVector, pInstance));
    return S_OK;
}

template<class T, class U> bool
DiagnosticsInterop::VectorTryGetIndexOf(
    _In_ wfc::IVector<T*>* vector,
    _In_ IInspectable* item,
    unsigned int* index)
{
    ctl::ComPtr<U> spVectorElement;
    boolean found = FALSE;
    if (SUCCEEDED(item->QueryInterface(spVectorElement.ReleaseAndGetAddressOf())))
    {
        IFCFAILFAST(vector->IndexOf(spVectorElement.Get(), index, &found));
    }
    return !!found;
}

HRESULT
DiagnosticsInterop::SetValue(
    _In_ const ctl::ComPtr<IInspectable>& spInspectable,
    _In_ KnownPropertyIndex setPropertyIndex,
    _In_opt_ IInspectable* pValue)
{
    auto baseProp = DirectUI::MetadataAPI::GetPropertyBaseByIndex(setPropertyIndex);
    ctl::ComPtr<xaml::IDependencyObject> spDO;

    if (auto depProp = baseProp->AsOrNull<CDependencyProperty>())
    {
        if (auto customProperty = depProp->AsOrNull<CCustomProperty>())
        {
            IFC_RETURN(customProperty->GetXamlPropertyNoRef()->SetValue(
                spInspectable.Get(),
                pValue));
        }
        else
        {
            // Handle the dependency property case
            IFC_RETURN(spInspectable.As(&spDO));
            ctl::ComPtr<xaml::IDependencyProperty> spDP;
            IFC_RETURN(MetadataAPI::GetIDependencyProperty(setPropertyIndex, &spDP));
            auto spDOAsFW = spDO.AsOrNull<xaml::IFrameworkElement>();
            ctl::ComPtr<xaml_data::IBindingBase> spValueBinding;
            auto coreDO = spDO.Cast<DirectUI::DependencyObject>()->GetHandle();

            if (spDOAsFW && pValue != nullptr && SUCCEEDED(pValue->QueryInterface<xaml_data::IBindingBase>(&spValueBinding)))
            {
                IFC_RETURN(spDOAsFW->SetBinding(spDP.Get(), spValueBinding.Get()));
            }
            else
            {
                IFC_RETURN(spDO->SetValue(spDP.Get(), pValue));
            }

            IFC_RETURN(OnValueChanged(coreDO, setPropertyIndex));
        }
    }
    else if (auto simpleProp = baseProp->AsOrNull<CSimpleProperty>())
    {
        // Note: some types like BrushTransition are publicly not an IDependencyObject,
        // but do have a CDependencyObject.  So we query for our private DirectUI::DependencyObject
        // directly instead of the public IDependencyObject interface.

        ctl::ComPtr<DirectUI::DependencyObject> spDDO;
        IFC_RETURN(spInspectable.As(&spDDO));
        auto coreDO = spDDO->GetHandle();
        IFC_RETURN(SetValueSimpleProperty(simpleProp, coreDO, pValue));
    }

    return S_OK;
}

HRESULT
DiagnosticsInterop::InitializeStringWithFallback(
    _In_ const xstring_ptr& inputString,
    _In_ PCWSTR fallbackString,
    _Out_ HSTRING* outputString)
{
    wrl_wrappers::HString str;
    ASSERT(fallbackString);

    if (inputString.IsNullOrEmpty())
    {
        IFC_RETURN(str.Set(fallbackString));
    }
    else
    {
        IFC_RETURN(str.Set(inputString.GetBuffer()));
    }

    *outputString = str.Detach();

    return S_OK;
}

// Queries an IUIElement for its hand-in visual, returns null if it has none
HRESULT
DiagnosticsInterop::GetElementChildVisual(
    _In_ xaml::IUIElement *iuiElement,
    _Outptr_result_maybenull_ WUComp::IVisual** handinVisual)
{
    *handinVisual = nullptr;
    CUIElement* cuiElement = static_cast<CUIElement*>(static_cast<UIElement*>(iuiElement)->GetHandle());
    IFC_RETURN(cuiElement->GetHandInVisual(handinVisual));
    return S_OK;
}

// Queries an IUIElement to see if it has a Handout Visual already created for it
HRESULT
DiagnosticsInterop::TryGetElementVisual(
    _In_ xaml::IUIElement *iuiElement,
    _Outptr_result_maybenull_ WUComp::IVisual **handoutVisual)
{
    *handoutVisual = nullptr;
    UIElement* uiElement = static_cast<UIElement*>(iuiElement);
    CUIElement* cuiElement = static_cast<CUIElement*>(uiElement->GetHandle());
    if (cuiElement->IsUsingHandOffVisual() == TRUE)
    {
        IFC_RETURN(uiElement->GetElementVisual(handoutVisual));
    }
    return S_OK;
}

HRESULT
DiagnosticsInterop::UpdateSetterForStyle(
    _In_ const ctl::ComPtr<xaml::IStyle>& spStyle,
    _In_ KnownPropertyIndex propertyIndex,
    _In_ IInspectable* pValue,
    _Out_ bool* madeNewSetter)
{
    *madeNewSetter = false;

    wrl::ComPtr<xaml::ISetterBaseCollection> spSetterCollection;
    wrl::ComPtr<wfc::IVector<xaml::SetterBase*>> spSetterVector;
    unsigned int setterCount = 0;

    // Get the collection and vectors from the style.
    IFC_RETURN(spStyle->get_Setters(&spSetterCollection));
    IFC_RETURN(spSetterCollection.As(&spSetterVector));
    IFC_RETURN(spSetterVector->get_Size(&setterCount));

    //Do we have to create a new Setter for this style?
    bool shouldMakeSetter = true;

    // Find the setter targeting the given property index and modify it - start from the end of the setter
    // vector to ensure our change isn't overwritten by a setter after it
    for (int k = setterCount - 1; k >= 0; k--)
    {
        wrl::ComPtr<xaml::ISetterBase> spSetterBase;
        wrl::ComPtr<xaml::ISetter> spSetter;
        wrl::ComPtr<xaml::IDependencyProperty> spProperty;

        IFC_RETURN(spSetterVector->GetAt(k, &spSetterBase));

        if (SUCCEEDED(spSetterBase.As(&spSetter)) && spSetter)
        {
            IFC_RETURN(spSetter->get_Property(&spProperty));
            auto index = static_cast<DependencyPropertyHandle*>(spProperty.Get())->GetDP()->GetIndex();

            // Only copy the setters that aren't the new one.
            if (index == propertyIndex)
            {
                CSetterBase *pCSetter = static_cast<CSetterBase*>(static_cast<SetterBase*>(spSetterBase.Get())->GetHandle());
                SetterUnsealer resealCleanup(pCSetter);
                IFC_RETURN(spSetter->put_Value(pValue));
                shouldMakeSetter = false;
                break;
            }
        }
    }

    if (shouldMakeSetter && pValue)
    {
        ctl::ComPtr<Setter> spNewSetter;
        wrl::ComPtr<xaml::IDependencyProperty> spDP;

        IFC_RETURN(ctl::make(&spNewSetter));
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(static_cast<KnownPropertyIndex>(propertyIndex), &spDP));

        IFC_RETURN(spNewSetter->put_Property(spDP.Get()));
        IFC_RETURN(spNewSetter->put_Value(pValue));

        auto coreStyle = checked_cast<CStyle>(static_cast<Style*>(spStyle.Get())->GetHandle());
        auto newCoreSetter = checked_cast<CSetter>(static_cast<Setter*>(spNewSetter.Get())->GetHandle());
        newCoreSetter->SubscribeToValueChangedNotification(coreStyle);
        GetCore()->NotifyMutableStyleValueChangedListeners(coreStyle, propertyIndex);

        CSetterBaseCollection *pCSetterCollection = static_cast<CSetterBaseCollection*>(static_cast<SetterBaseCollection*>(spSetterCollection.Get())->GetHandle());
        SetterCollectionUnsealer resealCleanup(pCSetterCollection);
        IFC_RETURN(spSetterVector->Append(spNewSetter.Cast<xaml::ISetterBase>()));
        *madeNewSetter = true;
    }

    return S_OK;
}

wrl::ComPtr<IInspectable> DiagnosticsInterop::GetIInspectableFromKey(
        const xstring_ptr_view& key,
        const bool isImplicitStyle)
{
    wrl::ComPtr<IInspectable> value;
    VERIFYHR(DirectUI::ResourceDictionary::GetStrKeyAsInspectable(
        key,
        isImplicitStyle,
        &value));
    return value;
}

HRESULT DiagnosticsInterop::UpdateToolbarOffset(
    _In_ xaml::IUIElement* customTitleBar)
{
    wfn::Vector2 actualSize = {};

    // Offset the in-app toolbar by the custom titlebar's height to prevent the toolbar from
    // being occluded by the custom titlebar's glass window
    IFCPTR_RETURN(customTitleBar);
    IFC_RETURN(customTitleBar->get_ActualSize(&actualSize));

    IFC_RETURN(SetToolbarOffset(customTitleBar, actualSize.Y));

    return S_OK;
}

HRESULT DiagnosticsInterop::ClearToolbarOffset(
    _In_ xaml::IXamlRoot* xamlRoot)
{
    IFC_RETURN(SetToolbarOffset(xamlRoot, 0));
    return S_OK;
}

HRESULT DiagnosticsInterop::SetToolbarOffset(
    _In_ xaml::IUIElement* customTitleBar,
    _In_ float offset)
{
    ctl::ComPtr<xaml::IXamlRoot> spXamlRoot;

    IFCPTR_RETURN(customTitleBar);

    // Get the xamlroot for this window containing the titlebar
    IFC_RETURN(customTitleBar->get_XamlRoot(&spXamlRoot));

    
    return SetToolbarOffset(spXamlRoot.Get(), offset);
}


HRESULT DiagnosticsInterop::SetToolbarOffset(
    _In_ xaml::IXamlRoot* ixamlRoot,
    _In_ float offset)
{
    // If the XamlRoot for this element is null, that means the titlebar hasn't had its Enter walk or been measured yet
    // and isn't part of a visual tree.  Therefore we don't know what visual tree the titlebar belongs to, and offset the toolbar corresponding to the titlebar.
    // However when the custom titlebar is loaded in the future and its size/offset have been measured, this method will be reinvoked with
    // the actual offset and a valid XamlRoot.
    // So a null XamlRoot is OK since we'll be reinvoked when it eventually is, and apply the correct toolbar offset
    if (ixamlRoot == nullptr)
    {
        // If the XamlRoot is null, it shouldn't be part of a visual tree
        // and therefore have 0 height/no offset
        ASSERT(offset == 0.0);
        return S_OK;
    }
    ctl::ComPtr<DirectUI::XamlRoot> xamlRoot(static_cast<DirectUI::XamlRoot*>(ixamlRoot));
    CGrid* uiLayerRoot = xamlRoot->GetVisualTreeNoRef()->GetVisualDiagnosticsRoot();
    IFCPTR_RETURN(uiLayerRoot);

    // Get the in-app UI layer's list of children.  This can consist of the elements Visual Studio injects:
    // - The adorner layer for hit-testing/highlighting elements with in-app selection. We do NOT want to offset this, as it messes up hit-testing + highlight rendering
    // - The in-app toolbar, which we're trying to offset
    IFC_RETURN(uiLayerRoot->EnsureChildrenCollection());
    CUIElementCollection* gridChildren = uiLayerRoot->GetChildren();
    IFCPTR_RETURN(gridChildren);

    // Listen for any new children being added under the in-app UI layer's root.  The toolbar may've been disabled or not loaded yet,
    // and we still need to apply whatever the current offset is if/when it is loaded.
    gridChildren->SetToolbarAddedCallback([offset](CDependencyObject* potentialToolbar) { return UpdateToolbarOffsetCallback(potentialToolbar, offset); });

    // Go through any current children and update the toolbar if it's there.
    for (auto& child : *gridChildren)
    {
        IFC_RETURN(UpdateToolbarOffsetCallback(child, offset));
    }
    return S_OK;
}

HRESULT DiagnosticsInterop::UpdateToolbarOffsetCallback(
    _In_ CDependencyObject* potentialToolbar,
    _In_ float offset)
{
    HSTRING elementName;
    xaml::Thickness xamlThickness;

    // HACK: "InAppMenuRoot" is an implementation detail on the Visual Studio end, and is the name of the root element
    // of the in-app toolbar.
    const LPCWSTR toolbarName = L"InAppMenuRoot";
    const unsigned int toolbarNameLength = 13;

    // Check if the UIElement we're looking at has a name of "InAppMenuRoot", indicating it's Visual Studio's in-app toolbar
    ctl::ComPtr<DirectUI::DependencyObject> spDO;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(potentialToolbar, &spDO));
    IFC_RETURN(spDO->GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Name, &elementName));

    LPCWSTR nameStr = ::WindowsGetStringRawBuffer(elementName, nullptr);

    if (_wcsnicmp(nameStr, toolbarName, toolbarNameLength) == 0)
    {
        // Offset the vertical aspect of the in-app toolbar below the custom titlebar (or reset it).  VS uses Margin to position the toolbar horizontally
        // and it can be dragged/repositioned, so we need to respect the other existing Margin values
        IFC_RETURN(spDO->GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, &xamlThickness));
        xamlThickness.Top = offset;
        IFC_RETURN(spDO->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, xamlThickness));
    }

    return S_OK;
}
}
