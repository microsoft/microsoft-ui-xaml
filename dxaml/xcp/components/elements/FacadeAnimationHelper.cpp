// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DCompTreeHost.h>
#include <WRLHelper.h>
#include <Corep.h>
#include <PropertySetListener.h>
#include <FacadeReferenceWrapper.h>
#include <FacadeAnimationHelper.h>
#include "WindowRenderTarget.h"
#include <ErrorHelper.h>
#include <MetadataAPI.h>
#include "StableXbfIndexes.g.h"
#include <RuntimeProfiler.h>

using namespace DirectUI;

_Check_return_ HRESULT FacadeAnimationHelper::StartAnimationGroup(_In_ WUComp::ICompositionAnimationGroup* animationGroup)
{
    wrl::ComPtr<wfc::IIterable<WUComp::CompositionAnimation*>> iterable;
    VERIFYHR(animationGroup->QueryInterface(IID_PPV_ARGS(&iterable)));

    wrl::ComPtr<wfc::IIterator<WUComp::CompositionAnimation*>> iterator;
    IFCFAILFAST(iterable->First(&iterator));

    boolean hasCurrent = false;
    IFCFAILFAST(iterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        wrl::ComPtr<WUComp::ICompositionAnimation> animation;
        IFCFAILFAST(iterator->get_Current(&animation));

        wrl::ComPtr<WUComp::ICompositionAnimationBase> animationBase;
        VERIFYHR(animation.As(&animationBase));

        // Note:  Ideally we would implement the "strong guarantee" here, which would be to return all properties back to the
        // state they were in before StartAnimation was called if any failure occurs.
        // Returning failure here doesn't give us the strong guarantee, particularly in the case of having multiple animations
        // in the group, and failure happens on the 2nd or greater one, but it's quite tricky to implement, leaving it simple for now.
        IFC_RETURN(StartSingleAnimation(animationBase.Get(), false /* isImplicitAnimation */));

        IFCFAILFAST(iterator->MoveNext(&hasCurrent));
    }

    return S_OK;
}

_Check_return_ HRESULT FacadeAnimationHelper::StartSingleAnimation(_In_ WUComp::ICompositionAnimationBase* animation, bool isImplicitAnimation)
{
    // Figure out which property is being targeted, or return an error if it's invalid.
    KnownPropertyIndex facadeID;
    wrl_wrappers::HString targetProperty;
    IFC_RETURN(ValidateTargetProperty(animation, true, &facadeID, targetProperty.GetAddressOf()));

    // Not all facades are strict.  Validate strictness here, based on which facade is being animated.
    // Note that no strictness validation is enforced for StopAnimation, as it's not necessary.
    IFC_RETURN(m_object->ValidateStrictnessOnProperty(MetadataAPI::GetPropertyBaseByIndex(facadeID)));

    // Every facade is backed by a Composition object.  Create or retrieve that object now.
    wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();

    bool doHandOff = false;
    DOFacadeAnimationInfo* currentInfo = m_storage.TryGetFacadeAnimationInfo(m_object, facadeID);
    if (currentInfo != nullptr)
    {
        // There's already an animation running which targets this property.
        // If the target name of the new animation matches the current animation, we'll allow hand-off.
        // If the target name doesn't match, this becomes a more complex sub-channel targeting scenario
        // where multiple animations are targeting different sets of sub-channels.  Currently this isn't supported.
        xephemeral_string_ptr newTarget(GetAnimationTarget(animation).Get());
        xephemeral_string_ptr currentTarget(GetAnimationTarget(currentInfo->m_animation.Get()).Get());
        if (newTarget.Equals(currentTarget, xstrCompareCaseInsensitive))
        {
            doHandOff = true;
        }
        else
        {
            IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ANIMATION_MULTI_TARGET_UNSUPPORTED));
        }
    }
    else
    {
        // If we're not in a HandOff situation, make sure we have populated the backing PropertySet
        // with the current static value for this property, otherwise StartAnimation will fail.
        PopulateBackingCompositionObjectWithFacade(facadeID);
    }

    // Create a scoped batch and completion callback to listen for animation completion
    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
    DCompTreeHost* dcompTreeHost = m_context->GetDCompTreeHost();
    // Note:  We need the type of ScopedBatch that waits for ExpressionAnimations to complete (in addition to KeyFrame animations),
    // otherwise the ScopedBatch will complete immediately, which will confuse our book-keeping into thinking the animation has completed.
    IFC_RETURN(dcompTreeHost->GetCompositor()->CreateScopedBatch(WUComp::CompositionBatchTypes_AllAnimations, &scopedBatch));

    // Start the animation on the backing composition object.
    wrl::ComPtr<WUComp::ICompositionAnimation> ca;
    IFC_RETURN(animation->QueryInterface(IID_PPV_ARGS(&ca)));

    if (m_object->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // First try to use ICompositionObjectStatics::StartAnimationObjectWithIAnimationObject() for UIElement,
        // as this is needed to correctly handle expression animations with "this.Target" in the expression.
        // Facades_TODO:  Remove the type check once Brush classes implement IAnimationObject.

        // During the process of calling StartAnimationWithIAnimationObject(), Composition will call back into XAML to resolve
        // the target property.  This is the same code path used for resolving properties referenced by expression animations,
        // and would cause XAML to permanently create a backing CompositionObject as well as start tracking the referenced property.
        // This would be bad for perf and is unnecessary, so we temporarily mark the DO as "deferring references" using the
        // DeferReferenceGuard RAII object.
        DeferReferenceGuard guard(m_storage, m_object);

        wrl::ComPtr<WUComp::IAnimationObject> animationObject;
        VERIFYHR(reinterpret_cast<IUnknown*>(m_object->GetDXamlPeer())->QueryInterface(IID_PPV_ARGS(&animationObject)));

        wrl::ComPtr<WUComp::ICompositionObjectStatics> compFactory;
        IFCFAILFAST(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Microsoft_UI_Composition_CompositionObject).Get(), &compFactory));

        IFC_RETURN(compFactory->StartAnimationWithIAnimationObject(animationObject.Get(), GetAnimationTarget(animation).Get(), ca.Get()));
    }
    else
    {
        // Fallback to normal StartAnimation for objects that don't support IAnimationObject.
        IFC_RETURN(backingCO->StartAnimation(targetProperty.Get(), ca.Get()));
    }

    scopedBatch->End();

    // Tell the core about this facade animation, used for WaitForIdle
    m_context->IncrementActiveFacadeAnimationCount();

    if (doHandOff)
    {
        // Cleanup our book-keeping info on this animation without actually cancelling it,
        // this will allow the animation to hand off to the new one.
        m_context->DecrementActiveFacadeAnimationCount();
    }
    else
    {
        // Inform our PropertySet listener that an animation was started, it will start listening for changes.
        auto listener = m_storage.GetBackingListener(m_object);
        if (listener != nullptr)
        {
            listener->OnAnimationStarted(backingCO.Get(), facadeID);
        }
    }

    // Listen for animation completion
    DOFacadeAnimationInfo& info = m_storage.EnsureFacadeAnimationInfo(m_object, facadeID);
    info.m_facadeID = facadeID;
    info.m_animation = animation;
    info.m_scopedBatch = scopedBatch;
    info.m_animationRunning = true;
    info.m_isImplicitAnimation = isImplicitAnimation;

    IFCFAILFAST(scopedBatch->add_Completed(m_callbacks->CreateCompletedCallback().Get(), &info.m_token));

    InstrumentStartSingleAnimation(facadeID);

    return S_OK;
}

void FacadeAnimationHelper::InstrumentStartSingleAnimation(_In_ KnownPropertyIndex facadeID)
{
    switch (facadeID)
    {
        case KnownPropertyIndex::UIElement_Opacity:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_Opacity);
            break;

        case KnownPropertyIndex::UIElement_CenterPoint:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_CenterPoint);
            break;

        case KnownPropertyIndex::UIElement_Rotation:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_Rotation);
            break;

        case KnownPropertyIndex::UIElement_RotationAxis:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_RotationAxis);
            break;

        case KnownPropertyIndex::UIElement_Scale:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_Scale);
            break;

        case KnownPropertyIndex::UIElement_Translation:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_Translation);
            break;

        case KnownPropertyIndex::UIElement_TransformMatrix:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_TransformMatrix);
            break;
    }
}

_Check_return_ HRESULT FacadeAnimationHelper::StopAnimationGroup(_In_ WUComp::ICompositionAnimationGroup* animationGroup)
{
    wrl::ComPtr<wfc::IIterable<WUComp::CompositionAnimation*>> iterable;
    VERIFYHR(animationGroup->QueryInterface(IID_PPV_ARGS(&iterable)));

    wrl::ComPtr<wfc::IIterator<WUComp::CompositionAnimation*>> iterator;
    IFCFAILFAST(iterable->First(&iterator));

    boolean hasCurrent = false;
    IFCFAILFAST(iterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        wrl::ComPtr<WUComp::ICompositionAnimation> animation;
        IFCFAILFAST(iterator->get_Current(&animation));

        wrl::ComPtr<WUComp::ICompositionAnimationBase> animationBase;
        VERIFYHR(animation.As(&animationBase));

        IFC_RETURN(StopSingleAnimation(animationBase.Get()));

        IFCFAILFAST(iterator->MoveNext(&hasCurrent));
    }

    return S_OK;
}

_Check_return_ HRESULT FacadeAnimationHelper::StopSingleAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    KnownPropertyIndex facadeID;
    wrl_wrappers::HString targetProperty;
    IFC_RETURN(ValidateTargetProperty(animation, true, &facadeID, targetProperty.GetAddressOf()));

    DOFacadeAnimationInfo* animationInfo = m_storage.TryGetFacadeAnimationInfo(m_object, facadeID);
    if (animationInfo == nullptr || !animationInfo->m_animationRunning)
    {
        // Composition returns S_FALSE in this situation, mirror that policy here
        return S_FALSE;
    }

    wrl::ComPtr<WUComp::ICompositionObject> backingCO = FacadeAnimationHelper::EnsureBackingCompositionObjectForFacade();

    IFC_RETURN(backingCO->StopAnimation(targetProperty.Get()));
    animationInfo->m_animationRunning = false;

    // Now that we've stopped the animation, we'll simply wait for the scoped batch to complete.
    // This will allow us to query for the final value and propagate it back into the property system.

    return S_OK;
}

// Forcefully stop the animation of given facade if an animation is currently running, otherwise do nothing.
void FacadeAnimationHelper::CancelSingleAnimation(KnownPropertyIndex facadeID)
{
    // This function is used in the context of setting a static value while an animation is running.
    // The behavior we want is to stop the animation and don't push a final value back into the property system.
    // We accomplish this by unregistering our scoped batch completion handler and cleaning up the m_storage.

    // Query for animation info for this facade.
    DOFacadeAnimationInfo* animationInfo = m_storage.TryGetFacadeAnimationInfo(m_object, facadeID);
    if (animationInfo != nullptr)
    {
        if (animationInfo->m_animationRunning)
        {
            // Stop the animation
            wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();
            IFCFAILFAST(backingCO->StopAnimation(GetCompositionTarget(facadeID).Get()));
        }

        CleanupAnimationOnCompletion(animationInfo);
    }
}

_Check_return_ HRESULT FacadeAnimationHelper::ScopedBatchCompleted(_In_ IInspectable* sender, _In_ WUComp::ICompositionBatchCompletedEventArgs* args)
{
    // The Completed event handler signature is a bit limited - it provides only the ScopedBatch as a way to identify
    // which animation has completed.  We'll take that ScopedBatch and perform a reverse lookup in our map for the
    // animation info we have in order to identify which facade this animation is associated with.
    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
    VERIFYHR(sender->QueryInterface(IID_PPV_ARGS(&scopedBatch)));
    DOFacadeAnimationInfo* animationInfo = m_storage.GetFacadeAnimationInfoForScopedBatch(m_object, scopedBatch.Get());

    KnownPropertyIndex facadeID = animationInfo->m_facadeID;
    wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();

    CleanupAnimationOnCompletion(animationInfo);

    // Now that we know which facade was being animated, query the backing PropertySet for the appropriate value
    // and propagate the now final value back into the property system.
    VERIFYHR(m_callbacks->PullFacadePropertyValueFromCompositionObject(backingCO.Get(), facadeID));

    return S_OK;
}

void FacadeAnimationHelper::CleanupAnimationOnCompletion(_In_ DOFacadeAnimationInfo* animationInfo)
{
    KnownPropertyIndex facadeID = animationInfo->m_facadeID;
    wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();

    // Inform our PropertySetListener that the animation is no longer running - it will stop listening for changes
    auto listener = m_storage.GetBackingListener(m_object);
    if (listener != nullptr)
    {
        listener->OnAnimationCompleted(backingCO.Get(), facadeID);
    }

    // Clear out our book-keeping data about this animation.
    m_storage.ClearFacadeAnimationInfo(m_object, animationInfo->m_facadeID);

    m_callbacks->FacadeAnimationComplete(facadeID);

    // If we no longer have any facade animations running, clear this CompNode requirement
    if (!m_storage.HasAnyFacadeAnimations(m_object))
    {
        m_callbacks->AllFacadeAnimationsComplete();
    }

    // Tell the core this animation completed, used for WaitForIdle
    m_context->DecrementActiveFacadeAnimationCount();
}

_Check_return_ HRESULT FacadeAnimationHelper::ValidateTargetProperty(_In_ WUComp::ICompositionAnimationBase* animation, bool validateHasWriteAccess, _Out_ KnownPropertyIndex* facadeID, _Out_opt_ HSTRING* compositionTarget)
{
    wrl::ComPtr<WUComp::ICompositionAnimation2> animation2;
    IFC_RETURN(animation->QueryInterface(IID_PPV_ARGS(&animation2)));
    wrl_wrappers::HString target;
    IFC_RETURN(animation2->get_Target(target.GetAddressOf()));

    return ValidateTargetProperty(target.Get(), validateHasWriteAccess, facadeID, compositionTarget);
}

wrl_wrappers::HStringReference FacadeAnimationHelper::GetCompositionTarget(KnownPropertyIndex facadeID)
{
    const FacadeMatcherEntry* entries;
    size_t count;
    m_callbacks->GetFacadeEntries(&entries, &count);

    for (size_t i = 0; i < count; i++)
    {
        if (entries[i].m_facadeID == facadeID)
        {
            if (entries[i].m_compPropertyName == nullptr)
            {
                return wrl_wrappers::HStringReference(entries[i].m_propertyName);
            }
            else
            {
                return wrl_wrappers::HStringReference(entries[i].m_compPropertyName);
            }
        }
    }
    IFCFAILFAST(E_INVALIDARG);
    return wrl_wrappers::HStringReference(L"");
}

_Check_return_ HRESULT FacadeAnimationHelper::ValidateTargetProperty(HSTRING target, bool validateHasWriteAccess, _Out_ KnownPropertyIndex* facadeID, _Out_opt_ HSTRING* compositionTarget)
{
    xephemeral_string_ptr strTarget(target);
    UINT32 propertyNameLength = 0;
    if (strTarget.IsNull())
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ANIMATION_TARGET_UNSPECIFIED));
    }

    const FacadeMatcherEntry* entries;
    size_t count;
    m_callbacks->GetFacadeEntries(&entries, &count);

    bool found = false;
    bool supportsSubChannels = false;
    bool isReadOnly = false;
    LPCWSTR compPropertyName = nullptr;
    for (size_t i = 0; i < count; i++)
    {
        xephemeral_string_ptr propertyName(entries[i].m_propertyName, entries[i].m_bufferSize - 1);
        if (strTarget.StartsWith(propertyName, xstrCompareCaseInsensitive))
        {
            *facadeID = entries[i].m_facadeID;
            propertyNameLength = entries[i].m_bufferSize - 1;
            supportsSubChannels = entries[i].m_supportsSubChannels;
            isReadOnly = entries[i].m_isReadOnly;
            compPropertyName = entries[i].m_compPropertyName;
            found = true;
            break;
        }
    }
    if (!found)
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(E_INVALIDARG, ERROR_ANIMATION_PROPERTY_UNRECOGNIZED, strTarget));
    }

    if (validateHasWriteAccess && isReadOnly)
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(E_ACCESSDENIED, ERROR_ANIMATION_PROPERTY_READONLY, strTarget));
    }

    // No subchannels were used
    if (strTarget.GetCount() == propertyNameLength)
    {
        if (compositionTarget != nullptr)
        {
            if (compPropertyName == nullptr)
            {
                IFCFAILFAST(::WindowsDuplicateString(target, compositionTarget));
            }
            else
            {
                IFCFAILFAST(wrl_wrappers::HStringReference(compPropertyName).CopyTo(compositionTarget));
            }
        }
        return S_OK;
    }

    // The property doesn't handle subchannels or the character following the name isn't a "."
    if (!supportsSubChannels || strTarget.GetChar(propertyNameLength) != L'.')
    {
        // The app supplied a target name that is a superstring of one of our properties, but not one we recognize.
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(E_INVALIDARG, ERROR_ANIMATION_PROPERTY_UNRECOGNIZED, strTarget));
    }

    // We are all good, so if we don't need to construct the composition property name, return
    if (compositionTarget == nullptr)
    {
        return S_OK;
    }

    // If our compPropertyName is null, then this is just the original passed in name (with subchannel)
    if (compPropertyName == nullptr)
    {
        IFCFAILFAST(::WindowsDuplicateString(target, compositionTarget));
        return S_OK;
    }

    // Concatenate the subchannel information with the real property name
    xstring_ptr propertyNamePart;
    xstring_ptr subchannelPart;
    xstring_ptr newTarget;
    xruntime_string_ptr newTargetPromoted;
    IFCFAILFAST(strTarget.SubString(propertyNameLength, strTarget.GetCount(), &subchannelPart));
    IFCFAILFAST(xstring_ptr::CloneBuffer(compPropertyName, &propertyNamePart));
    IFCFAILFAST(xstring_ptr::Concatenate(propertyNamePart, 0, subchannelPart, 0, &newTarget));
    IFCFAILFAST(newTarget.Promote(&newTargetPromoted));
    *compositionTarget = newTargetPromoted.DetachHSTRING();
    return S_OK;
}

// Helper function to retrieve the Target property from the given animation
wrl_wrappers::HString FacadeAnimationHelper::GetAnimationTarget(_In_ WUComp::ICompositionAnimationBase* animation)
{
    // Note that this function assumes the animation has already successfully run through ValidateTargetProperty(),
    // if not, you run the risk of fail-fast if either of the checks below fail.
    wrl::ComPtr<WUComp::ICompositionAnimation2> animation2;
    IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&animation2)));
    wrl_wrappers::HString target;
    IFCFAILFAST(animation2->get_Target(target.GetAddressOf()));

    return target;
}

wrl::ComPtr<WUComp::ICompositionObject> FacadeAnimationHelper::EnsureBackingCompositionObjectForFacade()
{
    wrl::ComPtr<WUComp::ICompositionObject> backingCO = m_storage.GetBackingCompositionObject(m_object);
    if (backingCO == nullptr)
    {
        wrl::ComPtr<IFacadePropertyListener> backingListener;
        m_callbacks->CreateBackingCompositionObjectForFacade(m_context->GetDCompTreeHost()->GetCompositor(), &backingCO, &backingListener);
        m_storage.SetBackingCompositionObject(m_object, backingCO.Get());
        m_storage.SetBackingListener(m_object, backingListener.Get());
    }

    return backingCO;
}

void FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacade(KnownPropertyIndex facadeID)
{
    wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();
    m_callbacks->PopulateBackingCompositionObjectWithFacade(backingCO.Get(), facadeID);

    // Inform our PropertySet listener that we've set a PropertySet value, used for building the mapping from facadeID to DComp PropertyID.
    auto listener = m_storage.GetBackingListener(m_object);
    if (listener != nullptr)
    {
        listener->OnFacadePropertyInserted(facadeID);
    }
}

// Helper to propagate current property into backing CompositionObject, but only if the property is referenced.
// Whenever a referenced property changes, this will keep the backing property up to date.
void FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferencedImpl(KnownPropertyIndex facadeID)
{
    wrl::ComPtr<FacadeReferenceWrapper> referenceWrapper = m_storage.GetReferenceWrapper(m_object);
    if (referenceWrapper != nullptr)
    {
        if (referenceWrapper->IsPropertyIDReferenced(facadeID))
        {
            wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();
            m_callbacks->PopulateBackingCompositionObjectWithFacade(backingCO.Get(), facadeID);
        }
    }
}

wrl::ComPtr<FacadeReferenceWrapper> FacadeAnimationHelper::EnsureFacadeReferenceWrapper()
{
    wrl::ComPtr<FacadeReferenceWrapper> referenceWrapper = m_storage.GetReferenceWrapper(m_object);
    if (referenceWrapper == nullptr)
    {
        referenceWrapper.Attach(new FacadeReferenceWrapper());
        m_storage.SetReferenceWrapper(m_object, referenceWrapper.Get());
    }

    return referenceWrapper;
}

// References:  Given a candidate property name string, validate it and if valid return wrapper around backing CO.
_Check_return_ HRESULT FacadeAnimationHelper::PopulatePropertyInfoImpl(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    KnownPropertyIndex facadeID;
    IFC_RETURN(ValidateTargetProperty(propertyName, false, &facadeID));

    wrl::ComPtr<WUComp::ICompositionObject> backingCO = EnsureBackingCompositionObjectForFacade();

    // If we already have an animation running targeting this facade, make sure not to populate the backing CO
    // with a property value, otherwise setting the value will stomp over the animation.
    if (!m_storage.HasAnimation(m_object, facadeID))
    {
        PopulateBackingCompositionObjectWithFacade(facadeID);
    }

    wrl::ComPtr<WUComp::IAnimationObject> backingCOAnimatable;
    VERIFYHR(backingCO.As(&backingCOAnimatable));

    IFCFAILFAST(backingCOAnimatable->PopulatePropertyInfo(propertyName, animationPropertyInfo));

    if (!m_storage.IsDeferringReferences(m_object))
    {
        wrl::ComPtr<FacadeReferenceWrapper> wrapper = EnsureFacadeReferenceWrapper();
        wrapper->AddReferencedPropertyID(facadeID);
    }

    InstrumentPopulatePropertyInfo(facadeID);
    return S_OK;
}

void FacadeAnimationHelper::InstrumentPopulatePropertyInfo(_In_ KnownPropertyIndex facadeID)
{
    switch (facadeID)
    {
        case KnownPropertyIndex::UIElement_ActualOffset:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_ActualOffset);
            break;

        case KnownPropertyIndex::UIElement_ActualSize:
            __RP_Marker_ClassMemberById(Parser::StableXbfTypeIndex::UIElement, Parser::StableXbfPropertyIndex::UIElement_ActualSize);
            break;
    }
}
