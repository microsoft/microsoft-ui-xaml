// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "includes.h"
#include "xamlDiagnostics\HandleStore.h"
#include "XamlOM.WinUI.Private.h"
#include "DependencyLocator\inc\DependencyLocator.h"
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "com\inc\ComPtr.h"
#include "DiagnosticsInterop.h"
#include <set>
#include <array>
#include "RuntimeObject.h"
#include "RuntimeObjectCache.h"
#include "XcpAllocation.h"
#include <fwd/Microsoft.UI.Xaml.h>
#include <fwd/Microsoft.UI.Xaml.hosting.h>

class xstring_ptr;

namespace DebugTool {
    static LPCWSTR g_wszInitializationData = L"XAML_DM_INITIALIZATIONDATA";
}

namespace Resources {
    struct ResolvedResource;
}

namespace Diagnostics {
    class ResourceDependency;
    class PropertyChainEvaluator;
    struct ResourceGraphKey;
    struct ResourceGraphKeyWithParent;
    class RuntimeElement;
    class RuntimeProperty;
}

class
    __declspec(uuid("{2b5c2ba6-fa8d-4566-a151-8f4da65c2643}"))
    XamlDiagnostics : public wrl::RuntimeClass<
        wrl::RuntimeClassFlags<wrl::ClassicCom>,
        IXamlDiagnostics,
        IXamlDiagnostics2,
        // ChainInterfaces is a swell little helper that makes our lives better when dealing with COM
        // interfaces that derive from base interfaces. The class only implements the first one, so this
        // should always be the most derived, but will still be able to QI for the other classes in the list.
        wrl::ChainInterfaces<IVisualTreeService3, IVisualTreeService2, IVisualTreeService>,
        IXamlDiagnosticsTestHooks,
        wrl::FtmBase>
{

private:
    static LPCWSTR _emptyString;

public:
    static HRESULT Create(_COM_Outptr_ IXamlDiagnostics** instance);

    XamlDiagnostics();
    ~XamlDiagnostics() override;
    HRESULT RuntimeClassInitialize();

    void Launch(
        _In_ const dt::EnvironmentMap& env,
        _In_opt_ msy::IDispatcherQueue* dispatcher);

    void SignalMutation(
        _In_ xaml::IDependencyObject* pReference,
        _In_ VisualMutationType type);

    HRESULT OnCoreDeinitialized(
        _In_ DWORD coreThreadId);

    void SignalRootMutation(
        _In_opt_ IInspectable* root,
        _In_ VisualMutationType type);

    void OnWindowActivated(
        _In_ const wrl::ComPtr<xaml::IWindow>& window);

    void OnDesktopWindowXamlSourceClosed(
        _In_ xaml_hosting::IDesktopWindowXamlSource* windowSource);

    void OnDesktopWindowXamlSourceCreated(
        _In_ xaml_hosting::IDesktopWindowXamlSource* windowSource);

    void OnElementStateChanged(
        VisualElementState state,
        _In_ CDependencyObject* element,
        _In_ const CDependencyProperty* invalidProperty);

    // IVisualTreeService Interface

    IFACEMETHOD(AdviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback) override;
    IFACEMETHOD(UnadviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback) override;

    IFACEMETHOD(GetEnums)(
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) EnumType** ppEnums) override;

    IFACEMETHOD(CreateInstance)(
        _In_ BSTR typeName,
        _In_opt_ BSTR valueStr,
        _Out_ InstanceHandle* pInstanceHandle) override;

    IFACEMETHOD(GetPropertyValuesChain)(
        _In_ InstanceHandle instanceHandle,
        _Out_ unsigned int* pSourceCount,
        _Deref_post_opt_count_(*pSourceCount) PropertyChainSource** ppPropertySources,
        _Out_ unsigned int* pPropertyCount,
        _Deref_post_opt_count_(*pPropertyCount) PropertyChainValue** ppPropertyValues) override;

    IFACEMETHOD(SetProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ InstanceHandle valueHandle,
        _In_ unsigned int propertyIndex) override;

    IFACEMETHOD(ClearProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int propertyIndex) override;

    IFACEMETHOD(GetCollectionCount)(
        _In_ InstanceHandle instanceHandle,
        _Out_ unsigned int* pCollectionSize) override;

    IFACEMETHOD(GetCollectionElements)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int startIndex,
        _Inout_ unsigned int* pElementCount,
        _Deref_post_opt_count_(*pElementCount) CollectionElementValue** ppElementValues) override;

    IFACEMETHOD(AddChild)(
        _In_ InstanceHandle parent,
        _In_ InstanceHandle child,
        _In_ unsigned int index) override;

    IFACEMETHOD(RemoveChild)(
        _In_ InstanceHandle parent,
        _In_ unsigned int index) override;

    IFACEMETHOD(ClearChildren)(
        _In_ InstanceHandle parent) override;

    //IVisualTreeService2 Interface
    IFACEMETHOD(GetPropertyIndex)(
        _In_ InstanceHandle object,
        _In_ LPCWSTR propertyName,
        _Out_ unsigned int* propertyIndex) override;

    IFACEMETHOD(GetProperty)(
        _In_ InstanceHandle object,
        _In_ unsigned int propertyIndex,
        _Out_ InstanceHandle* pValue) override;

    IFACEMETHOD(ReplaceResource)(
        _In_ InstanceHandle resourceDictionary,
        _In_ InstanceHandle key,
        _In_ InstanceHandle newValue) override;

    IFACEMETHOD(RenderTargetBitmap)(
        _In_ InstanceHandle handle,
        _In_ RenderTargetBitmapOptions options,
        _In_ unsigned int maxPixelWidth,
        _In_ unsigned int maxPixelHeight,
        _Outptr_ IBitmapData** ppBitmapData) override {return E_NOTIMPL; }

    // IVisualTreeService3 Interface

    IFACEMETHOD(ResolveResource)(
        _In_ InstanceHandle resourceContext,
        _In_z_ LPCWSTR resourceName,
        _In_ ResourceType resourceType,
        _In_ unsigned int propertyIndex) override;

    IFACEMETHOD(GetDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_z_ LPCWSTR resourceName,
        _In_ BOOL resourceIsImplicitStyle,
        _Out_ InstanceHandle* resourceHandle) override;

    IFACEMETHOD(AddDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle resourcKey,
        _In_ InstanceHandle resourceHandle) override;

    IFACEMETHOD(RemoveDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle resourcKey) override;

    // IXamlDiagnostics Interface

    IFACEMETHOD(GetDispatcher)(
        _Outptr_ IInspectable** ppDispatcher) override;

    IFACEMETHOD(GetUiLayer)(
        _Outptr_ IInspectable** ppLayer) override;

    IFACEMETHOD(GetApplication)(
        _Outptr_ IInspectable** ppApplication) override;

    IFACEMETHOD(GetIInspectableFromHandle)(
        _In_ InstanceHandle instanceHandle,
        _Outptr_ IInspectable** ppInstance) override;

    IFACEMETHOD(GetHandleFromIInspectable)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pHandle) override;

    IFACEMETHOD(HitTest)(
        _In_ RECT rect,
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles) override;

    IFACEMETHOD(RegisterInstance)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pInstanceHandle) override;

    IFACEMETHOD(GetInitializationData)(
        _Out_ BSTR *pInitializationData) override;

    // IXamlDiagnostics2 Interface
    IFACEMETHOD(GetUiLayerForXamlRoot)(
        _In_ InstanceHandle instanceHandle,
        _Outptr_ IInspectable** ppLayer) override;

    IFACEMETHOD(HitTestForXamlRoot)(
        _In_ InstanceHandle instanceHandle,
        _In_ RECT rect,
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles) override;

    // IXamlDiagnosticsTestHooks Interface
    IFACEMETHOD(UnregisterInstance)(\
        _In_ InstanceHandle handle) override;
    IFACEMETHOD(TryGetDispatcherQueueForObject)(\
        _In_ InstanceHandle handle,
        _Outptr_ IInspectable** queue) override;

public:
    // Public static helper methods
    static bool CanConvertValueToString(
        _In_ IInspectable* valueIInsp);

    static wil::unique_sourceinfo GetSourceInfo(
        _In_ xaml::ISourceInfoPrivate* sourceInfo);

private:

    HRESULT PopulateElementAndRelationCache(
        const std::shared_ptr<Diagnostics::RuntimeElement>& root,
        _In_ int depth,
        _In_ bool srcInfo,
        _Inout_ ComValueCollection<wil::unique_visualelement>& elements,
        _Inout_ std::map<InstanceHandle, ParentChildRelation>& relations);

    HRESULT PopulateDOPropertyChain(
        const std::shared_ptr<Diagnostics::RuntimeObject>& obj,
        _Inout_ std::vector<wil::unique_propertychainsource>& sources,
        _Inout_ std::vector<wil::unique_propertychainvalue>& values);

    HRESULT ConvertEnumsVector(
        _In_ const Collection<dt::DebugEnumInfo2>& enums,
        _Out_ ComValueCollection<EnumType>& result);

    HRESULT GiveRootToCallback(
        const std::shared_ptr<Diagnostics::RuntimeElement>& root,
        _In_ IVisualTreeServiceCallback* callback,
        _In_ const ParentChildRelation& rootParentRelation);

    HRESULT GiveRootsToCallback(
        _In_ const wrl::ComPtr<IVisualTreeServiceCallback>& callbacks);

    HRESULT GiveCurrentWindowToCallback(
        _In_ const wrl::ComPtr<IVisualTreeServiceCallback>& callback
    );

    void RemoveRootObjectFromLVT(_In_ IInspectable* root);
    void RemoveElementFromLVT(const std::shared_ptr<Diagnostics::RuntimeElement>& removedElement);
    void AddElementToLVT(const std::shared_ptr<Diagnostics::RuntimeElement>& addedElement, const std::shared_ptr<Diagnostics::RuntimeElement>& parentElement);

    void PopulateParentRelation(
        const std::shared_ptr<Diagnostics::RuntimeElement>& runtimeElement,
        const std::shared_ptr<Diagnostics::RuntimeElement>& runtimeParent,
        _Inout_ ParentChildRelation& relation);

    HRESULT PopulateElementAndRelationCache(
        const std::shared_ptr<Diagnostics::RuntimeElement>& element,
        _Inout_ ComValueCollection<wil::unique_visualelement>& elements,
        _Inout_ std::map<InstanceHandle, ParentChildRelation>& relations,
        _Inout_ std::queue<std::shared_ptr<Diagnostics::RuntimeElement>>& processingQueue,
        _In_ bool srcInfo);

    void PopulateElementInfo(
        const std::shared_ptr<Diagnostics::RuntimeElement>& runtimeElement,
        _Inout_ VisualElement& element);

    HRESULT AddBindingPropertiesToChain(
        const std::shared_ptr<Diagnostics::RuntimeObject>& bindingObj,
        _Inout_ std::vector<wil::unique_propertychainvalue>& values);

    HRESULT ValidatePropertyIndex(const Diagnostics::RuntimeProperty& prop) const;

    void AddDependencyObjectToMap(
        _In_ xaml::IDependencyObject* pDO,
        _Out_opt_ InstanceHandle* handle = nullptr);

    bool GetHandleForDependencyObject(
        _In_ xaml::IDependencyObject* pDO,
        _Out_ InstanceHandle* handle
    );

    HRESULT GetWindowText(_In_ const wrl::ComPtr<xaml::IWindow>& window, _Out_ wil::unique_bstr& text);

    static long long GetMetadataBits(_In_ const wrl::ComPtr<IInspectable>& object);

    inline static bool HasMetadataBit(
        _In_ long long bits,
        _In_ MetadataBit bit
    );

    template<typename T>
    static T* Realize(std::vector<T>&& vec, _Out_ unsigned int *size)
    {
        auto vecSize = vec.size();
        T* pResult = reinterpret_cast<T*>(CoTaskMemAlloc(sizeof(T) * vecSize));
        IFCOOMFAILFAST(pResult);
        ZeroMemory(pResult, sizeof(T) * vecSize);
        auto guard = wil::scope_exit([&]()
        {
            CoTaskMemFree(pResult);
        });

        std::move(vec.begin(), vec.end(), pResult);
        guard.release();
        vec.clear();

        *size = static_cast<unsigned int>(vecSize);
        return pResult;
    }

    HRESULT UpdateDependentItem(
        const std::shared_ptr<Diagnostics::ResourceDependency>& dependency,
        const xstring_ptr& key,
        const xref_ptr<CResourceDictionary>& dictionary);

    // Common helper methods for when adding/removing items from a dictionary, or
    // when adding/removing dictionaries from a collection
    HRESULT UpdateDependentItemsOnAdd(
        const Diagnostics::ResourceGraphKey& currentResolution,
        _In_ xaml::IResourceDictionary* newDictionary);

    HRESULT UpdateDependentItemsOnRemove(
        const Diagnostics::ResourceGraphKeyWithParent& removedGraphKey);

    bool TryFindObjectFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeObject>& obj);
    bool TryFindElementFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeElement>& elem);
    bool TryFindCollectionFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeCollection>& obj);
    bool TryFindDictionaryFromHandle(InstanceHandle handle, std::shared_ptr<Diagnostics::RuntimeDictionary>& obj);
    std::shared_ptr<Diagnostics::RuntimeObject> GetObjectFromInspectable(
        _In_ IInspectable* inspectable,
        std::shared_ptr<Diagnostics::RuntimeObject> optionalParent = std::shared_ptr<Diagnostics::RuntimeObject>());

private:
    // InstanceHandle value to be passed in for switching rendering state.
    static const InstanceHandle RenderingSwitch = 0;

    bool m_launched;

#if XCP_MONITOR
    // We use the LeakIgnoringAllocator here because the unordered_set stores it's buckets in a vector that it doesn't resize
    // when you use .clear(). The lookup/insert/erase benefits of unordered_set (compared to std::set) outweigh this little nuance.
    // We should verify during post test cleanup that these tables are clear.
    typedef std::set<DWORD, std::less<DWORD>, XcpAllocation::LeakIgnoringAllocator<DWORD>> EnabledThreadSet;

#else
    typedef std::set<DWORD> EnabledThreadSet;
#endif

    EnabledThreadSet m_enabledThreads;
    dt::EnvironmentMap m_env;

    std::shared_ptr<Diagnostics::DiagnosticsInterop> m_spDiagInterop;
    std::shared_ptr<Diagnostics::RuntimeObjectCache> m_runtimeObjectCache;

    ctl::ComPtr<msy::IDispatcherQueue> m_spDispatcher;

    wrl::ComPtr<IVisualTreeServiceCallback> m_visualTreeCallback;

    // we need this map because we can't QI in SignalMutation when the DO is leaving the tree.
    // instead we will store the InstanceHandle representation of the do inside of SignalMutation
    // when the object enters the tree and then query this map when it leaves.
#if XCP_MONITOR
    typedef std::map<xaml::IDependencyObject*, InstanceHandle,
        std::less<xaml::IDependencyObject*>,
        XcpAllocation::LeakIgnoringAllocator<std::pair<xaml::IDependencyObject*, InstanceHandle>>> AvoidQIHandleMap;
#else
    typedef std::map<xaml::IDependencyObject*, InstanceHandle> AvoidQIHandleMap;
#endif
    AvoidQIHandleMap m_doHandleMap;

    // we need this to synchronize access to our plain std::map of DOs
    wil::srwlock m_doHandleMapLock;
};

CoCreatableClass(XamlDiagnostics);
