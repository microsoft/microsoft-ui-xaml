// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "WRLHelper.h"
#include "FacadeStorage.h"

namespace Microsoft {
    namespace WRL   {
        namespace Wrappers  {
            class HString;
        }
    }
}


interface IFacadePropertyListener;
class DOFacadeAnimationInfo;

struct FacadeMatcherEntry
{
    const wchar_t* m_propertyName;
    UINT m_bufferSize;
    KnownPropertyIndex m_facadeID;
    bool m_supportsSubChannels;
    bool m_isReadOnly;
    const wchar_t* m_compPropertyName;
};

// Virtual base class for FacadeAnimationHelpersCallbacks so that we can get to them in a cpp file
// and not have to write huge amounts of code into headers.
class FacadeAnimationHelperCallbacks
{
public:
    virtual void GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count) = 0;
    virtual void PopulateBackingCompositionObjectWithFacade(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID) = 0;
    virtual _Check_return_ HRESULT PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeId) = 0;
    virtual void FacadeAnimationComplete(KnownPropertyIndex animatedProperty) = 0;
    virtual void AllFacadeAnimationsComplete() = 0;
    virtual void CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener) = 0;
    virtual wrl::ComPtr<wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>> CreateCompletedCallback() = 0;
};

// Typed class that will forward the callbacks to a strongly typed object
template <class T>
class TypedFacadeAnimationHelperCallbacks : public FacadeAnimationHelperCallbacks
{
private:
    T* m_object;
public:
    template <class T>
    TypedFacadeAnimationHelperCallbacks(T* object) : m_object(object)
    {
    }

    void GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count) override
    {
        m_object->GetFacadeEntries(entries, count);
    }
    void PopulateBackingCompositionObjectWithFacade(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID) override
    {
        m_object->PopulateBackingCompositionObjectWithFacade(backingCO, facadeID);
    }
    _Check_return_ HRESULT PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID) override
    {
        return m_object->PullFacadePropertyValueFromCompositionObject(backingCO, facadeID);
    }
    void FacadeAnimationComplete(KnownPropertyIndex animatedProperty) override
    {
        m_object->FacadeAnimationComplete(animatedProperty);
    }
    void AllFacadeAnimationsComplete() override
    {
        m_object->AllFacadeAnimationsComplete();
    }
    void CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener) override
    {
        m_object->CreateBackingCompositionObjectForFacade(compositor, backingCO, listener);
    }

    wrl::ComPtr<wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>> CreateCompletedCallback() override
    {
        xref::weakref_ptr<T> elementWeak = xref::get_weakref(m_object);
        auto callback = WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>>(
            [elementWeak](IInspectable* sender, WUComp::ICompositionBatchCompletedEventArgs* pEventArgs) -> HRESULT
        {
            return FacadeAnimationHelper::OnScopedBatchCompleted(elementWeak.lock_noref(), sender, pEventArgs);
        });
        return callback;
    }
};

class FacadeAnimationHelper
{
private:
    template <class T>
    FacadeAnimationHelper(T* object, FacadeAnimationHelperCallbacks* callbacks) :
        m_object(object),
        m_context(object->GetContext()),
        m_callbacks(callbacks),
        m_storage(m_context->GetFacadeStorage())
    {
    }

public:
    template <class T>
    static _Check_return_ HRESULT StartAnimation(_In_ T* element, _In_ WUComp::ICompositionAnimationBase* animation, bool isImplicitAnimation)
    {
        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);

        wrl::ComPtr<WUComp::ICompositionAnimationGroup> animationGroup;
        if (SUCCEEDED(animation->QueryInterface(IID_PPV_ARGS(&animationGroup))))
        {
            // Implicit animations come from Scalar/Vector3Transitions, which never create an animation group.
            ASSERT(!isImplicitAnimation);

            return helper.StartAnimationGroup(animationGroup.Get());
        }
        else
        {
            return helper.StartSingleAnimation(animation, isImplicitAnimation);
        }
    }

    template <class T>
    static _Check_return_ HRESULT StopAnimation(_In_ T* element, _In_ WUComp::ICompositionAnimationBase* animation)
    {
        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);

        wrl::ComPtr<WUComp::ICompositionAnimationGroup> animationGroup;
        if (SUCCEEDED(animation->QueryInterface(IID_PPV_ARGS(&animationGroup))))
        {
            return helper.StopAnimationGroup(animationGroup.Get());
        }
        else
        {
            return helper.StopSingleAnimation(animation);
        }
    }

    template <class T>
    static void CancelAnimation(_In_ T* element, KnownPropertyIndex facadeID)
    {
        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);
        helper.CancelSingleAnimation(facadeID);
    }

    template <class T>
    static _Check_return_ HRESULT OnScopedBatchCompleted(_In_ T* element, _In_ IInspectable* sender, _In_ WUComp::ICompositionBatchCompletedEventArgs* pEventArgs)
    {
        // If our element is null then our object was destructed as this was being called.
        if (element == nullptr) return S_OK;

        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);
        return helper.ScopedBatchCompleted(sender, pEventArgs);
    }


    template <class T>
    static void PopulateBackingCompositionObjectWithFacadeIfReferenced(_In_ T* element, KnownPropertyIndex facadeID)
    {
        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);
        helper.PopulateBackingCompositionObjectWithFacadeIfReferencedImpl(facadeID);
    }

    template <class T>
    static _Check_return_ HRESULT PopulatePropertyInfo(
        _In_ T* element,
        _In_ HSTRING propertyName,
        _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
        )
    {
        TypedFacadeAnimationHelperCallbacks<T> callbacks(element);
        FacadeAnimationHelper helper(element, &callbacks);
        return helper.PopulatePropertyInfoImpl(propertyName, animationPropertyInfo);
    }

private:
    _Check_return_ HRESULT StartAnimationGroup(_In_ WUComp::ICompositionAnimationGroup* animationGroup);
    _Check_return_ HRESULT StartSingleAnimation(_In_ WUComp::ICompositionAnimationBase* animation, bool isImplicitAnimation);
    _Check_return_ HRESULT StopAnimationGroup(_In_ WUComp::ICompositionAnimationGroup* animationGroup);
    _Check_return_ HRESULT StopSingleAnimation(_In_ WUComp::ICompositionAnimationBase* animation);
    void CancelSingleAnimation(KnownPropertyIndex facadeID);

    void CleanupAnimationOnCompletion(_In_ DOFacadeAnimationInfo* animationInfo);
    _Check_return_ HRESULT ScopedBatchCompleted(_In_ IInspectable* sender, _In_ WUComp::ICompositionBatchCompletedEventArgs* pEventArgs);

    _Check_return_ HRESULT ValidateTargetProperty(_In_ WUComp::ICompositionAnimationBase* animation, bool validateHasWriteAccess, _Out_ KnownPropertyIndex* facadeID, _Out_opt_ HSTRING* compositionTarget = nullptr);
    _Check_return_ HRESULT ValidateTargetProperty(HSTRING target, bool validateHasWriteAccess, _Out_ KnownPropertyIndex* facadeID, _Out_opt_ HSTRING* compositionTarget = nullptr);
    wrl_wrappers::HStringReference GetCompositionTarget(KnownPropertyIndex facadeID);

    wrl::ComPtr<WUComp::ICompositionObject> EnsureBackingCompositionObjectForFacade();

    static wrl_wrappers::HString GetAnimationTarget(_In_ WUComp::ICompositionAnimationBase* animation);

    static void InstrumentStartSingleAnimation(_In_ KnownPropertyIndex facadeID);
    static void InstrumentPopulatePropertyInfo(_In_ KnownPropertyIndex facadeID);
    void PopulateBackingCompositionObjectWithFacade(KnownPropertyIndex facadeID);
    void PopulateBackingCompositionObjectWithFacadeIfReferencedImpl(KnownPropertyIndex facadeID);

    wrl::ComPtr<FacadeReferenceWrapper> EnsureFacadeReferenceWrapper();

    _Check_return_ HRESULT PopulatePropertyInfoImpl(
        _In_ HSTRING propertyName,
        _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
        );

private:
    FacadeAnimationHelperCallbacks* m_callbacks;
    CDependencyObject* m_object;
    CCoreServices* m_context;
    FacadeStorage& m_storage;
};
