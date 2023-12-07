// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "inc\ResourceResolver.h"
#include "inc\ScopedResources.h"
#include <corep.h>
#include <CDependencyObject.h>
#include <Resources.h>
#include <framework.h>
#include <CControl.h>
#include <Template.h>
#include <TemplateContent.h>
#include <XamlSchemaContext.h>
#include <XamlServiceProviderContext.h>
#include <CustomWriterRuntimeObjectCreator.h>
#include <ObjectWriterStack.h>
#include <ObjectWriterFrame.h>
#include <ObjectWriterContext.h>
#include <XamlPropertyToken.h>
#include <XamlTypeTokens.h>
#include <XamlQualifiedObject.h>
#include <XamlType.h>
#include <XamlProperty.h>
#include <SavedContext.h>
#include <XamlOM.WinUI.h>
#include <DOPointerCast.h>
#include <DXamlServices.h>
#include <DependencyObjectWrapper.h>
#include "dependencyLocator\inc\DependencyLocator.h"
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "theming\inc\ThemeResource.h"
#include "vsm\inc\VisualState.h"
#include "vsm\inc\CVisualStateManager2.h"
#include "Collection\Inc\VisualStateGroupCollection.h"
#include <DiagnosticsInterop.h>
#include <DependencyObject.h>
#include "deferral\inc\CustomWriterRuntimeContext.h"
#include <FxCallbacks.h>

namespace Diagnostics {
    std::shared_ptr<ResourceGraph> GetResourceGraph()
    {
        static PROVIDE_DEPENDENCY(ResourceGraph, DependencyLocator::StoragePolicyFlags::PerThread);
        static DependencyLocator::Dependency<Diagnostics::ResourceGraph> s_resourceGraph;
        return s_resourceGraph.Get();
    }
}

namespace Resources {

    bool ResourceResolver::s_isLoadingThemeResources = false;

    const _Check_return_ HRESULT ResourceResolver::ResolveStaticResource(
        const xstring_ptr& strResourceKey,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        bool shouldCheckGlobalThemeResources,
        _Out_ ResolvedResource& resolvedResource,
        _In_opt_ CStyle* optimizedStyleParent,
        KnownPropertyIndex stylePropertyIndex)
    {
        IFC_RETURN(ResolveResourceImpl(
            strResourceKey,
            spServiceProviderContext,
            pCore,
            shouldCheckGlobalThemeResources,
            ResourceTypeStatic,
            resolvedResource));

        RegisterResourceDependency(
            strResourceKey,
            spServiceProviderContext,
            resolvedResource.DictionaryReadFrom,
            ResourceTypeStatic,
            optimizedStyleParent,
            stylePropertyIndex);

        return S_OK;
    }

    const _Check_return_ HRESULT ResourceResolver::ResolveThemeResource(
        const xstring_ptr& strResourceKey,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        bool shouldCheckGlobalThemeResources,
        _Out_ ResolvedResource& resolvedResource,
        _In_opt_ CStyle* optimizedStyleParent,
        KnownPropertyIndex stylePropertyIndex)
    {
        IFC_RETURN(ResolveResourceImpl(
            strResourceKey,
            spServiceProviderContext,
            pCore,
            shouldCheckGlobalThemeResources,
            ResourceTypeTheme,
            resolvedResource));

        RegisterResourceDependency(
            strResourceKey,
            spServiceProviderContext,
            resolvedResource.DictionaryReadFrom,
            ResourceTypeTheme,
            optimizedStyleParent,
            stylePropertyIndex);

        return S_OK;
    }

    const _Check_return_ HRESULT ResourceResolver::ResolveResourceImpl(
        const xstring_ptr& strResourceKey,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        bool shouldCheckGlobalThemeResources,
        ResourceType resourceType,
        _Out_ ResolvedResource& resolvedResource)
    {
         // Load Theme Resources if needed, because StaticResources may use them.
        // ThemeResources can have StaticResource in them, so protect against recursive call
        if (!pCore->GetThemeResources() && !s_isLoadingThemeResources)
        {
            s_isLoadingThemeResources = true;
            HRESULT hr = FxCallbacks::FrameworkCallbacks_LoadThemeResources();
            s_isLoadingThemeResources = false;
            IFC_RETURN(hr);
        }
        CDependencyObject *pObject = NULL;

        CResourceDictionary *pDictionary = NULL;
        xref_ptr<CResourceDictionary> dictionaryReadFrom;

        auto qoRootObject = spServiceProviderContext->GetRootObject();
        auto scope = shouldCheckGlobalThemeResources ? Resources::LookupScope::All : Resources::LookupScope::LocalOnly;
        if (qoRootObject)
        {
            pDictionary = do_pointer_cast<CResourceDictionary>(qoRootObject->GetValue());
            if (pDictionary)
            {
                IFC_RETURN(pDictionary->GetKeyForResourceResolutionNoRef(strResourceKey,
                    scope,
                    &pObject,
                    &dictionaryReadFrom));
                scope&= ~Resources::LookupScope::GlobalTheme;
            }
        }

        if (!pObject)
        {
            AmbientValuesVector vecAmbientValues;
            IFC_RETURN(GetAmbientValues(spServiceProviderContext, vecAmbientValues));

            // MSFT:7381670 - Theme resources sometimes override page resources in {StaticResource} resolution
            //
            // Do not check global theme resources here even if shouldCheckGlobalThemeResources is set to true.
            // This is because a resource could be found in the inner dictionary and break out of the loop
            // without getting to the Page dictionary. Instead we will do an explicit call below to check
            // global theme resources after going through the stack provided by GetAllAmbientValues.
            const bool skipGlobalThemeResources = resourceType == ResourceTypeStatic;

            for (auto ambientValuesIterator = vecAmbientValues.m_vector.begin(); !pObject && (ambientValuesIterator != vecAmbientValues.m_vector.end()); ++ambientValuesIterator)
            {
                IFC_RETURN(DoPointerCast(pDictionary, ambientValuesIterator->get()));

                IFC_RETURN(pDictionary->GetKeyForResourceResolutionNoRef(strResourceKey,
                    skipGlobalThemeResources ? Resources::LookupScope::LocalOnly : scope,
                    &pObject,
                    &dictionaryReadFrom));

                // No need to check theme resources anymore in subsequent calls if looking up a theme resource or
                // if using the quirk for static resources because we would've looked in GlobalTheme dictionaries
                // in the previous lookup.
                if (resourceType == ResourceTypeTheme)
                {
                    scope &= ~Resources::LookupScope::GlobalTheme;
                }

            }

        }

        // if not found yet, check global theme resources and the application
        if (!pObject)
        {
            IFC_RETURN(FallbackGetKeyForResourceResolutionNoRef(
                pCore,
                strResourceKey,
                scope,
                &pObject,
                &dictionaryReadFrom));
        }

        //[Bug fix: 95299]
        // If we've found the resource, make sure to unwrap it in the case of a CDependencyObjectWrapper
        // The purpose of CDependencyObjectWrapper is to wrap a DO to work around parenting issues when inserting into ResourceDictionary
        if (resourceType == ResourceTypeStatic && pObject)
        {
            auto wrapper = do_pointer_cast<CDependencyObjectWrapper>(pObject);
            if (wrapper)
            {
                pObject = wrapper->WrappedDO();
                IFCEXPECT_RETURN(pObject);
            }
        }

        {
            IFC_RETURN(ScopedResources::MarkAsOverride(
                strResourceKey,
                spServiceProviderContext));

            if (pObject)
            {
                CDependencyObject* obj = nullptr;
                xref_ptr<CResourceDictionary> dict;

                IFC_RETURN(ScopedResources::TryCreateOverrideForContext(
                    strResourceKey,
                    pObject,
                    dictionaryReadFrom.get(),
                    spServiceProviderContext,
                    &obj,
                    &dict));

                if (obj)
                {
                    pObject = obj;
                    dictionaryReadFrom = std::move(dict);
                }
            }
        }

        // We need to styles with their owning dictionary if XamlDiag is enabled
        AddStyleContext(do_pointer_cast<CStyle>(pObject), dictionaryReadFrom.get(), false);

        xref_ptr<CResourceDictionary> dictionaryForThemeReference;
        if (pObject && resourceType == ResourceTypeTheme)
        {
            dictionaryForThemeReference = GetDictionaryForThemeReference(pCore, pDictionary, dictionaryReadFrom.get());
        }
        resolvedResource = ResolvedResource(pObject, std::move(dictionaryReadFrom), std::move(dictionaryForThemeReference));

        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::FindNextResolvedValueNoRef(
        _In_ CDependencyObject* depObj,
        _In_ CThemeResource* themeResource,
        bool searchForOverrides,
        _Out_ CDependencyObject** resultNoRef)
    {
        IFC_RETURN(FindResolvedValueNoRefImpl(
            depObj,
            themeResource->GetResourceKey(),
            searchForOverrides,
            nullptr,
            resultNoRef));
        return S_OK;
    }

    const _Check_return_ HRESULT ResourceResolver::ResolveResourceRuntime(
        _In_ CDependencyObject* resolutionContext,
        _In_opt_ CResourceDictionary* dictionaryFoundIn,
        const xstring_ptr& resourceKey,
        ResourceType resourceType,
        _Out_ ResolvedResource& resolvedResource)
    {
        ResolvedResource resolved;

        if (dictionaryFoundIn)
        {
            // If the element passed in was found in a resource dictionary, look in it's LocalOnly scope to see if it can be found.
            CDependencyObject *valueNoRef = nullptr;
            IFC_RETURN(dictionaryFoundIn->GetKeyForResourceResolutionNoRef(resourceKey,
                Resources::LookupScope::LocalOnly,
                &valueNoRef,
                &resolvedResource.DictionaryReadFrom));
            if (valueNoRef)
            {
                resolvedResource.Value = valueNoRef;
            }
        }

        bool hadContext = false;
        // Do the tree walk first if this is a template member, this is essentially what the parser does. If it isn't found, then
        // we'll use the cached context
        if (!resolvedResource.Value && !resolutionContext->IsTemplateNamescopeMember())
        {
            // Try to see if we have cached parser context for things like template content, VSM, or Styles
            IFC_RETURN(TryResolveResourceFromCachedParserContext(resolutionContext, resourceKey, resourceType, resolvedResource, &hadContext));
        }

        if (!resolvedResource.Value && !hadContext)
        {
            // If still not resolved, then walk up the tree and grab all the ResourceDictionaries and try to resolve them.
            xref_ptr<CResourceDictionary> dictionaryReadFrom;
            AmbientValuesVector vecAmbientValues;
            GetAmbientValuesRuntime(resolutionContext, vecAmbientValues);
            CDependencyObject* keyNoRef = nullptr;

            CResourceDictionary* dictionary = nullptr;
            for (auto ambientValuesIterator = vecAmbientValues.m_vector.begin(); !keyNoRef && (ambientValuesIterator != vecAmbientValues.m_vector.end()); ++ambientValuesIterator)
            {
                dictionary = checked_cast<CResourceDictionary>(ambientValuesIterator->get());

                IFC_RETURN(dictionary->GetKeyForResourceResolutionNoRef(resourceKey,
                    Resources::LookupScope::LocalOnly,
                    &keyNoRef,
                    &dictionaryReadFrom));
            }

            // We only searched in the dictionaries local scope, if we haven't found the value at this point, then fallback to searching in the global
            // resources and app dictionary. We don't fallback and get the key if this is inside a template. This will happen when we resolve using
            // the cached context
            if (!keyNoRef)
            {
                IFC_RETURN(FallbackGetKeyForResourceResolutionNoRef(
                    resolutionContext->GetContext(),
                    resourceKey,
                    Resources::LookupScope::All,
                    &keyNoRef,
                    &dictionaryReadFrom));
            }

            xref_ptr<CResourceDictionary> dictionaryForThemeReference;
            if (keyNoRef && resourceType == ResourceTypeTheme)
            {
                dictionaryForThemeReference = GetDictionaryForThemeReference(resolutionContext->GetContext(), dictionary, dictionaryReadFrom.get());
            }
            resolvedResource = ResolvedResource(keyNoRef, std::move(dictionaryReadFrom), std::move(dictionaryForThemeReference));
        }

        // If this is part of a template, we walked up the template first to try and resolve the resource, if it wasn't found
        // used the cached template content
        if (!resolvedResource.Value && resolutionContext->IsTemplateNamescopeMember())
        {
            // Try to see if we have cached parser context for things like template content, VSM, or Styles
            bool hadContextIgnore;
            IFC_RETURN(TryResolveResourceFromCachedParserContext(resolutionContext, resourceKey, resourceType, resolvedResource, &hadContextIgnore));
        }

        // Add Style in-case it existed in markup, but wasn't referenced.
        // We need to styles with their owning dictionary if XamlDiag is enabled
        AddStyleContext(do_pointer_cast<CStyle>(resolvedResource.Value), resolvedResource.DictionaryReadFrom.get(), false);


        return S_OK;
    }

    const _Check_return_ HRESULT ResourceResolver::TryResolveResourceFromCachedParserContext(
        _In_ CDependencyObject* resolutionContext,
        const xstring_ptr& resourceKey,
        ResourceType resourceType,
        ResolvedResource& resolvedResource,
        _Out_ bool* hadContext)
    {
        *hadContext = false;
        xref_ptr<CDependencyObject> root;
        auto savedContext = TryFindSavedContext(resolutionContext, &root);

        if (!savedContext && IsTemplate(root.get()))
        {
            // If the template was created on the fly, try to see if the parent dictionary or style
            // has cached context. If template is defined inline on the element, then we'll do a runtime tree
            // walk
            auto parent = Diagnostics::GetParentForElementStateChanged(root.get());
            savedContext = TryFindSavedContext(parent, &root);
        }

        if (savedContext && root)
        {
            *hadContext = true;
            // Get an object writer from the template content's saved context. We can use this to resolve
            // the resource like we do during parse time.
            std::shared_ptr<ObjectWriterContext> objectWriterContext;
            IFC_RETURN(GetObjectWriterFromSavedContext(savedContext, root, objectWriterContext));
            IFC_RETURN(ResolveResourceImpl(
                resourceKey,
                objectWriterContext->get_MarkupExtensionContext(),
                resolutionContext->GetContext(),
                true, // checkThemeResources
                resourceType,
                resolvedResource));
        }

        return S_OK;
    }


    void ResourceResolver::GetAmbientValuesRuntime(
        CDependencyObject* resolutionContext,
        _Out_ AmbientValuesVector& ambientValues)
    {
        // Build the markup tree looking for all resource dictionaries
        CDependencyObject* templatedParent = nullptr;
        if (resolutionContext->IsTemplateNamescopeMember())
        {
            // If the object is part of a template, we never want to go past the templated parent looking for resource
            // dictionaries. This means that the saved template content didn't have the resource, and so we should look
            // to see if it was contained inside the template content itself.
            templatedParent = resolutionContext->GetTemplatedParent();
            xref_ptr<CDependencyObject> eventRoot;
            if (!TryFindSavedContext(resolutionContext, &eventRoot))
            {
                // If the template is created on the fly, then there is no saved context, so grab all the ambient values w.r.t
                // the template as the resolution context. If this is contained in a dictionary instantiated through ResourceDictionary.Source,
                // then this will stop at the root <ResourceDictionary> tag, if it's defined on a Page
                if (IsTemplate(eventRoot.get()))
                {
                    GetAmbientValuesRuntime(eventRoot.get(), ambientValues);
                }
            }
        }

        auto parent = Diagnostics::GetParentForElementStateChanged(resolutionContext);
        while (parent && parent != templatedParent)
        {
            auto parentFE = do_pointer_cast<CFrameworkElement>(parent);
            auto parentDictionary = parentFE ? parentFE->GetResourcesNoCreate() : do_pointer_cast<CResourceDictionary>(parent);
            if (parentDictionary)
            {
                ambientValues.m_vector.push_back(xref_ptr<CDependencyObject>(parentDictionary));
            }

            // For templates defined in a user control, don't escape the scope of the UserControl. It's impossible for us
            // to tell what markup is actually being edited. So we'll assume that it is highly unlikely for someone to do this
            // in MainPage.xaml:
            // <Page x:Class="local:MainPage">
            //    <local:MyUserControl >
            //       <UserControl.Resources>
            //          <DataTemplate x:Key="T1"/>
            //       <UserControl.Resources>
            //       <ItemsControl ItemTemplate="{StaticResource T1}"/>
            //    </local:MyUserControl>
            // </Page>
            // Rather, it's most likely they are making this change inside MyUserControl.xaml:
            // <UserControl x:Class="MyUserControl">
            //   <UserControl.Resources>
            //      <DataTemplate x:Key="T1"/>
            //   <UserControl.Resources>
            //   <ItemsControl ItemTemplate="{StaticResource T1}"/>
            // </UserControl>
            // This is ultimately for correctness in trying to match the parse time resolution behavior, as if we allow
            // ourselves to look outside the scope of the UserControl, we will incorrectly allow this scenario to work:
            //  - MainPage.xaml:
            // <Page x:Class="local:MainPage">
            //    <Page.Resources>
            //      <DataTemplate x:Key="T1"/>
            //    </Page.Resources>
            //    <local:MyUserControl />
            // </Page>
            //  - MyUserControl.xaml:
            // <UserControl x:Class="MyUserControl">
            //   <ItemsControl ItemTemplate="{StaticResource T1}"/>
            // </UserControl>
            if (IsTemplate(resolutionContext) && parent->OfTypeByIndex<KnownTypeIndex::UserControl>())
            {
                templatedParent = parent;
            }
            else
            {
                parent = Diagnostics::GetParentForElementStateChanged(parent);
            }
        }
    }

    _Check_return_ HRESULT ResourceResolver::FindResolvedValueNoRefImpl(
        _In_ CDependencyObject* depObj,
        const xstring_ptr& themeResourceKey,
        bool searchForOverrides,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom,
        _Out_opt_ CDependencyObject** resultNoRef)
    {
        CDependencyObject* tmpResultNoRef = nullptr;

        IFC_RETURN(ScopedResources::GetOrCreateOverrideForVisualTree(
            depObj,
            themeResourceKey,
            searchForOverrides,
            &tmpResultNoRef,
            dictionaryReadFrom));

        if (resultNoRef != nullptr) *resultNoRef = tmpResultNoRef;
        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::GetAmbientValues(
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Out_ AmbientValuesVector& vecAmbientValues)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        IFC_RETURN(spServiceProviderContext->GetSchemaContext(spSchemaContext));

        XamlPropertyToken propertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::FrameworkElement_Resources);
        XamlTypeToken typeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::ResourceDictionary);

        std::shared_ptr<XamlProperty> spXamlProperty;
        IFC_RETURN(spSchemaContext->GetXamlProperty(propertyToken, typeToken, spXamlProperty));
        IFCEXPECT_RETURN(spXamlProperty);

        std::shared_ptr<XamlType> spTypeToFind;
        IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spTypeToFind));
        IFCEXPECT_RETURN(spTypeToFind);

        std::shared_ptr<XamlType> spEmptyType;
        IFC_RETURN(spServiceProviderContext->GetAllAmbientValues(
            spEmptyType, spXamlProperty, spTypeToFind, CompressedStackCacheHint::Resources, vecAmbientValues));

        return S_OK;
    }

    void ResourceResolver::RegisterResourceDependency(
            const xstring_ptr& strResourceKey,
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            const xref_ptr<CResourceDictionary>& dictionary,
            ResourceType resourceType,
            _In_ CStyle* optimizedStyleParent,
            KnownPropertyIndex stylePropertyIndex)
    {
        // For XamlDiagnostics: register a dependency on the  resource if we got an object and its dictionary

        if (dictionary)
        {
            // This method shows up on traces, so call it only if necessary.
            if (!DirectUI::DXamlServices::ShouldStoreSourceInformation())
            {
                return;
            }

            auto resourceGraph = Diagnostics::GetResourceGraph();

            std::shared_ptr<XamlProperty> spXamlTargetProperty;
            std::shared_ptr<XamlQualifiedObject> spXamlTargetObject;

            spServiceProviderContext->GetMarkupExtensionTargetProperty(spXamlTargetProperty);
            spServiceProviderContext->GetMarkupExtensionTargetObject(spXamlTargetObject);

            if (spXamlTargetObject != nullptr)
            {
                // If the target object is a CControlTemplate (or CDataTemplate), then register it specially. This means that the template content
                // is pre-resolving the resources and we want to ensure we can correctly register resource dependencies
                // for items inside the template at a later time. We check for the template first because we could be inside a Setter.Value
                // (see example below) in which case the target property would be FrameworkElement_Template. In this scenario we
                // still want to register the control template dependency and ignore the property since we will have future resolutions. For
                // the example below, both 'ContentBackground' and 'ContentBrush' would be pre-resolved and cached. Future instantiations of the
                // template would then try to resolve these resources to the Border with the correct properties set.

                // <Style TargetType='ToolTip'>
                //     <Setter Property='Template'>
                //         <Setter.Value>
                //             <ControlTemplate TargetType='ToolTip'>
                //                 <Border x:Name='border'
                //                     Padding='6'
                //                     Background='{StaticResource ContentBackground}'
                //                     BorderBrush='{StaticResource ContentBrush}'
                //                     BorderThickness='1'>
                //                 </Border>
                //             </ControlTemplate>
                //         </Setter.Value>
                //     </Setter>
                // </Style>

                if (IsTemplate(spXamlTargetObject->GetDependencyObject()))
                {
                    resourceGraph->RegisterControlTemplateDependency(
                        static_cast<CControlTemplate*>(spXamlTargetObject->GetDependencyObject()),
                        dictionary.get(),
                        strResourceKey);
                }
                else if (spXamlTargetProperty != nullptr)
                {
                    // If we got the StaticResource's value from a resource dictionary successfully, register the dependency
                    // on this resource
                    resourceGraph->RegisterResourceDependency(
                        spXamlTargetObject->GetDependencyObject(),
                        spXamlTargetProperty->get_PropertyToken().GetHandle(),
                        dictionary.get(),
                        strResourceKey,
                        resourceType);
                }
            }
            else if (optimizedStyleParent)
            {
                // If we're dealing with an optimized style where Setters use  a static or theme resource,
                // there isn't actually a Setter DO in an optimized style, so we can't register normally here.
                // Instead of registering the non-existant Setter DO as the dependency, we register the Style
                // itself and let DiagnosticsInterop::SetPropertyValue correctly update the Setter of the Style instead of the Style.
                resourceGraph->RegisterResourceDependency(optimizedStyleParent, stylePropertyIndex, dictionary.get(), strResourceKey, resourceType);
            }
        }
    }

    std::shared_ptr<XamlSavedContext> ResourceResolver::TryFindSavedContext(
        _In_ CDependencyObject* depObj,
        _Out_ xref_ptr<CDependencyObject>* rootInstance)
    {
        std::shared_ptr<XamlSavedContext> savedContext;
        if (depObj->IsTemplateNamescopeMember())
        {
            // If the object is part of a template, get the TemplateContent and return the saved context
            // from the TemplateContent.
            auto templatedParent = do_pointer_cast<CFrameworkElement>(depObj->GetTemplatedParent());
            if (templatedParent)
            {
                auto controlTemplate = templatedParent->GetTemplate();
                if (controlTemplate && controlTemplate->m_pTemplateContent)
                {
                    *rootInstance = controlTemplate->m_pTemplateContent->GetSavedEventRoot();
                    if (*rootInstance != controlTemplate)
                    {
                        savedContext = controlTemplate->m_pTemplateContent->GetSavedContext();
                    }
                }
            }
        }
        else
        {
            // Otherwise, see if this is in a VisualState and get the CustomRuntimeContext from that. This could be things
            // like Setters, Storyboards, or StateTriggers
            auto visualState = Diagnostics::DiagnosticsInterop::TryFindVisualState(depObj);
            CustomWriterRuntimeContext* runtimeContext = nullptr;
            if (visualState)
            {
                auto setterGroupCollection = CVisualStateManager2::GetGroupCollectionFromVisualState(visualState);
                runtimeContext = setterGroupCollection->GetCustomRuntimeContext();
            }
            else if (auto style = do_pointer_cast<CStyle>(depObj))
            {
                // If not insde a VisualState, see if the object passed in is a style, or a style setter and get
                // the cached runtime contxt from there.
                auto resourceGraph = Diagnostics::GetResourceGraph();
                if (!style)
                {
                    auto setter = do_pointer_cast<CSetter>(depObj);
                    if (setter && setter->IsStyleSetter())
                    {
                        auto parentCollection = checked_cast<CSetterBaseCollection>(setter->GetParentInternal(false /*publicParentOnly*/));
                        style = resourceGraph->GetOwningStyle(parentCollection);
                    }
                }

                if (style)
                {
                    // Found a style, get the cached context from that
                    runtimeContext = resourceGraph->GetCachedRuntimeContext(style);
                }
            }
            else if (auto dictionary = do_pointer_cast<CResourceDictionary>(depObj))
            {
                runtimeContext = dictionary->GetDeferredResourcesRuntimeContext();
            }

            if (runtimeContext)
            {
                savedContext = std::make_shared<XamlSavedContext>(
                    runtimeContext->GetSchemaContext(),
                    runtimeContext->GetObjectWriterStack(),
                    runtimeContext->GetBaseUri(),
                    xref_ptr<IPALUri>(), // DEPRECATED
                    runtimeContext->GetXbfHash());

                *rootInstance = runtimeContext->GetRootInstance();
            }
        }

        return savedContext;
    }

    _Check_return_ HRESULT ResourceResolver::GetObjectWriterFromSavedContext(
        const std::shared_ptr<XamlSavedContext>& savedContext,
        const xref_ptr<CDependencyObject>& rootInstance,
        _Out_ std::shared_ptr<ObjectWriterContext>& objectWriterContext)
    {
        SP_ObjectWriterStack spObjectWriterStack;
        IFC_RETURN(savedContext->get_Stack(spObjectWriterStack));

        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(savedContext->get_SchemaContext(schemaContext));

        std::shared_ptr<XamlQualifiedObject> qoEventRoot;
        XamlTypeToken token(tpkNative, rootInstance->GetTypeIndex());
        IFC_RETURN(XamlQualifiedObject::Create(rootInstance->GetContext(), token, rootInstance.get(), qoEventRoot));

        IFC_RETURN(ObjectWriterContext::Create(savedContext, qoEventRoot, true /*expandTemplate*/, nullptr /*spParentXBindConnector*/, objectWriterContext));

        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::FallbackGetKeyForResourceResolutionNoRef(
        _In_ CCoreServices *pCore,
        const xstring_ptr_view& resourceKey,
        Resources::LookupScope scope,
        _Outptr_ CDependencyObject** keyNoRef,
        _Out_ xref_ptr<CResourceDictionary>* dictionaryReadFrom)
    {
        CDependencyObject* foundKeyNoRef = nullptr;
        if (Resources::DoesScopeMatch(scope, Resources::LookupScope::GlobalTheme))
        {
            IFC_RETURN(CResourceDictionary::GetKeyFromGlobalThemeResourceNoRef(pCore,
                nullptr /* Not in a Resource Dictionary Context */,
                ResourceKey(resourceKey, false /* fKeyIsType */),
                &foundKeyNoRef,
                dictionaryReadFrom));
        }

        // if not found yet, look it up in App
        if (!foundKeyNoRef && pCore)
        {
            auto appDictionary = pCore->GetApplicationResourceDictionary();
            if (appDictionary)
            {
                xref_ptr<CResourceDictionary> appDictionaryReadFrom;
                IFC_RETURN(appDictionary->GetKeyForResourceResolutionNoRef(resourceKey,
                    Resources::LookupScope::LocalOnly,
                    &foundKeyNoRef,
                    &appDictionaryReadFrom));
                if (foundKeyNoRef)
                {
                    appDictionaryReadFrom->SetUseAppResourcesForThemeRef(true);
                    *dictionaryReadFrom = std::move(appDictionaryReadFrom);
                }
            }
        }

        *keyNoRef = foundKeyNoRef;

        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::FindCurrentDictionary(
        _In_ CResourceDictionary* dictionary,
        const xstring_ptr& key,
        bool isImplicitStyle,
        _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn)
    {
         CDependencyObject* keyNoRef = nullptr;

        // Looks in the dictionaries local dictionaries to see if the key can be found in there.
        if (isImplicitStyle)
        {
            IFC_RETURN(dictionary->GetImplicitStyleKeyNoRef(key, Resources::LookupScope::LocalOnly, &keyNoRef, &dictionaryFoundIn));
        }
        else
        {
            IFC_RETURN(dictionary->GetKeyForResourceResolutionNoRef(key, Resources::LookupScope::LocalOnly, &keyNoRef, &dictionaryFoundIn));
        }

        // If not found in a local dictionary, then walk up the tree
        if (!dictionaryFoundIn)
        {
            IFC_RETURN(FindCurrentDictionaryFromOwner(dictionary, key, isImplicitStyle, dictionaryFoundIn));
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::FindCurrentDictionaryFromOwner(
        _In_ CResourceDictionary* dictionary,
        const xstring_ptr& key,
        bool isImplicitStyle,
        _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn)
    {
        CDependencyObject* keyNoRef = nullptr;

        // Get the owner of the dictionary and see if we can walk up the tree and find where this would resolve to.
        // We start with the owner's parent because if we passed in the parent to the resource dictionary we'd just re-check the dictionary we just checked.
        // Owner can be null if we're in Application.Resources.  In that case we'll fall through to FallbackGetKeyForResourceResolutionNoRef anyway
        auto owner = dictionary->GetResourceOwnerNoRef();
        if (isImplicitStyle && owner)
        {
            IFC_RETURN(ResolveImplicitStyleKey(owner, key, nullptr, &dictionaryFoundIn));
        }
        else if (owner)
        {
            IFC_RETURN(FindResolvedValueNoRefImpl(owner, key, true, &dictionaryFoundIn));
        }

        // Still not found, look in Application and Global resources
        if (!dictionaryFoundIn)
        {
            IFC_RETURN(FallbackGetKeyForResourceResolutionNoRef(dictionary->GetContext(), key, Resources::LookupScope::All, &keyNoRef, &dictionaryFoundIn));
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::ResolveImplicitStyleKey(
        _In_ CFrameworkElement* element,
        bool isLeavingParentStyle,
        _Out_ xref_ptr<CStyle>* style,
        _Out_ ImplicitStyleProvider* provider,
        _Out_ xref::weakref_ptr<CUIElement>* parent)
    {
        return ResolveImplicitStyleKeyImpl(element, element->GetClassName(), isLeavingParentStyle, style, provider, parent);
    }

    _Check_return_ HRESULT ResourceResolver::ResolveImplicitStyleKey(
        _In_ CDependencyObject* startingPoint,
        const xstring_ptr& key,
        _Out_ xref_ptr<CStyle>* style,
        _Out_ xref_ptr<CResourceDictionary>* dictionaryFoundIn)
    {
        ImplicitStyleProvider provider = ImplicitStyleProvider::None;
        xref::weakref_ptr<CUIElement> parent;
        const bool isLeavingParentStyle = false;

        IFC_RETURN(ResolveImplicitStyleKeyImpl(startingPoint, key, isLeavingParentStyle, style, &provider, &parent, dictionaryFoundIn));
        return S_OK;
    }

    _Check_return_ HRESULT ResourceResolver::ResolveImplicitStyleKeyImpl(
        _In_ CDependencyObject* element,
        const xstring_ptr& className,
        bool isLeavingParentStyle,
        _Out_ xref_ptr<CStyle>* style,
        _Out_ ImplicitStyleProvider* provider,
        _Out_ xref::weakref_ptr<CUIElement>* parent,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryFoundIn)
    {
        // Walk parent chain to find resource whose key is the full name of this class
        // unless it's being cleaned up.
        CDependencyObject* resourceNoRef = nullptr;
        CDependencyObject * const templatedParent = element->GetTemplatedParent();
        auto core = element->GetContext();
        const bool isControl = element->OfTypeByIndex<KnownTypeIndex::Control>();
        xref_ptr<CResourceDictionary> dictionaryFoundInInternal;

        if (!isLeavingParentStyle)
        {
            CDependencyObject *current = element;
            while (current)
            {
                xref_ptr<CResourceDictionary> resourceDictionary;

                auto currentAsFe = do_pointer_cast<CFrameworkElement>(current);
                if (currentAsFe)
                {
                    resourceDictionary = currentAsFe->GetResourcesNoCreate();
                }

                if (resourceDictionary && resourceDictionary->HasImplicitStyle())
                {

                    auto scope = Resources::LookupScope::LocalOnly;

                    // There is a case in designer(by specifying the generic Xaml file path) that loads the generic Xaml
                    // file directly which includes the built-in style, theme resources and framework explicit styles.
                    // In case of the loading the generic.xaml file directly, the global theme resource's built-in style
                    // shouldn't be retrieved here for looking the implicit style in the resource dictionary.
                    // Otherwise, the implicit style logic will be broken by applying the built-in style accidentally.
                    // bShouldCheckThemeResources flag is FALSE if the generic Xaml file path is specified from hosting.
                    if (!core || !core->GetIsUsingGenericXamlFilePath())
                    {
                        scope |= Resources::LookupScope::GlobalTheme;
                    }

                    IFC_RETURN(resourceDictionary->GetImplicitStyleKeyNoRef(className,
                        scope,
                        &resourceNoRef,
                        &dictionaryFoundInInternal));

                    if (resourceNoRef)
                    {
                        if (current == element)
                        {
                            *provider = ImplicitStyleProvider::Self;
                        }
                        else
                        {
                            *provider = ImplicitStyleProvider::Parent;
                            // Create a WeakRef to the implicit style parent for clean up purposes
                            *parent = xref::get_weakref(do_pointer_cast<CUIElement>(current));
                        }
                        break;
                    }
                }

                // Resource not found, look in parent.  Check logical parent first and fallback on visual parent.
                current = current->GetInheritanceParentInternal(TRUE/*bLogicalParent*/);

                // For non-Control elements with templated parent we should stop looking
                // an implicit style on templated root (not templated parent as WPF does).
                if (!isControl && (current == templatedParent))
                {
                    break;
                }
            }
        }

        // if no style found yet try to lookup Application.Resources
        // 1. For all controls.
        // 2. For non-controls which are not template parts.
        if (!resourceNoRef && (isControl || templatedParent == nullptr))
        {
            // Let's look in the Application Domain
            if (core)
            {
                auto appResources = core->GetApplicationResourceDictionary();
                if (appResources && appResources->HasImplicitStyle())
                {
                    // Look in the application domain, any of it's merged/theme dictionaries, as well as the global resources.
                    IFC_RETURN(appResources->GetImplicitStyleKeyNoRef(className, Resources::LookupScope::All, &resourceNoRef, &dictionaryFoundInInternal));
                }
            }
            if (resourceNoRef)
            {
                if (element->IsActive())
                {
                    *provider = ImplicitStyleProvider::AppWhileInTree;
                }
                else
                {
                    *provider = ImplicitStyleProvider::AppWhileNotInTree;
                }
            }
        }

        //We need to associate implicit styles with their owning dictionary if XamlDiag is enabled
        AddStyleContext(do_pointer_cast<CStyle>(resourceNoRef), dictionaryFoundInInternal.get(), true);

        if (dictionaryFoundIn != nullptr && dictionaryFoundInInternal != nullptr)
        {
            *dictionaryFoundIn = std::move(dictionaryFoundInInternal);
        }

        if (style != nullptr)
        {
            xref_ptr<CStyle> resourceStyle(do_pointer_cast<CStyle>(resourceNoRef));
            *style = std::move(resourceStyle);
        }
        return S_OK;
    }

    void ResourceResolver::AddStyleContext(
        _In_ CStyle* style,
        _In_ CResourceDictionary* foundIn,
        bool isImplicitStyle)
    {
        if (foundIn &&
            style &&
            DirectUI::DXamlServices::ShouldStoreSourceInformation())
        {
            auto resourceGraph = Diagnostics::GetResourceGraph();
            resourceGraph->AddStyleContext(style, foundIn, isImplicitStyle);
        }
    }

    xref_ptr<CResourceDictionary> ResourceResolver::GetDictionaryForThemeReference(
        _In_ CCoreServices* coreServices,
        _In_opt_ CResourceDictionary* ambientDictionary,
        _In_opt_ CResourceDictionary* dictionaryReadFrom)
    {
        xref_ptr<CResourceDictionary> dictionaryForThemeReference;
        // Find out the ambient dictionary to store for a theme reference. Note that this is mostly for compat with
        // pre-RS1 apps. In RS1+, we do a tree walk on theme updates.
        if (dictionaryReadFrom)
        {
            if (dictionaryReadFrom->IsSystemColorsDictionary())
            {
                // For system colors, we can use the dictionary itself. These colors get destroyed and re-created on theme changes.
                dictionaryForThemeReference = dictionaryReadFrom;
            }
            else if (dictionaryReadFrom->IsGlobal() && dictionaryReadFrom->IsThemeDictionary())
            {
                // For a theme dictionary in the global theme dictionaries we want to store the non-theme specific dictionary.
                // If this theme refernce is created while loading theme resources, then CCoreServices::GetThemeResources will
                // return null. This is ok, as the ambientDictionary passed in will be the resource dictionary we want to use
                if (!coreServices->IsLoadingGlobalThemeResources())
                {
                    dictionaryForThemeReference = coreServices->GetThemeResources();
                }
            }
            else if (dictionaryReadFrom->UseAppResourcesForThemeRef())
            {
                // If the dictionary was found inside App.xaml, then use application resources
                dictionaryForThemeReference = coreServices->GetApplicationResourceDictionary();
            }
        }

        if (!dictionaryForThemeReference)
        {
            // Otherwise, fall back to the ambient dictionary found
            dictionaryForThemeReference = ambientDictionary;
        }

        return dictionaryForThemeReference;
    }

    bool ResourceResolver::IsTemplate(_In_opt_ const CDependencyObject* object)
    {
        auto typeIndex = object ? object->GetTypeIndex() : KnownTypeIndex::UnknownType;
        // Even though CDataTemplate derives from CControlTemplate,
        // it fails OfTypeByIndex<KnownTypeIndex::ControlTemplate>()
        // so we need to check both
        return typeIndex == KnownTypeIndex::ControlTemplate || typeIndex == KnownTypeIndex::DataTemplate;
    }
}