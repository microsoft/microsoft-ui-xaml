// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PageStackEntry.g.h"
#include "Page.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


_Check_return_ HRESULT
PageStackEntryFactory::CreateInstanceImpl(
    _In_ wxaml_interop::TypeName sourcePageType, 
    _In_ IInspectable* parameter, 
    _In_ xaml_animation::INavigationTransitionInfo* transitionInfo, 
    _Outptr_ xaml::Navigation::IPageStackEntry** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PageStackEntry> pPageStackEntry;
    xruntime_string_ptr strDescriptor;
    const CClassInfo* pType = nullptr;

    IFC(ctl::make<PageStackEntry>(&pPageStackEntry));

    IFC(pPageStackEntry->put_SourcePageType(sourcePageType));
    IFC(MetadataAPI::GetClassInfoByTypeName(sourcePageType, &pType));
    IFC(pType->GetFullName().Promote(&strDescriptor));
    IFC(pPageStackEntry->SetDescriptor(strDescriptor.GetHSTRING())); 
   
    IFC(pPageStackEntry->put_Parameter(parameter));
    IFC(pPageStackEntry->put_NavigationTransitionInfo(transitionInfo));

    IFC(pPageStackEntry.CopyTo(ppInstance)); 

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntry::Create(
    _In_ xaml_controls::IFrame *pIFrame,
    _In_ HSTRING descriptor,
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo* pTransitionInfo, 
    _Outptr_ PageStackEntry **ppPageStackEntry)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PageStackEntry> spPageStackEntry;
    wxaml_interop::TypeName sourcePageType = {};

    *ppPageStackEntry = NULL;

    IFC(ctl::make(&spPageStackEntry));

    spPageStackEntry->SetFrame(pIFrame);
    IFC(spPageStackEntry->put_Parameter(pParameterIInspectable));
    IFC(spPageStackEntry->put_NavigationTransitionInfo(pTransitionInfo));

    IFC(spPageStackEntry->SetDescriptor(descriptor));
    IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(descriptor), &sourcePageType));
    IFC(spPageStackEntry->put_SourcePageType(sourcePageType));

    *ppPageStackEntry = spPageStackEntry.Detach();

Cleanup:
    DELETE_STRING(sourcePageType.Name);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: PrepareContent
//
//  Synopsis: 
//     Set the frame on the page that is being navigated to.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntry::PrepareContent(
    _In_ IInspectable *pContentIInspectable)
{
    HRESULT hr = S_OK;
    Page *pPage = NULL;
    IPage *pIPage = NULL;
    xaml_controls::IFrame *pIFrame = NULL;

    // Query for IPage from IInspectable(Content).
    pIPage = ctl::query_interface<IPage>(pContentIInspectable);
    IFCPTR(pIPage);    
    pPage = static_cast<Page *>(pIPage);
        
    IFC(GetFrame(&pIFrame));
    // PrepareContent is called while navigating to a page and frame should never be null at this point.
    // So ensure, it is not NULL.
    IFCPTR(pIFrame);    

    // Set page's frame (Page.Frame). 
    // Frame holds a strong ref on the page using Frame.Content, and page holds a strong ref on the 
    // frame using Page.Frame. So there is a ref cycle. The cycle is broken when frame's content is 
    // changed/cleared, when a new page is set as content or during visual tree tear down.  
    // Then the frame releases the ref on the old page, which should be the last ref on the page, 
    // unless the page is cached. When the page is released, it releases the ref on the frame. 
    IFC(pPage->put_Frame(pIFrame));
    IFC(pPage->SetDescriptor(m_descriptor.Get()));

Cleanup:
    ReleaseInterface(pIPage);
    ReleaseInterface(pIFrame);
    RRETURN(hr);

}

_Check_return_ HRESULT
PageStackEntry::GetDescriptor(
    _Out_ HSTRING *pDescriptor)
{
    HRESULT hr = S_OK;

    *pDescriptor = NULL;

    IFCPTR(m_descriptor);

    IFC(m_descriptor.CopyTo(pDescriptor));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntry::SetDescriptor(
    _In_ HSTRING descriptor)
{
    HRESULT hr = S_OK;
    
    IFC(m_descriptor.Set(descriptor));
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntry::SetFrame(
    _In_ xaml_controls::IFrame* pIFrame)
{
    HRESULT hr = S_OK;

    m_wrFrame = nullptr;
    if(pIFrame)
    {
        IFC(ctl::AsWeak(pIFrame, &m_wrFrame));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntry::GetFrame(
    _Outptr_ xaml_controls::IFrame** ppIFrame)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IFrame> spFrame;    

    *ppIFrame = NULL;
    
    IFC(m_wrFrame.As(&spFrame));    
    IFC(spFrame.CopyTo(ppIFrame));

Cleanup:
    RRETURN(hr);    
}

//------------------------------------------------------------------------
//
//  Method: CanBeAddedToFrame
//
//  Synopsis: 
//     Validate whether this PageStackEntry can be added to the pIFrame.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntry::CanBeAddedToFrame(
    _In_ xaml_controls::IFrame* pIFrame, 
    _Out_ BOOLEAN* pCanAdd)
{
    HRESULT hr = S_OK;
    xaml_controls::IFrame* pFrame = NULL;
    
    *pCanAdd = FALSE;
    
    IFC(GetFrame(&pFrame));

    // The PageStackEntry can be added to the frame only if its frame is null or 
    // is equal to the frame we are trying to add this to.
    if(pFrame == NULL || pIFrame == pFrame)
    {
        *pCanAdd = TRUE;
    }

Cleanup:
    ReleaseInterface(pFrame);
    RRETURN(hr);    
}

