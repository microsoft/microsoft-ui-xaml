// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CImage;

//
// Summary:
//      Image automation peer for accessibility.
//

class CImageAutomationPeer final : public CFrameworkElementAutomationPeer
{

public:
    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CImageAutomationPeer>::Index;
    }

    CImage* GetImage() { return m_pImage; }

private:
    CImage* m_pImage;

protected:
    CImageAutomationPeer(
        _In_ CCoreServices *pCore,
        _In_ CValue        &value);

    ~CImageAutomationPeer() override;
};
