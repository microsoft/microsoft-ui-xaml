// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"

#include <ObjectWriterCallbacksTemplateParentSetter.h>
#include <namescope\inc\NameScopeRoot.h>
#include "ObjectWriterSettings.h"
#include "XamlQualifiedObject.h"
#include "DirectUIXamlQualifiedObject.h"

#include "ThemeResource.h"
#include "Value.h"
#include "DXamlServices.h"
#include "CValueBoxer.h"

CFrameworkTemplate::~CFrameworkTemplate()
{
    ReleaseInterface(m_pTemplateContent);
}

//-------------------------------------------------------------------------
//i
//  Function:   CFrameworkTemplate::LoadContent
//
//  Synopsis:   This will fulfill the template's destiny by instantiating
//              the visuals that the template was created to instantiate.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkTemplate::LoadContent(_Outptr_result_maybenull_ CDependencyObject** ppResult, _In_opt_ CDependencyObject *pTemplatedParent,
    _In_ XUINT32 bRegisterNamesInTemplateNamescope, bool skipRegistrationEntirely, const std::shared_ptr<XamlQualifiedObject>& xBindConnector)
{
    HRESULT hr = S_OK;
    xref_ptr<CDependencyObject> pEventRoot;
    CDependencyObject *pResult = nullptr;
    xref_ptr<INameScope> spNameScope;

    // If asked to register names in template namescope, a templatedParent should be provided.
    ASSERT(bRegisterNamesInTemplateNamescope && pTemplatedParent || !bRegisterNamesInTemplateNamescope);

    if (!ppResult)
    {
        IFC(E_INVALIDARG);
    }

    // Performance marker
    XCP_PERF_LOG(XCP_PMK_PARSE_BEGIN);

    TraceLoadTemplateContentBegin((XUINT64)this);

    // Get the current event root for this template
    if (m_pTemplateContent != nullptr)
    {
        pEventRoot = m_pTemplateContent->GetSavedEventRoot();
    }


    if ( m_pTemplateContent )
    {
        std::shared_ptr<ObjectWriterCallbacksDelegate> spCallbacks;
        CValue cEventRoot;
        CValue cXBindParentConnector;
        ObjectWriterSettings objectWriterSettings;

        if (xBindConnector)
        {
            objectWriterSettings.set_XBindConnector(xBindConnector);
        }


        cEventRoot.SetObjectNoRef(pEventRoot.detach());

        if (m_spParentXBindConnector)
        {
            wrl::ComPtr<IInspectable> parentConnector;
            m_spParentXBindConnector->Resolve<IInspectable>(&parentConnector);
            if (parentConnector != nullptr)
            {
                cXBindParentConnector.SetIInspectableNoRef(parentConnector.Detach());
            }
        }

        auto qoEventRoot = std::make_shared<XamlQualifiedObject>();
        IFC(qoEventRoot->SetValue(cEventRoot));

        objectWriterSettings.set_EventRoot(qoEventRoot);

        auto qoParentXBindConnector = std::make_shared<XamlQualifiedObject>();
        IFC(qoParentXBindConnector->SetValue(cXBindParentConnector))
        objectWriterSettings.set_XBindParentConnector(qoParentXBindConnector);

        // FUTURE: Notice how eerily similar ObjectWriterCallbacksTemplateParentSetter and
        // NameScopeHelper are? The slow convergence of these two classes is not unintentional.
        // As it becomes more clear that they are simply both expressions of an abstraction designed
        // to transparently pass knowledge of Templates to the parser it becomes more clear that
        // they should be collapsed into a single interface.

        // if the template is going to have its items registered separately,
        // create the template namescope.
        if (bRegisterNamesInTemplateNamescope)
        {
            CCoreServices* pCore = NULL;

            spCallbacks = std::make_shared<ObjectWriterCallbacksTemplateParentSetter>(
                xref_ptr<CDependencyObject>(pTemplatedParent), Jupiter::NameScoping::NameScopeType::TemplateNameScope);
            objectWriterSettings.set_ObjectWriterCallbacks(spCallbacks);

            // Create a namescope store
            pCore = GetContext();
            IFCEXPECT(pCore);
            pCore->GetNameScopeRoot().EnsureNameScope(pTemplatedParent, m_pTemplateContent);

            // Register the template's namescope
            spNameScope = make_xref<NameScopeHelper>(pCore, pTemplatedParent, Jupiter::NameScoping::NameScopeType::TemplateNameScope);
            objectWriterSettings.set_NameScope(spNameScope);
        }
        else
        {

            spCallbacks = std::make_shared<ObjectWriterCallbacksTemplateParentSetter>(
                xref_ptr<CDependencyObject>(pTemplatedParent), Jupiter::NameScoping::NameScopeType::StandardNameScope);
            objectWriterSettings.set_ObjectWriterCallbacks(spCallbacks);


            spNameScope = skipRegistrationEntirely ?
                static_sp_cast<INameScope>(make_xref<NullNameScopeHelper>()) :
                static_sp_cast<INameScope>(make_xref<NameScopeHelper>(GetContext(), pTemplatedParent,
                    Jupiter::NameScoping::NameScopeType::StandardNameScope));
            objectWriterSettings.set_NameScope(spNameScope);
        }

        IFC(m_pTemplateContent->Load(objectWriterSettings, TRUE /* bTryOptimizeContent */, &pResult));

        // Addref the core that the DO points to, since this fragment is not in the tree..
        if (pResult)
        {
            pResult->ContextAddRef();
        }
    }

    *ppResult = pResult;
Cleanup:
    // Performance marker
    XCP_PERF_LOG(XCP_PMK_PARSE_END);

    TraceLoadTemplateContentEnd((XUINT64)this, (XUINT64)(ppResult != NULL ? *ppResult : NULL));

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDisplayMemberTemplate::LoadContent
//
//  Synopsis:   ContentPresenter needs a way remember that it will be visualizing with a TextBlock bound to an item path.
//              Since we don't have the concept of DataTemplateSelector we save this information in a separate template class.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CDisplayMemberTemplate::LoadContent(_Outptr_result_maybenull_ CDependencyObject** ppResult, _In_opt_ CDependencyObject* pTemplatedParent)
{
    HRESULT hr;
    CControlTemplate* pTemplate = NULL;

    IFC(ppResult ? S_OK : E_INVALIDARG);

    if (pTemplatedParent->OfTypeByIndex<KnownTypeIndex::ContentPresenter>())
    {
        IFC(static_cast<CContentPresenter*>(pTemplatedParent)->CreateDefaultContent(m_isDefaultTemplate ? NULL : &m_strMemberPath, ppResult));
    }
    else
    {
        IFC(CContentControl::CreateDefaultTemplate(&pTemplate, pTemplatedParent, m_strMemberPath));
        IFC(pTemplate ? S_OK : E_FAIL);
        IFC(pTemplate->LoadContent(ppResult, pTemplatedParent));
    }

Cleanup:
    ReleaseInterface(pTemplate);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CItemsPanelTemplate::LoadContent
//
//  Synopsis:   The ItemsPanelTemplate specifies the panel that is used for the layout of items in an ItemsControl.
//
//-------------------------------------------------------------------------

_Check_return_
HRESULT
CItemsPanelTemplate::LoadContent(_Outptr_result_maybenull_ CDependencyObject** ppResult, _In_opt_ CDependencyObject* pTemplatedParent)
{
    HRESULT hr = S_OK;
    CREATEPARAMETERS cp(GetContext());
    CPanel *pPanel = NULL;
    CDependencyObject * pResult = NULL;

    if (!ppResult || !pTemplatedParent)
    {
        IFC(E_UNEXPECTED);
    }

    *ppResult = NULL;

    if (HasTemplateContent())
    {
        // Let the base class do the work.
        IFC(CFrameworkTemplate::LoadContent(&pResult, pTemplatedParent, FALSE, true));
    }
    else
    {
        // The default panel is a StackPanel.
        IFC(CStackPanel::Create(&pResult, &cp));
        IFC(pResult->SetTemplatedParent(pTemplatedParent));
        // Returns S_FALSE if the SetTemplatedParent virtual method is not overridden
        if (hr == S_FALSE)
        {
            hr = S_OK; // Do not propagate S_FALSE
            goto Cleanup;
        }
    }

    IFCEXPECT_ASSERT(pResult);

    // The user supplied template must instanticate an object that derives from Panel.
    pPanel = do_pointer_cast < CPanel >(pResult);
    if (pPanel == NULL)
    {
        IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_ITEMS_CONTROL_INVALID_PANEL));
    }

    // The user supplied template must have a Panel object without children
    if (pPanel->GetChildren() && pPanel->GetChildren()->GetCount() >  0)
    {
        IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_ITEMS_PANEL_TEMPLATE_CHILDREN));
    }

    * ppResult = pResult;
    pResult = NULL;

Cleanup:
    ReleaseInterface(pResult);
    RRETURN(hr);
}

_Check_return_
HRESULT
CControlTemplate::LoadContent(_Outptr_result_maybenull_ CDependencyObject** ppResult, _In_opt_ CDependencyObject* pTemplatedParent)
{
    std::shared_ptr<XamlQualifiedObject> xBindConnector;
    if (pTemplatedParent && m_pTemplateContent)
    {
        xBindConnector = CreateXBindConnector(pTemplatedParent);
    }

    IFC_RETURN(__super::LoadContent(ppResult, pTemplatedParent, pTemplatedParent != NULL /* bRegisterNamesInTemplateNamescope */, false, xBindConnector));

    // Connect the template after all templated children have been connected
    if (xBindConnector)
    {
        IFC_RETURN(ConnectTemplate(xBindConnector));
    }
    return S_OK;
}

_Check_return_ HRESULT CControlTemplate::TargetType(
    _In_ CDependencyObject *object,
    _In_ XUINT32 numArgs,
    _Inout_updates_(numArgs) CValue *args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue *result)
{
    auto controlTemplate = checked_cast<CControlTemplate>(object);
    if (numArgs == 0)
    {
        // Getting the value
        result->SetTypeHandle(controlTemplate->GetTargetTypeIndex());
    }
    else if (numArgs == 1 && args->GetType() == valueTypeHandle)
    {
        // Setting the value
        controlTemplate->SetTargetType(args->AsTypeHandle());
    }
    else
    {
        ASSERT(false);
        IFC_RETURN(E_INVALIDARG);
    }
    return S_OK;
}

_Check_return_ HRESULT CControlTemplate::ConnectTemplate(const std::shared_ptr<XamlQualifiedObject>& xBindConnector)
{
    wrl::ComPtr<IInspectable> value(xBindConnector->GetValue().AsIInspectable());
    wrl::ComPtr<DirectUI::IWeakInspectable> weakValue;
    IFC_RETURN(value.As(&weakValue));

    wrl::ComPtr<IInspectable> connectorIInspectable;
    IFC_RETURN(weakValue->GetInspectable(&connectorIInspectable));
    if (connectorIInspectable)
    {
        wrl::ComPtr<xaml_markup::IComponentConnector> connector;
        IFC_RETURN(connectorIInspectable.As(&connector));
        wrl::ComPtr<IInspectable> templatePeer;
        IFC_RETURN(DirectUI::DXamlServices::TryGetPeer(this, IID_PPV_ARGS(&templatePeer)));
        if (templatePeer)
        {
            IFC_RETURN(connector->Connect(m_connectionId, templatePeer.Get()));
        }
    }

    return S_OK;
}

void CFrameworkTemplate::SetParentXBindConnector(const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector)
{
    if (spParentXBindConnector != nullptr)
    {
        wrl::ComPtr<IInspectable> coreParentConnector;
        XamlQualifiedObject qoLocalComponentConnectorVal;
        VERIFYHR(spParentXBindConnector->ConvertForManaged(qoLocalComponentConnectorVal));

        wrl::ComPtr<DirectUI::IWeakInspectable> coreParentConnectorWeak;

        VERIFYHR(reinterpret_cast<DirectUI::XamlQualifiedObject*>(&qoLocalComponentConnectorVal)->GetValue(&coreParentConnector, FALSE /*fPegNoRef*/));

        IGNOREHR(coreParentConnector.As(&coreParentConnectorWeak));

        if (coreParentConnectorWeak)
        {
            VERIFYHR(coreParentConnectorWeak->GetInspectable(&coreParentConnector));
        }

        if (coreParentConnector)
        {
            wrl::ComPtr<IWeakReferenceSource> spWeakRefSource;
            VERIFYHR(coreParentConnector.As(&spWeakRefSource));
            VERIFYHR(spWeakRefSource->GetWeakReference(&m_spParentXBindConnector));
        }
    }
}

std::shared_ptr<XamlQualifiedObject> CControlTemplate::CreateXBindConnector(_In_ CDependencyObject* templatedParent)
{
    wrl::ComPtr<xaml_markup::IComponentConnector> connector;
    wrl::ComPtr<xaml_markup::IComponentConnector> connectorGetter;

    // Retrieve the IInspectable corresponding to the parent IComponentConnector
    wrl::ComPtr<IInspectable> coreParentConnector;

    if (m_spParentXBindConnector)
    {
        VERIFYHR(m_spParentXBindConnector->Resolve<IInspectable>(&coreParentConnector));
    }


    if (m_connectionId != -1)
    {
        // See if one is already stored on the templated parent
        if (!templatedParent->IsPropertyDefaultByIndex(KnownPropertyIndex::XamlBindingHelper_DataTemplateComponent))
        {
            CValue dataTemplateComponentValue;
            IFCFAILFAST(templatedParent->GetValueByIndex(KnownPropertyIndex::XamlBindingHelper_DataTemplateComponent, &dataTemplateComponentValue));
            if (!dataTemplateComponentValue.IsNull())
            {
                IFCFAILFAST(DirectUI::CValueBoxer::UnboxObjectValue(&dataTemplateComponentValue, nullptr, IID_PPV_ARGS(&connector)));
            }
        }
        else if ((coreParentConnector && SUCCEEDED(coreParentConnector.As(&connectorGetter))) && connectorGetter)
        {
            // This case is for when x:Bind uses named elements outside of its namescope.  In that scenario,
            // we need to call GetBindingConnector on the IComponentConnector for the ancestor namescope of this template.
            // That way, the IComponentConnector::GetBindingConnector call can associate this new connector as a child
            // of its ancestor to resolve named elements in its scope.

            // No existing xBind connector. First, assume the IComponentConnector exists on the parent connector
            // instead of the event root. We pass in the templated parent to the root so that we can then get the
            // IComponentConnector for this specific template instantiation.
            wrl::ComPtr<IInspectable> templatedParentPeer;
            if (SUCCEEDED(DirectUI::DXamlServices::TryGetPeer(templatedParent, IID_PPV_ARGS(&templatedParentPeer))) && templatedParentPeer)
            {
                VERIFYHR(connectorGetter->GetBindingConnector(m_connectionId, templatedParentPeer.Get(), &connector));
            }
        }

        if (!connector && (SUCCEEDED(DirectUI::DXamlServices::TryGetPeer(m_pTemplateContent->GetSavedEventRoot().get(), IID_PPV_ARGS(&connectorGetter))) && connectorGetter))
        {
            // In this case, our x:Bind doesn't need to resolve any named elements outside of its namescope, so the ancestor connector would return null when trying to create it.
            // If we couldn't create our x:Bind connector by calling GetBindingConnector on its ancestor connector, fall back to retrieving it from the event root (the root Page/UserControl/etc.).
            // We use the root control/page's GetBindingConnector to get it instead since we aren't concerned about our namescope/connector hierarchy.

            // If there was no IComponentConnector on the parent connector, fall back to retrieving it from the event root.
            wrl::ComPtr<IInspectable> templatedParentPeer;
            if (SUCCEEDED(DirectUI::DXamlServices::TryGetPeer(templatedParent, IID_PPV_ARGS(&templatedParentPeer))) && templatedParentPeer)
            {
                VERIFYHR(connectorGetter->GetBindingConnector(m_connectionId, templatedParentPeer.Get(), &connector));
            }
        }
    }

    std::shared_ptr<XamlQualifiedObject> xBindConnector;
    if (connector)
    {
        wrl::ComPtr<IInspectable> connectorWeakReference;
        IFCFAILFAST(DirectUI::ValueWeakReference::Create(connector.Get(), &connectorWeakReference));
        xBindConnector = std::make_shared<XamlQualifiedObject>();
        xBindConnector->GetValue().SetIInspectableAddRef(connectorWeakReference.Get());
    }

    return xBindConnector;
}

//-------------------------------------------------------------------------
//
//  Function:   CControlTemplate::GetValue
//
//  Synopsis:   Get the value of a dependency property from a ControlTemplate.
//              This also returns the default value of Control if the TargetType
//              property has not been set.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CControlTemplate::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue)
{
    HRESULT hr = S_OK;
    CType* pType = nullptr;

    // Get the default value for TargetType if it hasn't been set
    if (pdp &&
        pdp->GetIndex() == KnownPropertyIndex::ControlTemplate_TargetType &&
        IsPropertyDefault(pdp))
    {
        CREATEPARAMETERS cp(GetContext());

        // Create a new Type instance
        IFC(CType::Create((CDependencyObject**)&pType, &cp));

        // The default TargetType is System.Windows.Controls.Control
        pType->m_nTypeIndex = KnownTypeIndex::Control;

        pValue->SetObjectNoRef(pType);
        pType = nullptr;
    }
    else
    {
        IFC(CFrameworkTemplate::GetValue(pdp, pValue));
    }

Cleanup:
    ReleaseInterface(pType);
    RRETURN(hr);
}

const CClassInfo* CControlTemplate::GetTargetType() const
{
    return DirectUI::MetadataAPI::GetClassInfoByIndex(m_targetTypeIndex);
}

_Check_return_
HRESULT
CDataTemplate::LoadContent(_Outptr_result_maybenull_ CDependencyObject** result, _In_opt_ CDependencyObject* templatedParent)
{
    // Task 16544853: Allow Deferred attaching of namescope/registrations.
    // We cannot reuse the cached instance currently because we did not register the names during the query call.
    // So falling back to create a new tree. This means we will end up creating one extra tree per template. Not ideal but it
    // allows us to unblock the functionality while we figure out how to deal with the name registrations.

    //if (m_cachedInstance)
    //{
    //    // There is a cached instance. use that and clear the cache.
    //    *result = m_cachedInstance;

    //    // We can let go of the strong peg and take a NoRef peg to emulate what LoadContent would have done.
    //    IFC_RETURN(RemovePeerReferenceToItem(m_cachedInstance));
    //}
    //else
    {
        // Let the base class do the work.
        IFC_RETURN(CFrameworkTemplate::LoadContent(result, templatedParent, templatedParent != nullptr /* bRegisterNamesInTemplateNamescope */, false /* skipRegistrationEntirely */));
    }

    return S_OK;
}

_Check_return_
HRESULT
CDataTemplate::QueryContentNoRef(_Outptr_result_maybenull_ CDependencyObject** result, _In_opt_ CDependencyObject* templatedParent)
{
    if (m_cachedInstance)
    {
        // We have a cached instance, use that.
        // Multiple queries can be answered with the same instance - although this does not happen with the modern panels.
        *result = m_cachedInstance.get();
    }
    else
    {
        // Load a new one and cache it.
        IFC_RETURN(CFrameworkTemplate::LoadContent(m_cachedInstance.ReleaseAndGetAddressOf(), templatedParent, templatedParent != nullptr /* bRegisterNamesInTemplateNamescope */, false /* skipRegistrationEntirely */));
        *result = m_cachedInstance.get();

        if (m_cachedInstance)
        {
            // Add a reference to m_cachedInstance's managed peer so it does not get destroyed while we hold on to m_cachedInstance.
            IFC_RETURN(AddPeerReferenceToItem(m_cachedInstance));
            // The NoRef peg applied by the parser can then be removed.
            m_cachedInstance->UnpegManagedPeerNoRef();
        }
    }

    return S_OK;
}

CDataTemplate::~CDataTemplate()
{
    if (m_cachedInstance)
    {
        IGNOREHR(RemovePeerReferenceToItem(m_cachedInstance));
        m_cachedInstance = nullptr;
    }
}