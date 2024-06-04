// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <cdeployment.h>

CDeployment::~CDeployment()
{
    ReleaseInterface(m_pApplication);

    ReleaseInterface(m_pTempRootVisual);
    ReleaseInterface(m_pTempRootScrollViewer);
    ReleaseInterface(m_pTempRootContentPresenter);

    ReleaseInterface(m_pTempApplicationForResourceLookup);
}

_Check_return_ HRESULT CDeployment::SetCurrentApplication(_In_ CApplication *pApplication)
{
    AddRefInterface(pApplication);
    ReleaseInterface(m_pApplication);
    m_pApplication = pApplication;

    return S_OK;
}

// Used by Jupiter Native code to fire the startup event.
// TODO: Remove code duplication between this function and CDeployment::Complete
_Check_return_ HRESULT
CDeployment::JupiterComplete(_In_ CApplication* pApplication)
{
    CUIElement* pRootVisual = NULL;
    EnterParams params(
        /*isLive*/                TRUE,
        /*skipNameRegistration*/  TRUE,
        /*coercedIsEnabled*/      TRUE,
        /*useLayoutRounding*/     EnterParams::UseLayoutRoundingDefault,
        /*visualTree*/            pApplication->GetContext()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot()->GetVisualTreeNoRef()
    );

    // pApplication->m_pRootVisual could be non-NULL if the root visual was set through XAML.
    // Temporarily remove it, since we don't want to enter it until after the Startup Event has happened.
    pRootVisual = pApplication->m_pRootVisual;
    pApplication->m_pRootVisual = NULL;

    // Finally, Enter the tree
    IFC_RETURN(pApplication->Enter(pApplication, params) );

    // Place any pre-existing RootVisual back on the Application.
    pApplication->m_pRootVisual = pRootVisual;

    if (m_pTempRootVisual)
    {
        // If the m_pTempRootVisual field is non-NULL, that means managed code set the root
        // visual from the application's constructor. Update the CApplication with this
        // root visual.
        ReleaseInterface(pApplication->m_pRootVisual);
        pApplication->m_pRootVisual = m_pTempRootVisual;
        m_pTempRootVisual = NULL;
    }

    // Fire the startup event.
    pApplication->FireStartupEvent();

    m_pApplication = pApplication;
    AddRefInterface(m_pApplication);

    return S_OK;
}

_Check_return_ HRESULT
CDeployment::StartupEventComplete()
{
    RRETURN(GetContext()->StartApplication(m_pApplication));
}

void
CDeployment::Exit()
{
    if (m_pApplication)
    {
        m_pApplication->FireExitEvent();
    }
}

// Called by the parser to get a CApplication object to use
// for resource lookup.
//
// This function should be used instead of accessing the
// CDeployment::m_pApplication field directly. During app startup,
// when creating the application object that field won't be set,
// but application-level resources may still be required.
CApplication* CDeployment::GetApplicationObjectForResourceLookup()
{
    return m_pApplication ? m_pApplication : m_pTempApplicationForResourceLookup;
}

// Called by the application object if application-level resources
// are set while an application object is being created.
_Check_return_ HRESULT CDeployment::SetTempApplicationObjectForResourceLookup(_In_ CApplication* pApp)
{
    IFCEXPECT_ASSERT_RETURN(!m_pApplication);

    m_pTempApplicationForResourceLookup = pApp;
    AddRefInterface(m_pTempApplicationForResourceLookup);

    return S_OK;
}



