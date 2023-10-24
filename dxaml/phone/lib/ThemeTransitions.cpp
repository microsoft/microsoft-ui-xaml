// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Animation
{
    const DOUBLE NavigateTransitionHelper::TURNSTILE_AXIS_X = -0.1;
    const DOUBLE NavigateTransitionHelper::TURNSTILE_AXIS_Z = -100.0;

    const DOUBLE NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN = 0.5;
    const DOUBLE NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT = 0.5;

    // NavigationThemeTransition

    NavigationThemeTransition::NavigationThemeTransition()
    {

    }

    NavigationThemeTransition::~NavigationThemeTransition()
    {

    }

    _Check_return_
    HRESULT
    NavigationThemeTransition::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_animation::ITransitionFactoryPrivate> spInnerFactory;
        wrl::ComPtr<xaml_animation::ITransition> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(NavigationThemeTransitionFactory::EnsureProperties());

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Transition).Get(),
                &spInnerFactory));

        IFC(spInnerFactory->CreateInstance(
                static_cast<IInspectable*>(static_cast<xaml_animation::ITransition*>(this)),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

    HRESULT
    NavigationThemeTransition::OnPropertyChanged(
        _In_ xaml::IDependencyPropertyChangedEventArgs* /* pArgs */)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    NavigationThemeTransition::get_DefaultNavigationTransitionInfo(
        _Outptr_ xaml_animation::INavigationTransitionInfo** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> defaultNavigationTransitionInfoProperty;
        wrl::ComPtr<xaml_animation::INavigationThemeTransitionStatics> nttStatics;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> nti;
        wrl::ComPtr<NavigationThemeTransition> ntt (this);
        wrl::ComPtr<xaml::IDependencyObject> nttAsDO;

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationThemeTransition).Get(), &nttStatics));

        IFC(nttStatics->get_DefaultNavigationTransitionInfoProperty(&defaultNavigationTransitionInfoProperty));
        IFC(ntt.As(&nttAsDO))
        IFC(nttAsDO->GetValue(defaultNavigationTransitionInfoProperty.Get(), &nti));

        IFC(nti.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    NavigationThemeTransition::put_DefaultNavigationTransitionInfo(
        _In_ xaml_animation::INavigationTransitionInfo* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> defaultNavigationTransitionInfoProperty;
        wrl::ComPtr<xaml_animation::INavigationThemeTransitionStatics> nttStatics;
        wrl::ComPtr<NavigationThemeTransition> ntt (this);
        wrl::ComPtr<xaml::IDependencyObject> nttAsDO;

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationThemeTransition).Get(), &nttStatics));

        IFC(nttStatics->get_DefaultNavigationTransitionInfoProperty(&defaultNavigationTransitionInfoProperty));
        IFC(ntt.As(&nttAsDO))

        IFC(nttAsDO->SetValue(defaultNavigationTransitionInfoProperty.Get(), value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    NavigationThemeTransition::ParticipatesInTransition(
        _In_ xaml::IUIElement* /*element*/,
        _In_ xaml::TransitionTrigger transitionTrigger,
        _Out_ BOOLEAN* returnValue)
    {
        HRESULT hr = S_OK;

        *returnValue = ((transitionTrigger == xaml::TransitionTrigger_Load) || (transitionTrigger == xaml::TransitionTrigger_Unload));

        RRETURN(hr);
    }

    IFACEMETHODIMP
    NavigationThemeTransition::CreateStoryboard(
        _In_ xaml::IUIElement* element,
        _In_ wf::Rect /*start*/,
        _In_ wf::Rect /*destination*/,
        _In_ xaml::TransitionTrigger transitionTrigger,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
        _Out_ xaml::TransitionParent* /*parentForTransition*/)
    {
        HRESULT hr = S_OK;
        BOOLEAN shouldRun = FALSE;
        BOOLEAN isBackNavigation = FALSE;
        BOOLEAN isInitialPage = FALSE;

        wrl::ComPtr<xaml::IUIElement> spElement;
        wrl::ComPtr<xaml_controls::IPage> spPage;
        wrl::ComPtr<xaml_controls::IFrame> spFrame;
        wrl::ComPtr<xaml_controls::IFramePrivate> spFramePrivate;
        wrl::ComPtr<xaml_controls::IContentControl> spFrameAsContentControl;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spDefinition;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfoPrivate> spDefinitionPrivate;
        wrl::ComPtr<xaml_animation::IStoryboard> spStoryboard;

        shouldRun = ((transitionTrigger == xaml::TransitionTrigger_Load) || (transitionTrigger == xaml::TransitionTrigger_Unload));

        if (!shouldRun)
        {
            goto Cleanup;
        }

        spElement = wrl::ComPtr<xaml::IUIElement>(element);


        IGNOREHR(spElement.As(&spPage));

        // NTT could be placed on any element: only continue if this is a page.
        if(!spPage)
        {
            goto Cleanup;
        }

        // Get the NavigationTransitionInfo (from Frame.Navigate) from the Frame.  If we aren't in
        // a frame then there is no navigation, so bail.
        IFC(spPage->get_Frame(&spFrame));
        if (!spFrame)
        {
            goto Cleanup;
        }

        IFC(spFrame.As(&spFramePrivate));
        IFC(spFramePrivate->GetNavigationTransitionInfoOverride(&spDefinition, &isBackNavigation, &isInitialPage));

        // Check to see if the override was set.
        if(spDefinition)
        {
            // An override was passed in, use this definition.
            IFC(spDefinition.As(&spDefinitionPrivate));
        }
        else
        {
            // We will always run an animation unless it is the forward navigation to the first page.
            shouldRun = (!isInitialPage || isBackNavigation);

            // Check new page for its default.
            wrl::ComPtr<xaml_controls::IPage> spCurrentPage;
            wrl::ComPtr<xaml::IUIElement> spCurrentPageAsUIElement;
            wrl::ComPtr<xaml_animation::INavigationThemeTransition> spNavigationThemeTransition;

            // Get the current page (new page) from the Frame.
            IFC(spFrame.As(&spFrameAsContentControl));
            IFC(spFrameAsContentControl->get_Content(&spCurrentPage));

            // Get the Transitions collection from the Page.
            if (spCurrentPage != nullptr)
            {
                wrl::ComPtr<wfc::IVector<xaml_animation::Transition*>> spTransitionsCurrentPage;
                IFC(spCurrentPage.As(&spCurrentPageAsUIElement));
                IFC(spCurrentPageAsUIElement->get_Transitions(&spTransitionsCurrentPage));
                IFC(GetNavigationThemeTransitionFromCollection(spTransitionsCurrentPage.Get(), &spNavigationThemeTransition));
            }

            if(spNavigationThemeTransition == nullptr)
            {
                // App did not override Page.Transitions, so get the
                // Frame.ContentTransitions.
                wrl::ComPtr<wfc::IVector<xaml_animation::Transition*>> spTransitionsCurrentPage;
                spFrameAsContentControl->get_ContentTransitions(&spTransitionsCurrentPage);
                IFC(GetNavigationThemeTransitionFromCollection(spTransitionsCurrentPage.Get(), &spNavigationThemeTransition));
            }

            if (spNavigationThemeTransition != nullptr)
            {
                // We have Navigation Theme Transition so we will run the animation.
                shouldRun = true;

                // See if there is a specific animation being requested.
                IFC(spNavigationThemeTransition->get_DefaultNavigationTransitionInfo(&spDefinition));
                if (spDefinition)
                {
                    IFC(spDefinition.As(&spDefinitionPrivate));

                    // Set this override definition as the override on the Frame.
                    IFC(spFramePrivate->SetNavigationTransitionInfoOverride(spDefinition.Get()));
                }
            }
        }

        if (shouldRun)
        {
            // If the definition has not been set yet, use the default definition.
            if (!spDefinitionPrivate)
            {
                IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EntranceNavigationTransitionInfo).Get(), &spDefinition));
                IFC(spDefinition.As(&spDefinitionPrivate));
            }

            IFCPTR(spDefinitionPrivate.Get());

            if (isBackNavigation)
            {
                if (transitionTrigger == xaml::TransitionTrigger_Load)
                {
                    IFC(spDefinitionPrivate->CreateStoryboards(element, xaml_animation::NavigationTrigger_BackNavigatingTo, storyboards));
                }
                else if (transitionTrigger == xaml::TransitionTrigger_Unload)
                {
                    IFC(spDefinitionPrivate->CreateStoryboards(element, xaml_animation::NavigationTrigger_BackNavigatingAway, storyboards));
                }
            }
            else
            {
                if (transitionTrigger == xaml::TransitionTrigger_Load)
                {
                    IFC(spDefinitionPrivate->CreateStoryboards(element, xaml_animation::NavigationTrigger_NavigatingTo, storyboards));
                }
                else if (transitionTrigger == xaml::TransitionTrigger_Unload)
                {
                    IFC(spDefinitionPrivate->CreateStoryboards(element, xaml_animation::NavigationTrigger_NavigatingAway, storyboards));
                }
            }
        }

        IFC(CommonNavigationTransitionInfoFactory::ClearStaggerElements());
        IFC(ContinuumNavigationTransitionInfoFactory::ClearExitElements());
        IFC(ContinuumNavigationTransitionInfoFactory::ClearEntranceElements());
        IFC(EntranceNavigationTransitionInfoFactory::ClearTargetElements());

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT NavigationThemeTransition::GetNavigationThemeTransitionFromCollection(_In_ wfc::IVector<xaml_animation::Transition*>* transitions, _Out_ xaml_animation::INavigationThemeTransition** themeTransition)
    {
        *themeTransition = nullptr;

        // Get the size of that collection (if we have one)
        UINT32 numTransitions = 0;
        if (transitions != nullptr)
        {
            IFC_RETURN(transitions->get_Size(&numTransitions));
        }

        // Iterate through the collection to find an NTT.
        for (UINT32 k = numTransitions; *themeTransition == nullptr && k > 0 ; k--)
        {
            wrl::ComPtr<xaml_animation::ITransition> spTransition;

            IFC_RETURN(transitions->GetAt(k - 1, &spTransition));
            if (spTransition)
            {
                wrl::ComPtr<xaml_animation::INavigationThemeTransition> spNavigationTransition;
                IGNOREHR(spTransition.As(&spNavigationTransition));
                *themeTransition = spNavigationTransition.Detach();
            }
        }
        return S_OK;
    }

    // NavigationThemeTransitionFactory

    wrl::ComPtr<xaml::IDependencyProperty> NavigationThemeTransitionFactory::s_spDefaultNavigationTransitionInfoProperty;

    _Check_return_
    HRESULT
    NavigationThemeTransitionFactory::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        IFC(EnsureProperties());

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigationThemeTransitionFactory::EnsureProperties()
    {
        HRESULT hr = S_OK;

        IFC(InitializeDefaultNavigationTransitionInfoProperty());

    Cleanup:
        RRETURN(hr);
    }

    void
    NavigationThemeTransitionFactory::ClearProperties()
    {
        s_spDefaultNavigationTransitionInfoProperty.Reset();
    }

    _Check_return_
    HRESULT
    NavigationThemeTransitionFactory::InitializeDefaultNavigationTransitionInfoProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spDefaultNavigationTransitionInfoProperty)
        {
            IFC(Private::InitializeDependencyProperty(
                L"DefaultNavigationTransitionInfo",
                L"Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationThemeTransition,
                FALSE /* isAttached */,
                nullptr /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(NavigationThemeTransitionFactory::OnPropertyChanged).Get(),
                &s_spDefaultNavigationTransitionInfoProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    NavigationThemeTransitionFactory::ActivateInstance(
        _Outptr_ IInspectable** ppInspectable)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<NavigationThemeTransition> ntt;

        ARG_VALIDRETURNPOINTER(ppInspectable);

        IFC(wrl::MakeAndInitialize<NavigationThemeTransition>(&ntt));

        IFC(ntt.CopyTo(ppInspectable));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigationThemeTransitionFactory::OnPropertyChanged(
        _In_ xaml::IDependencyObject* pSender,
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<INavigationThemeTransition> ntt;

        IFC(wrl::ComPtr<xaml::IDependencyObject>(pSender).As<INavigationThemeTransition>(&ntt));

        IFC(static_cast<NavigationThemeTransition*>(ntt.Get())->OnPropertyChanged(pArgs));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    NavigationThemeTransitionFactory::get_DefaultNavigationTransitionInfoProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spDefaultNavigationTransitionInfoProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    // CommonNavigationTransitionInfo

    CommonNavigationTransitionInfo::CommonNavigationTransitionInfo()
    {

    }

    CommonNavigationTransitionInfo::~CommonNavigationTransitionInfo()
    {

    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfo::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        // Ensure the properties for this factory have been properly initialized.
        IFC(CommonNavigationTransitionInfoFactory::EnsureProperties());

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_CommonNavigationTransitionInfo).Get(),
                &m_spTNTIStatics));

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
                &spInnerFactory));

        IFC(spInnerFactory->CreateInstance(
                static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfo::OnPropertyChanged(
        _In_ xaml::IDependencyPropertyChangedEventArgs* /* pArgs */)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfo::get_IsStaggeringEnabled(
        _Out_ BOOLEAN* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> isStaggeringEnabledProperty;
        wrl::ComPtr<CommonNavigationTransitionInfo> tnti (this);
        wrl::ComPtr<xaml::IDependencyObject> tntiAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);

        IFC(m_spTNTIStatics->get_IsStaggeringEnabledProperty(&isStaggeringEnabledProperty));
        IFC(tnti.As(&tntiAsDO))
        IFC(tntiAsDO->GetValue(isStaggeringEnabledProperty.Get(), &boxedValue));

        IFC(boxedValue->GetBoolean(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfo::put_IsStaggeringEnabled(
        _In_ BOOLEAN value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> isStaggeringEnabledProperty;
        wrl::ComPtr<CommonNavigationTransitionInfo> tnti (this);
        wrl::ComPtr<xaml::IDependencyObject> tntiAsDO;
        RoVariant boxedValue;

        IFC(m_spTNTIStatics->get_IsStaggeringEnabledProperty(&isStaggeringEnabledProperty));
        IFC(tnti.As(&tntiAsDO));

        IFC(Private::ValueBoxer::CreateBoolean(value, &boxedValue));

        IFC(tntiAsDO->SetValue(isStaggeringEnabledProperty.Get(), boxedValue.Get()));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HString navState;
        BOOLEAN isStaggeringEnabled = FALSE;

        // The navigation state system saved in strings.
        // This navigation state for TNTI only needs to save
        // the StaggeringEnabled property; as such, we are
        // saving "1" for TRUE and "0" for FALSE.

        ARG_VALIDRETURNPOINTER(string);
        IFC(navState.Set(STR_LEN_PAIR(L"0")));

        IFC(get_IsStaggeringEnabled(&isStaggeringEnabled));

        if(isStaggeringEnabled)
        {
            IFC(navState.Set(STR_LEN_PAIR(L"1")));
        }

        IFC(navState.CopyTo(string));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING string)
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HStringReference navStateEnabled (STR_LEN_PAIR(L"1"));
        INT32 ordinal = -1;
        BOOLEAN isStaggeringEnabled = FALSE;

        IFC(WindowsCompareStringOrdinal(navStateEnabled.Get(), string, &ordinal));

        if(ordinal == 0)
        {
            isStaggeringEnabled = TRUE;
        }

        IFC(put_IsStaggeringEnabled(isStaggeringEnabled));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfo::CreateStoryboards(
        _In_ xaml::IUIElement* element,
        _In_ xaml_animation::NavigationTrigger trigger,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
    {
        HRESULT hr = S_OK;

        BOOLEAN isStaggeringEnabled = FALSE;

        wrl_wrappers::HString propertyNameRotation;
        wrl_wrappers::HString propertyNameCenterX;
        wrl_wrappers::HString propertyNameCenterZ;
        wrl_wrappers::HString propertyNameOpacity;
        wrl_wrappers::HString propertyNameIsHitTestVisible;

        wrl::ComPtr<xaml::IUIElement> spElement;
        wrl::ComPtr<xaml_animation::IStoryboard> spStoryboard;
        std::vector<wrl::ComPtr<xaml::IUIElement>> visibleTargets;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasing;
        std::vector<DoubleAnimationDescription> animations;
        std::vector<std::vector<DoubleKeyFrameDescription>*> allKeyFrames;
        std::vector<xaml_animation::IEasingFunctionBase*> allEasingsNoRef;

        ARG_VALIDRETURNPOINTER(storyboards);

        // Create storyboard.
        spElement = wrl::ComPtr<xaml::IUIElement>(element);

        // Instantiate property string.
        IFC(propertyNameRotation.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.RotationY)")));
        IFC(propertyNameCenterX.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationX)")));
        IFC(propertyNameCenterZ.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationZ)")));
        IFC(propertyNameOpacity.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));
        IFC(propertyNameIsHitTestVisible.Set(STR_LEN_PAIR(L"UIElement.IsHitTestVisible")));

        IFC(get_IsStaggeringEnabled(&isStaggeringEnabled));

        // Do the work for getting visible targets, as we don't
        // want to unnecessarily repeat the code.
        if(isStaggeringEnabled)
        {
            wrl::ComPtr<xaml::IFrameworkElement> spElementAsFrameworkElement;
            std::vector<wrl::WeakRef> targets;

            // Get the Page's Rect.
            DOUBLE pageWidth = 0;
            DOUBLE pageHeight = 0;
            wf::Rect pageRect = {};

            IFC(spElement.As(&spElementAsFrameworkElement));

            IFC(spElementAsFrameworkElement->get_ActualWidth(&pageWidth));
            IFC(spElementAsFrameworkElement->get_ActualHeight(&pageHeight));

            pageRect.X = pageRect.Y = 0;
            pageRect.Width = static_cast<FLOAT>(pageWidth);
            pageRect.Height = static_cast<FLOAT>(pageHeight);

            // Get the stagger targets.
            IFC(CommonNavigationTransitionInfoFactory::GetStaggerElements(&targets));

            // Iterate through stagger targets.
            for(auto &it : targets)
            {
                wrl::ComPtr<xaml::IUIElement> spTarget;

                IFC(it.As(&spTarget));

                if(spTarget)
                {
                    BOOLEAN pageIsAncestor = FALSE;
                    wrl::ComPtr<xaml_media::IGeneralTransform> spTargetTransform;
                    wrl::ComPtr<xaml::IFrameworkElement> spTargetAsFrameworkElement;

                    // Get the target's Rect.
                    DOUBLE targetWidth = 0;
                    DOUBLE targetHeight = 0;
                    wf::Rect targetRect = {};

                    wf::Rect targetTransformRect = {};

                    IFC(spTarget.As(&spTargetAsFrameworkElement));

                    IFC(spTargetAsFrameworkElement->get_ActualWidth(&targetWidth));
                    IFC(spTargetAsFrameworkElement->get_ActualHeight(&targetHeight));

                    targetRect.X = targetRect.Y = 0;
                    targetRect.Width = static_cast<FLOAT>(targetWidth);
                    targetRect.Height = static_cast<FLOAT>(targetHeight);

                    // Transform the target to the Page and transform its Rect.
                    IFC(spTarget->TransformToVisual(spElement.Get(), &spTargetTransform));
                    IFC(spTargetTransform->TransformBounds(targetRect, &targetTransformRect));

                    // Check for intersection.
                    BOOLEAN doIntersect =
                        !(pageRect.Width < 0 || targetTransformRect.Width < 0) &&
                        (targetTransformRect.X <= pageRect.X + pageRect.Width) &&
                        (targetTransformRect.X + targetTransformRect.Width >= pageRect.X) &&
                        (targetTransformRect.Y <= pageRect.Y + pageRect.Height) &&
                        (targetTransformRect.Y + targetTransformRect.Height >= pageRect.Y);

                    IFC(NavigateTransitionHelper::IsAncestor(spElementAsFrameworkElement.Get(), spTargetAsFrameworkElement.Get(), &pageIsAncestor));

                    if(doIntersect && pageIsAncestor)
                    {
                        // Add to visible targets.
                        visibleTargets.push_back(spTarget);
                    }
                }
            }
        }

        // size() never throws.
        if (visibleTargets.size() <= 0)
        {
            // If there are no targets, then this page should turnstile.
            isStaggeringEnabled = false;
        }

        if(trigger == xaml_animation::NavigationTrigger_NavigatingTo)
        {
            if(isStaggeringEnabled)
            {
                // First add opacity change for page.
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacityPage = new std::vector<DoubleKeyFrameDescription>();
                allKeyFrames.push_back(keyFramesOpacityPage);

                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::FULL_OPACITY));

                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacityPage));

                // Iterate through visible targets and create animations.
                size_t numVisibleTargets = 0;

                // size() never throws.
                numVisibleTargets = visibleTargets.size();

                for(size_t k = 0; k < numVisibleTargets; k++)
                {
                    wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasingInner;
                    IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseOut, &spEasingInner));
                    allEasingsNoRef.push_back(spEasingInner.Get());
                    spEasingInner.Get()->AddRef();

                    std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                    wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                    allKeyFrames.push_back(keyFramesRotation);
                    allKeyFrames.push_back(keyFramesCenterX);
                    allKeyFrames.push_back(keyFramesCenterZ);
                    allKeyFrames.push_back(keyFramesOpacity);

                    DOUBLE offset = (DOUBLE)k * NavigateTransitionHelper::TURNSTILE_FEATHER_OFFSET_IN;

                    // Set rotation key frames.
                    keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, -1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, -1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_END_TIME) + (INT64)offset, NavigateTransitionHelper::NO_OFFSET, spEasingInner.Get()));

                    // Set center key frames.
                    keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_X));
                    keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                    // Set opacity key frames.
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::NO_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset + 1, NavigateTransitionHelper::FULL_OPACITY));

                    // Clobber this, as we have to clobber values anyway.
                    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                    IFC(visibleTargets[k]->put_Projection(spElementPlaneProjection.Get()));

                    // Add animations
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameRotation, keyFramesRotation));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterX, keyFramesCenterX));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterZ, keyFramesCenterZ));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameOpacity, keyFramesOpacity));
                }
            }
            else
            {
                IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseOut, &spEasing));

                std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                allKeyFrames.push_back(keyFramesRotation);
                allKeyFrames.push_back(keyFramesCenterX);
                allKeyFrames.push_back(keyFramesCenterZ);
                allKeyFrames.push_back(keyFramesOpacity);

                // Set rotation key frames.
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,-1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME,-1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_END_TIME,NavigateTransitionHelper::NO_OFFSET, spEasing.Get()));

                // Set center key frames.
                keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_X));
                keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                // Set opacity key frames.
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME,NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME+1,NavigateTransitionHelper::FULL_OPACITY));

                // Add animations
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameRotation, keyFramesRotation));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterX, keyFramesCenterX));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterZ, keyFramesCenterZ));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacity));
            }
        }
        else if(trigger == xaml_animation::NavigationTrigger_NavigatingAway)
        {
            if (isStaggeringEnabled)
            {
                // First add opacity change for page.
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacityPage = new std::vector<DoubleKeyFrameDescription>();
                allKeyFrames.push_back(keyFramesOpacityPage);

                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::NO_OPACITY));

                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacityPage));

                // Iterate through visible targets and create animations.
                size_t numVisibleTargets = 0;

                // size() never throws.
                numVisibleTargets = visibleTargets.size();

                for (size_t k = 0; k < numVisibleTargets; k++)
                {
                    wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasingInner;
                    IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseIn, &spEasingInner));
                    allEasingsNoRef.push_back(spEasingInner.Get());
                    spEasingInner.Get()->AddRef();

                    std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                    wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                    allKeyFrames.push_back(keyFramesRotation);
                    allKeyFrames.push_back(keyFramesCenterX);
                    allKeyFrames.push_back(keyFramesCenterZ);
                    allKeyFrames.push_back(keyFramesOpacity);

                    DOUBLE offset = (DOUBLE)k * NavigateTransitionHelper::TURNSTILE_FEATHER_OFFSET_OUT;

                    // Set rotation key frames.
                    keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME + (INT64)offset, NavigateTransitionHelper::NO_OFFSET));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT, spEasing.Get()));

                    // Set center key frames.
                    keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_X));
                    keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                    // Set opacity key frames.
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME + (INT64)offset, NavigateTransitionHelper::FULL_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::FULL_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset + 1, NavigateTransitionHelper::NO_OPACITY));

                    // Clobber this, as we have to clobber values anyway.
                    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                    IFC(visibleTargets[k]->put_Projection(spElementPlaneProjection.Get()));

                    // Add animations
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameRotation, keyFramesRotation));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterX, keyFramesCenterX));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterZ, keyFramesCenterZ));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameOpacity, keyFramesOpacity));
                }
            }
            else
            {
                IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseIn, &spEasing));

                std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                allKeyFrames.push_back(keyFramesRotation);
                allKeyFrames.push_back(keyFramesCenterX);
                allKeyFrames.push_back(keyFramesCenterZ);
                allKeyFrames.push_back(keyFramesOpacity);

                // Set rotation key frames.
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OFFSET));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT, spEasing.Get()));

                // Set center key frames.
                keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_X));
                keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                // Set opacity key frames.
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::NO_OPACITY));

                // Add animations
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameRotation, keyFramesRotation));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterX, keyFramesCenterX));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterZ, keyFramesCenterZ));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacity));
            }
        }
        else if(trigger == NavigationTrigger_BackNavigatingTo)
        {
            if (isStaggeringEnabled)
            {
                // First add opacity change for page.
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacityPage = new std::vector<DoubleKeyFrameDescription>();
                allKeyFrames.push_back(keyFramesOpacityPage);

                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::FULL_OPACITY));

                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacityPage));

                // Iterate through visible targets and create animations.
                size_t numVisibleTargets = 0;

                // size() never throws.
                numVisibleTargets = visibleTargets.size();

                for (size_t k = 0; k < numVisibleTargets; k++)
                {
                    wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasingInner;
                    IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseOut, &spEasingInner));
                    allEasingsNoRef.push_back(spEasingInner.Get());
                    spEasingInner.Get()->AddRef();

                    std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                    wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                    allKeyFrames.push_back(keyFramesRotation);
                    allKeyFrames.push_back(keyFramesCenterX);
                    allKeyFrames.push_back(keyFramesCenterZ);
                    allKeyFrames.push_back(keyFramesOpacity);

                    DOUBLE offset = (DOUBLE)k * NavigateTransitionHelper::TURNSTILE_FEATHER_OFFSET_IN;

                    // Set rotation key frames.
                    keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_END_TIME) + (INT64)offset, NavigateTransitionHelper::NO_OFFSET, spEasing.Get()));

                    // Set center key frames.
                    keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_X));
                    keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                    // Set opacity key frames.
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::NO_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_IN * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset + 1, NavigateTransitionHelper::FULL_OPACITY));

                    // Clobber this, as we have to clobber values anyway.
                    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                    IFC(visibleTargets[k]->put_Projection(spElementPlaneProjection.Get()));

                    // Add animations
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameRotation, keyFramesRotation));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterX, keyFramesCenterX));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterZ, keyFramesCenterZ));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameOpacity, keyFramesOpacity));
                }
            }
            else
            {
                IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseOut, &spEasing));

                std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                allKeyFrames.push_back(keyFramesRotation);
                allKeyFrames.push_back(keyFramesCenterX);
                allKeyFrames.push_back(keyFramesCenterZ);
                allKeyFrames.push_back(keyFramesOpacity);

                // Set rotation key frames.
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::TURNSTILE_OFFSET_OUT));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_END_TIME, NavigateTransitionHelper::NO_OFFSET, spEasing.Get()));

                // Set center key frames.
                keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_X));
                keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                // Set opacity key frames.
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::NO_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::FULL_OPACITY));

                // Add animations
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameRotation, keyFramesRotation));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterX, keyFramesCenterX));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterZ, keyFramesCenterZ));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacity));
            }
        }
        else if(trigger == xaml_animation::NavigationTrigger_BackNavigatingAway)
        {
            if(isStaggeringEnabled)
            {
                // First add opacity change for page.
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacityPage = new std::vector<DoubleKeyFrameDescription>();
                allKeyFrames.push_back(keyFramesOpacityPage);

                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacityPage->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::NO_OPACITY));

                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacityPage));

                // Iterate through visible targets and create animations.
                size_t numVisibleTargets = 0;

                // size() never throws.
                numVisibleTargets = visibleTargets.size();

                for(size_t k = 0; k < numVisibleTargets; k++)
                {
                    wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasingInner;
                    IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseIn, &spEasingInner));
                    allEasingsNoRef.push_back(spEasingInner.Get());
                    spEasingInner.Get()->AddRef();

                    std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                    std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                    wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                    allKeyFrames.push_back(keyFramesRotation);
                    allKeyFrames.push_back(keyFramesCenterX);
                    allKeyFrames.push_back(keyFramesCenterZ);
                    allKeyFrames.push_back(keyFramesOpacity);

                    DOUBLE offset = (DOUBLE)k * NavigateTransitionHelper::TURNSTILE_FEATHER_OFFSET_OUT;

                    // Set rotation key frames.
                    keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME + (INT64)offset, NavigateTransitionHelper::NO_OFFSET));
                    keyFramesRotation->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, -1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN, spEasingInner.Get()));

                    // Set center key frames.
                    keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_X));
                    keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                    // Set opacity key frames.
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME + (INT64)offset, NavigateTransitionHelper::FULL_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset, NavigateTransitionHelper::FULL_OPACITY));
                    keyFramesOpacity->push_back(DoubleKeyFrameDescription((INT64)(NavigateTransitionHelper::TURNSTILE_FEATHER_SCALE_OUT * NavigateTransitionHelper::TURNSTILE_MID_TIME) + (INT64)offset + 1, NavigateTransitionHelper::NO_OPACITY));

                    // Clobber this, as we have to clobber values anyway.
                    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                    IFC(visibleTargets[k]->put_Projection(spElementPlaneProjection.Get()));

                    // Add animations
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameRotation, keyFramesRotation));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterX, keyFramesCenterX));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameCenterZ, keyFramesCenterZ));
                    animations.push_back(DoubleAnimationDescription(visibleTargets[k].Get(), &propertyNameOpacity, keyFramesOpacity));
                }
            }
            else
            {
                IFC(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::TURNSTILE_EASE, xaml_animation::EasingMode_EaseIn, &spEasing));

                std::vector<DoubleKeyFrameDescription>* keyFramesRotation = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterX = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesCenterZ = new std::vector<DoubleKeyFrameDescription>();
                std::vector<DoubleKeyFrameDescription>* keyFramesOpacity = new std::vector<DoubleKeyFrameDescription>();

                allKeyFrames.push_back(keyFramesRotation);
                allKeyFrames.push_back(keyFramesCenterX);
                allKeyFrames.push_back(keyFramesCenterZ);
                allKeyFrames.push_back(keyFramesOpacity);

                // Set rotation key frames.
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::NO_OFFSET));
                keyFramesRotation->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME,-1 * NavigateTransitionHelper::TURNSTILE_OFFSET_IN, spEasing.Get()));

                // Set center key frames.
                keyFramesCenterX->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_X));
                keyFramesCenterZ->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME,NavigateTransitionHelper::TURNSTILE_AXIS_Z));

                // Set opacity key frames.
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_START_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME, NavigateTransitionHelper::FULL_OPACITY));
                keyFramesOpacity->push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TURNSTILE_MID_TIME + 1, NavigateTransitionHelper::NO_OPACITY));

                // Add animations
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameRotation, keyFramesRotation));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterX, keyFramesCenterX));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameCenterZ, keyFramesCenterZ));
                animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, keyFramesOpacity));
            }
        }

        // Create storyboard.
        IFC(NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(&animations, &spStoryboard));

        // Ensure sure that the animated pages are not hit test visible.
        {
            wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
            wrl::ComPtr<ITimeline> spTimeline;
            wrl::ComPtr<IObjectAnimationUsingKeyFrames> spAnimation;
            std::vector<ObjectKeyFrameDescription> keyFramesIsHitTestVisible;

            // Set isHitTestVisible key frames.
            keyFramesIsHitTestVisible.push_back(ObjectKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, FALSE));

            // Add object animations to storyboard.
            IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));
            IFC(NavigateTransitionHelper::GetObjectAnimation(spStoryboardStatics, ObjectAnimationDescription(spElement.Get(), &propertyNameIsHitTestVisible, &keyFramesIsHitTestVisible), &spAnimation));

            IFC(spAnimation.As(&spTimeline));

            IFC(NavigateTransitionHelper::AddTimelineToStoryboard(spTimeline.Get(), spStoryboard.Get()));
        }

        IFC(storyboards->Append(spStoryboard.Get()));

Cleanup:
        // Delete all of the std::vector<DoubleKeyFrameDescription> that were created as pointers.
        for(auto &it : allKeyFrames)
        {
            SAFE_DELETE(it);
        }

        // Release all of the IEasingFunctionBase that were AddRefed to keep them alive for commit.
        for (auto &it : allEasingsNoRef)
        {
            xaml_animation::IEasingFunctionBase* item = it;
            item->Release();
        }

        RRETURN(hr);
    }

    // CommonNavigationTransitionInfoFactory

    wrl::ComPtr<xaml::IDependencyProperty> CommonNavigationTransitionInfoFactory::s_spIsStaggeringEnabledProperty;
    wrl::ComPtr<xaml::IDependencyProperty> CommonNavigationTransitionInfoFactory::s_spIsStaggerElementProperty;
    std::vector<wrl::WeakRef> CommonNavigationTransitionInfoFactory::s_spStaggerElements;

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        IFC(EnsureProperties());

    Cleanup:
        RRETURN(hr);
    }

    void
    CommonNavigationTransitionInfoFactory::ClearProperties()
    {
        s_spIsStaggeringEnabledProperty.Reset();
        s_spIsStaggerElementProperty.Reset();
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::EnsureProperties()
    {
        HRESULT hr = S_OK;

        IFC(InitializeIsStaggeringEnabledProperty());
        IFC(InitializeIsStaggerElementProperty());

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::InitializeIsStaggeringEnabledProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spIsStaggeringEnabledProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC(Private::InitializeDependencyProperty(
                L"IsStaggeringEnabled",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_CommonNavigationTransitionInfo,
                FALSE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(CommonNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spIsStaggeringEnabledProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::InitializeIsStaggerElementProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spIsStaggerElementProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC(Private::InitializeDependencyProperty(
                L"IsStaggerElement",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_CommonNavigationTransitionInfo,
                TRUE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(CommonNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spIsStaggerElementProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfoFactory::ActivateInstance(
        _Outptr_ IInspectable** ppInspectable)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<ICommonNavigationTransitionInfo> tnti;

        ARG_VALIDRETURNPOINTER(ppInspectable);

        IFC(wrl::MakeAndInitialize<CommonNavigationTransitionInfo>(&tnti));

        IFC(tnti.CopyTo(ppInspectable));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::OnPropertyChanged(
        _In_ xaml::IDependencyObject* pSender,
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IDependencyObject> spDO(pSender);
        wrl::ComPtr<xaml::IDependencyPropertyChangedEventArgs> spArgs(pArgs);
        wrl::ComPtr<xaml::IDependencyProperty> spProperty;
        wrl::ComPtr<IInspectable> spNewValueInsp;

        ARG_VALIDRETURNPOINTER(spDO.Get());
        ARG_VALIDRETURNPOINTER(spArgs.Get());

        IFC(spArgs->get_Property(&spProperty));
        IFC(spArgs->get_NewValue(&spNewValueInsp));

        if(spProperty.Get() == s_spIsStaggeringEnabledProperty.Get())
        {
            // This is just a DP: bubble up to DO.
            wrl::ComPtr<ICommonNavigationTransitionInfo> tnti;
            IFC(wrl::ComPtr<xaml::IDependencyObject>(pSender).As<ICommonNavigationTransitionInfo>(&tnti));
            IFC(static_cast<CommonNavigationTransitionInfo*>(tnti.Get())->OnPropertyChanged(pArgs));
        }
        else if(spProperty.Get() == s_spIsStaggerElementProperty)
        {
            // This is an attached property, bubble up to page.
            wrl::ComPtr<wf::IPropertyValue> spNewValue;
            IFC(spNewValueInsp.As(&spNewValue));

            BOOLEAN isStaggerElement = FALSE;
            IFC(spNewValue->GetBoolean(&isStaggerElement));

            // Erase the value in the vector.
            for (auto it = s_spStaggerElements.begin(); it != s_spStaggerElements.end(); ++it)
            {
                wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

                IFC((*it).As(&spItAsDO));

                if (spItAsDO.Get() == spDO.Get())
                {
                    s_spStaggerElements.erase(it);
                    break;
                }
            }

            if(isStaggerElement)
            {
                wrl::WeakRef spElement;
                IFC(spDO.AsWeak(&spElement));

                s_spStaggerElements.push_back(spElement);
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfoFactory::get_IsStaggeringEnabledProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spIsStaggeringEnabledProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfoFactory::get_IsStaggerElementProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spIsStaggerElementProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfoFactory::GetIsStaggerElement(
        _In_ xaml::IUIElement* element,
        _Out_ BOOLEAN* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);
        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(spElementAsDO->GetValue(s_spIsStaggerElementProperty.Get(), &boxedValue));
        IFC(boxedValue->GetBoolean(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    CommonNavigationTransitionInfoFactory::SetIsStaggerElement(
        _In_ xaml::IUIElement* element,
        _In_ BOOLEAN value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(Private::ValueBoxer::CreateBoolean(value, &boxedValue));
        IFC(spElementAsDO->SetValue(s_spIsStaggerElementProperty.Get(), boxedValue.Get()));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::GetStaggerElements(
        _Out_ std::vector<wrl::WeakRef>* elements)
    {
        ARG_VALIDRETURNPOINTER(elements);

        *elements = s_spStaggerElements;

        RRETURN(S_OK);
    }

    _Check_return_
    HRESULT
    CommonNavigationTransitionInfoFactory::ClearStaggerElements()
    {
        HRESULT hr = S_OK;

        auto it = s_spStaggerElements.begin();

        while (it != s_spStaggerElements.end())
        {
            wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

            IFC((*it).As(&spItAsDO));

            if (!spItAsDO)
            {
                it = s_spStaggerElements.erase(it);
            }
            else
            {
                ++it;
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    // SlideNavigationTransitionInfoFactory

    wrl::ComPtr<xaml::IDependencyProperty> SlideNavigationTransitionInfoFactory::s_effectProperty;

    _Check_return_ HRESULT SlideNavigationTransitionInfoFactory::RuntimeClassInitialize()
    {
        IFC_RETURN(EnsureProperties());
        return S_OK;
    }

    IFACEMETHODIMP SlideNavigationTransitionInfoFactory::ActivateInstance(_Outptr_ IInspectable** ppInspectable)
    {
        ARG_VALIDRETURNPOINTER(ppInspectable);

        wrl::ComPtr<ISlideNavigationTransitionInfo> transitionInfo;
        IFC_RETURN(wrl::MakeAndInitialize<SlideNavigationTransitionInfo>(&transitionInfo));
        IFC_RETURN(transitionInfo.CopyTo(ppInspectable));
        return S_OK;
    }

    void SlideNavigationTransitionInfoFactory::ClearProperties()
    {
        s_effectProperty.Reset();
    }

    _Check_return_ HRESULT SlideNavigationTransitionInfoFactory::EnsureProperties()
    {
        if (!s_effectProperty)
        {
            wrl::ComPtr<IInspectable> defaultValue;
            IFC_RETURN(Private::ValueBoxer::CreateReference<SlideNavigationTransitionEffect>(SlideNavigationTransitionEffect_FromBottom, &defaultValue))
            IFC_RETURN(Private::InitializeDependencyProperty(
                L"Effect",
                L"Microsoft.UI.Xaml.Media.Animation.SlideNavigationTransitionEffect",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SlideNavigationTransitionInfo,
                FALSE /* isAttached */,
                defaultValue.Get(),
                nullptr, /* onPropertyChanged */
                &s_effectProperty));
        }

        return S_OK;
    }

    IFACEMETHODIMP SlideNavigationTransitionInfoFactory::get_EffectProperty(_Outptr_ xaml::IDependencyProperty** value)
    {
        ARG_VALIDRETURNPOINTER(value);

        IFC_RETURN(s_effectProperty.CopyTo(value));
        return S_OK;
    }

    // SlideNavigationTransitionInfo

    SlideNavigationTransitionInfo::SlideNavigationTransitionInfo()
    {

    }

    SlideNavigationTransitionInfo::~SlideNavigationTransitionInfo()
    {

    }

    _Check_return_
    HRESULT
    SlideNavigationTransitionInfo::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(SlideNavigationTransitionInfoFactory::EnsureProperties());

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
                &spInnerFactory));

        IFC(spInnerFactory->CreateInstance(
                static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    SlideNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HStringReference navState (L"0");

        ARG_VALIDRETURNPOINTER(string);

        IFC(navState.CopyTo(string));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    SlideNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING /*string*/)
    {
        RRETURN(S_OK);
    }

    IFACEMETHODIMP
    SlideNavigationTransitionInfo::CreateStoryboards(
        _In_ xaml::IUIElement* element,
        _In_ xaml_animation::NavigationTrigger trigger,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
    {
        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml_animation::IStoryboard> spStoryboard;
        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasing;
        wrl::ComPtr<xaml::IDependencyObject> spAnimationTargetAsDO;

        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboard));
        IFC_RETURN(spStoryboard->get_Children(&spTimelines));

        IFC_RETURN(spElement.As(&spAnimationTargetAsDO));

        // Our translate may be in the x or y directions so let the right code path initialize it.
        wrl_wrappers::HString propertyNameTranslate;

        ARG_VALIDRETURNPOINTER(storyboards);

        // Create storyboard.
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spTranslateAnimation;
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;
        SlideNavigationTransitionEffect effect;
        IFC_RETURN(get_Effect(&effect));
        if (effect == SlideNavigationTransitionEffect_FromLeft || effect == SlideNavigationTransitionEffect_FromRight)
        {
            const DOUBLE translationExitOffset = 150;
            const DOUBLE translationEntranceOffset = -200;
            const wf::Point inControlPoint1 = { 0.1f, 0.9f };
            const wf::Point inControlPoint2 = { 0.2f, 1.0f };
            const wf::Point outControlPoint1 = { 0.7f, 0.0f };
            const wf::Point outControlPoint2 = { 1.0f, .5f };
            const UINT64 outDuration = 150;
            const UINT64 inDuration = 300;
            DOUBLE reverseTranslationFactor = effect == SlideNavigationTransitionEffect_FromLeft ? 1 : -1;

            IFC_RETURN(propertyNameTranslate.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX")));
            switch (trigger)
            {
            case xaml_animation::NavigationTrigger_NavigatingAway:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, outDuration));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateAnimation.Get(), translationExitOffset * reverseTranslationFactor, outDuration, outControlPoint1, outControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_NavigatingTo:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, outDuration));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), translationEntranceOffset * reverseTranslationFactor, outDuration));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateAnimation.Get(), 0.0, outDuration + inDuration, inControlPoint1, inControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingAway:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, outDuration));  // Hide it at end of translate animation

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateAnimation.Get(), translationEntranceOffset * reverseTranslationFactor, outDuration, outControlPoint1, outControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingTo:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, outDuration));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), translationExitOffset * reverseTranslationFactor, outDuration));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateAnimation.Get(), 0.0, outDuration + inDuration, inControlPoint1, inControlPoint2));
                break;
            }
        }
        else
        {
            IFC_RETURN(propertyNameTranslate.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY")));
            switch (trigger)
            {
            case xaml_animation::NavigationTrigger_NavigatingTo:
                IFC_RETURN(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::SLIDE_EASE, xaml_animation::EasingMode_EaseOut, &spEasing));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), NavigateTransitionHelper::SLIDE_OFFSET_IN, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), NavigateTransitionHelper::SLIDE_OFFSET_IN, NavigateTransitionHelper::SLIDE_MID_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterEasingKeyFrame(spTranslateAnimation.Get(), NavigateTransitionHelper::NO_OFFSET, NavigateTransitionHelper::SLIDE_END_TIME, spEasing.Get()));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME - 10));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME));
                break;
            case xaml_animation::NavigationTrigger_NavigatingAway:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME - 10));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingTo:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME - 10));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingAway:
                IFC_RETURN(NavigateTransitionHelper::GetExponentialEase(NavigateTransitionHelper::SLIDE_EASE, xaml_animation::EasingMode_EaseIn, &spEasing));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateAnimation.Get(), NavigateTransitionHelper::NO_OFFSET, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterEasingKeyFrame(spTranslateAnimation.Get(), NavigateTransitionHelper::SLIDE_OFFSET_OUT, NavigateTransitionHelper::SLIDE_END_TIME, spEasing.Get()));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_START_TIME));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::FULL_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME - 10));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), NavigateTransitionHelper::NO_OPACITY, NavigateTransitionHelper::SLIDE_MID_TIME));
            }
        }

        if (spTranslateAnimation != nullptr)
        {
            // Target property was initialized to the appropriate value depending on which codepath we took.
            wrl::ComPtr<ITimeline> spTranslateAnimationAsTimeline;
            IFC_RETURN(spTranslateAnimation.As(&spTranslateAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spTranslateAnimationAsTimeline.Get(), spAnimationTargetAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spTranslateAnimationAsTimeline.Get(), propertyNameTranslate.Get()));
            IFC_RETURN(spTimelines->Append(spTranslateAnimationAsTimeline.Get()));
        }

        if (spOpacityAnimation != nullptr)
        {
            wrl_wrappers::HString propertyNameOpacity;
            IFC_RETURN(propertyNameOpacity.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));
            wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;
            IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spAnimationTargetAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
            IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
        }

        // Ensure sure that the animated pages are not hit test visible while animating.
        NavigateTransitionHelper::RemoveHitTestVisbility(spStoryboardStatics.Get(), spTimelines.Get(), spAnimationTargetAsDO.Get());

        IFC_RETURN(storyboards->Append(spStoryboard.Get()));

        return S_OK;
    }

    _Check_return_ HRESULT SlideNavigationTransitionInfo::get_Effect(_Out_ SlideNavigationTransitionEffect* value)
    {
        wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty;
        wrl::ComPtr<xaml_animation::ISlideNavigationTransitionInfoStatics> statics;
        wrl::ComPtr<wf::IReference<xaml_animation::SlideNavigationTransitionEffect>> effectValue;
        wrl::ComPtr<SlideNavigationTransitionInfo> transitionInfo(this);
        wrl::ComPtr<xaml::IDependencyObject> dependencyObject;

        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SlideNavigationTransitionInfo).Get(), &statics));

        IFC_RETURN(statics->get_EffectProperty(&dependencyProperty));
        IFC_RETURN(transitionInfo.As(&dependencyObject));
        IFC_RETURN(dependencyObject->GetValue(dependencyProperty.Get(), &effectValue));

        IFC_RETURN(effectValue->get_Value(value));
        return S_OK;
    }

    _Check_return_ HRESULT SlideNavigationTransitionInfo::put_Effect(_In_ SlideNavigationTransitionEffect value)
    {
        wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty;
        wrl::ComPtr<xaml_animation::ISlideNavigationTransitionInfoStatics> statics;
        wrl::ComPtr<SlideNavigationTransitionInfo> transitionInfo(this);
        wrl::ComPtr<xaml::IDependencyObject> dependencyObject;

        wrl::ComPtr<IInspectable> boxedValue;
        IFC_RETURN(Private::ValueBoxer::CreateReference<SlideNavigationTransitionEffect>(value, &boxedValue));


        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SlideNavigationTransitionInfo).Get(), &statics));

        IFC_RETURN(statics->get_EffectProperty(&dependencyProperty));
        IFC_RETURN(transitionInfo.As(&dependencyObject));
        IFC_RETURN(dependencyObject->SetValue(dependencyProperty.Get(), boxedValue.Get()));
        return S_OK;
    }


    // ContinuumNavigationTransitionInfo

    ContinuumNavigationTransitionInfo::ContinuumNavigationTransitionInfo()
    {

    }

    ContinuumNavigationTransitionInfo::~ContinuumNavigationTransitionInfo()
    {

    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfo::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(ContinuumNavigationTransitionInfoFactory::EnsureProperties());

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo).Get(),
                &m_spCNTIStatics));

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
                &spInnerFactory));

        IFC(spInnerFactory->CreateInstance(
                static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfo::OnPropertyChanged(
        _In_ xaml::IDependencyPropertyChangedEventArgs* /* pArgs */)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfo::get_ExitElement(
        _Outptr_ xaml::IUIElement** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> exitElementProperty;
        wrl::ComPtr<xaml::IUIElement> element;
        wrl::ComPtr<ContinuumNavigationTransitionInfo> cnti (this);
        wrl::ComPtr<xaml::IDependencyObject> cntiAsDO;

        ARG_VALIDRETURNPOINTER(value);

        IFC(m_spCNTIStatics->get_ExitElementProperty(&exitElementProperty));
        IFC(cnti.As(&cntiAsDO))
        IFC(cntiAsDO->GetValue(exitElementProperty.Get(), &element));

        IFC(element.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfo::put_ExitElement(
        _In_ xaml::IUIElement* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IDependencyProperty> exitElementProperty;
        wrl::ComPtr<xaml::IUIElement> element;
        wrl::ComPtr<ContinuumNavigationTransitionInfo> cnti (this);
        wrl::ComPtr<xaml::IDependencyObject> cntiAsDO;

        IFC(m_spCNTIStatics->get_ExitElementProperty(&exitElementProperty));
        IFC(cnti.As(&cntiAsDO))

        IFC(cntiAsDO->SetValue(exitElementProperty.Get(), value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HStringReference navState (L"0");

        ARG_VALIDRETURNPOINTER(string);

        IFC(navState.CopyTo(string));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING /*string*/)
    {
        RRETURN(S_OK);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfo::CreateStoryboards(
        _In_ xaml::IUIElement* element,
        _In_ xaml_animation::NavigationTrigger trigger,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
    {
      HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement (element);
        wrl::ComPtr<xaml::IUIElement> spContinuumTarget;
        wrl::ComPtr<xaml_animation::IStoryboard> spStoryboard;
        std::vector<DoubleAnimationDescription> animations;

        // Strings used for property names.
        wrl_wrappers::HString propertyNameIsHitTestVisible;
        wrl_wrappers::HString propertyNameOpacity;
        wrl_wrappers::HString propertyNameTranslateX;
        wrl_wrappers::HString propertyNameTranslateY;
        wrl_wrappers::HString propertyNameScaleX;
        wrl_wrappers::HString propertyNameScaleY;
        wrl_wrappers::HString propertyNameCenterOfRotationX;
        wrl_wrappers::HString propertyNameCenterOfRotationY;
        wrl_wrappers::HString propertyNameCenterOfRotationZ;
        wrl_wrappers::HString propertyNameRotationX;

        // Collections of key-frame descriptions used to define the animation.
        // The "target" refers to the element that flies around during continuum
        // while the background refers to the page. These are referred as the active
        // and inactive elements respectively in UIX.
        std::vector<DoubleKeyFrameDescription> centerOfRotationXKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> centerOfRotationYKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> centerOfRotationZKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> rotationXKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> scaleXKeyFramesForBackground;
        std::vector<DoubleKeyFrameDescription> scaleYKeyFramesForBackground;
        std::vector<DoubleKeyFrameDescription> scaleXKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> scaleYKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> opacityKeyFramesForBackground;
        std::vector<DoubleKeyFrameDescription> opacityKeyFramesForTarget;
        std::vector<DoubleKeyFrameDescription> translateYKeyFramesForBackground;
        std::vector<DoubleKeyFrameDescription> translateYKeyFramesForTarget;

        // Easing functions.
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spPowerEasingOut2;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spCircleEasingOut;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingIn2;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingIn6;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingOut4;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingOut6;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingOut8;
        wrl::ComPtr<xaml_animation::IEasingFunctionBase> spExponentialEasingInOut;

        ARG_VALIDRETURNPOINTER(storyboards);

        // Instantiate strings for property names.
        IFC(propertyNameIsHitTestVisible.Set(STR_LEN_PAIR(L"UIElement.IsHitTestVisible")));
        IFC(propertyNameOpacity.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));
        IFC(propertyNameTranslateX.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX")));
        IFC(propertyNameTranslateY.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY")));
        IFC(propertyNameScaleX.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX")));
        IFC(propertyNameScaleY.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY")));
        IFC(propertyNameCenterOfRotationX.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationX)")));
        IFC(propertyNameCenterOfRotationY.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationY)")));
        IFC(propertyNameCenterOfRotationZ.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationZ)")));
        IFC(propertyNameRotationX.Set(STR_LEN_PAIR(L"(UIElement.Projection).(PlaneProjection.RotationX)")));

        // Get easing functions.
        IFC(NavigateTransitionHelper::GetPowerEase(2, xaml_animation::EasingMode_EaseOut, &spPowerEasingOut2));
        IFC(NavigateTransitionHelper::GetCircleEase(xaml_animation::EasingMode_EaseOut, &spCircleEasingOut));
        IFC(NavigateTransitionHelper::GetExponentialEase(2, xaml_animation::EasingMode_EaseIn, &spExponentialEasingIn2));
        IFC(NavigateTransitionHelper::GetExponentialEase(6, xaml_animation::EasingMode_EaseIn, &spExponentialEasingIn6));
        IFC(NavigateTransitionHelper::GetExponentialEase(4, xaml_animation::EasingMode_EaseOut, &spExponentialEasingOut4));
        IFC(NavigateTransitionHelper::GetExponentialEase(6, xaml_animation::EasingMode_EaseOut, &spExponentialEasingOut6));
        IFC(NavigateTransitionHelper::GetExponentialEase(8, xaml_animation::EasingMode_EaseOut, &spExponentialEasingOut8));
        IFC(NavigateTransitionHelper::GetExponentialEase(0.75, xaml_animation::EasingMode_EaseInOut, &spExponentialEasingInOut));

        if(trigger == xaml_animation::NavigationTrigger_NavigatingTo)
        {
            // Continuum animation forward in.
            const INT64 speedFactor = 1;
            const INT64 staggeringFactor = 267;
            const INT64 startTime = (0 + staggeringFactor) * speedFactor;
            const INT64 middleTime1 = (100 + staggeringFactor) * speedFactor;
            const INT64 middleTime2 = (167 + staggeringFactor) * speedFactor;
            const INT64 endTime = (350 + staggeringFactor) * speedFactor;
            const DOUBLE scaleFactorForBackground = 0.9;
            const DOUBLE centerOfRotationXForTarget = 0.0;
            const DOUBLE centerOfRotationYForTarget = 1.0;
            const DOUBLE centerOfRotationZForTarget = 0.0;
            const INT xRotationOffsetForTarget = -50;
            const INT yTranslationOffsetDownForTarget = 200;
            const INT yTranslationOffsetUpForTarget = -40;
            const DOUBLE scaleFactorForTarget = 1.5;
            const wf::Point transformOrigin = { 0.5f, 0.5f };

            // Define the key frames to scale the background along the X axis.
            scaleXKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, scaleFactorForBackground, spCircleEasingOut.Get()));
            scaleXKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_SCALE, spCircleEasingOut.Get()));

            // Define the key frames to scale the background along the Y axis.
            scaleYKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, scaleFactorForBackground, spCircleEasingOut.Get()));
            scaleYKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_SCALE, spCircleEasingOut.Get()));

            // Define the key frames to change the opacity of the background.
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::NO_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime - 1, NavigateTransitionHelper::NO_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::FULL_OPACITY));

            // Define the animations.
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameScaleX, &scaleXKeyFramesForBackground));
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameScaleY, &scaleYKeyFramesForBackground));
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, &opacityKeyFramesForBackground));

            // Animation for target.
            IFC(GetLogicalEntranceElement(element, &spContinuumTarget));

            if (spContinuumTarget)
            {
                wrl::ComPtr<xaml_media::IProjection> spPlaneProjection;

                // Clobber this, as we have to clobber values anyway.
                IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spPlaneProjection));
                IFC(spContinuumTarget->put_Projection(spPlaneProjection.Get()));

                // Define the key frame collections to hold the center of rotation
                // around the X, Y and Z axes.
                centerOfRotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationXForTarget));
                centerOfRotationYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationYForTarget));
                centerOfRotationZKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationZForTarget));

                // Define the key frames to rotate the target around the X axis.
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, xRotationOffsetForTarget));
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_OFFSET, spPowerEasingOut2.Get()));

                // Define the key frames to translate the target along the Y axis.
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, yTranslationOffsetDownForTarget));
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime1, yTranslationOffsetUpForTarget, spExponentialEasingOut8.Get()));
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_OFFSET, spExponentialEasingInOut.Get()));

                // Define the key frames to scale the target along the X axis.
                scaleXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, scaleFactorForTarget));
                scaleXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_SCALE, spPowerEasingOut2.Get()));

                // Define the key frames to scale the target along the Y axis.
                scaleYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, scaleFactorForTarget));
                scaleYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_SCALE, spPowerEasingOut2.Get()));

                // Define the key frames to change the opacity of the target.
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::NO_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, NavigateTransitionHelper::FULL_OPACITY, spExponentialEasingOut6.Get()));

                // Define the animations.
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationX, &centerOfRotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationY, &centerOfRotationYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationZ, &centerOfRotationZKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameRotationX, &rotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameTranslateY, &translateYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameScaleX, &scaleXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameScaleY, &scaleYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameOpacity, &opacityKeyFramesForTarget));
            }

            // Create the storyboard.
            IFC(NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(&animations, &spStoryboard));

            // Set the transform origin by adding a point animation.
            IFC(NavigateTransitionHelper::SetTransformOrigin(spElement.Get(), transformOrigin, spStoryboard.Get()));
        }
        else if(trigger == xaml_animation::NavigationTrigger_NavigatingAway)
        {
            // Continuum animation forward out.
            const INT64 speedFactor = 1;
            const INT64 staggeringFactor = 0;
            const INT64 startTime = (0 + staggeringFactor) * speedFactor;
            const INT64 middleTime1 = (100 + staggeringFactor) * speedFactor;
            const INT64 middleTime2 = (120 + staggeringFactor) * speedFactor;
            const INT64 endTime = (250 + staggeringFactor) * speedFactor;
            const DOUBLE centerOfRotationXForTarget = 0.0;
            const DOUBLE centerOfRotationYForTarget = 1.0;
            const DOUBLE centerOfRotationZForTarget = 0.0;
            const INT xRotationOffsetForTarget = 80;
            const INT yTranslationOffsetForTarget = 200;
            const DOUBLE scaleFactorForTarget = 1.5;
            const wf::Point transformOrigin = { 0.0f, 1.0f };

            // Define the key frames to change the opacity of the background.
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::FULL_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::FULL_OPACITY));
            // NOTE: This next key frame is not part of the original fade-out animation.
            // We are adding it in order to prevent the target from becoming invisible
            // halfway through the animation.
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(middleTime2, NavigateTransitionHelper::FULL_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_OPACITY, spCircleEasingOut.Get()));

            // Define the animations.
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, &opacityKeyFramesForBackground));

            // Animation for target.
            IFC(GetLogicalExitElement(element, &spContinuumTarget));

            if(spContinuumTarget)
            {
                wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                // Clobber this, as we have to clobber values anyway.
                IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                IFC(spContinuumTarget->put_Projection(spElementPlaneProjection.Get()));

                // Define the key frame collections to hold the center of rotation
                // around the X, Y and Z axes.
                centerOfRotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationXForTarget));
                centerOfRotationYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationYForTarget));
                centerOfRotationZKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationZForTarget));

                // Define the key frames to rotate the target around the X axis.
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_OFFSET));
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, xRotationOffsetForTarget, spExponentialEasingIn2.Get()));

                // Define the key frames to translate the target along the Y axis.
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_OFFSET));
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, yTranslationOffsetForTarget, spExponentialEasingIn6.Get()));

                // Define the key frames to scale the target along the X axis.
                scaleXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_SCALE));
                scaleXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, scaleFactorForTarget, spExponentialEasingIn6.Get()));

                // Define the key frames to scale the target along the Y axis.
                scaleYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_SCALE));
                scaleYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, scaleFactorForTarget, spExponentialEasingIn6.Get()));

                // Define the key frames to change the opacity of the target.
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::FULL_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::FULL_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime1, NavigateTransitionHelper::FULL_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime2, NavigateTransitionHelper::NO_OPACITY));

                // Define the animations.
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationX, &centerOfRotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationY, &centerOfRotationYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationZ, &centerOfRotationZKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameRotationX, &rotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameTranslateY, &translateYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameScaleX, &scaleXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameScaleY, &scaleYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameOpacity, &opacityKeyFramesForTarget));
            }

            // Create the storyboard.
            IFC(NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(&animations, &spStoryboard));

            if (spContinuumTarget)
            {
                // Set the transform origin by adding a point animation.
                IFC(NavigateTransitionHelper::SetTransformOrigin(spContinuumTarget.Get(), transformOrigin, spStoryboard.Get()));
            }
        }
        else if(trigger == xaml_animation::NavigationTrigger_BackNavigatingTo)
        {
            // Continuum animation backward in.
            const INT64 speedFactor = 1;
            const INT64 staggeringFactor = 267;
            const INT64 startTime = (0 + staggeringFactor) * speedFactor;
            const INT64 middleTime = (200 + staggeringFactor) * speedFactor;
            const INT64 endTime = (350 + staggeringFactor) * speedFactor;
            const DOUBLE centerOfRotationXForTarget = 0.0;
            const DOUBLE centerOfRotationYForTarget = 0.0;
            const DOUBLE centerOfRotationZForTarget = 0.0;
            const INT xRotationOffsetForTarget = -90;
            const INT yTranslationOffsetForTarget = -20;

            // Define the key frames to change the opacity of the background.
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::NO_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_OPACITY, spCircleEasingOut.Get()));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::FULL_OPACITY, spCircleEasingOut.Get()));

            // Define the animations.
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, &opacityKeyFramesForBackground));

            // Animation for target.
            // [TODO] - This call only works when NavigationCacheMode on the Page is
            // "Enabled" or "Required".  How we will handle this is currently unknown, as the cache and
            // state story in Jupiter is weak.
            IFC(GetLogicalExitElement(element, &spContinuumTarget));

            if(spContinuumTarget)
            {
                wrl::ComPtr<xaml_media::IProjection> spElementPlaneProjection;

                // Clobber this, as we have to clobber values anyway.
                IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_PlaneProjection).Get(), &spElementPlaneProjection));
                IFC(spContinuumTarget->put_Projection(spElementPlaneProjection.Get()));

                // Define the key frame collections to hold the center of rotation
                // around the X, Y and Z axes.
                centerOfRotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationXForTarget));
                centerOfRotationYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationYForTarget));
                centerOfRotationZKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, centerOfRotationZForTarget));

                // Define the key frames to rotate the target around the X axis.
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, xRotationOffsetForTarget));
                rotationXKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime, NavigateTransitionHelper::NO_OFFSET, spExponentialEasingOut4.Get()));

                // Define the key frames to translate the target along the Y axis.
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, yTranslationOffsetForTarget));
                translateYKeyFramesForTarget.push_back(DoubleKeyFrameDescription(middleTime, NavigateTransitionHelper::NO_OFFSET, spExponentialEasingOut4.Get()));

                // Define the key frames to change the opacity of the target.
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::NO_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime - 1, NavigateTransitionHelper::NO_OPACITY));
                opacityKeyFramesForTarget.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::FULL_OPACITY));

                // Define the animations.
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationX, &centerOfRotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationY, &centerOfRotationYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameCenterOfRotationZ, &centerOfRotationZKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameRotationX, &rotationXKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameTranslateY, &translateYKeyFramesForTarget));
                animations.push_back(DoubleAnimationDescription(spContinuumTarget.Get(), &propertyNameOpacity, &opacityKeyFramesForTarget));
            }

            // Create the storyboard.
            IFC(NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(&animations, &spStoryboard));
        }
        else if(trigger == xaml_animation::NavigationTrigger_BackNavigatingAway)
        {
            // Continuum animation backward out.
            const INT64 speedFactor = 1;
            const INT64 staggeringFactor = 0;
            const INT64 startTime = (0 + staggeringFactor) * speedFactor;
            const INT64 middleTime = (240 + staggeringFactor) * speedFactor;
            const INT64 endTime = (250 + staggeringFactor) * speedFactor;
            const INT yOffsetForBackground = 200;

            // Define the key frames to translate the background along the Y axis.
            translateYKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::NO_OFFSET));
            translateYKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, yOffsetForBackground, spExponentialEasingIn6.Get()));

            // Define the key frames to change the opacity of the background.
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, NavigateTransitionHelper::FULL_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(startTime, NavigateTransitionHelper::FULL_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(middleTime, NavigateTransitionHelper::FULL_OPACITY));
            opacityKeyFramesForBackground.push_back(DoubleKeyFrameDescription(endTime, NavigateTransitionHelper::NO_OPACITY));

            // Define the animations.
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameTranslateY, &translateYKeyFramesForBackground));
            animations.push_back(DoubleAnimationDescription(spElement.Get(), &propertyNameOpacity, &opacityKeyFramesForBackground));

            // Create the storyboard.
            IFC(NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(&animations, &spStoryboard));
        }

        // Ensure that the animated pages are not hit test visible.
        {
            wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
            wrl::ComPtr<ITimeline> spTimeline;
            wrl::ComPtr<IObjectAnimationUsingKeyFrames> spAnimation;
            std::vector<ObjectKeyFrameDescription> keyFramesIsHitTestVisible;

            // Set isHitTestVisible key frames.
            keyFramesIsHitTestVisible.push_back(ObjectKeyFrameDescription(NavigateTransitionHelper::TIME_ZERO, FALSE));

            // Add object animations to storyboard.
            IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));
            IFC(NavigateTransitionHelper::GetObjectAnimation(spStoryboardStatics, ObjectAnimationDescription(spElement.Get(), &propertyNameIsHitTestVisible, &keyFramesIsHitTestVisible), &spAnimation));

            IFC(spAnimation.As(&spTimeline));

            IFC(NavigateTransitionHelper::AddTimelineToStoryboard(spTimeline.Get(), spStoryboard.Get()));
        }

        IFC(storyboards->Append(spStoryboard.Get()));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfo::GetLogicalExitElement(
        _In_ xaml::IUIElement* page,
        _Outptr_result_maybenull_ xaml::IUIElement** target)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IUIElement> spExitElement;

        ARG_VALIDRETURNPOINTER(target);

        // Check for explicit target.
        IFC(get_ExitElement(&spExitElement));
        IFC(spExitElement.CopyTo(target));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfo::GetLogicalEntranceElement(
        _In_ xaml::IUIElement* page,
        _Outptr_result_maybenull_ xaml::IUIElement** target)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IUIElement> spEntranceElementResult;
        std::vector<wrl::WeakRef> entranceElements;

        ARG_VALIDRETURNPOINTER(target);

        IFC(ContinuumNavigationTransitionInfoFactory::GetEntranceElements(&entranceElements));

        for (auto &it : entranceElements)
        {
            wrl::ComPtr<xaml::IUIElement> spEntranceElement;

            IFC((it).As(&spEntranceElement));

            if (spEntranceElement)
            {
                wrl::ComPtr<xaml::IUIElement> spPage (page);
                wrl::ComPtr<xaml::IFrameworkElement> spPageAsFE;
                wrl::ComPtr<xaml::IFrameworkElement> spEntranceElementAsFE;
                BOOLEAN isAncestor = FALSE;

                IFC(spPage.As(&spPageAsFE));
                IFC(spEntranceElement.As(&spEntranceElementAsFE));

                IFC(NavigateTransitionHelper::IsAncestor(spPageAsFE.Get(), spEntranceElementAsFE.Get(), &isAncestor));

                if (isAncestor)
                {
                    IFC(spEntranceElement.As(&spEntranceElementResult));
                    break;
                }
            }
        }

        IFC(spEntranceElementResult.CopyTo(target));

    Cleanup:
        RRETURN(hr);
    }

    // ContinuumNavigationTransitionInfoFactory

    wrl::ComPtr<xaml::IDependencyProperty> ContinuumNavigationTransitionInfoFactory::s_spExitElementProperty;
    wrl::ComPtr<xaml::IDependencyProperty> ContinuumNavigationTransitionInfoFactory::s_spIsEntranceElementProperty;
    wrl::ComPtr<xaml::IDependencyProperty> ContinuumNavigationTransitionInfoFactory::s_spIsExitElementProperty;
    wrl::ComPtr<xaml::IDependencyProperty> ContinuumNavigationTransitionInfoFactory::s_spExitElementContainerProperty;

    std::vector<wrl::WeakRef> ContinuumNavigationTransitionInfoFactory::s_spEntranceElements;
    std::vector<wrl::WeakRef> ContinuumNavigationTransitionInfoFactory::s_spExitElements;

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        IFC(EnsureProperties());

    Cleanup:
        RRETURN(hr);
    }

    void
    ContinuumNavigationTransitionInfoFactory::ClearProperties()
    {
        s_spExitElementProperty.Reset();
        s_spIsEntranceElementProperty.Reset();
        s_spIsExitElementProperty.Reset();
        s_spExitElementContainerProperty.Reset();
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::EnsureProperties()
    {
        HRESULT hr = S_OK;

        IFC(InitializeExitElementProperty());
        IFC(InitializeIsEntranceElementProperty());
        IFC(InitializeIsExitElementProperty());
        IFC(InitializeExitElementContainerProperty());

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::InitializeExitElementProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spExitElementProperty)
        {
            IFC(Private::InitializeDependencyProperty(
                L"ExitElement",
                L"Microsoft.UI.Xaml.UIElement",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo,
                FALSE /* isAttached */,
                nullptr /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(ContinuumNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spExitElementProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::InitializeIsEntranceElementProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spIsEntranceElementProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC(Private::InitializeDependencyProperty(
                L"IsEntranceElement",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo,
                TRUE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(ContinuumNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spIsEntranceElementProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::InitializeIsExitElementProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spIsExitElementProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC(Private::InitializeDependencyProperty(
                L"IsExitElement",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo,
                TRUE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(ContinuumNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spIsExitElementProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::InitializeExitElementContainerProperty()
    {
        HRESULT hr = S_OK;

        if(!s_spExitElementContainerProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC(Private::InitializeDependencyProperty(
                L"ExitElementContainer",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo,
                TRUE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(ContinuumNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spExitElementContainerProperty));
        }

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::ActivateInstance(
        _Outptr_ IInspectable** ppInspectable)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<IContinuumNavigationTransitionInfo> cnti;

        ARG_VALIDRETURNPOINTER(ppInspectable);

        IFC(wrl::MakeAndInitialize<ContinuumNavigationTransitionInfo>(&cnti));

        IFC(cnti.CopyTo(ppInspectable));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::OnPropertyChanged(
        _In_ xaml::IDependencyObject* pSender,
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IDependencyObject> spDO(pSender);
        wrl::ComPtr<xaml::IDependencyPropertyChangedEventArgs> spArgs(pArgs);
        wrl::ComPtr<xaml::IDependencyProperty> spProperty;
        wrl::ComPtr<IInspectable> spNewValueInsp;

        ARG_VALIDRETURNPOINTER(spDO.Get());
        ARG_VALIDRETURNPOINTER(spArgs.Get());

        IFC(spArgs->get_Property(&spProperty));
        IFC(spArgs->get_NewValue(&spNewValueInsp));

        if (spProperty.Get() == s_spExitElementProperty.Get())
        {
            // This is just a DP: bubble up to DO.
            wrl::ComPtr<IContinuumNavigationTransitionInfo> cnti;
            IFC(wrl::ComPtr<xaml::IDependencyObject>(pSender).As<IContinuumNavigationTransitionInfo>(&cnti));
            IFC(static_cast<ContinuumNavigationTransitionInfo*>(cnti.Get())->OnPropertyChanged(pArgs));
        }
        else if (spProperty.Get() == s_spIsEntranceElementProperty)
        {
            // This is an attached property, bubble up to page.
            wrl::ComPtr<wf::IPropertyValue> spNewValue;
            IFC(spNewValueInsp.As(&spNewValue));

            BOOLEAN isEntranceElement = FALSE;
            IFC(spNewValue->GetBoolean(&isEntranceElement));

            // Erase the value in the list.
            for (auto it = s_spEntranceElements.begin(); it != s_spEntranceElements.end(); ++it)
            {
                wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

                IFC((*it).As(&spItAsDO));

                if (spItAsDO.Get() == spDO.Get())
                {
                    s_spEntranceElements.erase(it);
                    break;
                }
            }

            if (isEntranceElement)
            {
                wrl::WeakRef spElement;
                IFC(spDO.AsWeak(&spElement));

                s_spEntranceElements.push_back(spElement);
            }
        }
        else if(spProperty.Get() == s_spIsExitElementProperty)
        {
            // This is an attached property, bubble up to page.
            wrl::ComPtr<wf::IPropertyValue> spNewValue;
            IFC(spNewValueInsp.As(&spNewValue));

            BOOLEAN isExitElement = FALSE;
            IFC(spNewValue->GetBoolean(&isExitElement));

            // Erase the value in the list.
            for (auto it = s_spExitElements.begin(); it != s_spExitElements.end(); ++it)
            {
                wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

                IFC((*it).As(&spItAsDO));

                if (spItAsDO.Get() == spDO.Get())
                {
                    s_spExitElements.erase(it);
                    break;
                }
            }

            if (isExitElement)
            {
                wrl::WeakRef spElement;
                IFC(spDO.AsWeak(&spElement));

                s_spExitElements.push_back(spElement);
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::get_ExitElementProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spExitElementProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::get_IsEntranceElementProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spIsEntranceElementProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::get_IsExitElementProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spIsExitElementProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::get_ExitElementContainerProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(value);

        IFC(s_spExitElementContainerProperty.CopyTo(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::GetIsEntranceElement(
        _In_ xaml::IUIElement* element,
        _Out_ BOOLEAN* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);
        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(spElementAsDO->GetValue(s_spIsEntranceElementProperty.Get(), &boxedValue));
        IFC(boxedValue->GetBoolean(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::SetIsEntranceElement(
        _In_ xaml::IUIElement* element,
        _In_ BOOLEAN value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(Private::ValueBoxer::CreateBoolean(value, &boxedValue));
        IFC(spElementAsDO->SetValue(s_spIsEntranceElementProperty.Get(), boxedValue.Get()));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::GetIsExitElement(
        _In_ xaml::IUIElement* element,
        _Out_ BOOLEAN* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);
        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(spElementAsDO->GetValue(s_spIsExitElementProperty.Get(), &boxedValue));
        IFC(boxedValue->GetBoolean(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::SetIsExitElement(
        _In_ xaml::IUIElement* element,
        _In_ BOOLEAN value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(Private::ValueBoxer::CreateBoolean(value, &boxedValue));
        IFC(spElementAsDO->SetValue(s_spIsExitElementProperty.Get(), boxedValue.Get()));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::GetExitElementContainer(
        _In_ xaml_controls::IListViewBase* element,
        _Out_ BOOLEAN* value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_controls::IListViewBase> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);
        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(spElementAsDO->GetValue(s_spExitElementContainerProperty.Get(), &boxedValue));
        IFC(boxedValue->GetBoolean(value));

    Cleanup:
        RRETURN(hr);
    }

    IFACEMETHODIMP
    ContinuumNavigationTransitionInfoFactory::SetExitElementContainer(
        _In_ xaml_controls::IListViewBase* element,
        _In_ BOOLEAN value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_controls::IListViewBase> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC(spElement.As(&spElementAsDO));

        IFC(Private::ValueBoxer::CreateBoolean(value, &boxedValue));
        IFC(spElementAsDO->SetValue(s_spExitElementContainerProperty.Get(), boxedValue.Get()));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::GetEntranceElements(
        _Outptr_ std::vector<wrl::WeakRef>* elements)
    {
        ARG_VALIDRETURNPOINTER(elements);

        *elements = s_spEntranceElements;

        RRETURN(S_OK);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::GetExitElements(
        _Out_ std::vector<wrl::WeakRef>* elements)
    {
        ARG_VALIDRETURNPOINTER(elements);

        *elements = s_spExitElements;

        RRETURN(S_OK);
    }

    _Check_return_
    HRESULT
    ContinuumNavigationTransitionInfoFactory::ClearExitElements()
    {
        HRESULT hr = S_OK;

        auto it = s_spExitElements.begin();

        while (it != s_spExitElements.end())
        {
            wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

            IFC((*it).As(&spItAsDO));

            if (!spItAsDO)
            {
                it = s_spExitElements.erase(it);
            }
            else
            {
                ++it;
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
        HRESULT
        ContinuumNavigationTransitionInfoFactory::ClearEntranceElements()
    {
            HRESULT hr = S_OK;

            auto it = s_spEntranceElements.begin();

            while (it != s_spEntranceElements.end())
            {
                wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

                IFC((*it).As(&spItAsDO));

                if (!spItAsDO)
                {
                    it = s_spEntranceElements.erase(it);
                }
                else
                {
                    ++it;
                }
            }

        Cleanup:
            RRETURN(hr);
    }

    // DrillInNavigationTransitionInfo

    _Check_return_ HRESULT
    DrillInNavigationTransitionInfo::RuntimeClassInitialize()
    {
        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
            &spInnerFactory));

        IFC_RETURN(spInnerFactory->CreateInstance(
            static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
            &spInnerInspectable,
            &spInnerInstance));

        IFC_RETURN(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

        return S_OK;
    }

    IFACEMETHODIMP
    DrillInNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        IFC_RETURN(wrl_wrappers::HStringReference(L"0").CopyTo(string));

        return S_OK;
    }

    IFACEMETHODIMP
    DrillInNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING /*string*/)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    DrillInNavigationTransitionInfo::CreateStoryboards(
        _In_ xaml::IUIElement* element,
        _In_ xaml_animation::NavigationTrigger trigger,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
    {
        const wf::Point transformOrigin = { 0.5, 0.5 };
        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<IStoryboard> spStoryboard;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;

        wrl_wrappers::HString propertyNameScaleX;
        wrl_wrappers::HString propertyNameScaleY;
        wrl_wrappers::HString propertyNameOpacity;

        ARG_VALIDRETURNPOINTER(storyboards);

        // Instantiate property string.
        IFC_RETURN(propertyNameScaleX.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX")));
        IFC_RETURN(propertyNameScaleY.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY")));
        IFC_RETURN(propertyNameOpacity.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));

        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboard));

        IFC_RETURN(spElement.As(&spElementAsDO));
        IFC_RETURN(spStoryboard->get_Children(&spTimelines));

        if (trigger == xaml_animation::NavigationTrigger_NavigatingTo)
        {
            const DOUBLE scaleFactor = 0.94;
            const wf::Point scaleCurveControlPoint1 = { 0.1f, 0.9f };
            const wf::Point scaleCurveControlPoint2 = { 0.2f, 1.0f };
            const wf::Point opacityCurveControlPoint1 = { 0.17f, 0.17f };
            const wf::Point opacityCurveControlPoint2 = { 0.0f, 1.0f };
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleXAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleYAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;
            wrl::ComPtr<ITimeline> spScaleXAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spScaleYAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;

            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleXAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleYAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));

            IFC_RETURN(NavigateTransitionHelper::SetTransformOrigin(spElement.Get(), transformOrigin, spStoryboard.Get()));

            IFC_RETURN(spScaleXAnimation.As(&spScaleXAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleXAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleXAnimationAsTimeline.Get(), propertyNameScaleX.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), scaleFactor, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_NavigatingToScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spScaleYAnimation.As(&spScaleYAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleYAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleYAnimationAsTimeline.Get(), propertyNameScaleY.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), scaleFactor, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_NavigatingToScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 0.0, 0, opacityCurveControlPoint1, opacityCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_NavigatingToOpacityDuration, opacityCurveControlPoint1, opacityCurveControlPoint2));

            IFC_RETURN(spTimelines->Append(spScaleXAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spScaleYAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
        }
        else if (trigger == xaml_animation::NavigationTrigger_NavigatingAway)
        {
            const DOUBLE scaleFactor = 1.04;
            const wf::Point scaleCurveControlPoint1 = { 0.1f, 0.9f };
            const wf::Point scaleCurveControlPoint2 = { 0.2f, 1.0f };
            const wf::Point opacityCurveControlPoint1 = { 0.17f, 0.17f };
            const wf::Point opacityCurveControlPoint2 = { 0.0f, 1.0f };
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleXAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleYAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;
            wrl::ComPtr<ITimeline> spScaleXAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spScaleYAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;

            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleXAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleYAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));

            IFC_RETURN(NavigateTransitionHelper::SetTransformOrigin(spElement.Get(), transformOrigin, spStoryboard.Get()));

            IFC_RETURN(spScaleXAnimation.As(&spScaleXAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleXAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleXAnimationAsTimeline.Get(), propertyNameScaleX.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), 1.0, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), scaleFactor, DrillInNavigationTransitionInfo::s_NavigatingAwayScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spScaleYAnimation.As(&spScaleYAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleYAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleYAnimationAsTimeline.Get(), propertyNameScaleY.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), 1.0, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), scaleFactor, DrillInNavigationTransitionInfo::s_NavigatingAwayScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 1.0, 0, opacityCurveControlPoint1, opacityCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 0.0, DrillInNavigationTransitionInfo::s_NavigatingAwayOpacityDuration, opacityCurveControlPoint1, opacityCurveControlPoint2));

            IFC_RETURN(spTimelines->Append(spScaleXAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spScaleYAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
        }
        else if (trigger == xaml_animation::NavigationTrigger_BackNavigatingTo)
        {
            const DOUBLE scaleFactor = 1.06;
            const wf::Point scaleCurveControlPoint1 = { 0.12f, 0.0f };
            const wf::Point scaleCurveControlPoint2 = { 0.0f, 1.0f };
            const wf::Point opacityCurveControlPoint1 = { 0.17f, 0.17f };
            const wf::Point opacityCurveControlPoint2 = { 0.0f, 1.0f };
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleXAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleYAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;
            wrl::ComPtr<ITimeline> spScaleXAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spScaleYAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;

            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleXAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleYAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));

            IFC_RETURN(NavigateTransitionHelper::SetTransformOrigin(spElement.Get(), transformOrigin, spStoryboard.Get()));

            IFC_RETURN(spScaleXAnimation.As(&spScaleXAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleXAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleXAnimationAsTimeline.Get(), propertyNameScaleX.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), scaleFactor, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_BackNavigatingToScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spScaleYAnimation.As(&spScaleYAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleYAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleYAnimationAsTimeline.Get(), propertyNameScaleY.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), scaleFactor, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_BackNavigatingToScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 0.0, 0, opacityCurveControlPoint1, opacityCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 1.0, DrillInNavigationTransitionInfo::s_BackNavigatingToOpacityDuration, opacityCurveControlPoint1, opacityCurveControlPoint2));

            IFC_RETURN(spTimelines->Append(spScaleXAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spScaleYAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
        }
        else if (trigger == xaml_animation::NavigationTrigger_BackNavigatingAway)
        {
            const DOUBLE scaleFactor = 0.96;
            const wf::Point scaleCurveControlPoint1 = { 0.1f, 0.9f };
            const wf::Point scaleCurveControlPoint2 = { 0.2f, 1.0f };
            const wf::Point opacityCurveControlPoint1 = { 0.17f, 0.17f };
            const wf::Point opacityCurveControlPoint2 = { 0.0f, 1.0f };
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleXAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spScaleYAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;
            wrl::ComPtr<ITimeline> spScaleXAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spScaleYAnimationAsTimeline;
            wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;

            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleXAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spScaleYAnimation));
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));

            IFC_RETURN(NavigateTransitionHelper::SetTransformOrigin(spElement.Get(), transformOrigin, spStoryboard.Get()));

            IFC_RETURN(spScaleXAnimation.As(&spScaleXAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleXAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleXAnimationAsTimeline.Get(), propertyNameScaleX.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), 1.0, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleXAnimation.Get(), scaleFactor, DrillInNavigationTransitionInfo::s_BackNavigatingAwayScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spScaleYAnimation.As(&spScaleYAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spScaleYAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spScaleYAnimationAsTimeline.Get(), propertyNameScaleY.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), 1.0, 0, scaleCurveControlPoint1, scaleCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spScaleYAnimation.Get(), scaleFactor, DrillInNavigationTransitionInfo::s_BackNavigatingAwayScaleDuration, scaleCurveControlPoint1, scaleCurveControlPoint2));

            IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
            IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spElementAsDO.Get()));
            IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 1.0, 0, opacityCurveControlPoint1, opacityCurveControlPoint2));
            IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 0.0, DrillInNavigationTransitionInfo::s_BackNavigatingAwayOpacityDuration, opacityCurveControlPoint1, opacityCurveControlPoint2));

            IFC_RETURN(spTimelines->Append(spScaleXAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spScaleYAnimationAsTimeline.Get()));
            IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
        }

        IFC_RETURN(storyboards->Append(spStoryboard.Get()));

        return S_OK;
    }

    // SuppressNavigationTransitionInfo

    _Check_return_ HRESULT
    SuppressNavigationTransitionInfo::RuntimeClassInitialize()
    {
        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
            &spInnerFactory));

        IFC_RETURN(spInnerFactory->CreateInstance(
            static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
            &spInnerInspectable,
            &spInnerInstance));

        IFC_RETURN(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

        return S_OK;
    }

    IFACEMETHODIMP
    SuppressNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        IFC_RETURN(wrl_wrappers::HStringReference(L"0").CopyTo(string));

        return S_OK;
    }

    IFACEMETHODIMP
    SuppressNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING /*string*/)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    SuppressNavigationTransitionInfo::CreateStoryboards(
        _In_ xaml::IUIElement* /*element*/,
        _In_ xaml_animation::NavigationTrigger /*trigger*/,
        _In_ wfc::IVector<xaml_animation::Storyboard*>* /*storyboards*/)
    {
        // The transition uses an empty storyboard.
        return S_OK;
    }

    // EntranceNavigationTransitionInfo

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfo::RuntimeClassInitialize()
    {
        wrl::ComPtr<xaml_animation::INavigationTransitionInfoFactory> spInnerFactory;
        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        // Ensure the properties related to this factory are registered.
        IFC_RETURN(EntranceNavigationTransitionInfoFactory::EnsureProperties());

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EntranceNavigationTransitionInfo).Get(),
            &m_spENTIStatics));

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationTransitionInfo).Get(),
            &spInnerFactory));

        IFC_RETURN(spInnerFactory->CreateInstance(
            static_cast<IInspectable*>(static_cast<xaml_animation::INavigationTransitionInfo*>(this)),
            &spInnerInspectable,
            &spInnerInstance));

        IFC_RETURN(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

        return S_OK;
    }

    _Check_return_
        HRESULT
        EntranceNavigationTransitionInfo::OnPropertyChanged(
        _In_ xaml::IDependencyPropertyChangedEventArgs* /* pArgs */)
    {
        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfo::GetNavigationStateCore(
        _Out_ HSTRING* string)
    {
        IFC_RETURN(wrl_wrappers::HStringReference(L"0").CopyTo(string));

        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfo::SetNavigationStateCore(
        _In_ HSTRING /*string*/)
    {
        return S_OK;
    }

    IFACEMETHODIMP
        EntranceNavigationTransitionInfo::CreateStoryboards(
            _In_ xaml::IUIElement* element,
            _In_ xaml_animation::NavigationTrigger trigger,
            _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
    {
        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IUIElement> spTarget;
        wrl::ComPtr<xaml::IDependencyObject> spAnimationTargetAsDO;
        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<IStoryboard> spStoryboard;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;

        wrl_wrappers::HString propertyNameTranslateX;
        wrl_wrappers::HString propertyNameTranslateY;
        wrl_wrappers::HString propertyNameOpacity;

        ARG_VALIDRETURNPOINTER(storyboards);

        // Instantiate property string.
        IFC_RETURN(propertyNameTranslateY.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY")));
        IFC_RETURN(propertyNameOpacity.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));

        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboard));

        IFC_RETURN(GetLogicalTargetElement(element, &spTarget));

        if (spTarget)
        {
            IFC_RETURN(spTarget.As(&spAnimationTargetAsDO));
        }
        else
        {
            IFC_RETURN(spElement.As(&spAnimationTargetAsDO));
        }

        IFC_RETURN(spStoryboard->get_Children(&spTimelines));

        {
            const DOUBLE translationOffset = 140;
            const wf::Point inControlPoint1 = { 0.1f, 0.9f };
            const wf::Point inControlPoint2 = { 0.2f, 1.0f };
            const wf::Point outControlPoint1 = { 0.7f, 0.0f };
            const wf::Point outControlPoint2 = { 1.0f, .5f };
            const UINT64 outDuration = 150;
            const UINT64 inDuration = 300;

            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spTranslateYAnimation;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spOpacityAnimation;

            switch (trigger)
            {
            case xaml_animation::NavigationTrigger_NavigatingAway:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 0.0, outDuration, outControlPoint1, outControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_NavigatingTo:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, outDuration));

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateYAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateYAnimation.Get(), translationOffset, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateYAnimation.Get(), translationOffset, outDuration));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateYAnimation.Get(), 0.0, outDuration + inDuration, inControlPoint1, inControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingAway:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 1.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, outDuration));  // Hide it at end of translate animation

                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spTranslateYAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spTranslateYAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spTranslateYAnimation.Get(), translationOffset, outDuration, outControlPoint1, outControlPoint2));
                break;
            case xaml_animation::NavigationTrigger_BackNavigatingTo:
                IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spOpacityAnimation));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, 0));
                IFC_RETURN(NavigateTransitionHelper::RegisterDiscreteKeyFrame(spOpacityAnimation.Get(), 0.0, outDuration));
                IFC_RETURN(NavigateTransitionHelper::RegisterSplineKeyFrame(spOpacityAnimation.Get(), 1.0, outDuration + inDuration, inControlPoint1, inControlPoint2));
                break;
            }

            if (spTranslateYAnimation != nullptr)
            {
                wrl::ComPtr<ITimeline> spTranslateYAnimationAsTimeline;
                IFC_RETURN(spTranslateYAnimation.As(&spTranslateYAnimationAsTimeline));
                IFC_RETURN(spStoryboardStatics->SetTarget(spTranslateYAnimationAsTimeline.Get(), spAnimationTargetAsDO.Get()));
                IFC_RETURN(spStoryboardStatics->SetTargetProperty(spTranslateYAnimationAsTimeline.Get(), propertyNameTranslateY.Get()));
                IFC_RETURN(spTimelines->Append(spTranslateYAnimationAsTimeline.Get()));
            }

            if (spOpacityAnimation != nullptr)
            {
                wrl::ComPtr<ITimeline> spOpacityAnimationAsTimeline;
                IFC_RETURN(spOpacityAnimation.As(&spOpacityAnimationAsTimeline));
                IFC_RETURN(spStoryboardStatics->SetTarget(spOpacityAnimationAsTimeline.Get(), spAnimationTargetAsDO.Get()));
                IFC_RETURN(spStoryboardStatics->SetTargetProperty(spOpacityAnimationAsTimeline.Get(), propertyNameOpacity.Get()));
                IFC_RETURN(spTimelines->Append(spOpacityAnimationAsTimeline.Get()));
            }
        }

        IFC_RETURN(storyboards->Append(spStoryboard.Get()));

        return S_OK;
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfo::GetLogicalTargetElement(
        _In_ xaml::IUIElement* page,
        _Outptr_result_maybenull_ xaml::IUIElement** target)
    {
        wrl::ComPtr<xaml::IUIElement> spTargetElement;
        std::vector<wrl::WeakRef> targetElements;

        ARG_VALIDRETURNPOINTER(target);

        IFC_RETURN(EntranceNavigationTransitionInfoFactory::GetTargetElements(&targetElements));

        for (auto &it : targetElements)
        {
            IFC_RETURN((it).As(&spTargetElement));

            if (spTargetElement)
            {
                wrl::ComPtr<xaml::IUIElement> spPage(page);
                wrl::ComPtr<xaml::IFrameworkElement> spPageAsFE;
                wrl::ComPtr<xaml::IFrameworkElement> spTargetElementAsFE;
                BOOLEAN isAncestor = FALSE;

                IFC_RETURN(spPage.As(&spPageAsFE));
                IFC_RETURN(spTargetElement.As(&spTargetElementAsFE));

                IFC_RETURN(NavigateTransitionHelper::IsAncestor(spPageAsFE.Get(), spTargetElementAsFE.Get(), &isAncestor));

                if (isAncestor)
                {
                    // Valid target element found.
                    break;
                }
            }
        }

        IFC_RETURN(spTargetElement.CopyTo(target));

        return S_OK;
    }

    // EntranceNavigationTransitionInfoFactory

    wrl::ComPtr<xaml::IDependencyProperty> EntranceNavigationTransitionInfoFactory::s_spIsTargetElementProperty;
    std::vector<wrl::WeakRef> EntranceNavigationTransitionInfoFactory::s_spTargetElements;

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::RuntimeClassInitialize()
    {
        IFC_RETURN(EnsureProperties());

        return S_OK;
    }

    void
    EntranceNavigationTransitionInfoFactory::ClearProperties()
    {
        s_spIsTargetElementProperty.Reset();
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::EnsureProperties()
    {
        IFC_RETURN(InitializeIsTargetElementProperty());

        return S_OK;
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::InitializeIsTargetElementProperty()
    {
        if (!s_spIsTargetElementProperty)
        {
            wrl::ComPtr<IInspectable> spDefaultValue;

            IFC_RETURN(Private::ValueBoxer::CreateBoolean(FALSE, &spDefaultValue));

            IFC_RETURN(Private::InitializeDependencyProperty(
                L"IsEntranceElement",
                L"Boolean",
                RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EntranceNavigationTransitionInfo,
                TRUE /* isAttached */,
                spDefaultValue.Get() /* defaultValue */,
                wrl::Callback<xaml::IPropertyChangedCallback>(EntranceNavigationTransitionInfoFactory::OnPropertyChanged).Get(),
                &s_spIsTargetElementProperty));
        }

        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfoFactory::ActivateInstance(
        _Outptr_ IInspectable** ppInspectable)
    {
        wrl::ComPtr<IEntranceNavigationTransitionInfo> enti;

        ARG_VALIDRETURNPOINTER(ppInspectable);

        IFC_RETURN(wrl::MakeAndInitialize<EntranceNavigationTransitionInfo>(&enti));
        IFC_RETURN(enti.CopyTo(ppInspectable));

        return S_OK;
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::OnPropertyChanged(
        _In_ xaml::IDependencyObject* pSender,
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
    {
        wrl::ComPtr<xaml::IDependencyObject> spDO(pSender);
        wrl::ComPtr<xaml::IDependencyPropertyChangedEventArgs> spArgs(pArgs);
        wrl::ComPtr<xaml::IDependencyProperty> spProperty;
        wrl::ComPtr<IInspectable> spNewValueInsp;

        ARG_VALIDRETURNPOINTER(spDO.Get());
        ARG_VALIDRETURNPOINTER(spArgs.Get());

        IFC_RETURN(spArgs->get_Property(&spProperty));
        IFC_RETURN(spArgs->get_NewValue(&spNewValueInsp));

        if (spProperty.Get() == s_spIsTargetElementProperty)
        {
            // This is an attached property, bubble up to page.
            BOOLEAN isTargetElement = FALSE;
            wrl::ComPtr<wf::IPropertyValue> spNewValue;

            IFC_RETURN(spNewValueInsp.As(&spNewValue));
            IFC_RETURN(spNewValue->GetBoolean(&isTargetElement));

            // Erase the value in the list.
            for (auto it = s_spTargetElements.begin(); it != s_spTargetElements.end(); ++it)
            {
                wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

                IFC_RETURN((*it).As(&spItAsDO));

                if (spItAsDO.Get() == spDO.Get())
                {
                    s_spTargetElements.erase(it);
                    break;
                }
            }

            if (isTargetElement)
            {
                wrl::WeakRef spElement;
                IFC_RETURN(spDO.AsWeak(&spElement));

                s_spTargetElements.push_back(spElement);
            }
        }

        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfoFactory::get_IsTargetElementProperty(
        _Outptr_ xaml::IDependencyProperty** value)
    {
        ARG_VALIDRETURNPOINTER(value);
        IFC_RETURN(s_spIsTargetElementProperty.CopyTo(value));

        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfoFactory::GetIsTargetElement(
        _In_ xaml::IUIElement* element,
        _Out_ BOOLEAN* value)
    {
        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(value);
        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC_RETURN(spElement.As(&spElementAsDO));

        IFC_RETURN(spElementAsDO->GetValue(s_spIsTargetElementProperty.Get(), &boxedValue));
        IFC_RETURN(boxedValue->GetBoolean(value));

        return S_OK;
    }

    IFACEMETHODIMP
    EntranceNavigationTransitionInfoFactory::SetIsTargetElement(
        _In_ xaml::IUIElement* element,
        _In_ BOOLEAN value)
    {
        wrl::ComPtr<xaml::IUIElement> spElement(element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        RoVariant boxedValue;

        ARG_VALIDRETURNPOINTER(spElement.Get());
        IFC_RETURN(spElement.As(&spElementAsDO));

        IFC_RETURN(Private::ValueBoxer::CreateBoolean(value, &boxedValue));
        IFC_RETURN(spElementAsDO->SetValue(s_spIsTargetElementProperty.Get(), boxedValue.Get()));

        return S_OK;
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::GetTargetElements(
        _Outptr_ std::vector<wrl::WeakRef>* elements)
    {
        ARG_VALIDRETURNPOINTER(elements);
        *elements = s_spTargetElements;

        return S_OK;
    }

    _Check_return_ HRESULT
    EntranceNavigationTransitionInfoFactory::ClearTargetElements()
    {
        auto it = s_spTargetElements.begin();

        while (it != s_spTargetElements.end())
        {
            wrl::ComPtr<xaml::IDependencyObject> spItAsDO;

            IFC_RETURN((*it).As(&spItAsDO));

            if (!spItAsDO)
            {
                it = s_spTargetElements.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return S_OK;
    }

} } } } } XAML_ABI_NAMESPACE_END