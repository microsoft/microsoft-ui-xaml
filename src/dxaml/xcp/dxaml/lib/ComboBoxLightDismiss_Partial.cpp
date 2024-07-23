// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBoxLightDismiss.g.h"
#include "ComboBoxLightDismissAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create ComboBoxLightDismissAutomationPeer to represent the ComboBoxLightDismiss.
IFACEMETHODIMP ComboBoxLightDismiss::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
   HRESULT hr = S_OK;

   ctl::ComPtr<ComboBoxLightDismissAutomationPeer> spAutomationPeer;
   IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::ComboBoxLightDismissAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
   IFC(spAutomationPeer->put_Owner(this));
   *ppAutomationPeer = spAutomationPeer.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxLightDismiss::put_Owner(_In_ xaml_controls::IComboBox* pOwner)
{
    RRETURN(ctl::AsWeak(pOwner, &m_wrComboBox));
}

HRESULT ComboBoxLightDismiss::AutomationClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IComboBox> spComboBox;
    IFC(m_wrComboBox.As(&spComboBox));
    IFCPTR(spComboBox);

    // Invoke through UIAutomation
    IFC(spComboBox->put_IsDropDownOpen(FALSE));

    // Once LightDismiss is executed we no longer need this AP, hence we can get rid of it right away. This
    // leads clean navigation via Screen readers. A lot of them depends upon cached elements. When we collapse
    // using LightDismiss, Narrator Focus is still on cached LightDismiss, Resetting here makes sure LisghtDismiss
    // AP no longer exist and hence narrator updates its cache.
    ResetAutomationPeer();

Cleanup:
    RRETURN(hr);
}
