// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeObject.h"
#include "PropertyChainEvaluator.h"
#include "DiagnosticsInterop.h"
#include "RuntimeCollection.h"
#include "RuntimeElement.h"
#include "RuntimeShareableObject.h"
#include "RuntimeApplication.h"
#include "MetadataAPI.h"
#include "DXamlServices.h"
#include "Style.h"
#include "RuntimeDictionary.h"
#include "ComUtils.h"
#include "XamlDiagnostics.h"
#include "helpers.h"
#include "RuntimeObjectCache.h"
#include "DOPointerCast.h"


namespace Diagnostics
{
    wrl::ComPtr<IInspectable> GetIdentity(_In_ IInspectable* backingObject, InstanceHandle& identityHandle)
    {
        wrl::ComPtr<IInspectable> identity;
        identityHandle = 0u;
        if (backingObject)
        {
            IGNOREHR(backingObject->QueryInterface<IInspectable>(&identity));
            identityHandle = reinterpret_cast<InstanceHandle>(identity.Get());
        }
        return identity;
    }

    InstanceHandle MakeHandle(_In_ IInspectable* backingObject)
    {
        InstanceHandle handle = 0;
        GetIdentity(backingObject, handle);
        return handle;
    }

    RuntimeObject::~RuntimeObject()
    {
        GetRuntimeObjectCache()->InvalidateHandle(GetHandle());
    }

    void RuntimeObject::Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        m_backingObject = GetIdentity(backingObject, m_handle);
        m_parent = parent;
        GetRuntimeObjectCache()->AddToCache(shared_from_this());
        m_uiThreadId = GetCurrentThreadId();
    }

    bool RuntimeObject::TryGetAsElement(std::shared_ptr<RuntimeElement>& element)
    {
        element = std::shared_ptr<RuntimeElement>();
        return false;
    }

    bool RuntimeObject::TryGetAsCollection(std::shared_ptr<RuntimeCollection>& collection)
    {
        collection = std::shared_ptr<RuntimeCollection>();
        return false;
    }

    bool RuntimeObject::TryGetAsDictionary(std::shared_ptr<RuntimeDictionary>& dictionary)
    {
        dictionary = std::shared_ptr<RuntimeDictionary>();
        return false;
    }

    bool RuntimeObject::TryGetAsShareable(std::shared_ptr<RuntimeShareableObject>& shareable)
    {
        shareable = std::shared_ptr<RuntimeShareableObject>();
        return false;
    }

    bool RuntimeObject::TryGetAsApplication(std::shared_ptr<RuntimeApplication>& application)
    {
        application = std::shared_ptr<RuntimeApplication>();
        return false;
    }

    void RuntimeObject::SetParent(std::shared_ptr<RuntimeObject> parent)
    {
        const bool hadParent = HasParent();
        m_parent = parent;
        // Once parented, we can be removed from the cache
        if (!hadParent && parent)
        {
            GetRuntimeObjectCache()->RemoveFromCache(shared_from_this());
        }
    }

    bool RuntimeObject::TryGetParent(std::shared_ptr<RuntimeObject>& parent) const
    {
        if (HasParent())
        {
            parent = m_parent.lock();
            return true;
        }
        return false;
    }

    std::shared_ptr<RuntimeObject> RuntimeObject::GetParent() const
    {
        std::shared_ptr<RuntimeObject> parent;
        if (!TryGetParent(parent))
        {
            // Caller issue, if unsure if there is a parent, use TryGetParent
            XAML_FAIL_FAST();
        }

        return parent;
    }

    bool RuntimeObject::HasParent() const
    {
        return !m_parent.expired();
    }

    std::shared_ptr<RuntimeObject> RuntimeObject::GetXamlRoot() const
    {
        std::shared_ptr<RuntimeObject> xamlRoot;
        if (m_xamlRoot == nullptr)
        {
            // Caller issue, should only be invoked for DesktopWindowXamlSource objects
            XAML_FAIL_FAST();
        }

        return m_xamlRoot;
    }

    void RuntimeObject::SetXamlRoot(std::shared_ptr<RuntimeObject> xamlRoot)
    {
        m_xamlRoot = xamlRoot;
    }

    Microsoft::WRL::ComPtr<IInspectable> RuntimeObject::GetBackingObject() const
    {
        return m_backingObject;
    }

    void RuntimeObject::StoreValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value)
    {
        // Don't store the parent property. We keep that reference separately
        if (!prop.IsParentProperty() && value)
        {
            auto insert = m_values.emplace(prop, value);
            if (!insert.second)
            {
                insert.first->second = value;
            }

            std::shared_ptr<RuntimeShareableObject> shareable;
            if (value->TryGetAsShareable(shareable))
            {
                shareable->AddParent(shared_from_this());
            }
            else
            {
                value->SetParent(shared_from_this());
            }
        }
    }

    void RuntimeObject::StoreValueSource(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> valueSource)
    {
        if (valueSource)
        {
            std::shared_ptr<RuntimeShareableObject> shareable;
            if (valueSource->TryGetAsShareable(shareable))
            {
                m_valueSources.emplace(prop.GetPropertyChainIndex(), valueSource);
                shareable->AddParent(shared_from_this());
            }
            // Property sources should always be considered shareable. It's not the end of the world if it isn't
            ASSERT(shareable);
        }
    }

    bool RuntimeObject::TryGetPropertySource(uint32_t propertyChainIndex, std::shared_ptr<RuntimeObject>& source)
    {
        auto iter = m_valueSources.find(propertyChainIndex);
        if (iter != m_valueSources.end())
        {
            source = iter->second;
            return true;
        }
        return false;
    }

    void RuntimeObject::RemoveValue(const RuntimeProperty& prop)
    {
        auto iter = m_values.find(prop);
        if (iter != m_values.end())
        {
            std::shared_ptr<RuntimeShareableObject> shareable;
            if (iter->second->TryGetAsShareable(shareable))
            {
                shareable->RemoveParent(shared_from_this());
            }

            m_values.erase(iter);
        }
    }

    _Check_return_ HRESULT RuntimeObject::GetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject>& result)
    {
        if (!prop.IsFakeProperty())
        {
            auto baseProp = DirectUI::MetadataAPI::GetPropertyBaseByIndex(static_cast<KnownPropertyIndex>(prop.GetIndex()));

            ctl::ComPtr<IInspectable> value;
            // Handling dependency objects (including non-DPs)
            if (auto coreObj = DiagnosticsInterop::ConvertToCore(GetBackingObject().Get()))
            {
                IFC_RETURN(Diagnostics::PropertyChainEvaluator::GetEffectiveValue(coreObj, baseProp, value));
            }
            else
            {
                IFC_RETURN(DiagnosticsInterop::GetValueOfCustomProperty(baseProp, GetBackingObject().Get(), &value));
            }

            result = GetRuntimeObject(value.Get());
            StoreValue(prop, result);
        }
        else
        {
            // We don't query the property system for fake properties. Just get it from the local cache.
            auto value = m_values.find(prop);
            if (value != m_values.end())
            {
                result = value->second;
            }
        }

        return S_OK;
    }

    bool RuntimeObject::TryClearValue(const RuntimeProperty& prop)
    {
        bool succeeded = true;
        if (!prop.IsFakeProperty())
        {
            succeeded = SUCCEEDED(DiagnosticsInterop::ClearPropertyValue(GetBackingObject().Get(), prop.GetIndex()));
        }

        if (succeeded)
        {
            RemoveValue(prop);
        }

        return succeeded;
    }

    bool RuntimeObject::TrySetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value)
    {
        // Fake properties we just store locally
        bool succeeded = true;
        if (!prop.IsFakeProperty())
        {
            wrl::ComPtr<IInspectable> valueObject;
            if (value)
            {
                valueObject = value->GetBackingObject();
            }

            succeeded = SUCCEEDED(GetInterop()->SetPropertyValue(GetBackingObject().Get(), prop.GetIndex(), valueObject.Get()));
        }

        if (succeeded)
        {
            StoreValue(prop, value);
        }

        return succeeded;
    }

    InstanceHandle RuntimeObject::GetHandle() const
    {
        return m_handle;
    }

    bool RuntimeObject::IsDependencyObject() const
    {
        wrl::ComPtr<xaml::IDependencyObject> depObj;
        return !IsNull() && !IsWindow() && SUCCEEDED(GetBackingObject().As(&depObj));
    }

    bool RuntimeObject::IsWindow() const
    {
        wrl::ComPtr<xaml::IWindow> window;
        return !IsNull() && SUCCEEDED(GetBackingObject().As(&window));
    }

    bool RuntimeObject::IsDesktopWindowXamlSource() const
    {
        wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSource> xamlSource;
        return !IsNull() && SUCCEEDED(GetBackingObject().As(&xamlSource));
    }

    bool RuntimeObject::IsXamlIsland() const
    {
        wrl::ComPtr<xaml::IXamlIsland> xamlSource;
        return !IsNull() && SUCCEEDED(GetBackingObject().As(&xamlSource));
    }

    bool RuntimeObject::IsValueType() const
    {
        wrl::ComPtr<wf::IPropertyValue> propValue;
        return !IsNull() && SUCCEEDED(GetBackingObject().As(&propValue));
    }

    bool RuntimeObject::IsNull() const
    {
        return GetBackingObject() == nullptr;
    }

    PropertyChainData RuntimeObject::GetPropertyChainDataFromValue(const std::shared_ptr<RuntimeObject>& value)
    {
        auto iter = std::find_if(m_values.begin(), m_values.end(), [value](const auto& pair)
        {
            return pair.second == value;
        });

        PropertyChainData data;
        if (iter != m_values.end())
        {
            data.Index = static_cast<KnownPropertyIndex>(iter->first.GetIndex());
            data.Source = iter->first.GetValueSource();
            if (data.Source == BaseValueSourceStyle || data.Source == BaseValueSourceBuiltInStyle)
            {
                auto sourceIter = m_valueSources.find(iter->first.GetPropertyChainIndex());
                if (sourceIter == m_valueSources.end())
                {
                    XAML_FAIL_FAST();
                }
                data.Style = do_pointer_cast<CStyle>(DiagnosticsInterop::ConvertToCore(sourceIter->second->GetBackingObject().Get()));
            }
        }

        return data;
    }

    xstring_ptr RuntimeObject::ToString() const
    {
        xstring_ptr value;
        if (FAILED(XamlDiagnosticsHelpers::ValueToString(GetBackingObject().Get(), &value)))
        {
            wil::unique_hstring runtimeClassName;
            if (SUCCEEDED(GetBackingObject()->GetRuntimeClassName(&runtimeClassName)))
            {
                IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(runtimeClassName.get(), &value));
            }
        }

        return value;
    }

    void RuntimeObject::Close()
    {
        GetRuntimeObjectCache()->RemoveFromCache(shared_from_this());

        // Clear values and value sources to break references cycles. We *probably* should make the
        // value sources weak_ptr<RuntimeObject>, as they will point to either styles or setters that are contained
        // in a dictionary. However, if the peer gets destroyed, we lose source info and this seems to be enough to break the cycles.
        // Also, if we had a weak ref and didn't parent ourselves to the source, it's likely the object would never be cleaned up.
        // This just makes things simpler. We have leak detection which will help detect if this ever becomes an issue.
        m_values.clear();
        m_valueSources.clear();
    }

    std::shared_ptr<Diagnostics::DiagnosticsInterop> RuntimeObject::GetInterop()
    {
        static auto interop = GetDiagnosticsInterop(true);
        return interop;
    }

    DWORD RuntimeObject::GetAssociatedThreadId() const
    {
        return m_uiThreadId;
    }

    std::shared_ptr<RuntimeElement> GetRuntimeElement(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        std::shared_ptr<RuntimeElement> runtimeElement;
        auto runtimeObject = GetRuntimeObject(backingObject, parent);
        FAIL_FAST_ASSERT(runtimeObject->TryGetAsElement(runtimeElement));
        return runtimeElement;
    }

    std::shared_ptr<RuntimeElement> GetRuntimeElement(_In_ IInspectable* backingObject)
    {
        return GetRuntimeElement(backingObject, std::shared_ptr<RuntimeObject>());
    }

    std::shared_ptr<RuntimeCollection> GetRuntimeCollection(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        std::shared_ptr<RuntimeCollection> runtimeCollection;
        auto runtimeObject = GetRuntimeObject(backingObject, parent);
        FAIL_FAST_ASSERT(runtimeObject->TryGetAsCollection(runtimeCollection));
        runtimeCollection->EnsureItems();
        return runtimeCollection;
    }

    std::shared_ptr<RuntimeObject> TryGetRuntimeObject(_In_ CDependencyObject* backingObject)
    {
        std::shared_ptr<RuntimeObject> object;
        wrl::ComPtr<IInspectable> peer;
        if(SUCCEEDED(DirectUI::DXamlServices::TryGetPeer(backingObject, IID_PPV_ARGS(&peer))) && peer != nullptr)
        {
            object = GetRuntimeObject(peer.Get());
        }
        return object;
    }

    std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_opt_ IInspectable* backingObject)
    {
        return GetRuntimeObject(backingObject, std::shared_ptr<RuntimeObject>());
    }

    std::shared_ptr<RuntimeApplication> GetRuntimeApplication(_In_ IInspectable* backingObject)
    {
        // The application object is a bit different than others, it's a singleton object with a per-thread
        // property in Application.Resources. This method should only be called on a known UI thread. Each instance
        // will share the same backing Handle, but that should be OK. Our only interaction is storing it when we
        // first attach to a thread, and removing it when we detach. The only thing of real interest is trying to get
        // their underlying ResourceDictionary handles, which will work as expected.
        std::shared_ptr<RuntimeApplication> app(new RuntimeApplication());
        app->Initialize(backingObject, std::shared_ptr<RuntimeObject>());
        return app;
    }

    std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_opt_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        std::shared_ptr<RuntimeDictionary> owningDictionary;
        wrl::ComPtr<IInspectable> fixedBackingObject(backingObject);
        if (parent && parent->TryGetAsDictionary(owningDictionary))
        {
            // For dictionaries, just keep the value in the collection, we don't care about the key
            if (auto pair = XamlDiagnosticsHelpers::as_or_null<wfc::IKeyValuePair<IInspectable*, IInspectable*>>(backingObject))
            {
                IFCFAILFAST(pair->get_Value(&fixedBackingObject));
            }
        }

        // We don't use make_shared since we want to prohibit anyone from making a RuntimeObject incorrectly.
        // There will only ever be a 1-1 relationship with IInspectable->RuntimeObject
        std::shared_ptr<RuntimeObject> result;
        const auto objectCache = GetRuntimeObjectCache();
        if (objectCache->TryFindInCache(MakeHandle(fixedBackingObject.Get()), result))
        {
            // The object was found in our cache, parent it now.
            std::shared_ptr<RuntimeShareableObject> shareable;
            if (result->TryGetAsShareable(shareable) && parent)
            {
                shareable->AddParent(parent);
            }
            else if (parent)
            {
                result->SetParent(parent);
            }
            return result;
        }

        if (XamlDiagnosticsHelpers::is<xaml::IResourceDictionary>(fixedBackingObject.Get()))
        {
            result = std::shared_ptr<RuntimeDictionary>(new RuntimeDictionary());
        }
        else if (DiagnosticsInterop::IsCollection(fixedBackingObject.Get()))
        {
            result = std::shared_ptr<RuntimeCollection>(new RuntimeCollection());
        }
        else if (XamlDiagnosticsHelpers::is<xaml::IUIElement>(fixedBackingObject.Get()) ||
                 XamlDiagnosticsHelpers::is<xaml::IWindow>(fixedBackingObject.Get()) ||
                 XamlDiagnosticsHelpers::is<xaml_hosting::IDesktopWindowXamlSource>(fixedBackingObject.Get()) ||
                 XamlDiagnosticsHelpers::is<xaml::IXamlIsland>(fixedBackingObject.Get()))
        {
            result = std::shared_ptr<RuntimeElement>(new RuntimeElement());
        }
        else if (DiagnosticsInterop::IsShareable(fixedBackingObject.Get()) || backingObject == nullptr)
        {
            result = std::shared_ptr<RuntimeShareableObject>(new RuntimeShareableObject());
        }
        else if (XamlDiagnosticsHelpers::is<xaml::IApplication>(fixedBackingObject.Get()))
        {
            // Creating the Application object should only happen on the core thread. See comment in GetRuntimeApplication
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(DirectUI::DXamlServices::IsDXamlCoreInitialized());
            return GetRuntimeApplication(backingObject);
        }
        else
        {
            result = std::shared_ptr<RuntimeObject>(new RuntimeObject());
        }

        result->Initialize(fixedBackingObject.Get(), parent);
        return result;
    }
}
