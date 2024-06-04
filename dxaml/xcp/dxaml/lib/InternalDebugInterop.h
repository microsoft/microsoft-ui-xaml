// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InternalDebugInteropModel.h"

// This class implements the IInternalDebugInterop interface, acting as a helper
// and intermediary for internal debugging tools like XAML Snoop.
class __declspec(uuid("804d7c5f-5373-492c-ba40-88e40d9965d6")) InternalDebugInterop :
    public wrl::RuntimeClass<
    wrl::RuntimeClassFlags<wrl::WinRt>,
    DebugTool::IInternalDebugInterop,
    wrl::FtmBase>
{

    InspectableClass(L"InternalDebugInterop", BaseTrust);

public:

    #pragma region IInternalDebugInterop

    #pragma region Type Methods

    HRESULT GetBaseType(
        _In_ const CClassInfo* pType,
        _Outptr_ const CClassInfo** ppBaseType) override;

    HRESULT GetBaseUri(
        _In_ xaml::IDependencyObject* pReference,
        _Out_ HSTRING* phName) override;

    HRESULT GetName(
        _In_ xaml::IFrameworkElement* pReference,
        _Out_ HSTRING *phName) override;

    HRESULT GetTypeDisplayNameFromObject(
        _In_ IInspectable* pObject,
        _Out_ HSTRING* phDisplayName) override;

    HRESULT GetTypeFor(
        _In_ xaml::IDependencyObject* pReference,
        _Outptr_ const CClassInfo** ppType) override;

    PCWSTR GetTypeName(
        _In_ const CClassInfo* pType) override;

    bool IsTypePublic(
        _In_ const CClassInfo* pType) override;

    #pragma endregion

    #pragma region Property Methods

    HRESULT ClearPropertyValue(
        _In_ xaml::IDependencyObject* pReference,
        _In_ KnownPropertyIndex nPropertyIndex) override;

    HRESULT GetAllDependencyProperties(
        _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection) override;

    HRESULT GetCollectionSize(
        _In_ const DebugTool::DebugPropertyInfo& info,
        _Out_ UINT* pSize) override;

    HRESULT GetCollectionItem(
        _In_ const DebugTool::DebugPropertyInfo& info,
        _In_ UINT index,
        _Outptr_ xaml::IDependencyObject** ppDO) override;

    HRESULT GetContentProperty(
        _In_ xaml::IDependencyObject* pDO,
        _Out_ DebugTool::DebugPropertyInfo** ppPropertyInfo) override;

    HRESULT GetContentPropertyName(
        _In_ xaml::IDependencyObject* pDO,
        _Out_ LPCWSTR* pName) override;

    HRESULT GetDefaultValue(
        _In_ xaml::IDependencyObject* pReference,
        _In_ xaml::IDependencyProperty* pIDP,
        _Outptr_ IInspectable** ppValue) override;

    DirectUI::PropertyPathStep* GetNextStep(
        _In_ DirectUI::PropertyPathStep* pStep) override;

    PCWSTR GetPropertyName(
        _In_ DirectUI::PropertyPathStep* pStep) override;

    HRESULT GetPropertyValue(
        _In_ DirectUI::PropertyPathStep* pStep,
        _Out_ IInspectable** ppValue) override;

    HRESULT GetPropertiesForDO(
        _In_ xaml::IDependencyObject* pReference,
        _In_ bool bGetDefaultValues,
        _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection) override;

    HRESULT GetValue(
        _In_ xaml::IDependencyObject* pReference,
        _In_ xaml::IDependencyProperty* pIDP,
        _Outptr_ IInspectable** ppValue) override;

    bool IsCollection(
        _In_ const DebugTool::DebugPropertyInfo& info) override;

    HRESULT SetPropertyValue(
        _In_ xaml::IDependencyObject* pReference,
        _In_ KnownPropertyIndex nPropertyIndex,
        _In_opt_ PCWSTR pszValue) override;

    HRESULT SetPropertyValue(
        _In_ xaml::IDependencyObject* pReference,
        _In_ KnownPropertyIndex nPropertyIndex,
        _In_opt_ IInspectable* pValue) noexcept override;

    #pragma endregion

    #pragma region Tree Methods

    HRESULT GetChildren(
        _In_ xaml::IDependencyObject* pReference,
        _Outptr_ xaml::IDependencyObject** ppDO) override;

    HRESULT GetParent(
        _In_ xaml::IDependencyObject* pReference,
        _Outptr_ xaml::IDependencyObject** ppDO) override;

    bool HasTemplatedParent(
        _In_ xaml::IDependencyObject* pReference) override;

    HRESULT HitTest(
        _In_ RECT rect,
        _Inout_ DebugTool::ICollection<xaml::IDependencyObject*>* pElements) override;

    #pragma endregion

    #pragma region Framework Methods

    HRESULT GetPopupRoot(
        _Outptr_ xaml::IDependencyObject** ppPopupRoot) override;

    HRESULT GetVisualRoot(
        _Outptr_result_maybenull_ xaml::IDependencyObject** ppRoot) override;

#pragma endregion

    #pragma region Layout Methods

    HRESULT AdjustBoundingRectToRoot(
        _In_ xaml::IUIElement* pReference,
        _Inout_ XRECTF* pRect) override;

    HRESULT GetGlobalBounds(
        _In_ xaml::IUIElement* pReference,
        _In_ bool ignoreClipping,
        _Out_ XRECTF* pBounds) override;

    HRESULT GetLayoutInfo(
        _In_ xaml::IFrameworkElement* pReference,
        _Out_ XTHICKNESS* pMargin,
        _Out_ XTHICKNESS* pPadding) override;

    bool GetRenderTransformLocal(
        _In_ xaml::IUIElement* pReference,
        _Inout_ CMILMatrix* pMatrix) override;

    void GetScaleDimensions(
        _In_ CMILMatrix* pMatrix,
        _Out_ XFLOAT* prScaleX,
        _Out_ XFLOAT* prScaleY) override;

    bool IsVisible(
        _In_ xaml::IUIElement* pReference) override;

    HRESULT VisualRootUpdateLayout() override;

    #pragma endregion

    #pragma region Feature Methods

    HRESULT GetAnimationSpeedFactor(
        _In_ INT64* piAnimationSpeed) override;

    HRESULT GetShowingFrameCounter(
        _Out_ bool* pbFrameCounterEnabled) override;

    HRESULT SetAnimationSpeedFactor(
        _In_ INT64 iAnimationSpeed) override;

    HRESULT SetShowingFrameCounter(
        _In_ bool bFrameCounterEnabled) override;

    #pragma endregion

    #pragma region Other Methods

    HRESULT ConvertValueToString(
        _In_ wf::IPropertyValue* pPropertyValue,
        _In_ wf::PropertyType propertyType,
        _Out_ HSTRING* phstr) override;

    DebugTool::IDebugTool* GetDebugToolNoRef() override;

    #pragma endregion

    #pragma endregion

    #pragma region Getters and Setters

    void SetDebugTool(_In_ DebugTool::IDebugTool* pDebugTool);

    #pragma endregion

    #pragma region Constructors

    InternalDebugInterop();

    HRESULT RuntimeClassInitialize();

    HRESULT RuntimeClassInitialize(
        _In_ HWND hwndParent);

    #pragma endregion

    #pragma region Statics


    static HRESULT GetDefaultValueInternal(
        _In_ xaml::IDependencyObject* pReference,
        _In_ xaml::IDependencyProperty* pIDP,
        _Outptr_ IInspectable** ppValue);

    static HRESULT GetValueInternal(
        _In_ xaml::IDependencyObject* pReference,
        _In_ xaml::IDependencyProperty* pIDP,
        _Outptr_ IInspectable** ppValue);

    static HRESULT PopulateBaseValueDebugPropertyInfo(
        _In_ CCoreServices* pCore,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _In_opt_ DirectUI::DependencyObject* pObject,
        _In_opt_ const CDependencyProperty* pDP,
        _In_ KnownPropertyIndex propertyIndex,
        _In_ bool bGetDefaultValue,
        _Inout_ bool& didPopulate,
        _Out_ DebugTool::DebugPropertyInfo* pDebugPropertyInfo);

    static HRESULT PopulateDebugPropertyInfo(
        _In_opt_ const CDependencyProperty* pDP,
        _In_opt_ xaml::IDependencyObject* pSetterDO,
        _Inout_opt_ IInspectable* pValue,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _In_ BaseValueSource baseValueSource,
        _In_opt_ DebugTool::IDebugBindingExpression* pDebugBindingExpression,
        _In_ bool isExpression,
        _In_ bool isNonDefaultValue,
        _Out_ DebugTool::DebugPropertyInfo* pDebugPropertyInfo);

    static HRESULT PopulateAllDependencyProperties(
        _In_ CCoreServices* pCore,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pCollection);

    static HRESULT PopulateDebugPropertyInfoCollection(
        _In_ DirectUI::DependencyObject* pDO,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _In_ std::vector<DebugTool::DebugPropertyInfo>& collection,
        _In_ bool pGetDefaultValues);

    static HRESULT TryGetBindingExpression(
        _In_ DirectUI::DependencyObject* pDO,
        _In_ KnownPropertyIndex propertyIndex,
        _COM_Outptr_result_maybenull_ DebugTool::IDebugBindingExpression** pDebugBindingExpression);

    #pragma endregion

private:

    #pragma region Private Member Methods

    CCoreServices* GetCore();

    HRESULT CreateInstance(
        _In_ const CClassInfo* pType,
        _In_ LPCWSTR value,
        _Outptr_ IInspectable** ppInstance);

    bool IsInvalidProperty(
        _In_ const CDependencyProperty* const dependencyProperty);

    #pragma endregion

private:

    DebugTool::IDebugTool *m_pDebugTool;
    wrl::ComPtr<DirectUI::IValueConverterInternal> m_spValueConverter;

};

ActivatableClass(InternalDebugInterop);
