// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MinPal.h"
#include <unordered_map>
#include "Events.h"
#include "xstring_ptr.h"
#include "Indexes.g.h"
#include "XamlServiceProviderContext.h"
#include "DeclareMacros.h"
#include "CompositionRequirement.h"
#include "IndependentAnimationType.h"
#include "CreateGroupFn.h"
#include "DirtyFlags.h"
#include "BaseValueSource.h"
#include "EnterParams.h"
#include "LeaveParams.h"
#include "ErrorType.h"
#include "xmap.h"
#include "XcpList.h"
#include "CreateParameters.h"
#include <unordered_set>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include "ReferenceTrackerInterfaces.h"
#include "PropertyChangedParams.h"
#include "EffectiveValue.h"
#include "InheritanceContextChangeKind.h"
#include <weakref_count.h>
#include <weakref_ptr.h>
#include <vector_map.h>
#include <forward_list>
#include <stack_allocator.h>
#include <wil\result.h>
#include <ModifiedValue.h>
#include "SetValueParams.h"
#include <NamespaceAliases.h>
#include <CDOAssociative.h>
#include <FlyweightCoreAdapter.h>
#include <CDOSharedState.h>
#include <InterfaceForwarder.h>
#include <Microsoft.UI.Xaml.coretypes2.h>
#include <MetadataAPI.h>
#include <FeatureFlags.h>
#include <fwd/windows.ui.composition.h>
#include <Microsoft.UI.Input.Partner.h>

// Handy macro to compute the offset from the beginning of a structure to the
// the specified member.  Similar to the macro 'offsetof' in stddef.h but its
// been modified to work under PAL types.

#define OFFSET(s, m)    XUINT32(XUINT64(XHANDLE(&(((s *) 0)->m))))

// Given a target pointer access the field specified by the offset.
#define READ_OFFSET(target, offset) ((void*)&((UINT8*)target)[offset])

// Safely release an interface pointer reached from the above macro

#if defined(RELEASE)
#undef RELEASE
#endif

#define RELEASE(o)      \
    if (NULL != *((void **) &((XUINT8 *) this)[o]))  \
        (*((CDependencyObject **) &((XUINT8 *) this)[o]))->Release()

class CDependencyProperty;
class CPropertyBase;
class CValue;
class CClassInfo;
class CAutomationPeer;
class CCoreServices;
enum class KnownPropertyIndex : UINT16;
class CCoreServices;
class NamespaceInfo;
class CType;
class CThemeResource;
class CThemeResourceExtension;
class CEventArgs;
class CFontFamily;
class CBrush;
class CUIElement;
class TextFormatting;
class InheritedProperties;
class DependencyObjectDCompRegistry;
class CDeferredElement;
class CDeferredStorage;
class CTimeManager;
class WinRTExpressionConversionContext;
struct TemplateCacheExpansionContext;
class CPopup;

// Flags used with ParticipatesInManagedTreeInternal
//
// Peer doesn't have state. Such peers will not be protected from GC, and
// can be released and resurrected as needed, because they have no state.
#define DOESNT_PARTICIPATE_IN_MANAGED_TREE       0
//
// Peer initially doesn't have state, but could gain state.
// For example, state is gained when a framework property is set or it
// if it is used in Binding. When the peer gains state, the object is marked
// as participating using m_bitFields.fParticipatesInManagedTreeDefault.
// Peer is protected from GC only when it starts participating.
#define OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE  1
//
// Peer has state. It is protected from GC, and cannot be resurrected,
// because the state would be lost.
#define PARTICIPATES_IN_MANAGED_TREE             2

const uintptr_t c_hasEverHadManagedPeer = 0x00000001;

namespace DirectUI
{
    class DependencyObject;
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {
    class ConversionUnitTests;
    class PropertySystemUnitTests;
} } } } } }

namespace Theming {
    enum class Theme : uint8_t;
}

namespace Diagnostics {
    class DiagnosticsInterop;
    class ResourceDependency;
}
//------------------------------------------------------------------------
//
//  Class:  CDependencyObject
//
//  Synopsis:
//      Base typed reference counted object.
//
//------------------------------------------------------------------------

enum ValueOperation
{
    ValueOperationDefault = 0x0,
    ValueOperationReevaluate = 0x1,
    ValueOperationClearValue = (0x2 | ValueOperationReevaluate),
    ValueOperationFromSetValue = 0x4
};

enum class ObjectStrictness : uint8_t
{
    Agnostic = 0,
    NonStrictOnly = 1,
    StrictOnly = 2
};

class CDependencyObject
{
    // Allow unit tests to access everything.
    friend class Microsoft::UI::Xaml::Tests::Framework::DependencyObject::ConversionUnitTests;
    friend class Microsoft::UI::Xaml::Tests::Framework::DependencyObject::PropertySystemUnitTests;

    friend class Diagnostics::DiagnosticsInterop;
    friend class Diagnostics::ResourceDependency;

    template <typename Ty>
    friend xref::weakref_ptr<Ty> xref::get_weakref(_In_opt_ Ty* pTargetObject);

    enum LayoutCausality
    {
        None = 0x0,
        ElementAlreadyDirty = 0x1,
        Layout = 0x2,
        Render = 0x4

    };

    struct EffectiveValueParams
    {
        const CDependencyProperty* m_pDP;
        const CValue& m_value;
        BaseValueSource m_baseValueSource;
        IInspectable* m_pValueOuterNoRef;

        EffectiveValueParams(_In_ const CDependencyProperty* pDP, _In_ const CValue& value, _In_ BaseValueSource baseValueSource, _In_opt_ IInspectable* pValueOuter = nullptr)
            : m_pDP(pDP), m_value(value), m_baseValueSource(baseValueSource), m_pValueOuterNoRef(pValueOuter)
        {
            ASSERT(pDP != nullptr);
        }

        EffectiveValueParams& operator=(const EffectiveValueParams& other) = delete;
    };

    struct UpdateEffectiveValueParams
    {
        const CDependencyProperty* m_pDP;
        const CValue& m_value;
        XUINT32 m_valueOperation;
        const std::shared_ptr<CModifiedValue>& m_modifiedValue;
        IInspectable* m_pValueOuterNoRef;
        BaseValueSource m_baseValueSource;

        UpdateEffectiveValueParams(
            _In_ const CDependencyProperty* pDP,
            _In_ const CValue& value,
            _In_ const std::shared_ptr<CModifiedValue>& modifiedValue = std::shared_ptr<CModifiedValue>(),
            _In_ XUINT32 valueOperation = ValueOperationDefault,
            _In_ BaseValueSource baseValueSource = BaseValueSourceUnknown,
            _In_opt_ IInspectable* pValueOuter = nullptr)
            : m_pDP(pDP)
            , m_value(value)
            , m_valueOperation(valueOperation)
            , m_modifiedValue(modifiedValue)
            , m_baseValueSource(baseValueSource)
            , m_pValueOuterNoRef(pValueOuter)
        {
            ASSERT(pDP != nullptr);
        }

        UpdateEffectiveValueParams& operator=(const UpdateEffectiveValueParams& other) = delete;
    };

    // Bit fields.  These are wrapped in a struct so that they can
    // be updated in an InterlockedCompareExchange (see SetHasManagedPeer).
    // Wrapping in the struct has no effect on the in-memory storage.
    struct DependencyObjectBitFields
    {
        // There are only 7 types of independent animations (see CTimeManager::UpdateIATargets) and 1 type of manipulation,
        // so the counter can fit in 4 bits.
        static constexpr int c_independentCounterBits = 4;

        DependencyObjectBitFields()
            : m_bitFieldsAsDWORD(0)
        {
            fParentIsPublic = TRUE;
        }

        union
        {
            struct
            {
                XUINT32 fLive : 1;                  // 0
                XUINT32 fIsAssociated : 1;          // 1

                //
                // Namescope-related flags.
                //
                XUINT32 fIsNamescopeOwner : 1;      // 2
                XUINT32 fIsNamescopeMember : 1;     // 3

                // fShouldRegisterInParentNamescope:
                //
                // Indicates that this element, while it may have its own namescope
                // (meaning that it could have fIsPermanentNamescopeOwner set to true),
                // should have its name registered in its parent's namescope.
                //
                XUINT32 fShouldRegisterInParentNamescope : 1;   // 4

                // This DO is part of a Template Namescope.  This flag is set by the Parser on any DO
                // that is created by the parser when a ControlTemplate is being Loaded/Applied.
                XUINT32 fIsTemplateNamescopeMember : 1;         // 5

                XUINT32 fParentIsPublic : 1;                    // 6

                // This will be set if we have done an AddRef on the core, so that we can call Release on our m_pCore propertly
                XUINT32 fNeedToReleaseCore : 1;                 // 7

                // m_Valid on DO can be a pointer to XUINT32 array or can be just store to bitfileds.
                // It depends on how much properties that class has. If it's an pointer to an allocated
                // array we have to release that array. This is a special bit reserved to check if the default valid
                // store is a pointer or just 32 bit bitfiled number
                XUINT32 fIsValidAPointer : 1;                   // 8

                // This indicates that there is a managed peer referencing this object.
                XUINT32 fHasManagedPeer : 1;                    // 9

                // Is expected reference being held on peer? Occurs when peer is stateful.
                XUINT32 hasExpectedReferenceOnPeer : 1;         // 10

                // This flag will be set when this object is on parser's stack and parser owns its parent relationship.
                // It will be reset when the parser pops the object off its stack.
                XUINT32 fParserOwnsParent : 1;                  // 11

                // We do not implement the full Freezable semantics, but sometimes we want to simulate a
                //  frozen object in order to approach WPF behavior.  This only blocks SetValue() and is
                //  not a true freeze - object state can still be modified if the object has sub-values
                //  that do not have this flag set.  (Example: simFrozen.SomeMember.SetValue())
                XUINT32 fSimulatingFrozen : 1;                  // 12

                // This flag indicates if the managed peer of this object is a knowntype vs a type defined in a managed assembly
                XUINT32 fIsCustomType : 1;                      // 13

                //  The parser is currently parsing this element.  The flag is set after
                //  the element has been created, but before the first attribute is parsed
                //  and is cleared after the end tag is parsed.
                XUINT32 fIsParsing : 1;                         // 14

                // Indicates whether the DO is currently processing Enter/Leave.  It is used to prevent stack overflows
                // caused by cycles.
                XUINT32 fIsProcessingEnterLeave : 1;            // 15

                // Indicates whether the DO is currently processing themes.  It is used to prevent stack overflows
                // caused by cycles.
                XUINT32 fIsProcessingThemeWalk : 1;             // 16

                // Indicates whether the DO is currently processing a device related cleanup. It is used to prevent stack overflows.
                XUINT32 fIsProcessingDeviceRelatedCleanup : 1;  // 17

                // If an object returns "optionally" from ParticipatesInManagedTreeInternal,
                // then the value of this flag is returned from ParticipatesInManagedTree.
                XUINT32 fParticipatesInManagedTreeDefault : 1;  // 18

                // If this dependency object is using as value of Inherited property the flag
                // will indicate that fact.
                XUINT32 fValueOfInheritedProperty : 1;          // 19

                // If an InheritanceContextChanged event should be raised because there is a consumer listening to it
                XUINT32 fWantsInheritanceContextChanged : 1;    // 20

                // Indicates whether m_pParent should only be used for InheritanceContext purposes.  InheritanceContext borrows
                // the parent property by setting it when Custom DP's are set.  Previously, the parent was null in this
                // case, and this flag is used to maintain that behavior for GetParent.  Borrowing m_pParent allows us to save
                // on allocation.
                XUINT32 fParentIsInheritanceContextOnly : 1;    // 21

                // Is name a usage name or definition name?
                XUINT32 fHasUsageName : 1;                      // 22

                // Is this element pegged by being the child of an active CRootVisual?
                XUINT32 isParentAnActiveRootVisual : 1;           // 23

                // These flags are used by the input manager for marking input state on each CDependencyObject
                // during its tree walks for DragEnter/Leave input events.
                XUINT32 hasInputDragEnter : 1;                    // 24
                XUINT32 isDragInputNodeDirty : 1;                 // 25

                // Used to prevent cycles in GC's reference tracker walk
                XUINT32 isProcessingReferenceTrackerWalk : 1;     // 26

                // Indicates element is a parent of deferred element(s)
                XUINT32 hasDeferred : 1;                          // 27

                // A count of the number of types of independent animations or manipulations targeting this DO. It registers
                // itself with the RenderTarget iff this count is nonzero.
                XUINT32 m_independentTargetCount : c_independentCounterBits;    // 28-31
            };

            XUINT32 m_bitFieldsAsDWORD;
        };
    };

    static_assert(sizeof(DependencyObjectBitFields) == 4, "DependencyObjectBitFields must fit in 4 bytes so it can be interlocked-exchanged (see SetHasManagedPeer)");

public:
#if defined(__XAML_UNITTESTS__)
    CDependencyObject()  // !!! FOR UNIT TESTING ONLY !!!
        : CDependencyObject(nullptr)
    {}
#endif

    // Validate a newly allocated object and call init.  This helper function is created
    // to reduce the code we have in our macro.
    static _Check_return_ HRESULT ValidateAndInit(_In_ CDependencyObject *pDO, _Out_ CDependencyObject **ppDO);

    // Updates generation counters so that inherited properties know to re-query for the inherited value.
    void InvalidateInheritedProperty(_In_ const CDependencyProperty* pDP);

    CAutomationPeer* OnCreateAutomationPeerInternal();

    void UnpegManagedPeer(_In_opt_ bool isShutdownException = false);
    _Check_return_ HRESULT PegManagedPeer(_In_opt_ bool isShutdownException = false, _Out_opt_ bool* pfPegged = nullptr);

    // All objects derived from dependency objects expose this method. It would be
    // a virtual method except we're using it to create the object that will have
    // the vtable on it. C'est la vie.

    DECLARE_CREATE(CDependencyObject);

    // AddRef and Release are intentionally not virtual since they are very hot and we're instead implementing
    // IUnknown and IInspectable via interface forwarding.  This is good because it avoids a bunch of costs
    // of virtuals on these hot functions - CFG checks, vtable costs in terms of binary size, and so on.
    // We also differ from the traditional COM AddRef/Release in that we return void - none of our code needs the
    // return values today, so by returning void we can let the compiler optimize a bit more around these calls.
    void AddRef();
    void Release();

    virtual void ReleaseOverride() { }

    virtual _Check_return_ HRESULT OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsGCRoot);

#if DBG
    virtual void PreOnManagedPeerCreated();
    virtual void PostOnManagedPeerCreated();
#endif

    void ClearPeerReferences();
    bool ShouldFrameworkClearCoreExpectedReference();
    void DisconnectManagedPeer();

    // Set/Get the flag that is use if this object returns
    // OPTIONALLY_PARTICIPATE_IN_MANAGED_TREE from ParticipatesInManagedTreeInternal.
    _Check_return_ HRESULT SetParticipatesInManagedTreeDefault();
    bool GetParticipatesInManagedTreeDefault();

    void ReferenceTrackerWalk(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer);

    virtual bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer);

    _Check_return_ HRESULT SetExpectedReferenceOnPeer();
    _Check_return_ HRESULT ClearExpectedReferenceOnPeer();
    bool HasExpectedReferenceOnPeer() { return m_bitFields.hasExpectedReferenceOnPeer ? true : false; }
    void OnClearedExpectedReferenceOnPeer();

    _Check_return_ HRESULT AddExtraExpectedReferenceOnPeer();
    _Check_return_ HRESULT ClearExtraExpectedReferenceOnPeer();

protected:
    CDependencyObject(_In_ CCoreServices *pCore);

    virtual  ~CDependencyObject();

    // TODO: Task 40510398: Remove Special DependencyObject AddRef/Release implementation methods
    UINT32 ThreadSafeAddRef();
    UINT32 ThreadSafeRelease();

public:
    UINT32 GetRefCount() const
    {
        return m_ref_count.GetRefCount();
    }

protected:
    DependencyObjectDCompRegistry* GetDCompObjectRegistry() const;

    bool HasRefCountOnCCoreServices() const
    {
        return !!m_bitFields.fNeedToReleaseCore;
    }

private:
    void AddRefImpl(UINT32 cRef);
    void ReleaseImpl(UINT32 cRef);

    // End of DO AddRef/Release implementation methods

    _Ret_notnull_ xref::details::control_block* EnsureControlBlock()
    {
        return m_ref_count.ensure_control_block();
    }

public:
    _Check_return_ HRESULT ClearValueByIndex(_In_ KnownPropertyIndex index);

    virtual _Check_return_ HRESULT
        GetValue(
        _In_ const CDependencyProperty* dp,
        _Out_ CValue* pValue);

    _Check_return_ HRESULT GetValueByIndex(
        _In_ KnownPropertyIndex index,
        _Out_ CValue* value) const;

    _Check_return_ HRESULT IsAmbientPropertyValueAvailable(
        _In_  const CDependencyProperty *pdp,
        _Out_ bool               *pbAvailable);

    _Check_return_ HRESULT GetValueInternal(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue);

    _Check_return_ HRESULT SetValue(
        _In_ const CDependencyProperty* dp,
        _In_ const CValue& value);

    virtual _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args);

    _Check_return_ HRESULT SetValueByIndex(
        _In_ KnownPropertyIndex nPropertyIndex,
        _In_ const CValue& value);

    _Check_return_ HRESULT SetAnimatedValue(
        _In_ const CDependencyProperty* dp,
        _In_ const CValue& value,
        _In_ xref_ptr<CDependencyObject> sourceSetter = nullptr);

    _Check_return_ HRESULT ClearAnimatedValue(
        _In_ const CDependencyProperty* dp,
        _In_ const CValue& holdEndValue);

    virtual _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false);

    virtual _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue);

    _Check_return_ HRESULT ReadLocalValue(
        _In_ const CDependencyProperty* dp,
        _Inout_ CValue* pValue,
        _Inout_ bool* hasLocalValue,
        _Inout_ bool* isTemplateBound);

    bool HasLocalOrModifierValue(_In_ const CDependencyProperty* dp);

    _Check_return_ HRESULT ClearValue(_In_ const CDependencyProperty* dp);

    virtual _Check_return_ HRESULT InvalidateProperty(_In_ const CDependencyProperty* dp, _In_ BaseValueSource baseValueSource);

    void SetPropertyIsDefault(_In_ const CDependencyProperty* dp);

    _Check_return_ HRESULT SetPropertyIsLocal(_In_ const CDependencyProperty* dp);

    // This method should be overridden with care. It's likely we could make it nonvirtual and
    // remove the override today in CMultiParentShareableDependencyObject. Do NOT override it
    // in new classes unless fundementally changing the way parenting works in the framework.
    virtual CDependencyObject* GetParent() const;

    CDependencyObject* GetParentFollowPopups();
    CPopup* GetFirstAncestorPopup(bool windowedPopupOnly);

    _Check_return_ xstring_ptr GetDebugLabel() const;

    _Check_return_ xstring_ptr GetClassName() const;

    xstring_ptr GetUINameForTracing();
    xstring_ptr GetUIPathForTracing(bool followDOParentChain);
    template <typename T> T* GetNamedSelfOrParent();

    bool OfTypeByIndex(_In_ KnownTypeIndex nIndex) const;

    template <KnownTypeIndex targetTypeIndex>
    constexpr bool OfTypeByIndex() const
    {
        return DirectUI::MetadataAPI::IsAssignableFrom<targetTypeIndex>(GetTypeIndex());
    }

    const CClassInfo* GetClassInformation() const;

    const CDependencyProperty *GetContentProperty();

    const CDependencyProperty* GetPropertyByIndexInline(_In_ KnownPropertyIndex index) const;

    CCoreServices *GetContextInterface() const;

    virtual CAutomationPeer* OnCreateAutomationPeer()
    {
        return NULL;
    }

    virtual CAutomationPeer* GetAutomationPeer()
    {
        return NULL;
    }

    virtual _Check_return_ HRESULT SetAutomationPeer(_In_ CAutomationPeer*)
    {
        return S_OK;
    }

    virtual CAutomationPeer* OnCreateAutomationPeerImpl()
    {
        return NULL;
    }

    #pragma warning( suppress : 6101 ) // pppReturnAP doesn't get set, but this copy of the function should never be called.
    virtual XUINT32 GetAPChildren(_Outptr_result_buffer_(return) CAutomationPeer ***pppReturnAP)
    {
        (pppReturnAP); // Ignore the parameter.
        return 0;
    }

    virtual CAutomationPeer* GetPopupAssociatedAutomationPeer()
    {
        return NULL;
    }

    CDependencyObject* GetTreeRoot(_In_ bool publicParentOnly = false);
    CDependencyObject* GetParentInternal(_In_ bool publicParentOnly = true) const;
    CUIElement* GetUIElementParentInternal(_In_ bool publicParentOnly = true) const;

    virtual _Check_return_ CDependencyObject* GetInheritanceParentInternal(
        bool fLogicalParent = false // Not used here, used in the CFrameworkElement override.
        ) const
    {
        UNREFERENCED_PARAMETER(fLogicalParent);
        return m_bitFields.fParentIsInheritanceContextOnly ? nullptr : m_pParent;
    }

    virtual _Check_return_ HRESULT OnInheritanceContextChanged() { RRETURN(S_OK); }

    virtual _Check_return_ HRESULT AddParent(
        _In_ CDependencyObject *pNewParent,
        bool fPublic = true,
        _In_opt_ RENDERCHANGEDPFN pfnParentRenderChangedHandler = NULL
        );

    virtual _Check_return_ HRESULT RemoveParent(_In_ CDependencyObject *pDO = nullptr);

#if DBG
    virtual bool HasParent(_In_ CDependencyObject *pCandidate) const;
#endif

    // It's not clear if this is adding any value or is simply a bygone...
    // Consider removing this entire concept from the framework at some point
    // in the future after vetting it.
    virtual void SetParentForInheritanceContextOnly(_In_opt_ CDependencyObject *pDO);

    //some elements like media and visual brushed must update their dirty state before the rendering starts
    virtual  _Check_return_ HRESULT UpdateState() { return S_OK; }

    bool ParticipatesInManagedTree();

    // Some peers can be created, deleted, and re-created across the lifetime of this core CDO
    // Others, once created, should never be deleted unless the CDO is being deleted too.
    bool AllowPeerResurrection()
    {
        // If it hasn't ever had a managed peer, it's certainly OK to create one.
        // If it has, but it doesn't "participate in the managed tree", that means that the
        // peer is stateless and OK to re-create.
        return !HasEverHadManagedPeer() || !ParticipatesInManagedTree();
    }

    virtual XUINT32 ParticipatesInManagedTreeInternal();

    // Do we need to strengthen/weaken the ref on the managed peer
    // based on m_cRefs?
    virtual bool ControlsManagedPeerLifetime()
    {
        return false;
    }

    // Add/Remove a managed reference to the child object to keep track of the managed
    // peer within the managed peer of this object
    _Check_return_ HRESULT AddPeerReferenceToItem(_In_ CDependencyObject *pChild);
    _Check_return_ HRESULT RemovePeerReferenceToItem(_In_ CDependencyObject *pChild);
    _Check_return_ HRESULT SetPeerReferenceToProperty(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& value,
        _In_ bool bPreservePegNoRef = false,
        _In_opt_ IInspectable* pNewValueOuter = nullptr,
        _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter = nullptr);

    // During SetEffectiveValue, this method is used to update as appropriate the references between this object's
    // managed peer and the new property value's managed peer.
    HRESULT UpdatePeerReferenceToProperty(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& newValue,
        _In_ bool bOldValueIsCached,
        _In_ bool bNewValueNeedsCaching,
        _In_opt_ IInspectable* pNewValueOuter = nullptr,
        _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter = nullptr);

    // Called from generic Create implementations

    virtual _Check_return_ HRESULT InitInstance()
    {
#if DBG
        m_type = GetTypeIndex(); // InitInstance is not called for all core types, so m_type will not be set for some types. Need to hook into GetCoreConstructor in the future.
#endif
        return S_OK;
    }

    // CDependencyObject methods

    // called when the parser has finished parsing the tag associated
    // with this element.  All properties will have been set, and all
    // children will have been added by this point.
    virtual _Check_return_ HRESULT CreationComplete()
    {
        return S_OK;
    }

    _Check_return_ HRESULT  ClearPropertySetFlag(_In_ const CDependencyProperty *pdp);

    bool IsPropertyDefault(_In_ const CDependencyProperty* pDP) const;

    // IsPropertySetBySlot exists specifically to provide performance for
    // GetInheritanceParentInternal and IsRightToLeft, both of which must be
    // able to walk very quickly up a chain of inheritance parents and
    // test for locally set properties.
    // For other uses, please call DO::IsPropertyDefault above.
    bool IsPropertySetBySlot(UINT32 slot) const;

    bool IsPropertyDefaultByIndex(_In_ KnownPropertyIndex index) const;

    _Check_return_ HRESULT GetAnimationBaseValue(_In_ const  CDependencyProperty* dp, _Out_ CValue* pValue);

    // Get's the animated value for the property. Fails if the property is not animated
    _Check_return_ HRESULT GetAnimatedValue(_In_ const  CDependencyProperty* const dp, _Inout_ CValue* pValue) const;
    virtual _Ret_maybenull_ CDependencyObject* GetMentor();

    bool UseLogicalParent(_In_ KnownPropertyIndex propertyIndex) const;

    XINT32 IsValueInherited(_In_ const  CDependencyProperty *pdp);

    _Check_return_ HRESULT GetValueInherited(
        _In_  const  CDependencyProperty *pdp,
        _Out_ CValue              *pValue
        );

    virtual bool HasInheritedProperties()
    {
        return false;
    }


    virtual _Check_return_ HRESULT PullInheritedTextFormatting()
    {
        RRETURN(E_UNEXPECTED);
    }

    virtual TextFormatting **GetTextFormattingMember()
    {
        return NULL;
    }

    CREATE_GROUP_FN static EnsureTextFormatting;
    _Check_return_ HRESULT UpdateTextFormatting(
        _In_opt_ const CDependencyProperty  *pProperty,
        _In_     bool                  forGetValue,
        _In_     TextFormatting      **ppTextFormatting
        );

    _Check_return_ HRESULT EnsureTextFormattingForRead();

    _Check_return_ HRESULT GetTextFormatting(const TextFormatting **ppTextFormatting);

    _Check_return_ HRESULT UpdateInheritedPropertiesForRead();

    _Check_return_ HRESULT EnsureInheritedPropertiesForRead();

    void DisconnectInheritedProperties();

    _Check_return_ HRESULT GetInheritedProperties(const InheritedProperties **ppInheritedProperties);

    CREATE_GROUP_FN static EnsureInheritedProperties;

    _Check_return_ HRESULT GetParentTextFormatting(_Outptr_ TextFormatting **ppTextFormatting);

    virtual bool IsRightToLeft();

    virtual bool IsPropertySetByStyle(_In_ const CDependencyProperty*) const
    {
        return false;
    }

    virtual bool IsPropertyTemplateBound(_In_ const CDependencyProperty*) const
    {
        return false;
    }

    virtual _Check_return_ HRESULT SetIsSetByStyle(_In_ const CDependencyProperty* pdp, bool fSet = true)
    {
        (pdp);  // Ignore the parameter.
        (fSet); // Ignore the parameter.
        RRETURN(S_OK);
    } // Takes effect only in override

    virtual bool GetCoercedIsEnabled()
    {
        return true;
    }

    virtual bool GetUseLayoutRounding() const;

    inline CCoreServices* GetContext() const
    {
        return m_sharedState->Value().GetCoreServices();
    }

    void ContextAddRef();

    void ContextRelease();

    void TracePropertyChanged(_In_ const CDependencyProperty *pdp, _In_ const CValue* newValue);

    static void NWSetRenderDirty(_In_ CDependencyObject *pDO, DirtyFlags flags);

    virtual _Check_return_ HRESULT SetRequiresComposition(
        CompositionRequirement compositionReason,
        IndependentAnimationType detailedAnimationReason
        ) noexcept;

    virtual void UnsetRequiresComposition(
        CompositionRequirement compositionReason,
        IndependentAnimationType detailedAnimationReason
        ) noexcept;

    virtual void SetDCompResourceDirty() { }

    _Check_return_ HRESULT NotifyInheritanceContextChanged(_In_ InheritanceContextChangeKind kind = InheritanceContextChangeKind::Default);

    bool IsSimpleSparseSet(KnownPropertyIndex pid) const;
    void SetSimpleSparseFlag(KnownPropertyIndex pid, bool value);

protected:
    virtual void NWPropagateDirtyFlag(DirtyFlags flags);

    RENDERCHANGEDPFN NWGetRenderChangedHandlerInternal() const;
    void NWSetRenderChangedHandlerInternal(RENDERCHANGEDPFN handler);

    void NWPropagateInheritedDirtyFlag(
        _In_ CUIElement *pParent,
        DirtyFlags flags);

    IPALUri* GetPreferredBaseUri() const;

public:
    bool GetWantsInheritanceContextChanged()
    {
        return m_bitFields.fWantsInheritanceContextChanged;
    }

#if DBG
    // If we are currently rendering then we should not be calling SetAffected. If we must we had better be dirty already.
    // CURRENTLY OFF DUE TO ASSERT IN Graphics.Perspective.2
    //#define ASSERT_CAN_SET_AFFECTED ASSERT(!GetContext()->InRenderWalk() || GetContext()->GetDirtyState());
#define ASSERT_CAN_SET_AFFECTED
#else
#define ASSERT_CAN_SET_AFFECTED
#endif

    // Dirty state cannot be modified during a render walk unless we're already in a dirty part of the tree.
#if DBG
#define NW_ASSERT_CAN_PROPAGATE_DIRTY_FLAGS ASSERT(!GetContext()->InRenderWalk() || GetContext()->GetDirtyState());
#else
#define NW_ASSERT_CAN_PROPAGATE_DIRTY_FLAGS
#endif

    // Sets a bit flag indicating that parser owns this object's parent.
    void SetParserParentLock() { m_bitFields.fParserOwnsParent = TRUE; }

    // Resets the parent lock flag.
    void ResetParserParentLock() { m_bitFields.fParserOwnsParent = FALSE; }

    XHANDLE GetPropertyOffset(_In_ const CDependencyProperty* pDP, _In_ bool forGetValue);

    _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex uKnownID, _In_ const CValue& value);
    _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex uKnownID, _In_opt_ CDependencyObject* value);
    _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex uKnownID, _In_ INT32 value);
    _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex uKnownID, _In_ FLOAT value);
    _Check_return_ HRESULT SetValueByKnownIndex(_In_ KnownPropertyIndex uKnownID, _In_ bool value);

    _Check_return_ HRESULT NotifyPropertyChanged(const PropertyChangedParams& args);

    CDependencyObject* GetDependencyObjectFromPropertyStorage(_In_ const CDependencyProperty *pProperty);
    CDependencyObject* MapPropertyAndGroupOffsetToDO(_In_ UINT offset, _In_ UINT groupOffset);
    CValue* MapPropertyAndGroupOffsetToCValueNoRef(_In_ UINT offset, _In_ UINT groupOffset);

    typedef containers::vector_map<KnownPropertyIndex, EffectiveValue> SparseValueTable;
    typedef SparseValueTable::value_type SparseValueEntry;
    const std::unique_ptr<SparseValueTable>& GetValueTable() const
    {
        return m_pValueTable;
    }

    static const std::size_t DefaultSparseArenaSize = 26 * sizeof(SparseValueEntry);

    // ignoreRawIInspectables - Due to a hack in lifetime management of external objects
    // (see CDependencyObject::SetEffectiveValueInSparseStorage), where core drops strong reference and wraps
    // IInspectable and framework is responsible for keeping framework DO and object alive,
    // there is a case where object pointer by core can be freed.  Since we cannot guarantee it's lifetime here,
    // and currently we don't do anything interesting with it anyway,
    // if ignoreRawIInspectables is true - don't call CopyFrom (and add-ref), but substitute nullptr for the value.

    template <bool ignoreRawIInspectables = false, std::size_t ArenaSize = DefaultSparseArenaSize>
    std::vector<SparseValueEntry, Jupiter::stack_allocator<SparseValueEntry, ArenaSize>> GetSparseValueEntries(Jupiter::arena<ArenaSize>& arena)
    {
        typedef Jupiter::stack_allocator<SparseValueEntry, ArenaSize> Alloc;
        std::vector<SparseValueEntry, Jupiter::stack_allocator<SparseValueEntry, ArenaSize>> result{ Alloc(arena) };

        if (m_pValueTable)
        {
            result.reserve(m_pValueTable->size());

            for (const auto& entry : *m_pValueTable)
            {
                EffectiveValue temp;

                // Only known properties should be stored in sparse storage, and known properties should never fail any of the checks in CopyConverted().

                if constexpr (ignoreRawIInspectables)
                {
                    if (!entry.second.IsRawIInspectable())
                    {
                        IFCFAILFAST(temp.CopyFrom(entry.second));
                    }
                }
                else
                {
                    IFCFAILFAST(temp.CopyFrom(entry.second));
                }

                result.emplace_back(entry.first, std::move(temp));
            }
        }

        return result;
    }

private:

    // Temporarily exists to facilitate unit tests that don't work with framework peers.
    bool ShouldReleaseCoreObjectWhenTrackingPeerReference() const;

    // This is a helper function used in SetEffectiveValue.  If this is true, then it indicates that
    // SetPeerReferenceToProperty/UpdatePeerReferenceToProperty should be called for this property value.
    bool ShouldTrackWithPeerReferenceToProperty(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& value,
        _In_ bool bValueIsSetByStyle);

    _Check_return_ HRESULT SetParent(
        _In_opt_ CDependencyObject *pNewParent,
        bool fPublic = true,
        _In_opt_ RENDERCHANGEDPFN pfnParentRenderChangedHandler = NULL
        );

    void ResetReferencesFromChildren();
    void ResetReferencesFromSparsePropertyValues(bool isDestructing);

    static _Check_return_ HRESULT ValidateCValue(_In_ const CDependencyProperty* pDP, _In_ const CValue& value, _In_ ValueType valueType);
    static _Check_return_ HRESULT ValidateFloatValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ FLOAT eValue);
    static _Check_return_ HRESULT ValidateSignedValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ INT32 nValue);
    static _Check_return_ HRESULT ValidateThicknessValue(_In_ KnownPropertyIndex nPropertyIndex, _In_ const XTHICKNESS* thickness);
    static _Check_return_ HRESULT ValidateCornerRadiusValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ const XCORNERRADIUS* cornerRadius);
    static _Check_return_ HRESULT ValidateGridLengthValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ XGRIDLENGTH* pGridLength);

    _Check_return_ HRESULT TryProcessingThemeResourcePropertyValue(
        _In_ const CDependencyProperty* dp,
        _In_ CModifiedValue* pModifiedValue,
        _In_ CValue* pEffectiveValue,
        _In_ BaseValueSource baseValueSource,
        _Out_ bool* processed);
    _Check_return_ HRESULT NotifyPropertyValueOfThemeChange(_In_ const CDependencyProperty* dp, _In_ CValue* effectiveValue);
    void EnsureGCRootPegOnAncestor(_In_ CDependencyObject *pParentOrAssociationOwner);

protected:
    // Stores a WarningContext with the provided type and info. This DependencyObject's instance pointer and type index are added to
    // the string array included in the memory dump, should it be created.
    virtual bool StoreWarningContext(WarningContextLog::WarningContextType type, _In_ std::vector<std::wstring>& warningInfo, size_t framesToSkip);

    virtual _Check_return_ HRESULT ResetReferenceFromChild(_In_ CDependencyObject* child);

    _Check_return_ HRESULT OnParentChange(_In_ CDependencyObject *pOldParent, _In_ CDependencyObject *pParent, _In_ bool hasAtLeastOneParent);
    _Check_return_ HRESULT OnMultipleAssociationChange(_In_opt_ CDependencyObject *pAssociationOwner);

    // Set/Get the flag that indicates that object is value of inherited property
    void SetIsValueOfInheritedProperty(_In_ bool fValueOfInheritedProperty)
    {
        m_bitFields.fValueOfInheritedProperty = (fValueOfInheritedProperty != 0);
    }

    bool IsValueOfInheritedProperty()
    {
        return m_bitFields.fValueOfInheritedProperty;
    }

    _Check_return_ HRESULT UpdateEffectiveValue(_In_ const UpdateEffectiveValueParams& args);

    _Check_return_ HRESULT EvaluateEffectiveValue(
        _In_ const CDependencyProperty* dp,
        _In_opt_ CModifiedValue* pModifiedValue,
        _In_ UINT32 valueOperation,
        _Out_ CValue* pEffectiveValue,
        _Out_ BaseValueSource* pBaseValueSource,
        _Out_ bool* valueSetNeeded);

    _Check_return_ HRESULT EvaluateBaseValue(
        _In_ const CDependencyProperty* dp,
        _In_opt_ CModifiedValue* pModifiedValue,
        _In_ UINT32 valueOperation,
        _Out_ CValue* pBaseValue,
        _Out_ BaseValueSource* pBaseValueSource,
        _Out_ bool* valueSetNeeded);

    _Check_return_ HRESULT SetBaseValueSource(
        _In_ const CDependencyProperty* pDP,
        _In_ BaseValueSource baseValueSource,
        _In_opt_ EffectiveValue* sparseEntry = nullptr);

    // Source information is stored on the managed peer.
    bool TryGetSourceInfoFromPeer(
        _Out_ UINT32* line,
        _Out_ UINT32* column,
        _Out_ xstring_ptr* uri,
        _Out_ xstring_ptr* hash) const;

public:
    BaseValueSource GetBaseValueSource(_In_ const CDependencyProperty* pDP) const;
    BaseValueSource GetBaseValueSource(_In_ const EffectiveValue& sparseEntry) const;

protected:

    _Check_return_ HRESULT CreateModifiedValue(_In_ const CDependencyProperty* dp, _Out_ std::shared_ptr<CModifiedValue>& modifiedValue);
    void DeleteModifiedValue(_In_ const std::shared_ptr<CModifiedValue>& modifiedValue);
    std::shared_ptr<CModifiedValue> GetModifiedValue(_In_ const CDependencyProperty* dp) const;

    _Check_return_ HRESULT GetEffectiveValue(
        _In_ const CDependencyProperty *pdp,
        _In_opt_ CValue *pValue,
        _In_opt_ CModifiedValue *pModifiedValue,
        _Out_ CValue *pEffectiveValue);

    virtual _Check_return_ HRESULT MarkInheritedPropertyDirty(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue* pValue);

#pragma region NameScoping
    public:
        // TemplateNameScoes lookup redirection is handled entirely in CCoreServices- this tree walk is only concerned with
        // standard runtime-built NameScope entries.
        CDependencyObject* GetStandardNameScopeOwner();

        // This method is called for the actual PropertySet of the Name property on a CDependencyObject.
        _Check_return_ HRESULT SetName(_In_ const xstring_ptr& strNewName);

        virtual _Check_return_ HRESULT RegisterName(_In_ CDependencyObject *pNamescopeOwner, _In_ XUINT32 bTemplateNamescope = FALSE);
        _Check_return_ HRESULT UnregisterName(_In_ CDependencyObject *pNamescopeOwner);

        virtual bool SkipNameRegistrationForChildren()
        {
            // DOCollection calls here to determine if should use fSkipNameRegistration on the Enter walk.
            // We want to skip when in the parser, just as we do in CDependencyObject::SetName, in order to let the parser do the
            // name registration.
            // We also want to skip when the owner is a namescope owner(a UserControl or anything else with an x:Class on it), in
            // order to maintain compatibility with previous behavior(since Win8 and likely Silverlight).
            return ParserOwnsParent() && !IsStandardNameScopeOwner();
        }

        void SetIsStandardNameScopeMember(XINT32 fIsNamescopeMember) { m_bitFields.fIsNamescopeMember = fIsNamescopeMember; }
        XINT32 IsStandardNameScopeMember() { return (m_bitFields.fIsNamescopeMember == TRUE); }

        void SetIsStandardNameScopeOwner(XINT32 fIsNamescopeOwner) { m_bitFields.fIsNamescopeOwner = fIsNamescopeOwner; }
        XINT32 IsStandardNameScopeOwner() const { return (m_bitFields.fIsNamescopeOwner == TRUE); }

        void SetShouldRegisterInParentNamescope(XINT32 fShouldRegisterInParentNamescope) { m_bitFields.fShouldRegisterInParentNamescope = fShouldRegisterInParentNamescope; }
        XINT32 ShouldRegisterInParentNamescope() const { return (m_bitFields.fShouldRegisterInParentNamescope == TRUE); }

        void SetHasUsageName(XUINT32 fHasUsageName);
        bool HasUsageName() const { return m_bitFields.fHasUsageName == TRUE; }

        bool ShouldParticipateInParentNameScope() const
        {
            return !ShouldRegisterInParentNamescope() || HasUsageName();
        }

        bool HasNameScopeDefinitionName() const
        {
            return !m_strName.IsNullOrEmpty() &&
                (!HasUsageName() && (!!ShouldRegisterInParentNamescope() && !!IsStandardNameScopeOwner()));
        }

        void MarkAsPossiblyHavingDefinitionName()
        {
            SetShouldRegisterInParentNamescope(TRUE);
        }
        void SetIsTemplateNamescopeMember(bool fIsTemplateNamescopeMember)
        {
            m_bitFields.fIsTemplateNamescopeMember = fIsTemplateNamescopeMember;
        }

        bool IsTemplateNamescopeMember() const { return m_bitFields.fIsTemplateNamescopeMember; }

    protected:
        CDependencyObject* GetStandardNameScopeOwnerInternal(_In_ CDependencyObject* pFirstOwner);
        virtual CDependencyObject* GetStandardNameScopeParent();

    private:
        // To enable standard NameScope behavior with elements deferred as part of a CustomRuntimeData-based
        // optimization two things need to happen:
        // - Stand-in stubs need to be registered in the parser's NameScope at the invocation of SetCustomRuntimeData
        // - These two methods need to be overridden in order to register and unregister the names when
        //   they are created in a standard (non-template) NameScope context.
        virtual void RegisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner) {};
        virtual void UnregisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner) {};
#pragma endregion

public:
    virtual void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp);
    virtual void ReleaseDCompResources();

protected:
    virtual _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params);
    virtual _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params);
    virtual _Check_return_ HRESULT InvokeImpl(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner);

    void SetParentInternal(
        _In_opt_ CDependencyObject *pNewParent,
        bool fFireChanged,
        _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler = NULL
        );

    virtual void NotifyParentChange(
        _In_ CDependencyObject *pNewParent,
        _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
        );

    virtual _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args);
    virtual _Check_return_ HRESULT OnPropertySetImpl(_In_ const CDependencyProperty* pDP, _In_ const CValue& oldValue, _In_ const CValue& value, _In_ bool propertyChangedValue);

    // Allows a parent to react to a setvalue on another parent
    // Only called (for perf) before we are about to fail because the object is already associated.
    // That allows a parent to recover.
    // note: method is not guaranteed to be called, currently only called on specific error condition.
    virtual _Check_return_ HRESULT PerformEmergencyInvalidation(_In_ CDependencyObject* /* pToBeReparented */)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool forceRefresh = false);
    _Check_return_ HRESULT NotifyThemeChangedCoreImpl(_In_ Theming::Theme theme, _In_ bool forceRefresh = false, _In_ bool ignoreGetValueFailures = false);

    // Required because this is stubbed out in unittests
    Theming::Theme GetRequestedThemeForSubTreeFromCore();
    void SetRequestedThemeForSubTreeOnCore(Theming::Theme);

public:
    _Check_return_ HRESULT NotifyThemeChanged(_In_ Theming::Theme theme, _In_ bool forceRefresh = false);

    // Notifies us that a DP's value is going to be set from a ThemeResourceExpression in the framework.
    // These methods will make sure that when the expression tries to update the effective value, we
    // preserve whether the effective value is a modifier value (e.g. animated value), or a base value.
    void BeginPropertyThemeUpdate(_In_ const CDependencyProperty* pDP);
    void EndPropertyThemeUpdate(_In_ const CDependencyProperty* pDP);
    bool IsAnimatedProperty(_In_ const CDependencyProperty* const dp) const;
    bool IsAnimatedPropertyOverwritten(_In_ const CDependencyProperty* const dp) const;
    _Check_return_ xref_ptr<CDependencyObject> TryGetAnimatedPropertySource(_In_ const CDependencyProperty* const dp);

    virtual bool IsActive() const { return m_bitFields.fLive; }
    virtual void ActivateImpl();
    virtual void DeactivateImpl();

    // This is added so that during destruction, when an object is destructing/Leaving its descendents, those descendents
    // are able to avoid using this object.
    bool IsDestructing() const
    {
        return m_ref_count.GetRefCount() == 0;
    }


    // Tells the EventManager whether this DO has already added requests for its event handlers.
    // This lets the EventManager know, when a new Listener is attached, that a new request should
    // automatically be added to the event queue.
    virtual bool IsFiringEvents();

    virtual _Check_return_ HRESULT Enter(_In_ CDependencyObject *pNamescopeOwner, EnterParams params);
    virtual _Check_return_ HRESULT Leave(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params);

    _Check_return_ HRESULT Invoke(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner, _In_ bool live);

    // Verify that we're executing on the thread this DO belongs to.
    _Check_return_ HRESULT CheckThread();

    // Set and originate an error.  This function always returns hrToOriginate, so callers can IFC() this function call.
    _Check_return_ HRESULT SetAndOriginateError(_In_ HRESULT hrToOriginate, _In_::ErrorType eType, _In_ XUINT32 iErrorCode, _In_opt_ XUINT32 cParams = 0, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams = nullptr);

    // What is a templated parent exactly? What does it mean in the context of our framework? These are good questions
    // that have muddy answers. A template parent is a parse-time concept that should ONLY be used in the context of
    // features that work using the parse-time XAML tree. Today that list of features includes NameScoping and TemplateBindings,
    // which are features that operate on the tree as it appears in XAML, not as it appears at runtime. (they are
    // often but not always identical in many ways)
    //
    // You'll find corners of the platform where we directly instantiate an object and then set its TemplateParent. This
    // is an evil, bad thing to do. It makes no sense if you accept the One True Definition of a template parent above.
    //
    // What this code is trying to do is usually one of the following scenarios:
    // - Make FindName work for Template NameScopes when using the created object as a reference object: FindName should
    //   not work here by design and now you've created an object that doesn't have the IsTemplateNameScopeMember flag set
    //   which means all kinds of assumptions for how namescopes should be registered are now invalid.
    // - Use GetTemplateParent as a pseudo 'GetOwningControl'. The platform has a strong idea of controls as being little
    //   self-contained units and we anchor a lot of stuff to controls like the UIA tree. Over time it's been noticed
    //   that calling GetTemplateParent is a way to quickly get the owning control for a UIElement. This is an invalid
    //   invarient. Sometimes controls have elements dynamically created, and sometimes things that arne't controls
    //   are TemplateParents. In this case the code should simply traverse up the tree looking for the correct type.
    //
    // TemplateParents come from ControlTemplate parses, and from the Data and Item templates on Presenters. You should
    // NOT set this yourself, as any changes or improvements we make that take advantage of template parse origination will
    // need to take your special out-of-band set into account.
    _Check_return_ HRESULT SetTemplatedParent(_In_ CDependencyObject* parent)
    {
        // These are the only types that serve as templated parents today.
        ASSERT(parent && (
            parent->OfTypeByIndex<KnownTypeIndex::ContentPresenter>() ||
            parent->OfTypeByIndex<KnownTypeIndex::Control>() ||
            parent->OfTypeByIndex<KnownTypeIndex::ItemsPresenter>()));

        // SetTemplatedParent is a single-shot operation at parse time.
        ASSERT(GetTemplatedParent() == nullptr);

        SetTemplatedParentImpl(parent);
        return S_OK;
    }

    virtual _Check_return_ HRESULT SetTemplateBinding(_In_ const CDependencyProperty* /* targetProperty */, _In_ const CDependencyProperty* /* sourceProperty */)
    {
        return S_FALSE;
    }

    _Check_return_ HRESULT UpdateAllThemeReferences();
    _Check_return_ HRESULT UpdateThemeReference(_In_ CThemeResource* themeResource);
    _Check_return_ HRESULT UpdateThemeReference(KnownPropertyIndex propertyIndex);

    bool HasThemeResourceBinding(_In_ KnownPropertyIndex propertyIndex)
    {
        return GetThemeResourceNoRef(propertyIndex) != nullptr;
    }

    _Check_return_ HRESULT SetThemeResourceBinding(
        _In_ const CDependencyProperty* pDP,
        _In_opt_ CModifiedValue* pModifiedValue,
        _In_ CThemeResource* pThemeResource,
        _In_ BaseValueSource baseValueSource);


    virtual _Check_return_ HRESULT SubscribeToPropertyChanges(
        _In_ const CDependencyProperty* /* sourceProperty */,
        _In_ CDependencyObject* /* target */,
        _In_ const CDependencyProperty* /* targetProperty */)
    {
        return S_FALSE;
    }

    virtual bool IsUpdatingBindings() { return false; }

    virtual CDependencyObject *GetTemplatedParent() { return NULL; } // DO by default won't have templated parents

    virtual IPALUri* GetBaseUri();
    void SetBaseUri(IPALUri* uri);

    // In general, the parser is responsible for assigning a base URI to objects created from markup.
    // The exception is when Application.LoadComponent() is used to instantiate a logical tree from markup
    // (e.g. a UserControl or custom type derived from ResourceDictionary); in this scenario
    // Application.LoadComponent() sets the base URI on the root object that is handed to the parser.
    // However, during the course of object creation the parser might overwrite this base URI with what
    // it thinks the base URI should be even if the latter is incorrect. An example of such a scenario:
    //
    // Imagine a UserControl MyAwesomeControl (has corresponding markup file 'MyAwesomeControl.xaml') that
    // is referenced from 'MainPage.xaml'.
    // <Page x:Class="MyApp.MainPage"
    //       xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    //       xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    //       xmlns:local="using:MyApp">
    //   <local:MyAwesomeControl />
    // </Page>
    // 1. Application.LoadComponent() sets base URI of the MainPage instance to "ms-appx:///MainPage.xaml"
    //    and then invokes the parser to process the markup.
    // 2. Parser encounters instruction to create new instance of type "MyApp.MyAwesomeControl"
    // 3. MyAwesomeControl's constructor is called which in turn calls Application.LoadComponent;
    //    this is a recursive parser invocation.
    // 4. MyAwesomeControl instance's base URI is set to "ms-appx:///MyAwesomeControl.xaml" by 
    //    Application.LoadComponent. The inner parser processes MyAwesomeControl's markup before returning 
    //    the fully initialized control to the outer parser.
    // 5. The outer parser sets the current base URI, "ms-appx:///MainPage.xaml", on the just-created control
    //    not knowing that the inner parser had already set it to the correct value of 
    //    "ms-appx:///MyAwesomeControl.xaml".
    //
    // We can avoid this by setting the below flag on the root object at the same time that 
    // Application.LoadComponent sets its base URI thereby informing the parser that it should not try to 
    // overwrite it with its own value (see XamlManagedRuntime::SetUriBase/XamlNativeRuntime::SetUriBase)
    bool GetCanParserOverwriteBaseUri() const { return m_canParserOverwriteBaseUri; }
    void SetCanParserOverwriteBaseUri(bool value) { m_canParserOverwriteBaseUri = value; }

    wrl::ComPtr<ixp::IIslandInputSitePartner> GetElementIslandInputSite();

    HWND GetElementPositioningWindow();

    bool IsIndependentTarget() const
    {
        return m_bitFields.m_independentTargetCount > 0;
    }

protected:
    virtual void SetTemplatedParentImpl(_In_opt_ CDependencyObject*) {};

    _Check_return_ HRESULT ClonePropertySetField(_In_ const CDependencyObject *pdoOriginal);

public:
    virtual bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL)
    {
        UNREFERENCED_PARAMETER(hEvent);
        UNREFERENCED_PARAMETER(fInputEvent);
        UNREFERENCED_PARAMETER(pArgs);
        return true;
    }

    virtual void PropagateLayoutDirty(bool affectsMeasure, bool affectsArrange);
private:

    // Create a peer for this object (if it doesn't already have one), and give the requested peg.
    // This is the main ensure-and-peg method that the other methods call.
    _Check_return_ HRESULT PrivateEnsurePeerAndTryPeg(
        _In_ bool fPegNoRef,    // NoRef peg is not counted, and is removed automatically when the object enters the tree
        _In_ bool fPegRef,      // Ref peg is counted, and requires an UnpegManagedPeer call to remove
        _In_ bool isShutdownException,
        _Out_opt_ bool* pfPegged = NULL);

    void SetHasEverHadManagedPeer(bool setHasEverHadManagedPeer);
    bool HasEverHadManagedPeer() const { return ((m_hasEverHadManagedPeer & c_hasEverHadManagedPeer) ? true : false); }

public:

    // Returns TRUE if the dependency property stores a back-reference to something else in the visual tree. When
    // walking through objects in the visual tree, we don't want to walk through back references.
    static bool IsDependencyPropertyBackReference(_In_ KnownPropertyIndex propertyIndex);

    // Returns TRUE if the dependency property stores a weak reference.
    static bool IsDependencyPropertyWeakRef(_In_ KnownPropertyIndex propertyIndex);

    // Ensure this object has a peer, but don't add any pegging.  (When the element is already in the
    // tree, the pegging is unnecessary, because the tree will protect it.)
    _Check_return_ HRESULT EnsurePeer()
    {
        return PrivateEnsurePeerAndTryPeg(
            FALSE,  // fPegNoRef
            FALSE, // fPegRef
            FALSE, // isShutdownException
            nullptr); // pfPegged
    }

    // Ensure this object has a peer, and try to put a (ref-counted) peg on it.
    // The peg needs to be removed later with a call to UnpegManagedPeer.
    _Check_return_ HRESULT EnsurePeerAndTryPeg(
        _Out_opt_ bool* pfPegged = NULL)
    {
        return PrivateEnsurePeerAndTryPeg(
                    FALSE, // fPegNoRef
                    TRUE,  // fPegRef
                    FALSE,
                    pfPegged );
    }


    // Ensure this object has a peer, and if it doesn't already, create one and put a no-ref peg on it.
    // There's ordinarily no need to call UnpegManagedPeerNoRef; that happens automatically when the object
    // is put into the tree.  This is intended to be used when a tree is getting created.
    // (Is there a way we can eliminate this, and always set the peg in EnsurePeer, unless it's already in the tree?)
    _Check_return_ HRESULT EnsurePeerDuringCreate()
    {
        return PrivateEnsurePeerAndTryPeg(
            TRUE,  // fPegNoRef
            FALSE, // fPegRef
            FALSE, // isShutdownException
            nullptr); // pfPegged
    }

    _Check_return_ HRESULT TryEnsureManagedPeer(
        _Out_ bool* pIsPeerAvailable,
        _In_ bool fPegNoRef = false,
        _In_ bool fPegRef = false,
        _In_ bool isShutdownException = false,
        _Out_opt_ bool* pfPegged = nullptr);


    // Helper for derived types to quickly mark a type as a custom type.
    void SetIsCustomType()
    {
        m_bitFields.fIsCustomType = TRUE;
    }
    void SetHasManagedPeer(XUINT32 fHasManagedPeer, XUINT32 fIsCustomType);

    XINT32 HasManagedPeer() const
    {
        return m_bitFields.fHasManagedPeer == TRUE;
    }

    void PegManagedPeerNoRef();
    void UnpegManagedPeerNoRef();

    void TryPegPeer(_Out_ bool *pPegged, _Out_ bool *pIsPendingDelete);

    bool IsCustomType() const;

    // While C++ doesn't really have the concept of 'abstract' or 'sealead' classes
    // the functional equivalent for us is as follows: A class that can't be created
    // directly from XAML but acts as a base class is considered an 'abstract' class.
    // A class that has no other classes derived from it is considered 'final'.
    // Every class that isn't 'abstract' must override this method.  Any 'final'
    // class that can be placed in a collection must override this method.

    virtual KnownTypeIndex GetTypeIndex() const;

    virtual bool AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const
    {
        UNREFERENCED_PARAMETER(iListenerType);
        UNREFERENCED_PARAMETER(eventIndex);
        return false;
    }

    virtual void SetAssociated(_In_ bool isAssociated, _In_opt_ CDependencyObject *pAssociationOwner)
    {
        UNREFERENCED_PARAMETER(pAssociationOwner);
        m_bitFields.fIsAssociated = isAssociated;
    }

    virtual bool IsAssociated() const
    {
        return (m_bitFields.fIsAssociated == TRUE);
    }

    XINT32 ParserOwnsParent()
    {
        return (m_bitFields.fParserOwnsParent == TRUE);
    }

    // This is overridden by objects which allows multiple assocation. By default, all DependencyObject
    // returns FALSE. But DrawingAttributes (Inking in Jolt) does override this to allow same
    // DrawingAttributes to be shared by multiple strokes
    virtual bool DoesAllowMultipleAssociation() const
    {
        return false;
    }

    //we keep a list of parents for shareable objects that participate in the visual tree so they can
    //notify their parents when the dirty state changes
    virtual bool DoesAllowMultipleParents() const
    {
        return false;
    }
    //sometimes we need to know if the object cares about his parent - i.e. is either
    // simple DO or MultiParentDependencyObject
    bool IsParentAware() const;

    // This object should start pretending to be a frozen object and disallow SetValue()
    void SimulateFreeze()
    {
        m_bitFields.fSimulatingFrozen = TRUE;
    }

    void SimulateUnfreeze()
    {
        m_bitFields.fSimulatingFrozen = FALSE;
    }

    bool IsFrozen()
    {
        return m_bitFields.fSimulatingFrozen;
    }

    void SetIsParsing(XINT32 fIsParsing)  { m_bitFields.fIsParsing = fIsParsing; }
    bool IsParsing() const { return !!m_bitFields.fIsParsing; }

    bool HasWeakRef() const { return m_ref_count.get_control_block() != nullptr; }

    void SetWantsInheritanceContextChanged(bool fWantsInheritanceContextChanged) { m_bitFields.fWantsInheritanceContextChanged = fWantsInheritanceContextChanged; }

    DirectUI::DependencyObject* GetDXamlPeer() const
    {
        // To get the actual DXaml peer, clear the low order bit of m_pDXAMLPeer (this is a no op on chk builds)
        return reinterpret_cast<DirectUI::DependencyObject*>(reinterpret_cast<uintptr_t>(m_pDXAMLPeer) & ~c_hasEverHadManagedPeer);
    }
    void SetDXamlPeer(_In_opt_ DirectUI::DependencyObject* pDXamlPeer);

    _Check_return_ HRESULT ValidateStrictnessOnProperty(_In_ const CPropertyBase* prop);
    _Check_return_ HRESULT DefaultStrictApiCheck();
    _Check_return_ HRESULT StrictOnlyApiCheck(const WCHAR* apiName);
    _Check_return_ HRESULT NonStrictOnlyApiCheck(const WCHAR* apiName);
    const WCHAR* FindFirstNonStrictOnlyPropertyInUse() const;
    const WCHAR* FindFirstStrictOnlyPropertyInUse() const;

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    bool IsStrictType() const;
#endif

    ObjectStrictness GetObjectStrictness() const
    {
        return m_objectStrictness;
    }

    void SetObjectStrictness(ObjectStrictness strictness)
    {
        m_objectStrictness = strictness;
    }

    // Input methods
public:
    bool HasDragEnter() const
    {
        return m_bitFields.hasInputDragEnter;
    }

    void SetDragEnter(bool bVal)
    {
        m_bitFields.hasInputDragEnter = bVal;
    }

    bool IsDragInputNodeDirty() const
    {
        return m_bitFields.isDragInputNodeDirty;
    }

    void SetDragInputNodeDirty(bool bVal)
    {
        m_bitFields.isDragInputNodeDirty = bVal;
    }

    bool IsProcessingEnterLeave()   { return (m_bitFields.fIsProcessingEnterLeave); }
    bool IsProcessingThemeWalk()   { return (m_bitFields.fIsProcessingThemeWalk); }
    bool IsProcessingDeviceRelatedCleanup()   { return (m_bitFields.fIsProcessingDeviceRelatedCleanup); }
    void SetThemeResource(_In_ const CDependencyProperty *pdp, _In_ CThemeResource* pThemeResource);

    // Should this property be notified of theme change?
    bool ShouldNotifyPropertyOfThemeChange(_In_ KnownPropertyIndex propertyIndex);

    _Check_return_ HRESULT GetDefaultValue(_In_ const CDependencyProperty* dp, _Out_ CValue* pDefaultValue);
    _Check_return_ HRESULT GetDefaultValue(_In_ const CDependencyProperty* dp, _In_ const CClassInfo* pInfo, _Out_ CValue* pDefaultValue);
    _Check_return_ HRESULT GetDefaultInheritedPropertyValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value);

    DirectUI::ElementSoundMode GetEffectiveSoundMode();

    // Returns true if the effective value for the given sparse property is currently set.
    bool IsEffectiveValueInSparseStorage(_In_ KnownPropertyIndex ePropertyIndex) const;

    // Calling GetValue on an on-demand property will create it, well, on demand.
    // To test for the existence of an on-demand property without creating it, use this method
    CValue CheckOnDemandProperty(_In_ const CDependencyProperty* dp) const;
    CValue CheckOnDemandProperty(_In_ KnownPropertyIndex index) const;

private:
    void SetIsProcessingEnterLeave(bool fIsProcessingEnterLeave)  { m_bitFields.fIsProcessingEnterLeave = (fIsProcessingEnterLeave != 0); }
    void SetIsProcessingThemeWalk(bool fIsProcessingThemeWalk)  { m_bitFields.fIsProcessingThemeWalk = (fIsProcessingThemeWalk != 0); }
    void SetIsProcessingDeviceRelatedCleanup(bool fIsProcessingDeviceRelatedCleanup)
    {
        m_bitFields.fIsProcessingDeviceRelatedCleanup = (fIsProcessingDeviceRelatedCleanup != 0);
    }

    _Check_return_ HRESULT AppendEffectiveValueToCollectionInField(_In_ const EffectiveValueParams& args, _In_ XHANDLE field);

    _Check_return_ HRESULT ClearEffectiveValueInObjectField(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* previousDO);

    _Check_return_ HRESULT EnterEffectiveValue(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* obj);
    _Check_return_ HRESULT LeaveEffectiveValue(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* obj);
    _Check_return_ HRESULT LeaveEffectiveValueInField_Object(_In_ const CDependencyProperty* pDP, _In_ XHANDLE field, _Inout_ CValue& oldValue, _Inout_ bool& oldValueIsCached);

    // Gets the effective value for a field-backed property.
    _Check_return_ HRESULT GetEffectiveValueInField(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue);
    _Check_return_ HRESULT GetEffectiveValueInField_Object(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* objectAtOffset, _Out_ CValue* pValue);

    bool IsProcessingReferenceTrackerWalk() const { return m_bitFields.isProcessingReferenceTrackerWalk ;}
    void SetIsProcessingReferenceTrackerWalk(bool isProcessingReferenceTrackerWalk)  { m_bitFields.isProcessingReferenceTrackerWalk = (isProcessingReferenceTrackerWalk != 0); }

    // Gets the effective value for a method-backed property.
    _Check_return_ HRESULT GetEffectiveValueViaMethodCall(_In_ const CDependencyProperty* pDP, _Out_ CValue *pValue);

    _Check_return_ HRESULT SetEffectiveValue(_In_ const EffectiveValueParams& args);

    // Stores the effective value for a field-backed property.
    _Check_return_ HRESULT SetEffectiveValueInField(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue);
    _Check_return_ HRESULT SetEffectiveValueInField_Any(_In_ const EffectiveValueParams& args, _In_ XHANDLE field, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue);
    _Check_return_ HRESULT SetEffectiveValueInField_Object(_In_ const EffectiveValueParams& args, _In_ XHANDLE field, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue);

    // Stores the effective value for a sparse property.
    _Check_return_ HRESULT ClearEffectiveValueInSparseStorage(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue);
    _Check_return_ HRESULT SetEffectiveValueInSparseStorage(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue);

    _Check_return_ HRESULT SetEffectiveValueViaMethodCall(_In_ const EffectiveValueParams& args, _Inout_ bool& propertyChangedValue);

    _Check_return_ HRESULT OnPropertySet(_In_ const CDependencyProperty* pDP, _In_ const CValue& oldValue, _In_ const CValue& value, _In_ bool propertyChangedValue);

    // Verify the specified value can be associated with the current object.
    _Check_return_ HRESULT VerifyCanAssociate(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& value);

    // Try to add a value to the content property's collection if applicable.
    _Check_return_ HRESULT TryAddToContentPropertyCollection(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& value,
        _Out_ bool* wasCollectionAdd);

    bool IsParentValid() { return m_pParent && !m_bitFields.fParentIsInheritanceContextOnly; }

    HRESULT SetExpectedReferenceOnPeerAndNotifyParentUpdated();

    CDependencyObject* GetParentForSoundMode();
    DirectUI::ElementSoundMode GetSoundModeIfAvailable();

protected:
    // Gets the effective value for a sparse property.
    // TODO: The only things that call this pass in a property index.
    _Check_return_ HRESULT GetEffectiveValueInSparseStorage(_In_ const CDependencyProperty* dp, _Out_ CValue* value);

    // Stores the effective value for a sparse property.
    _Check_return_ HRESULT ClearEffectiveValueInSparseStorage(_In_ const CDependencyProperty* dp);
    _Check_return_ HRESULT SetEffectiveValueInSparseStorage(_In_ const CDependencyProperty* dp, _In_ const CValue& value);

    CThemeResource * GetThemeResourceNoRef(_In_ KnownPropertyIndex ePropertyIndex);
    void ClearThemeResource(_In_ const CDependencyProperty *pdp, _Out_opt_ xref_ptr<CThemeResource>* themeResource = nullptr);

    // Property bitfields
    // These are either an XUINT32 or a pointer to an array of XUINT32s.

    union BitField
    {
        XUINT32* pointer = nullptr;
        XUINT32 bits;
    };

    void SetPropertyBitField(BitField& field, UINT16 propertyCount, UINT bit);
    void ClearPropertyBitField(BitField& field, UINT16 cProperties, UINT bit);
    bool GetPropertyBitField(const BitField& field, UINT16 propertyCount, UINT bit) const;

    XUINT32 IsBitFieldValidPointer() const
    {
        return m_bitFields.fIsValidAPointer;
    }

    // Associative storage management

    // Modified Values. Maps a property to a ModifiedValue if that property
    // has a modifier, like an Animation.
    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(ModifiedValuesList, ModifiedValues);

    // ThemeResourceMap maps property index to ThemeResourceExtension
    // If a property value is a ThemeResourceExtension, it is
    // stored here to defer creation of ThemeResourceExpression until
    // the first theme change, for better perf.
    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(ThemeResourceMap, ThemeResources);

    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(xref_ptr<CDependencyObject>, AutomationAnnotations);

    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(xstring_ptr, XUid);

    // List of CStyles interested in being told when Setter.Value changes.
    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(SetterValueChangedNoficationSubscribersList, SetterValueChangedNoficationSubscribers);

private:
    bool IsSettingValueFromManaged(_In_ CDependencyObject* obj) const;
    _Check_return_ HRESULT EnterSparseProperties(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params);
    _Check_return_ HRESULT LeaveSparseProperties(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params);

    _Check_return_ HRESULT EnterObjectProperty(_In_ CDependencyObject* pDO, _In_ CDependencyObject* namescopeOwner, _In_ EnterParams params);
    _Check_return_ HRESULT LeaveObjectProperty(_In_ CDependencyObject* pDO, _In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params);

    void ClearThemeResources();

    _Check_return_ HRESULT TryReCreateUIAWrapper();

public:
    virtual std::size_t GetParentCount() const;
    virtual _Ret_notnull_ CDependencyObject* GetParentItem(std::size_t index) const;

    Theming::Theme GetTheme() const { return m_theme; }

    // Used for WinRT WUC::KeyFrameAnimations
    IUnknown* GetDCompAnimation(KnownPropertyIndex propertyIndex);
    void SetDCompAnimation(_In_opt_ IUnknown* animation, KnownPropertyIndex propertyIndex);

    xref_ptr<WUComp::ICompositionAnimation> GetWUCDCompAnimation(KnownPropertyIndex propertyIndex);

    // Xaml animations will create WUC animations and push them on their target DOs. It's then up to the DOs to pick them
    // up during the render walk, connect them, and start them. If a DO isn't visited during the render walk (e.g. it's the
    // child of a collapsed element), then the animation would never start. This causes problems because that unstarted
    // animation will never fire its DComp completed event, which means the Xaml animation won't fire its completed event
    // either.
    //
    // This method is meant to ensure that the WUC animation is connected and started, even if the target isn't walked. We
    // collect a list of animated targets when animations are ticked, and explicitly visit them all at the end of the frame
    // to make sure they've attached their animations. A DO that has been visited will have cleaned its dirty flags and will
    // no-op during this walk.
    virtual void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context);

    CTimeManager* GetTimeManager();

    _Check_return_ HRESULT GetRealizingProxy(
        _Outptr_ CDeferredElement** proxy);

    _Check_return_ HRESULT SetRealizingProxy(
        _In_ CDeferredElement* proxy);

    _Check_return_ HRESULT ClearRealizingProxy();

    void SetHasDeferred(_In_ bool hasDeferred)
    {
        m_bitFields.hasDeferred = hasDeferred;
    }

    bool HasDeferred() const
    {
        return m_bitFields.hasDeferred;
    }

    CDeferredStorage& EnsureAndGetScopeDeferredStorage();

    bool IsAncestorOf(_In_ CDependencyObject* object);

    static _Check_return_ HRESULT GetAnnotations(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    ASSOCIATIVE_STORAGE_ACCESSORS_DECL(Resources::ScopedResources::OverrideInfo, OverrideResourceKey);

    bool ShouldCheckForResourceOverrides() const    { return m_checkForResourceOverrides; };

    CDependencyObject* GetPublicRootVisual();

public:
    // Default values for properties with storage type float array

    static constexpr XPOINTF     DefaultValuePoint      = { 0, 0 };
    static constexpr XRECTF      DefaultValueRect       = { 0, 0, 0, 0 };
    static constexpr XGRIDLENGTH DefaultValueGridLength = { DirectUI::GridUnitType::Star, {}, 1 };
    static constexpr XDOUBLE     PositiveInfinity       = std::numeric_limits<DOUBLE>::infinity();
    static constexpr XRECTF      EmptyRectValue         = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };

    // CDO per-instance state

private:
    xref_ptr<CDOSharedState::Wrapper>   m_sharedState;                      // Shared state is declared first to guarantee core pointer is available during clean-up as it will be destroyed last.

public:
    xstring_ptr                         m_strName;                          // Property system backing field for Name
    InheritedProperties*                m_pInheritedProperties = nullptr;   // Property system backing field for inherited properties

    VisualTree* GetVisualTree();
    void SetVisualTree(_In_ VisualTree* visualTree);

protected:
    std::unique_ptr<SparseValueTable>       m_pValueTable;                      // Sparse property storage.

private:
    // Holds HadEverHadManagedPeer state and pointer to Peer to save space.
    // HadEverHadManagedPeer is true if LSB of pointer to Peer is set
    // (Fre builds only)
    #if !DBG
    union
    {
    #endif
        DirectUI::DependencyObject*     m_pDXAMLPeer = nullptr;
        uintptr_t                       m_hasEverHadManagedPeer;
    #if !DBG
    };
    #endif

    // Caution: m_pParent is a WeakRef to the parent if fParentIsInheritanceContextOnly is TRUE
    union
    {
        CDependencyObject*                      m_pParent = nullptr;
        xref::weakref_ptr<CDependencyObject>*   m_pMentor;
    };

    xref::details::optional_ref_count   m_ref_count;            // Holds the ref count
    BitField                            m_valid;                // Either bit-field or pointer to storage allowing fast check if property is set.  Indexed by code-generated slot number.
    DependencyObjectBitFields           m_bitFields;            // Multi-purpose bitfield.

    // On-demand storage for fields.  Use for fields set on a small percentage of CDOs.
    AssociativeStorage::LocalStorage<AssociativeStorage::CDOFields> m_associativeStorage;

    // Holds both the base theme (Light, Dark) and any HighContrast theme
    // ThemeNone indicates that the initial theme is being used, and that
    // no theme change has occurred. ThemeNone is used to defer ThemeResourceExpression
    // binding until the first theme change occurs, for better perf.
    Theming::Theme  m_theme                                 : 5;    // 5
    bool            m_isProcessingInheritanceContextChanged : 1;    // 6
protected:
    // These two flags are to allow us to optimize AddRef/Release to avoid virtual calls as much as possible in these very hot paths.
    // m_requiresThreadSafeAddRefRelease is pretty self-explanatory.  A few of our types (mainly media) requires their ref count to be interlocked, so they set this flag.
    // m_requiresReleaseOverride says that this instance wants ReleaseOverride() to be called for them to do special things - examples are Style breaking circular refs and
    //    Popup dealing with its funky async release behavior.
    // Both of these flags should be used rarely, and really should be set only in constructors and be "immutable" (if we had a way to enforce that).
    bool m_requiresThreadSafeAddRefRelease                  : 1;    // 7
    bool m_requiresReleaseOverride                          : 1;    // 8
    ObjectStrictness m_objectStrictness                     : 2;    // 9 - 10

private:
    bool m_checkForResourceOverrides                        : 1;    // 11
    bool m_canParserOverwriteBaseUri                                 : 1;    // 12
    bool                                                    : 1;    // 13- Unused
    bool                                                    : 1;    // 14- Unused
    bool                                                    : 1;    // 15- Unused
    bool                                                    : 1;    // 16- Unused

#if DBG
    bool                                m_fCalledOnManagedPeerCreated     = false;
    KnownTypeIndex                      m_type                            = KnownTypeIndex::UnknownType;    // CDOs type, for debugging convenience. Currently not set for all types.
    DirectUI::DependencyObject*         m_pLastDXAMLPeerNoRef             = nullptr;
#endif

#if XCP_MONITOR
    void*                               m_pCorePartTimeStrongPointerContext = nullptr;
#endif

};

#ifndef DBG

// You need to have a very good reason to change this!

#if defined(_X86_) || defined(_ARM_)

static_assert(sizeof(CDependencyObject) <= 52, "Size of CDependencyObject on 32-bit platform regressed.");
static_assert(alignof(CDependencyObject) <= 4, "Alignment of CDependencyObject on 32-bit platform regressed.");

#elif defined(_AMD64_) || defined(_ARM64_)

static_assert(sizeof(CDependencyObject) <= 96, "Size of CDependencyObject on 64-bit platform regressed.");
static_assert(alignof(CDependencyObject) <= 8, "Alignment of CDependencyObject on 64-bit platform regressed.");

#else

#error Unknown platform

#endif

#endif
