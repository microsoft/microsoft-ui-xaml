// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Derivation of ICommand that relays Execute and CanExecute
//      to provided events, or to a child ICommand.

#pragma once

#include "XamlUICommand.g.h"
#include "CommandingContainer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(XamlUICommand)
    {
        public:
            _Check_return_ HRESULT CanExecuteImpl(_In_opt_ IInspectable* parameter, _Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT ExecuteImpl(_In_opt_ IInspectable* parameter);
            _Check_return_ HRESULT NotifyCanExecuteChangedImpl();

        protected:
            ~XamlUICommand() override { }

        private:
            _Check_return_ HRESULT GetFocusedCommandingContainer(_Outptr_result_maybenull_ CommandingContainer** commandingContainer);
    };
}
