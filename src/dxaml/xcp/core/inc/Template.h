// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NoParentShareableDependencyObject.h"

class CType;
struct XamlQualifiedObject;

class CFrameworkTemplate : public CNoParentShareableDependencyObject
{
public:
    DECLARE_CREATE(CFrameworkTemplate);

    CFrameworkTemplate() = delete;
    explicit CFrameworkTemplate(CCoreServices* pCore)
        : CNoParentShareableDependencyObject(pCore)
        , m_fHasEventRoot(false)
        , m_targetTypeIndex(KnownTypeIndex::UnknownType)
    {
    }

    ~CFrameworkTemplate() override;

    // We will keep a reference to this type on the managed
    // side when this object is assigned to a property to ensure
    // that the managed peer stays alive.
    // We only need to do this if there's an event root assigned to this 
    // template, or we're cacheing resource references,
    // otherwise the managed peer can be recreated as needed. 
    // This works because the event root is the only piece of state that is kept
    // on the managed side. 
    // the base implementation takes into account any subclasses of templates
    XUINT32 ParticipatesInManagedTreeInternal() override
    { 
        if( HasManagedPeer() && m_fHasEventRoot )
            return PARTICIPATES_IN_MANAGED_TREE;
        else
            return CNoParentShareableDependencyObject::ParticipatesInManagedTreeInternal();
    }

    void SetHasEventRoot(bool fNewValue) { m_fHasEventRoot = fNewValue; }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFrameworkTemplate>::Index;
    }

protected:
    _Check_return_ virtual HRESULT LoadContent(
        _Outptr_result_maybenull_ CDependencyObject** pResult,
        _In_opt_ CDependencyObject *pTemplatedParent,
        _In_ XUINT32 bRegisterNamesInTemplateNamescope,
        bool skipRegistrationEntirely,
        const std::shared_ptr<XamlQualifiedObject>& bindingConnector = std::shared_ptr<XamlQualifiedObject>());

    bool HasTemplateContent() { return (m_pTemplateContent != nullptr); }

public:
    _Check_return_ virtual HRESULT LoadContent(_Outptr_result_maybenull_ CDependencyObject** pResult, _In_opt_ CDependencyObject *pTemplatedParent = nullptr)
    {
        return LoadContent(pResult, pTemplatedParent, FALSE /* bRegisterNamesInTemplateNamescope */, false);
    }

   _Check_return_ HRESULT AddGroups(_In_ const xstring_ptr& strStateGroupName, _In_ const xstring_ptr& strVisualStateString);

   void SetParentXBindConnector(const std::shared_ptr<XamlQualifiedObject>& parentXBindConnector);

private:
    bool        m_fHasEventRoot : 1;        // Whether the event root was set or not
protected:
    KnownTypeIndex m_targetTypeIndex : 16;
    wrl::ComPtr<IWeakReference> m_spParentXBindConnector;

public:
    CTemplateContent * m_pTemplateContent = nullptr;
};

class CControlTemplate : public CFrameworkTemplate
{
protected:
    CControlTemplate(CCoreServices* pCore) : CFrameworkTemplate(pCore)
    {
    }

public:
    DECLARE_CREATE(CControlTemplate);

    _Check_return_ HRESULT LoadContent(_Outptr_result_maybenull_ CDependencyObject** pResult, _In_opt_ CDependencyObject *pTemplatedParent = nullptr) override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CControlTemplate>::Index;
    }

    const CClassInfo* GetTargetType() const;

    bool HasTargetType() const
    {
        return (m_targetTypeIndex != KnownTypeIndex::UnknownType);
    }

    _Check_return_ HRESULT static TargetType(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    void SetTargetType(KnownTypeIndex targetType)
    {
        m_targetTypeIndex = targetType;
    }

    KnownTypeIndex GetTargetTypeIndex() const
    {
        return m_targetTypeIndex;
    }

    // Get the value of a dependency property from a ControlTemplate.  This also
    // returns the default value of Control if the TargetType property has not
    // been set.
    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Out_ CValue *pValue) final;

    void SetConnectionId(int32_t id)
    {
        m_connectionId = id;
    }

private:
    std::shared_ptr<XamlQualifiedObject> CreateXBindConnector(_In_ CDependencyObject* templatedParent);
    _Check_return_ HRESULT ConnectTemplate(const std::shared_ptr<XamlQualifiedObject>& xBindConnector);

    // TODO: We could perhaps store this up on a derived class since it may pack better
    int32_t m_connectionId = -1;
};

class CDataTemplate: public CControlTemplate
{
protected:
    CDataTemplate(CCoreServices* pCore) : CControlTemplate(pCore)
    {
    }
    ~CDataTemplate() override;

public:
    DECLARE_CREATE(CDataTemplate);

    _Check_return_ HRESULT LoadContent(_Outptr_result_maybenull_ CDependencyObject** result, _In_opt_ CDependencyObject *templatedParent = nullptr) override;

    // This method is used to load an instance for querying but not using the instance. So we cache the instance so that
    // we can serve it later during a LoadContent call.
    _Check_return_ HRESULT QueryContentNoRef(_Outptr_result_maybenull_ CDependencyObject** result, _In_opt_ CDependencyObject *templatedParent = nullptr);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDataTemplate>::Index;
    }

    // We will keep a reference to this type on the managed
    // side when this object is assigned to a property to ensure
    // that the managed peer stays alive.
    // For example, when DataTemplate is used in ItemsControl.ItemTemplate, it keeps
    // a peer reference on the m_cachedInstance peer.
    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

private:
    xref_ptr<CDependencyObject> m_cachedInstance;
};

//
//  CDisplayMemberTemplate
//
//  CContentPresenter needs a way remember that it will be visualizing with a TextBlock bound to an item path.
//  Since we don't have the concept of DataTemplateSelector we save this information in a separate template class.
//

class CDisplayMemberTemplate final : public CDataTemplate
{
protected:
    CDisplayMemberTemplate(CCoreServices* pCore) : CDataTemplate(pCore)
    {
    }

public:
    DECLARE_CREATE(CDisplayMemberTemplate);

    ~CDisplayMemberTemplate() override
    {
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDisplayMemberTemplate>::Index;
    }

    _Check_return_ HRESULT LoadContent(_Outptr_result_maybenull_ CDependencyObject** pResult, _In_opt_ CDependencyObject *pTemplatedParent = nullptr) override;

    xstring_ptr m_strMemberPath;
    bool m_isDefaultTemplate = false;
};

//
//  CItemsPanelTemplate
//
//  The ItemsPanelTemplate specifies the panel that is used for the layout of items in an ItemsControl.
//

class CItemsPanelTemplate final : public CFrameworkTemplate
{
protected:
    CItemsPanelTemplate(CCoreServices* pCore) : CFrameworkTemplate(pCore) {}

public:
    DECLARE_CREATE(CItemsPanelTemplate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CItemsPanelTemplate>::Index;
    }

    _Check_return_ HRESULT LoadContent(_Outptr_result_maybenull_ CDependencyObject** pResult, _In_opt_ CDependencyObject *pTemplatedParent = nullptr) override;
};

