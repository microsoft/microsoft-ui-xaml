// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TryStartConnectedAnimationOperation.h"
#include "ConnectedAnimationService.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

ULONG TryStartConnectedAnimationOperation::z_ulUniqueAsyncActionId = 1;

TryStartConnectedAnimationOperation::TryStartConnectedAnimationOperation()
{ 
}

// elements needed to register for events and then start the animation.
_Check_return_ HRESULT TryStartConnectedAnimationOperation::InitAndStart(_In_ ListViewBase* listview, _In_ xaml_animation::IConnectedAnimation* animation, _In_ IInspectable* item, _In_ HSTRING elementName)
{
    m_listview = listview;
    m_item = item;
    m_animation = animation;
    m_elementName = elementName;
    Start();
    return S_OK;
}

_Check_return_ HRESULT TryStartConnectedAnimationOperation::Clear()
{
    IFC_RETURN(m_listview->remove_ContainerContentChanging(m_eventToken));
    m_eventToken = {};
    m_listview = nullptr;
    m_item = nullptr;
    m_animation = nullptr;
    return S_OK;
}

_Check_return_ HRESULT TryStartConnectedAnimationOperation::OnStart()
{
    auto callback = wrl::Callback<wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>>(
        this,
        &TryStartConnectedAnimationOperation::OnContainerContentChanging);


    ctl::ComPtr<xaml::IUIElement> animationElement;
    {
        HRESULT hr = GetAnimationElement(&animationElement);
        if (hr == E_INVALIDARG)
        {
            // We don't have the element yet, but we might get it as the listview generates containers.
            IFC_RETURN(m_listview->add_ContainerContentChanging(callback.Get(), &m_eventToken));
            return S_OK;
        }
        IFC_RETURN(hr);
    }
    
    // We already have the element so go ahead and try to start.
    IFC_RETURN(m_animation->TryStart(animationElement.Get(), &m_result));
    AsyncBase::TryTransitionToCompleted();
    IFC_RETURN(Clear());
    AsyncBase::FireCompletion();
    
    // $TODO:  Add timer call back to remove the item if we never get OnContainerContentChanging.

    return S_OK;
}

_Check_return_ HRESULT TryStartConnectedAnimationOperation::OnContainerContentChanging(_In_ xaml_controls::IListViewBase* listview, _In_ xaml_controls::IContainerContentChangingEventArgs* args)
{
    // Only process if this change is for the item we are looking for.
    ctl::ComPtr<IInspectable> item;
    IFC_RETURN(args->get_Item(&item));
    if (item != m_item) return S_OK;

    ctl::ComPtr<xaml::IUIElement> animationElement;
    HRESULT hr = GetAnimationElement(&animationElement);
    if (FAILED(hr))
    {
        AsyncBase::TryTransitionToError(hr);
    }
    else
    {
        IFC_RETURN(m_animation->TryStart(animationElement.Get(), &m_result));
        AsyncBase::TryTransitionToCompleted();
    }

    IFC_RETURN(Clear());
    AsyncBase::FireCompletion();
    return S_OK;
}

void TryStartConnectedAnimationOperation::OnCancel()
{
    VERIFYHR(Clear());
}

void TryStartConnectedAnimationOperation::OnClose()
{
    VERIFYHR(Clear());
}

STDMETHODIMP TryStartConnectedAnimationOperation::GetResults(
    _Inout_ boolean *result)
{
    *result = m_result;
    return S_OK;
}

_Check_return_ HRESULT TryStartConnectedAnimationOperation::GetAnimationElement(_Outptr_ xaml::IUIElement ** animationElement)
{
    ctl::ComPtr<xaml_controls::IContentControl> containerElement;
    ctl::ComPtr<xaml::IDependencyObject> object;
    IFC_RETURN(m_listview->ContainerFromItem(m_item.Get(), &object));
    IFC_RETURN(object.As(&containerElement));

    if (containerElement != nullptr)
    {
        ctl::ComPtr<xaml::IUIElement> contentElement;
        IFC_RETURN(containerElement->get_ContentTemplateRoot(&contentElement));

        if (contentElement != nullptr)
        {
            ctl::ComPtr<xaml::IFrameworkElement> contentRoot;
            IFC_RETURN(contentElement.As(&contentRoot));

            ctl::ComPtr<IInspectable> animationElementInspectable;
            IFC_RETURN(contentRoot->FindName(m_elementName, &animationElementInspectable));

            if (animationElementInspectable != nullptr)
            {
                IFC_RETURN(animationElementInspectable.CopyTo(animationElement));
                return S_OK;
            }
        }
    }
    return E_INVALIDARG;
}

