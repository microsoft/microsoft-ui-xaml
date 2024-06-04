// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Windows { namespace UI { namespace Core {

MIDL_INTERFACE("e7118f2d-771b-436f-b5d1-ebfa7651f45f")
INavigationFocusEventArgs2 : public IInspectable
{
public:
    IFACEMETHOD(get_CorrelationId)(_Out_ GUID* pValue) = 0;
};

} } } XAML_ABI_NAMESPACE_END

