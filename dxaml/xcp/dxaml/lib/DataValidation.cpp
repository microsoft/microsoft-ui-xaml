// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "comInstantiation.h"
#include "InputValidationError.g.h"
#include "DataErrorsChangedEventArgs.g.h"
#include "InputValidationErrorEventArgs.g.h"
#include "HasValidationErrorsChangedEventArgs.g.h"
#include "InputValidationCommand.g.h"
#include "InputValidationContext.g.h"
#include "Control_Partial.h"
#include "UIElement_Partial.h"

namespace DirectUI
{
    _Check_return_ HRESULT InputValidationErrorFactory::CreateInstanceImpl(_In_ HSTRING errorMessage, _In_opt_ IInspectable* outer, _Outptr_ IInspectable** inner, _Outptr_ xaml_controls::IInputValidationError** instance)
    {
        ctl::ComPtr<xaml_controls::IInputValidationError> error;
        IFC_RETURN(ctl::AggregableActivationFactory<InputValidationError>::ActivateInstance(outer, inner));
        IFC_RETURN(ctl::do_query_interface(error, *inner));
        
        IFCFAILFAST(ctl::impl_cast<InputValidationError>(error.Get())->put_ErrorMessage(errorMessage));
        *instance = error.Detach();
        return S_OK;
    }

    _Check_return_ HRESULT DataErrorsChangedEventArgsFactory::CreateInstanceImpl(_In_opt_ HSTRING name, _Outptr_ xaml_data::IDataErrorsChangedEventArgs** instance)
    {

        ctl::ComPtr<DataErrorsChangedEventArgs> errorArgs;
        IFCFAILFAST(ctl::make<DataErrorsChangedEventArgs>(&errorArgs));
        IFCFAILFAST(errorArgs->put_PropertyName(name));
        *instance = errorArgs.Detach();
        return S_OK;
    }

    _Check_return_ HRESULT InputValidationErrorEventArgsFactory::CreateInstanceImpl(_In_ xaml_controls::InputValidationErrorEventAction action, _In_ xaml_controls::IInputValidationError* error, _Outptr_ xaml_controls::IInputValidationErrorEventArgs** instance)
    {
        ctl::ComPtr<InputValidationErrorEventArgs> errorArgs;
        IFCFAILFAST(ctl::make<InputValidationErrorEventArgs>(&errorArgs));
        IFCFAILFAST(errorArgs->put_Error(error));
        IFCFAILFAST(errorArgs->put_Action(action));
        *instance = errorArgs.Detach();
        return S_OK;
    }

    _Check_return_ HRESULT HasValidationErrorsChangedEventArgsFactory::CreateInstanceImpl(_In_ BOOLEAN newValue, _Outptr_ xaml_controls::IHasValidationErrorsChangedEventArgs** instance)
    {
        ctl::ComPtr<HasValidationErrorsChangedEventArgs> eventArgs;
        IFCFAILFAST(ctl::make<HasValidationErrorsChangedEventArgs>(&eventArgs));
        IFCFAILFAST(eventArgs->put_NewValue(newValue));
        *instance = eventArgs.Detach();
        return S_OK;
    }

    _Check_return_ HRESULT InputValidationCommand::CanValidateCoreImpl(_In_ xaml_controls::IInputValidationControl* validationControl, _Out_ BOOLEAN* returnValue)
    {
        // If the control has validation errors, try to invoke the command to re-validate the value of the input
        // property. This allows continual validation after the user has inputted a mistake.
        boolean canValidate = TRUE;
        IFC_RETURN(validationControl->get_HasValidationErrors(&canValidate));

        xaml_controls::InputValidationMode mode = xaml_controls::InputValidationMode_Auto;
        IFC_RETURN(get_InputValidationMode(&mode));
        if (mode == xaml_controls::InputValidationMode_Auto)
        {
            IFC_RETURN(validationControl->get_InputValidationMode(&mode));
            if (mode == xaml_controls::InputValidationMode_Disabled)
            {
                // If disabled, we don't validate, even if we have errors.
                canValidate = FALSE;
            }
        }

        *returnValue = canValidate;
        return S_OK;
    }

    _Check_return_ HRESULT InputValidationCommand::ValidateCoreImpl(_In_ xaml_controls::IInputValidationControl* validationControl)
    {
        UNREFERENCED_PARAMETER(validationControl);
        return S_OK;
    }

    _Check_return_ HRESULT InputValidationContextFactory::CreateInstanceImpl(_In_ HSTRING memberName, _In_ BOOLEAN isRequired, _In_opt_ IInspectable* outer, _Outptr_ IInspectable** inner, _Outptr_ xaml_controls::IInputValidationContext** instance)
    {
        ctl::ComPtr<xaml_controls::IInputValidationContext> context;
        IFC_RETURN(ctl::AggregableActivationFactory<InputValidationContext>::ActivateInstance(outer, inner));
        IFC_RETURN(ctl::do_query_interface(context, *inner));

        IFCFAILFAST(ctl::impl_cast<InputValidationContext>(context.Get())->put_IsInputRequired(isRequired));
        IFCFAILFAST(ctl::impl_cast<InputValidationContext>(context.Get())->put_MemberName(memberName));
        *instance = context.Detach();
        return S_OK;
    }
}