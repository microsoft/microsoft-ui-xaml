// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CAutomationPeer;
class CAPWrapperValidator;
class CFrameworkElement;
class CUIElement;

class CFrameworkElementAutomationPeer : public CAutomationPeer
{
public:
    // Destructor
    ~CFrameworkElementAutomationPeer() override;

    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate)
    {
        HRESULT hr = S_OK;
        CFrameworkElementAutomationPeer *pObject = NULL;

        IFCEXPECT(pCreate);

        if (pCreate->m_value.GetType() != valueObject)
        {
            IFC(E_NOTIMPL);
        }
        else
        {
            pObject = new CFrameworkElementAutomationPeer(pCreate->m_pCore, pCreate->m_value);
            IFC(ValidateAndInit(pObject, ppObject));

            // On success we've transferred ownership
            pObject = NULL;
        }

    Cleanup:
        delete pObject;
        RRETURN(hr);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFrameworkElementAutomationPeer>::Index;
    }

    CAutomationPeer* GetAPParent() final;

// Interface to override
    XINT32 GetRootChildrenCore(CAutomationPeer ***pChildrenAP) final;
    HRESULT HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal) override;
    HRESULT IsEnabledHelper(_Out_ BOOLEAN* pRetVal) override;
    HRESULT IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal) override;
    HRESULT IsOffscreenHelper(bool ignoreClippingOnScrollContentPresenters, _Out_ BOOLEAN* pRetVal) override;
    HRESULT SetFocusHelper() override;
    HRESULT ShowContextMenuHelper() override;
    HRESULT GetCultureHelper(_Out_ int* returnValue) override;

protected:
    CFrameworkElementAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value);
    _Check_return_ HRESULT IsEnabledForFocus(_Out_ BOOLEAN* result);
};
