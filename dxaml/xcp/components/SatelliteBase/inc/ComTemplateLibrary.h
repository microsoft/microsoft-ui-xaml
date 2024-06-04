// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace pctl
{

    interface INonDelegatingUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        virtual ULONG STDMETHODCALLTYPE NonDelegatingAddRef( void) = 0;
        virtual ULONG STDMETHODCALLTYPE NonDelegatingRelease( void) = 0;
    };

    interface INonDelegatingInspectable: public INonDelegatingUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetIids( 
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetRuntimeClassName( 
            /* [out] */ __RPC__deref_out_opt HSTRING *className) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetTrustLevel( 
            /* [out] */ __RPC__out TrustLevel *trustLevel) = 0;
    };


    // --------------------------------------------------------------------------------------------
    //
    // class AggregableComObject<TBASE>
    //
    // Synopsis:
    //      AggregableComObject augments TBASE type with the ability to participate in COM
    //      aggregation as the aggregatee. The implementation is roughly based on 
    //      ctl::AggregableComObject, the main difference being that it it intended to be used with
    //      wrl only, whereas ctl::AggregableComObject depends on other infastructure provided in
    //      the ctl library.
    //
    //      It is expected that TBASE
    //      1. Inherits from wrl::RuntimeClass
    //      2. Does no significant work in the constructor.
    //      3. Has a protected InitializeImpl method that performs all initialization.
    //      4. Has a protected virtual IsComposed method that returns FALSE.
    //      5. Optionally implements an I<TBASE>Overrides interface, which defines the methods
    //         that an aggregating object should be able to override, and in it's implementation
    //          checks IsComposed to determine whether or not calls to those methods should be
    //          delegated to the outer object.
    //
    //      See PickerFlyoutBaseFactory::CreateInstance and PickerFlyoutBase for an example of the
    //      use of this class.
    //
    // --------------------------------------------------------------------------------------------
    template <class TBASE, class TINTERFACE = IInspectable>
    class AggregableComObject:
        public INonDelegatingInspectable,
        public TBASE
    {

    private:

        AggregableComObject()
        {
        }

        HRESULT InitializeWithOuter(_In_opt_ IInspectable* pOuter, 
                                    _COM_Outptr_ IInspectable** inner)
        {
            wrl::ComPtr<IInspectable> spInner;
            *inner = nullptr;

            // Set inner to point to the INonDelegatingInspectable vtable for this object
            IFC_RETURN(NonDelegatingQueryInterface(__uuidof(IInspectable), static_cast<void**>(&spInner)));

            if (pOuter)
            {
                m_pControllingUnknown = pOuter;
                m_isComposed = true;
            }
            else
            {
                // There is no outer object, so there's no one for the delegating IInpsectable
                // methods to delegate to. In this case, we set them up to forward everything to
                // the non-delegating IInspectable methods instead. 
                m_pControllingUnknown = spInner.Get();
                m_isComposed = false;
            } 

            IFC_RETURN(TBASE::InitializeImpl(pOuter));
           
            // The inner object needs to be passed back to the caller with refcount 1, so 
            // release the ref that the NonDelegatingQueryInterface added.
            NonDelegatingRelease();
            *inner = spInner.Detach();
            return S_OK;
        }

    protected:

        virtual ~AggregableComObject()
        {
            // Weak ref - no need to release
            m_pControllingUnknown = nullptr;
        }

        // Any class used as the TBASE template parameter should declare a virtual
        // IsComposed() method that returns FALSE, and should use this method to
        // determine whether or not to QI for an overrides interface when invoking
        // a public virtual method.
        boolean IsComposed() override
        {
            return m_isComposed;
        }

    public:
        
        // IInspectable (non-delegating) implementation
        IFACEMETHODIMP NonDelegatingQueryInterface(_In_ REFIID iid, _Outptr_ void **ppValue) override
        {
            IUnknown *pInterface = NULL;
           
            // This works because the vtable layout for INonDelegatingUnknown and 
            // INonDelegatingInspectable is identical to that of IUnknown and IInspectable, 
            // respectively. Hence, future uses of the IInspectable methods on the returned 
            // pointers will actually go to to the non-delegating methods 
            if (iid == __uuidof(IUnknown))
            {
                pInterface = reinterpret_cast<IUnknown *>(static_cast<INonDelegatingUnknown *>(this));
                pInterface->AddRef();
            }
            else if (iid == __uuidof(IInspectable))
            {
                pInterface = reinterpret_cast<IInspectable *>(static_cast<INonDelegatingInspectable *>(this));
                pInterface->AddRef();
            }
            else
            {
                return TBASE::QueryInterface(iid, ppValue);
            }

            *ppValue = pInterface;
            return S_OK;
        }

        IFACEMETHODIMP_(ULONG) NonDelegatingAddRef() override
        {
            return TBASE::AddRef();
        }

        IFACEMETHODIMP_(ULONG) NonDelegatingRelease() override
        {
            return TBASE::Release();
        }

        IFACEMETHODIMP NonDelegatingGetRuntimeClassName(_Out_ HSTRING *pClassName) override
        {
            return TBASE::GetRuntimeClassName(pClassName);
        }

        IFACEMETHODIMP NonDelegatingGetTrustLevel(_Out_ TrustLevel *trustLvl) override
        {
            return TBASE::GetTrustLevel(trustLvl);
        }

        IFACEMETHODIMP NonDelegatingGetIids(
            _Out_ ULONG *iidCount,
            _When_(*iidCount == 0, _At_(*iids, _Post_null_))
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_))
            _Outptr_result_buffer_maybenull_(*iidCount) _Result_nullonfailure_ IID **iids) override
        {
            return TBASE::GetIids(iidCount, iids);
        }

        // IInspectable (delegating) implementation
        IFACEMETHODIMP QueryInterface(_In_ REFIID iid, _Outptr_ void **ppValue) override
        {
            return m_pControllingUnknown->QueryInterface(iid, ppValue);
        }

        IFACEMETHODIMP_(ULONG) AddRef() override
        {
            return m_pControllingUnknown->AddRef();
        }

        IFACEMETHODIMP_(ULONG) Release() override
        {
            return m_pControllingUnknown->Release();
        }

        IFACEMETHODIMP GetRuntimeClassName(_Out_ HSTRING *pClassName) override
        {
            return m_pControllingUnknown->GetRuntimeClassName(pClassName);
        }

        IFACEMETHODIMP GetTrustLevel(_Out_ TrustLevel *trustLvl) override
        {
            return m_pControllingUnknown->GetTrustLevel(trustLvl);
        }

        IFACEMETHODIMP GetIids(
            _Out_ ULONG *iidCount,
            _When_(*iidCount == 0, _At_(*iids, _Post_null_))
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_))
            _Outptr_result_buffer_maybenull_(*iidCount) _Result_nullonfailure_ IID **iids) override
        {
            return m_pControllingUnknown->GetIids(iidCount, iids);
        }

    public:

       
        
        // ----------------------------------------------------------------------------------------
        //
        // static AggregableComObject::CreateInstance
        //
        // Synopsis:
        //      Although TBASE is expected to derive from RuntimeClass, we're not following the
        //      standard pattern required by wrl::Make and wrl::MakeAndInitialize because the 
        //      instantiation and initialization of an aggregable object need to be tighly 
        //      controlled to avoid difficult-to-diagnose issues. For example, MakeAndInitialize<T,I>
        //      does a QI for interface I after instantiating the new object. This QI will be
        //      delegated to the outer, which has not yet set up it's composable base pointers,
        //      and hence the call will fail with E_NOINTERFACE.
        //
        //      This static factory method allocates space for and initializes an 
        //      AggregableComObject using a slight variation of the MakeAndInitialize code to 
        //      avoid these pitfalls.
        //
        // ----------------------------------------------------------------------------------------
        static _Check_return_ HRESULT CreateInstance(
            _In_opt_ IInspectable* pOuter, 
            _COM_Outptr_ IInspectable **ppInner)
        {
            void* buffer = nullptr;
            AggregableComObject<TBASE>* aggregableObj = nullptr;
            wrl::Details::MakeAllocator<AggregableComObject<TBASE>> allocator;
            
            IFCPTRRC_RETURN(ppInner, E_INVALIDARG);
            *ppInner = nullptr;

            buffer = allocator.Allocate();
            IFCOOM_RETURN(buffer);

            aggregableObj = new (buffer)AggregableComObject<TBASE>();
            IFC_RETURN(aggregableObj->InitializeWithOuter(pOuter, ppInner));

            allocator.Detach();

            return S_OK;
        }

        static _Check_return_ HRESULT CreateInstance(
            _In_opt_ IInspectable* pOuter,
            _COM_Outptr_opt_ IInspectable **ppInner,
            _COM_Outptr_ TINTERFACE **ppInstance)
        {
            wrl::ComPtr<IInspectable> spInner;
            wrl::ComPtr<TINTERFACE> spInstance;

            if (ppInner)
            {
                *ppInner = nullptr;
            }

            IFCPTRRC_RETURN(ppInstance, E_INVALIDARG);
            *ppInstance = nullptr;

            IFC_RETURN(AggregableComObject<TBASE>::CreateInstance(pOuter, &spInner));

            IFC_RETURN(spInner.As(&spInstance));

            if (ppInner)
            {
                *ppInner = spInner.Detach();
            }

            *ppInstance = spInstance.Detach();

            return S_OK;
        }

    private:

        IInspectable* m_pControllingUnknown{};
        bool m_isComposed{};
    };


}
