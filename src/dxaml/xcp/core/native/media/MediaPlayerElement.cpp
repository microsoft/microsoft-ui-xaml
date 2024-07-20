// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MediaPlayerElement.h"
#include "MediaPlayerPresenter.h"
#include "theming\inc\Theme.h"

// Headers for calling into MediaPlayerElement DXaml peer
#include "comInstantiation.h"
#include "comTemplateLibrary.h"
#include "Microsoft.UI.Xaml.h"
#include "Microsoft.UI.Xaml.private.h"
namespace DirectUI {
    class XamlServiceProviderContext;
}
#include "DependencyObject.h"
#include "MediaPlayerElement.g.h"

bool CMediaPlayerElement::HasTemplateChild()
{
    if ( m_isFullWindow && m_bTemplateApplied )
    {
        // In full window mode, the elements created by the template
        // were moved to the full window root.
        // So, the core does not find the template
        // and it tries to recreate it again.
        // We need to tell the core that we have already created our child elements from the template
        return true;
    }

    return CControl::HasTemplateChild();
}

_Check_return_ HRESULT CMediaPlayerElement::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    IFC_RETURN(CControl::NotifyThemeChangedCore(theme, fForceRefresh));

    // Forward theme value to FullWindowMediaRoot's RequestedTheme
    CFullWindowMediaRoot* pFullWindowMediaRootNoRef = GetContext()->GetMainFullWindowMediaRoot();
    if (pFullWindowMediaRootNoRef)
    {
        CValue themeValue;
        themeValue.Set(Theming::GetBaseValue(GetTheme()), KnownTypeIndex::ElementTheme);
        IFC_RETURN(pFullWindowMediaRootNoRef->SetValueByIndex(KnownPropertyIndex::FrameworkElement_RequestedTheme, themeValue));
    }

    return S_OK;
}

void CMediaPlayerElement::SetPresenter(_In_opt_ CMediaPlayerPresenter* pPresenter)
{
    m_wpPresenter = xref::weakref_ptr<CMediaPlayerPresenter>(pPresenter);
    if(pPresenter)
    {
        pPresenter->SetOwner(this);
    }
}

_Check_return_ HRESULT CMediaPlayerElement::MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize)
{
    CMediaPlayerPresenter *pPresenter = m_wpPresenter.lock();
    if (pPresenter && !m_isFullWindow)
    {
        pPresenter->PreMeasure(availableSize, desiredSize);
        if (desiredSize.height > 0 && desiredSize.width > 0)
        {
            // If the presenter wants to have a non zero size, then
            // update the available size to the size requested by the presenter
            availableSize = desiredSize;
        }
    }

    IFC_RETURN(__super::MeasureOverride(availableSize, desiredSize));

    return S_OK;
}

void CMediaPlayerElement::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    DirectUI::MediaPlayerElement* mediaPlayerElement = static_cast<DirectUI::MediaPlayerElement*>(GetDXamlPeer());
    if (mediaPlayerElement)
    {
        mediaPlayerElement->CleanupDeviceRelatedResources(cleanupDComp);
    }
    CControl::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
}
