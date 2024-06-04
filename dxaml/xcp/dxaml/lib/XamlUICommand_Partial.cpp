// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Derivation of ICommand that relays Execute and CanExecute
//      to provided events, or to a child ICommand.

#include "precomp.h"
#include "XamlUICommand.g.h"
#include "CommandingContainer.g.h"
#include "CanExecuteRequestedEventArgs.g.h"
#include "ExecuteRequestedEventArgs.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT XamlUICommand::CanExecuteImpl(_In_opt_ IInspectable* parameter, _Out_ BOOLEAN* returnValue)
{
    BOOLEAN canExecute = FALSE;
    
    CanExecuteRequestedEventSourceType* eventSource = nullptr;
    ctl::ComPtr<CanExecuteRequestedEventArgs> args;

    IFC_RETURN(ctl::make(&args));
    IFC_RETURN(args->put_Parameter(parameter));
    IFC_RETURN(args->put_CanExecute(TRUE));
    
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
    ctl::ComPtr<CommandingContainer> focusedCommandingContainer;
    IFC_RETURN(GetFocusedCommandingContainer(&focusedCommandingContainer));
    
    if (focusedCommandingContainer)
    {
        IFC_RETURN(args->put_CommandTarget(focusedCommandingContainer->GetCommandTargetNoRef()));
        IFC_RETURN(args->put_ListCommandTarget(focusedCommandingContainer->GetListCommandTargetNoRef()));
    }
#endif

    IFC_RETURN(GetCanExecuteRequestedEventSourceNoRef(&eventSource));
    ctl::ComPtr<IXamlUICommand> thisAsIXamlUICommand;
    IFC_RETURN(ctl::do_query_interface(thisAsIXamlUICommand, this));
    IFC_RETURN(eventSource->Raise(thisAsIXamlUICommand.Get(), args.Get()));

    IFC_RETURN(args->get_CanExecute(&canExecute));
    
    ctl::ComPtr<ICommand> childCommand;
    IFC_RETURN(get_Command(&childCommand));
    
    if (childCommand)
    {
        BOOLEAN childCommandCanExecute = FALSE;
        IFC_RETURN(childCommand->CanExecute(parameter, &childCommandCanExecute));
        canExecute = canExecute && childCommandCanExecute;
    }
    
    *returnValue = canExecute;
    return S_OK;
}

_Check_return_ HRESULT XamlUICommand::ExecuteImpl(_In_opt_ IInspectable* parameter)
{
    ExecuteRequestedEventSourceType* eventSource = nullptr;
    ctl::ComPtr<ExecuteRequestedEventArgs> args;

    IFC_RETURN(ctl::make(&args));
    IFC_RETURN(args->put_Parameter(parameter));
    
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
    ctl::ComPtr<CommandingContainer> focusedCommandingContainer;
    IFC_RETURN(GetFocusedCommandingContainer(&focusedCommandingContainer));
    
    if (focusedCommandingContainer)
    {
        IFC_RETURN(args->put_CommandTarget(focusedCommandingContainer->GetCommandTargetNoRef()));
        IFC_RETURN(args->put_ListCommandTarget(focusedCommandingContainer->GetListCommandTargetNoRef()));
    }
#endif

    IFC_RETURN(GetExecuteRequestedEventSourceNoRef(&eventSource));
    ctl::ComPtr<IXamlUICommand> thisAsIXamlUICommand;
    IFC_RETURN(ctl::do_query_interface(thisAsIXamlUICommand, this));
    IFC_RETURN(eventSource->Raise(thisAsIXamlUICommand.Get(), args.Get()));
    
    ctl::ComPtr<ICommand> childCommand;
    IFC_RETURN(get_Command(&childCommand));
    
    if (childCommand)
    {
        IFC_RETURN(childCommand->Execute(parameter));
    }
    
    return S_OK;
}

_Check_return_ HRESULT XamlUICommand::NotifyCanExecuteChangedImpl()
{
    CanExecuteChangedEventSourceType* eventSource = nullptr;

    IFC_RETURN(GetCanExecuteChangedEventSourceNoRef(&eventSource));
    ctl::ComPtr<IXamlUICommand> thisAsIXamlUICommand;
    IFC_RETURN(ctl::do_query_interface(thisAsIXamlUICommand, this));
    IFC_RETURN(eventSource->Raise(thisAsIXamlUICommand.Get(), nullptr));

    return S_OK;
}

/* static */ _Check_return_ HRESULT XamlUICommand::GetFocusedCommandingContainer(_Outptr_result_maybenull_ CommandingContainer** commandingContainer)
{
    *commandingContainer = nullptr;
    
    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));
    
    ctl::ComPtr<xaml::IDependencyObject> currentObject(focusedElement);
    ctl::ComPtr<ICommandingContainer> container;

    // To get the command target, first we need to find the commanding container.
    // If there is one, the command target is the last focused element of its commanding target.
    // If there isn't one, then we can't determine a command target - we'll return null.
    while (currentObject)
    {
        // We'll first see if this object itself is the commanding container.
        container = currentObject.AsOrNull<ICommandingContainer>();

        // If it's not, we'll see if this is a commanding target associated with a commanding container.
        if (!container)
        {
            IFC_RETURN(CommandingContainerFactory::GetCommandingContainerStatic(currentObject.Get(), &container));
        }

        if (container)
        {
            break;
        }

        ctl::ComPtr<xaml::IDependencyObject> parent;
        IFC_RETURN(VisualTreeHelper::GetParentStatic(currentObject.Get(), &parent));
        currentObject = parent;
    }

    if (container)
    {
        ctl::ComPtr<CommandingContainer> containerImpl;
        IFC_RETURN(container.As(&containerImpl));
        *commandingContainer = containerImpl.Detach();
    }
    
    return S_OK;
}
