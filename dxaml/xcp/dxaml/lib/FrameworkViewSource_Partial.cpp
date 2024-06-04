// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkViewSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP FrameworkViewSource::CreateView(_Outptr_ wac::IFrameworkView** ppFrameworkView)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HStringReference frameworkViewAcid(RuntimeClass_Microsoft_UI_Xaml_FrameworkView);

    IFC(wf::ActivateInstance(frameworkViewAcid.Get(), ppFrameworkView));

Cleanup:
    RRETURN(hr);
}

