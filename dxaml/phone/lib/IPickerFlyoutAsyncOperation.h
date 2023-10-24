// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls {

    template<typename TResult>
    struct IPickerFlyoutAsyncOperation
        : public wf::IAsyncOperation<TResult>
    {
    public:
        virtual _Check_return_ HRESULT StartOperation(_In_ xaml_primitives::IFlyoutBase* pAssociatedFlyout) = 0;
        virtual _Check_return_ HRESULT CompleteOperation(_In_ TResult result) = 0;
    };


}}}} XAML_ABI_NAMESPACE_END
