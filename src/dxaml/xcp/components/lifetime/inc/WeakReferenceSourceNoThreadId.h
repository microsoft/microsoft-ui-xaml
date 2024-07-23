// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComBase.h>
#include "ReferenceTrackerInterfaces.h"
#include "ReferenceTrackerExtension.h"
#include "ComposingTrackerWrapper.h"
#include "TrackerPtr.h"
#include "InterfaceForwarder.h"
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>

namespace DirectUI
{
    class DependencyObject;
}

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
#pragma region forwarders
    template<typename impl_type>
    class interface_forwarder<IWeakReferenceSource, impl_type>
        : public ctl::iunknown_forwarder_base<IWeakReferenceSource, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }
        IFACEMETHOD(GetWeakReference)(_COM_Outptr_ IWeakReference** weakReference) final { return This()->GetWeakReference(weakReference); }
    };

    template<typename impl_type>
    class interface_forwarder<xaml_hosting::IReferenceTrackerInternal, impl_type>
        : public ctl::iunknown_forwarder_base<xaml_hosting::IReferenceTrackerInternal, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }

        // IReferenceTracker methods
        IFACEMETHOD(ConnectFromTrackerSource)() final { return This()->ConnectFromTrackerSource(); }
        IFACEMETHOD(DisconnectFromTrackerSource)() final { return This()->DisconnectFromTrackerSource(); }
        IFACEMETHOD(FindTrackerTargets)(_In_::IFindReferenceTargetsCallback *callback) final { return This()->FindTrackerTargets(callback); }
        IFACEMETHOD(GetReferenceTrackerManager)(_Out_::IReferenceTrackerManager **value) final { return This()->GetReferenceTrackerManager(value); }
        IFACEMETHOD(AddRefFromTrackerSource)() final { return This()->AddRefFromTrackerSource(); }
        IFACEMETHOD(ReleaseFromTrackerSource)() final { return This()->ReleaseFromTrackerSource(); }
        IFACEMETHOD(PegFromTrackerSource)() final { return This()->PegFromTrackerSource(); }

        // IReferenceTrackerInternal methods
        void PrepareForReferenceWalking() final { return This()->PrepareForReferenceWalking(); }
        void OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore) final { return This()->OnReferenceTrackingProcessed(pCore); }
        void ClearReferenceTrackerPeg() final { return This()->ClearReferenceTrackerPeg(); }

        bool IsReferencedByTrackerSource() final { return This()->IsReferencedByTrackerSource(); }
        bool IsReferenceTrackerPegSet() final { return This()->IsReferenceTrackerPegSet(); }
        bool IsReachable() final { return This()->IsReachable(); }
        bool IsPegged(bool isRefCountPegged) final { return This()->IsPegged(isRefCountPegged); }
        bool IsNativeAndComposed() final { return This()->IsNativeAndComposed(); }

        bool HasBeenWalked(_In_ DirectUI::EReferenceTrackerWalkType walkType) final { return This()->HasBeenWalked(walkType); }

        void UpdatePeg(bool peg) final { return This()->UpdatePeg(peg); }

        ULONG GetRefCount(xaml_hosting::RefCountType refCountType) final { return This()->GetRefCount(refCountType); }
        void UpdateExpectedRefCount(xaml_hosting::RefCountUpdateType updateType) final { return This()->UpdateExpectedRefCount(updateType); }
        bool IsAlive() final { return This()->IsAlive(); }
        bool ImplicitPegAllowed() final { return This()->ImplicitPegAllowed(); }
        void ResetLastFindWalkId() final { return This()->ResetLastFindWalkId(); }

        bool ReferenceTrackerWalk(DirectUI::EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false) final { return This()->ReferenceTrackerWalk(walkType, fIsRoot); }

        _Check_return_ HRESULT NewTrackerPtrWrapper(_Outptr_ xaml_hosting::ITrackerPtrWrapper** ppValue) final { return This()->NewTrackerPtrWrapper(ppValue); }
        _Check_return_ HRESULT DeleteTrackerPtrWrapper(_In_ xaml_hosting::ITrackerPtrWrapper* pValue) final { return This()->DeleteTrackerPtrWrapper(pValue); }
        xaml_hosting::IReferenceTrackerGCLock* GetGCLock() final { return This()->GetGCLock(); }
        _Check_return_ HRESULT SetExtensionInterfaces(
            _In_ ::IReferenceTrackerExtension* extension,
            _In_ xaml_hosting::IReferenceTrackerInternal* overrides) final
        {
            return This()->SetExtensionInterfaces(extension, overrides);
        }

        void OnReferenceTrackerWalk(_In_ INT walkType) final { This()->OnReferenceTrackerWalk(walkType); }
#if DBG
        bool ShouldSkipTrackerLeakCheck() final { return This()->ShouldSkipTrackerLeakCheck(); }
        void ClearSkipTrackerLeakCheck() final { return This()->ClearSkipTrackerLeakCheck(); }
        void TouchDebugMemory() final { return This()->TouchDebugMemory(); }
#endif

        _Check_return_ HRESULT EndShutdown() final { return This()->EndShutdown(); }
    };

    template<typename impl_type>
    class interface_forwarder<::ITrackerOwner, impl_type> final
        : public ctl::iunknown_forwarder_base<::ITrackerOwner, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }
        IFACEMETHOD(CreateTrackerHandle)(_Out_ ::TrackerHandle *returnValue) override
        {
            return This()->CreateTrackerHandle(returnValue);
        }
        IFACEMETHOD(DeleteTrackerHandle)(_In_ ::TrackerHandle handle) override
        {
            return This()->DeleteTrackerHandle(handle);
        }
        IFACEMETHOD(SetTrackerValue)(
            _In_ ::TrackerHandle handle,
            _In_opt_ IUnknown *value) override
        {
            return This()->SetTrackerValue(handle, value);
        }
        _Success_(!!return) _Check_return_ IFACEMETHOD_(BOOLEAN, TryGetSafeTrackerValue)(
            _In_ ::TrackerHandle handle,
            _COM_Outptr_result_maybenull_ IUnknown **returnValue) override
        {
            return This()->TryGetSafeTrackerValue(handle, returnValue);
        }
    };
#pragma endregion

    /////////////////////////////////////
    //
    // WeakReferenceSourceNoThreadId
    //
    // Base class that provides:
    // * Basic Object Lifetime Support
    // * WeakReferenceSource
    // * SupportErrorInfo
    //
    // Use this class *only* if you don't require Thread affinity support.
    //
    /////////////////////////////////////
    class WeakReferenceSourceNoThreadId
        : public SupportErrorInfo
        , public ctl::forwarder_holder<IWeakReferenceSource, WeakReferenceSourceNoThreadId>
        , public ctl::forwarder_holder<xaml_hosting::IReferenceTrackerInternal, WeakReferenceSourceNoThreadId>
    {
    protected:
        WeakReferenceSourceNoThreadId();

        friend class ctl::interface_forwarder<xaml_hosting::IReferenceTrackerInternal, WeakReferenceSourceNoThreadId>;
        friend class ctl::interface_forwarder<::ITrackerOwner, DirectUI::DependencyObject>;

        #if DBG
        ~WeakReferenceSourceNoThreadId() override;
        #endif

        BEGIN_INTERFACE_MAP(WeakReferenceSourceNoThreadId, SupportErrorInfo)
            INTERFACE_ENTRY(WeakReferenceSourceNoThreadId, IWeakReferenceSource)
        END_INTERFACE_MAP(WeakReferenceSourceNoThreadId, SupportErrorInfo)

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void **ppObject) override;

    public:
        // IWeakReferenceSource methods
        _Check_return_ HRESULT GetWeakReference(_COM_Outptr_ IWeakReference **weakReference);
        void Resurrect();

        // Override to break reference cycles.
        virtual void Deinitialize() {}

        virtual _Check_return_ HRESULT EndShutdown();

        // IReferenceTracker methods
        _Check_return_ HRESULT GetReferenceTrackerManager(_Out_::IReferenceTrackerManager **value);
        _Check_return_ HRESULT FindTrackerTargets(_In_::IFindReferenceTargetsCallback *callback);
        _Check_return_ HRESULT ConnectFromTrackerSource();
        _Check_return_ HRESULT DisconnectFromTrackerSource();
        bool ReferenceTrackerWalkCore(DirectUI::EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false);
        virtual bool ReferenceTrackerWalk(DirectUI::EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false);

        _Check_return_ HRESULT AddRefFromTrackerSource();
        _Check_return_ HRESULT ReleaseFromTrackerSource();

        _Check_return_ HRESULT PegFromTrackerSource()
        {
            return S_OK;
        }

        // Lifetime Treewalk Helpers
        void PrepareForReferenceWalking();
        virtual void OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore);

        #if DBG
        // Debug aid:  Call this on an object instance to make it trace whenever it's walked (and to allow a bp to be set).
        bool m_breakOnWalk;
        void SetBreakOnWalk( bool breakOnWalk ) { m_breakOnWalk = breakOnWalk; }
        #endif

        // States used in Lifetime Tree Walk
        bool IsReferencedByTrackerSource();
        bool IsReferenceTrackerPegSet();
        bool IsReachable();
        bool IsPegged(bool isRefCountPegged);
        bool IsPeggedNoRef();
        bool HasBeenWalked(_In_ DirectUI::EReferenceTrackerWalkType walkType);

        // Pegging
        void SetReferenceTrackerPeg();
        void ClearReferenceTrackerPeg();
        void SetRefCountPeg();
        void ClearRefCountPeg();
        void UpdatePeg(bool peg);
        void PegNoRef();
        void UnpegNoRef( bool suppressClearReferenceTrackerPeg = false );

        //Check Thread (No affinity)
        virtual _Check_return_ HRESULT CheckThread() const { RRETURN(S_OK); }

        // Final Release
        bool OnFinalReleaseOffThread(_In_ bool allowOffThreadDelete = TRUE);
        virtual DirectUI::IDXamlCore* GetCoreForObject() { return NULL; }

        ULONG GetRefCount(xaml_hosting::RefCountType refCountType);
        void UpdateExpectedRefCount(xaml_hosting::RefCountUpdateType updateType);
        bool IsAlive();

        virtual bool ImplicitPegAllowed() { return false; }
        void ResetLastFindWalkId() { m_lastFindWalkID = 0; }

        bool IsNativeAndComposed()
        {
            return (DirectUI::ComposingTrackerExtensionWrapper::IsComposed(this) && !DirectUI::ComposingTrackerTargetWrapper::IsTrackerTarget(this));
        }

#if DBG
        bool ShouldSkipTrackerLeakCheck() { return m_bShouldSkipTrackerLeakCheck; }
        void  ClearSkipTrackerLeakCheck()  { m_bShouldSkipTrackerLeakCheck = FALSE; }

        // Do a write to a specific memory location to enable debugging the reference tracking work for a specific object (using memory breakpoints).
        void TouchDebugMemory() { m_pMemoryCheck = this; }
#endif

        template <typename T>
        void RemovePtrValue(_In_ DirectUI::TrackerPtr<T>& ptr)
        {
            UnregisterPtr(ptr.GetTrackerReference());
            ptr.Clear();
        }

        template <typename T, typename U>
        void SetPtrValue(_In_ DirectUI::TrackerPtr<T>& ptr, _In_ U* value
#if DBG
            , bool fStatic = false
#endif
            )
        {
            RegisterPtr(ptr.GetTrackerReference());
#if DBG
            ptr.Set(value, fStatic);
#else
            ptr.Set(value);
#endif
        }

        template <typename T, typename U>
        void SetPtrValue(_In_ DirectUI::TrackerPtr<T>& ptr, _In_ const ctl::ComPtr<U>& sp
#if DBG
            , bool fStatic = false
#endif
            )
        {
            RegisterPtr(ptr.GetTrackerReference());
#ifdef DBG
            ptr.Set(sp.Get(), fStatic);
#else
            ptr.Set(sp.Get());
#endif
        }

    protected:
        template <typename T, typename U>
        _Check_return_ HRESULT SetPtrValueWithQI(_In_ DirectUI::TrackerPtr<T>& ptr, _In_ U* value)
        {
            RegisterPtr(ptr.GetTrackerReference());
            IFC_RETURN(ptr.SetWithQI(value));

            return S_OK;
        }

        template <typename T, typename U>
        void SetPtrValueWithQIOrNull(_In_ DirectUI::TrackerPtr<T>& ptr, _In_ U* value)
        {
            RegisterPtr(ptr.GetTrackerReference());
            ptr.SetWithQIOrNull(value);
        }

        //
        // Portion of IReferenceTrackerInternal that MUXP calls.
        //
        _Check_return_ HRESULT NewTrackerPtrWrapper(_Outptr_ xaml_hosting::ITrackerPtrWrapper** ppValue);
        _Check_return_ HRESULT DeleteTrackerPtrWrapper(_In_ xaml_hosting::ITrackerPtrWrapper* pValue);
        xaml_hosting::IReferenceTrackerGCLock* GetGCLock();
        _Check_return_ HRESULT SetExtensionInterfaces(
            _In_ ::IReferenceTrackerExtension* extension,
            _In_ xaml_hosting::IReferenceTrackerInternal* overrides);

        //
        // Portion of IReferenceTrackerInternal that MUXP overrides.
        //
        virtual void OnReferenceTrackerWalk(_In_ INT walkType);

        void AddToReferenceTrackingList();

    protected:
        unsigned short m_ulPegRefCount : 14;        // Ref-counted pegs
        unsigned short m_bIsPeggedNoRef : 1;        // Has a NoRef peg
        unsigned short m_bReferenceTrackerPeg : 1;  // Temporary root used by ReferenceTrackerManager

        unsigned short m_lastFindWalkID; // ID of the last find walk that visited

#if DBG
        bool m_bOnReferenceTrackerWalked;
        void* m_pMemoryCheck;
        bool m_bShouldSkipTrackerLeakCheck;
#endif

        ULONG m_ulRefCountFromTrackerSource; // Number of tracker sources referencing this object

        ULONG m_ulExpectedRefCount; // The expected reference count of this object from Tree/RCWs

#ifdef DBG
        bool m_bQueuedForUnreachableCleanup;
#endif

        // Keep flags which are updated during ReferenceTrackerWalk separate from the above bit field,
        // to avoid race condition (where two threads are updating the same dword at the same time).
        struct ReferenceTrackerBitFields
        {
            bool bFindWalked : 1;           // Flag to indicate if DO has been the root of a RCW Walk
            bool bPegWalked : 1;            // Flag to indicate object has been visited for RTW_PEG
            bool bReachable : 1;            // Can be reached by a pegged DO or rooted tracker source
            bool bRefCountPeg : 1;          // Temporary root used by when expected ref count is not equal to actual ref count
            bool bEnsuredTrackerTarget : 1; // Flag to indicate that we already QI-ed for IReferenceTrackerTarget.
            bool bWeakReferenceDisconnected : 1;  // Indicates whether we disconnected the weak reference.
            bool AddedToReferenceTrackingList : 1; // Has been added to referenceTrackingList
            bool peggedByCoreTable : 1;     // Pegged because it's in core's m_PegNoRefCoreObjectsWithoutPeers
            bool MemoryDiagWalked : 1;   // Flag to indicate object has been visited for memory diagnostics (RTW_GetElementCount, RTW_TotalCompressedImageSize)
        } m_referenceTrackerBitFields;

        // These protected flags were moved out of DependencyObject to fit into the free padding here
        bool m_bIsDisconnected : 1;   // Disconnect
        bool m_bIsDisconnectedFromCore : 1;   // Disconnected from core DO
        bool m_bHasState : 1;         // DXaml peer is stateful, can't be re-created
        bool m_bCastedAsControl : 1;         // Whether or not we've tried to QI cast this DO as a Control

    public:
        // ITrackerOwner - These should only be called from the public API
        // Make these methods private after we move that interface from DependencyObject up to here.
        _Check_return_ HRESULT CreateTrackerHandle(_Out_ ::TrackerHandle *returnValue);
        _Check_return_ HRESULT DeleteTrackerHandle(_In_ ::TrackerHandle handle);
        _Check_return_ HRESULT SetTrackerValue(
            _In_ ::TrackerHandle handle,
            _In_opt_ IUnknown *value);
        _Check_return_ HRESULT GetTrackerValue(
            _In_::TrackerHandle handle,
            _COM_Outptr_result_maybenull_ IUnknown **returnValue);
        _Success_(!!return) _Check_return_ BOOLEAN TryGetSafeTrackerValue(
            _In_ ::TrackerHandle handle,
            _COM_Outptr_result_maybenull_ IUnknown **returnValue);

    private:
        void ClearWeakReference();

        void RegisterPtr(_In_ DirectUI::TrackerTargetReference* const pTrackerPtr);
        void UnregisterPtr(DirectUI::TrackerTargetReference* pTrackerPtr);

        _Check_return_ HRESULT EnsureCompositionWrapper();

        // We instantiate a composition wrapper only when needed (ie this object is composed by
        // a managed or MUXP instance). This is not the case for most instances and, in doing this,
        // we save a few bytes.
        std::unique_ptr<DirectUI::ComposingTrackerWrapper> m_compositionWrapper;

        friend class DirectUI::ComposingTrackerTargetWrapper;
        friend class DirectUI::ComposingTrackerExtensionWrapper;

        struct ReferenceTrackerGCLock final :
            public xaml_hosting::IReferenceTrackerGCLock
        {
            INT64 AcquireLock() override;
            void ReleaseLock(INT64 token) override;
        };

        std::unique_ptr<std::vector<DirectUI::TrackerTargetReference*>> m_trackers;

        static ReferenceTrackerGCLock s_refTrackerGCLock;
    };
}
