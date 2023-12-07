// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides a connection between commands and the controls they operate on.

#include "precomp.h"
#include "CommandingContainer.g.h"
#include "CommandingContextChangedEventArgs.g.h"
#include "PropertyChangedParamsHelper.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

xaml::IDependencyObject* CommandingContainer::GetCommandTargetNoRef()
{
    ctl::ComPtr<xaml::IDependencyObject> commandTarget = m_wrCommandTarget.AsOrNull<xaml::IDependencyObject>();
    
    if (commandTarget)
    {
        return commandTarget.Get();
    }
    else
    {
        return nullptr;
    }
}

xaml_controls::IItemsControl* CommandingContainer::GetListCommandTargetNoRef()
{
    ctl::ComPtr<xaml_controls::IItemsControl> listCommandTarget = m_wrListCommandTarget.AsOrNull<xaml_controls::IItemsControl>();
    
    if (listCommandTarget)
    {
        return listCommandTarget.Get();
    }
    else
    {
        return nullptr;
    }
}

_Check_return_ HRESULT CommandingContainer::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::CommandingContainer_CommandingTarget)
    {
        ctl::ComPtr<IInspectable> oldValueAsI;
        ctl::ComPtr<xaml::IDependencyObject> oldValue;
        ctl::ComPtr<IInspectable> newValueAsI;
        ctl::ComPtr<xaml::IDependencyObject> newValue;
        
        IFC_RETURN(PropertyChangedParamsHelper::GetObjects(args, &oldValueAsI, &newValueAsI));
        ASSERT(CDependencyObject::IsDependencyPropertyWeakRef(KnownPropertyIndex::CommandingContainer_CommandingTarget));

        if (oldValueAsI)
        {
            oldValue = ValueWeakReference::get_value_as<xaml::IDependencyObject>(oldValueAsI.Get());
        }

        if (newValueAsI)
        {
            newValue = ValueWeakReference::get_value_as<xaml::IDependencyObject>(newValueAsI.Get());
        }
        
        IFC_RETURN(OnCommandingTargetChanged(oldValue.Get(), newValue.Get()));
    }
    else
    {
        IFC_RETURN(CommandingContainerGenerated::OnPropertyChanged2(args));
    }
    
    return S_OK;
}

_Check_return_ HRESULT CommandingContainer::OnCommandingTargetChanged(_In_ xaml::IDependencyObject* oldTarget, _In_ xaml::IDependencyObject* newTarget)
{
    if (oldTarget)
    {
        CommandingContainerFactory::SetCommandingContainerStatic(oldTarget, nullptr);
        
        ctl::ComPtr<xaml::IDependencyObject> oldTargetComPtr(oldTarget);
        ctl::ComPtr<xaml::IUIElement> oldTargetAsUIE = oldTargetComPtr.AsOrNull<xaml::IUIElement>();
        
        if (oldTargetAsUIE)
        {
            IFC_RETURN(m_commandingTargetGotFocusHandler.DetachEventHandler(oldTargetAsUIE.Get()));
        }
    }

    // We have a new commanding target, so we don't yet have any idea what our command target is, if anything.
    m_wrCommandTarget.Reset();
    m_wrListCommandTarget.Reset();

    if (newTarget)
    {
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
        ctl::ComPtr<ICommandingContainer> thisAsICommandingContainer;
        IFC_RETURN(ctl::do_query_interface(thisAsICommandingContainer, this));
        CommandingContainerFactory::SetCommandingContainerStatic(newTarget, thisAsICommandingContainer.Get());
#endif
        
        ctl::ComPtr<xaml::IDependencyObject> newTargetComPtr(newTarget);
        ctl::ComPtr<xaml::IUIElement> newTargetAsUIE = newTargetComPtr.AsOrNull<xaml::IUIElement>();
        
        if (newTargetAsUIE)
        {
            IFC_RETURN(m_commandingTargetGotFocusHandler.AttachEventHandler(newTargetAsUIE.Get(),
                [this](IInspectable *sender, xaml::IRoutedEventArgs *args)
                {
                    IFC_RETURN(OnCommandingTargetGotFocus(args));
                    return S_OK;
                }));
        }
    }
    
    return S_OK;
}

_Check_return_ HRESULT CommandingContainer::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    IFC_RETURN(CommandingContainerGenerated::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive)
    {
        if (!m_commandingTargetGotFocusHandler)
        {
            ctl::ComPtr<xaml::IDependencyObject> commandingTarget;
            IFC_RETURN(get_CommandingTarget(&commandingTarget));
            
            ctl::ComPtr<xaml::IUIElement> commandingTargetAsUIE = commandingTarget.AsOrNull<xaml::IUIElement>();

            if (commandingTargetAsUIE)
            {
                IFC_RETURN(m_commandingTargetGotFocusHandler.AttachEventHandler(commandingTargetAsUIE.Get(),
                    [this](IInspectable *sender, xaml::IRoutedEventArgs *args)
                    {
                        IFC_RETURN(OnCommandingTargetGotFocus(args));
                        return S_OK;
                    }));
            }
        }

        if (!m_gotFocusHandler)
        {
            IFC_RETURN(m_gotFocusHandler.AttachEventHandler(this,
                [this](IInspectable *sender, xaml::IRoutedEventArgs *args)
            {
                IFC_RETURN(OnGotFocus(args));
                return S_OK;
            }));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingContainer::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    IFC_RETURN(CommandingContainerGenerated::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

    if (bLive)
    {
        if (m_commandingTargetGotFocusHandler)
        {
            ctl::ComPtr<xaml::IDependencyObject> commandingTarget;
            IFC_RETURN(get_CommandingTarget(&commandingTarget));
            
            ctl::ComPtr<xaml::IUIElement> commandingTargetAsUIE = commandingTarget.AsOrNull<xaml::IUIElement>();

            if (commandingTargetAsUIE)
            {
                IFC_RETURN(m_commandingTargetGotFocusHandler.DetachEventHandler(commandingTargetAsUIE.Get()));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingContainer::OnGotFocus(_In_ xaml::IRoutedEventArgs* args)
{
    ctl::ComPtr<xaml::IDependencyObject> commandingTarget;
    IFC_RETURN(get_CommandingTarget(&commandingTarget));
    
    // If don't have a commanding target, then we are the commanding target,
    // and we should respond to focus within ourselves.
    // If we do have a commanding target, however, then we should do nothing -
    // we track focus on the commanding target.
    if (!commandingTarget)
    {
        IFC_RETURN(OnCommandingTargetGotFocus(args));
    }
    
    return S_OK;
}

_Check_return_ HRESULT CommandingContainer::OnCommandingTargetGotFocus(_In_ xaml::IRoutedEventArgs* args)
{
    ctl::ComPtr<DependencyObject> focusedElement;
    ctl::ComPtr<IInspectable> originalSourceAsI;

    IFC_RETURN(GetFocusedElement(&focusedElement));
    IFC_RETURN(args->get_OriginalSource(&originalSourceAsI));

    // GotFocus is async, so let's make sure the original source is still the focused element.
    if (!originalSourceAsI || !focusedElement || !ctl::are_equal(originalSourceAsI.Get(), focusedElement.Get()))
    {
        return S_OK;
    }

    ctl::ComPtr<xaml::IDependencyObject> originalSource;
    IFC_RETURN(originalSourceAsI.As(&originalSource));
    IFC_RETURN(CommandingContainer::NotifyContextChangedStatic(originalSource.Get()));
    return S_OK;
}

/* static */ _Check_return_ HRESULT CommandingContainer::NotifyContextChangedStatic(_In_ xaml::IDependencyObject* commandTarget)
{
    // To notify that we have a new commanding context, we'll bubble up the visual tree
    // until we find an element that has an associated commanding container.
    // Once we find one, we'll raise its ContextChanged event so it can know that
    // something interesting has changed and that it should update its UI.
    // If we don't find a commanding container, this will be a no-op.
    ctl::ComPtr<xaml_controls::IItemsControl> listCommandTarget;
    ctl::ComPtr<xaml::IDependencyObject> currentElement(commandTarget);

    while (currentElement)
    {
        if (!listCommandTarget)
        {
            listCommandTarget = currentElement.AsOrNull<xaml_controls::IItemsControl>();
        }
        
        ctl::ComPtr<xaml_controls::ICommandingContainer> commandingContainer = currentElement.AsOrNull<xaml_controls::ICommandingContainer>();
        
        if (!commandingContainer)
        {
            IFC_RETURN(CommandingContainerFactory::GetCommandingContainerStatic(currentElement.Get(), &commandingContainer));
        }

        if (commandingContainer)
        {
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
            ctl::ComPtr<xaml::IDependencyObject> commandTargetComPtr(commandTarget);
            ctl::ComPtr<CommandingContainer> commandingContainerImpl;
            IFC_RETURN(commandingContainer.As(&commandingContainerImpl));
            
            IFC_RETURN(commandTargetComPtr.AsWeak(&commandingContainerImpl->m_wrCommandTarget));
            IFC_RETURN(listCommandTarget.AsWeak(&commandingContainerImpl->m_wrListCommandTarget));
    
            ContextChangedEventSourceType* eventSourceNoRef = nullptr;
            IFC_RETURN(commandingContainerImpl->GetContextChangedEventSourceNoRef(&eventSourceNoRef));
            
            ctl::ComPtr<CommandingContextChangedEventArgs> args;
            IFC_RETURN(ctl::make(&args));
            IFC_RETURN(args->put_CommandTarget(commandTarget));
            IFC_RETURN(args->put_ListCommandTarget(listCommandTarget.Get()));

            IFC_RETURN(eventSourceNoRef->Raise(commandingContainer.Get(), args.Get()));
#endif
            break;
        }

        ctl::ComPtr<xaml::IDependencyObject> currentElementAsDO;
        ctl::ComPtr<xaml::IDependencyObject> parent;
        
        IFC_RETURN(currentElement.As(&currentElementAsDO));
        IFC_RETURN(VisualTreeHelper::GetParentStatic(currentElementAsDO.Get(), &parent));
        IFC_RETURN(parent.As(&currentElement));
    }
    
    return S_OK;
}

_Check_return_ HRESULT CommandingContainerFactory::NotifyContextChangedImpl(_In_ xaml::IDependencyObject* commandTarget)
{
    IFC_RETURN(CommandingContainer::NotifyContextChangedStatic(commandTarget));
    return S_OK;
}