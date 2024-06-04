// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class TimePickedEventArgs :
        public TimePickedEventArgsGenerated
    {
    public:
        TimePickedEventArgs() {}

    private:
        ~TimePickedEventArgs() {}

        _Check_return_ HRESULT InitializeImpl() override
        {
            HRESULT hr = S_OK;

            wrl::ComPtr<xaml::IDependencyObjectFactory> spInnerFactory;
            wrl::ComPtr<xaml::IDependencyObject> spDelegatingInner;
            wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;

            IFC(TimePickedEventArgsGenerated::InitializeImpl());

            IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyObject).Get(),
                &spInnerFactory));

            IFC(spInnerFactory->CreateInstance(
                static_cast<ITimePickedEventArgs*>(this),
                &spNonDelegatingInnerInspectable,
                &spDelegatingInner));

            IFC(SetComposableBasePointers(
                spNonDelegatingInnerInspectable.Get(),
                spInnerFactory.Get()));

        Cleanup:
            return hr;
        }
    };

    ActivatableClass(TimePickedEventArgs);

}}}} XAML_ABI_NAMESPACE_END
