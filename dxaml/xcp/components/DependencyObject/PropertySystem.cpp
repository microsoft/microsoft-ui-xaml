// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include <CDependencyObject.h>
#include <UIElement.h>
#include <CControl.h>
#include <dopointercast.h>
#include <TypeTableStructs.h>
#include <GridLength.h>
#include <UIAEnums.h>
#include <EnumDefs.h>
#include <primitives.h>
#include <CString.h>
#include <Point.h>
#include <Rect.h>
#include <Size.h>
#include <Double.h>
#include <InheritedProperties.h>
#include <DOCollection.h>
#include <Type.h>
#include <ModifiedValue.h>
#include <ThemeResourceExtension.h>
#include <framework.h>
#include <ThemeResource.h>
#include <ValueBuffer.h>
#include <corep.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include <DeferredMapping.h>
#include <CValueUtil.h>
#include "theming\inc\Theme.h"
#include "DurationVO.h"
#include "RepeatBehaviorVO.h"
#include "KeyTimeVO.h"
#include <FocusableHelper.h>
#include "CircularMemoryLogger.h"

using namespace DirectUI;

// Struct used for recording failures in property system value updates.  s_failedUpdates ring buffer will contain
// information about calls to CDependencyObject::UpdateEffectiveValue which resulted in failures.
// This is useful in debugging stowed exceptions raised in property system (provided heap dump is available).

struct UpdateValueInfo
{
    UpdateValueInfo() = default;

    UpdateValueInfo(
        KnownPropertyIndex propertyIndex,
        uint16_t threadId,
        CDependencyObject* target,
        const CValue* effectiveValue)
        : m_propertyIndex(propertyIndex)
        , m_threadId(threadId)
        , m_target(target)
    {
        SetOrClearValue(m_untypedEffectiveValue, effectiveValue);
    }

    bool operator!=(const UpdateValueInfo& rhs) const
    {
        if (std::tie(m_propertyIndex, m_threadId, m_target) !=
            std::tie(rhs.m_propertyIndex, rhs.m_threadId, rhs.m_target))
        {
            return true;
        }

        if (memcmp(m_untypedEffectiveValue, rhs.m_untypedEffectiveValue, sizeof(m_untypedEffectiveValue)) != 0)
        {
            return true;
        }

        return false;
    }

    template <size_t N>
    static void SetOrClearValue(uint8_t (&buffer)[N], const CValue* value)
    {
        static_assert(sizeof(buffer) == sizeof(*value), "Size mismatch.");

        if (value)
        {
            memcpy(buffer, value, sizeof(buffer));
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
        }
    }

    KnownPropertyIndex m_propertyIndex  = KnownPropertyIndex::UnknownType_UnknownProperty;
    uint16_t m_threadId                 = 0;
    CDependencyObject* m_target         = nullptr;
    uint8_t m_untypedEffectiveValue[sizeof(CValue)]{};
};

static CircularMemoryLogger<8, UpdateValueInfo> s_failedUpdates;
static SRWLOCK s_failedUpdatesLock = SRWLOCK_INIT;

static void AppendFailedUpdateValueInfo(
    KnownPropertyIndex propertyIndex,
    CDependencyObject* target,
    const CValue* effectiveValue)
{
    auto autoLock = wil::AcquireSRWLockExclusive(&s_failedUpdatesLock);

    UpdateValueInfo current(
        propertyIndex,
        ::GetCurrentThreadId() % 0xffff,
        target,
        effectiveValue);

    const UpdateValueInfo* last = s_failedUpdates.Last();

    if (!last || *last != current)
    {
        s_failedUpdates.Log(current);
    }
}

// Returns TRUE if the dependency property stores a back-reference to something else in the visual tree. When
// walking through objects in the visual tree, we don't want to walk through back references.
bool CDependencyObject::IsDependencyPropertyBackReference(_In_ KnownPropertyIndex propertyIndex)
{
    switch (propertyIndex)
    {
    case KnownPropertyIndex::Page_Frame:
    case KnownPropertyIndex::Hub_SemanticZoomOwner:
    case KnownPropertyIndex::ListViewBase_SemanticZoomOwner:
    case KnownPropertyIndex::UIElement_AccessKeyScopeOwner:
    case KnownPropertyIndex::UIElement_KeyTipTarget:
    case KnownPropertyIndex::TextElement_AccessKeyScopeOwner:
    case KnownPropertyIndex::UIElement_XYFocusLeft:
    case KnownPropertyIndex::UIElement_XYFocusRight:
    case KnownPropertyIndex::UIElement_XYFocusUp:
    case KnownPropertyIndex::UIElement_XYFocusDown:
    case KnownPropertyIndex::Hyperlink_XYFocusLeft:
    case KnownPropertyIndex::Hyperlink_XYFocusRight:
    case KnownPropertyIndex::Hyperlink_XYFocusUp:
    case KnownPropertyIndex::Hyperlink_XYFocusDown:
    case KnownPropertyIndex::FlyoutBase_OverlayInputPassThroughElement:
    case KnownPropertyIndex::FlyoutBase_Target:
    case KnownPropertyIndex::Popup_OverlayInputPassThroughElement:
    case KnownPropertyIndex::KeyboardAccelerator_ScopeOwner:
    case KnownPropertyIndex::UIElement_KeyboardAcceleratorPlacementTarget:
    case KnownPropertyIndex::Control_FocusTargetDescendant:
    case KnownPropertyIndex::CommandingContainer_CommandingTarget:
    case KnownPropertyIndex::CommandingContainer_CommandingContainer:
        return true;
    default:
        return false;
    }
}

// Returns TRUE if the dependency property stores a weak reference.
bool CDependencyObject::IsDependencyPropertyWeakRef(_In_ KnownPropertyIndex propertyIndex)
{
    return IsDependencyPropertyBackReference(propertyIndex);
}

// Gets called when the references in the sparse storage table can no longer be safely accessed.
void CDependencyObject::ClearPeerReferences()
{
    // If the peer is stateful, it may contain sparse property state set using SetPeerReferenceToProperty,
    // corresponding to values in the sparse property table. This is typically done to manage the lifetime
    // of some property values. Clear the sparse property value table because the non-resurrectable peer
    // is being deinitialized. Stateful peers have approximately the same lifetime as the core object,
    // so it is OK to clear the sparse property table.
    //
    // If the peer is stateless, it can be resurrected, and the core object can continue to live,
    // so don't clear the sparse property value table. (SetPeerReferenceToProperty will mark the property
    // owner as participating, so a stateless peer means that SetPeerReferenceToProperty was not called.)
    if (ParticipatesInManagedTree())
    {
        // Enumerate all the sparse property values that are objects and reset their references.
        ResetReferencesFromSparsePropertyValues(false /* isDestructing */);
    }
}

//  Map a property and group offset to the referenced DO.
CDependencyObject* CDependencyObject::MapPropertyAndGroupOffsetToDO(_In_ UINT offset, _In_ UINT groupOffset)
{
    CDependencyObject* resultDO;

    resultDO = *(CDependencyObject**)READ_OFFSET(this, offset);

    if (resultDO != nullptr && groupOffset != 0)
    {
        // resultDO is a storage group pointer. Follow it to get to the actual resultDO.
        resultDO = *(CDependencyObject**)READ_OFFSET(resultDO, groupOffset);
    }

    return resultDO;
}

//  Map a property and group offset to the referenced CValue.
//
//  The returned CValue content is not AddRef'd, because this may be
//  be called from the GC thread, and CDependencyObject AddRef is not
//  thread-safe.
CValue* CDependencyObject::MapPropertyAndGroupOffsetToCValueNoRef(_In_ UINT offset, _In_ UINT groupOffset)
{
    CValue *pValue;

    pValue = (CValue*)READ_OFFSET(this, offset);

    if (pValue != nullptr && groupOffset != 0)
    {
        // pValue is a storage group pointer. Follow it to get to the actual pValue.
        pValue = (CValue*)READ_OFFSET(pValue, groupOffset);
    }

    return pValue;
}

// Returns the DependencyObject stored in a field backed storage which is a CValue or Object.
CDependencyObject* CDependencyObject::GetDependencyObjectFromPropertyStorage(_In_ const CDependencyProperty *pProperty)
{
    if (pProperty->GetStorageType() == valueObject)
    {
        return MapPropertyAndGroupOffsetToDO(pProperty->GetOffset(), pProperty->GetGroupOffset());
    }
    else if (pProperty->GetStorageType() == valueAny)
    {
        CValue *pValueNoRef = nullptr;
        pValueNoRef = MapPropertyAndGroupOffsetToCValueNoRef(pProperty->GetOffset(), pProperty->GetGroupOffset());
        if (pValueNoRef)
        {
            return pValueNoRef->AsObject();
        }
    }

    return nullptr;
}

// Activate side-effects of a given property.
_Check_return_ HRESULT CDependencyObject::Invoke(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner, _In_ bool live)
{
    if (live && pDP->NeedsInvoke())
    {
        IFC_RETURN(InvokeImpl(pDP, namescopeOwner));
    }
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::MarkInheritedPropertyDirty(_In_ const CDependencyProperty* pDP, _In_ const CValue* pValue)
{
    return S_OK;
}

// Clear animated value. Called by Animation value source to end animation.
_Check_return_ HRESULT CDependencyObject::ClearAnimatedValue(
    _In_ const CDependencyProperty* dp,
    _In_ const CValue& holdEndValue)
{
    HRESULT hr = S_OK;

    // Clear animated value in modified value
    std::shared_ptr<CModifiedValue> modifiedValue = GetModifiedValue(dp);

    if (modifiedValue != nullptr && modifiedValue->IsAnimated())
    {
        if (modifiedValue->IsEffectiveValueThemeResource())
        {
            // Clear any theme reference associated with the animated value.
            // UpdateEffectiveValue will re-set the reference for the base value if necessary.
            ClearThemeResource(dp);
        }

        // If a specific value was provided to set at the end of animation
        // (to support FillBehavior=HoldEnd), set that as local value.
        if (!holdEndValue.IsUnset())
        {
            IFC(modifiedValue->SetBaseValue(holdEndValue, BaseValueSourceLocal));
        }

        IFC(modifiedValue->ClearAnimatedValue());
        IFC(UpdateEffectiveValue(
            UpdateEffectiveValueParams(
                dp,
                CValue::Empty(),
                modifiedValue,
                ValueOperationReevaluate /* Reevaluate because animation has ended */)));
    }

Cleanup:
    // Remove modified value if it has no modifiers
    if (modifiedValue != nullptr && !modifiedValue->HasModifiers())
    {
        IGNOREHR(DeleteModifiedValue(modifiedValue));
    }
    return hr;
}

_Check_return_ HRESULT CDependencyObject::ClearValue(_In_ const CDependencyProperty* dp)
{
    // Update value by reevaluating for ClearValue.
    IFC_RETURN(UpdateEffectiveValue(
        UpdateEffectiveValueParams(
            dp,
            CValue::Empty(),
            GetModifiedValue(dp),
            ValueOperationClearValue)));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ClearValueByIndex(_In_ KnownPropertyIndex index)
{
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(index);

    IFC_RETURN(ClearValue(dp));
    return S_OK;
}

// Checks whether the given property is set locally on the DO.
bool CDependencyObject::HasLocalOrModifierValue(_In_ const CDependencyProperty* dp)
{
    bool hasLocalValue = !(IsPropertyDefault(dp) || IsPropertySetByStyle(dp));
    return hasLocalValue;
}

// Re-evaluate property value.
_Check_return_ HRESULT CDependencyObject::InvalidateProperty(
    _In_ const CDependencyProperty* dp,
    _In_ BaseValueSource baseValueSource)
{
    // Ensure that the property is actually a property on the passed in class.
    if (!dp->IsSparse() && // For now, skip validation on sparse properties, because we never did this validation before either.
        !dp->IsInheritedAttachedPropertyInStorageGroup() &&
        !MetadataAPI::IsAssignableFrom(dp->GetTargetType()->GetIndex(), GetTypeIndex()))
    {
        IFCEXPECT_RETURN(false);
    }

    // If new base value source of the property is lower precedence than old
    // base value source, the base value need not be changed. Currently,
    // this is done only for Template to limit scope of fix close to SL4 ship
    // date. In SL5, this needs to be extended to all properties, after adding
    // a quirk to retain behavior for SL4 apps.
    if (dp->GetIndex() == KnownPropertyIndex::Control_Template)
    {
        // Currently GetBaseValueSource returns BaseValueSourceStyle both for
        // style and built-in style, because built-in styles don't have a
        // bitfield similar to CFrameworkElement.m_setByStyle. Since
        // built-in style is applied only once, this is OK (see
        // CControl.m_fIsBuiltInStyleApplied). If the old value source is
        // BaseValueSourceStyle and the new value source is
        // BaseValueSourceBuiltInStyle, we can assume that old value source is
        // a style and not a built-in style.
        BaseValueSource oldBaseValueSource = GetBaseValueSource(dp);
        if ((baseValueSource != BaseValueSourceUnknown) && (baseValueSource < oldBaseValueSource))
        {
            return S_OK;
        }
    }

    // Update value by re-evaluating.
    IFC_RETURN(UpdateEffectiveValue(
        UpdateEffectiveValueParams(
            dp,
            CValue::Empty(),
            GetModifiedValue(dp),
            ValueOperationReevaluate)));

    return S_OK;
}

bool CDependencyObject::IsAnimatedProperty(_In_ const CDependencyProperty* const dp) const
{
    auto modifiedValue = GetModifiedValue(dp);
    if (modifiedValue)
    {
        return modifiedValue->IsAnimated();
    }
    return false;
}

bool CDependencyObject::IsAnimatedPropertyOverwritten(_In_ const CDependencyProperty* const dp) const
{
    auto modifiedValue = GetModifiedValue(dp);
    if (modifiedValue)
    {
        return modifiedValue->IsAnimatedValueOverwritten();
    }
    return false;
}

_Check_return_ xref_ptr<CDependencyObject> CDependencyObject::TryGetAnimatedPropertySource(_In_ const CDependencyProperty* dp)
{
    auto modifiedValue = GetModifiedValue(dp);
    if (modifiedValue && modifiedValue->IsAnimated())
    {
        return modifiedValue->GetAnimatedValueSource();
    }

    return nullptr;
}

// Removes the weak reference back from this child DO to this DO, its parent.
_Check_return_ HRESULT CDependencyObject::ResetReferenceFromChild(_In_ CDependencyObject* child)
{
    if (child->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>())
    {
        IFC_RETURN(((CCollection*)child)->SetAndPropagateOwner(nullptr));

        // If we're this object's parent, we need to clear it.
        if (child->IsParentAware() && (child->GetParentInternal(false) == this))
        {
            IFC_RETURN(child->RemoveParent(this));
        }
    }
    else if (child->DoesAllowMultipleParents()
        || (!child->DoesAllowMultipleAssociation() && (child->GetParentInternal(false) == this)))
    {
        IFC_RETURN(child->RemoveParent(this));
    }

    // Property value is not associated with this DO anymore
    if (child->DoesAllowMultipleAssociation())
    {
        child->SetAssociated(false, nullptr);
    }

    return S_OK;
}

// Cleans up references from child DOs back to this parent DO by iterating through all of this object's DO-valued properties.
// Should be called only on CDependencyObject destruction.
void CDependencyObject::ResetReferencesFromChildren()
{
    if (GetContext() != nullptr)
    {
        const CObjectDependencyProperty* pNullObjectProperty = MetadataAPI::GetNullObjectProperty();

        for (const CObjectDependencyProperty* pObjectProperty = GetClassInformation()->GetFirstObjectProperty();
             pObjectProperty != pNullObjectProperty;
             pObjectProperty = pObjectProperty->GetNextProperty())
        {
            auto dp = MetadataAPI::GetDependencyPropertyByIndex(pObjectProperty->m_nPropertyIndex);
            auto cdo = GetDependencyObjectFromPropertyStorage(dp);

            if (cdo != nullptr)
            {
                VERIFYHR(ResetReferenceFromChild(cdo));
            }
        }
    }

    ResetReferencesFromSparsePropertyValues(true /* isDestructing */);
}

// Reset references to this DO from sparse property object values
void CDependencyObject::ResetReferencesFromSparsePropertyValues(bool isDestructing)
{
    if (m_pValueTable != nullptr)
    {
        // To protect against reentrancy, copy into a temporary vector before iterating.
        Jupiter::arena<DefaultSparseArenaSize> localArena;

        // Don't fetch non-owned IInspectables.
        auto tempValues = GetSparseValueEntries<true>(localArena);

        //  Cleans up references from child DOs back to this parent DO by iterating through all of this object's DO-valued properties.
        for (auto& entry : tempValues)
        {
            auto pDP = MetadataAPI::GetDependencyPropertyByIndex(entry.first);
            auto pDO = entry.second.value.AsObject();
            const bool removeFromTable = !pDP->HadFieldInBlue();

            if (pDO != nullptr)
            {
                if (pDP->IsVisualTreeProperty() &&
                    !IsDependencyPropertyBackReference(pDP->GetIndex()))
                {
                    VERIFYHR(ResetReferenceFromChild(pDO));
                }

                // Property value is not associated with this DO anymore.
                // Shareable objects have a share count & ResetReferencesFromSparsePropertyValues can be called
                // multiple times, so ensure SetAssociated(false) is called only when the property value is being
                // removed from the table.
                if (removeFromTable || isDestructing)
                {
                    if (pDO->DoesAllowMultipleAssociation())
                    {
                        pDO->SetAssociated(false, nullptr);
                    }
                }
            }

            // Even though we checked m_pValueTable for null, check again. The ResetReferenceFromChild call above could have released
            // the DXaml peer of some child object, which could have released the final reference on the DXaml peer for this DO. When
            // the DXaml peer of this DO is deleted, OnFinalRelease will call into ClearPeerReferences, which causes reentrancy into
            // this method. At the end of this method in the reentrant call, m_pValueTable will be released. When we return out of
            // the ResetReferenceFromChild call, m_pValueTable could be gone.
            if (m_pValueTable != nullptr)
            {
                if (removeFromTable)
                {
                    auto iter = m_pValueTable->find(entry.first);
                    if (iter != m_pValueTable->end())
                    {
                        CValue oldvalue = std::move(entry.second.value);

                        {
                            // the garbage collection walk iterates over the m_pValueTable
                            // and hence entries need to be modified in a gc thread safe manner
                            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                            m_pValueTable->erase(iter);
                        }
                    }
                }
                else
                {
                    auto iter = m_pValueTable->find(entry.first);

                    if (iter != m_pValueTable->end())
                    {
                        // TODO: Remove this hack once we have proper lifetime semantics in the core.
                        // Remove the strong reference that the above CopyConverted call made on the core side, because we will
                        // make a tracker reference on the framework side later on (see the call to UpdatePeerReferenceToProperty).
                        auto& effectiveValue = iter->second;

                        if ((effectiveValue.value.GetType() == valueObject) &&
                            !effectiveValue.value.OwnsValue() &&
                            ShouldReleaseCoreObjectWhenTrackingPeerReference() &&
                            ShouldTrackWithPeerReferenceToProperty(pDP, effectiveValue.value, effectiveValue.IsSetByStyle()))
                        {
                            CValue temp = std::move(effectiveValue.value);
                            effectiveValue.value.SetObjectAddRef(temp.AsObject());
                        }
                        else if (effectiveValue.value.GetType() == valueIInspectable &&
                            !effectiveValue.value.OwnsValue() &&
                            ShouldTrackWithPeerReferenceToProperty(pDP, effectiveValue.value, effectiveValue.IsSetByStyle()))
                        {
                            CValue temp = std::move(effectiveValue.value);
                            effectiveValue.value.SetIInspectableAddRef(temp.AsIInspectable());
                        }
                    }
                }
            }
        }

        if (m_pValueTable != nullptr && m_pValueTable->empty())
        {
            // the garbage collection walk iterates over the m_pValueTable
            // and replacing the table must be done in a gc thread safe manner
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

            m_pValueTable.reset();
        }
    }
}

#pragma region GetValue related methods
// Gets the base value of an animated property. If the animation doesn't exist, then it returns a regular GetValue.
_Check_return_ HRESULT CDependencyObject::GetAnimationBaseValue(_In_ const CDependencyProperty* dp, _Out_ CValue* pValue)
{
    auto modifiedValue = GetModifiedValue(dp);

    if (modifiedValue && modifiedValue->IsAnimated())
    {
        IFC_RETURN(modifiedValue->GetBaseValue(pValue));
    }
    else
    {
        // If animation doesn't exist, return regular GetValue for the property.
        IFC_RETURN(GetValue(dp, pValue));
    }

    return S_OK;
}
// Gets the value of the animated property. Can't use a regular GetValue because it could be overwritten locally.
_Check_return_ HRESULT CDependencyObject::GetAnimatedValue(_In_ const CDependencyProperty* const dp, _Out_ CValue* pValue) const
{
    auto modifiedValue = GetModifiedValue(dp);
    pValue->ReleaseAndReset();

    IFCEXPECT_ASSERT_RETURN(modifiedValue);

    // We should know we are animated if calling this method.
    ASSERT(modifiedValue->IsAnimated());
    IFC_RETURN(modifiedValue->GetAnimatedValue(pValue));

    return S_OK;
}

//  Retrieves the default value for the property on this type.
//  Parameter pAllocated indicates if the defaultValue buffer was initialized in the method.
_Check_return_ HRESULT CDependencyObject::GetDefaultValue(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pDefaultValue)
{
    return pDP->GetDefaultValue(GetContext(), /* pReferenceObject */ this, GetClassInformation(), pDefaultValue);
}

_Check_return_ HRESULT CDependencyObject::GetDefaultValue(
    _In_ const CDependencyProperty* pDP,
    _In_ const CClassInfo* pInfo,
    _Out_ CValue* pDefaultValue)
{
    return pDP->GetDefaultValue(GetContext(), /* pReferenceObject */ this, pInfo, pDefaultValue);
}

_Check_return_ HRESULT CDependencyObject::GetEffectiveValueInField(_In_ const CDependencyProperty* dp, _Out_ CValue* pValue)
{
    // For now all properties are stored directly in the object at the specified offset.
    XHANDLE field = GetPropertyOffset(dp, /* forGetValue */ true); // default OK for GetValue
    IFCPTR_RETURN(field);

    switch (dp->GetStorageType())
    {
        case valueFloat:
            pValue->SetFloat(*((FLOAT*)field));
            break;

        case valueSigned:
            pValue->SetSigned(*((INT32*)field));
            break;

        case valueTypeHandle:
            pValue->SetTypeHandle(*((KnownTypeIndex*)field));
            break;

        case valueEnum:
            pValue->Set<valueEnum>({ *reinterpret_cast<uint32_t*>(field), dp->GetPropertyType()->GetIndex() });
            break;
        case valueEnum8:
            pValue->Set<valueEnum8>({ *reinterpret_cast<uint8_t*>(field), dp->GetPropertyType()->GetIndex() });
            break;

        case valueBool:
            pValue->Set<valueBool>(*((bool*)field));
            break;

        case valueColor:
            pValue->SetColor(*((UINT32*)field));
            break;

        case valueString:
            pValue->SetString(reinterpret_cast<xstring_ptr*>(field)[0]);
            break;

        case valueFloatArray:
            pValue->WrapFloatArray(*((UINT32*)field), *((FLOAT**)((UINT8*)field + sizeof(UINT32))));
            break;

        case valuePointArray:
            pValue->WrapPointArray(*((UINT32*)field), *((XPOINTF**)((UINT8*)field + sizeof(UINT32))));
            break;

        case valueSize:
            pValue->WrapSize((XSIZEF*)field);
            break;

        case valuePoint:
            pValue->WrapPoint((XPOINTF*)field);
            break;

        case valueRect:
            pValue->WrapRect((XRECTF*)field);
            break;

        case valueDouble:
            pValue->SetDouble(*(DOUBLE*)field);
            break;

        case valueGridLength:
            pValue->WrapGridLength((XGRIDLENGTH*)field);
            break;

        case valueThickness:
            pValue->WrapThickness((XTHICKNESS*)field);
            break;

        case valueCornerRadius:
            pValue->WrapCornerRadius((XCORNERRADIUS*)field);
            break;

        case valueObject:
            IFC_RETURN(GetEffectiveValueInField_Object(dp, *((CDependencyObject**)field), pValue));
            break;

        case valueVO:
            pValue->SetAddRef<valueVO>(*reinterpret_cast<Flyweight::PropertyValueObjectBase**>(field));
            break;

        case valueAny:
            if (!IsPropertyDefault(dp))
            {
                IFC_RETURN(pValue->CopyConverted(*((CValue*)field)));
            }
            else
            {
                // Default for a "valueAny" property type is an object reference of null.
                pValue->SetObjectNoRef(nullptr);
            }
            break;
        default:
            // Handle any types we don't expect
            ASSERT(false);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::GetEffectiveValueInField_Object(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* objectAtOffset, _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;

    if (!objectAtOffset && pDP->IsOnDemandProperty())
    {
        auto core = GetContext();

        // This is an OnDemand property..it is only created when requested ..so create one...
        CREATEPARAMETERS cp(core);

        hr = pDP->CreateDefaultValueObject(core, pValue);

        if (SUCCEEDED(hr))
        {
            bool isPropertyDefault = IsPropertyDefault(pDP);
            hr = SetValue(pDP, *pValue);

            //restore the m_valid bitfield value for this property.
            if (isPropertyDefault)
            {
                SetPropertyIsDefault(pDP);
            }

            if (FAILED(hr))
            {
                // Release previously held interface, but keep the ValueType the same.
                pValue->SetObjectNoRef(nullptr);
            }

            // don't need to addref here because we have the
            // implicit reference from having created the object.
        }
    }
    else
    {
        pValue->SetObjectAddRef(objectAtOffset);
    }

    return hr;
}

// Returns true if the effective value for the given sparse property is currently set.
bool CDependencyObject::IsEffectiveValueInSparseStorage(_In_ KnownPropertyIndex ePropertyIndex) const
{
    bool found = false;

    if (m_pValueTable != nullptr)
    {
        auto it = m_pValueTable->find(ePropertyIndex);
        if (it != m_pValueTable->end())
        {
            found = true;
        }
    }

    return found;
}

// Gets the effective value for a sparse property.
_Check_return_ HRESULT CDependencyObject::GetEffectiveValueInSparseStorage(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue)
{
    if (m_pValueTable != nullptr)
    {
        auto it = m_pValueTable->find(pDP->GetIndex());
        if (it != m_pValueTable->end())
        {
            pValue->CopyConverted(it->second.value);
            return S_OK;
        }
    }

    if (pDP->IsOnDemandProperty())
    {
        IFC_RETURN(pDP->CreateDefaultValueObject(GetContext(), pValue));
        IFC_RETURN(SetValue(pDP, *pValue));
        SetPropertyIsDefault(pDP);
    }
    else
    {
        IFC_RETURN(GetDefaultValue(pDP, pValue));
    }

    return S_OK;
}

// Get a value from the dependency object.
_Check_return_ HRESULT CDependencyObject::GetEffectiveValueViaMethodCall(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue)
{
    METHODPFN pfn = pDP->GetPropertyMethod();
    ASSERT(pDP->IsPropMethodCall() && pfn);

    IFC_RETURN(pfn(this, 0, /* pArgs */ nullptr, /* pValueOuter */ nullptr, pValue));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::GetValue(
    _In_ const CDependencyProperty* dp,
    _Out_ CValue* pValue)
{
    ASSERT(dp);

    if (dp->IsInheritedAttachedPropertyInStorageGroup())
    {
        // Inherited attached properties in storage groups do not have
        // slots on the class, rather unset/set state is managed entirely
        // within the storage group.
        IFC_RETURN(GetValueInternal(dp, pValue));
    }
    else if (dp->IsInherited() && !dp->IsStorageGroup())
    {
        IFC_RETURN(GetValueInherited(dp, pValue));
    }
    else
    {
        IFC_RETURN(GetValueInternal(dp, pValue));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::GetValueByIndex(
    _In_ KnownPropertyIndex index,
    _Out_ CValue* value) const
{
    // TODO: Add a GetValue() const version that guarantees there are no side-effects.
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(index);

    IFC_RETURN(const_cast<CDependencyObject*>(this)->GetValue(dp, value));
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetValueByIndex(
    _In_ KnownPropertyIndex index,
    _In_ const CValue& value)
{
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(index);

    IFC_RETURN(SetValue(dp, value));
    return S_OK;
}

// Starting at the requested dependency object walks its ancestors looking for one in which the requested property has been set
// explicitly, and retuns that set value.
_Check_return_ HRESULT CDependencyObject::GetValueInherited(_In_ const CDependencyProperty* dp, _Out_ CValue* pValue)
{
    CDependencyObject* obj = this;
    KnownTypeIndex targetClassIndex = dp->GetDeclaringType()->GetIndex();

    // Determine the dependency property and type for this index.
    // For an inherited property, iterate up through ancestors if necessary to find a dependency object that has had this value set.
    while (obj != nullptr)
    {
        if (obj->OfTypeByIndex(targetClassIndex) && !obj->IsPropertyDefault(dp))
        {
            // We found a DO that has this property, and it has been set locally.
            IFC_RETURN(obj->GetValueInternal(dp, pValue));
            break;
        }

        obj = obj->GetInheritanceParentInternal(UseLogicalParent(dp->GetIndex()));
    }

    if (obj == nullptr)
    {
        // Inherited property not set in any ancestor
        IFC_RETURN(GetDefaultInheritedPropertyValue(dp, pValue));
    }

    return S_OK;
}

XHANDLE CDependencyObject::GetPropertyOffset(_In_ const CDependencyProperty* pDP, _In_ bool forGetValue)
{
    if (pDP->IsStorageGroup())
    {
        // Ensure that the storage is there and inheritance is up to date.
        if (FAILED((pDP->GetGroupCreator())(this, pDP, forGetValue)))
        {
            return nullptr;
        }

        // Get the address of the pointer to the group...
        XHANDLE pGroup = READ_OFFSET(this, pDP->GetOffset());
        // ..to the group itself...
        XHANDLE* ppGroup = (XHANDLE*)pGroup;
        // ...to the actual location inside of the group.
        return ((void*)&((UINT8*)*ppGroup)[pDP->GetGroupOffset()]);
    }
    else
    {
        return READ_OFFSET(this, pDP->GetOffset());
    }
}

// Internal helper for GetValue. By default copies member variable to *pValue.
// Also implements:
//     PROP_STORAGE_GROUP via GetPropertyOffset()
//     PROP_METHOD_CALL via GetEffectiveValueViaMethodCall()
//     PROP_ON_DEMAND as a check using pdp->IsPropertyOnDemand()
_Check_return_ HRESULT CDependencyObject::GetValueInternal(_In_  const CDependencyProperty* pDP, _Out_ CValue* pValue)
{
    if (pDP->IsSparse())
    {
        return GetEffectiveValueInSparseStorage(pDP, pValue);
    }
    else if (pDP->IsPropMethodCall())
    {
        // we need to call a method and package up the return value....
        return GetEffectiveValueViaMethodCall(pDP, pValue);
    }
    else
    {
        return GetEffectiveValueInField(pDP, pValue);
    }
}
#pragma endregion

#pragma region SetValue related methods
_Check_return_ HRESULT CDependencyObject::AppendEffectiveValueToCollectionInField(_In_ const EffectiveValueParams& args, _In_ XHANDLE field)
{
    xref_ptr<CDependencyObject> collectionDO;
    const CDependencyProperty* pDP = args.m_pDP;
    CValue* pValue = const_cast<CValue*>(&args.m_value);

    // Get the DP's class info.
    const CClassInfo* propertyType = pDP->GetPropertyType();

    // Collections have one level of inheritance - use that info here and get the base class info of the DP type.
    const CClassInfo* propertyTypeBaseClass = propertyType->GetBaseType();

    // CTransformCollection needs special casing since it derives from CTransform and not CCollection.
    if (propertyTypeBaseClass->GetIndex() == KnownTypeIndex::PresentationFrameworkCollection)
    {
        // If this is indeed a collection member and not set, create one on the fly as needed.
        if (nullptr == ((void**)field)[0])
        {
            CREATEPARAMETERS cp(GetContext());

            const CREATEPFN pfnCreate = pDP->GetPropertyType()->GetCoreConstructor();
            IFC_RETURN(pfnCreate(collectionDO.ReleaseAndGetAddressOf(), &cp));

            // Make sure we mark this collection as associated to this object
            collectionDO->SetAssociated(true, this);
            IFC_RETURN(collectionDO->AddParent(this, false, pDP->GetRenderChangedHandler()));

            ((void**)field)[0] = collectionDO.get();
        }
        else
        {
            collectionDO.attach(((CDependencyObject**)field)[0]);
        }

        // Try to store the object in the collection.
        IFC_RETURN(collectionDO->SetValue(collectionDO->GetContentProperty(), *pValue));

        // Backing field owns the collection object.
        collectionDO.detach();
    }
    else
    {
        // The types are incompatible
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ClearEffectiveValueInObjectField(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* previousDO)
{
    if (pDP->IsStorageGroup())
    {
        // We support releasing inherited property storage groups.
        CDependencyObject* pObject = MapPropertyAndGroupOffsetToDO(pDP->GetOffset(), pDP->GetGroupOffset());

        // Release the object.
        ReleaseInterface(pObject);
    }
    else
    {
        RELEASE(pDP->GetOffset()); // note: we do not support other types of storagegroup with objects in them
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ClearEffectiveValueInSparseStorage(_In_ const CDependencyProperty* dp)
{
    CValue value;
    CValue oldValue;
    bool propertyChangedValue = false;
    return ClearEffectiveValueInSparseStorage(
        EffectiveValueParams(dp, value, BaseValueSourceUnknown),
        oldValue,
        /* ppOldValueOuter */ nullptr,
        propertyChangedValue);
}

_Check_return_ HRESULT CDependencyObject::ClearEffectiveValueInSparseStorage(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue)
{
    propertyChangedValue = false;
    bool oldValueFound = false;

    if (m_pValueTable != nullptr)
    {
        auto iter = m_pValueTable->find(args.m_pDP->GetIndex());
        if (iter != m_pValueTable->end())
        {
            oldValueFound = true;
            ASSERT(iter->second.value.GetType() != valueAny);

            // the garbage collection walk iterates over the m_pValueTable
            // and modifying entries needs to be done in a gc thread safe manner
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                // Move value out of storage, but hold on to it locally for our change notification.
                oldValue = std::move(iter->second.value);
            }

            bool oldValueIsCached = false;

            if (oldValue.GetType() == valueObject)
            {
                CDependencyObject* pOldObjectNoRef = oldValue.AsObject();

                if (pOldObjectNoRef != nullptr)
                {
                    // If the object is not a FrameworkElement, then clear its inheritance context.
                    if (!pOldObjectNoRef->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
                    {
                        pOldObjectNoRef->SetParentForInheritanceContextOnly(nullptr);
                        IFC_RETURN(pOldObjectNoRef->NotifyInheritanceContextChanged());
                    }

                    IFC_RETURN(LeaveEffectiveValue(args.m_pDP, pOldObjectNoRef));

                    // The last few calls may have incurred another set/clear in our sparse table and invalidated our iterator. Let's fetch a fresh one
                    iter = m_pValueTable->find(args.m_pDP->GetIndex());
                    ASSERT(iter != m_pValueTable->end());

                    // Check if we protected the old reference's peer.
                    oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, oldValue, iter->second.IsSetByStyle());
                }
            }
            else if (oldValue.GetType() == valueIInspectable)
            {
                // Check if we protected the old reference's peer.
                oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, oldValue, iter->second.IsSetByStyle());
            }

            // Notify the framework layer of the change.
            IFC_RETURN(UpdatePeerReferenceToProperty(args.m_pDP, iter->second.value, oldValueIsCached, /* newValueNeedsCaching */ false, /* valueOuterNoRef */ nullptr, ppOldValueOuter));

            // m_value is the default value. See if the effective value really changes because of our ClearValue operation.
            propertyChangedValue = (oldValue != args.m_value);

            // the garbage collection walk iterates over the m_pValueTable
            // and hence entries need to be modified in a gc thread safe manner
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                m_pValueTable->erase(iter);
            }
        }
    }

    // No need to call SetBaseValueSource, because we cleared the EffectiveValue entry that would store
    // this state anyway. After a ClearValue operation, the value source is BaseValueSourceDefault, and that's
    // what GetBaseValueSource will return when there's no EffectiveValue entry.

    if (!oldValueFound)
    {
        IFC_RETURN(GetDefaultValue(args.m_pDP, &oldValue));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::EnterEffectiveValue(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* obj)
{
    if (pDP->RequiresMultipleAssociationCheck() || obj->DoesAllowMultipleAssociation())
    {
        obj->SetAssociated(true, this);
    }

    if (pDP->IsVisualTreeProperty())
    {
        EnterParams enterParams(
            /*isLive*/                IsActive(),
            /*skipNameRegistration*/  false,
            /*coercedIsEnabled*/      GetCoercedIsEnabled(),
            /*useLayoutRounding*/     GetUseLayoutRounding(),
            /*visualTree*/            VisualTree::GetForElementNoRef(this, LookupOptions::NoFallback)
            );

        enterParams.fCheckForResourceOverrides = ShouldCheckForResourceOverrides();

        // This will let us register keyboard accelerator collections added through codebehind.
        if (pDP->GetIndex() == KnownPropertyIndex::UIElement_KeyboardAccelerators
            && static_cast<CDOCollection*>(obj)->GetCount()
            )
        {
            enterParams.fIsForKeyboardAccelerator = true;
        }

        // Set the parent of collection items or the DO so we can can do a bottom-up tree walk to propagate dirty flags.
        if (obj->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>())
        {
            // Store the owner and association, if the collection requires it.
            CCollection* collectionObj = static_cast<CCollection*>(obj);
            IFC_RETURN(collectionObj->SetAndPropagateOwner(this, pDP->GetRenderChangedHandler()));

            if (pDP->AffectsMeasure())
            {
                collectionObj->SetAffectsOwnerMeasure(true);
            }

            if (pDP->AffectsArrange())
            {
                collectionObj->SetAffectsOwnerArrange(true);
            }

            if (obj->IsParentAware())
            {
                // Be the parent for this collection.
                IFC_RETURN(obj->AddParent(this, false, pDP->GetRenderChangedHandler()));
            }
        }
        else if (obj->IsParentAware())
        {
            IFC_RETURN(obj->AddParent(this, true, pDP->GetRenderChangedHandler()));

            if (pDP->IsInherited())
            {
                // This bit is only cleared for CMultiParentShareableDOs (in RemoveParent),
                // so it should only be set for those types.  Setting it on a single-parent
                // DO is unexpected since it wouldn't be cleaned up correctly.
                ASSERT(obj->DoesAllowMultipleParents());
                obj->SetIsValueOfInheritedProperty(true);
            }
        }

        IFC_RETURN(obj->Enter(GetStandardNameScopeOwner(), enterParams));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::EnterSparseProperties(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params)
{
    // To protect against reentrancy, copy into a temporary vector before iterating.
    Jupiter::arena<DefaultSparseArenaSize> localArena;

    // Don't fetch non-owned IInspectables.
    auto tempValues = GetSparseValueEntries<true>(localArena);

    // For SparseProperties we don't propogate down the visualTree pointer.  These elements seem to be
    // able to have parents in different trees.  TODO: Investigate this more.
    // Bug 19548424: Investigate places where an element entering the tree doesn't have a unique VisualTree ptr
    EnterParams newParams(params);
    newParams.visualTree = nullptr;

    for (auto& entry : tempValues)
    {
        auto pDP = MetadataAPI::GetDependencyPropertyByIndex(entry.first);
        auto pDO = entry.second.value.AsObject();

        // TODO: Consolidate with EnterProperties list.
        if (pDO != nullptr &&
            pDP->IsVisualTreeProperty())
        {
            IFC_RETURN(EnterObjectProperty(pDO, namescopeOwner, newParams));
        }

        if (pDP->NeedsInvoke())
        {
            IFC_RETURN(Invoke(pDP, namescopeOwner, newParams.fIsLive));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::EnterObjectProperty(_In_ CDependencyObject* pDO, _In_ CDependencyObject* namescopeOwner, _In_ EnterParams params)
{
    CDependencyObject *pAdjustedNamescopeOwner = namescopeOwner;

    // only want to follow the logical tree
    CFrameworkElement *pFE = do_pointer_cast<CFrameworkElement>(pDO);
    if (pFE && pFE->GetLogicalParentNoRef() != this)
    {
        // name registration should follow the logical tree. We expect that
        // a control that takes logical ownership of a node in the tree will
        // take care of name registration.
        params.fSkipNameRegistration = TRUE;

        // the passed in namescopeowner is incorrect at this point.
        // Passing in the nso is a perf optimization and the nso is updated at
        // namescope boundaries. In the case where an element is actually pulled in from
        // another part of the tree (think content and ContentControl) the NSO is probably
        // something else. GetNamescopeOwner will follow logical tree.
        pAdjustedNamescopeOwner = pDO->GetStandardNameScopeOwner();
    }

    IFC_RETURN(pDO->Enter(pAdjustedNamescopeOwner, params));
    return S_OK;
}


// Evaluate base value in the dependency object.
// The base value is evaluated using following precedence rules:
// 1. Local value
// 2. Style
// 3. Built-in style
// 4. Default value
_Check_return_ HRESULT CDependencyObject::EvaluateBaseValue(
    _In_ const CDependencyProperty* dp,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ UINT32 valueOperation,
    _Out_ CValue* pBaseValue,
    _Out_ BaseValueSource* pBaseValueSource,
    _Out_ bool* valueSetNeeded)
{
    bool gotValue = false;

    *valueSetNeeded = true;

    // If operation is not clear value, and there is local value, return that.
    if (valueOperation != ValueOperationClearValue)
    {
        if (pModifiedValue)
        {
            if (pModifiedValue->GetBaseValueSource() == BaseValueSourceLocal)
            {
                if (pModifiedValue->IsBaseValueThemeResource())
                {
                    IFC_RETURN(pModifiedValue->GetBaseValueThemeResource(pBaseValue));
                }
                else
                {
                    IFC_RETURN(pModifiedValue->GetBaseValue(pBaseValue));
                }
                *pBaseValueSource = BaseValueSourceLocal;
                gotValue = true;
            }
        }
        else
        {
            if ((!IsPropertyDefault(dp) && !IsPropertySetByStyle(dp)) || IsPropertyTemplateBound(dp))
            {
                // Base value is already set, so need not be set again.
                *valueSetNeeded = false;
                gotValue = true;
            }
        }
    }

    if (!gotValue)
    {
        // Try to get value from style.
        if(OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            IFC_RETURN(static_cast<CFrameworkElement*>(this)->GetValueFromStyle(dp, pBaseValue, &gotValue));
        }

        if (gotValue)
        {
            *pBaseValueSource = BaseValueSourceStyle;
        }
        else
        {
            // Try to get value from built-in style.
            if(OfTypeByIndex<KnownTypeIndex::Control>())
            {
                IFC_RETURN(static_cast<CControl*>(this)->GetValueFromBuiltInStyle(dp, pBaseValue, &gotValue));
            }

            if (gotValue)
            {

                *pBaseValueSource = BaseValueSourceBuiltInStyle;
            }
            else
            {
                // Get default value.
                IFC_RETURN(GetDefaultValue(dp, pBaseValue));
                *pBaseValueSource = BaseValueSourceDefault;
            }
        }
    }

    return S_OK;
}

// Evaluate effective value in the dependency object.
// The effective value is evaluated using following precedence rules:
// 1. Modified value from source like Animation
// 2. Base value
_Check_return_ HRESULT CDependencyObject::EvaluateEffectiveValue(
    _In_ const CDependencyProperty* dp,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ UINT32 valueOperation,
    _Out_ CValue* pEffectiveValue,
    _Out_ BaseValueSource* pBaseValueSource,
    _Out_ bool* valueSetNeeded)
{
    CValue baseValue;
    bool baseValueSetNeeded = true;

    *valueSetNeeded = true;

    // Evaluate base value
    IFC_RETURN(EvaluateBaseValue(dp, pModifiedValue, valueOperation, &baseValue,
        pBaseValueSource, &baseValueSetNeeded));

    // If there is a modifier, set the new base value in pModifiedValue, so it
    // can be set when modifiers are removed
    if (pModifiedValue)
    {
        IFC_RETURN(pModifiedValue->SetBaseValue(baseValue, *pBaseValueSource));

        IFC_RETURN(pModifiedValue->GetEffectiveValue(pEffectiveValue));

        // If there are no modifiers, the effective value is the base value.
        // So return flag if the value needs to be set. (If a base value
        // is already set, it need not be set again.)
        if (!pModifiedValue->HasModifiers())
        {
            *valueSetNeeded = baseValueSetNeeded;
        }
    }
    else
    {
        IFC_RETURN(pEffectiveValue->CopyConverted(baseValue));
        *valueSetNeeded = baseValueSetNeeded;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::LeaveEffectiveValue(_In_ const CDependencyProperty* pDP, _In_ CDependencyObject* obj)
{
    if (pDP->IsVisualTreeProperty())
    {
        LeaveParams leaveParams(
            /*fIsActive*/             IsActive(),
            /*fSkipNameRegistration*/ false,
            /*fCoercedIsEnabled*/     GetCoercedIsEnabled(),
            /*fVisualTreeBeingReset*/ false
            );

        // This will let us de-register keyboard accelerator collections added through codebehind
        if (pDP->GetIndex() == KnownPropertyIndex::UIElement_KeyboardAccelerators
            && static_cast<CDOCollection*>(obj)->GetCount()
            )
        {
            leaveParams.fIsForKeyboardAccelerator = true;
        }

        // If this object is leaving the 'live' tree then we need to stop
        // animations, downloads, etc.
        IFC_RETURN(obj->Leave(GetStandardNameScopeOwner(), leaveParams));

        // Reset parent property if removed from the tree.
        if (obj->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>())
        {
            IFC_RETURN(((CCollection*)obj)->SetAndPropagateOwner(nullptr));
            ((CCollection*)obj)->SetAffectsOwnerMeasure(false);
            ((CCollection*)obj)->SetAffectsOwnerArrange(false);

            // If we're this collection's parent, we need to clear it.
            if (obj->IsParentAware() && (obj->GetParentInternal(false) == this))
            {
                VERIFYHR(obj->RemoveParent(this));
            }
        }
        else
        {
            IFC_RETURN(obj->RemoveParent(this));
        }
    }

    if (pDP->RequiresMultipleAssociationCheck() || obj->DoesAllowMultipleAssociation())
    {
        obj->SetAssociated(false, this);
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::LeaveSparseProperties(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params)
{
    // To protect against reentrancy, copy into a temporary vector before iterating.
    Jupiter::arena<DefaultSparseArenaSize> localArena;

    // Don't fetch non-owned IInspectables.
    auto tempValues = GetSparseValueEntries<true>(localArena);

    // Enumerate all the sparse properties with values and leave as needed.
    for (auto& entry : tempValues)
    {
        // TODO: Consolidate with EnterProperties list.
        auto pDP = MetadataAPI::GetDependencyPropertyByIndex(entry.first);
        auto pDO = entry.second.value.AsObject();

        if (pDO != nullptr &&
            pDP->IsVisualTreeProperty())
        {
            IFC_RETURN(LeaveObjectProperty(pDO, namescopeOwner, params));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::LeaveObjectProperty(_In_ CDependencyObject* pDO, _In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params)
{
    CDependencyObject *pAdjustedNamescopeOwner = namescopeOwner;

    // only want to follow the logical tree
    CFrameworkElement *pFE = do_pointer_cast<CFrameworkElement>(pDO);
    if (pFE && pFE->GetLogicalParentNoRef() != this)
    {
        // name registration should follow the logical tree. We expect that
        // a control that takes logical ownership of a node in the tree will
        // take care of name registration.
        params.fSkipNameRegistration = TRUE;

        // the passed in namescopeowner is incorrect at this point.
        // Passing in the nso is a perf optimization and the nso is updated at
        // namescope boundaries. In the case where an element is actually pulled in from
        // another part of the tree (think content and ContentControl) the NSO is probably
        // something else. GetNamescopeOwner will follow logical tree.
        pAdjustedNamescopeOwner = pDO->GetStandardNameScopeOwner();
    }
    IFC_RETURN(pDO->Leave(pAdjustedNamescopeOwner, params));
    return S_OK;
}

// Returns local value or valueNull if default or set by style.
_Check_return_ HRESULT CDependencyObject::ReadLocalValue(
    _In_ const CDependencyProperty* dp,
    _Inout_ CValue* pValue,
    _Inout_ bool* hasLocalValue,
    _Inout_ bool* isTemplateBound)
{
    if (HasLocalOrModifierValue(dp))
    {
        *hasLocalValue = true;

        if (IsPropertyTemplateBound(dp))
        {
            pValue->SetNull();
            *isTemplateBound = true;
        }
        else
        {
            // Could be animation modifier.
            auto modifiedValue = GetModifiedValue(dp);

            if (modifiedValue != nullptr)
            {
                if (modifiedValue->GetBaseValueSource() == BaseValueSourceLocal)
                {
                    IFC_RETURN(modifiedValue->GetBaseValue(pValue));
                }
                else
                {
                    *hasLocalValue = false;
                    pValue->SetNull();
                }
            }
            else
            {
                IFC_RETURN(GetValue(dp, pValue));
            }
        }
    }
    else
    {
        // Default or set by style.
        *hasLocalValue = false;
        pValue->SetNull();
    }

    return S_OK;
}

// Set animated value. Called by Animation value source to animate a property.
_Check_return_ HRESULT CDependencyObject::SetAnimatedValue(
    _In_ const CDependencyProperty* dp,
    _In_ const CValue& value,
    _In_ xref_ptr<CDependencyObject> sourceSetter)
{
    auto modifiedValue = GetModifiedValue(dp);

    if (modifiedValue == nullptr)
    {
        IFC_RETURN(CreateModifiedValue(dp, modifiedValue));
    }

    if (modifiedValue->IsEffectiveValueThemeResource())
    {
        // Clear any theme reference associated with the animated value.
        // UpdateEffectiveValue will re-set the reference for the animated value if necessary.
        ClearThemeResource(dp);
    }

    IFC_RETURN(modifiedValue->SetAnimatedValue(value, sourceSetter));

    IFC_RETURN(UpdateEffectiveValue(
        UpdateEffectiveValueParams(
            dp,
            CValue::Empty(),
            modifiedValue,
            ValueOperationDefault)));

    return S_OK;
}

// Set the effective value in the dependency object.
_Check_return_ HRESULT CDependencyObject::SetEffectiveValue(_In_ const EffectiveValueParams& args)
{
    bool propertyChangedValue = true; // assume we changed unless we detect otherwise
    const CDependencyProperty* pDP = args.m_pDP;
    CValue* pValue = const_cast<CValue*>(&args.m_value);
    ValueBuffer valueBuffer(GetContext());
    CValue oldValue;
    ValueType valueType = pDP->GetStorageType();
    ctl::ComPtr<IInspectable> spOldValueOuter;

    // Skip association checks, repackaging, validation, etc. for custom DPs, because we never did those
    // things in the past. If we decide to enable that, we will have to add a quirk.
    if (!pDP->Is<CCustomDependencyProperty>())
    {
        IFC_RETURN(VerifyCanAssociate(pDP, *pValue));

        // This object is simulating a frozen freezable so we block SetValue() on self.
        // This would not block changes to sub-properties like a true Freezable would.
        if (m_bitFields.fSimulatingFrozen)
        {
            IFC_RETURN(E_ACCESSDENIED);
        }

        bool wasCollectionAdd;
        IFC_RETURN(TryAddToContentPropertyCollection(pDP, *pValue, &wasCollectionAdd));
        if (wasCollectionAdd)
        {
            return S_OK;
        }

        // Repackage the incoming value so the types match.
        IFC_RETURN(valueBuffer.RepackageValueAndSetPtr(pDP, pValue, &pValue));

        // Validate the incoming CValue based on its type.
        IFC_RETURN(ValidateCValue(pDP, *pValue, valueType));
    }

    if (pDP->IsSparse())
    {
        if (args.m_baseValueSource == BaseValueSourceDefault)
        {
            IFC_RETURN(ClearEffectiveValueInSparseStorage(
                EffectiveValueParams(pDP, *pValue, args.m_baseValueSource, args.m_pValueOuterNoRef),
                oldValue,
                &spOldValueOuter,
                propertyChangedValue));
        }
        else
        {
            IFC_RETURN(SetEffectiveValueInSparseStorage(
                EffectiveValueParams(pDP, *pValue, args.m_baseValueSource, args.m_pValueOuterNoRef),
                oldValue,
                &spOldValueOuter,
                propertyChangedValue));
        }

        // Don't call SetBaseValueSource - we already called it in SetEffectiveValueInSparseStorage. We called it there
        // to avoid re-querying for the sparse entry in SetBaseValueSource.
    }
    else if (pDP->IsPropMethodCall())
    {
        IFC_RETURN(SetEffectiveValueViaMethodCall(
            EffectiveValueParams(pDP, *pValue, args.m_baseValueSource, args.m_pValueOuterNoRef),
            propertyChangedValue));
        IFC_RETURN(SetBaseValueSource(pDP, args.m_baseValueSource));
    }
    else
    {
        IFC_RETURN(SetEffectiveValueInField(
            EffectiveValueParams(pDP, *pValue, args.m_baseValueSource, args.m_pValueOuterNoRef),
            oldValue,
            &spOldValueOuter,
            propertyChangedValue));
        IFC_RETURN(SetBaseValueSource(args.m_pDP, args.m_baseValueSource));
    }

    // If the value of an inherited DP has been cleared, we need to use the newly
    // inherited value for the property changed notification's new value arg.
    if (propertyChangedValue && pDP->IsInherited() && args.m_baseValueSource == BaseValueSourceDefault)
    {
        IFC_RETURN(GetValueInherited(pDP, pValue));
    }

    IFC_RETURN(OnPropertySet(pDP, oldValue, *pValue, propertyChangedValue));

    if (propertyChangedValue)
    {
        IFC_RETURN(NotifyPropertyChanged(PropertyChangedParams(pDP, oldValue, *pValue, spOldValueOuter.Get(), args.m_pValueOuterNoRef)));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetValue(_In_ const CDependencyProperty* dp, _In_ const CValue& value)
{
    return SetValue(SetValueParams(dp, value));
}

// Set a value in the dependency object. This is the local value.
_Check_return_ HRESULT CDependencyObject::SetValue(_In_ const SetValueParams& args)
{
    ASSERT(args.m_pDP != nullptr);

    // See if we're setting a modifier value (e.g. animated value).
    auto modifiedValue = args.m_modifierValueBeingSet;
    if (modifiedValue == nullptr)
    {
        // We're not. This is a regular set (local value). If there's a CModifiedValue, specify
        // that it will now be holding a local value instead of a modified value.
        modifiedValue = GetModifiedValue(args.m_pDP);

        // Double check we're really not setting the modifier value. This is the old pattern that
        // we still use in a couple of places; we should clean this up in MQ.
        if (modifiedValue != nullptr && !modifiedValue->IsModifierValueBeingSet())
        {
            IFC_RETURN(modifiedValue->SetBaseValue(args.m_value, BaseValueSourceLocal));
        }
    }

    IFC_RETURN(UpdateEffectiveValue(
        UpdateEffectiveValueParams(
            args.m_pDP,
            args.m_value,
            modifiedValue,
            ValueOperationFromSetValue,
            args.m_baseValueSource,
            args.m_pValueOuterNoRef)));

    return S_OK;
}

//  Calls SetValue with the given CValue as the value on the property specified by the given known ID.
_Check_return_ HRESULT CDependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex index, _In_ const CValue& value)
{
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(index);

    IFC_RETURN(SetValue(dp, value));
    return S_OK;
}

//  Calls SetValue with the given DO as the value on the property specified by the given known ID.
_Check_return_ HRESULT CDependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex index, _In_opt_ CDependencyObject* value)
{
    CValue cVal;
    cVal.SetObjectAddRef(value);

    IFC_RETURN(SetValueByKnownIndex(index, cVal)); // Adds reference to the object when set.
    return S_OK;
}

//  Calls SetValue with the given INT32 as the value on the property specified by the given known ID.
_Check_return_ HRESULT CDependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex index, INT32 value)
{
    CValue cVal;
    cVal.SetSigned(value);

    IFC_RETURN(SetValueByKnownIndex(index, cVal));
    return S_OK;
}

//  Calls SetValue with the given FLOAT as the value on the property specified by the given known ID.
_Check_return_ HRESULT CDependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex index, _In_ FLOAT value)
{
    CValue cVal;
    cVal.SetFloat(value);

    IFC_RETURN(SetValueByKnownIndex(index, cVal));
    return S_OK;
}

//  Calls SetValue with the given bool as the value on the property specified by the given known ID.
_Check_return_ HRESULT CDependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex index, _In_ bool value)
{
    CValue cVal;
    cVal.SetBool(value);
    IFC_RETURN(SetValueByKnownIndex(index, cVal));
    return S_OK;
}

// Try to add a value to the content property's collection if applicable.
_Check_return_ HRESULT CDependencyObject::TryAddToContentPropertyCollection(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& value,
    _Out_ bool* wasCollectionAdd)
{
    *wasCollectionAdd = false;

    if (pDP->IsContentProperty())
    {
        // TODO: Cleanup this implementation. It appears most of this code was written a long time ago. With today's
        //       type system, we should be able to simplify most of this code. We should also consider generating
        //       errors instead of implicitly adding nullptrs for types we don't support (may require a quirk).

        // See if we're adding to a generic collection (or transform collection).
        if (OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>())
        {
            ValueBuffer valueBuffer(GetContext());
            CCollection* pThisCollection = nullptr;
            CDOCollection* pThisDOCollection = nullptr;
            xref_ptr<CDependencyObject> obj;
            CValue* pValue = const_cast<CValue*>(&value);

            *wasCollectionAdd = true;

            IFC_RETURN(DoPointerCast(pThisCollection, this));

            if (valueObject != value.GetType())
            {
                switch (value.GetType())
                {
                    case valueFloat:
                        break;

                    case valueSigned:
                    case valueEnum:
                    case valueEnum8:
                    case valueBool:
                        break;

                    case valueColor:
                        break;

                    case valueString:
                        break;

                    case valuePoint:
                        IFC_RETURN(pThisCollection->Append(*pValue));
                        return S_OK;

                    case valueSize:
                        {
                            CREATEPARAMETERS cp(GetContext(), *pValue);
                            IFC_RETURN(CSize::Create(obj.ReleaseAndGetAddressOf(), &cp));
                        }
                        break;

                    case valueRect:
                    case valueThickness:
                    case valueCornerRadius:
                        break;

                    case valueFloatArray:
                    case valuePointArray:
                        break;

                    case valueAny:
                    case valueObject:
                    default:
                        // Handle any types we don't expect
                        ASSERT(FALSE);
                        IFC_RETURN(E_UNEXPECTED);
                        break;
                }

                valueBuffer.GetValuePtr()->SetObjectNoRef(obj.detach());
                pValue = valueBuffer.GetValuePtr();
            }
            else
            {
                // We actually have no index to distinguish if the collection is a DO collection or not,
                // and casting eg. a CDoubleCollection to CDOCollection is bad (see bug #23546).
                //
                // We have to infer from the type of the child.

                ASSERT(value.GetType() == valueObject);

                CDependencyObject* valueAsObject = value.AsObject();

                if (valueAsObject->OfTypeByIndex<KnownTypeIndex::Point>() ||
                    valueAsObject->OfTypeByIndex<KnownTypeIndex::Double>())
                {
                    // Type checking is handled inside the CCollection::Append
                    IFC_RETURN(pThisCollection->Append(*pValue));

                    // The value collections do their own notification. (OnAddToCollection is protected)
                    return S_OK;
                }
            }

            IFCEXPECT_RETURN(pThisCollection->IsDOCollection());
            pThisDOCollection = static_cast<CDOCollection*>(pThisCollection);

            // Type checking is handled inside the CCollection::Append
            IFC_RETURN(pThisDOCollection->Append(pValue->AsObject()));

            // Make sure the collection knows it changed - needed for type-specific side effects
            IFC_RETURN(pThisDOCollection->OnAddToCollection(pValue->AsObject()));
        }
    }

    return S_OK;
}

// Updates effective value of a property. This function is called in all cases where the effective value needs to be updated
// using precedence rules that determine which value source's value is effective.
_Check_return_ HRESULT CDependencyObject::UpdateEffectiveValue(_In_ const UpdateEffectiveValueParams& args)
{
    CValue effectiveValue;
    BaseValueSource baseValueSource = BaseValueSourceLocal;
    CValue* pEffectiveValue = nullptr;
    bool valueSetNeeded = true;

    auto failLogGuard = wil::scope_exit([this, &pEffectiveValue, &args]()
    {
        AppendFailedUpdateValueInfo(
            args.m_pDP->GetIndex(),
            this,
            pEffectiveValue);
    });

    if (args.m_valueOperation & ValueOperationReevaluate)
    {
        // Re-evaluate effective value.
        IFC_RETURN(EvaluateEffectiveValue(
            args.m_pDP,
            args.m_modifiedValue.get(),
            args.m_valueOperation,
            &effectiveValue,
            &baseValueSource,
            &valueSetNeeded));

        pEffectiveValue = &effectiveValue;
    }
    else
    {
        // If Effective value is not being re-evaluated, either value or modified value should be passed. Modifiers have
        // higher precedence.
        if (args.m_modifiedValue != nullptr)
        {
            IFC_RETURN(args.m_modifiedValue->GetEffectiveValue(&effectiveValue));
            pEffectiveValue = &effectiveValue;
        }
        else
        {
            pEffectiveValue = const_cast<CValue*>(&args.m_value);
        }
    }

    if (valueSetNeeded)
    {
        // Validate strictness - whether or not the API call is allowed based on strict mode.
        // Note that other higher level strictness checks will happen to help generate a failure at the public
        // API boundary but not all can be caught easily, so this check acts as a safety net to catch them all,
        // since all property setting mechanisms end up coming through this code path.
        IFC_RETURN(ValidateStrictnessOnProperty(args.m_pDP));

        // If this is a call from SetValue(), call SetEffectiveValue to set the value. Otherwise call SetValue so that
        // derived class overrides of SetValue are called.
        if (args.m_valueOperation & ValueOperationFromSetValue)
        {
            IFC_RETURN(SetEffectiveValue(
                EffectiveValueParams(
                    args.m_pDP,
                    *pEffectiveValue,
                    (args.m_baseValueSource != BaseValueSourceUnknown) ? args.m_baseValueSource : baseValueSource,
                    args.m_pValueOuterNoRef)));

            // If this DP had an associated theme reference, clear it because a new value has been set.
            ClearThemeResource(args.m_pDP);

            if (m_theme != Theming::Theme::None)
            {
                IFC_RETURN(NotifyPropertyValueOfThemeChange(args.m_pDP, pEffectiveValue));
            }
        }
        else
        {
            // Mark if a modified value is being set, so
            // CDependencyObject::SetValue can differentiate between
            // a base value or a modified value being set.

            auto cleanupGuard = wil::scope_exit([&args]
            {
                args.m_modifiedValue->SetModifierValueBeingSet(false);
            });

            bool hasModifiedValue = (args.m_modifiedValue && args.m_modifiedValue->HasModifiers());

            cleanupGuard.release();

            bool processed = false;

            IFC_RETURN(TryProcessingThemeResourcePropertyValue(
                args.m_pDP,
                args.m_modifiedValue.get(),
                pEffectiveValue,
                baseValueSource,
                &processed));

            if (!processed)
            {
                IFC_RETURN(SetValue(
                    SetValueParams(
                        args.m_pDP,
                        *pEffectiveValue,
                        baseValueSource,
                        /* valueOuterNoRef */ nullptr,
                        hasModifiedValue ? args.m_modifiedValue : nullptr)));
            }
        }
    }

    failLogGuard.release();

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::OnPropertySet(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& oldValue,
    _In_ const CValue& value,
    _In_ bool propertyChangedValue)
{
    if (!pDP->Is<CCustomDependencyProperty>())
    {
        if (propertyChangedValue)
        {
            if (pDP->IsInherited())
            {
                // Propagate the inherited property change down the tree.
                IFC_RETURN(MarkInheritedPropertyDirty(pDP, &value));
            }

            // Propagate the rendering change to the element the DP was set on using
            // the handler specified by the DP.
            RENDERCHANGEDPFN pfnRenderChanged = pDP->GetRenderChangedHandler();
            if (pfnRenderChanged != nullptr)
            {
                pfnRenderChanged(
                    this,
                    value.GetCustomData().IsIndependent() ? DirtyFlags::Independent : DirtyFlags::None
                    );
            }

            if (pDP->AffectsMeasure() || pDP->AffectsArrange())
            {
                PropagateLayoutDirty(pDP->AffectsMeasure(), pDP->AffectsArrange());
            }
        }

        // For compat reasons (e.g. Media_PME DRT compatibility) we need to call Invoke even if the property's value
        // did not change.
        IFC_RETURN(Invoke(pDP, /* namescopeOwner */ nullptr, IsActive()));
    }

    // Call out for virtual overrides
    IFC_RETURN(OnPropertySetImpl(pDP, oldValue, value, propertyChangedValue));
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::OnPropertySetImpl(
    _In_ const CDependencyProperty* dp,
    _In_ const CValue&,
    _In_ const CValue&,
    _In_ bool)
{
    if (HasDeferred())
    {
        IFC_RETURN(CDeferredMapping::NotifyParentValueSet(this, dp->GetIndex()));
    }

    return S_OK;
}

template <typename T, typename U = T>
static bool UpdateValueHelper(
    _In_ XHANDLE field,
    _In_ const CValue& newValue,
    _In_ CValue& savedValue,
    U(CValue::*Getter)() const,
    void (CValue::*Setter)(U))
{
    T& fieldValue = reinterpret_cast<T*>(field)[0];
    U newRawValue = (newValue.*Getter)();

    if (newRawValue == fieldValue)
    {
        return false;
    }
    else
    {
        (savedValue.*Setter)(fieldValue);
        fieldValue = newRawValue;
        return true;
    }
}

template <typename T, typename U = T>
static bool UpdateValueHelper(
    _In_ KnownTypeIndex index,
    _In_ XHANDLE field,
    _In_ const CValue& newValue,
    _In_ CValue& savedValue,
    U(CValue::*Getter)() const,
    void (CValue::*Setter)(U))
{
    T& fieldValue = reinterpret_cast<T*>(field)[0];
    U newRawValue = (newValue.*Getter)();

    if (newRawValue == fieldValue)
    {
        return false;
    }
    else
    {
        (savedValue.*Setter)({ fieldValue, index });
        fieldValue = newRawValue.m_value;
        return true;
    }
}

template<>
STATIC_SPEC bool UpdateValueHelper<float>(
    _In_ XHANDLE field,
    _In_ const CValue& newValue,
    _In_ CValue& savedValue,
    float (CValue::*Getter)() const,
    void (CValue::*Setter)(float))
{
    float& fieldValue = reinterpret_cast<float*>(field)[0];
    float newRawValue = (newValue.*Getter)();

    // if both the new and old value are NaN
    // then treat them as equal for property change
    if ((_isnan(newRawValue) && _isnan(fieldValue))
       || (newRawValue == fieldValue))
    {
        return false;
    }
    else
    {
        (savedValue.*Setter)(fieldValue);
        fieldValue = newRawValue;
        return true;
    }
}

template <typename T>
static bool UpdateValueHelper(
    _In_ XHANDLE field,
    _In_ const CValue& newValue,
    T* (CValue::*Getter)() const)
{
    T& fieldValue = reinterpret_cast<T*>(field)[0];
    T* newRawValue = (newValue.*Getter)();

    // NOTE:
    // The if (!newRewValue) check was added to mimic pre RS1 behavior of the
    // property system. Consider throwing a quirked validation error after
    // RS1 to prevent apps from sending invalid values (perhaps in RepackageValue).
    // Since RS1 apps have already taken dependency on this, it cannot be
    // quirked without causing applications to fail.
    // See OSG Bug 7046533 for more details.
    if (!newRawValue ||
        (*newRawValue == fieldValue))
    {
        return false;
    }
    else
    {
        fieldValue = *newRawValue;
        return true;
    }
}

template <typename T>
static bool UpdateValueHelperVO(
    _In_ XHANDLE field,
    _In_ const CValue& newValue,
    _Out_ CValue& savedValue)
{
    using wrapper_type = Flyweight::PropertyValueObjectWrapper<T>;

    xref_ptr<wrapper_type>* fieldValue = reinterpret_cast<xref_ptr<wrapper_type>*>(field);
    const wrapper_type* newRawValue = static_cast<const wrapper_type*>(newValue.As<valueVO>());

    // FUT: Ignore whether the value has actually changed.  For every value set, object was repackaged into a
    // new instance at different address, and to determine if value changed a pointer comparison was used.
    // As a result notification was always raised.

    if (*fieldValue != nullptr || newRawValue != nullptr)
    {
        savedValue.SetAddRef<valueVO>(*fieldValue);
        *fieldValue = const_cast<wrapper_type*>(newRawValue);
        return true;
    }
    else
    {
        // both null -- no change
        return false;
    }
}

// Stores the effective value for a field-backed property.
_Check_return_ HRESULT CDependencyObject::SetEffectiveValueInField(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue)
{
    const CDependencyProperty* pDP = args.m_pDP;
    ValueType storageType = pDP->GetStorageType();
    CValue* pValue = const_cast<CValue*>(&args.m_value);

    // Value Storage
    if (pDP->GetIndex() == KnownPropertyIndex::DependencyObject_Name)
    {
        // the type should have been validated / coerced by now.
        ASSERT(pValue->GetType() == valueString);
        IFC_RETURN(SetName(pValue->AsString()));
    }
    else
    {
        // Now store the value at the specified offset.
        XHANDLE field = GetPropertyOffset(pDP, /* forGetValue */ false); // default NOT OK for SetValue
        IFCPTR_RETURN(field);

        ASSERT(storageType == valueAny ||
               storageType == pValue->GetType() ||
               (storageType == valueObject && pValue->GetType() == valueNull));

        switch (storageType)
        {
            case valueFloat:
                propertyChangedValue = UpdateValueHelper<float>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::AsFloat,
                    &CValue::SetFloat);
                break;

            case valueSigned:
                propertyChangedValue = UpdateValueHelper<XINT32>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::AsSigned,
                    &CValue::SetSigned);
                break;

            case valueEnum:
                propertyChangedValue = UpdateValueHelper<uint32_t>(
                    pDP->GetPropertyType()->GetIndex(),
                    field,
                    *pValue,
                    oldValue,
                    &CValue::As<valueEnum>,
                    &CValue::Set<valueEnum>);
                break;
            case valueEnum8:
                propertyChangedValue = UpdateValueHelper<uint8_t>(
                    pDP->GetPropertyType()->GetIndex(),
                    field,
                    *pValue,
                    oldValue,
                    &CValue::As<valueEnum8>,
                    &CValue::Set<valueEnum8>);
                break;
            case valueBool:
                propertyChangedValue = UpdateValueHelper<bool>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::As<valueBool>,
                    &CValue::Set<valueBool>);
                break;

            case valueColor:
                propertyChangedValue = UpdateValueHelper<XUINT32>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::AsColor,
                    &CValue::SetColor);
                break;

            case valueTypeHandle:
                propertyChangedValue = UpdateValueHelper<KnownTypeIndex>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::AsTypeHandle,
                    &CValue::SetTypeHandle);
                break;

            case valueString:
                {
                    xstring_ptr& fieldValue = reinterpret_cast<xstring_ptr*>(field)[0];

                    if (!fieldValue.Equals(pValue->AsString()))
                    {
                        oldValue.SetString(std::move(fieldValue));

                        if (pValue->GetType() == valueNull)
                        {
                            fieldValue.Reset();
                        }
                        else
                        {
                            fieldValue = pValue->AsString();
                        }
                    }
                    else
                    {
                        propertyChangedValue = false;
                    }
                }
                break;

            case valueDouble:
                propertyChangedValue = UpdateValueHelper<DOUBLE>(
                    field,
                    *pValue,
                    oldValue,
                    &CValue::AsDouble,
                    &CValue::SetDouble);
                break;

            case valuePoint:
                propertyChangedValue = UpdateValueHelper<XPOINTF>(
                    field,
                    *pValue,
                    &CValue::AsPoint);
                break;

            case valueSize:
                propertyChangedValue = UpdateValueHelper<XSIZEF>(
                    field,
                    *pValue,
                    &CValue::AsSize);
                break;

            case valueRect:
                propertyChangedValue = UpdateValueHelper<XRECTF>(
                    field,
                    *pValue,
                    &CValue::AsRect);
                break;

            case valueCornerRadius:
                propertyChangedValue = UpdateValueHelper<XCORNERRADIUS>(
                    field,
                    *pValue,
                    &CValue::AsCornerRadius);
                break;

            case valueThickness:
                propertyChangedValue = UpdateValueHelper<XTHICKNESS>(
                    field,
                    *pValue,
                    &CValue::AsThickness);
                break;

            case valueGridLength:
                propertyChangedValue = UpdateValueHelper<XGRIDLENGTH>(
                    field,
                    *pValue,
                    &CValue::AsGridLength);
                break;

            case valueFloatArray:
            case valuePointArray:
                break;

            case valueObject:
                IFC_RETURN(SetEffectiveValueInField_Object(args, field, oldValue, ppOldValueOuter, propertyChangedValue));
                break;

            case valueAny:
                IFC_RETURN(SetEffectiveValueInField_Any(args, field, oldValue, ppOldValueOuter, propertyChangedValue));
                break;

            case valueVO:
                {
                    auto valueAsVO = pValue->As<valueVO>();

                    if (valueAsVO)
                    {
                        IFCEXPECT_RETURN(args.m_pDP->GetPropertyType()->GetIndex() == valueAsVO->GetTypeIndex());
                    }
                }

                switch (args.m_pDP->GetPropertyType()->GetIndex())
                {
                    case KnownTypeIndex::Duration:
                        propertyChangedValue = UpdateValueHelperVO<DurationVO>(
                            field,
                            *pValue,
                            oldValue);
                        break;

                    case KnownTypeIndex::RepeatBehavior:
                        propertyChangedValue = UpdateValueHelperVO<RepeatBehaviorVO>(
                            field,
                            *pValue,
                            oldValue);
                        break;

                    case KnownTypeIndex::KeyTime:
                        propertyChangedValue = UpdateValueHelperVO<KeyTimeVO>(
                            field,
                            *pValue,
                            oldValue);
                        break;

                    default:
                        ASSERT(FALSE);
                }
                break;

            default:
                // Handle any types we don't expect.
                ASSERT(false);
                IFC_RETURN(E_UNEXPECTED);
                break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetEffectiveValueInField_Any(_In_ const EffectiveValueParams& args, _In_ XHANDLE field, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue)
{
    const CDependencyProperty* pDP = args.m_pDP;
    CValue* pValue = const_cast<CValue*>(&args.m_value);
    bool oldValueIsCached = false;
    bool newValueNeedsCaching = false;

    // A property of type valueAny must point its offset to its CValue member.
    CValue* destination = (CValue*)field;

    // Not pointer comparison - invoke the CValue's operator== override.
    if ((*destination) == (*pValue))
    {
        propertyChangedValue = false;
    }
    else
    {
        // If the existing value is an object check if we did cache it.
        CDependencyObject* destinationAsObject = destination->AsObject();

        if (destinationAsObject)
        {
            // Keep the parent up-to-date, just as for valueObject.
            if (destinationAsObject->IsParentAware())
            {
                IFC_RETURN(destinationAsObject->RemoveParent(this));
            }
            else if (destinationAsObject->DoesAllowMultipleAssociation())
            {
                destinationAsObject->SetAssociated(false, nullptr);
            }

            // Check if SetPeerReferenceToProperty was called.
            oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(pDP, *destination, !!IsPropertySetByStyle(pDP));
        }

        // If the value is an object then check if we need to cache it.
        CDependencyObject* valueAsObject = pValue->AsObject();

        if (valueAsObject)
        {
            // Keep the parent up-to-date, just as for valueObject.
            if (valueAsObject->IsParentAware())
            {
                IFC_RETURN(valueAsObject->AddParent(this, true, pDP->GetRenderChangedHandler()));
            }
            else if (valueAsObject->DoesAllowMultipleAssociation())
            {
                valueAsObject->SetAssociated(true, this);
            }

            // Check if SetPeerReferenceToProperty should be called.
            bool isSetByStyle = (args.m_baseValueSource == BaseValueSourceBuiltInStyle || args.m_baseValueSource == BaseValueSourceStyle);
            newValueNeedsCaching = ShouldTrackWithPeerReferenceToProperty(pDP, *pValue, isSetByStyle);
        }

        {
            // DO property value is walked for GC, so take GC lock before it is changed
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

            // Move value out of storage, but hold on to it locally for our change notification.
            oldValue = std::move(*destination);

            // Wholly contained CValue: copied directly.  (Color, Enum, Double, Int.)
            // CValue with pointer to memory: make our own copy.  (String, Point, Rect.)
            // CValue with DO: AddRef().
            IFC_RETURN(destination->CopyConverted(*pValue));
        }

        // Notify the managed layer of the change
        IFC_RETURN(UpdatePeerReferenceToProperty(pDP, *pValue, oldValueIsCached, newValueNeedsCaching, args.m_pValueOuterNoRef, ppOldValueOuter));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetEffectiveValueInField_Object(_In_ const EffectiveValueParams& args, _In_ XHANDLE field, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue)
{
    const CDependencyProperty* pDP = args.m_pDP;
    CValue* pValue = const_cast<CValue*>(&args.m_value);
    CDependencyObject* valueAsObject = pValue->AsObject();

    ASSERT(pValue->GetType() == valueObject ||
           pValue->GetType() == valueNull);

    // Check if the object types are the compatible.
    if (valueAsObject &&
        !valueAsObject->OfTypeByIndex(pDP->GetPropertyTypeIndex()))
    {
        // If they aren't, see if the property is a known collection type and try to append the value.
        IFC_RETURN(AppendEffectiveValueToCollectionInField(args, field));
    }
    else
    {
        // The types are compatible, set the DP appropriately...
        CDependencyObject* previousDONoRef = (CDependencyObject*)(((void**)field)[0]);

        if (previousDONoRef == valueAsObject)
        {
            propertyChangedValue = false;
        }
        else
        {
            bool oldValueIsCached = false;
            bool newValueNeedsCaching = false;

            oldValue.SetObjectAddRef(previousDONoRef);

            // Disconnect existing value, if set.
            if (previousDONoRef != nullptr)
            {
                // Check to see if SetPeerReferenceToProperty was called.
                oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(pDP, oldValue, !!IsPropertySetByStyle(pDP));

                IFC_RETURN(LeaveEffectiveValue(pDP, previousDONoRef));
            }

            {
                // DO property value is walked for GC, so take GC lock while old value is cleared and new value is set
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                // Clear old value
                if (previousDONoRef != nullptr)
                {
                    IFC_RETURN(ClearEffectiveValueInObjectField(pDP, previousDONoRef));
                }

                // Set new value.
                ((void**)field)[0] = valueAsObject;
            }

            if (valueAsObject)
            {
                valueAsObject->AddRef();

                IFC_RETURN(EnterEffectiveValue(pDP, valueAsObject));

                // Check to see if SetPeerReferenceToProperty should be called.
                bool isSetByStyle = (args.m_baseValueSource == BaseValueSourceBuiltInStyle || args.m_baseValueSource == BaseValueSourceStyle);
                newValueNeedsCaching = ShouldTrackWithPeerReferenceToProperty(pDP, *pValue, isSetByStyle);
            }

            // Notify the managed layer of the change
            IFC_RETURN(UpdatePeerReferenceToProperty(pDP, *pValue, oldValueIsCached, newValueNeedsCaching, args.m_pValueOuterNoRef, ppOldValueOuter));
        }
    }

    return S_OK;
}

// Stores the effective value for a sparse property.
_Check_return_ HRESULT CDependencyObject::SetEffectiveValueInSparseStorage(_In_ const CDependencyProperty* dp, _In_ const CValue& value)
{
    CValue oldValue;
    bool propertyChangedValue = false;
    return SetEffectiveValueInSparseStorage(
        EffectiveValueParams(dp, value, BaseValueSourceUnknown),
        oldValue,
        /* ppOldValueOuter */ nullptr,
        propertyChangedValue);
}

// Stores the effective value for a sparse property.
_Check_return_ HRESULT CDependencyObject::SetEffectiveValueInSparseStorage(_In_ const EffectiveValueParams& args, _Inout_ CValue& oldValue, _Outptr_result_maybenull_ IInspectable** ppOldValueOuter, _Inout_ bool& propertyChangedValue)
{
    bool oldValueIsCached = false;
    bool newValueNeedsCaching = false;

    // Ensure the value table exists.
    if (!m_pValueTable)
    {
        // the garbage collection walk iterates over the m_pValueTable
        // and replacing the table must be done in a gc thread safe manner
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        m_pValueTable.reset(new SparseValueTable);
    }

    SparseValueTable::iterator sparseEntry;
    // the garbage collection walk iterates over the m_pValueTable
    // and hence entries need to be added in a gc thread safe manner
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        // If this property doesn't yet have an entry, populate a default one
        // If it does, then insert is a no-op
        sparseEntry = m_pValueTable->insert(std::make_pair(args.m_pDP->GetIndex(), EffectiveValue())).first;
    }

    if (sparseEntry->second.value.GetType() != valueAny)
    {
        // the garbage collection walk iterates over the m_pValueTable
        // and modifying entries needs to be done in a gc thread safe manner
        {
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

            // Move value out of storage, but hold on to it locally for our change notification.
            oldValue = std::move(sparseEntry->second.value);
        }

        CDependencyObject* pOldObjectNoRef = oldValue.AsObject();

        if (pOldObjectNoRef != nullptr)
        {
            // If the object is not a FrameworkElement, then clear its inheritance context.
            if (!pOldObjectNoRef->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
            {
                pOldObjectNoRef->SetParentForInheritanceContextOnly(nullptr);
                IFC_RETURN(pOldObjectNoRef->NotifyInheritanceContextChanged());
            }

            IFC_RETURN(LeaveEffectiveValue(args.m_pDP, pOldObjectNoRef));

            // The last few calls may have incurred another set/clear in our sparse table and invalidated our iterator. Let's fetch a fresh one
            sparseEntry = m_pValueTable->find(args.m_pDP->GetIndex());
            ASSERT(sparseEntry != m_pValueTable->end());

            // Check if we protected the old reference's peer.
            oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, oldValue, sparseEntry->second.IsSetByStyle());
        }
        else if (oldValue.GetType() == valueIInspectable)
        {
            // Check if we protected the old reference's peer.
            oldValueIsCached = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, oldValue, sparseEntry->second.IsSetByStyle());
        }
    }
    else
    {
        IFC_RETURN(GetDefaultValue(args.m_pDP, &oldValue));
    }

    {
        CValue tempValue;
        tempValue.CopyConverted(args.m_value);

        // the garbage collection walk iterates over the m_pValueTable
        // and modifying entries needs to be done in a gc thread safe manner
        {
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

            sparseEntry->second.value.swap(tempValue);
        }
    }

    if (args.m_baseValueSource != BaseValueSourceUnknown)
    {
        IFC_RETURN(SetBaseValueSource(args.m_pDP, args.m_baseValueSource, &sparseEntry->second));
    }

    CDependencyObject* pNewObjectNoRef = sparseEntry->second.value.AsObject();

    if (pNewObjectNoRef != nullptr)
    {
        IFC_RETURN(EnterEffectiveValue(args.m_pDP, pNewObjectNoRef));

        // If the object is not a FrameworkElement, then set its inheritance context.
        if (!pNewObjectNoRef->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            pNewObjectNoRef->SetParentForInheritanceContextOnly(this);
            IFC_RETURN(pNewObjectNoRef->NotifyInheritanceContextChanged());
        }

        // The last few calls may have incurred another set/clear in our sparse table and invalidated our iterator. Let's fetch a fresh one
        sparseEntry = m_pValueTable->find(args.m_pDP->GetIndex());
        ASSERT(sparseEntry != m_pValueTable->end());

        // Check if we need to protect the new reference's peer.
        newValueNeedsCaching = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, sparseEntry->second.value, sparseEntry->second.IsSetByStyle());

        if (newValueNeedsCaching && ShouldReleaseCoreObjectWhenTrackingPeerReference())
        {
            // TODO: Remove this hack once we have proper lifetime semantics in the core.
            // Remove the strong reference that the above CopyConverted call made on the core side, because we will
            // make a tracker reference on the framework side later on (see the call to UpdatePeerReferenceToProperty).
            CValueUtil::ReleaseRefAndDropObjectOwnership(sparseEntry->second.value);
        }
    }
    else if (sparseEntry->second.value.GetType() == valueIInspectable)
    {
        // Check if we need to protect the new reference's peer.
        newValueNeedsCaching = ShouldTrackWithPeerReferenceToProperty(args.m_pDP, sparseEntry->second.value, sparseEntry->second.IsSetByStyle());

        if (newValueNeedsCaching)
        {
            // TODO: Remove this hack once we have proper lifetime semantics in the core.
            // Remove the strong reference that the above CopyConverted call made on the core side, because we will
            // make a tracker reference on the framework side later on (see the call to UpdatePeerReferenceToProperty).
            CValueUtil::ReleaseRefAndDropObjectOwnership(sparseEntry->second.value);
        }
    }

    // Notify the framework layer of the change.
    IFC_RETURN(UpdatePeerReferenceToProperty(args.m_pDP, args.m_value, oldValueIsCached, newValueNeedsCaching, args.m_pValueOuterNoRef, ppOldValueOuter));

    propertyChangedValue = (oldValue != args.m_value);

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetEffectiveValueViaMethodCall(_In_ const EffectiveValueParams& args, _Inout_ bool& propertyChangedValue)
{
    METHODPFN pfn = args.m_pDP->GetPropertyMethod();

    ASSERT(args.m_pDP->IsPropMethodCall() && pfn);

    CValue valReturn;
    valReturn.SetBool(propertyChangedValue);

    // call the method
    IFC_RETURN(pfn(this, 1, const_cast<CValue*>(&args.m_value), args.m_pValueOuterNoRef, &valReturn));
    propertyChangedValue = valReturn.AsBool();

    return S_OK;
}

// This is a helper function used in SetValue.  If this is true, then it indicates that
// SetPeerReferenceToProperty should be called for this property value.
bool CDependencyObject::ShouldTrackWithPeerReferenceToProperty(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& value,
    _In_ bool bValueIsSetByStyle)
{
    CDependencyObject* pDO = value.AsObject();

    // We expect the value to be a valueObject (DO). If the property is a sparse property, then we also
    // support valueIInspectable.
    ASSERT(value.GetType() == valueObject || (pDP->IsSparse() && value.GetType() == valueIInspectable));

    // Visual tree properties (such as Border.Child) get their protection from the tree, so we don't need
    // any additional peer references for protection.
    // Property values that support multiple association and are not parent aware use
    // CShareableDependencyObject::OnMultipleAssociationChange to protect peer references
    return (pDP->IsSparse() && !pDP->IsVisualTreeProperty()) ||
        // Put ExternalObjectReference in m_pPropertyValueReferences, so DependencyObject::GetValue
        // can support object identity.
        pDO->GetTypeIndex() == DependencyObjectTraits<CManagedObjectReference>::Index;
}

// Verify the specified value can be associated with the current object.
_Check_return_ HRESULT CDependencyObject::VerifyCanAssociate(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& value)
{
    // If it's an object, and it's already been set somewhere else, then we fail
    // Note: The last check is for setting the same object to itself, which we don't want to fail
    CDependencyObject* valueAsObject = value.AsObject();

    if (valueAsObject &&
        valueAsObject->m_bitFields.fIsAssociated &&
        pDP->RequiresMultipleAssociationCheck() &&
        !valueAsObject->DoesAllowMultipleAssociation())
    {
        // At this point we would have returned E_INVALIDARG indicating
        // failure. But, we don't want to fail if the value passed is
        // already assigned to this dp via Method Call or we can recover
        // by unassociating.
        HRESULT xr = E_INVALIDARG;  // If the values don't match this is going to happen

        // Check if the existing value is equal to the passed in value.
        if (pDP->IsPropMethodCall() || pDP->IsSparse())
        {
            CValue existingValue;

            if (pDP->IsPropMethodCall())
            {
                IFC_RETURN(GetEffectiveValueViaMethodCall(pDP, &existingValue));
            }
            else
            {
                IFC_RETURN(GetEffectiveValueInSparseStorage(pDP, &existingValue));
            }

            if (existingValue.GetType() == valueObject && existingValue.AsObject() == valueAsObject)
            {
                // Nothing changed, we're done.
                return S_OK;
            }
        }
        else if (MapPropertyAndGroupOffsetToDO(pDP->GetOffset(), pDP->GetGroupOffset()) == valueAsObject)
        {
            // Nothing changed, we're done.
            return S_OK;
        }

        // this might be an unloading element that we can safely remove from tree
        CUIElement* pTargetAsUIElement = do_pointer_cast<CUIElement>(valueAsObject);

        if (pTargetAsUIElement)
        {
            IFC_RETURN(pTargetAsUIElement->OnAssociationFailure());

            if (!valueAsObject->m_bitFields.fIsAssociated)
            {
                // successfully recovered
                xr = S_OK;
            }
        }

        if (FAILED(xr)) // Try to recover by unassociating.
        {
            // Notify the parent of this impending failure. It may process this and unassociate. In that case we can
            // recover.
            CDependencyObject* pParent = valueAsObject->GetUIElementParentInternal(false);
            if (pParent)   // will only be able to recover from IsAssociated condition currently
            {
                // remark: this fix is currently scoped down just for CP. Border and other controls will still have issues.
                // furthermore, it is only active when in Designmode. This is done to unblock Blend. We are tracking this
                // issue for SL5 and will remove the condition.

                // ContentPresenter does not disassociate its content while leaving the visual tree.
                // So disassociate content now, before content is re-parented. This is not done in ContentPresenter::Leave because some
                // apps depend on the old content being parented to the ContentPresenter, even when not in the visual tree.
                IFC_RETURN(pParent->PerformEmergencyInvalidation(valueAsObject));
            }

            if (!valueAsObject->m_bitFields.fIsAssociated)
            {
                // successfully recovered
                xr = S_OK;
            }
        }

        // unable to recover.
        if (FAILED(xr))
        {
            IFC_RETURN(xr);
        }
        // otherwise continue processing
    }

    return S_OK;
}
#pragma endregion

#pragma region BaseValueSource methods
BaseValueSource CDependencyObject::GetBaseValueSource(_In_ const CDependencyProperty* pDP) const
{
    if (IsPropertySetByStyle(pDP))
    {
        return BaseValueSourceStyle;
    }
    else if (IsPropertyDefault(pDP))
    {
        return BaseValueSourceDefault;
    }
    else
    {
        return BaseValueSourceLocal;
    }
}

BaseValueSource CDependencyObject::GetBaseValueSource(_In_ const EffectiveValue& sparseEntry) const
{
    if (sparseEntry.IsSetByStyle())
    {
        return BaseValueSourceStyle;
    }
    else if (!sparseEntry.IsSetLocally())
    {
        return BaseValueSourceDefault;
    }
    else
    {
        return BaseValueSourceLocal;
    }
}

//  Checks if the specified property has ever been set on this object
//  Note:
//     If the property is an inherited property, returns true if the value is
//     not set locally. i.e. the value is not necessarily the default for this
//     property but may instead be any value inherited from a parent of this DO.
bool CDependencyObject::IsPropertyDefault(_In_ const CDependencyProperty* pDP) const
{
    if (pDP->IsSparse())
    {
        // We use a slower check for sparse properties because they don't participate in the slot system.
        if (m_pValueTable != nullptr)
        {
            SparseValueTable::iterator it = m_pValueTable->find(pDP->GetIndex());
            if (it != m_pValueTable->end())
            {
                return !it->second.IsSetLocally();
            }
        }

        return true;
    }
    else if (pDP->IsInheritedAttachedPropertyInStorageGroup())
    {
        if (m_pInheritedProperties == nullptr)
        {
            // If there's no local m_pInheritedProperties then we can never have
            // set a local value.
            return true;
        }
        else
        {
            // If the local m_pInheritedProperties belongs to another DO then we
            // haven't set a local value.
            return (m_pInheritedProperties->m_pWriter != this)
                || (!m_pInheritedProperties->IsPropertyFlagSet(pDP, InheritedPropertyFlag::IsSetLocally));
        }
    }
    else
    {
        UINT8 slotCount = MetadataAPI::GetPropertySlotCount(GetTypeIndex());
        UINT8 slot = MetadataAPI::GetPropertySlot(pDP->GetIndex());
        if (slot >= slotCount)
        {
            // Avoid a buffer overflow in our bit field. This protection is especially needed for Windows 8.1 apps and older
            // because they don't validate that a valid property is being set.
            // We pretend that invalid properties have a non-default value for compatibility with Windows 8.1 and older.
            return false;
        }
        return !GetPropertyBitField(m_valid, slotCount, slot);
    }
}

bool CDependencyObject::IsPropertySetBySlot(UINT32 slot) const
{
    // Provides very fast property set state lookup for clients that
    // cache the slot number of the property they want.
    // For example, used by GetInheritanceParentInternal to get the local
    // set state of the framework element logical parent dependency property.
    if (m_bitFields.fIsValidAPointer)
    {
        return (m_valid.pointer[slot / 32] & 1 << (slot & 31)) ? true : false;
    }
    else if (slot < 32)
    {
        return (m_valid.bits & (1 << slot)) ? true : false;
    }
    else
    {
        // Note: We get here in two cases: either this DO has less than
        // 32 properties, and none are set locally, OR there are more than
        // 32 but we're testing before m_valid has been allocated as
        // a pointer. In both cases this slot is not set locally.
        return false;
    }
}

// Set base value source pf property in DO.
_Check_return_ HRESULT CDependencyObject::SetBaseValueSource(
    _In_ const CDependencyProperty* pDP,
    _In_ BaseValueSource baseValueSource,
    _In_opt_ EffectiveValue* sparseEntry)
{
    if (pDP->IsSparse())
    {
        ASSERT(sparseEntry != nullptr);
        switch (baseValueSource)
        {
        case BaseValueSourceDefault:
            sparseEntry->SetIsSetLocally(false);
            sparseEntry->SetIsSetByStyle(false);
            break;

        case BaseValueSourceStyle:
        case BaseValueSourceBuiltInStyle:
            // Currently style and built-in style are marked the same in the DO
            sparseEntry->SetIsSetLocally(true);
            sparseEntry->SetIsSetByStyle(true);
            break;

        case BaseValueSourceLocal:
            sparseEntry->SetIsSetLocally(true);
            sparseEntry->SetIsSetByStyle(false);
            break;

        default:
            // Invalid base value source
            ASSERT(0);
            IFC_RETURN(E_FAIL);
            break;
        }

        if (pDP->IsInherited())
        {
            InvalidateInheritedProperty(pDP);
        }
    }
    else
    {
        switch (baseValueSource)
        {
        case BaseValueSourceDefault:
            SetPropertyIsDefault(pDP);
            IFC_RETURN(SetIsSetByStyle(pDP, false));
            break;

        case BaseValueSourceStyle:
        case BaseValueSourceBuiltInStyle:
            // Currently style and built-in style are marked the same in the DO
            IFC_RETURN(SetPropertyIsLocal(pDP));
            IFC_RETURN(SetIsSetByStyle(pDP));
            break;

        case BaseValueSourceLocal:
            IFC_RETURN(SetPropertyIsLocal(pDP));
            IFC_RETURN(SetIsSetByStyle(pDP, false));
            break;

        default:
            // Invalid base value source
            ASSERT(0);
            IFC_RETURN(E_FAIL);
            break;
        }
    }

    return S_OK;
}

// Updates the "property value is default" bit for the given property.
// This is a subset of functionality of ClearValue() used by methods that can't call ClearValue() directly.  (Example: SetValue() overrides.)
// Before using this function, verify that the caller doesn't actually need the full functionality of ClearValue().
void CDependencyObject::SetPropertyIsDefault(_In_ const CDependencyProperty* pDP)
{
    if (pDP->IsSparse())
    {
        if (m_pValueTable != nullptr)
        {
            SparseValueTable::iterator it = m_pValueTable->find(pDP->GetIndex());
            if (it != m_pValueTable->end())
            {
                it->second.SetIsSetLocally(false);
            }
        }
    }
    else
    {
        if (pDP->IsInheritedAttachedPropertyInStorageGroup())
        {
            if (m_pInheritedProperties != nullptr && m_pInheritedProperties->m_pWriter == this)
            {
                m_pInheritedProperties->SetPropertyFlag(pDP, InheritedPropertyFlag::IsSetLocally, false);
            }
        }
        else
        {
            ClearPropertyBitField(m_valid, MetadataAPI::GetPropertySlotCount(GetTypeIndex()), MetadataAPI::GetPropertySlot(pDP->GetIndex()));
        }
    }

    if (pDP->IsInherited())
    {
        InvalidateInheritedProperty(pDP);
    }
}


// Updates the "property value is default" bit for the given property.
// This is a subset of functionality of ClearValue() used by methods that can't call ClearValue() directly.  (Example: SetValue() overrides.)
// Before using this function, verify that the caller doesn't actually need the full functionality of ClearValue().
_Check_return_ HRESULT CDependencyObject::SetPropertyIsLocal(_In_ const CDependencyProperty* pDP)
{
    if (pDP->IsSparse())
    {
        ASSERT(m_pValueTable != nullptr);
        (*m_pValueTable)[pDP->GetIndex()].SetIsSetLocally(true);
    }
    else if (pDP->IsInheritedAttachedPropertyInStorageGroup())
    {
        // We would normally expect the property to have been written prior to
        // calling SetPropertyIsLocal, which would have caused
        // m_pInheritedProperties to exist and be owned by this DO.
        ASSERT(m_pInheritedProperties != nullptr && m_pInheritedProperties->m_pWriter == this);
        IFC_RETURN(EnsureInheritedProperties(this, pDP, false));
        m_pInheritedProperties->SetPropertyFlag(pDP, InheritedPropertyFlag::IsSetLocally, true);
    }
    else
    {
        SetPropertyBitField(m_valid, MetadataAPI::GetPropertySlotCount(GetTypeIndex()), MetadataAPI::GetPropertySlot(pDP->GetIndex()));
    }

    if (pDP->IsInherited())
    {
        InvalidateInheritedProperty(pDP);
    }

    return S_OK;
}
#pragma endregion

#pragma region ModifiedValue related methods
// Create a ModifiedValue instance for a  property, which will hold base and modified values when a modifier like animation
// takes effect.
_Check_return_ HRESULT CDependencyObject::CreateModifiedValue(
    _In_ const CDependencyProperty* dp,
    _Outptr_ std::shared_ptr<CModifiedValue>& modifiedValue)
{
    CValue value;
    auto instance = std::make_shared<CModifiedValue>(dp);

    EnsureModifiedValuesStorage().push_front(instance);

    // Get a ThemeResourceExtension for the current base value, if one exists.
    xref_ptr<CThemeResource> pThemeResource(GetThemeResourceNoRef(dp->GetIndex()));

    if (pThemeResource != nullptr)
    {
        value.SetThemeResourceAddRef(pThemeResource.get());
    }
    else
    {
        // Get current base value and store in instance.
        IFC_RETURN(GetValue(dp, &value));
    }

    IFC_RETURN(instance->SetBaseValue(value, GetBaseValueSource(dp)));

    modifiedValue = std::move(instance);

    return S_OK;
}

// Delete ModifiedValue instance for a property. This is done when there are no more modifiers, like animation, in effect.
void CDependencyObject::DeleteModifiedValue(_In_ const std::shared_ptr<CModifiedValue>& modifiedValue)
{
    ModifiedValuesList* modifiedValues = GetModifiedValuesStorage();

    if (modifiedValues)
    {
        modifiedValues->remove(modifiedValue);
    }
}

std::shared_ptr<CModifiedValue> CDependencyObject::GetModifiedValue(
    _In_ const CDependencyProperty* dp) const
{
    std::shared_ptr<CModifiedValue> result;

    const ModifiedValuesList* modifiedValues = GetModifiedValuesStorage();

    if (modifiedValues)
    {
        auto iter = std::find_if(
            modifiedValues->cbegin(),
            modifiedValues->cend(),
            [dp](const std::shared_ptr<CModifiedValue>& entry) -> bool
        {
            // Search list for ModifiedValue for the given property
            return dp->GetIndex() == entry->GetPropertyIndex();
        });

        if (iter != modifiedValues->cend())
        {
            result = *iter;
        }
    }

    return result;
}
#pragma endregion

#pragma region Bit field helpers
void CDependencyObject::ClearPropertyBitField(BitField& field, UINT16 propertyCount, UINT32 bit)
{
    ASSERT(bit < propertyCount);

    if (propertyCount <= 32)
    {
        field.bits &= ~(1 << bit);
    }
    // Only if we have allocated space (and have therefore set something) do we need to do anything.
    else if (nullptr != field.pointer)
    {
        // At least up to Windows 8.1 there was a possibility that we were called with an invalid DP (and thus invalid bit). Protect against
        // out of bounds when that happens.
        if (bit < propertyCount)
        {
            field.pointer[bit / 32] &= ~(1 << (bit & 31));
        }
    }
}

bool CDependencyObject::GetPropertyBitField(const BitField& field, UINT16 propertyCount, UINT32 bit) const
{
    ASSERT(bit < propertyCount);

    if (propertyCount <= 32)
    {
        return (field.bits & (1 << bit)) ? true : false;
    }
    else if (nullptr != field.pointer)
    {
        // At least up to Windows 8.1 there was a possibility that we were called with an invalid DP (and thus invalid bit). Protect against
        // out of bounds when that happens.
        if (bit < propertyCount)
        {
            return (field.pointer[bit / 32] & (1 << (bit & 31))) ? true : false;
        }
    }

    return false;
}

void CDependencyObject::SetPropertyBitField(BitField& field, UINT16 propertyCount, UINT bit)
{
    ASSERT(bit < propertyCount);

    if (propertyCount > 32)
    {
        if (!field.pointer)
        {
            field.pointer = new(ZERO_MEM_FAIL_FAST) UINT32[(propertyCount + 31) / 32];
            m_bitFields.fIsValidAPointer = true;
        }

        field.pointer[bit / 32] |= (1 << (bit & 31));
    }
    else
    {
        field.bits |= (1 << bit);
    }
}
#pragma endregion

#pragma region CValue validation
_Check_return_ HRESULT CDependencyObject::ValidateCValue(_In_ const CDependencyProperty* pDP, _In_ const CValue& value, _In_ ValueType valueType)
{
    switch (valueType)
    {
        case valueFloat:
        {
            IFC_RETURN(ValidateFloatValue(pDP->GetIndex(), value.AsFloat()));
            break;
        }

        case valueSigned:
        {
            IFC_RETURN(ValidateSignedValue(pDP->GetIndex(), value.AsSigned()));
            break;
        }

        case valueThickness:
        {
            IFC_RETURN(ValidateThicknessValue(pDP->GetIndex(), value.AsThickness()));
            break;
        }

        case valueCornerRadius:
        {
            IFC_RETURN(ValidateCornerRadiusValue(pDP->GetIndex(), value.AsCornerRadius()));
            break;
        }

        case valueGridLength:
        {
            IFC_RETURN(ValidateGridLengthValue(pDP->GetIndex(), value.AsGridLength()));
            break;
        }

        default: // no default value type
            break;
    }

    return S_OK;
}

// Validate float property value.
_Check_return_ HRESULT CDependencyObject::ValidateFloatValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ FLOAT value)
{
    switch (ePropertyIndex)
    {
    case KnownPropertyIndex::FrameworkElement_Width:
    case KnownPropertyIndex::FrameworkElement_Height:
        // Infinite values are not allowed.
        if (IsInfiniteF(value))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // NaN (auto) is only allowed for width & height
        // Values < 0 are invalid.
        // NB: MAKE SURE ISNANF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
        // but DLLs can enable floating-point exceptions in their code in a way that
        // affects the whole process, even when the NaN is non-signaling. To avoid that,
        // we'll test for NaN first by doing a comparison here that doesn't raise
        // a floating-point exception.
        if (!IsNanF(value) && value < 0)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        break;

    case KnownPropertyIndex::FrameworkElement_MaxHeight:
    case KnownPropertyIndex::FrameworkElement_MaxWidth:
    case KnownPropertyIndex::RowDefinition_MaxHeight:
    case KnownPropertyIndex::ColumnDefinition_MaxWidth:
    case KnownPropertyIndex::FrameworkElement_MinHeight:
    case KnownPropertyIndex::FrameworkElement_MinWidth:
        //negative values and NaN are not allowed for these properties
        if (IsNanF(value) || value < 0)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        break;

    case KnownPropertyIndex::TextBlock_FontSize:
    case KnownPropertyIndex::RichTextBlock_FontSize:
    case KnownPropertyIndex::TextElement_FontSize:
    case KnownPropertyIndex::Control_FontSize:
    case KnownPropertyIndex::ContentPresenter_FontSize:
        // Infinite values are not allowed.
        if (IsInfiniteF(value))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        //only finite positive values allowed for these properties
        if (IsNanF(value) || value <= 0)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        break;

    default:
        // Infinite values are not allowed.
        if (IsInfiniteF(value))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        if (IsNanF(value))
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ValidateSignedValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ INT32 value)
{
    switch (ePropertyIndex)
    {
        // Values < 0 are invalid.
    case KnownPropertyIndex::Grid_Row:
    case KnownPropertyIndex::Grid_Column:
        if (value < 0)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        break;

        // Values <= 0 are invalid
    case KnownPropertyIndex::Grid_RowSpan:
    case KnownPropertyIndex::Grid_ColumnSpan:
        if (value <= 0)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ValidateGridLengthValue(_In_ KnownPropertyIndex ePropertyIndex, _In_ XGRIDLENGTH* pGridLength)
{
    switch (ePropertyIndex)
    {
    case KnownPropertyIndex::RowDefinition_Height:
    case KnownPropertyIndex::ColumnDefinition_Width:
    case KnownPropertyIndex::RowDefinition_MaxHeight:
    case KnownPropertyIndex::RowDefinition_MinHeight:
    case KnownPropertyIndex::ColumnDefinition_MaxWidth:
    case KnownPropertyIndex::ColumnDefinition_MinWidth:
        IFC_RETURN(CGridLength::Validate(*pGridLength));
        break;
    default: // by default we don't know how to interpret XGRIDLENGTH value.
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ValidateThicknessValue(
    _In_ KnownPropertyIndex ePropertyIndex,
    _In_ const XTHICKNESS* thickness)
{
    switch (ePropertyIndex)
    {
        case KnownPropertyIndex::TextBlock_Padding:
        case KnownPropertyIndex::RichTextBlock_Padding:
        case KnownPropertyIndex::Border_Padding:
        case KnownPropertyIndex::Border_BorderThickness:
        case KnownPropertyIndex::Grid_BorderThickness:
        case KnownPropertyIndex::ContentPresenter_BorderThickness:
        case KnownPropertyIndex::StackPanel_BorderThickness:
        case KnownPropertyIndex::RelativePanel_BorderThickness:
            // NB: MAKE SURE ISNANF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
            // but DLLs can enable floating-point exceptions in their code in a way that
            // affects the whole process, even when the NaN is non-signaling. To avoid that,
            // we'll test for NaN first by doing a comparison here that doesn't raise
            // a floating-point exception.
            if (thickness
                && ((!IsNanF(thickness->left) && thickness->left < 0.0f)
                || (!IsNanF(thickness->top) && thickness->top < 0.0f)
                || (!IsNanF(thickness->right) && thickness->right < 0.0f)
                || (!IsNanF(thickness->bottom) && thickness->bottom < 0.0f)))
            {
                // Negative values are invalid for Padding and BorderThickness.
                IFC_RETURN(E_INVALIDARG);
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ValidateCornerRadiusValue(
    _In_ KnownPropertyIndex ePropertyIndex,
    _In_ const XCORNERRADIUS* cornerRadius)
{
    switch (ePropertyIndex)
    {
        case KnownPropertyIndex::Border_CornerRadius:
            // NB: MAKE SURE ISFINITEF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
            // but DLLs can enable floating-point exceptions in their code in a way that
            // affects the whole process, even when the NaN is non-signaling. To avoid that,
            // we'll test for NaN first by doing a comparison here that doesn't raise
            // a floating-point exception.
            if (!IsFiniteF(cornerRadius->topLeft) ||
                !IsFiniteF(cornerRadius->topRight) ||
                !IsFiniteF(cornerRadius->bottomRight) ||
                !IsFiniteF(cornerRadius->bottomLeft) ||
                cornerRadius->topLeft < 0.0f ||
                cornerRadius->topRight < 0.0f ||
                cornerRadius->bottomRight < 0.0f ||
                cornerRadius->bottomLeft < 0.0f)
            {
                // Negative values are not valid for Border.CornerRadius
                IFC_RETURN(E_INVALIDARG);
            }
            break;
    }

    return S_OK;
}
#pragma endregion

// Calling GetValue on an on-demand property will create it, well, on demand.
// To test for the existence of an on-demand property without creating it, use this method
CValue CDependencyObject::CheckOnDemandProperty(_In_ const CDependencyProperty* dp) const
{
    CDependencyObject* pDO = nullptr;
    ASSERT(dp->IsOnDemandProperty());
    ASSERT(dp->GetStorageType() == valueObject);
    ASSERT(!dp->IsPropMethodCall());

    if (dp->IsSparse()) {
        if (m_pValueTable) {
            auto iter = m_pValueTable->find(dp->GetIndex());
            if (iter != m_pValueTable->end()) {
                ASSERT(iter->second.value.GetType() == valueObject || iter->second.value.IsNull());
                pDO = iter->second.value.AsObject();
            }
        }
    }
    else {
        XHANDLE field = const_cast<CDependencyObject*>(this)->GetPropertyOffset(dp, /* forGetValue */ true);
        ASSERT(field);
        pDO = *(reinterpret_cast<CDependencyObject**>(field));
    }

    CValue result;
    result.SetObjectAddRef(pDO);
    return result;
}
CValue CDependencyObject::CheckOnDemandProperty(_In_ KnownPropertyIndex index) const
{
    return CheckOnDemandProperty(MetadataAPI::GetPropertyByIndex(index));
}

