// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Setter.h>
#include <CPropertyPath.h>
#include <TargetPropertyPath.h>
#include <xcperrorresource.h>
#include <corep.h>
#include <timemgr.h>
#include <animation.h>
#include <host.h>
#include <DeferredAnimationOperation.h>
#include <UIThreadScheduler.h>
#include "VisualStateSetterHelper.h"
#include "CVisualStateManager2.h"
#include "DOPointerCast.h"
#include "setterbasecollection.h"
#include "VisualState.h"

namespace VisualStateSetterHelper
{
    _Check_return_ HRESULT ResolveSetterAnimationTargets(
        _In_ CSetter* setter,
        _Out_ xref_ptr<CDependencyObject>& modifiedObject,
        _Outptr_ const CDependencyProperty** modifiedProperty,
        _Out_ CValue& value)
    {
        // Verify that the setter actually has a target set
        xref_ptr<CTargetPropertyPath> targetPropertyPath = setter->GetTargetPropertyPath();
        if (!targetPropertyPath)
        {
            if (CTimeManager::ShouldFailFast())
            {
                IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
            }
            else
            {
                IFC_RETURN(setter->SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_VSM_SETTER_MISSING_TARGET));
            }
        }

        xref_ptr<CDependencyObject> targetObject;
        xref_ptr<CDependencyObject> targetPropertyOwner;
        const CDependencyProperty* targetProperty;

        // Resolve the target object
        IFC_RETURN(targetPropertyPath->ResolveTargetForVisualStateSetter(
            setter,
            targetObject,
            targetPropertyOwner,
            &targetProperty));

        ASSERT(targetPropertyOwner && targetProperty);

        // Verify that the setter actually has a value set
        IFC_RETURN(setter->ResolveValueUsingProperty(targetProperty));
        IFC_RETURN(setter->GetSetterValue(&value));
        if (value.IsUnset())
        {
            if (CTimeManager::ShouldFailFast())
            {
                IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
            }
        }

        modifiedObject = targetPropertyOwner;
        *modifiedProperty = targetProperty;

        return S_OK;
    }

    // Performs the specified operation (set/unset) given the provided target object,
    // target property, and desired value. Set operations should not provide an unset value;
    // for unset operations, the provided value is mostly ignored. By default, this follows
    // general animation semantics, i.e. the [Set,Clear]AnimatedValue is performed synchronously
    // if it is a built-in property, otherwise it is deferred to the next tick. For some
    // scenarios (e.g. refreshing the applied value after a theme change notification)
    // it is necessary to force all operations to be deferred to the next tick.
    _Check_return_ HRESULT PerformAnimatedValueOperation(
        _In_ SetterOperation operation,
        _In_ xref_ptr<CDependencyObject> targetObject,
        _In_ const CDependencyProperty* targetProperty,
        _In_ const CValue value,
        _In_ xref_ptr<CDependencyObject> originalSetter,
        _In_ const bool forceDeferOperation)
    {
        // The guts of this method are a slimmed down version of CAnimation::DoAnimationValueOperation
        // along with some other magic that would otherwise be provided by the animation framework

        // Allow and no-op unset VisualState setters
        if (operation == SetterOperation::Set && value.IsUnset())
        {
            return S_OK;
        }

        ASSERT((operation == SetterOperation::Set && !value.IsUnset()) || (operation == SetterOperation::Unset));

        if (!targetObject)
        {
            // Might be null if the object got cleaned up
            return S_OK;
        }

        auto context = targetObject->GetContext();

        // Animation can run while the target object's delete is pending. Although the
        // core object is currently alive, the peer may have been deleted or may be
        // queued for release in UIAffinityReleaseQueue. No-op if delete is
        // pending. If the peer exists and is valid, peg it during this operation.
        bool peggedPeer = false;
        bool peerIsPendingDelete = false;
        auto cleanupGuard = wil::scope_exit([&]
        {
            if (peggedPeer)
            {
                targetObject->UnpegManagedPeer();
            }
        });

        targetObject->TryPegPeer(&peggedPeer, &peerIsPendingDelete);
        if (peerIsPendingDelete)
        {
            return S_OK;
        }

        // See if we are animating this property and, if so, tell the active animation to
        // give up control of the property. Additionally, remove that animation from the timing
        // manager. This will prevent the value we're about to set from being clobbered by the
        // aforementioned animation (if it's part of a VSM storyboard) when it's stopped later by
        // VisualStateManagerActuator::StopAndRemoveStoryboards.
        CTimeManager *pTimeManager = context->GetTimeManager();
        IFCPTR_RETURN(pTimeManager);
        auto pActiveAnimation = pTimeManager->GetAnimationOnProperty(
            xref::get_weakref(targetObject),
            targetProperty->GetIndex());
        if (pActiveAnimation)
        {
            pTimeManager->ClearAnimationOnProperty(
                xref::get_weakref(targetObject),
                targetProperty->GetIndex());

            pActiveAnimation->ReleaseControlOfTarget();
        }

        CValue setterValue;
        switch (operation)
        {
            case SetterOperation::Set:
            {
                ASSERT(!value.IsUnset());
                IFC_RETURN(setterValue.CopyConverted(value));

                // ObjectAnimationUsingKeyFrames is always considered dependent
                // (because changes are instantaneous and thus can't be interpolated smoothly)
                setterValue.GetCustomData().SetIsIndependent(false);
                break;
            }
            case SetterOperation::Unset:
            {
                setterValue.Unset();
                break;
            }
            default:
            {
                ASSERT(false);
                break;
            }
        }
        // For core properties, just set the property directly as long as the caller
        // didn't request that the operation be deferred until the next tick.
        // For custom properties, queue the call to set the property, so that we're
        // not calling out into app code until we're in a safe state.
        bool shouldSetPropertySynchronously =    !targetProperty->Is<CCustomDependencyProperty>()
                                              && !forceDeferOperation;

        if (shouldSetPropertySynchronously)
        {
            // Since we're modifying this property synchronously, any deferred animated value modifications
            // are now obsolete, so make sure those are cleared out (otherwise they'll clobber us in the near
            // future)
            context->RemoveMatchingDeferredAnimationOperations(targetObject.get(), targetProperty->GetIndex());

            if (operation == SetterOperation::Set)
            {
                IFC_RETURN(targetObject->SetAnimatedValue(targetProperty, setterValue, originalSetter));
            }
            else
            {
                ASSERT(operation == SetterOperation::Unset);

                IFC_RETURN(targetObject->ClearAnimatedValue(targetProperty, setterValue));
            }
        }

        // MSFT:11036284
        // In order to match the behavior of VSM (because VSM is a special snowflake and therefore the worst),
        // we need to also queue up a deferred animation operation even if we decided previously that it needed to be
        // performed synchronously. Specifically, VSM tells the storyboard to set its first frame value synchronously,
        // in addition to starting the storyboard normally. When the next tick happens, the storyboard does its normal
        // stuff; in the case of a DiscreteObjectKeyFrameAnimation, this means that the value is actually applied *twice*,
        // once "synchronously" (this may actually be deferred, e.g. it's a custom property), and once on the next tick.
        //
        // See VisualStateManagerActuator::AttemptStartStoryboard() for the special snowflake behavior.

        // Queue the deferred operation
        context->EnqueueDeferredAnimationOperation(
            std::make_shared<CDeferredAnimationOperation>(
                std::make_pair(targetObject, targetProperty->GetIndex()),
                setterValue,
                !!peggedPeer,
                (operation == SetterOperation::Set)
                ? CDeferredAnimationOperation::DeferredOperation::Set
                : CDeferredAnimationOperation::DeferredOperation::Clear,
                originalSetter));

        // CDeferredAnimationOperation's dtor will unpeg
        cleanupGuard.release();

        // Schedule a tick to ensure the deferred animation operation queue gets flushed
        IXcpBrowserHost *pBH = context->GetBrowserHost();
        if (pBH)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

            if (pFrameScheduler)
            {
                VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::VSMAnimation));
            }
        }

        return S_OK;
    }

    void PerformSetterOperationIfStateActive(_In_ CVisualState* visualState, _In_ CSetter* setter, _In_ VisualStateSetterHelper::SetterOperation operation)
    {
        if (setter && CVisualStateManager2::IsActiveVisualState(visualState) && setter->GetTargetPropertyPath())
        {
            xref_ptr<CDependencyObject> targetObject;
            xref_ptr<CDependencyObject> targetPropertyOwner;
            const CDependencyProperty* targetProperty;

            // Try to resolve the target object. If the setter was added to a collection during Edit & Continue before
            // it had the target or property set, then this would fail.
            if (FAILED(setter->GetTargetPropertyPath()->ResolveTargetForVisualStateSetter(
                setter,
                targetObject,
                targetPropertyOwner,
                &targetProperty)))
            {
                // Expected failure case if Setter isn't valid.
                return;
            }
            CValue value;
            if (operation == VisualStateSetterHelper::SetterOperation::Set)
            {
                // As before, if the value hasn't been set yet, we won't fail.
                VERIFYHR(setter->ResolveValueUsingProperty(targetProperty));
                VERIFYHR(setter->GetSetterValue(&value));
            }
            else
            {
                value.Unset();
            }

            const HRESULT hr = VisualStateSetterHelper::PerformAnimatedValueOperation(
                operation,
                targetPropertyOwner,
                targetProperty,
                value,
                xref_ptr<CDependencyObject>(setter));
            MICROSOFT_TELEMETRY_ASSERT_HR(hr);
        }
    }

    _Check_return_ CVisualState* GetVisualStateSetterVisualState(_In_ CSetter* setter)
    {
        CDependencyObject* setterBaseCollectionAsCDO = setter->GetParentInternal(false);
        CSetterBaseCollection* setterBaseCollection = do_pointer_cast<CSetterBaseCollection>(setterBaseCollectionAsCDO);

        if (setterBaseCollection)
        {
            CDependencyObject* visualStateAsCDO = setterBaseCollectionAsCDO->GetParentInternal(false);
            return do_pointer_cast<CVisualState>(visualStateAsCDO);
        }

        return nullptr;
    }
}
