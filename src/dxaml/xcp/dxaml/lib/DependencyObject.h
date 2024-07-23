// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSourceNoThreadId.h"
#include "BaseValueSource.h"
#include "InheritanceContextChangeKind.h"
#include "SetValueParams.h"
#include "comTemplateLibrary.h" // uses BetterAggregableAbstractCoreObjectActivationFactory
#include "InterfaceForwarder.h"
#include <SimpleProperties.h>
#include <InputActivationBehavior.h>

struct PropertyChangedParams;
enum class KnownPropertyIndex : UINT16;
enum class KnownTypeIndex : UINT16;
enum ValueOperation;
enum class DeferredElementStateChange;
namespace Theming {
    enum class Theme : uint8_t;
}

namespace DirectUI
{
    class Binding;
    class DPChangedEventSource;
    class BindingExpressionBase;
    class InheritanceContextChangedEventSource;
    class ContentPresenter;
    class Panel;
    class FrameworkElement;
    class EffectiveValueEntry;
    class IDispatcher;
    class DependencyObject;
    class Control;
    interface IDPChangedEventSource;
    interface IInheritanceContextChangedEventSource;
    interface ICorePropertyChangedEventHandler;
    interface ICorePropertyChangedEventArgs;
    interface IUntypedEventSource;

    template <typename T> struct NeedsBoxerBuffer;

    struct PropertyChangedEventArgsParam
    {
        PropertyChangedEventArgsParam() :
            nPropertyIndex(KnownPropertyIndex::UnknownType_UnknownProperty),
            pOldValueNoRef(NULL),
            pNewValueNoRef(NULL)
        {
        }

        // No old/new values are provided.
        PropertyChangedEventArgsParam(_In_ KnownPropertyIndex index) :
            nPropertyIndex(index),
            pOldValueNoRef(NULL),
            pNewValueNoRef(NULL)
        {
        }

        // New value is provided only.
        PropertyChangedEventArgsParam(_In_ KnownPropertyIndex index, _In_ IInspectable* pValue) :
            nPropertyIndex(index),
            pOldValueNoRef(NULL),
            pNewValueNoRef(pValue)
        {
        }

        // Old/New value are both provided.
        PropertyChangedEventArgsParam(_In_ KnownPropertyIndex index, _In_ IInspectable* pOldValue, _In_ IInspectable* pNewValue) :
            nPropertyIndex(index),
            pOldValueNoRef(pOldValue),
            pNewValueNoRef(pNewValue)
        {
        }

        KnownPropertyIndex nPropertyIndex;
        IInspectable* pOldValueNoRef;
        IInspectable* pNewValueNoRef;
    };
}

namespace DirectUI
{
    //+--------------------------------------------------------------------------------
    //
    //  DependencyObject
    //
    //+--------------------------------------------------------------------------------
    template <class TSOURCE, class THANDLER, class TARGS> class CEventSource;
    template <class TSOURCE, class THANDLER, class TARGS> class CRoutedEventSource;

    class __declspec(uuid("0c881986-86e1-40b3-9642-9e5090a8a6b0")) DependencyObject:
        public xaml::IDependencyObject,
        public ctl::WeakReferenceSourceNoThreadId,
        public ctl::forwarder_holder<::ITrackerOwner, DependencyObject>
    {
        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.DependencyObject");

        BEGIN_INTERFACE_MAP(DependencyObject, ctl::WeakReferenceSourceNoThreadId)
            INTERFACE_ENTRY(DependencyObject, xaml::IDependencyObject)
            INTERFACE_ENTRY(DependencyObject, ::ITrackerOwner)
        END_INTERFACE_MAP(DependencyObject, ctl::WeakReferenceSourceNoThreadId)

        // Grant the TemplateBindingExpression class friend access so it can
        // call SetValueExpression from its SetTemplateBinding callback.
        friend class TemplateBindingExpression;
        friend class ThemeResourceExpression;
        friend class CustomDependencyProperty;

        // Grant Control class friend so Control::GetAsControlNoRef can access
        // m_bCastedAsControl and m_pThisAsControlNoRef
        friend class Control;

    protected:
        // Constructors/destructors.
        DependencyObject();
        ~DependencyObject() override;

        _Check_return_ HRESULT Initialize(CDependencyObject* pDO);

        // Prepares DO's state.
        // TODO: This method needs to be made non-virtual. Derived DOs should override PrepareState() instead to avoid
        // re-entrancy through the peer table.
        _Check_return_ HRESULT Initialize() override;

    public:

        // Utility to wrap constructors as function pointers in the static
        // type tables defined in DXamlTypeTable.g.h.  It will call the constructor.
        static _Check_return_ HRESULT CreateNonAggregableDO(
            DependencyObject* (*creatorfunc)(IInspectable* /* pOuter */),
            _In_ CDependencyObject* pCoreDO,
            _In_opt_ IInspectable* pOuter,
            _Out_ IInspectable** ppNewInstance)
        {
            HRESULT hr = S_OK;
            DependencyObject* pDO = nullptr;

            IFCEXPECT(pCoreDO != nullptr);
            IFCEXPECT(pOuter == nullptr);

            pDO = (*creatorfunc)(nullptr /* pOuter */);
            IFC(pDO->Initialize(pCoreDO));
            *ppNewInstance = ctl::as_iinspectable(pDO);
            pDO = NULL;

        Cleanup:
            ctl::release_interface(pDO);
            RRETURN(hr);
        }

        static _Check_return_ HRESULT CreateAggregableDO(
            DependencyObject* (*creatorfunc)(IInspectable* /* pOuter */),
            _In_ CDependencyObject* pCoreDO,
            _In_opt_ IInspectable* pOuter,
            _Out_ IInspectable** ppNewInstance)
        {
            HRESULT hr = S_OK;
            DependencyObject* pDO = nullptr;

            IFCEXPECT(pCoreDO != nullptr);
            IFCPTR(ppNewInstance);

            if (pOuter != nullptr)
            {
                pDO = (*creatorfunc)(pOuter);
                IFC(pDO->Initialize(pCoreDO));
                *ppNewInstance = ctl::as_iinspectable(pDO);
                pDO = nullptr;
            }
            else
            {
                IFC(CreateNonAggregableDO(creatorfunc, pCoreDO, nullptr, ppNewInstance));
            }

        Cleanup:
            ctl::release_interface(pDO);
            RRETURN(hr);
        }

        // Verify that we're executing on the thread this DO belongs to.
        _Check_return_ HRESULT CheckThread() const final;

        DWORD GetThreadID() const;

        IDXamlCore* GetCoreForObject() override;

        // Event type defs.
        typedef CEventSource<ICorePropertyChangedEventHandler,
                    IInspectable,
                    ICorePropertyChangedEventArgs> CorePropertyChangedEventSourceType;

        typedef CEventSource<xaml::IDependencyPropertyChangedCallback,
                    xaml::IDependencyObject,
                    xaml::IDependencyProperty> CorePropertyChangedCallbackEventSourceType;

        // IDependencyObject
        IFACEMETHOD(SetValue)(_In_ xaml::IDependencyProperty* pDP, _In_ IInspectable* pValue) override;
        IFACEMETHOD(GetValue)(_In_ xaml::IDependencyProperty* pDP, _Out_ IInspectable** ppValue) override;
        IFACEMETHOD(ClearValue)(_In_ xaml::IDependencyProperty* pDP) override;
        IFACEMETHOD(ReadLocalValue)(_In_ xaml::IDependencyProperty* pDP, _Out_ IInspectable** ppValue) override;
        IFACEMETHOD(GetAnimationBaseValue)(_In_ xaml::IDependencyProperty* pDP, _Out_ IInspectable** ppValue) override;
        IFACEMETHOD(get_Dispatcher)(__RPC__deref_out_opt wuc::ICoreDispatcher** ppValue) override;
        IFACEMETHOD(get_DispatcherQueue)(__RPC__deref_out_opt msy::IDispatcherQueue** ppValue) override;
        IFACEMETHOD(RegisterPropertyChangedCallback)(_In_ xaml::IDependencyProperty *pdp, _In_ xaml::IDependencyPropertyChangedCallback *handler, _Out_ INT64 *token) override;
        IFACEMETHOD(UnregisterPropertyChangedCallback)(_In_ xaml::IDependencyProperty *pdp, _In_ INT64 token) override;

        virtual _Check_return_ HRESULT PrepareState() { return S_OK; };

    public:
        bool ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false) override;
        void OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore) override;

    protected:
        // Overrides from ComBase
        void OnFinalRelease() override;
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;

    public:
        BOOLEAN AllowResurrection();

    public:
        CDependencyObject* GetHandle() const;
        CDependencyObject* GetHandleAddRef();

        _Check_return_ HRESULT GetFocusedElement(_Outptr_result_maybenull_ DependencyObject** ppFocusedElement);

        void UpdatePegWithPossibleShutdownException(bool peg, BOOLEAN fShutdownException = FALSE);

        bool ImplicitPegAllowed() final
        {
            return true;
        }

        void BeginShutdown();
        _Check_return_ HRESULT EndShutdown() final;

        KnownTypeIndex GetTypeIndex() const override;

        _Check_return_ HRESULT GetValueCore(_In_ const CDependencyProperty* pDP,  CValue& pValue);
        _Check_return_ HRESULT SetValueCore(_In_ const SetValueParams& args, _In_ bool fSetEffectiveValueOnly = false);
        _Check_return_ HRESULT SetValueCore(_In_ const CDependencyProperty* pDP, _In_ const CValue& value)
        {
            return SetValueCore(SetValueParams(pDP, value));
        }
        _Check_return_ HRESULT SetValueCore(_In_ const CDependencyProperty* pDP, _In_opt_ IInspectable* pValue)
        {
            return SetValueCore(pDP, pValue, false);
        }
        _Check_return_ HRESULT SetValueCore(_In_ const CDependencyProperty* dp, _In_opt_ IInspectable* value, _In_ bool setEffectiveValueOnly, _In_ ::BaseValueSource baseValueSource = ::BaseValueSourceUnknown);

        #pragma region Generated code helpers.
        template<class T>
        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ T** ppValue)
        {
            RRETURN(GetValueByKnownIndex(nPropertyIndex, __uuidof(T), reinterpret_cast<void**>(ppValue)));
        }

        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_ REFIID iid, _Out_ void** ppValue);

        // For IInspectable return values, avoid the QI for IInspectable. Every interface is an IInspectable already.
        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ IInspectable** ppValue);

        template<class T>
        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ wf::IReference<T>** ppValue)
        {
            ARG_VALIDRETURNPOINTER(ppValue);
            IFC_RETURN(CheckThread());

            CValue boxedValue;
            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(DependencyObject::GetValueCore(pProperty, boxedValue));
            IFC_RETURN(CValueBoxer::UnboxValue<T>(&boxedValue, ppValue));

            return S_OK;
        }

        template<class T>
        _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ T* pValue)
        {
            ARG_VALIDRETURNPOINTER(pValue);
            IFC_RETURN(CheckThread());

            CValue boxedValue;
            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(GetValueCore(pProperty, boxedValue));
            IFC_RETURN(CValueBoxer::UnboxValue(&boxedValue, pValue));

            return S_OK;
        }

        template<class T>
        _Check_return_ typename std::enable_if<std::is_enum<T>::value && sizeof(T) == sizeof(UINT), HRESULT>::type GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ T* pValue)
        {
            ARG_VALIDRETURNPOINTER(pValue);
            IFC_RETURN(CheckThread());

            CValue boxedValue;
            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(GetValueCore(pProperty, boxedValue));
            IFC_RETURN(CValueBoxer::UnboxEnumValue(&boxedValue, pProperty->GetPropertyType(), reinterpret_cast<UINT*>(pValue)));

            return S_OK;
        }

        // For private-compact enums.
        template<class T>
        _Check_return_ typename std::enable_if<std::is_enum<T>::value && sizeof(T) == sizeof(uint8_t), HRESULT>::type GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ T* pValue)
        {
            ARG_VALIDRETURNPOINTER(pValue);
            IFC_RETURN(CheckThread());

            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);

            CValue boxedValue;
            IFC_RETURN(GetValueCore(pProperty, boxedValue));

            UINT temp = static_cast<UINT>(*pValue);
            IFC_RETURN(CValueBoxer::UnboxEnumValue(&boxedValue, pProperty->GetPropertyType(), &temp));
            *pValue = static_cast<T>(temp);

            return S_OK;
        }

        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ HSTRING* pValue);

        template<class T>
        _Check_return_ typename std::enable_if<!ctl::IsComObject<T>::value, HRESULT>::type SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ T* pValue)
        {
            RRETURN(SetValueByKnownIndex(nPropertyIndex, static_cast<IInspectable*>(pValue)));
        }

        template<class T>
        _Check_return_ typename std::enable_if<ctl::IsComObject<T>::value, HRESULT>::type SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ T* pValue)
        {
            RRETURN(SetValueByKnownIndex(nPropertyIndex, ctl::iinspectable_cast(pValue)));
        }
        _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ IInspectable* ppValue);

        template<class T>
        _Check_return_ typename std::enable_if<!std::is_enum<T>::value && NeedsBoxerBuffer<T>::value, HRESULT>::type SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_ T value)
        {
            IFC_RETURN(CheckThread());

            BoxerBuffer buffer;
            CValue boxedValue;
            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value, &buffer));
            IFC_RETURN(DependencyObject::SetValueCore(pProperty, boxedValue));

            return S_OK;
        }

        template<class T>
        _Check_return_ typename std::enable_if<!std::is_enum<T>::value && !NeedsBoxerBuffer<T>::value, HRESULT>::type SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_ T value)
        {
            CValue boxedValue;

            IFC_RETURN(CheckThread());

            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
            IFC_RETURN(DependencyObject::SetValueCore(pProperty, boxedValue));

            return S_OK;
        }

        template<class T>
        _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_ T value)
        {
            CValue boxedValue;

            IFC_RETURN(CheckThread());

            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
            IFC_RETURN(CValueBoxer::BoxEnumValue(&boxedValue, static_cast<XUINT32>(value)));
            IFC_RETURN(DependencyObject::SetValueCore(pProperty, boxedValue));

            return S_OK;
        }

        _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ HSTRING value);

        template<class T>
        static _Check_return_ HRESULT GetAttachedValueByKnownIndex(_In_ DependencyObject* pElement, _In_ KnownPropertyIndex nPropertyIndex, _Out_ T* pValue)
        {
            HRESULT hr = S_OK;
            ARG_NOTNULL(pElement, "element");
            IFC(pElement->GetValueByKnownIndex(nPropertyIndex, pValue));
        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT ClearValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex);

        template<class T>
        static _Check_return_ HRESULT SetAttachedValueByKnownIndex(_In_ DependencyObject* pElement, _In_ KnownPropertyIndex nPropertyIndex, _In_ T value)
        {
            HRESULT hr = S_OK;
            ARG_NOTNULL(pElement, "element");
            IFC(pElement->SetValueByKnownIndex(nPropertyIndex, value));
        Cleanup:
            RRETURN(hr);
        }

        #pragma endregion

        // Internal SetValue/GetValue/ClearValue methods
        virtual _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty* pProperty, _Out_ IInspectable** ppValue);
        _Check_return_ HRESULT GetValueByKnownIndex(_In_ KnownPropertyIndex ePropertyIndex,  CValue& value);
        _Check_return_ HRESULT SetValue(_In_ const CDependencyProperty* pProperty, _In_ IInspectable* pValue);
        _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex ePropertyIndex, _In_ const CValue& value);
        _Check_return_ HRESULT ClearValue(_In_ const CDependencyProperty* pDP);
        _Check_return_ HRESULT ReadLocalValue(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue);
        _Check_return_ HRESULT GetAnimationBaseValue(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue);
        _Check_return_ HRESULT RegisterPropertyChangedCallback(_In_ const CDependencyProperty *pdp, _In_ xaml::IDependencyPropertyChangedCallback *callback, _Out_ INT64 *token);
        _Check_return_ HRESULT UnregisterPropertyChangedCallback(_In_ const CDependencyProperty *pdp, _In_ const INT64 token);

        // Get the XAML dispatcher associated with this DO - callable from any thread
        IDispatcher* GetXamlDispatcherNoRef();
        _Check_return_ HRESULT GetXamlDispatcher(_Out_ ctl::ComPtr<IDispatcher>* pspDispatcher);

    public:
        // Internal SetBinding
        _Check_return_ HRESULT SetBindingCore(_In_ const CDependencyProperty* pProperty, _In_ Binding* pBinding);
        _Ret_maybenull_ EffectiveValueEntry* TryGetEffectiveValueEntry(_In_ KnownPropertyIndex nPropertyIndex);

        _Check_return_ HRESULT GetDefaultValueInternal(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue);

        _Check_return_ HRESULT ClearCorePropertyExpression(_In_ KnownPropertyIndex nPropertyIndex);
        _Check_return_ HRESULT ClearCorePropertyThemeResourceExpression(_In_ const CDependencyProperty* pDP);
        _Check_return_ HRESULT DetachExpression(_In_ const CDependencyProperty* pDP, _In_ BindingExpressionBase* pExpression);
        _Check_return_ HRESULT RefreshExpression(_In_ const CDependencyProperty* pDP);
        _Check_return_ HRESULT SetValueInternal(_In_ const CDependencyProperty* pDP, _In_ IInspectable* pValue, _In_ bool fAllowReadOnly, _In_::BaseValueSource baseValueSource = ::BaseValueSourceUnknown);
        _Check_return_ HRESULT UpdateEffectiveValue(_In_ const CDependencyProperty* pDP, _In_opt_ EffectiveValueEntry* pValueEntry, _In_ ValueOperation valueOperation, _In_ ::BaseValueSource baseValueSource, _In_opt_ IInspectable* pCorePropertyNewValue = NULL);
        _Check_return_ HRESULT EvaluateEffectiveValue(_In_ const CDependencyProperty* pDP, _Inout_ EffectiveValueEntry* pValueEntry, _In_ ValueOperation valueOperation);
        _Check_return_ HRESULT EvaluateBaseValue(_In_ const CDependencyProperty* pDP, _Inout_ EffectiveValueEntry* pValueEntry, _In_ ValueOperation valueOperation);
        _Check_return_ HRESULT TryProcessThemeResourceBaseValue(
            _In_ const CDependencyProperty* pDP,
            _In_ EffectiveValueEntry* pValueEntry,
            _In_opt_ IInspectable* pBaseValue,
            BaseValueSource baseValueSource,
            _Out_ bool *pProcessed);

        bool IsInLiveTree() const;

        // Check to see if the property is set locally
        _Check_return_ HRESULT IsPropertyLocal(_In_ const CDependencyProperty* pDP, _Out_ BOOLEAN* pfIsLocal);

        // Property changed event
        // NOTE: This is the sync event raised from calling SetValue on the DP
        _Check_return_ HRESULT GetDPChangedEventSource(_Out_ IDPChangedEventSource** ppEventSource);

        // These variants will not create the event sure if one is already not there
        _Check_return_ HRESULT TryGetDPChangedEventSource(_Out_ IDPChangedEventSource** ppEventSource);

        // InheritanceContextChanged event
        _Check_return_ HRESULT GetInheritanceContextChangedEventSource(_Outptr_ IInheritanceContextChangedEventSource** ppEventSource);

        _Check_return_ HRESULT FireEvent(KnownEventIndex nEventId, IInspectable* pSender, IInspectable* pArgs);

        // ExternalObjectReference is special
        virtual bool IsExternalObjectReference() { return false; }

        // Mentor functionality
        _Check_return_ HRESULT GetMentor(_Outptr_ FrameworkElement** ppMentor);

        static BOOLEAN IsCollection(KnownTypeIndex index);

        // Tree change notifications
        virtual _Check_return_ HRESULT OnParentUpdated(
            _In_opt_ CDependencyObject* pOldParentCore,
            _In_opt_ CDependencyObject* pNewParentCore,
            _In_ bool isNewParentAlive);

        virtual _Check_return_ HRESULT OnTreeParentUpdated(_In_opt_ CDependencyObject* pNewParent, BOOLEAN isParentAlive) { return S_OK; }

        virtual _Check_return_ HRESULT OnCollectionChanged(_In_ XUINT32 nCollectionChangeType, _In_ XUINT32 nIndex)  { return S_OK; }

        _Check_return_ HRESULT NotifyInheritanceContextChanged(_In_ InheritanceContextChangeKind kind = InheritanceContextChangeKind::Default);
        virtual _Check_return_ HRESULT OnInheritanceContextChanged();

        bool GetHasState() { return m_bHasState; }

        // Break reference cycles between peers and parent/children.
        void Deinitialize() override;

        // Indicates if the given dependency object is a child of this object.
        _Check_return_ HRESULT IsAncestorOf(
            _In_ DependencyObject* pElement,
            _Out_ BOOLEAN* pIsAncestor);

    protected:
        virtual _Check_return_ HRESULT DisconnectFrameworkPeerCore();

        // If possible, use CDependencyProperty::GetDefaultValue instead to supply default values for a built-in DP.
        virtual _Check_return_ HRESULT GetDefaultValue2(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue) { return E_NOTIMPL; }

        _Check_return_ HRESULT GetDefaultValueBase(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue);

        // Note: please think twice before overriding these methods and adding work
        // In the past, way too much work has crept into these methods, slowing things down
        // Usually, the Loaded event is the more appropriate place
        virtual _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding) { RRETURN(S_OK); }

        virtual _Check_return_ HRESULT LeaveImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset) { RRETURN(S_OK); }

        void OnReferenceTrackerWalk(INT walkType) override;

    private:
        void ClearPeerReferences();

        _Ret_notnull_ EffectiveValueEntry* CreateEffectiveValueEntry(_In_ KnownPropertyIndex propertyIndex);
        void RemoveEffectiveValueEntryIfExists(_In_ KnownPropertyIndex nPropertyIndex);

        _Check_return_ HRESULT ClearEffectiveValueEntryExpression(_In_ EffectiveValueEntry* pValueEntry);

        _Check_return_ HRESULT DisconnectFrameworkPeer(bool fFinalRelease);

        _Check_return_ HRESULT RaiseDPChanged(_In_ const CDependencyProperty* pDP);

        _Check_return_ HRESULT RefreshExpression_Helper(_In_ const CDependencyProperty* pDP, _In_ EffectiveValueEntry* pValueEntry);

    public:
        static _Check_return_ HRESULT NotifyPropertyChanged(
            _In_ CDependencyObject* pDO,
            _In_ const PropertyChangedParams& args);

        static _Check_return_ HRESULT GetDefaultValueCallback(
            _In_ CDependencyObject* pReferenceObject,
            _In_ const CDependencyProperty* pDP,
            _Out_ CValue* pValue);

        static _Check_return_ HRESULT SetBindingCallback(
            _In_ CDependencyObject* pTarget,
            _In_ KnownPropertyIndex propertyId,
            _In_ CDependencyObject* pBinding);

        static _Check_return_ HRESULT AddPeerReferenceToItemCallback(
            _In_ CDependencyObject* nativeSource,
            _In_ CDependencyObject* nativeTarget);

        static _Check_return_ HRESULT RemovePeerReferenceToItemCallback(
            _In_ CDependencyObject* nativeSource,
            _In_ CDependencyObject* nativeTarget);

        static _Check_return_ HRESULT SetPeerReferenceToPropertyCallback(
            _In_ CDependencyObject* nativeTarget,
            _In_ const CDependencyProperty* pDP,
            _In_ const CValue& value,
            _In_ bool bPreservePegNoRef = false,
            _In_opt_ IInspectable* pNewValueOuter = nullptr,
            _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter = nullptr);

        static _Check_return_ HRESULT EnterImpl(
            _In_ CDependencyObject* nativeDO,
            _In_ CDependencyObject* nativeNamescopeOwner,
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding);

        static _Check_return_ HRESULT LeaveImpl(
            _In_ CDependencyObject* nativeDO,
            _In_ CDependencyObject* nativeNamescopeOwner,
            _In_ bool bLive, _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset);

        static _Check_return_ HRESULT OnCollectionChangedCallback(
            _In_ CDependencyObject* nativeObject,
            _In_ XUINT32 nCollectionChangeType,
            _In_ XUINT32 nIndex);

        static _Check_return_ HRESULT SetFocusedElement(
            _In_ DependencyObject* pFocusedElement,
            _In_ xaml::FocusState focusState,
            _In_ BOOLEAN animateIfBringIntoView,
            _Out_ BOOLEAN* pFocusUpdated,
            _In_ bool isProcessingTab = false,
            _In_ bool isShiftPressed = false,
            _In_ bool forceBringIntoView = false);

        static _Check_return_ HRESULT SetFocusedElementWithDirection(
            _In_ DependencyObject* pFocusedElement,
            _In_ xaml::FocusState focusState,
            _In_ BOOLEAN animateIfBringIntoView,
            _Out_ BOOLEAN* pFocusUpdated,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
            _In_ bool forceBringIntoView = false,
            InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation); // default to request activation to match legacy behavior
        

        static _Check_return_ HRESULT OnParentUpdated(
            _In_ CDependencyObject* pChildCore,
            _In_opt_ CDependencyObject* pOldParentCore,
            _In_opt_ CDependencyObject* pNewParentCore,
            _In_ bool isNewParentAlive);

        static _Check_return_ HRESULT ReferenceTrackerWalk(
            _In_ CDependencyObject* pCoreDO,
            _In_ DirectUI::EReferenceTrackerWalkType walkType,
            _In_ bool isRoot,
            _Out_ bool *pIsPeerAlive,
            _Out_ bool *pWalked);

        static _Check_return_ HRESULT SetExpectedReferenceOnPeer(
            _In_ CDependencyObject* pCoreDO);

        static _Check_return_ HRESULT ClearExpectedReferenceOnPeer(
            _In_ CDependencyObject* pCoreDO);

        static _Check_return_ HRESULT RefreshExpressionsOnThemeChange(_In_ CDependencyObject* pCoreDO, _In_ Theming::Theme theme, _In_ bool forceRefresh);

    protected:
        bool ShouldRaiseEvent(KnownEventIndex eventID);
        _Check_return_ HRESULT EventAddPreValidation(_In_ void* const pValue, EventRegistrationToken* const ptToken) const;
        _Check_return_ HRESULT GetEventSourceNoRefWithArgumentValidation(KnownEventIndex nEventIndex, _Outptr_ IUntypedEventSource** ppEventSource);
        _Ret_maybenull_ IUntypedEventSource* GetEventSourceNoRef(KnownEventIndex nEventIndex);
        _Check_return_ HRESULT StoreEventSource(KnownEventIndex nEventIndex, _In_ IUntypedEventSource* pEventSource);
        _Check_return_ HRESULT RemoveEventSource(KnownEventIndex nEventIndex);
        _Check_return_ HRESULT MoveEventSources(_In_ DependencyObject* pTarget);
        _Check_return_ HRESULT RestoreEventSources(_In_ DependencyObject* pSource);

    private:
        _Check_return_ HRESULT MoveEventSourcesImpl(_In_ DependencyObject* pTarget);

    private:
        // Peer references representing core property values
        std::unique_ptr<std::unordered_map<KnownPropertyIndex, TrackerPtr<IInspectable> > > m_pPropertyValueReferences;

        // Peer references representing the core tree and collection itmes
        std::unique_ptr<std::list<DependencyObject*>> m_pItemReferences;

        // Caching this to reduce repetitive and expensive QI casts into the CLR, since if a DO is
        // a Control that fact won't change for the lifetime of the object
        Control* m_pThisAsControlNoRef = nullptr;

        // Mapping of EventSources for per-instance/per-property callbacks
        typedef std::pair<KnownPropertyIndex, ctl::ComPtr<CorePropertyChangedCallbackEventSourceType>> NotificationVectorEntry;
        std::unique_ptr<std::vector<NotificationVectorEntry>> m_pNotificationVector;

        typedef containers::vector_map<KnownEventIndex, IUntypedEventSource*> EventMapping;
        std::unique_ptr<EventMapping> m_pEventMap;

        union
        {
            XUINT32 m_uThreadId;
            CDependencyObject* m_pDO;
        };

        // Mapping of dependency property to effective value
        typedef containers::vector_map<KnownPropertyIndex, std::unique_ptr<EffectiveValueEntry>> EffectiveValueStore;
        std::unique_ptr<EffectiveValueStore> m_pMapValueTable;

#if DBG
        ULONG m_ulPegRefCountShutDownException;
#endif

    private:
        DPChangedEventSource* m_pDPChangedEventSource;
        InheritanceContextChangedEventSource* m_pInheritanceContextChangedEventSource;

    private:
        // See comment on StoreM3PeerReferenceToObject
        // Silverlight#101268: Remove these after all types are added at the end of M3

        std::list<CDependencyObject*> *m_pM3Parents;
        _Check_return_ HRESULT StoreM3PeerReferenceToObject(CDependencyObject *parentElement);
        _Check_return_ HRESULT RemoveM3PeerReferenceToObject(CDependencyObject *parentElement);

    protected:
        _Check_return_ HRESULT SetValueExpression(_In_ const CDependencyProperty* dp, _In_ BindingExpressionBase* expression, _In_ ::BaseValueSource baseValueSource = ::BaseValueSourceUnknown);

        // This is the new property changed method.
        virtual _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args);

        _Check_return_ HRESULT EnsureReferenceStore();
        _Check_return_ HRESULT EnsureItemReferenceStore();

        _Check_return_ HRESULT GetReference(_In_ const CDependencyProperty* pProperty, _Outptr_ DependencyObject** ppObject);

    private:
        _Check_return_ HRESULT NotifyPropertyChanged(_In_ const PropertyChangedParams& args);

    public:
        _Check_return_ HRESULT StorePeerPropertyReferenceToObject(
            _In_ const CDependencyProperty* pProperty,
            _In_ IInspectable* pObject,
            _In_ bool bPreservePegNoRef,
            _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter = nullptr);

        _Check_return_ HRESULT StorePeerItemReferenceToObject(_In_ DependencyObject *pObject);
        _Check_return_ HRESULT ClearPeerItemReferenceToObject(_In_ DependencyObject *pObject);

        virtual _Check_return_ HRESULT OnChildUpdated(_In_ DependencyObject *pChild) { return S_OK; };
        _Check_return_ HRESULT MarkHasState();

    private:
        // Special AddRef/Release that takes composition into account for reference tracking
        void AddRefForPeerReferenceHelper();
        void ReleaseForPeerReferenceHelper();

    protected:
        // Helper forwarder to AddTrackingRef()
        void AddRefForPeerReference( DependencyObject *pDO )
        {
            pDO->AddRefForPeerReferenceHelper();
        }

        //  Helper forwarder to ReleaseTrackingRef()
        void ReleaseForPeerReference(DependencyObject *&pDO)
        {
            if( pDO != NULL )
            {
                pDO->ReleaseForPeerReferenceHelper();
                pDO = NULL;
            }
        }

    public:
        static _Check_return_ HRESULT NotifyDeferredElementStateChangedStatic(
            _In_ CDependencyObject* parent,
            _In_ KnownPropertyIndex propertyIndex,
            _In_ DeferredElementStateChange state,
            _In_ UINT32 collectionIndex,
            _In_ CDependencyObject* realizedElement);

    protected:
        virtual _Check_return_ HRESULT NotifyDeferredElementStateChanged(
            _In_ KnownPropertyIndex propertyIndex,
            _In_ DeferredElementStateChange state,
            _In_ UINT32 collectionIndex,
            _In_ CDependencyObject* realizedElement);

        template <typename SenderType, typename EventSourceType>
        static _Check_return_ HRESULT RaiseSimplePropertyChangedNotification(
            SimpleProperty::objid_t obj,
            KnownEventIndex eventId,
            _In_opt_ IInspectable* args)
        {
            CDependencyObject* coreObj = SimpleProperty::MapObjIdToInstance<CDependencyObject>(obj);
            DependencyObject* fxDO = coreObj->GetDXamlPeer();

            if (fxDO)
            {
                EventSourceType* eventSource = static_cast<EventSourceType*>(fxDO->GetEventSourceNoRef(eventId));

                if (eventSource)
                {
                    ctl::ComPtr<SenderType> senderTyped;
                    IFC_RETURN(ctl::do_query_interface(senderTyped, fxDO));
                    IFC_RETURN(eventSource->Raise(senderTyped.Get(), args));
                }
            }

            return S_OK;
        }
    };

    class DependencyObjectFactory :
        public xaml::IDependencyObjectFactory,
        public ctl::BetterAggregableAbstractCoreObjectActivationFactory
    {
        BEGIN_INTERFACE_MAP(DependencyObjectFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)
            INTERFACE_ENTRY(DependencyObjectFactory, xaml::IDependencyObjectFactory)
        END_INTERFACE_MAP(DependencyObjectFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)

    public:
        IFACEMETHOD(CreateInstance)(
            _In_ IInspectable* pOuter,
            _Outptr_ IInspectable** ppInner,
            _Outptr_ xaml::IDependencyObject** ppInstance);

        static _Check_return_ HRESULT GetUseStrictPropertyStatic(_Out_ const CDependencyProperty** ppValue);

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override;
    };
}
