// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CApplication;

//------------------------------------------------------------------------
//
//  Class:  CDeployment
//
//  Synopsis:
//
//
//------------------------------------------------------------------------

class CDeployment final : public CDependencyObject
{
private:
    CDeployment(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
        m_requiresThreadSafeAddRefRelease = true;
    }

protected:
    ~CDeployment() override;

public:
    // Creation method
    DECLARE_CREATE(CDeployment);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDeployment>::Index;
    }

    // CDeployment methods
    _Check_return_ HRESULT JupiterComplete(_In_ CApplication* pApplication);
    _Check_return_ HRESULT StartupEventComplete();
    _Check_return_ HRESULT SetCurrentApplication(_In_ CApplication *pApplication);

    void Exit();

    CApplication* GetApplicationObjectForResourceLookup();
    _Check_return_ HRESULT SetTempApplicationObjectForResourceLookup(_In_ CApplication* pApp);

public:
    CApplication* m_pApplication = nullptr;
    CUIElement* m_pTempRootVisual = nullptr;
    CScrollContentControl* m_pTempRootScrollViewer = nullptr;
    CContentPresenter* m_pTempRootContentPresenter = nullptr;

private:
    CApplication* m_pTempApplicationForResourceLookup = nullptr;
};
