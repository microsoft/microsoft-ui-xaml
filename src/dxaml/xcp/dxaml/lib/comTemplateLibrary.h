// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComBase.h"
#include "InterfaceForwarder.h"
#include "ComObject.h"

namespace DirectUI
{
    namespace DXamlServices
    {
        bool IsDXamlCoreInitializing();
        bool IsDXamlCoreInitialized();
    }
}

namespace ctl
{
    template <class T>
    class ActivationFactoryCreator
    {
    public:
        static IActivationFactory* CreateActivationFactory()
        {
            // We can't prevent the application from caching factories, so don't check them for leaks.
            T *pNewFactory = nullptr;
            HRESULT hr = ComObject<T>::CreateInstance(&pNewFactory, TRUE /* Disable Leak Check */);

            if (FAILED(hr))
            {
                return nullptr;
            }
            return ctl::interface_cast<IActivationFactory>(pNewFactory);
        }
    };

    // Factories are cached by the language projections so there is currently no safe way to reliably
    // release them so their reference count goes to 0 and get's deleted. We ignore the outstanding
    // allocation of the factory, however we don't want to ignore everything the factory keeps alive
    // in-case the ref counting get's messed up somewhere along the way. So instead we'll allow a way
    // to tell these factories to reset any outstanding allocations.
    class __declspec(novtable) BaseActivationFactory :
        public SupportErrorInfo
#if XCP_MONITOR
        , public wf::IClosable
#endif
    {
#if XCP_MONITOR

    public:
        IFACEMETHODIMP Close() override
        {
            return S_OK;
        }

        BEGIN_INTERFACE_MAP(BaseActivationFactory, SupportErrorInfo)
            INTERFACE_ENTRY(BaseActivationFactory, wf::IClosable)
            INTERFACE_ENTRY(BaseActivationFactory, IAgileObject)
        END_INTERFACE_MAP(BaseActivationFactory, SupportErrorInfo)

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wf::IClosable)))
            {
                *ppObject = static_cast<wf::IClosable*>(this);
            }
            else if (InlineIsEqualGUID(iid, IID_IAgileObject))
            {
                // Telling the caller we support IAgileObject, it allows for WinRT factory caching
                // Since IAgileObject has no members of its own, we can simply return IUnknown.
                return SupportErrorInfo::QueryInterfaceImpl(IID_IUnknown, ppObject);
            }
            else
            {
                return SupportErrorInfo::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }
#else
        BEGIN_INTERFACE_MAP(BaseActivationFactory, SupportErrorInfo)
            INTERFACE_ENTRY(BaseActivationFactory, IAgileObject)
        END_INTERFACE_MAP(BaseActivationFactory, SupportErrorInfo)

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, IID_IAgileObject))
            {
                // Telling the caller we support IAgileObject, it allows for WinRT factory caching
                // Since IAgileObject has no members of its own, we can simply return IUnknown.
                return SupportErrorInfo::QueryInterfaceImpl(IID_IUnknown, ppObject);
            }
            else
            {
                return SupportErrorInfo::QueryInterfaceImpl(iid, ppObject);
            }
        }
#endif

    public:
        static _Check_return_ HRESULT CheckActivationAllowedStatic(bool isFreeThreaded);
        virtual _Check_return_ HRESULT CheckActivationAllowed();
    };

#pragma region forwarders
    template<typename impl_type>
    class interface_forwarder<IActivationFactory, impl_type> final
        : public ctl::iinspectable_forwarder_base<IActivationFactory, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }
        IFACEMETHODIMP ActivateInstance(_Outptr_ IInspectable** ppInstance) override { return This()->ActivateInstance(ppInstance); }
    };
#pragma endregion
    class __declspec(novtable) TypedActivationFactoryBase
        : public BaseActivationFactory
        , public ctl::forwarder_holder<IActivationFactory, TypedActivationFactoryBase>
    {
    public:
        BEGIN_INTERFACE_MAP(TypedActivationFactoryBase, BaseActivationFactory)
            INTERFACE_ENTRY(TypedActivationFactoryBase, IActivationFactory)
        END_INTERFACE_MAP(TypedActivationFactoryBase, BaseActivationFactory)

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, IID_IActivationFactory))
            {
                *ppObject = ctl::interface_cast<IActivationFactory>(this);
            }
            else
            {
                return BaseActivationFactory::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        // IActivationFactory interface
        STDMETHODIMP ActivateInstance(_Outptr_ IInspectable **instance)
        {
            return ActivateInstanceImpl(instance);
        }

        virtual STDMETHODIMP ActivateInstanceImpl(_Outptr_ IInspectable **instance) = 0;
    };

    template <class T>
    class __declspec(novtable) ActivationFactory
        : public TypedActivationFactoryBase
    {

    public:
        // IActivationFactory interface
        STDMETHODIMP ActivateInstanceImpl(_Outptr_ IInspectable **instance) final
        {
            IFC_RETURN(CheckActivationAllowed());
            IFC_RETURN(ComObject<T>::CreateInstance(instance));
            return S_OK;
        }
    };

    class AbstractActivationFactory
        : public BaseActivationFactory
        , public ctl::forwarder_holder<IActivationFactory, AbstractActivationFactory>
    {
        BEGIN_INTERFACE_MAP(AbstractActivationFactory, BaseActivationFactory)
            INTERFACE_ENTRY(AbstractActivationFactory, IActivationFactory)
        END_INTERFACE_MAP(AbstractActivationFactory, BaseActivationFactory)

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, IID_IActivationFactory))
            {
                *ppObject = ctl::interface_cast<IActivationFactory>(this);
            }
            else
            {
                return BaseActivationFactory::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        STDMETHODIMP ActivateInstance(_Outptr_ IInspectable **)
        {
            return E_NOTIMPL;
        }

    protected:
        KnownTypeIndex GetTypeIndex() const override;
    };

    template <class T>
    class AggregableActivationFactory:
        public ActivationFactory<T>
    {
    public:

        static _Check_return_ HRESULT ActivateInstanceStatic(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance, bool shouldCheckActivationAllowed, bool isFreeThreaded)
        {
            HRESULT hr = S_OK;
            T* pObj = NULL;
            ComObject<T>* pObjAsAggregable = NULL;

            if (shouldCheckActivationAllowed)
            {
                IFC(ctl::BaseActivationFactory::CheckActivationAllowedStatic(isFreeThreaded));
            }

            if (pOuter)
            {
                IFC(ComObject<T>::CreateInstance(pOuter, &pObj));
                pObjAsAggregable = static_cast<ComObject<T>*>(pObj);
                IFC(pObjAsAggregable->NonDelegatingQueryInterface(IID_IInspectable, (void **)instance));
                pObjAsAggregable->NonDelegatingRelease();
                pObj = NULL;
            }
            else
            {
                IFC(ComObject<T>::CreateInstance(instance));
            }

        Cleanup:
            ctl::release_interface(pObj);
            return hr;
        }

        _Check_return_ HRESULT ActivateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance)
        {
            IFC_RETURN(this->CheckActivationAllowed());
            IFC_RETURN(ActivateInstanceStatic(pOuter, instance, false /* shouldCheckActivationAllowed */, false /* isFreeThreaded */));
            return S_OK;
        }

        static _Check_return_ HRESULT CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance)
        {
            HRESULT hr = S_OK;
            T* pObj = NULL;
            ComObject<T>* pObjAsAggregable = NULL;

            if (pOuter)
            {
                IFC(ComObject<T>::CreateInstance(pOuter, &pObj));
                pObjAsAggregable = static_cast<ComObject<T>*>(pObj);
                IFC(pObjAsAggregable->NonDelegatingQueryInterface(IID_IInspectable, (void **)instance));
                pObjAsAggregable->NonDelegatingRelease();
                pObj = NULL;
            }
            else
            {
                IFC(ComObject<T>::CreateInstance(instance));
            }

        Cleanup:

            ctl::release_interface(pObj);
            return hr;
        }
    };

    template <class T>
    class AggregableAbstractActivationFactory:
        public AbstractActivationFactory
    {
    public:

        static _Check_return_ HRESULT ActivateInstanceStatic(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance, bool shouldCheckActivationAllowed, bool isFreeThreaded)
        {
            HRESULT hr = S_OK;
            T* pObj = NULL;
            ComObject<T>* pObjAsAggregable = NULL;

            if (shouldCheckActivationAllowed)
            {
                IFC(ctl::BaseActivationFactory::CheckActivationAllowedStatic(isFreeThreaded));
            }

            IFCPTR(pOuter);
            IFC(ComObject<T>::CreateInstance(pOuter, &pObj));

            pObjAsAggregable = static_cast<ComObject<T>*>(pObj);
            IFC(pObjAsAggregable->NonDelegatingQueryInterface(IID_IInspectable, (void **)instance));
            pObjAsAggregable->NonDelegatingRelease();
            pObj = NULL;

        Cleanup:
            ctl::release_interface(pObj);
            return hr;
        }

        _Check_return_ HRESULT ActivateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance)
        {
            IFC_RETURN(CheckActivationAllowed());
            IFC_RETURN(ActivateInstanceStatic(pOuter, instance, false /* shouldCheckActivationAllowed */, false /* isFreeThreaded */));
            return S_OK;
        }
    };

    // Better factories.
    class BetterCoreObjectActivationFactory
        : public BaseActivationFactory
        , public ctl::forwarder_holder<IActivationFactory, BetterCoreObjectActivationFactory>
    {
        BEGIN_INTERFACE_MAP(AbstractActivationFactory, BaseActivationFactory)
            INTERFACE_ENTRY(AbstractActivationFactory, IActivationFactory)
        END_INTERFACE_MAP(AbstractActivationFactory, BaseActivationFactory)

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, IID_IActivationFactory))
            {
                *ppObject = ctl::interface_cast<IActivationFactory>(this);
            }
            else
            {
                return BaseActivationFactory::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        STDMETHODIMP ActivateInstance(_Outptr_ IInspectable** ppInstance);

    protected:
        KnownTypeIndex GetTypeIndex() const override;
    };

    class BetterAggregableCoreObjectActivationFactory:
        public BetterCoreObjectActivationFactory
    {
    public:
        static _Check_return_ HRESULT ActivateInstanceStatic(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance, _In_ KnownTypeIndex typeIndex, bool shouldCheckActivationAllowed, bool isFreeThreaded);
        _Check_return_ HRESULT ActivateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance);
        _Check_return_ HRESULT ActivateInstance(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pValue, _Outptr_ IInspectable** ppInstance);
    };

    class BetterAggregableAbstractCoreObjectActivationFactory:
        public BetterCoreObjectActivationFactory
    {
    public:
        static _Check_return_ HRESULT ActivateInstanceStatic(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance, _In_ KnownTypeIndex, bool shouldCheckActivationAllowed, bool isFreeThreaded);
        _Check_return_ HRESULT ActivateInstance(_In_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance);
        _Check_return_ HRESULT ActivateInstance(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pValue, _Outptr_ IInspectable** ppInstance);
    };

    class BetterActivationFactoryCreator
    {
    public:
        static IActivationFactory* GetForDO(KnownTypeIndex hClass);
    };

    class DynamicCoreObjectActivationFactory:
        public BetterCoreObjectActivationFactory
    {
    public:
        DynamicCoreObjectActivationFactory();

        using BetterCoreObjectActivationFactory::Initialize;

        _Check_return_ HRESULT Initialize(_In_ KnownTypeIndex eTypeIndex)
        {
            m_eTypeIndex = eTypeIndex;
            RRETURN(S_OK);
        }

    protected:
        KnownTypeIndex GetTypeIndex() const final
        {
            return m_eTypeIndex;
        }

    private:
        KnownTypeIndex m_eTypeIndex;
    };

    _Check_return_ HRESULT ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(_In_opt_ IInspectable* const pOuter, _In_ IInspectable** const ppInner, _Inout_ IUnknown** ppInstance, _In_ KnownTypeIndex typeIndex, bool isFreeThreaded);
    _Check_return_ HRESULT ValidateFactoryCreateInstanceWithBetterAggregableAbstractCoreObjectActivationFactory(_In_opt_ IInspectable* const pOuter, _In_ IInspectable** const ppInner, _Inout_ IUnknown** ppInstance, _In_ KnownTypeIndex typeIndex, bool isFreeThreaded);

    template <typename TConcrete, typename TInterface>
    _Check_return_ HRESULT ValidateFactoryCreateInstanceWithAggregableAbstractActivationFactory(_In_opt_ IInspectable* const pOuter, _In_ IInspectable** const ppInner, _Inout_ IUnknown** ppInstance, _In_ KnownTypeIndex typeIndex, bool isFreeThreaded)
    {
        ARG_VALIDRETURNPOINTER(ppInstance);
        IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);

#if DBG
        const GUID typeIIDForSanityCheck = MetadataAPI::GetClassInfoByIndex(typeIndex)->GetGuid();
        ASSERT(typeIIDForSanityCheck == __uuidof(TInterface));
#endif

        const GUID typeIID = __uuidof(TInterface);

        ComPtr<IInspectable> spInner;
        ComPtr<IUnknown> spInstance;
        IFC_RETURN(ctl::AggregableAbstractActivationFactory<TConcrete>::ActivateInstanceStatic(pOuter, &spInner, true /* shouldCheckActivationAllowed */, isFreeThreaded));
        IFC_RETURN(iunknown_cast(spInner.Get())->QueryInterface(typeIID, reinterpret_cast<void**>(spInstance.ReleaseAndGetAddressOf())));

        if (ppInner)
        {
            *ppInner = spInner.Detach();
        }

        *ppInstance = spInstance.Detach();

        return S_OK;
    }

    template <typename TConcrete, typename TInterface>
    _Check_return_ HRESULT ValidateFactoryCreateInstanceWithAggregableActivationFactory(
        _In_opt_ IInspectable* const pOuter,
        _In_ IInspectable** const ppInner,
        _Outptr_ IUnknown** ppInstance,
        _In_ KnownTypeIndex typeIndex,
        bool isFreeThreaded)
    {
        UNREFERENCED_PARAMETER(typeIndex);
        ComPtr<IInspectable> spInner;
        ComPtr<IUnknown> spInstance;
        const GUID typeIID = __uuidof(TInterface);

        ARG_VALIDRETURNPOINTER(ppInstance);
        IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);

        IFC_RETURN(ctl::AggregableActivationFactory<TConcrete>::ActivateInstanceStatic(pOuter, &spInner, true /* shouldCheckActivationAllowed */, isFreeThreaded));
        IFC_RETURN(iunknown_cast(spInner.Get())->QueryInterface(typeIID, reinterpret_cast<void**>(spInstance.ReleaseAndGetAddressOf())));

        if (ppInner)
        {
            *ppInner = spInner.Detach();
        }

        *ppInstance = spInstance.Detach();

        return S_OK;
    }
}
