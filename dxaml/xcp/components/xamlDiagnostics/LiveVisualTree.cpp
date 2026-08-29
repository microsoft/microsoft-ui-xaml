// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDiagnostics.h"
#include "wil\resource.h"
#include "HandleMap.h"
#include "runtimeEnabledFeatures\inc\RuntimeEnabledFeatures.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "DiagnosticsInterop.h"
#include "corewindow.h"
#include "RuntimeObject.h"
#include "RuntimeElement.h"
#include "DependencyObject.h"
#include "DoPointerCast.h"
#include <WeakReferenceSource.h>
#include "XamlIslandRoot.h"
#include "DCompTreeHost.h"
#include <WRLHelper.h>

using namespace RuntimeFeatureBehavior;
using namespace Diagnostics;

namespace Advising
{
    // We only need to get all the UI threads and dispatch the call when setting up the initial visual tree.
    // Otherwise, during normal operation it is the caller's responsibility to dispatch to the correct thread.
    HRESULT RunOnUIThread(msy::IDispatcherQueue* queue, const DispatcherMethod& func)
    {
        HRESULT innerHr = S_OK;
        wil::unique_event_nothrow completedEvent;
        IFC_RETURN(completedEvent.create(wil::EventOptions::None));
        auto dispatchedHandler = WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([&innerHr, &func, &completedEvent]()
        {
            innerHr = func();
            completedEvent.SetEvent();
            return S_OK;
        });

        auto completedHandler = wrl::Callback<wf::IAsyncActionCompletedHandler>([&completedEvent](wf::IAsyncAction*, wf::AsyncStatus) -> HRESULT {
            completedEvent.SetEvent();
            return S_OK;
        });

        boolean succeeded = FALSE;
        IFC_RETURN(queue->TryEnqueue(dispatchedHandler.Get(), &succeeded));
        IFCEXPECT_RETURN(succeeded);
        completedEvent.wait();
        return innerHr;
    }
}

void
XamlDiagnostics::SignalMutation(
    _In_ xaml::IDependencyObject* pReference,
    _In_ VisualMutationType mutationType)
{
    // We need to do some work no matter what if the object is leaving the tree.
    if (mutationType == VisualMutationType::Remove)
    {
        // It is possible to get here even if the object wasn't in the live tree. If it's not
        // in the live tree then we don't have a reference to the object and it's not safe to
        // QI in case the CCW is in a neutered state.
        InstanceHandle elementHandle = 0u;
        if (!GetHandleForDependencyObject(pReference, &elementHandle))
        {
            return;
        }

        // At this point we have a reference to the object so we know it is ok to QI.
        std::shared_ptr<RuntimeElement> runtimeElement;
        if (TryFindElementFromHandle(elementHandle, runtimeElement))
        {
            RemoveElementFromLVT(runtimeElement);
        }
    }
    else
    {
        wrl::ComPtr<IInspectable> parent;
        if (m_spDiagInterop->TryGetVisualTreeParent(pReference, &parent))
        {
            AddDependencyObjectToMap(pReference);
            std::shared_ptr<RuntimeElement> runtimeParent;
            if (parent)
            {
                runtimeParent = GetRuntimeElement(parent.Get());
            }
            auto runtimeElement = GetRuntimeElement(pReference, runtimeParent);
            AddElementToLVT(runtimeElement, runtimeParent);
        }
    }
}

void XamlDiagnostics::RemoveElementFromLVT(const std::shared_ptr<Diagnostics::RuntimeElement>& removedElement)
{
    if (m_visualTreeCallback)
    {
        wil::unique_visualelement element;
        element.Handle = removedElement->GetHandle();
        ParentChildRelation relation = { 0 };
        relation.Child = removedElement->GetHandle();

        m_visualTreeCallback->OnVisualTreeChange(
            relation,
            element.release(),
            VisualMutationType::Remove);
    }

    // This object is no longer in the tree so we can close it
    removedElement->Close();
}

void XamlDiagnostics::AddElementToLVT(const std::shared_ptr<Diagnostics::RuntimeElement>& addedElement, const std::shared_ptr<Diagnostics::RuntimeElement>& parentElement)
{
    // Work that needs to be done regardless if we are setup for callback notifications to ensure proper child/parent relationship.
    if (parentElement)
    {
        if (!parentElement->HasChild(addedElement))
        {
            // The parent currently doesn't know about this child, so add the child
            parentElement->AddChild(addedElement);
        }

        wrl::ComPtr<xaml::IDependencyObject> parentDo;
        if (SUCCEEDED(parentElement->GetBackingObject().As(&parentDo)))
        {
            AddDependencyObjectToMap(parentDo.Get());
        }
    }

    if (m_visualTreeCallback)
    {
        // Setup the element and parent child relationship. The element is the DO that is entering
        // the tree and is the child object in the parent child relationship.
        ComValueCollection<wil::unique_visualelement> elements;
        std::map<InstanceHandle, ParentChildRelation> relations;

        ParentChildRelation relation = { 0 };
        PopulateParentRelation(addedElement, parentElement, relation);

        wil::unique_visualelement element;
        PopulateElementInfo(addedElement, element);

        elements.Append(std::move(element));
        relations.emplace(relation.Child, std::move(relation));

        auto itRelation = relations.begin();
        // Iterate through callbacks to notify.
        for (auto itElement = elements.GetVectorView().begin(); itElement != elements.GetVectorView().end(); itElement++)
        {
            itRelation = relations.find(itElement->Handle);
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(itRelation != relations.end());
            if (itRelation != relations.end())
            {
                // We have to release here. Our API is very awkward and we pass the struct by value. It isn't clear who has ownership
                // and since previous versions of VS don't reallocate the strings inside of these structs, the contract is that we are
                // giving ownership of these strings to the callback.
                m_visualTreeCallback->OnVisualTreeChange(
                    itRelation->second,
                    itElement->release(),
                    VisualMutationType::Add);
            }
        }
    }
}

IFACEMETHODIMP
XamlDiagnostics::AdviseVisualTreeChange(
    _In_ IVisualTreeServiceCallback* pCallback)
{
    SuspendFailFastOnStowedException suspender;
    // We only support advising once
    IFCEXPECT_RETURN(m_visualTreeCallback == nullptr);
    IFCPTR_RETURN(pCallback);

    m_visualTreeCallback = pCallback;

    // It's ok if we don't have any dispatcher queues, that just means that AdviseVisualTreeChange was called before any core
    // was created. As long as we've set the callback, every thing will work correctly.
    auto queues = m_spDiagInterop->GetDispatcherQueues();

    const auto giveRoots = [&]{
        return GiveRootsToCallback(pCallback);
    };

    for (const auto& queue : m_spDiagInterop->GetDispatcherQueues())
    {
        IFC_RETURN(Advising::RunOnUIThread(queue.Get(), giveRoots));
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::UnadviseVisualTreeChange(
    _In_ IVisualTreeServiceCallback* pCallback)
{
    SuspendFailFastOnStowedException suspender;
    IFCPTR_RETURN(pCallback);

    IFCEXPECT_RETURN(pCallback == m_visualTreeCallback.Get());

    m_visualTreeCallback.Reset();

    return S_OK;
}

wil::unique_sourceinfo
XamlDiagnostics::GetSourceInfo(_In_ xaml::ISourceInfoPrivate* sourceInfo)
{
    wil::unique_sourceinfo info;

    wrl_wrappers::HString uri, hash;
    int sourceLine = 0, sourceColumn = 0;

    // Get the source information.
    VERIFYHR(sourceInfo->get_Line(&sourceLine));
    VERIFYHR(sourceInfo->get_Column(&sourceColumn));
    VERIFYHR(sourceInfo->get_ParseUri(uri.GetAddressOf()));
    VERIFYHR(sourceInfo->get_XbfHash(hash.GetAddressOf()));

    info.LineNumber = sourceLine;
    info.ColumnNumber = sourceColumn;
    info.FileName = SysAllocString(uri.GetRawBuffer(nullptr));
    info.Hash = SysAllocString(hash.GetRawBuffer(nullptr));

    return info;
}

HRESULT
XamlDiagnostics::PopulateElementAndRelationCache(
    const std::shared_ptr<RuntimeElement>& obj,
    _In_ int depth,
    _In_ bool srcInfo,
    _Inout_ ComValueCollection<wil::unique_visualelement>& elements,
    _Inout_ std::map<InstanceHandle, ParentChildRelation>& relations)
{
    std::queue<std::shared_ptr<RuntimeElement>> processingQueue;
    wrl::ComPtr<xaml::IDependencyObject> backingDO;
    if (SUCCEEDED(obj->GetBackingObject().As(&backingDO)))
    {
        AddDependencyObjectToMap(backingDO.Get());
    }

    // Set up the breadth-first processor.
    processingQueue.push(obj);

    while (!processingQueue.empty())
    {
        // Get the current element to process.
        auto current = processingQueue.front();
        // Pop the front of the queue.
        processingQueue.pop();

        // Try to insert current as a DependencyObject
        IFC_RETURN(PopulateElementAndRelationCache(current, elements, relations, processingQueue, srcInfo));
    }

    return S_OK;
}

HRESULT
XamlDiagnostics::PopulateElementAndRelationCache(
    const std::shared_ptr<RuntimeElement>& runtimeElement,
    _Inout_ ComValueCollection<wil::unique_visualelement>& elements,
    _Inout_ std::map<InstanceHandle, ParentChildRelation>& relations,
    _Inout_ std::queue<std::shared_ptr<RuntimeElement>>& processingQueue,
    _In_ bool srcInfo)
{
    // Add to the relationships and push children onto the queue.
    if (auto children = runtimeElement->GetChildren())
    {
        for (auto iter = children->begin(); iter != children->end(); ++iter)
        {
            std::shared_ptr<RuntimeObject> item = *iter;
            std::shared_ptr<RuntimeElement> childElement;
            if (item->TryGetAsElement(childElement))
            {
                processingQueue.push(childElement);
            }

            wrl::ComPtr<xaml::IDependencyObject> backingDO;
            IFCFAILFAST(item->GetBackingObject().As(&backingDO));
            AddDependencyObjectToMap(backingDO.Get());

            ParentChildRelation relation = {};
            relation.ChildIndex = static_cast<unsigned>(iter - children->begin());
            relation.Parent = runtimeElement->GetHandle();
            relation.Child = item->GetHandle();

            relations.emplace(relation.Child, std::move(relation));
        }
    }

    wil::unique_visualelement element;
    PopulateElementInfo(runtimeElement, element);
    elements.Append(std::move(element));

    return S_OK;
}

HRESULT
XamlDiagnostics::GiveRootToCallback(
    const std::shared_ptr<RuntimeElement>& root,
    _In_ IVisualTreeServiceCallback* callback,
    _In_ const ParentChildRelation& rootParentRelation)
{
    ComValueCollection<wil::unique_visualelement> elements;
    std::map<InstanceHandle, ParentChildRelation> relations;
    IFC_RETURN(PopulateElementAndRelationCache(root, 0 /* depth */, true, elements, relations));

    auto itElement = elements.GetVectorView().begin();

    // We have to release here. Our API is very awkward and we pass the struct by value. It isn't clear who has ownership
    // and since previous versions of VS don't reallocate the strings inside of these structs, the contract is that we are
    // giving ownership of these strings to the callback.
    IFC_RETURN(callback->OnVisualTreeChange(rootParentRelation, itElement->release(), VisualMutationType::Add));
    itElement++;
    auto itRelation = relations.begin();
    for (; itElement != elements.GetVectorView().end(); itElement++)
    {
        itRelation = relations.find(itElement->Handle);
        if (itRelation == relations.end())
        {
            return E_NOTFOUND;
        }
        // We have to release here. Our API is very awkward and we pass the struct by value. It isn't clear who has ownership
        // and since previous versions of VS don't reallocate the strings inside of these structs, the contract is that we are
        // giving ownership of these strings to the callback.
        IFC_RETURN(callback->OnVisualTreeChange(itRelation->second, itElement->release(), VisualMutationType::Add));
    }

    // Send the Xaml root information if we can and if the callback supports it.
    wrl::ComPtr<IVisualTreeServiceCallback> visualTreeCallback(callback);
    wrl::ComPtr<IVisualTreeServiceCallback3> xamlRootCallback;
    if (SUCCEEDED(visualTreeCallback.As(&xamlRootCallback)))
    {
        wrl::ComPtr<xaml::IXamlRoot> spXamlRoot;
        IFC_RETURN(m_spDiagInterop->GetXamlRoot(root->GetBackingObject().Get(), &spXamlRoot));
        auto publicXamlRootObject = GetRuntimeObject(spXamlRoot.Get());
        root->SetXamlRoot(publicXamlRootObject);
        IFC_RETURN(xamlRootCallback->OnXamlRootChange(publicXamlRootObject->GetHandle(), VisualMutationType::Add));
    }
    return S_OK;
}

HRESULT
XamlDiagnostics::GiveRootsToCallback(
    _In_ const wrl::ComPtr<IVisualTreeServiceCallback>& callback)
{
    std::shared_ptr<RuntimeApplication> app;
    if (!m_runtimeObjectCache->TryFindAssociatedApplication(GetCurrentThreadId(), app))
    {
        wrl::ComPtr<xaml::IApplication> spApplication;
        // Store the app object and it's dictionary in the cache. We create a new RuntimeApplication object for each thread, only so that we can store
        // the thread-dependent Application.Resources object on it.
        IFC_RETURN(m_spDiagInterop->GetApplication(&spApplication));
        app = GetRuntimeApplication(spApplication.Get());
    }

    // When advising, we'll grab the roots and iterate through the tree. We don't get the loaded event until
    // after the first tick, so if a view is created in the background it won't show up in the LVT until it has been shown.
    // This may not be intuitive to developers so we'll show it regardless if we've received the actual loaded
    // event.

    // Get the top level roots and give those back to the callback
    std::vector<wrl::ComPtr<IInspectable>> roots;
    IFC_RETURN(m_spDiagInterop->GetRoots(roots));
    for (auto &root : roots)
    {
        auto rootElement = GetRuntimeElement(root.Get());
        // Since these are the top level roots, the relation is empty.
        ParentChildRelation emptyRelation = {};
        IFC_RETURN(GiveRootToCallback(rootElement, callback.Get(), emptyRelation));
    }

    m_enabledThreads.emplace(GetCurrentThreadId());

    return S_OK;
}

void XamlDiagnostics::SignalRootMutation(_In_opt_ IInspectable* root, _In_ VisualMutationType type)
{
    if (root && type == VisualMutationType::Remove)
    {
        RemoveRootObjectFromLVT(root);
    }
    else if (type == VisualMutationType::Add)
    {
        auto rootElement = GetRuntimeElement(root);
        if (m_visualTreeCallback)
        {
            // Since these are the top level roots, the relation is empty.
            ParentChildRelation emptyRelation = {};
            VERIFYHR(GiveRootToCallback(rootElement, m_visualTreeCallback.Get(), emptyRelation));
        }
    }
}

void XamlDiagnostics::RemoveRootObjectFromLVT(_In_ IInspectable* root)
{
    ParentChildRelation relation = {};
    wil::unique_visualelement element = {};

    std::shared_ptr<RuntimeElement> rootElement;
    if (TryFindElementFromHandle(HandleMap::GetHandle(root), rootElement))
    {
        element.Handle = rootElement->GetHandle();
        rootElement->Close();

        relation.Child = element.Handle;
        if (m_visualTreeCallback)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(m_visualTreeCallback->OnVisualTreeChange(relation, element.release(), VisualMutationType::Remove));
        }

        // Send the Xaml root information if we can and if the callback supports it.
        // When adding the root to the LVT in GiveRootToCallback, we used DiagnosticsInterop::GetXamlRoot()
        // to get the XamlRoot object associated with the visual tree root, DesktopWindowXamlSource.
        // But here, the DesktopWindowXamlSource has already been disassociated from the visual tree,
        // so instead we'll retrieve the XamlRoot we previously associated with the DesktopWindowXamlSource.
        wrl::ComPtr<IVisualTreeServiceCallback3> xamlRootCallback;
        if (m_visualTreeCallback && SUCCEEDED(m_visualTreeCallback.As(&xamlRootCallback)))
        {
            auto xamlRootRuntimeObject = rootElement->GetXamlRoot();
            xamlRootCallback->OnXamlRootChange(xamlRootRuntimeObject->GetHandle(), VisualMutationType::Remove);
            xamlRootRuntimeObject->Close();
        }
    }
}

void
XamlDiagnostics::PopulateParentRelation(
    const std::shared_ptr<RuntimeElement>& runtimeElement,
     const std::shared_ptr<RuntimeElement>& runtimeParent,
    _Inout_ ParentChildRelation& relation)
{
    if (runtimeParent)
    {
        relation.Parent = runtimeParent->GetHandle();

        // Get relation data.
        if (auto children = runtimeParent->GetChildren())
        {
            size_t childIndex = 0;
#pragma warning(suppress: 4189) // C4189: PREFast does not know this local variable is being used at least in the chk build by the TELEMETRY_ASSERT() below.
            bool hasChild = children->TryGetIndexOf(runtimeElement, childIndex);
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(hasChild);
            relation.ChildIndex = static_cast<unsigned>(childIndex);
        }
    }

    relation.Child = runtimeElement->GetHandle();
}

void
XamlDiagnostics::PopulateElementInfo(
    const std::shared_ptr<Diagnostics::RuntimeElement>& runtimeElement,
    _Inout_ VisualElement& element)
{
    wrl_wrappers::HString typeName;
    if (runtimeElement->IsWindow())
    {
        wrl::ComPtr<xaml::IWindow> currentWindow;
        IFCFAILFAST(runtimeElement->GetBackingObject().As(&currentWindow));
        wil::unique_bstr windowTitle;
        VERIFYHR(GetWindowText(currentWindow, windowTitle));
        element.Name = windowTitle.release();
        VERIFYHR(currentWindow->GetRuntimeClassName(typeName.GetAddressOf()));
    }
    else
    {
        wrl_wrappers::HString strName;
        if (runtimeElement->IsDesktopWindowXamlSource() || runtimeElement->IsXamlIsland())
        {
            VERIFYHR(runtimeElement->GetBackingObject()->GetRuntimeClassName(typeName.GetAddressOf()));
        }
        else
        {
            VERIFYHR(m_spDiagInterop->GetTypeNameFor(runtimeElement->GetBackingObject().Get(), typeName.GetAddressOf()));
        }

        VERIFYHR(m_spDiagInterop->GetName(runtimeElement->GetBackingObject().Get(), strName.GetAddressOf()));

        wrl::ComPtr<xaml::ISourceInfoPrivate> sourceInfo;
        if (SUCCEEDED(runtimeElement->GetBackingObject().As(&sourceInfo)))
        {
            element.SrcInfo = GetSourceInfo(sourceInfo.Get()).release();
        }
        element.Name = wil::make_bstr_nothrow(strName.GetRawBuffer(nullptr)).release();
    }

    element.Type = wil::make_bstr_nothrow(typeName.GetRawBuffer(nullptr)).release();
    element.Handle = runtimeElement->GetHandle();
    if (auto children = runtimeElement->GetChildren())
    {
        element.NumChildren = static_cast<unsigned int>(children->size());
    }
}

void XamlDiagnostics::AddDependencyObjectToMap(
    _In_ xaml::IDependencyObject* pDO,
    _Out_opt_ InstanceHandle* result)
{
    InstanceHandle handle = HandleMap::GetHandle(pDO);
    // Store the handle for later to avoid a QI when objects leave
    auto guard = m_doHandleMapLock.lock_exclusive();
    m_doHandleMap.emplace(pDO, handle);

    if (result)
    {
        *result = handle;
    }
}

bool XamlDiagnostics::GetHandleForDependencyObject(
    _In_ xaml::IDependencyObject* pDO,
    _Out_ InstanceHandle* handle
)
{
    *handle = 0;
    auto guard = m_doHandleMapLock.lock_exclusive();
    auto iter = m_doHandleMap.find(pDO);
    if (iter != m_doHandleMap.end())
    {
        *handle = iter->second;
        m_doHandleMap.erase(iter);
        return true;
    }

    return false;
}

HRESULT XamlDiagnostics::GetWindowText(_In_ const wrl::ComPtr<xaml::IWindow>& window, _Out_ wil::unique_bstr& text)
{
    wrl::ComPtr<wuc::ICoreWindow> coreWindow;
    IFC_RETURN(window->get_CoreWindow(&coreWindow));

    if (coreWindow)
    {
        wrl::ComPtr<ICoreWindowInterop> interop;
        IFC_RETURN(coreWindow.As(&interop));

        HWND backingHwnd = NULL;
        IFC_RETURN(interop->get_WindowHandle(&backingHwnd));

        wchar_t title[MAX_PATH];
        int count = ::GetWindowText(backingHwnd, title, _countof(title));

        // it's possible there could be no window text, and even if we fail, is it the end of the world?
        if (count > 0)
        {
            text = wil::make_bstr_nothrow(title);
        }
    }

    return S_OK;
}
