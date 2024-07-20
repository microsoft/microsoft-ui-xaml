// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Flyout declaration.  Flyout manages content,
//      provides FlyoutPresenter and manages its Style.

#pragma once

#include "Flyout.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Flyout)
    {
    public:
        // Initializes a new instance.
        Flyout();

        // Destroys an instance.
        ~Flyout() override;

    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(CreatePresenter)(_Outptr_ xaml_controls::IControl** ppReturnValue) override;

        _Check_return_ HRESULT OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs) final;

    private:
    };
}
