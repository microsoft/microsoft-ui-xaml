// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    delegate void ExecuteDelegate(Platform::Object^ parameter);
    delegate bool CanExecuteDelegate(Platform::Object^ parameter);

    ref class KeyAcceleratorCommand sealed : public Microsoft::UI::Xaml::Input::ICommand
    {
        ExecuteDelegate^ _executeDelegate;

    public:
        KeyAcceleratorCommand(ExecuteDelegate^ executeDelegate, bool canExecuteFlag, Platform::String^ expectedParam)
        {
            _executeDelegate = executeDelegate;
            CanExecuteFlag = canExecuteFlag;
            m_expectedParam = expectedParam;
        }

        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;

        virtual void Execute(Platform::Object^ parameter)
        {
            VERIFY_IS_TRUE(m_expectedParam == parameter->ToString());

            _executeDelegate(parameter);
        }

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            return CanExecuteFlag;
        }

        property bool CanExecuteFlag;

    private:
        Platform::String^ m_expectedParam;
    };

} } } } }
