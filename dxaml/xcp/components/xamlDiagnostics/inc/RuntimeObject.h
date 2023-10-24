// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeProperty.h"
#include "XamlOM.WinUI.h"
#include "DiagnosticsInterop.h"
#include "wil\resource.h"
#include <vector>

enum class KnownTypeIndex : UINT16;
enum class KnownPropertyIndex : UINT16;
class xstring_ptr;
class CDependencyObject;

namespace Diagnostics
{
    class RuntimeObject;
    class RuntimeElement;
    class RuntimeCollection;
    class RuntimeDictionary;
    class RuntimeShareableObject;
    class RuntimeApplication;
    struct PropertyChainData;

    std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_opt_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_opt_ IInspectable* backingObject);
    std::shared_ptr<RuntimeObject> TryGetRuntimeObject(_In_ CDependencyObject* backingObject);
    std::shared_ptr<RuntimeElement> GetRuntimeElement(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    std::shared_ptr<RuntimeElement> GetRuntimeElement(_In_ IInspectable* backingObject);
    std::shared_ptr<RuntimeCollection> GetRuntimeCollection(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    std::shared_ptr<RuntimeApplication> GetRuntimeApplication(_In_ IInspectable* backingObject);
    InstanceHandle MakeHandle(_In_ IInspectable* backingObject);

    // RuntimeObject encapsulates an IInspectable that is being modified at design time. Generally, these are some sort of DependencyObject,
    // however that is not always the case. To retrieve a RuntimeObject, use the Diagnostics::GetRuntimeObject API. This ensures that there
    // is always a 1-1 relationship between IInspectable and RuntimeObject.
    class RuntimeObject : public std::enable_shared_from_this<RuntimeObject>
    {
        friend std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);

    public:
        virtual ~RuntimeObject();

        Microsoft::WRL::ComPtr<IInspectable> GetBackingObject() const;

        // Temporary methods for updating the storage of properties. Ideally we can remove this and GetValue/ClearValue.
        // Can do the right thing. These are required because of how ResourceDependency and ResolveResource works.
        // Perhaps we can refactor that to hold onto the RuntimeObject
        void StoreValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value);
        void StoreValueSource(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> valueSource);
        void RemoveValue(const RuntimeProperty& prop);

        bool TryGetPropertySource(uint32_t propertyChainIndex, std::shared_ptr<RuntimeObject>& source);
        _Check_return_ HRESULT GetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject>& result);
        PropertyChainData GetPropertyChainDataFromValue(const std::shared_ptr<RuntimeObject>& value);

        virtual bool TrySetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value);
        virtual bool TryClearValue(const RuntimeProperty& prop);

        bool TryGetParent(std::shared_ptr<RuntimeObject>& parent) const;
        std::shared_ptr<RuntimeObject> GetParent() const;
        virtual void SetParent(std::shared_ptr<RuntimeObject> parent);
        bool HasParent() const;

        // Gets and sets the XamlRoot associated with this object.  Currently only supported
        // for DesktopWindowXamlSource elements
        std::shared_ptr<RuntimeObject> GetXamlRoot() const;
        void SetXamlRoot(std::shared_ptr<RuntimeObject> xamlRoot);

        InstanceHandle GetHandle() const;

        // Helper methods for knowing what type of object is returned by GetBackingObject()
        bool IsDependencyObject() const;
        bool IsWindow() const;
        bool IsNull() const;
        bool IsDesktopWindowXamlSource() const;
        bool IsValueType() const;

        template <typename T>
        bool IsValueType() const
        {
            if (IsValueType())
            {
                wrl::ComPtr<wf::IReference<T>> reference;
                return SUCCEEDED(GetBackingObject().As(&reference));
            }
            return false;
        }

        // Virtual methods since we can't dynamic-cast.
        virtual bool TryGetAsDictionary(std::shared_ptr<RuntimeDictionary>& dictionary);
        virtual bool TryGetAsElement(std::shared_ptr<RuntimeElement>& element);
        virtual bool TryGetAsCollection(std::shared_ptr<RuntimeCollection>& collection);
        virtual bool TryGetAsShareable(std::shared_ptr<RuntimeShareableObject>& shareable);
        virtual bool TryGetAsApplication(std::shared_ptr<RuntimeApplication>& shareable);

        // When a RuntimeObject is no longer needed, it can be Closed, which will manually remove it from the RuntimeObjectCache.
        // Generally, this isn't needed as most items are already removed from the cache, and kept alive by whatever object is owning
        // them. However, in cases like the VisualTree roots and the per-thread Application object, they need to be manually closed.
        virtual void Close();

        // Converts the object to a string. If the object can not be converted to a string, this returns the
        // handle in string form. On failure, returns the runtime class name.
        xstring_ptr ToString() const;

        DWORD GetAssociatedThreadId() const;
    protected:
        RuntimeObject() = default;

        static std::shared_ptr<Diagnostics::DiagnosticsInterop> GetInterop();

        template <typename Derived>
        std::shared_ptr<Derived> derived_shared_from_this()
        {
            auto runtimObjThis = RuntimeObject::shared_from_this();
            return std::static_pointer_cast<Derived>(runtimObjThis);
        }

        virtual void Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    private:
        InstanceHandle m_handle;
        Microsoft::WRL::ComPtr<IInspectable> m_backingObject;
        std::weak_ptr<RuntimeObject> m_parent;
        std::map<RuntimeProperty, std::shared_ptr<RuntimeObject>> m_values;
        std::map<int, std::shared_ptr<RuntimeObject>> m_valueSources;
        std::shared_ptr<RuntimeObject> m_xamlRoot;
        DWORD m_uiThreadId = 0u;
    };
}
