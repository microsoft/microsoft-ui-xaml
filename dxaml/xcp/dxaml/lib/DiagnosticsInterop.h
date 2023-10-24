// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DiagnosticsInteropModel.h"
#include "Indexes.g.h"
#include "wilhelper.h"
#include <map>
#include <fwd/windows.system.h>
#include <fwd/Microsoft.UI.Xaml.hosting.h>

#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

class CStyle;
class CDependencyObject;
class CUIElement;
class CCoreServices;
class CDependencyProperty;
class CPropertyBase;
class xstring_ptr;
class XamlDiagnostics;
class CVisualState;
class CSetter;
class CThemeResourceExtension;
class CSetter;
class CThemeResourceExtension;
class CResourceDictionary;
class CMarkupExtensionBase;
class CResourceDictionaryCollection;
class CCustomProperty;

enum VisualElementState;

namespace ctl {
    template <typename T>
    class ComPtr;
}

namespace DirectUI {
    class Setter;
    class IXamlRoot;
}

namespace Diagnostics
{
    class ResourceDependency;
    struct ResourceGraphKey;
    struct ResourceGraphKeyWithParent;
    class ElementStateChangedBuilder;

    // Diagnostics interop is a communication layer between MUX and XamlDiagnostics. It's a singleton that can be accessed across all cores
    // and it's lifetime is managed by XamlDiagnostics. Once XamlDiagnostics goes out of scope, this object will get cleaned up, effectively
    // severing the connection. By the nature of the DependencyLocator, this object is created on-demand and so access to it should be hidden
    // behind a check for the RuntimeEnabledFeature::XamlDiagnostics. If XamlDiagnostics doesn't exist, any calls that require the connection
    // will no-op.
    class __declspec(uuid("c303ff35-f405-450b-862b-aba57b12755f")) DiagnosticsInterop final
    {

    public:

        DiagnosticsInterop();
        ~DiagnosticsInterop();

        bool IsEnabledForThread(DWORD threadId);

        void Launch(
            _In_ const wrl::ComPtr<msy::IDispatcherQueue>& dispatcher);

        void Launch(
            _In_ ctl::ComPtr<IXamlDiagnostics> diagnostics,
            _In_ const DebugTool::EnvironmentMap& map);

        void SignalMutation(
            _In_ xaml::IDependencyObject* pReference,
            _In_ VisualMutationType mutationType);

        void SignalRootMutation(
            _In_ IInspectable* pReference,
            _In_ VisualMutationType mutationType
        );

        void OnElementStateChanged(
            VisualElementState state,
            _In_ CDependencyObject* element,
            _In_ const CDependencyProperty* prop
        );

        void OnCoreDeinitialized(DWORD coreThreadId);

        std::vector<Microsoft::WRL::ComPtr<msy::IDispatcherQueue>> GetDispatcherQueues();
        // Converts the IInspectable to the CDependencyObject representation.
        // For DependencyObjects, this just returns the handle. For the application,
        // it gets the CoreApplication from DXamlCore, and for property values that
        // have primitive dependency objects (CDouble, CString, CInt32, etc.) it creates
        // a new CDependencyObject representing that.

        Microsoft::WRL::ComPtr<msy::IDispatcherQueue> GetDispatcherQueueForThreadId(DWORD threadId);

        static xref_ptr<CDependencyObject> ConvertToCore(
            _In_ IInspectable* object,
            _Out_opt_ bool* wasPeerPegged = nullptr);

        HRESULT CreateInstance(
            _In_ LPCWSTR typeName,
            _In_opt_ LPCWSTR value,
            _Outptr_ IInspectable** ppInstance);

        HRESULT GetName(
            _In_ IInspectable* pReference,
            _Out_ HSTRING *phName);

        static HRESULT GetTypeDisplayNameFromObject(
            _In_ IInspectable* pObject,
            _Out_ HSTRING* phDisplayName);

        static HRESULT GetTypeNameFor(
            _In_ IInspectable* pReference,
            _Out_ HSTRING *phName);

        static HRESULT ClearPropertyValue(
            _In_ IInspectable* pReference,
            _In_ uint32_t propertyIndex);

        HRESULT GetAllEnums(
            _Inout_ DebugTool::ICollection<DebugTool::DebugEnumInfo2>* pEnumCollection);

        HRESULT GetContentPropertyName(
            _In_ xaml::IDependencyObject* pDO,
            _Out_ LPCWSTR* pName);

        static HRESULT SetPropertyValue(
            _In_ IInspectable* pReference,
            _In_ uint32_t propertyIndex,
            _In_opt_ IInspectable* pValue);

        static HRESULT GetChildren(
            _In_ IInspectable* object,
            _Outptr_ IInspectable** children);

        // TryGetVisualTreeParent returns the parent of the element that is displayed in the visual tree.
        // If the element shouldn't be displayed in the LVT, then this method will return false. Otherwise,
        // it returns true regardless if there is a parent or not.
        bool TryGetVisualTreeParent(
            _In_ IInspectable* child,
            _COM_Outptr_result_maybenull_ IInspectable** parent);

        static bool TryGetOwnerFromIsland(
            _In_ xaml_hosting::IXamlIsland* island,
            _COM_Outptr_result_maybenull_ IInspectable** islandOwner);

        HRESULT HitTest(
            _In_ IInspectable* rootElement,
            _In_ RECT rect,
            _Inout_ DebugTool::ICollection<xaml::IDependencyObject*>* pElements);

        HRESULT GetApplication(
            _Outptr_ xaml::IApplication** ppApplication);

        static HRESULT GetApplicationStatic(
            _Outptr_ xaml::IApplication** ppApplication);

        HRESULT GetDispatcherQueue(
            _Outptr_ msy::IDispatcherQueue** ppDispatcher);

        HRESULT GetPopupRoot(
            _Outptr_ xaml::IDependencyObject** ppPopupRoot);

        HRESULT GetRoots(
            std::vector<wrl::ComPtr<IInspectable>>& roots);

        HRESULT GetXamlRoot(
            _In_ IInspectable* element,
            _Outptr_ xaml::IXamlRoot** xamlRoot);

        static HRESULT GetVisualRoot(
            _Outptr_result_maybenull_ xaml::IUIElement** ppRoot);

        static HRESULT GetVisualDiagnosticRoot(
            _In_ IInspectable* rootElement,
            _Outptr_result_maybenull_ xaml::IDependencyObject** ppRoot);

        HRESULT VisualRootUpdateLayout();

        HRESULT ConvertValueToString(
            _In_ wf::IPropertyValue* pPropertyValue,
            _In_ wf::PropertyType propertyType,
            _Out_ HSTRING* phstr);

        HRESULT GetPropertyIndex(
            _In_ IInspectable* pInspectable,
            _In_ LPCWSTR propertyName,
            _Out_ unsigned int* propertyIndex);

        HRESULT GetElementChildVisual(
            _In_ xaml::IUIElement *uiElement,
            _Outptr_result_maybenull_ WUComp::IVisual **handinVisual);

        HRESULT TryGetElementVisual(
            _In_ xaml::IUIElement *uiElement,
            _Outptr_result_maybenull_ WUComp::IVisual **handoutVisual);

        static bool IsCollection(
            _In_ IInspectable* pCollection);

        static HRESULT GetSize(
            _In_ IInspectable* pCollection,
            _Out_ unsigned int* pSize);

        static HRESULT GetAt(
            _In_ IInspectable* pCollection,
            _In_ unsigned int index,
            _Outptr_ IInspectable** ppInstance);

        static HRESULT InsertAt(
            _In_ IInspectable* pCollection,
            _In_ IInspectable* pElement,
            _In_ unsigned int index);

        static HRESULT RemoveAt(
            _In_ IInspectable* pCollection,
            _In_ unsigned int index);

        static bool TryGetIndexOf(
            _In_ IInspectable* collection,
            _In_ IInspectable* item,
            size_t& index);

        static HRESULT Clear(
            _In_ IInspectable* pCollection);

        static HRESULT SetPropertyValue(
            _In_ IInspectable* pReference,
            _In_ uint32_t propertyIndex,
            _In_opt_ IInspectable* pValue,
            _In_ bool unregisterStaticResource);

        static HRESULT ClearDependencyProperty(
            _In_ CDependencyObject* obj,
            _In_ const CDependencyProperty* property);

        static HRESULT ClearCustomProperty(
            _In_ IInspectable* reference,
            _In_ const CCustomProperty* customProp);

        static HRESULT SetThemeResourceBinding(
            _In_ CDependencyObject* depObj,
            _In_ CThemeResourceExtension* extension,
            KnownPropertyIndex propertyIndex);

        static HRESULT UpdateThemeResourceValue(
            _In_ CDependencyObject* depObj,
            _In_ CDependencyObject* value,
            KnownPropertyIndex propertyIndex);

        static HRESULT OnValueChanged(
            _In_ CDependencyObject* object,
            _In_ KnownPropertyIndex changedProperty);

        static HRESULT OnSetterChanged(
            _In_ CSetter* setter,
            _In_ bool isSetOperation = true);

        static bool IsSetterValid(_In_ CSetter* setter);
        static bool IsSetterValueValid(_In_ CSetter* setter);

        static HRESULT UpdateBasedOnStyleListeners(_In_ CStyle* style, _In_opt_ CStyle* oldBasedOn, _In_opt_ CStyle* newBasedOn);

        static HRESULT OnMarkupExtensionChanged(
            _In_ CMarkupExtensionBase* extension);

        static HRESULT ResolveResource(
            _In_ xaml::IDependencyObject* resolveFor,
            _In_opt_ xaml::IResourceDictionary* dictionaryFoundIn,
            const xstring_ptr& resourceKey,
            ResourceType resourceType,
            KnownPropertyIndex propertyIndex,
            _In_opt_ xaml::IUIElement* resolutionContext,
            _Out_ wrl::ComPtr<IInspectable>& unboxedValue);

        static HRESULT ResolveResource(
            _In_ const std::shared_ptr<ResourceDependency>& resourceDependency,
            _In_opt_ const xref_ptr<CResourceDictionary>& dictionaryFoundIn,
            const xstring_ptr& resourceKey,
            _In_opt_ xaml::IUIElement* resolutionContext,
            _Out_ wrl::ComPtr<IInspectable>& unboxedValue);

        static HRESULT AddDictionaryItem(
            _In_ xaml::IResourceDictionary* dictionaryFoundIn,
            _In_ IInspectable* key,
            _In_ IInspectable* dictionaryItem,
            _Out_ ResourceGraphKey& graphKey);

        static bool TryRemoveDictionaryItem(
            _In_ xaml::IResourceDictionary* dictionaryFoundIn,
            _In_ IInspectable* key,
            ResourceGraphKeyWithParent& graphKey);

        static HRESULT GetKeyFromIInspectable(
            _In_ IInspectable* keyInsp,
            _Out_ xstring_ptr& keyName,
            _Out_ bool* isImplicitStyle);

        static wrl::ComPtr<IInspectable> GetIInspectableFromKey(
            const xstring_ptr_view& key,
            const bool isImplicitStyle);

        static bool TryGetDictionaryItem(
            _In_ xaml::IResourceDictionary* resourceDictionary,
            const xstring_ptr_view& keyAsString,
            bool isImplicitStyle,
            _COM_Outptr_result_maybenull_ IInspectable** item);

        // Returns the core dictionary if the CDependencyObject passed in is a resource.
        static xref_ptr<CResourceDictionary> GetDictionaryIfResource(
            _In_ xaml::IResourceDictionary* resourceDictionary,
            _In_ xref_ptr<CDependencyObject> possibleKey);

        // Returns the core dictionary of the passed in dictionaries immediate parent, iff
        // the dictionary is part of a theme dictionary or merged dictionary collection
        static xref_ptr<CResourceDictionary> GetImmediateParentDictionary(
            _In_ CResourceDictionary* resourceDictionary) throw();

        // GetAllKeys uses ResourceGraphKeyWithParent because if the dictionary is removed from it's parent collection (if in one), it will
        // no longer be able to get it through CDependencyObject::GetParent
        static std::vector<ResourceGraphKeyWithParent> GetAllKeys(
            _In_ wfc::IVector<xaml::ResourceDictionary*>* dictionaryCollection);

        static std::vector<ResourceGraphKeyWithParent> GetAllKeys(
            _In_ xaml::IResourceDictionary* dictionary);

        // NOTE: Don't take dependencies on these methods.  They are only used inside RuntimeDictionary to maintain compat with older versions of VS
        // from the pre-RS5 timeframe. Retrieving ResourceDictionary entires by key is not reliable and there is no guarantee the order will
        // match the original markup.
        static std::vector<std::pair<xstring_ptr, bool>> GetKeysOrderedByIndex(_In_ xaml::IResourceDictionary* dictionary);

        // Adds the dictionary to the merged dictionary collection and returns a vector of all the keys that had previous resolutions. I.E:
        //  From:
        //      <ResourceDictionary.MergedDictionaries>
        //        <ResourceDictionary>
        //         <SolidColorBrush x:Key="mybrush">Red</SolidColorBrush>
        //       </ResourceDictionary>
        //     <ResourceDictionary.MergedDictionaries>
        // To:
        //     <ResourceDictionary.MergedDictionaries>
        //       <ResourceDictionary>
        //         <SolidColorBrush x:Key="mybrush">Red</SolidColorBrush>
        //       </ResourceDictionary>
        //       <ResourceDictionary>
        //         <SolidColorBrush x:Key="mybrush">Blue</SolidColorBrush>
        //       </ResourceDictionary>
        //     <ResourceDictionary.MergedDictionaries>
        // would return a vector with 1 element in it, which would be the dictionary containing the red brush.
        static HRESULT FindIntersectingKeys(
            _In_ wfc::IVector<xaml::ResourceDictionary*>* dictionaryCollection,
            _In_ xaml::IResourceDictionary* resourceDictionary,
            _Out_ std::vector<ResourceGraphKey>& intesectingKeys);

        static HRESULT FindOtherDictionaryForResolution(
            _In_ CResourceDictionary* dictionary,
            const xstring_ptr& key,
            bool isImplicitStyle,
            _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn);

        static HRESULT SetPropertyOnStyle(
            _In_ const ctl::ComPtr<xaml::IStyle>& style,
            _In_ KnownPropertyIndex propertyIndex,
            _In_opt_ IInspectable* pValue,
            _Out_ bool* wasValueSet);

        static HRESULT SetPropertyOnSetter(
            _In_ const ctl::ComPtr<DirectUI::Setter>& setter,
            _In_ KnownPropertyIndex propertyIndex,
            _In_opt_ IInspectable* pValue,
            _Out_ bool* wasValueSet);

        static HRESULT SetPropertyOnTimeline(
            _In_ const ctl::ComPtr<xaml_animation::ITimeline>& timeline,
            _In_ KnownPropertyIndex propertyIndex,
            _In_opt_ IInspectable* pValue,
            _Out_ bool* wasValueSet);

        static _Check_return_ HRESULT SetPropertyOnKeyFrame(
            _In_ const ctl::ComPtr<xaml_animation::IObjectKeyFrame>& keyFrame,
            _In_ KnownPropertyIndex propertyIndex,
            _In_opt_ IInspectable* pValue,
            _Out_ bool* wasValueSet);

        static ctl::ComPtr<DebugTool::IDebugBindingExpression> TryGetBindingExpression(
            _In_ DirectUI::DependencyObject* pDO,
            _In_ BaseValueSource valueSource,
            _In_ KnownPropertyIndex propertyIndex);

        static bool IsInvalidProperty(
            _In_ const CDependencyProperty* const dependencyProperty);

        static HRESULT SetValue(
            _In_ const ctl::ComPtr<IInspectable>& spInspectable,
            _In_ KnownPropertyIndex setPropertyIndex,
            _In_opt_ IInspectable* pValue);

        static HRESULT SetBinding(
            _In_ const ctl::ComPtr<xaml::IDependencyObject>& spDO,
            _In_ KnownPropertyIndex setPropertyIndex,
            _In_ xaml_data::IBindingBase* pValue,
            _Out_ bool* wasValueSet);

        static HRESULT UpdateSetterForStyle(
            _In_ const ctl::ComPtr<xaml::IStyle>& style,
            _In_ KnownPropertyIndex propertyIndex,
            _In_ IInspectable* value,
            _Out_ bool* madeNewSetter);

        static CVisualState* TryFindVisualState(_In_ const CDependencyObject* childOfVisualState);

        static HRESULT GetValueOfSimpleProperty(
            _In_ const CPropertyBase* property,
            _In_ CDependencyObject* pDO,
            _Outptr_ IInspectable** ppValue);

        static HRESULT GetValueOfCustomProperty(
            _In_ const CPropertyBase* property,
            _In_ IInspectable* pObj,
            _Outptr_ IInspectable** ppValue);

        static bool IsShareable(_In_ IInspectable* object);
        static bool CollectionIsParentToItems(_In_ IInspectable* collection);

        static KnownPropertyIndex GetSetterPropertyIndex(_In_ CSetter* setter);

        // When a custom titlebar is being used, we need to reposition the in-app toolbar below it
        // since the custom titlebar's glass window intercepts input before it can reach the in-app toolbar, making it unresponsive.
        // UpdateToolbarOffset is called when a custom titlebar has been added or its size has changed,
        // ClearToolbarOffset is called when a custom titlebar has been removed and its offset is no longer needed.
        static _Check_return_ HRESULT UpdateToolbarOffset(_In_ xaml::IUIElement* customTitleBar);
        static _Check_return_ HRESULT ClearToolbarOffset(_In_ xaml::IXamlRoot* xamlRoot);
        static _Check_return_ HRESULT SetToolbarOffset(_In_ xaml::IXamlRoot* xamlroot, float offset);

        // Will be invoked when a new UI element has been inserted as a child of the visual diagnostic root,
        // or invoked on all current children of the visual diagnostic root when the custom titlebar is inserted or changes.
        // Checks if the new element is the in-app toolbar, and offsets it if so
        static _Check_return_ HRESULT UpdateToolbarOffsetCallback(_In_ CDependencyObject* potentialToolbar, _In_ float offset);

    private:

        static HRESULT InitializeStringWithFallback(
            _In_ const xstring_ptr& inputString,
            _In_ PCWSTR fallbackString,
            _Out_ HSTRING* outputString);

        static CCoreServices* GetCore();
        template<class T, class U = T> static HRESULT VectorGetAt(
            _In_ wfc::IVector<T*>* pVector,
            _In_ unsigned int index,
            _COM_Outptr_result_maybenull_ IInspectable** ppInstance);

        template<class T, class U = T> static HRESULT VectorInsertAt(
            _In_ wfc::IVector<T*>* pVector,
            _In_ unsigned int index,
            _In_ IInspectable* pInstance);

        template<class T, class U = T> static bool VectorTryGetIndexOf(
            _In_ wfc::IVector<T*>* pVector,
            _In_ IInspectable* item,
            unsigned int* index);

        static void ForEachVisualTreeElement(
            _In_ const std::function< void(xaml::IFrameworkElement*) >& func);

        static std::vector<std::pair<wrl::ComPtr<xaml::IFrameworkElement>, wrl::ComPtr<xaml::IStyle>>> GetAllDependentItems(
            _In_ const std::vector<CStyle*>& usedStyles);

        static void UpdateSetterCollectionWithSingleChange(
            _In_ const ctl::ComPtr<xaml::ISetterBaseCollection>& spSetterCollection,
            _In_opt_ xaml::ISetter* pSetter,
            _In_ const std::function< void(wfc::IVector<xaml::SetterBase*>*, CStyle*)>& collectionFunc);

        static void UpdateSetterCollectionWithMultipleChanges(
            _In_ const ctl::ComPtr<xaml::ISetterBaseCollection>& spSetterCollection,
            _In_ const std::function< void(wfc::IVector<xaml::SetterBase*>*, CStyle*)>& collectionFunc);

        // When creating an object using the CreateInstance API's, it's possible that that the object was created, had it's name set, and then put in the visual tree.
        // If this happens, then the object will never be registered with the correct namescope. On the flip side, if an object is created, put in the tree, and then
        // has it's name set, we need to make sure it registered with the correct namescope owner. If this object is in a template, then if it has it's name property
        // set after being put in the tree, this will ensure its name is regeistered since CDependencyObject::SetName doesn't register objects if they are in a template
        static HRESULT EnsureElementInCorrectNamescope(
            _In_ const ctl::ComPtr<IInspectable>& parent,
            _In_ const ctl::ComPtr<IInspectable>& element);

        // Returns the templated parent and whether the child should be considered a namescope member
        static std::tuple<CDependencyObject*, bool> TryFindTemplatedParent(
            _In_ CDependencyObject* child);

        // Iterates through all the keys of the modified dictionary and finds where those
        // keys would resolve to using the parent dictionary as the starting scope to search.
        // For add operations, this should be done before the add is done. For removals, this
        // should be done after.
        static HRESULT FindIntersectingKeys(
            _In_ CResourceDictionary* modified,
            _In_ CResourceDictionary* parentDictionary,
            _Out_ std::vector<ResourceGraphKey>& intesectingKeys);

        static std::vector<ResourceGraphKeyWithParent> GetAllKeys(
            _In_ CResourceDictionaryCollection* mergedDictionaryCollection);

        static std::vector<ResourceGraphKeyWithParent> GetAllKeys(
            _In_ CResourceDictionary* dictionary);

        static HRESULT FindOtherDictionaryForResolution(
            _In_opt_ CResourceDictionary* startDictionary,
            _In_ const CResourceDictionary* dictionaryToSkip,
            const xstring_ptr& key,
            bool isImplicitStyle,
            _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn);

        void CacheRootObject(_In_ IInspectable*);
        void CacheDispatcherQueueForCurrentThread();

        static _Check_return_ HRESULT GetVisualTreeRoots(
            std::vector<wrl::ComPtr<IInspectable>>& roots);
        static _Check_return_ HRESULT GetXamlIslandRoots(
            std::vector<wrl::ComPtr<IInspectable>>& roots);

        static _Check_return_ HRESULT SetToolbarOffset(_In_ xaml::IUIElement* customTitleBar, float offset);

    private:

        XamlDiagnostics* m_diagnostics;

        wrl::ComPtr<DirectUI::IValueConverterInternal> m_spValueConverter;

        // In cases where we don't support multiple window's, we have to keep track of the main
        // thread so that we don't hand back mutations on multiple threads.
        DWORD m_mainThreadId;
    };

    std::shared_ptr<DiagnosticsInterop> GetDiagnosticsInterop(bool create);
}
