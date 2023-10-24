// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutomationProperty.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(AutomationProperty)
    {
        public:
            AutomationProperty()
            {
            }

            void SetAutomationPropertiesEnum(_In_ AutomationPropertiesEnum eProperty)
            {
                m_eProperty = eProperty;
            }

            void GetAutomationPropertiesEnum(_Out_ AutomationPropertiesEnum* pPropertiesEnum)
            {
                *pPropertiesEnum = m_eProperty;
            }
            
            static HRESULT EnsureProperty(ctl::ComPtr<xaml_automation::IAutomationProperty>& prop, DirectUI::AutomationPropertiesEnum propertyEnum)
            {
                if (prop == nullptr)
                {
                    IFC_RETURN(ctl::ComObject<DirectUI::AutomationProperty>::CreateInstance(prop.ReleaseAndGetAddressOf()));
                    static_cast<AutomationProperty*>(prop.Get())->SetAutomationPropertiesEnum(propertyEnum);
                }

                return S_OK;
            }

        private:
            DirectUI::AutomationPropertiesEnum m_eProperty;
    };
}
