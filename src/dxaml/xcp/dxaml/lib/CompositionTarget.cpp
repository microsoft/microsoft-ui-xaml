// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CompositionTarget.h"


namespace DirectUI
{
    CompositionTarget::CompositionTarget()
    {
    }

    HRESULT CompositionTarget::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
    {
        if (InlineIsEqualGUID(riid, __uuidof(xaml_media::ICompositionTargetStatics)))
        {
            *ppObject = static_cast<xaml_media::ICompositionTargetStatics *>(this);
        }
        else if (InlineIsEqualGUID(riid, __uuidof(xaml_media::ICompositionTarget)))
        {
            *ppObject = static_cast<xaml_media::ICompositionTarget *>(this);
        }
        else if (InlineIsEqualGUID(riid, __uuidof(ICompositionTargetPrivate)))
        {
            *ppObject = static_cast<ICompositionTargetPrivate*>(this);
        }
        else
        {
            return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
        }

        AddRefOuter();
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT CompositionTarget::add_Rendering(
        _In_ wf::IEventHandler<IInspectable*>* pValue,
        _Out_ EventRegistrationToken* ptToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetRenderingEventSource(&pEventSource));
        if (!pEventSource->HasHandlers())
        {
            // Tell the core we're interested in Rendering callbacks for this object.
            IFC(CoreImports::WantsEventStatic(static_cast<CCoreServices *>(pdxc->GetHandle()), ManagedEvent::ManagedEventRendering, TRUE));
        }
        IFC(pEventSource->AddHandler(pValue));
        ptToken->value = (INT64)pValue;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

    _Check_return_ HRESULT CompositionTarget::remove_Rendering(
        _In_ EventRegistrationToken tToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
        wf::IEventHandler<IInspectable*>* pValue = (wf::IEventHandler<IInspectable*>*)tToken.value;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        // removal of event handlers can occur during core shutdown
        // through ShutdownAllPeers(), we don't need to explicitly
        // clean up the handlers from the core in those cases.
        if (pdxc && DXamlCore::IsShuttingDownStatic())
        {
            goto Cleanup;
        }

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetRenderingEventSource(&pEventSource));
        IFC(pEventSource->RemoveHandler(pValue));

        if (!pEventSource->HasHandlers())
        {
            // Tell the core we're not interested in Rendering callbacks for this object.
            IFC(CoreImports::WantsEventStatic(static_cast<CCoreServices *>(pdxc->GetHandle()), ManagedEvent::ManagedEventRendering, FALSE));
        }

        tToken.value = 0;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

    _Check_return_ HRESULT CompositionTarget::add_SurfaceContentsLost(
        _In_ wf::IEventHandler<IInspectable*>* pValue,
        _Out_ EventRegistrationToken* ptToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetSurfaceContentsLostEventSource(&pEventSource));
        IFC(pEventSource->AddHandler(pValue));
        ptToken->value = (INT64)pValue;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

   _Check_return_ HRESULT CompositionTarget::add_Rendered(
        _In_ wf::IEventHandler<xaml_media::RenderedEventArgs*>* pValue,
        _Out_ EventRegistrationToken* ptToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>* pEventSource = NULL;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetRenderedEventSource(&pEventSource));
        if (!pEventSource->HasHandlers())
        {
            // Tell the core we're interested in Rendered callbacks for this object.
            IFC(CoreImports::WantsEventStatic(static_cast<CCoreServices *>(pdxc->GetHandle()), ManagedEvent::ManagedEventRendered, TRUE));
        }
        IFC(pEventSource->AddHandler(pValue));
        ptToken->value = (INT64)pValue;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

    _Check_return_ HRESULT CompositionTarget::remove_Rendered(
        _In_ EventRegistrationToken tToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>* pEventSource = NULL;
        wf::IEventHandler<xaml_media::RenderedEventArgs*>* pValue = (wf::IEventHandler<xaml_media::RenderedEventArgs*>*)tToken.value;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        // removal of event handlers can occur during core shutdown
        // through ShutdownAllPeers(), we don't need to explicitly
        // clean up the handlers from the core in those cases.
        if (pdxc && DXamlCore::IsShuttingDownStatic())
        {
            goto Cleanup;
        }

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetRenderedEventSource(&pEventSource));
        IFC(pEventSource->RemoveHandler(pValue));

        if (!pEventSource->HasHandlers())
        {
            // Tell the core we're not interested in Rendered callbacks for this object.
            IFC(CoreImports::WantsEventStatic(static_cast<CCoreServices *>(pdxc->GetHandle()), ManagedEvent::ManagedEventRendered, FALSE));
        }

        tToken.value = 0;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

    _Check_return_ HRESULT CompositionTarget::remove_SurfaceContentsLost(
        _In_ EventRegistrationToken tToken)
    {
        HRESULT hr = S_OK;
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
        wf::IEventHandler<IInspectable*>* pValue = (wf::IEventHandler<IInspectable*>*)tToken.value;
        DXamlCore *pdxc = DXamlCore::GetCurrent();

        // removal of event handlers can occur during core shutdown
        // through ShutdownAllPeers(), we don't need to explicitly
        // clean up the handlers from the core in those cases.
        if (pdxc && DXamlCore::IsShuttingDownStatic())
        {
            goto Cleanup;
        }

        IFC(CheckActivationAllowed());

        IFC(pdxc->GetSurfaceContentsLostEventSource(&pEventSource));
        IFC(pEventSource->RemoveHandler(pValue));

        tToken.value = 0;

    Cleanup:
        ctl::release_interface(pEventSource);
        RRETURN(hr);
    }

    _Check_return_ HRESULT CompositionTarget::SuspendRendering(_In_ BOOLEAN isSuspended)
    {
        DXamlCore* dxamlCore = DXamlCore::GetCurrent();

        IFC_RETURN(CheckActivationAllowed());

        if (isSuspended)
        {
            dxamlCore->TriggerSuspend(
                true /* isTriggeredByResourceTimer - don't free resources */,
                false /* allowOfferResources - don't free resources */);
        }
        else
        {
            dxamlCore->TriggerResume();
        }

        return S_OK;
    }

    _Check_return_ IActivationFactory* CreateActivationFactory_CompositionTarget()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::CompositionTarget>::CreateActivationFactory());
    }

    _Check_return_ HRESULT CompositionTarget::GetCompositorForCurrentThread(_Outptr_ WUComp::ICompositor** compositor)
    {
        IFC_RETURN(CheckActivationAllowed());
        
        IFCPTR_RETURN(compositor);
        *compositor = DXamlCore::GetCurrent()->GetHandle()->GetCompositor();
        AddRefInterface(*compositor);
        return S_OK;
    }
}
