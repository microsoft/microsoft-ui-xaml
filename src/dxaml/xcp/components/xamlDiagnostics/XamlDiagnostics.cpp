// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDiagnostics.h"
#include <stack>
#include "wil\resource.h"
#include "HandleMap.h"
#include "ElementStateChangedBuilder.h"
#include "runtimeEnabledFeatures\inc\RuntimeEnabledFeatures.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "metadata\inc\Indexes.g.h"
#include "DependencyObject.h"
#include "DiagnosticsInterop.h"
#include "DesignMode.h"
#include "corep.h"

#include "corewindow.h"
#include "diagnosticsInterop\inc\PropertyChainEvaluator.h"
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "deferral\inc\CustomWriterRuntimeContext.h"
#include "MultiParentShareableDependencyObject.h"
#include "MetadataAPI.h"
#include "DXamlServices.h"
#include "DoPointerCast.h"
#include "MarkupExtension.h"
#include "uielement.h"
#include "ObjectWriterStack.h"
#include "ObjectWriterFrame.h"
#include "RuntimeDictionary.h"
#include "RuntimeCollection.h"
#include "RuntimeElement.h"
#include "RuntimeApplication.h"
#include "XcpAllocationDebug.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

LPCWSTR XamlDiagnostics::_emptyString = L"";

using namespace RuntimeFeatureBehavior;
using namespace Diagnostics;
using namespace XamlDiagnosticsShared;

HRESULT XamlDiagnostics::Create(_COM_Outptr_ IXamlDiagnostics** instance)
{
    wrl::ComPtr<IXamlDiagnostics> xamlDiag;
    IFC_RETURN(wrl::MakeAndInitialize<XamlDiagnostics>(&xamlDiag));
    // The lifetime of XamlDiagnostics isn't managed by the framework. We
    // don't have our tests release this instance before leak detection because
    // we want to make sure we've released all our objects by design, and not because
    // we were forced to.
#if XCP_MONITOR
    XcpDebugSetLeakDetectionFlag(xamlDiag.Get(), true);
#endif

    *instance = xamlDiag.Detach();
    return S_OK;
}

XamlDiagnostics::XamlDiagnostics()
    : m_launched(false)
    , m_spDiagInterop(Diagnostics::GetDiagnosticsInterop(true /* create */))
    , m_runtimeObjectCache(Diagnostics::GetRuntimeObjectCache())
{
}

XamlDiagnostics::~XamlDiagnostics()
{
    // We release the instance of diagnostics interop and turn off the feature override. We turn off the features here
    // because VS keeps this instance alive even if someone detaches the debugger (they do call UnadviseVisualTreeChange).
    // We keep the features on in-case the user re-attaches.
    DependencyLocator::Internal::ReleaseInstance<Diagnostics::DiagnosticsInterop>();
    DependencyLocator::Internal::ReleaseInstance<Diagnostics::RuntimeObjectCache>();

    auto runtimeEnabledFeatureDetector = GetRuntimeEnabledFeatureDetector();
    runtimeEnabledFeatureDetector->SetFeatureOverride(RuntimeEnabledFeature::XamlDiagnostics, false);
}

HRESULT
XamlDiagnostics::RuntimeClassInitialize()
{
    IFC_RETURN(m_spDiagInterop->GetDispatcherQueue(&m_spDispatcher));
    return S_OK;
}

// Intializes the VS tap and sets site, if possible.
void
XamlDiagnostics::Launch(
    _In_ const dt::EnvironmentMap& env,
    _In_opt_ msy::IDispatcherQueue* dispatcher)
{
    HRESULT hr = S_OK;

    if (env.size() != 0)
    {
#if XCP_MONITOR
        auto lock = XcpDebugStartIgnoringLeaks();
#endif
        m_env = env;
    }

    // Only set the dispatcher once
    if (!m_spDispatcher)
    {
        m_spDispatcher = dispatcher;
    }

    // We can't launch if we don't have a dispatcher, or if we've already launched.
    // This is a silly pattern
    if (!m_spDispatcher || m_launched)
    {
        return;
    }

    CLSID clsId;
    HINSTANCE hTap = nullptr;

    wrl::ComPtr<IClassFactory> spFactory;
    wrl::ComPtr<IObjectWithSite> spSite;

    wrl::ComPtr<XamlDiagnostics> spThis;
    wrl::ComPtr<IUnknown> spThisUnknown;

    spThis.Attach(this);

    auto dllName = XamlDiagnosticsHelpers::GetEnv(L"XAML_DM_PATH", m_env, L"XamlDiagnosticsTap.dll");
    auto clsIdStr = XamlDiagnosticsHelpers::GetEnv(L"XAML_DM_CLSID", m_env, L"{28cb4df8-85eb-46ee-8d71-c614c2305f74}");

    hTap = GetModuleHandle(dllName.c_str());

    if (!hTap)
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

    IFC(CLSIDFromString(clsIdStr.c_str(), &clsId));

    auto getClassObject = reinterpret_cast<GetClassObjectMethod>(GetProcAddress(hTap, "DllGetClassObject"));
    IFC(getClassObject(clsId, IID_IClassFactory, &spFactory));

    IFC(spFactory->CreateInstance(nullptr, __uuidof(IObjectWithSite), reinterpret_cast<void**>(spSite.GetAddressOf())));

    IFC(spThis.As(&spThisUnknown));
    IFC(spSite->SetSite(spThisUnknown.Get()));

    m_launched = true;

    Cleanup:
    // the original code caught the exception, "Traced" it and then ate the error and returned.  This was because:
    //    1. There is not much we can do about a failure here.  The tap either could not load or could
    //       not add site, so the app will still need to run.
    //    2. If the site does not have a ref, then the caller will not have a ref, at which point this
    //       object will be cleaned up.
    // Our IFC macro has already logged the failure, so we just return
    return;
}

HRESULT
XamlDiagnostics::OnCoreDeinitialized(DWORD coreThreadId)
{
    m_enabledThreads.erase(coreThreadId);

    std::shared_ptr<RuntimeApplication> app;
    if (m_runtimeObjectCache->TryFindAssociatedApplication(coreThreadId, app))
    {
        app->Close();
    }

    // If this is the last thread to be deinitialized, we'll release all our resources.
    // We may want to store handles in a per thread map if this is problematic.
    if (m_enabledThreads.empty())
    {
        auto lock = m_doHandleMapLock.lock_exclusive();
        m_doHandleMap.clear();
        m_env.clear();
    }

    return S_OK;
}

void ValidateContext(const Diagnostics::ElementStateChangedContext& finalContext, _In_ CDependencyObject* lastKnownElement)
{
    bool valid = finalContext.RootHandle != 0;
    if (!valid)
    {
        // If we don't have a RootHandle, then let's check the last known element to see
        // if the reason we couldn't walk up the tree any further is expected.

        if (auto dictionary = do_pointer_cast<CResourceDictionary>(lastKnownElement))
        {
            // For dictionaries set by the Source property, visual studio is able to handle the fact that
            // we don't properly walk up the chain. Ideally we would show something like this:
            //    Resources:FrameworkElement/Source:ResourceDictionary["Key"]/Setters:Style/Value:Setter
            // Instead we just show this:
            //    ["Key"]/Setters:Style/Value:Setter
            // This works because visual studio is able to map the InstanceHandle of the dictionary to
            // whomever owns it.
            valid = !dictionary->m_strSource.IsNullOrEmpty();
        }
    }
    MICROSOFT_TELEMETRY_ASSERT_DISABLED(valid);
}

void XamlDiagnostics::OnElementStateChanged(
    VisualElementState state,
    _In_ CDependencyObject* element,
    _In_ const CDependencyProperty* invalidProperty)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IVisualTreeServiceCallback2> errorHandler;
    if (m_visualTreeCallback && SUCCEEDED(m_visualTreeCallback.As(&errorHandler)))
    {
        ElementStateChangedBuilder builder(element, invalidProperty);

        // Start with the element that was passed in.
        auto parent = GetParentForElementStateChanged(element);
        CDependencyObject* child = element;

        while (parent && !builder.IsContextReady())
        {
            auto parentDictionary = do_pointer_cast<CResourceDictionary>(parent);
            bool handled = false;
            if (parentDictionary)
            {
                IFC(builder.AddResourceDictionaryContext(parentDictionary, child, &handled));
            }
            if (!handled)
            {
                auto parentCollection = do_pointer_cast<CDOCollection>(parent);
                if (parentCollection)
                {
                    IFC(builder.AddCollectionContext(parentCollection, child, &handled));
                }
            }

            if (!handled)
            {
                IFC(builder.AddParentContext(parent, child, &handled));
            }
            ASSERT(handled);
            child = parent;
            parent = GetParentForElementStateChanged(child);
        }

        auto finalContext = builder.GetContext();
        ValidateContext(finalContext, child);
        if (!finalContext.RootHandle)
        {
            // We weren't able to find a parent trying to walk up the tree. Register this
            // instance with the RuntimeObjectCache
            wrl::ComPtr<IInspectable> lastParent;
            if (SUCCEEDED(DirectUI::DXamlServices::GetPeer(child, IID_PPV_ARGS(&lastParent))))
            {
                IFC(RegisterInstance(lastParent.Get(), &finalContext.RootHandle));
            }
        }
        IFC(errorHandler->OnElementStateChanged(finalContext.RootHandle, state, finalContext.PathToError.c_str()));
    }

Cleanup:
    // Previous behavior logged and then ignored a thrown exception.  Our IFC handlers will take care of logging, so
    // we just need to ignore any errors:
    return;
}

IFACEMETHODIMP
XamlDiagnostics::GetEnums(
    _Out_ unsigned int* pCount,
    _Deref_post_opt_count_(*pCount) EnumType** ppEnums)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppEnums);
    *ppEnums = nullptr;
    IFCPTR_RETURN(ppEnums);
    *pCount = 0;

    Collection<dt::DebugEnumInfo2> enums;
    IFC_RETURN(m_spDiagInterop->GetAllEnums(&enums));

    ComValueCollection<EnumType> result;
    for (auto& enumInfo : enums.GetVectorView())
    {
        EnumType enumType = {};
        const auto& values = enumInfo.GetValues();
        wil::unique_bstr name = wil::make_bstr_nothrow(enumInfo.GetName());
        SafeArrayPtr<int> valueIntsSafeArray(values.size());
        SafeArrayPtr<BSTR> valueStringsSafeArray(values.size());

        enumType.Name = name.release();

        for (size_t j = 0; j < values.size(); j++)
        {
            wil::unique_bstr valueString = wil::make_bstr_nothrow(values[j].second);

            valueIntsSafeArray[j] = values[j].first;
            valueStringsSafeArray[j] = valueString.release();
        }

        enumType.ValueInts = valueIntsSafeArray.Detach();
        enumType.ValueStrings = valueStringsSafeArray.Detach();

        result.Append(std::move(enumType));
    }

    *pCount = static_cast<unsigned int>(result.GetSize());
    *ppEnums = result.RealizeAndDetach();

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::CreateInstance(
    _In_ BSTR typeName,
    _In_opt_ BSTR valueStr,
    _Out_ InstanceHandle* pInstanceHandle)
{
    SuspendFailFastOnStowedException suspender;
    RETURN_HR_IF_NULL(E_INVALIDARG, pInstanceHandle);
    *pInstanceHandle = 0;
    RETURN_HR_IF_NULL(E_INVALIDARG, typeName);

    wrl::ComPtr<IInspectable> result;
    RETURN_IF_FAILED(m_spDiagInterop->CreateInstance(typeName, valueStr, &result));
    RETURN_IF_FAILED(RegisterInstance(result.Get(), pInstanceHandle));

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetPropertyValuesChain(
    _In_ InstanceHandle instanceHandle,
    _Out_ unsigned int* pSourceCount,
    _Deref_post_opt_count_(*pSourceCount) PropertyChainSource** ppPropertySources,
    _Out_ unsigned int* pPropertyCount,
    _Deref_post_opt_count_(*pPropertyCount) PropertyChainValue** ppPropertyValues)
{
    SuspendFailFastOnStowedException suspender;
    RETURN_HR_IF_NULL(E_INVALIDARG, ppPropertyValues);
    *ppPropertyValues = nullptr;
    RETURN_HR_IF_NULL(E_INVALIDARG, pPropertyCount);
    *pPropertyCount = 0;
    RETURN_HR_IF_NULL(E_INVALIDARG, ppPropertySources);
    *ppPropertySources = nullptr;
    RETURN_HR_IF_NULL(E_INVALIDARG, pSourceCount);
    *pSourceCount = 0;
    std::vector<wil::unique_propertychainsource> sources;
    std::vector<wil::unique_propertychainvalue> values;

    // If we own this, populate the property chain for it
    std::shared_ptr<RuntimeObject> foundObj;
    if (TryFindObjectFromHandle(instanceHandle, foundObj) && foundObj->IsDependencyObject())
    {
        RETURN_IF_FAILED(PopulateDOPropertyChain(foundObj, sources, values));
    }
    else if (foundObj && foundObj->IsWindow())
    {
        RETURN_HR(E_NOINTERFACE);
    }
    else
    {
        RETURN_HR(E_NOTFOUND);
    }

    *ppPropertySources = Realize(std::move(sources), pSourceCount);
    *ppPropertyValues = Realize(std::move(values), pPropertyCount);

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::SetProperty(
    _In_ InstanceHandle instanceHandle,
    _In_ InstanceHandle valueHandle,
    _In_ unsigned int propertyIndex)
{
    SuspendFailFastOnStowedException suspender;
    RuntimeProperty prop(propertyIndex);
    if (instanceHandle == RenderingSwitch &&
        prop.IsRenderingSwitch() &&
        DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        // This is the case where VS wants us to temporarily disable/enable rendering,
        // as they prepare to make layout updates.
        CCoreServices* coreServices = DirectUI::DXamlServices::GetHandle();
        coreServices->SetIsRenderEnabled(static_cast<bool>(valueHandle));
        return S_OK;
    }

    std::shared_ptr<RuntimeObject> foundProperty;
    const bool foundValue = TryFindObjectFromHandle(valueHandle, foundProperty);
    // If XamlDiagnostics owns the owner of the property, let XamlDiag
    //  set the property, otherwise let CompVisualDiag do it.
    std::shared_ptr<RuntimeObject> foundObj;
    if (TryFindObjectFromHandle(instanceHandle, foundObj) && foundValue)
    {
        // Ensure this isn't one of the "fake" property indexes we handed VS
        IFC_RETURN(ValidatePropertyIndex(prop));
        RETURN_HR_IF(E_FAIL, !foundObj->TrySetValue(prop, foundProperty));
        if (XamlDiagnosticsHelpers::is<xaml::IStyle>(foundObj->GetBackingObject().Get()))
        {
            RETURN_IF_FAILED(m_spDiagInterop->VisualRootUpdateLayout());
        }
    }
    else
    {
        RETURN_HR(E_NOTFOUND);
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::ClearProperty(
    _In_ InstanceHandle instanceHandle,
    _In_ unsigned int propertyIndex)
{
    SuspendFailFastOnStowedException suspender;

    RuntimeProperty prop(propertyIndex);
    // Ensure this isn't one of the "fake" property indexes we handed VS
    RETURN_IF_FAILED(ValidatePropertyIndex(prop));
    std::shared_ptr<RuntimeObject> foundObj;
    if (TryFindObjectFromHandle(instanceHandle, foundObj) && !foundObj->IsNull())
    {
        RETURN_HR_IF(E_FAIL, !foundObj->TryClearValue(prop));
    }
    else
    {
        RETURN_HR(E_NOTFOUND);
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetCollectionCount(
    _In_ InstanceHandle instanceHandle,
    _Out_ unsigned int* pCollectionSize)
{
    SuspendFailFastOnStowedException suspender;
    RETURN_HR_IF_NULL(E_INVALIDARG, pCollectionSize);
    *pCollectionSize = 0;
    unsigned int size = 0;

    std::shared_ptr<RuntimeCollection> foundCollection;
    std::shared_ptr<RuntimeDictionary> foundDictionary;
    if (TryFindCollectionFromHandle(instanceHandle, foundCollection))
    {
        size = static_cast<unsigned int>(foundCollection->size());
    }
    else if (TryFindDictionaryFromHandle(instanceHandle, foundDictionary))
    {
        size = static_cast<unsigned int>(foundDictionary->GetItemsByIndex().size());
    }
    else
    {
        RETURN_HR(E_NOTFOUND);
    }

    *pCollectionSize = size;

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetCollectionElements(
    _In_ InstanceHandle collectionHandle,
    _In_ unsigned int startIndex,
    _Inout_ unsigned int* pElementCount,
    _Deref_post_opt_count_(*pElementCount) CollectionElementValue** ppElementValues)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppElementValues);
    *ppElementValues = nullptr;

    unsigned int size = 0;
    IFC_RETURN(GetCollectionCount(collectionHandle, &size));
    unsigned int count = std::min(*pElementCount, size - startIndex);
    *pElementCount = 0;

    auto makeElement = [](const std::shared_ptr<RuntimeObject>& item, unsigned int index){
        wil::unique_collectionelementvalue element;
        element.Value = SysAllocString(item->ToString().GetBuffer());

        wrl_wrappers::HString spTypeName;
        IGNOREHR(DiagnosticsInterop::GetTypeDisplayNameFromObject(item->GetBackingObject().Get(), spTypeName.ReleaseAndGetAddressOf()));

        element.MetadataBits = GetMetadataBits(item->GetBackingObject());
        element.Index = index;
        element.ValueType = SysAllocString(spTypeName.GetRawBuffer(nullptr));
        return element;
    };

    std::shared_ptr<RuntimeCollection> foundCollection;
    std::shared_ptr<RuntimeDictionary> foundDictionary;

    if (TryFindCollectionFromHandle(collectionHandle, foundCollection))
    {
        ComValueCollection<wil::unique_collectionelementvalue> elements;
        auto startIter = foundCollection->begin() + startIndex;
        unsigned int index = startIndex;
        std::for_each(startIter, startIter + count, [&](const auto& item) {
            elements.Append(makeElement(item, index++));
        });

        *pElementCount = elements.GetSize();
        *ppElementValues = elements.RealizeAndDetach();
    }
    else if (TryFindDictionaryFromHandle(collectionHandle, foundDictionary))
    {
        ComValueCollection<wil::unique_collectionelementvalue> elements;
        auto items = foundDictionary->GetItemsByIndex();
        auto startIter = items.begin() + startIndex;
        unsigned int index = startIndex;
        std::for_each(startIter, startIter + count, [&](const auto& item) {
            elements.Append(makeElement(item, index++));
        });

        *pElementCount = elements.GetSize();
        *ppElementValues = elements.RealizeAndDetach();
    }
    else
    {
        IFC_RETURN(E_NOTFOUND);
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::AddChild(
    _In_ InstanceHandle collectionHandle,
    _In_ InstanceHandle childHandle,
    _In_ unsigned int index)
{
    SuspendFailFastOnStowedException suspender;

    std::shared_ptr<RuntimeCollection> collection;
    RETURN_HR_IF(E_NOTFOUND, !TryFindCollectionFromHandle(collectionHandle, collection));

    std::shared_ptr<RuntimeObject> child;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(childHandle, child));
    RETURN_HR_IF(E_INVALIDARG, child->IsNull());

    std::vector<ResourceGraphKey> currentResolutions;
    wrl::ComPtr<xaml::IResourceDictionary> childDictionary;
    if (auto parentCollection = XamlDiagnosticsHelpers::as_or_null<wfc::IVector<xaml::ResourceDictionary*>>(collection->GetBackingObject().Get()))
    {
        RETURN_IF_FAILED(child->GetBackingObject().As(&childDictionary));
        IFC_RETURN(DiagnosticsInterop::FindIntersectingKeys(parentCollection.Get(), childDictionary.Get(), currentResolutions));
    }

    RETURN_HR_IF(E_FAIL, !collection->TryInsertAt(index, child));
    for (const auto& graphKey : currentResolutions)
    {
        // Update current dependent items with the new dictionary that was added
        IFC_RETURN(UpdateDependentItemsOnAdd(ResourceGraphKey(graphKey.Dictionary.lock(), graphKey.Key), childDictionary.Get()));
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::RemoveChild(
    _In_ InstanceHandle collectionHandle,
    _In_ unsigned int index)
{
    SuspendFailFastOnStowedException suspender;

    std::shared_ptr<RuntimeCollection> collection;
    RETURN_HR_IF(E_NOTFOUND, !TryFindCollectionFromHandle(collectionHandle, collection));
    RETURN_HR_IF(E_INVALIDARG, index >= collection->size());

    auto removedChild = (*collection)[index];

    std::vector<ResourceGraphKeyWithParent> removedKeys;
    wrl::ComPtr<xaml::IResourceDictionary> dictionary;
    if (SUCCEEDED(removedChild->GetBackingObject().As(&dictionary)))
    {
        removedKeys = DiagnosticsInterop::GetAllKeys(dictionary.Get());
    }

    RETURN_HR_IF(E_FAIL, !collection->TryRemoveAt(index));

    for (const auto& removedKey : removedKeys)
    {
        IFC_RETURN(UpdateDependentItemsOnRemove(removedKey));
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::ClearChildren(
    _In_ InstanceHandle collectionHandle)
{
    SuspendFailFastOnStowedException suspender;

    std::shared_ptr<RuntimeCollection> foundCollection;
    std::shared_ptr<RuntimeDictionary> foundDictionary;
    if (TryFindCollectionFromHandle(collectionHandle, foundCollection))
    {
        RETURN_HR_IF(E_FAIL, !foundCollection->TryClear());
    }
    else if (TryFindDictionaryFromHandle(collectionHandle, foundDictionary))
    {
        RETURN_HR_IF(E_FAIL, !foundDictionary->TryClear());
    }
    else
    {
        return E_NOTFOUND;
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetPropertyIndex(
    _In_ InstanceHandle object,
    _In_ LPCWSTR propertyName,
    _Out_ unsigned int* pPropertyIndex)
{
    SuspendFailFastOnStowedException suspender;

    std::shared_ptr<RuntimeObject> foundObject;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(object, foundObject));
    RETURN_HR_IF(E_INVALIDARG, foundObject->IsNull());
    RETURN_HR_IF(E_NOINTERFACE, foundObject->IsWindow());

    RETURN_IF_FAILED(m_spDiagInterop->GetPropertyIndex(foundObject->GetBackingObject().Get(), propertyName, pPropertyIndex));
    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetProperty(
    _In_ InstanceHandle object,
    _In_ unsigned int propertyIndex,
    _Outptr_ InstanceHandle* pValue)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(pValue);
    *pValue = 0;

    RuntimeProperty prop(propertyIndex);
    // Ensure this isn't one of the "fake" property indexes we handed VS
    RETURN_IF_FAILED(ValidatePropertyIndex(prop));

    std::shared_ptr<RuntimeObject> foundObject;
    if (TryFindObjectFromHandle(object, foundObject) && !foundObject->IsNull())
    {
        if (foundObject->IsWindow())
        {
            RETURN_HR(E_NOINTERFACE);
        }
        std::shared_ptr<RuntimeObject> runtimeValue;
        IFC_RETURN(foundObject->GetValue(prop, runtimeValue));
        *pValue = runtimeValue->GetHandle();
    }
    else
    {
        RETURN_HR(E_NOTFOUND);
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetDispatcher(
    _Outptr_ IInspectable** ppDispatcher)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppDispatcher);
    *ppDispatcher = nullptr;

    if (!m_spDispatcher)
        return E_FAIL;

    return m_spDispatcher.CopyTo(ppDispatcher);
}

IFACEMETHODIMP
XamlDiagnostics::GetUiLayer(
    _Outptr_ IInspectable** ppLayer)
{
    return GetUiLayerForXamlRoot(0u, ppLayer);
}

IFACEMETHODIMP
XamlDiagnostics::GetApplication(
    _Outptr_ IInspectable** ppApplication)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppApplication);
    *ppApplication = nullptr;

    wrl::ComPtr<xaml::IApplication> spApplication;
    RETURN_IF_FAILED(m_spDiagInterop->GetApplication(&spApplication));
    *ppApplication = spApplication.Detach();

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetIInspectableFromHandle(
    _In_ InstanceHandle instanceHandle,
    _Outptr_ IInspectable** ppInstance)
{
    SuspendFailFastOnStowedException suspender;

    IFCPTR_RETURN(ppInstance);
    *ppInstance = nullptr;
    if (instanceHandle != 0u)
    {
        std::shared_ptr<RuntimeObject> object;
        if (TryFindObjectFromHandle(instanceHandle, object))
        {
            *ppInstance = object->GetBackingObject().Detach();
            return S_OK;
        }
    }

    return E_NOTFOUND;
}

bool XamlDiagnostics::TryFindObjectFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeObject>& object)
{
    if (m_runtimeObjectCache->TryFindInCache(handle, object))
    {
        return true;
    }

    if (handle == 0u)
    {
        // No null object has been made yet, create it now
        object = GetRuntimeObject(nullptr);
        return true;
    }

    return false;
}

bool XamlDiagnostics::TryFindElementFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeElement>& elem)
{
    std::shared_ptr<Diagnostics::RuntimeObject> obj;
    if (TryFindObjectFromHandle(handle, obj) && obj->TryGetAsElement(elem))
    {
        return true;
    }
    return false;
}

bool XamlDiagnostics::TryFindCollectionFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeCollection>& collection)
{
    std::shared_ptr<Diagnostics::RuntimeObject> obj;
    if (TryFindObjectFromHandle(handle, obj) && obj->TryGetAsCollection(collection))
    {
        // Collections aren't populated by default, so ensure their items now.
        collection->EnsureItems();
        return true;
    }
    return false;
}

bool XamlDiagnostics::TryFindDictionaryFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeDictionary>& dictionary)
{
    std::shared_ptr<Diagnostics::RuntimeObject> obj;
    if (TryFindObjectFromHandle(handle, obj) && obj->TryGetAsDictionary(dictionary))
    {
        return true;
    }
    return false;
}

IFACEMETHODIMP
XamlDiagnostics::GetHandleFromIInspectable(
    _In_ IInspectable* inspectable,
    _Out_ InstanceHandle* handle)
{
    SuspendFailFastOnStowedException suspender;
    auto foundObj = GetObjectFromInspectable(inspectable);
    *handle = foundObj->GetHandle();
    return S_OK;
}

std::shared_ptr<RuntimeObject> XamlDiagnostics::GetObjectFromInspectable(_In_ IInspectable* inspectable, std::shared_ptr<RuntimeObject> optionalParent)
{
    std::shared_ptr<RuntimeObject> foundObj;
    if (!TryFindObjectFromHandle(HandleMap::GetHandle(inspectable), foundObj))
    {
        foundObj = GetRuntimeObject(inspectable, optionalParent);
    }

    return foundObj;
}

IFACEMETHODIMP
XamlDiagnostics::HitTest(
    _In_ RECT rect,
    _Out_ unsigned int* pCount,
    _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles)
{
    return HitTestForXamlRoot(0u, rect, pCount, ppInstanceHandles);
}

IFACEMETHODIMP
XamlDiagnostics::RegisterInstance(
    _In_ IInspectable* pInstance,
    _Out_ InstanceHandle* pInstanceHandle)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(pInstanceHandle);
    *pInstanceHandle = 0;
    IFCPTR_RETURN(pInstance);

    *pInstanceHandle = GetRuntimeObject(pInstance)->GetHandle();

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetInitializationData(
    _Out_ BSTR *pInitializationData)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(pInitializationData);

    std::wstring initializationData = XamlDiagnosticsHelpers::GetEnv(DebugTool::g_wszInitializationData, m_env, L"");
    wil::unique_bstr bstrInitializationData = wil::make_bstr_nothrow(initializationData.data());
    *pInitializationData = bstrInitializationData.release();
    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetUiLayerForXamlRoot(
    _In_ InstanceHandle instanceHandle,
    _Outptr_ IInspectable** ppLayer)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppLayer);
    *ppLayer = nullptr;

    IInspectable* rootElement = nullptr;

    if (instanceHandle != 0u)
    {
        // Get the UI layer for the Xaml root we're hit testing for - for Xaml islands scenarios
        // we need this to be non-null to get the specific root, otherwise we can
        // assume it's for the main visual tree
        std::shared_ptr<RuntimeObject> rootObject;
        if (!(TryFindObjectFromHandle(instanceHandle, rootObject)))
        {
            RETURN_HR(E_INVALIDARG);
        }

        rootElement = rootObject->GetBackingObject().Get();
    }
    
    // Always grab the diagnostics root for the current thread. This way, VS can draw
    // on the correct window when debuggin an app with multiple window support.
    wrl::ComPtr<xaml::IDependencyObject> spDO;
    IFC_RETURN(m_spDiagInterop->GetVisualDiagnosticRoot(rootElement, &spDO));

    *ppLayer = spDO.Detach();
    return S_OK;
}

IFACEMETHODIMP 
XamlDiagnostics::HitTestForXamlRoot(
    _In_ InstanceHandle instanceHandle,
    _In_ RECT rect,
    _Out_ unsigned int* pCount,
    _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(ppInstanceHandles);
    *ppInstanceHandles = nullptr;
    IFCPTR_RETURN(pCount);
    *pCount = 0;

    IInspectable* rootElement = nullptr;
    if (instanceHandle != 0u)
    {
        // Get the object for the Xaml root we're hit testing for - for Xaml islands scenarios
        // we need this to be non-null to narrow down the tree we're hit testing for,
        // otherwise the handle can be null.
        std::shared_ptr<RuntimeObject> rootObject;
        if (!(TryFindObjectFromHandle(instanceHandle, rootObject)))
        {
            RETURN_HR(E_INVALIDARG);
        }
        
        rootElement = rootObject->GetBackingObject().Get();
    }

    ComValueCollectionTranslator<xaml::IDependencyObject*, InstanceHandle> hits;
    IFC_RETURN(m_spDiagInterop->HitTest(rootElement, rect, &hits));

    *pCount = hits.GetSize();
    *ppInstanceHandles = hits.RealizeAndDetach([&](xaml::IDependencyObject* pDO)
        {
            InstanceHandle handle = 0;

            // Since we are hit testing, we should only be hitting elements in the live tree, which means that we should
            // have a strong reference to the element already.
            IGNOREHR(HandleStore::CreateHandle(pDO, &handle));
            return handle;
        });

    return S_OK;
}

HRESULT
XamlDiagnostics::PopulateDOPropertyChain(
    const std::shared_ptr<RuntimeObject>& obj,
    _Inout_ std::vector<wil::unique_propertychainsource>& sources,
    _Inout_ std::vector<wil::unique_propertychainvalue>& values)
{
    if (!obj->IsDependencyObject())
    {
        XAML_FAIL_FAST();
    }

    wrl::ComPtr<xaml::IDependencyObject> backingDO;
    IFCFAILFAST(obj->GetBackingObject().As(&backingDO));

    Diagnostics::PropertyChainEvaluator evaluator(static_cast<DirectUI::DependencyObject*>(backingDO.Get()));
    values.reserve(evaluator.GetMaxPropertyCount());
    for (auto& data : evaluator)
    {
        EvaluatedValue reportedValue;
        IFC_RETURN(evaluator.Evaluate(data, reportedValue));

        const bool isValueHandle = HasMetadataBit(reportedValue.PropertyData.MetadataBits, MetadataBit::IsValueHandle) &&
                                    !HasMetadataBit(reportedValue.PropertyData.MetadataBits, MetadataBit::IsValueNull);

        RuntimeProperty prop(data.Index, data.Source, reportedValue.PropertyData.PropertyChainIndex);

        // We need to keep track of property sources, if we cause the peer to be destroyed, we lose source information
        // along with the objects identity.
        if (data.Source != BaseValueSourceDefault && data.Source != BaseValueSourceLocal)
        {
            std::shared_ptr<RuntimeObject> runtimeSource;
            if (!obj->TryGetPropertySource(reportedValue.PropertyData.PropertyChainIndex, runtimeSource))
            {
                runtimeSource = GetRuntimeObject(evaluator.GetSourceAtIndex(reportedValue.PropertyData.PropertyChainIndex).Get());
                obj->StoreValueSource(prop, runtimeSource);
            }
        }

        if (isValueHandle)
        {
            auto runtimeValue = GetRuntimeObject(reportedValue.Value.Get());
            obj->StoreValue(prop, runtimeValue);
        }

        values.push_back(std::move(reportedValue.PropertyData));
    }

    // If querying a binding, add those to the chain
    if (XamlDiagnosticsHelpers::is<xaml_data::IBinding>(backingDO.Get()))
    {
        IFC_RETURN(AddBindingPropertiesToChain(obj, values));
    }

    auto uiElement = XamlDiagnosticsHelpers::as_or_null<xaml::IUIElement>(obj->GetBackingObject().Get());
    if (uiElement)
    {
        wil::unique_propertychainvalue desiredSize;
        IFC_RETURN(evaluator.GetDesiredSize(uiElement.Get(), desiredSize));
        values.push_back(std::move(desiredSize));
    }

    values.shrink_to_fit();

    // Get the sources from the evaluator that it found while populating the property chain
    sources.reserve(evaluator.GetFoundSourceCount());
    for (auto& source : evaluator.StealSources())
    {
        sources.push_back(std::move(source.SourceData));
    }

    return S_OK;
}

bool
XamlDiagnostics::CanConvertValueToString(
    _In_ IInspectable* valueIInsp)
{
    return (valueIInsp == nullptr ||
        XamlDiagnosticsHelpers::is<wf::IPropertyValue>(valueIInsp) ||
        XamlDiagnosticsHelpers::is<wf::IUriRuntimeClass>(valueIInsp) ||
        XamlDiagnosticsHelpers::is<xaml_media::IFontFamily>(valueIInsp));
}

HRESULT
XamlDiagnostics::AddBindingPropertiesToChain(
    const std::shared_ptr<RuntimeObject>& bindingObj,
    _Inout_ std::vector<wil::unique_propertychainvalue>& values)
{
    auto target = bindingObj->GetParent();

    auto data = target->GetPropertyChainDataFromValue(bindingObj);

    wrl::ComPtr<xaml::IDependencyObject> targetDO;
    IFC_RETURN(target->GetBackingObject().As(&targetDO));

    Diagnostics::PropertyChainEvaluator evaluator(static_cast<DirectUI::DependencyObject*>(targetDO.Get()));
    EvaluatedValue evaluatedBinding;
    IFC_RETURN(evaluator.EvaluateBinding(data, evaluatedBinding));

    if (HasMetadataBit(evaluatedBinding.PropertyData.MetadataBits, MetadataBit::IsValueHandle))
    {
        std::shared_ptr<RuntimeObject> runtimeValue;
        if (!TryFindObjectFromHandle(std::stoll(evaluatedBinding.PropertyData.Value), runtimeValue))
        {
            // We couldn't find the existing object, make a new one
            runtimeValue = GetRuntimeObject(evaluatedBinding.Value.Get(), bindingObj);
        }
        bindingObj->StoreValue(RuntimeProperty(FakePropertyIndex::EvaluatedValue), runtimeValue);
    }

    values.push_back(std::move(evaluatedBinding.PropertyData));
    values.push_back(evaluator.GetIsBindingValid(data));

    return S_OK;
}

// Validates property indices for Xaml runtime. Doesn't know anything about composition indices. This
// should be used before trying to set/get/clear properties on xaml types.
HRESULT XamlDiagnostics::ValidatePropertyIndex(const Diagnostics::RuntimeProperty& prop) const
{
    return prop.IsFakeProperty() ? E_FAIL : S_OK;
}

long long XamlDiagnostics::GetMetadataBits(_In_ const wrl::ComPtr<IInspectable>& object)
{
    if (object)
    {
        MetadataBit isObjectBit = !XamlDiagnostics::CanConvertValueToString(object.Get()) ? MetadataBit::IsValueHandle : MetadataBit::None;
        MetadataBit isCollectionBit = Diagnostics::DiagnosticsInterop::IsCollection(object.Get()) ? MetadataBit::IsValueCollection : MetadataBit::None;

        return isObjectBit | isCollectionBit;
    }
    else
    {
        return MetadataBit::IsValueNull;
    }
}

bool XamlDiagnostics::HasMetadataBit(
    _In_ long long bits,
    _In_ MetadataBit bit
)
{
    return ((bits & bit) == bit);
}
