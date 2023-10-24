// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ICustomWriterRuntimeDataReceiver.h>
#include <resources\inc\ResourceDictionary2.h>
#include <stack_vector.h>
#include "collection\inc\DOCollection.h"
#include "MarkupExtension.h"

class XamlOptimizedNodeList;
class XamlSavedContext;
class ObjectWriter;
class CStyle;
class CustomWriterRuntimeContext;

namespace Theming {
    enum class Theme : uint8_t;
}

namespace Diagnostics {
    class DiagnosticsInterop;
    class ResourceGraph;
}

namespace Resources {
    enum class LookupScope : unsigned char
    {
        // Only lookup the key in the current dictionary
        SelfOnly        = 0x0,
        // LocalOnly is a mask for the (currently) 2 types of local
        // dictionaries.
        LocalOnly       = 0x3,
        Merged          = 0x1,
        LocalTheme      = 0x2,

        // GlobalTheme dictionaries
        GlobalTheme     = 0x4,

        // Will lookup in all local (to the current dictionary) and global dictionaries for the key
        All             = static_cast<unsigned char>(LocalOnly) | static_cast<unsigned char>(GlobalTheme),
    };
    DEFINE_ENUM_FLAG_OPERATORS(LookupScope);

    inline bool DoesScopeMatch(_In_ LookupScope definedScope, _In_ LookupScope scopeToMatch)
    {
        return (definedScope & scopeToMatch) == scopeToMatch;
    }

    namespace details
    {
        class ResourceKeyCache
        {
        public:
            bool Exists(const ResourceKey& key);
            void Add(const ResourceKey& key);
            void Remove(const ResourceKey& key);
            void Clear();

        private:
            void ShiftRight(uint32_t index);
            void ShiftRight();

            static constexpr uint32_t c_cacheSize = 8;
            uint32_t m_count = 0;
            std::array<ResourceKeyStorage, c_cacheSize> m_cache {};
        };
    }
}

// A collection of ResourceDictionaries
class CResourceDictionaryCollection final : public CDOCollection
{
private:
    CResourceDictionaryCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
    DECLARE_CREATE(CResourceDictionaryCollection);

public:
    // CCollection overrides
    _Check_return_ HRESULT Clear() override;

    // CDOCollection overrides
    _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *pDO) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex) override;

    void SetResourceOwner(_In_opt_ CDependencyObject* const pResourceOwner);
    CDependencyObject* GetResourceOwnerNoRef() const;
    _Check_return_ bool HasImplicitStyle();

    _Check_return_ HRESULT InvalidateImplicitStyles();

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    KnownTypeIndex GetTypeIndex() const override;

private:
    _Check_return_ HRESULT ClearNoInvalidationOfImplicitStyles();

private:
    // weak reference to resource owner.
    CDependencyObject* m_pResourceOwner = nullptr;
};

class CDeferredKeys;
class CustomWriterRuntimeData;
class CustomWriterRuntimeContext;
class CResourceDictionary2;

// A dictionary of resources (arbitrary objects) indexed by a key name (string).
class CResourceDictionary
    : public CDOCollection
    , public ICustomWriterRuntimeDataReceiver
{
    // Allow diagnostics to access private methods so that we can add dictionary items at runtime where
    // we normally wouldn't be able to. This enables live markup editing as well as core designer scenarios.
    friend class Diagnostics::DiagnosticsInterop;
    friend class Diagnostics::ResourceGraph;

protected:
    CResourceDictionary(_In_ CCoreServices *pCore);

public:
    ~CResourceDictionary() override;

    // Creation method
    DECLARE_CREATE(CResourceDictionary);

    // Creates a resource dictionary marked with the m_isSystemColorsDictionary set
    static HRESULT CreateSystemColors(
        _In_ CCoreServices* coreServices,
        _Outptr_ CResourceDictionary** systemDictionary);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT Add(
        _In_ const xstring_ptr& strKey,
        _In_ CValue *pValue,
        _Out_opt_ CValue *pResult,
        _In_ bool fKeyIsType)
    {
        return Add(
            ResourceKey(strKey, !!fKeyIsType),
            pValue,
            pResult,        // pResult
            false,          // keyIsOverride
            false,          // fIgnoreAllowItemsFlag
            false           // undeferringKey
        );
    }

    // Once there's more than one override kind, methods dealing with overrides should be refactored
    // to an intermediate class deriving from CResourceDictionary.

    _Check_return_ HRESULT AddOverride(
        const xstring_ptr& strKey,
        _In_ CValue *pValue,
        _Out_opt_ CValue *pResult)
    {
        ASSERT(AllowsResourceOverrides());

        return Add(
            ResourceKey(strKey, false),
            pValue,
            pResult,        // pResult
            true,           // keyIsOverride
            false,          // fIgnoreAllowItemsFlag
            false           // undeferringKey
        );
    }

    _Check_return_ HRESULT GetLocalOverrideNoRef(
        const xstring_ptr_view& strKey,
        _Outptr_result_maybenull_ CDependencyObject** resultDO,
        _Outptr_result_maybenull_ CResourceDictionary** resultDict);

    bool AllowsResourceOverrides() const;

    bool HasPotentialOverrides() const;

private:
    _Check_return_ HRESULT Add(
        const ResourceKey& key,
        _In_ CValue *pValue,
        _Out_opt_ CValue *pResult,
        bool keyIsOverride,
        bool fIgnoreAllowItemsFlag,
        bool undeferringKey);

public:
    virtual _Check_return_ HRESULT Remove(
        _In_ const xstring_ptr& strKey,
        _In_ bool fKeyIsType = false);

    bool HasKey(const xstring_ptr& key, bool isImplicitKey) const;

#pragma region Resource Lookup Methods
    // Get's the key for an implicit style. Looks for the style in the defined
    // scope that is passed in.
    _Check_return_ HRESULT GetImplicitStyleKeyNoRef(
        const xstring_ptr_view& strKey,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject **ppDO,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom = nullptr);

    // Default lookup method for getting a key from a ResourceDictionary. Caller is
    // agnostic as to which dictionary the key was found in. This will search the
    // entire chain of dictionaries (Theme, Merged, Global Theme) for the key.
    _Check_return_ HRESULT GetKeyNoRef(
        const xstring_ptr_view& strKey,
        _Outptr_ CDependencyObject **ppDO);

    // Overload to GetKeyNoRef, takes a resourceLookupScope parameter which allows
    // the caller to specify the scope of the resource lookup if the key is not
    // first found in the current dictionary.
    _Check_return_ HRESULT GetKeyNoRef(
        const xstring_ptr_view& strKey,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject **ppDO);

    // Gets the key and dictionary that the key was found in. Called during resource
    // resolution when the caller is interested in knowing which dictionary the key
    // was found in.
    _Check_return_ HRESULT GetKeyForResourceResolutionNoRef(
        const xstring_ptr_view& strKey,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject** keyNoRef,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryResourcesReadFrom);

    _Check_return_ HRESULT GetKeyForResourceResolutionNoRef(
        const ResourceKey& key,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject** keyNoRef,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryResourcesReadFrom);

    _Check_return_ HRESULT GetKeyAtIndex(
        _In_ XINT32 index,
        _Out_ CValue * pKey,
        _Out_ bool * keyIsImplicitStyle,
        _Out_ bool * keyIsType);

    _Check_return_ HRESULT GetKeyWithOverrideNoRef(
        const xstring_ptr_view& keyName,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject **ppDO);

    static _Check_return_ HRESULT GetKeyFromGlobalThemeResourceNoRef(
        _In_ CCoreServices *pCore,
        _In_ CResourceDictionary *pSkipDictionary,
        const ResourceKey& key,
        _Outptr_ CDependencyObject** ppDO,
        _Out_opt_ xref_ptr<CResourceDictionary>* themeResourcesReadFrom = nullptr);

private:
    _Check_return_ HRESULT GetKeyNoRefImpl(
        const ResourceKey& key,
        Resources::LookupScope resourceLookupScope,
        _Outptr_ CDependencyObject **ppDO,
        _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom = nullptr);

    _Check_return_ HRESULT GetKeyFromThemeDictionariesNoRef(
        const ResourceKey& key,
        _Outptr_ CDependencyObject** ppDO,
        _Out_opt_ xref_ptr<CResourceDictionary>* ppThemeResourcesReadFrom = nullptr);

    static _Check_return_ HRESULT GetKeyOverrideFromApplicationResourcesNoRef(
        _In_ CCoreServices *pCore,
        const ResourceKey& key,
        _Inout_ CDependencyObject** ppDO,
        _Out_opt_ xref_ptr<CResourceDictionary>* resourcesReadFrom);

#pragma endregion Resource Lookup Methods
public:
    _Check_return_ bool HasImplicitStyle();

    _Check_return_ HRESULT GetUndeferredImplicitStyles(Jupiter::stack_vector<CStyle*, 8>* styles);

    // override from CDOCollection
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

#pragma region CCollection overrides

    // Code coverage note: this is necessary for x:Named resources to be reachable via FindName;
    // however, it's not obvious why code coverage flags it as a missed block
    bool ShouldEnsureNameResolution() override { return true; }
    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Clear() override;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;

    _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *pDO) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex) override;
    XUINT32 GetCount() const override;

    _Check_return_ void * GetItemWithAddRef(UINT32 index) override;

#pragma endregion CCollection overrides

    void SetResourceOwner(_In_opt_ CDependencyObject* const pResourceOwner);
    CDependencyObject* GetResourceOwnerNoRef() const;

    void MarkIsThemeDictionaries()
    {
        m_bIsThemeDictionaries = true;
    }

    void MarkIsThemeDictionary()
    {
        m_isThemeDictionary = true;
    }

    void MarkIsGlobal()
    {
        m_isGlobal = true;
    }

    void MarkAllIsGlobal();

    bool IsGlobal() const { return m_isGlobal; }

    void SetUseAppResourcesForThemeRef(_In_ bool value)
    {
        m_useAppResourcesForThemeRef = value;
    }

    bool UseAppResourcesForThemeRef() const { return m_useAppResourcesForThemeRef; }

    bool IsGlobalThemeDictionaries() const { return m_isGlobal && m_bIsThemeDictionaries; }

    bool IsReadOnlyDictionary() const { return m_isSystemColorsDictionary || IsGlobalThemeDictionaries();  }

    bool IsThemeDictionary() const { return m_isThemeDictionary; }

    bool IsSystemColorsDictionary() const { return m_isSystemColorsDictionary; }

    bool IsThemeDictionaries() const { return m_bIsThemeDictionaries; }

    void MarkIsHighContrast()
    {
        m_isHighContrast = true;
    }

    bool IsHighContrast() const
    {
        return m_isHighContrast;
    }

    void InvalidateNotFoundCache(bool propagate);
    void InvalidateNotFoundCache(bool propagate, const ResourceKey& key);

    _Check_return_
    HRESULT DeferKeysAsXaml(
        _In_ const bool fIsDictionaryWithKeyProperty,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _In_ const std::shared_ptr<XamlSavedContext>& spContext);

    _Check_return_
    HRESULT Ensure(_In_ ObjectWriter *pOriginalWriter);

    _Check_return_
    HRESULT EnsureAll();

    Theming::Theme GetActiveTheme() const { return m_activeTheme; }

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    CResourceDictionaryCollection* GetMergedDictionaries() { return m_pMergedDictionaries; }
    CResourceDictionary* GetThemeDictionaries() const { return m_pThemeDictionaries; }

    CustomWriterRuntimeContext* GetDeferredResourcesRuntimeContext() const;
    static _Check_return_
    HRESULT ShouldDeferNodeStream(
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _Out_ bool *pfShouldDefer,
        _Out_ bool *pfHasNonDeferrableContent);

#pragma region ICustomWriterRuntimeDataReceiver

    _Check_return_
    HRESULT SetCustomWriterRuntimeData(
        _In_ std::shared_ptr<CustomWriterRuntimeData> data,
        _In_ std::unique_ptr<CustomWriterRuntimeContext> context) override;

#pragma endregion

private:
    // Used for legacy (i.e. at runtime) deferred resources
    std::unique_ptr<CDeferredKeys> m_pRuntimeDeferredKeys;

    // Used for XBFv2 deferred resources
    std::unique_ptr<CResourceDictionary2> m_pDeferredResources;

    // In the case of undeferring the entire key collection we need to be robust against
    // the scneario where a key references a key that actually occurs futher down in the
    // resource dictionary collection. In this case we'll have reentrancy into CResourceDictionary
    // in the GetKeyNoRef path and we'll need to make sure that we both 1) undefer the key
    // ahead of time and 2) we don't try to reundefer the key from the bulk undeferral
    // operation happening higher up in the stack.
    //
    // At the same time we want to hard fault on other operations like reentrancy for key removal,
    // which ideally should never occur.
    //
    // Behavior doesn't actually differ here, but we do ASSERT this condition for the Remove code paths
#if DBG
    bool m_processingBulkUndeferral;
#endif

    // We keep track of how many keys we've undeferred and added to our
    // runtime key collection to ensure the count stays accurate in the case
    // where we've only partially undeferred a dictionary.
    unsigned int m_undeferredKeyCount;

protected:
    _Check_return_ HRESULT ChildEnter(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        EnterParams params,
        bool fCanProcessEnterLeave
        ) override;

    _Check_return_ HRESULT ChildLeave(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        LeaveParams params,
        bool fCanProcessEnterLeave
        ) override;

    _Check_return_ HRESULT NotifyThemeChangedCore(
        Theming::Theme theme,
        bool forceRefresh) override;

private:
    _Check_return_ HRESULT AddKey(
        const ResourceKey& key,
        _In_ CDependencyObject *pValue,
        bool keyIsOverride,
        bool undeferringKey);

    _Check_return_ HRESULT RemoveKey(
        const ResourceKeyStorage& key,
        unsigned int index);

    CDependencyObject* FindResourceByKey(_In_ const ResourceKey& key) const;

    // Warning: This method breaks standard return object ownership
    // semantics. The returned out pointer does NOT transfer ownership
    // to the caller, instead lifetime is preserved via the key dictionary.
    _Check_return_ HRESULT TryLoadDeferredResource(
        const ResourceKey& key,
        _Out_ bool& undeferring,
        _Out_ CDependencyObject** ppResource);

    _Check_return_ HRESULT LoadAllDeferredResources();

    _Check_return_ HRESULT LoadFromSource();

    _Check_return_ HRESULT EnsureActiveThemeDictionary();

    _Check_return_ HRESULT FindDeferredResource(
        const ResourceKey& key,
        _Out_ bool& undeferring,
        _Outptr_ CDependencyObject** result);

    XUINT32 m_nImplicitStylesCount;

    struct KeyInfo
    {
        KeyInfo(bool isType, bool isOverride)
            : m_isType(isType)
            , m_isOverride(isOverride)
        {}

        bool m_isType       : 1;
        bool m_isOverride   : 1;
    };

    // Store of each resource's key by index.
    std::vector<std::pair<xstring_ptr, KeyInfo>> m_keyByIndex;

    // Map of keys to resources.
    ResourceMapType m_resourceMap;

    // Temporary storage for keys that are being undeferred. This allows us to
    // avoid an infinite loop when TryLoadDeferredResource is reentered.
    std::vector<ResourceKeyStorage> m_undeferringResources;

    std::unique_ptr<Resources::details::ResourceKeyCache> m_keysNotFoundCache;

    unsigned int m_bHasKey                     : 1;
    unsigned int m_bAllowItems                 : 1;
    unsigned int m_bIsThemeDictionaries        : 1; // Represents ResourceDictionary.ThemeDictionaries
    unsigned int m_isSystemColorsDictionary    : 1;
    unsigned int m_isThemeDictionary           : 1; // Represents a ResourceDictionary inside ResourceDictioary.ThemeDictionaries
    unsigned int m_isHighContrast              : 1;
    unsigned int m_useAppResourcesForThemeRef  : 1;
    unsigned int m_isGlobal                    : 1;

    // m_pActiveThemeDictionary's theme
    Theming::Theme m_activeTheme               : 5;

    // weak reference to resource owner.
    CDependencyObject* m_pResourceOwner;

    // Active theme dictionary
    // No ref stored.
    CResourceDictionary* m_pActiveThemeDictionary;

    enum class StyleUpdateType
    {
        Added,
        Removed,
        ForcedByDiagnotics
    };

    _Check_return_ HRESULT NotifyImplicitStyleChanged(_In_ CStyle *style, StyleUpdateType styleType);

    friend class CResourceDictionaryCollection;
    friend class CDeferredKeys;
    _Check_return_ HRESULT CycleCheckInternal(_In_ CResourceDictionary * pOriginalItem);

    _Check_return_ HRESULT InvalidateImplicitStyles(_In_opt_ CResourceDictionary *pOldResources);

private:
    _Check_return_ HRESULT OnThemeDictionariesChanged();

    _Check_return_ HRESULT NotifyThemeDictionariesChanged();

public:
    xstring_ptr m_strSource;
    CResourceDictionaryCollection * m_pMergedDictionaries;
    CResourceDictionary * m_pThemeDictionaries;
};

// Class used to represent StaticResource extension.
class CStaticResourceExtension
    final : public CMarkupExtensionBase
{
private:
    CStaticResourceExtension(_In_ CCoreServices *pCore)
        : CMarkupExtensionBase(pCore)
    {
    }

public:
    // Creation method
    DECLARE_CREATE(CStaticResourceExtension);

    ~CStaticResourceExtension() override
    {
    }

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override;

    static _Check_return_ HRESULT LookupResourceNoRef(
        _In_ const xstring_ptr& strKey,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        _Outptr_result_maybenull_ CDependencyObject** ppObject,
        _In_ bool bShouldCheckThemeResources,
        _In_opt_ CStyle* optimizedStyleParent = nullptr,
        _In_ KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty
        );

    void Reset() override;

public:
    xstring_ptr m_strResourceKey;
};

// Represents a {CustomResource resourceId} markup extension. Calls back into
// CCoreServices::GetCustomResourceLoader to retrieve the actual values.
class CustomResourceExtension final : public CMarkupExtensionBase
{
public:
    // Creation method
    DECLARE_CREATE(CustomResourceExtension);
    ~CustomResourceExtension() override;

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext> &spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject> &qoValue
                ) override;

    _Check_return_ HRESULT ProvideValueByPropertyIndex(
                const KnownPropertyIndex propIndex,
                _Out_ CValue& value
                ) const;

public:
    xstring_ptr m_strResourceKey;

private:
    CustomResourceExtension(_In_ CCoreServices *pCore);
};

class CColorPaletteResources final
    : public CResourceDictionary
{
    _Check_return_ HRESULT GetColor(
        KnownPropertyIndex propertyIndex,
        _Out_ CValue& result);

    _Check_return_ HRESULT SetColor(
        KnownPropertyIndex propertyIndex,
        const CValue& value);

protected:
    CColorPaletteResources(_In_ CCoreServices* core)
        : CResourceDictionary(core)
    {}

public:
    DECLARE_CREATE(CColorPaletteResources);
    KnownTypeIndex GetTypeIndex() const override;

    template <KnownPropertyIndex propertyIndex>
    static _Check_return_ HRESULT Color(
        _In_ CDependencyObject* obj,
        _In_ XUINT32 argCount,
        _Inout_updates_(argCount) CValue* args,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* result)
    {
        CColorPaletteResources* _this = nullptr;
        IFC_RETURN(DoPointerCast(_this, obj));

        if (argCount == 0)
        {
            IFC_RETURN(_this->GetColor(
                propertyIndex,
                *result));
        }
        else
        {
            IFC_RETURN(_this->SetColor(
                propertyIndex,
                args[0]));
        }

        return S_OK;
    }
};