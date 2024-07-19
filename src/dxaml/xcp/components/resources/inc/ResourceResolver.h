// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "base\inc\xref_ptr.h"
#include <Resources.h>
#include "ParserTypeDefs.h"

class CCoreServices;
class XamlServiceProviderContext;
class xstring_ptr;
class CThemeResource;
class ObjectWriterContext;
class CTemplateContent;
class CFrameworkElement;
enum ResourceType;   // defined in XamlOM.WinUI.h
struct ResolvedResource;

enum class ImplicitStyleProvider : uint8_t;

namespace xref {
    template <typename T>
    class weakref_ptr;
}
namespace Diagnostics
{
    class DiagnosticsInterop;
}

namespace Resources {

    struct ResolvedResource
    {
        ResolvedResource() = default;

        explicit ResolvedResource(
            _In_ CDependencyObject* value,
            const xref_ptr<CResourceDictionary>&& dictionary,
            const xref_ptr<CResourceDictionary>&& dictionaryForThemeReference)
            : Value(value)
            , DictionaryReadFrom(std::move(dictionary))
            , DictionaryForThemeReference(std::move(dictionaryForThemeReference))
        {
        }

        xref_ptr<CDependencyObject>     Value;
        xref_ptr<CResourceDictionary>   DictionaryReadFrom;
        xref_ptr<CResourceDictionary>   DictionaryForThemeReference;
    };

    class ResourceResolver final
    {
        friend class Diagnostics::DiagnosticsInterop;
    public:
        ResourceResolver() = delete;

        // Resolves the resource at parse time. Used by StaticResourceExtension and ThemeResourceExtension

        static const _Check_return_ HRESULT ResolveStaticResource(
            const xstring_ptr& strResourceKey,
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _In_ CCoreServices *pCore,
            bool bShouldCheckThemeResources,
            _Out_ ResolvedResource& resolveResource,
            _In_opt_ CStyle* optimizedStyleParent = nullptr,
            KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty);

        static const _Check_return_ HRESULT ResolveThemeResource(
            const xstring_ptr& strResourceKey,
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _In_ CCoreServices *pCore,
            bool bShouldCheckThemeResources,
            _Out_ ResolvedResource& resolveResource,
            _In_opt_ CStyle* optimizedStyleParent = nullptr,
            KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty);

        // Resolves resources after parsing. Used during theme changes if we need to walk up the tree to
        // find the value
        static _Check_return_ HRESULT FindNextResolvedValueNoRef(
            _In_ CDependencyObject* depObj,
            _In_ CThemeResource* themeResource,
            bool searchForOverrides,
            _Out_ CDependencyObject** resultNoRef);

        // Resolves resources after parsing. Called from XamlDiagnostics when resolving resources.
        // XamlDiagnostics is interested in knowing which dictionary the value was found. Calls the
        // implementation of FindNextResolvedValue, and then falls back to the global theme resources
        // and application
        static const _Check_return_ HRESULT ResolveResourceRuntime(
            _In_ CDependencyObject* depObj,
            _In_opt_ CResourceDictionary* dictionaryFoundIn,
            const xstring_ptr& resourceKey,
            ResourceType resourceType,
            _Out_ ResolvedResource& resolvedResource);

        // Finds the dictionary where this would resolve to. Used when adding items to a dictionary, but should
        // be called before adding the item, otherwise it will just return the passed in dictionary. Will
        // check if any of the merged or theme dictionaries in the passed in dictionary contain the matching key.
        // If not, it will then call FindCurrentDictionaryFromOwner to walk up the tree
        static _Check_return_ HRESULT FindCurrentDictionary(
            _In_ CResourceDictionary* dictionary,
            const xstring_ptr& key,
            bool isImplicitStyle,
            _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn);

        // Walks up the tree to find a dictionary where this would resolve to. Starts at the FrameworkElement that
        // owns the dictionary.
        static _Check_return_ HRESULT FindCurrentDictionaryFromOwner(
            _In_ CResourceDictionary* dictionary,
            const xstring_ptr& key,
            bool isImplicitStyle,
            _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn);

        // Resolves the implicit style for the passed in framework element and returns
        // the style, the provider kind, and the parent iff provider == ImplicitStyleProvider_Parent
        static _Check_return_ HRESULT ResolveImplicitStyleKey(
            _In_ CFrameworkElement* element,
            bool isLeavingParentStyle,
            _Out_ xref_ptr<CStyle>* style,
            _Out_ ImplicitStyleProvider* provider,
            _Out_ xref::weakref_ptr<CUIElement>* parent);

        // Resolves the implicit style for the key passed in with the object in the tree
        // as the starting point for the walk to find where the implicit style for the class
        // passed in is defined.
        static _Check_return_ HRESULT ResolveImplicitStyleKey(
            _In_ CDependencyObject* startingPoint,
            const xstring_ptr& className,
            _Out_ xref_ptr<CStyle>* style,
            _Out_ xref_ptr<CResourceDictionary>* dictionaryFoundIn);

        static _Check_return_ HRESULT GetAmbientValues(
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ AmbientValuesVector& ambientValues);

    private:
        static _Check_return_ HRESULT ResolveImplicitStyleKeyImpl(
            _In_ CDependencyObject* element,
            const xstring_ptr& className,
            bool isLeavingParentStyle,
            _Out_ xref_ptr<CStyle>* style,
            _Out_ ImplicitStyleProvider* provider,
            _Out_ xref::weakref_ptr<CUIElement>* parent,
            _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryFoundIn = nullptr);

        // Fallback for getting a resource during resource resolution. Checks in the global theme resources
        // and application resources
        static _Check_return_ HRESULT FallbackGetKeyForResourceResolutionNoRef(
            _In_ CCoreServices *pCore,
            _In_ const xstring_ptr_view& strKey,
            _In_ Resources::LookupScope scope,
            _Outptr_ CDependencyObject** keyNoRef,
            _Out_ xref_ptr<CResourceDictionary>* dictionaryResourcesReadFrom);

        static _Check_return_ HRESULT FindResolvedValueNoRefImpl(
            _In_ CDependencyObject* depObj,
            const xstring_ptr& themeResourceKey,
            bool searchForOverrides,
            _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom = nullptr,
            _Out_opt_ CDependencyObject** resultNoRef = nullptr);
        

        static const _Check_return_ HRESULT ResolveResourceImpl(
            const xstring_ptr& strResourceKey,
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _In_ CCoreServices *pCore,
            bool bShouldCheckThemeResources,
            ResourceType type,
            _Out_ ResolvedResource & resolvedResource);

        static void GetAmbientValuesRuntime(
            _In_ CDependencyObject* resolutionContext,
            _Out_ AmbientValuesVector& ambientValues);

        static void RegisterResourceDependency(
            const xstring_ptr& strResourceKey,
            const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            const xref_ptr<CResourceDictionary>& dictionary,
            ResourceType resourceType,
            _In_ CStyle* optimizedStyleParent,
            KnownPropertyIndex stylePropertyIndex);

        static void AddStyleContext(
            _In_ CStyle* style,
            _In_ CResourceDictionary* foundIn,
            bool isImplicitStyle);

        static const _Check_return_ HRESULT TryResolveResourceFromCachedParserContext(
            _In_ CDependencyObject* resolutionContext,
            const xstring_ptr& resourceKey,
            ResourceType resourceType,
            _Out_ ResolvedResource& resolvedResource,
            _Out_ bool* hadContext);

        static std::shared_ptr<XamlSavedContext> TryFindSavedContext(
            _In_ CDependencyObject* depObj,
            _Out_ xref_ptr<CDependencyObject>* rootInstance);

        static _Check_return_ HRESULT GetObjectWriterFromSavedContext(
            const std::shared_ptr<XamlSavedContext>& savedContext,
            const xref_ptr<CDependencyObject>& rootInstance,
            _Out_ std::shared_ptr<ObjectWriterContext>& objectWriterContext);

        static xref_ptr<CResourceDictionary> GetDictionaryForThemeReference(
            _In_ CCoreServices* coreServices,
            _In_opt_ CResourceDictionary* ambientDictionary,
            _In_opt_ CResourceDictionary* dictionaryReadFrom);

        static bool IsTemplate(_In_opt_ const CDependencyObject* object);
        static bool s_isLoadingThemeResources;
    };
}