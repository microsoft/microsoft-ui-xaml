// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Manages the sequence of navigated content and the navigations
//      between them. Serializes and deserializes the navigation entries
//      using Windows storage.
//  Notes:
//      Navigations are split into requests and commits because they can
//      be canceled synchronously, but are carried out asynchronously
//      because components are loaded asynchronously. Commits are atomic;
//      any intermediate error rolls back all changes.

#include "precomp.h"
#include "NavigationHistory.h"
#include "Frame.g.h"
#include "NavigationTransitionInfo.g.h"
#include "NavigationHelpers.h"
#include "PageStackEntry.g.h"
#include "PageStackEntryTrackerCollection.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

static const UINT32 c_versionNumber = 1;

namespace N = xaml::Navigation;

NavigationHistory::NavigationHistory() :
    m_pIFrame(NULL),
    m_isNavigationPending(FALSE),
    m_isSetNavigationStatePending(FALSE),
    m_navigationMode(N::NavigationMode_New)
{
}

NavigationHistory::~NavigationHistory()
{
    DeInit();
    m_pIFrame = NULL;
}

void NavigationHistory::DeInit()
{
    m_isNavigationPending = FALSE;
    m_isSetNavigationStatePending = FALSE;
    m_navigationMode = N::NavigationMode_New;
}

_Check_return_ HRESULT
NavigationHistory::ClearNavigationHistory()
{
    HRESULT hr = S_OK;

    // Clear the frame pointer on entries that are being removed from the
    // PageStack (BackStack or ForwardStack)
    IFC(ResetPageStackEntries(TRUE /* isBackStack */));
    IFC(ResetPageStackEntries(FALSE /* isBackStack */));

    IFC(m_tpBackStack->ClearInternal());
    IFC(m_tpForwardStack->ClearInternal());

    m_tpCurrentPageStackEntry.Clear();
    m_tpPendingPageStackEntry.Clear();

    DeInit();

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
NavigationHistory::Create(
    _In_ xaml_controls::IFrame *pIFrame,
    _Outptr_ NavigationHistory **ppNavigationHistory)
{
    HRESULT hr = S_OK;
    NavigationHistory *pNavigationHistory = NULL;

    ctl::ComPtr<PageStackEntryTrackerCollection> spBackStack;
    ctl::ComPtr<PageStackEntryTrackerCollection> spForwardStack;

    *ppNavigationHistory = NULL;

    IFC(ctl::ComObject<NavigationHistory>::CreateInstance(&pNavigationHistory));

    pNavigationHistory->m_pIFrame = pIFrame;

    IFC(ctl::make(&spBackStack));

    IFC(spBackStack->Init(pNavigationHistory, TRUE /* isBackStack */));
    pNavigationHistory->SetPtrValue(pNavigationHistory->m_tpBackStack, spBackStack);

    IFC(ctl::make(&spForwardStack));

    IFC(spForwardStack->Init(pNavigationHistory, FALSE /* isBackStack */));
    pNavigationHistory->SetPtrValue(pNavigationHistory->m_tpForwardStack, spForwardStack);

    *ppNavigationHistory = pNavigationHistory;
    pNavigationHistory = NULL;

Cleanup:
    ctl::release_interface( pNavigationHistory );
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::NavigatePrevious()
{
    HRESULT hr = S_OK;
    xaml::Navigation::IPageStackEntry *pIEntry = NULL;
    UINT nCount = 0;

    IFC(m_tpBackStack->get_Size(&nCount));
    IFCCHECK(nCount >= 1);

    m_isNavigationPending = TRUE;
    m_navigationMode = N::NavigationMode_Back;

    IFC(m_tpBackStack->GetAtEnd(&pIEntry));
    SetPtrValue(m_tpPendingPageStackEntry, static_cast<PageStackEntry*>(pIEntry));

Cleanup:
    ReleaseInterface( pIEntry );
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::NavigateNext()
{
    HRESULT hr = S_OK;
    xaml::Navigation::IPageStackEntry *pIEntry = NULL;
    UINT nCount = 0;

    IFC(m_tpForwardStack->get_Size(&nCount));
    IFCCHECK(nCount >= 1);

    m_isNavigationPending = TRUE;
    m_navigationMode = N::NavigationMode_Forward;

    IFC(m_tpForwardStack->GetAtEnd(&pIEntry));
    SetPtrValue(m_tpPendingPageStackEntry, static_cast<PageStackEntry*>(pIEntry));

Cleanup:
    ReleaseInterface( pIEntry );
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::NavigateNew(
    _In_ HSTRING newDescriptor,
    _In_opt_ IInspectable *pParameter,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo)
{
    HRESULT hr = S_OK;
    PageStackEntry *pEntry = NULL;

    m_navigationMode = N::NavigationMode_New;

    m_tpPendingPageStackEntry.Clear();

    IFC(PageStackEntry::Create(m_pIFrame, newDescriptor, pParameter, pTransitionInfo, &pEntry));
    IFCPTR(pEntry);
    SetPtrValue(m_tpPendingPageStackEntry, pEntry);

    m_isNavigationPending = TRUE;

Cleanup:
    ctl::release_interface( pEntry );
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::GetBackStack(
    _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue)
{
    HRESULT hr = S_OK;

    *pValue = NULL;
    IFCPTR(m_tpBackStack.Get());

    IFC(m_tpBackStack.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::GetForwardStack(
    _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue)
{
    HRESULT hr = S_OK;

    *pValue = NULL;
    IFCPTR(m_tpForwardStack.Get());

    IFC(m_tpForwardStack.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::GetCurrentPageStackEntry(
    _Outptr_result_maybenull_ PageStackEntry **ppPageStackEntry)
{
    *ppPageStackEntry = m_tpCurrentPageStackEntry.Cast<PageStackEntry>();

    RRETURN(S_OK);
}

_Check_return_ HRESULT
NavigationHistory::GetPendingPageStackEntry(
    _Outptr_ PageStackEntry **ppPageStackEntry)
{
    HRESULT hr = S_OK;

    *ppPageStackEntry = NULL;

    IFCEXPECT(m_isNavigationPending);
    IFCPTR(m_tpPendingPageStackEntry.Get());

    *ppPageStackEntry = static_cast<PageStackEntry*>(m_tpPendingPageStackEntry.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::GetPendingNavigationMode(
    _Out_ xaml::Navigation::NavigationMode *pNavigationMode)
{
    HRESULT hr = S_OK;

    *pNavigationMode = N::NavigationMode_New;

    IFCEXPECT(m_isNavigationPending);

    *pNavigationMode = m_navigationMode;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::GetCurrentNavigationMode(
_Out_ xaml::Navigation::NavigationMode *pNavigationMode)
{
    *pNavigationMode = m_navigationMode;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
NavigationHistory::CommitNavigation()
{
    HRESULT hr = S_OK;
    Frame *pFrame = NULL;
    wrl_wrappers::HString strNewDescriptor;
    BOOLEAN wasChanged = FALSE;
    BOOLEAN newCanGoBack = FALSE;
    BOOLEAN oldCanGoBack = FALSE;
    BOOLEAN newCanGoForward = FALSE;
    BOOLEAN oldCanGoForward = FALSE;
    wxaml_interop::TypeName oldSourcePageType = {};
    wxaml_interop::TypeName newSourcePageType = {};
    UINT nCount = 0;

    IFCPTR(m_pIFrame);
    IFCEXPECT(m_isNavigationPending);
    IFCPTR(m_tpPendingPageStackEntry.Get());

    pFrame = static_cast<Frame *>(m_pIFrame);

    IFC(pFrame->get_CanGoBack(&oldCanGoBack));
    IFC(pFrame->get_CanGoForward(&oldCanGoForward));
    IFC(pFrame->get_CurrentSourcePageType(&oldSourcePageType));
    if (oldSourcePageType.Name)
    {
        const CClassInfo* pType = nullptr;

        // NOTE: The call seems to serve as a type name check only.
        IFC(MetadataAPI::GetClassInfoByTypeName(oldSourcePageType, &pType));
    }

    switch (m_navigationMode)
    {
        case N::NavigationMode_New:
            newCanGoBack = m_tpCurrentPageStackEntry.Get() != NULL;
            newCanGoForward = FALSE;
            break;

        case N::NavigationMode_Back:
            IFC(m_tpBackStack->get_Size(&nCount));
            IFCEXPECT(nCount >= 1);

            newCanGoBack = (nCount - 1) >= 1;
            newCanGoForward = TRUE;
            break;

        case N::NavigationMode_Forward:
            IFC(m_tpForwardStack->get_Size(&nCount));
            IFCEXPECT(nCount >= 1);

            newCanGoBack = TRUE;
            newCanGoForward = (nCount - 1) >= 1;
            break;

        default:
            IFC(E_UNEXPECTED);
    }

    IFC(m_tpPendingPageStackEntry.Cast<PageStackEntry>()->GetDescriptor(strNewDescriptor.GetAddressOf()));

    wasChanged = TRUE;

    IFC(pFrame->put_CanGoBack(newCanGoBack));
    IFC(pFrame->put_CanGoForward(newCanGoForward));
    IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(strNewDescriptor.Get()), &newSourcePageType));
    IFC(pFrame->put_SourcePageType(newSourcePageType));
    IFC(pFrame->put_CurrentSourcePageType(newSourcePageType));

    switch (m_navigationMode)
    {
        case N::NavigationMode_New:
            if (m_tpCurrentPageStackEntry)
            {
                IFC(m_tpBackStack->AppendInternal(static_cast<PageStackEntry*>(m_tpCurrentPageStackEntry.Get())));
            }

            m_isNavigationPending = FALSE;
            IFC(m_tpForwardStack->Clear());
            break;

        case N::NavigationMode_Back:
            if (m_tpCurrentPageStackEntry)
            {
                IFC(m_tpForwardStack->AppendInternal(static_cast<PageStackEntry*>(m_tpCurrentPageStackEntry.Get())));
            }

            IFC( m_tpBackStack->RemoveAtEndInternal() );
            break;

        case N::NavigationMode_Forward:
            if (m_tpCurrentPageStackEntry)
            {
                IFC(m_tpBackStack->AppendInternal(static_cast<PageStackEntry*>(m_tpCurrentPageStackEntry.Get())));
            }

            IFC( m_tpForwardStack->RemoveAtEndInternal() );
            break;
    }

    nCount = 0;
    IFC(m_tpBackStack->get_Size(&nCount));
    IFC(pFrame->put_BackStackDepth(nCount));

    SetPtrValue(m_tpCurrentPageStackEntry, m_tpPendingPageStackEntry.Get());

Cleanup:
    if (FAILED(hr) && wasChanged)
    {
        IGNOREHR(pFrame->put_CanGoBack(oldCanGoBack));
        IGNOREHR(pFrame->put_CanGoForward(oldCanGoForward));
        IGNOREHR(pFrame->put_SourcePageType(oldSourcePageType));
        IGNOREHR(pFrame->put_CurrentSourcePageType(oldSourcePageType));

        m_tpPendingPageStackEntry.Clear();
    }

    DELETE_STRING(oldSourcePageType.Name);
    DELETE_STRING(newSourcePageType.Name);
    pFrame = NULL;
    m_isNavigationPending = FALSE;
    m_tpPendingPageStackEntry.Clear();

    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::CommitSetNavigationState(
    _In_ NavigationCache *pNavigationCache)
{
    HRESULT hr = S_OK;
    Frame *pFrame = NULL;
    BOOLEAN newCanGoBack = FALSE;
    BOOLEAN newCanGoForward = FALSE;
    wrl_wrappers::HString strDescriptior;
    wxaml_interop::TypeName sourcePageType = {};
    UINT nBackStackCount = 0;
    UINT nForwardStackCount = 0;

    IFCCHECK(m_isSetNavigationStatePending);

    pFrame = static_cast<Frame *>(m_pIFrame);

    // Enable/Disable GoBack & GoForward
    IFC(m_tpBackStack->get_Size(&nBackStackCount));
    newCanGoBack = (nBackStackCount >= 1);
    IFC(m_tpForwardStack->get_Size(&nForwardStackCount));
    newCanGoForward = (nForwardStackCount >= 1);
    IFC(pFrame->put_CanGoBack(newCanGoBack));
    IFC(pFrame->put_CanGoForward(newCanGoForward));

    // See source type in IFrame
    if (m_tpCurrentPageStackEntry)
    {
        IFC(m_tpCurrentPageStackEntry.Cast<PageStackEntry>()->GetDescriptor(strDescriptior.GetAddressOf()));
        IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(strDescriptior.Get()), &sourcePageType));
        IFC(pFrame->put_SourcePageType(sourcePageType));
        IFC(pFrame->put_CurrentSourcePageType(sourcePageType));
    }

    m_isSetNavigationStatePending = FALSE;

Cleanup:
    DELETE_STRING(sourcePageType.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::ValidateCanChangePageStack()
{
    HRESULT hr = S_OK;

    // Make sure we are not in the middle of a navigation.
    if(m_isNavigationPending)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALID_OPERATION, ERROR_FRAME_NAVIGATING));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::ValidateCanInsertEntry(_In_ PageStackEntry* pEntry)
{

    HRESULT hr = S_OK;
    BOOLEAN canAdd = FALSE;

    // Make sure the entry being inserted is not NULL.
    ARG_NOTNULL(pEntry, "entry");

    // Make sure this PageStackEntry isn't already owned by another frame.
    IFC(pEntry->CanBeAddedToFrame(m_pIFrame, &canAdd));
    if(!canAdd)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALID_OPERATION, ERROR_PAGESTACK_ENTRY_OWNED));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationHistory::ValidateCanClearPageStack()
{
    HRESULT hr = S_OK;

    // Make sure we are not in the middle of a navigation.
    if(m_isNavigationPending)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALID_OPERATION, ERROR_FRAME_NAVIGATING));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: ResetPageStackEntries
//
//  Synopsis:
//     Clear the frame pointer on entries that are being removed from the
//     PageStack (BackStack or ForwardStack)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::ResetPageStackEntries(
    _In_ BOOLEAN isBackStack)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterator<xaml::Navigation::PageStackEntry*>> spIterator;
    BOOLEAN hasCurrent = FALSE;

    if (isBackStack)
    {
        IFC(m_tpBackStack->First(&spIterator));
    }
    else
    {
        IFC(m_tpForwardStack->First(&spIterator));
    }
    IFC(spIterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        ctl::ComPtr<xaml::Navigation::IPageStackEntry> spEntry;
        IFC(spIterator->get_Current(&spEntry));
        IFC(spEntry.Cast<PageStackEntry>()->SetFrame(NULL));
        IFC(spIterator->MoveNext(&hasCurrent));
    }
Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method: OnPageStackChanging
//
//  Synopsis:
//     Do the necessary validations and clear the frame pointer
//     on entries that are being removed from the
//     PageStack (BackStack or ForwardStack)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::OnPageStackChanging(
            _In_ BOOLEAN isBackStack,
            _In_ wfc::CollectionChange action,
            _In_ UINT index,
            _In_opt_ PageStackEntry* pEntry)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    Frame *pFrame = NULL;
    xaml::Navigation::IPageStackEntry *pIEntry = NULL;
    ctl::ComPtr<wfc::IVector<xaml::Navigation::PageStackEntry*>> spPageStack;

    IFCPTR(m_pIFrame);
    pFrame = static_cast<Frame *>(m_pIFrame);

    if (isBackStack)
    {
        IFC(pFrame->get_BackStack(&spPageStack));
    }
    else
    {
        IFC(pFrame->get_ForwardStack(&spPageStack));
    }

    IFC(spPageStack->get_Size(&nCount));

    // Do the necessary validations and clear the frame pointer on entries that are being removed.
    switch (action)
    {
        case wfc::CollectionChange_ItemChanged:
        {
            IFC(ValidateCanChangePageStack());
            IFC(ValidateCanInsertEntry(static_cast<PageStackEntry*>(pEntry)));
            IFC(spPageStack->GetAt(index, &pIEntry));
            PageStackEntry *pPageStackEntry = static_cast<PageStackEntry*>(pIEntry);
            IFC(pPageStackEntry->SetFrame(NULL));
            ReleaseInterface(pIEntry);
            break;
        }
        case wfc::CollectionChange_ItemInserted:
        {
            IFC(ValidateCanChangePageStack());
            IFC(ValidateCanInsertEntry(static_cast<PageStackEntry*>(pEntry)));
            break;
        }
        case wfc::CollectionChange_ItemRemoved:
        {
            IFC(ValidateCanChangePageStack());
            IFC(spPageStack->GetAt(index, &pIEntry));
            PageStackEntry *pPageStackEntry = static_cast<PageStackEntry*>(pIEntry);
            IFC(pPageStackEntry->SetFrame(NULL));
            ReleaseInterface(pIEntry);
            break;
        }
        case wfc::CollectionChange_Reset:
        {
            IFC(ValidateCanClearPageStack());
            IFC(ResetPageStackEntries(isBackStack));
            break;
        }
        default:
            IFCEXPECT_ASSERT(FALSE);
            break;
    }

Cleanup:
    pFrame = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: OnPageStackChanged
//
//  Synopsis:
//     Update the frame pointer on entries that were added to the
//     PageStack (BackStack or ForwardStack)and update the
//     CanGoBack, CanGoForward and BackStackDepth properties.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::OnPageStackChanged(
    _In_ BOOLEAN isBackStack,
    _In_ wfc::CollectionChange action,
    _In_ UINT index)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    Frame *pFrame = NULL;
    xaml::Navigation::IPageStackEntry *pIEntry = NULL;
    ctl::ComPtr<wfc::IVector<xaml::Navigation::PageStackEntry*>> spPageStack;

    IFCPTR(m_pIFrame);
    pFrame = static_cast<Frame *>(m_pIFrame);

    if (isBackStack)
    {
        IFC(pFrame->get_BackStack(&spPageStack));
    }
    else
    {
        IFC(pFrame->get_ForwardStack(&spPageStack));
    }

    IFC(spPageStack->get_Size(&nCount));

    // Update the frame pointer on entries that were added and update the CanGoBack, CanGoForward and BackStackDepth properties.
    switch (action)
    {
        case wfc::CollectionChange_ItemInserted:
        {
            IFCCHECK(nCount > index);
            IFC(spPageStack->GetAt(index, &pIEntry));
            PageStackEntry *pPageStackEntry = static_cast<PageStackEntry*>(pIEntry);
            IFC(pPageStackEntry->SetFrame(m_pIFrame));
            ReleaseInterface(pIEntry);
            if (isBackStack)
            {
                IFC(pFrame->put_CanGoBack(TRUE));
                IFC(pFrame->put_BackStackDepth(nCount));
            }
            else
            {
                IFC(pFrame->put_CanGoForward(TRUE));
            }
            break;
        }
        case wfc::CollectionChange_ItemRemoved:
        {
            if (isBackStack)
            {
                IFC(pFrame->put_CanGoBack(((nCount > 0) ? TRUE : FALSE)));
                IFC(pFrame->put_BackStackDepth(nCount));
            }
            else
            {
                IFC(pFrame->put_CanGoForward(((nCount > 0) ? TRUE : FALSE)));
            }
            break;
        }
        case wfc::CollectionChange_Reset:
        {
            if (isBackStack)
            {
                IFC(pFrame->put_CanGoBack(FALSE));
                IFC(pFrame->put_BackStackDepth(nCount));
            }
            else
            {
                IFC(pFrame->put_CanGoForward(FALSE));
            }
            break;
        }
        default:
            IFCEXPECT_ASSERT(FALSE);
            break;
    }

Cleanup:
    pFrame = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHistory::GetNavigationState
//
//  Synopsis:
//     Serialize navigation history into an HSTRING
//
//     Format:
//  <version number>,<number of pages>,<current page index>,
//     <page1's type Name length>,<page1 type name>,<page1's parameter type (wf::PropertyType)>,
//              <length of page1's serialized parameter>,<page1's serialized parameter>,
//     :
//     <pageN type name length>,<pageN type name>,<pageN's parameter type (wf::PropertyType)>,
//              <length of pageN's serialized parameter>,<pageN's serialized parameter>,
//
//    If a page does not have a parameter, the format is:
//     <pageN type name length>,<pageN type name>,<wf::PropertyType_Empty>,
//
//    Supported parameter types are string, char, numeric and guid.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
 NavigationHistory::GetNavigationState(
    _Out_ HSTRING* pNavigationState)
{
    string buffer;

    // Write version number, to handle format changes
    IFC_RETURN(NavigationHelpers::WriteUINT32ToString(c_versionNumber, buffer));

    // Supported parameter types
    //
    // None of the following values should ever change because they are
    // public. If any of these values ever changes, the version number
    // of the string returned by GetNavigationState will need to be changed and
    // back compat handled.
    ASSERT(wf::PropertyType_Empty  == 0);
    ASSERT(wf::PropertyType_UInt8  == 1);
    ASSERT(wf::PropertyType_Int16  == 2);
    ASSERT(wf::PropertyType_UInt16 == 3);
    ASSERT(wf::PropertyType_Int32  == 4);
    ASSERT(wf::PropertyType_UInt32 == 5);
    ASSERT(wf::PropertyType_Int64  == 6);
    ASSERT(wf::PropertyType_UInt64 == 7);
    ASSERT(wf::PropertyType_Single == 8);
    ASSERT(wf::PropertyType_Double == 9);
    ASSERT(wf::PropertyType_Char16 == 10);
    ASSERT(wf::PropertyType_Boolean == 11);
    ASSERT(wf::PropertyType_String == 12);
    ASSERT(wf::PropertyType_Guid   == 16);

    UINT32 nextSize = 0;
    UINT32 previousSize = 0;
    UINT32 totalSize = 0;

    // Get size of entries before and after the current entry, and the total size
    IFC_RETURN(m_tpBackStack->get_Size(&previousSize));
    IFC_RETURN(m_tpForwardStack->get_Size(&nextSize));

    if (m_tpCurrentPageStackEntry)
    {
        // Previous entries + Current entry + Next entries
        totalSize = previousSize + 1 + nextSize;
    }
    else if (previousSize > 0)
    {
        totalSize = previousSize + nextSize;
    }
    else
    {
        totalSize = 0;
    }

    // Write number of entries in history.
    IFC_RETURN(NavigationHelpers::WriteUINT32ToString(totalSize, buffer));

    if (totalSize > 0)
    {
        ctl::ComPtr<xaml::Navigation::IPageStackEntry> pIEntry;
        ctl::ComPtr<xaml::Navigation::IPageStackEntry> tempCurrentIEntry;

        // If the current page is NULL consider the top element in BackStack as current and don't add it to the serialized BackStack.
        if (!m_tpCurrentPageStackEntry)
        {
            ASSERT(previousSize > 0);

            IFC_RETURN(m_tpBackStack->GetAt(previousSize - 1, &tempCurrentIEntry));
            previousSize--;
        }

        // Write index of current entry
        IFC_RETURN(NavigationHelpers::WriteUINT32ToString(previousSize, buffer));

        // Write previous entries
        for (UINT32 i = 0; i < previousSize; ++i)
        {
            IFC_RETURN(m_tpBackStack->GetAt(i, &pIEntry));
            IFC_RETURN(WritePageStackEntryToString(pIEntry.Cast<PageStackEntry>(), buffer));
        }

        // Write current entry
        if (tempCurrentIEntry.Get())
        {
            IFC_RETURN(WritePageStackEntryToString(tempCurrentIEntry.Cast<PageStackEntry>(), buffer));
        }
        else
        {
            IFC_RETURN(WritePageStackEntryToString(m_tpCurrentPageStackEntry.Cast<PageStackEntry>(), buffer));
        }

        // Write subsequent entries
        for (UINT32 i = 0; i < nextSize; ++i)
        {
            IFC_RETURN(m_tpForwardStack->GetAt(i, &pIEntry));
            IFC_RETURN(WritePageStackEntryToString(pIEntry.Cast<PageStackEntry>(), buffer));
        }
    }

    size_t position;

    // Remove last ',' delimiter
    position = buffer.rfind(L',');
    buffer.erase(position, 1);

    // Return HSTRING with navigation state
    IFC_RETURN(::WindowsCreateString(buffer.c_str(), buffer.length(), pNavigationState));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: NavigationHistory::SetNavigationState
//
//  Synopsis:
//     Read navigation history from an HSTRING. Caller needs to
//  to then create the current page and call CommitSetNavigationState
//  to complete this operation.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::SetNavigationState(
    _In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate)
{
    UINT32 versionNumber = 0;
    UINT32 contentCount = 0;
    size_t currentPosition = 0;
    size_t nextPosition = string::npos;
    string buffer;

    buffer.append(HStringUtil::GetRawBuffer(navigationState, nullptr));

    // Read version number
    IFC_RETURN(NavigationHelpers::ReadUINT32FromString(buffer, currentPosition, &versionNumber, &nextPosition));
    currentPosition = nextPosition;
    IFCCHECK_RETURN(versionNumber == c_versionNumber);

    // Read number of entries in history. Previous entries + Current entry + Next entries
    IFC_RETURN(NavigationHelpers::ReadUINT32FromString(buffer, currentPosition, &contentCount, &nextPosition));
    currentPosition = nextPosition;

    // Clear Navigation history, because new history is going to be read
    IFC_RETURN(ClearNavigationHistory());

    if (contentCount > 0)
    {
        UINT32 nextSize = 0;
        UINT32 previousSize = 0;
        UINT32 contentIndex = 0;
        ctl::ComPtr<PageStackEntry> pPageStackEntry;

        // Read index of current entry
        IFC_RETURN(NavigationHelpers::ReadUINT32FromString(buffer, currentPosition, &contentIndex, &nextPosition));
        currentPosition = nextPosition;
        IFCCHECK_RETURN(contentIndex < contentCount);

        previousSize = contentIndex;
        nextSize = contentCount - previousSize - 1;

        // Read previous entries
        for (UINT32 i = 0; i < previousSize; ++i)
        {
            IFC_RETURN(ReadPageStackEntryFromString(buffer, currentPosition, &pPageStackEntry, &nextPosition));
            currentPosition = nextPosition;
            IFC_RETURN(m_tpBackStack->AppendInternal(pPageStackEntry.Get()));
        }

        // Read current entry
        IFC_RETURN(ReadPageStackEntryFromString(buffer, currentPosition, &pPageStackEntry, &nextPosition));
        currentPosition = nextPosition;

        if (suppressNavigate)
        {
            IFC_RETURN(m_tpBackStack->AppendInternal(pPageStackEntry.Get()));
            m_tpCurrentPageStackEntry.Clear();
        }
        else
        {
            SetPtrValue(m_tpCurrentPageStackEntry, pPageStackEntry.Get());
        }

        // Read next entries
        for (UINT32 i = 0; i < nextSize; ++i)
        {
           IFC_RETURN(ReadPageStackEntryFromString(buffer, currentPosition, &pPageStackEntry, &nextPosition));
           currentPosition = nextPosition;
           IFC_RETURN(m_tpForwardStack->AppendInternal(pPageStackEntry.Get()));
        }
    }

    UINT nCount = 0;

    IFCPTR_RETURN(m_pIFrame);

    // Navigation can be set without navigating to the current page so we need to update BackStackDepth here because CommitNavigation could not be called.
    m_tpBackStack->get_Size(&nCount);
    IFC_RETURN(static_cast<Frame *>(m_pIFrame)->put_BackStackDepth(nCount));

    m_isSetNavigationStatePending = !suppressNavigate;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: NavigationHistory::WritePageStackEntryToString
//
//  Synopsis:
//     Write a PageStackEntry to a string.
//     Format:
//      <page type name length>,<page type name>,<page's parameter type (wf::PropertyType)>,
//              <length of page's serialized parameter>,<page's serialized parameter>,
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::WritePageStackEntryToString(
    _In_ PageStackEntry *pPageStackEntry,
    _Inout_ string &buffer)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strDescriptor;
    xstring_ptr strTransitionInfoType;
    xruntime_string_ptr strTransitionInfoTypePromoted;
    wrl_wrappers::HString strTransitionInfo;
    IInspectable *pParameterIInspectable = NULL;
    ctl::ComPtr<NavigationTransitionInfo> spTransitionInfo;
    ctl::ComPtr<xaml_animation::INavigationTransitionInfo> spTransitionInfoAsI;
    BOOLEAN isParameterTypeSupported = FALSE;

    // Write descriptor
    IFC(pPageStackEntry->GetDescriptor(strDescriptor.GetAddressOf()));
    IFCPTR(strDescriptor.Get());
    IFC(NavigationHelpers::WriteHSTRINGToString(strDescriptor.Get(), buffer));

    // Write parameter
    IFC(pPageStackEntry->get_Parameter(&pParameterIInspectable));
    IFC(NavigationHelpers::WriteNavigationParameterToString(pParameterIInspectable,
            buffer, &isParameterTypeSupported));

    // Get the NavigationTransitionInfo.
    IFC(spTransitionInfo.As(&spTransitionInfoAsI));
    IFC(pPageStackEntry->get_NavigationTransitionInfo(&spTransitionInfoAsI));

    if (spTransitionInfo.Get())
    {
        // Write NavigationTransitionInfo type.
        IFC(MetadataAPI::GetRuntimeClassName(spTransitionInfoAsI.Get(), &strTransitionInfoType));
        IFC(strTransitionInfoType.Promote(&strTransitionInfoTypePromoted));
        IFC(NavigationHelpers::WriteHSTRINGToString(strTransitionInfoTypePromoted.GetHSTRING(), buffer));

        // Write NavigationTransitionInfo.
        IFC(spTransitionInfo->GetNavigationStateCoreProtected(strTransitionInfo.GetAddressOf()));
        IFC(NavigationHelpers::WriteHSTRINGToString(strTransitionInfo.Get(), buffer));
    }
    else
    {
        // Placeholder for type.
        IFC(NavigationHelpers::WriteHSTRINGToString(NULL, buffer));

        // Only write the serialization of the type if we need to.
    }

    if (!isParameterTypeSupported)
    {
        // Throw exception saying that a parameter type is not supported for
        // serialization
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,
                ERROR_NAVIGATION_UNSUPPORTED_PARAM_TYPE_FOR_SERIALIZATION));
    }

Cleanup:
    ReleaseInterface(pParameterIInspectable);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: NavigationHistory::ReadPageStackEntryFromString
//
//  Synopsis:
//     Read a PageStackEntry from a string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NavigationHistory::ReadPageStackEntryFromString(
    _In_ string &buffer,
    _In_ size_t currentPosition,
    _In_ PageStackEntry **ppPageStackEntry,
    _Out_ size_t *pNextPosition)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strDescriptor;
    wrl_wrappers::HString strTransitionInfoType;
    wrl_wrappers::HString strTransitionInfo;
    ctl::ComPtr<IInspectable> spParameterIInspectable;
    const CClassInfo* pTransitionInfoTypeInfo = nullptr;
    ctl::ComPtr<NavigationTransitionInfo> spTransitionInfo;

    // Read descriptor
    IFC(NavigationHelpers::ReadHSTRINGFromString(
        buffer,
        currentPosition,
        strDescriptor.GetAddressOf(),
        pNextPosition));
    currentPosition = *pNextPosition;
    IFCEXPECT(strDescriptor.Get());

    // Read parameter
    IFC(NavigationHelpers::ReadNavigationParameterFromString(buffer, currentPosition,
            &spParameterIInspectable, pNextPosition));
    currentPosition = *pNextPosition;

    // Create NavigationTransitionInfo
    hr = NavigationHelpers::ReadHSTRINGFromString(
        buffer,
        currentPosition,
        strTransitionInfoType.GetAddressOf(),
        pNextPosition);

    if(SUCCEEDED(hr))
    {
        currentPosition = *pNextPosition;

        if (strTransitionInfoType.Get())
        {
            IFC(MetadataAPI::GetClassInfoByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(strTransitionInfoType.Get()), &pTransitionInfoTypeInfo));
            IFC(ActivationAPI::ActivateInstance(pTransitionInfoTypeInfo, &spTransitionInfo));

            // Read NavigationTransitionInfo.
            IFC(NavigationHelpers::ReadHSTRINGFromString(
                buffer,
                currentPosition,
                strTransitionInfo.GetAddressOf(),
                pNextPosition));
            currentPosition = *pNextPosition;

            if (strTransitionInfo.Get() != NULL)
            {
                IFC(spTransitionInfo->SetNavigationStateCoreProtected(strTransitionInfo.Get()));
            }
        }
    }
    else
    {
        // Swallowing the failure, as some apps override the navigation state manually;
        // therefore, we cannot expect this value to be present in Pre-Blue apps.

        #ifdef DBG
        WCHAR szTrace[256];
        IFCEXPECT(swprintf_s(szTrace, 256, L"==== NavigationTransitionInfo not present while parsing navigation state.") >= 0);
        Trace(szTrace);
        #endif

        hr = S_OK;
    }

    // Create PageStackEntry
    IFC(PageStackEntry::Create(
        m_pIFrame,
        strDescriptor.Get(),
        spParameterIInspectable.Get(),
        spTransitionInfo.Get(),
        ppPageStackEntry));

Cleanup:
    RRETURN(hr);
}

