// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <list>
#include <vector>
#include <map>
#include <utility>
#include <FrameworkUdk/DebugTool.h>
#include "DiagnosticsInteropModel.h"

#include "XamlOM.WinUI.h"
class CClassInfo;
class CJupiterControl;
class CCoreServices;

namespace DebugTool
{
    // This class stores information about properties set on a DependencyObject.
    class DebugPropertyInfo
    {
    public:
        DebugPropertyInfo() = default;
        DebugPropertyInfo(const DebugPropertyInfo& other)
        {
            *this = other;
        }
        DebugPropertyInfo(DebugPropertyInfo&& other)
        {
            *this = std::move(other);
        }

        DebugPropertyInfo& operator=(const DebugPropertyInfo& other)
        {
            if (this != &other)
            {
                m_propertyIndex = other.m_propertyIndex;
                m_typeIndex = other.m_typeIndex;
                m_spDP = other.m_spDP;
                m_spSetterDO = other.m_spSetterDO;
                m_strName = other.m_strName;
                m_strValue = other.m_strValue;
                m_pType = other.m_pType;
                m_pOwnerType = other.m_pOwnerType;
                m_pValueType = other.m_pValueType;
                m_baseValueSource = other.m_baseValueSource;
                m_nonDefaultValue = other.m_nonDefaultValue;
                m_readOnly = other.m_readOnly;
                m_collection = other.m_collection;
                m_valueCollection = other.m_valueCollection;
                m_spDebugBindingExpression = other.m_spDebugBindingExpression;
                m_isExpression = other.m_isExpression;
                m_spObjectValue = other.m_spObjectValue;
            }

            return *this;
        }
        DebugPropertyInfo& operator=(DebugPropertyInfo&& other)
        {
            if (this != &other)
            {
                m_propertyIndex = std::move(other.m_propertyIndex);
                m_typeIndex = std::move(other.m_typeIndex);
                m_spDP = std::move(other.m_spDP);
                m_spSetterDO = std::move(other.m_spSetterDO);
                m_strName = std::move(other.m_strName);
                m_strValue = std::move(other.m_strValue);
                m_pType = std::move(other.m_pType);
                m_pOwnerType = std::move(other.m_pOwnerType);
                m_pValueType = std::move(other.m_pValueType);
                m_baseValueSource = std::move(other.m_baseValueSource);
                m_nonDefaultValue = std::move(other.m_nonDefaultValue);
                m_readOnly = std::move(other.m_readOnly);
                m_collection = std::move(other.m_collection);
                m_valueCollection = std::move(other.m_valueCollection);
                m_spDebugBindingExpression = std::move(other.m_spDebugBindingExpression);
                m_isExpression = std::move(other.m_isExpression);
                m_spObjectValue = std::move(other.m_spObjectValue);
            }

            return *this;
        }

        HRESULT Initialize(
            _In_ KnownPropertyIndex index,
            _In_ KnownTypeIndex typeIndex,
            _In_ xaml::IDependencyProperty* pDP,
            _In_ xaml::IDependencyObject* pSetterDO,
            _In_ PCWSTR szName,
            _In_ PCWSTR szValue,
            _In_ const CClassInfo* pType,
            _In_ const CClassInfo* pOwnerType,
            _In_ const CClassInfo* pValueType,
            _In_ BaseValueSource source,
            _In_ bool nonDefaultValue,
            _In_ bool readOnly,
            _In_ bool collection,
            _In_ bool valueCollection,
            _In_opt_ IDebugBindingExpression* pDebugBindingExpression,
            _In_ bool isExpression,
            _In_opt_ IInspectable* pValue)
        {
            m_strName = szName;
            m_strValue = szValue;

            m_propertyIndex = index;
            m_typeIndex = typeIndex;
            m_spDP = pDP;
            m_spSetterDO = pSetterDO;
            m_pType = pType;
            m_pOwnerType = pOwnerType;
            m_pValueType = pValueType;
            m_baseValueSource = source;
            m_nonDefaultValue = nonDefaultValue;
            m_readOnly = readOnly;
            m_collection = collection;
            m_valueCollection = valueCollection;
            m_spDebugBindingExpression = pDebugBindingExpression;
            m_isExpression = isExpression;
            m_spObjectValue = pValue;

            return S_OK;
        }

        inline KnownPropertyIndex GetPropertyIndex() const { return m_propertyIndex; }
        inline KnownTypeIndex GetTypeIndex() const { return m_typeIndex; }
        inline xaml::IDependencyProperty* GetDependencyProperty() const { return m_spDP.Get(); }
        inline xaml::IDependencyObject* GetSetter() const { return m_spSetterDO.Get(); }
        inline PCWSTR GetName() const { return m_strName.c_str(); }
        inline PCWSTR GetValue() const { return m_strValue.c_str(); }
        inline const CClassInfo* GetType() const { return m_pType; }
        inline const CClassInfo* GetOwnerType() const { return m_pOwnerType; }
        inline const CClassInfo* GetValueType() const { return m_pValueType; }
        inline BaseValueSource GetSource() const { return m_baseValueSource; }
        inline bool IsNonDefaultValue() const { return m_nonDefaultValue; }
        inline bool IsReadOnly() const { return m_readOnly; }
        inline bool IsCollection() const { return m_collection; }
        inline bool IsValueCollection() const { return m_valueCollection; }
        inline IDebugBindingExpression* GetDebugBindingExpression() const { return m_spDebugBindingExpression.Get(); }
        inline bool IsExpression() const { return m_isExpression; }
        inline IInspectable* GetObjectValue() const { return m_spObjectValue.Get(); }

    private:
        KnownPropertyIndex m_propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        KnownTypeIndex m_typeIndex = KnownTypeIndex::UnknownType;
        wrl::ComPtr<xaml::IDependencyProperty> m_spDP;
        wrl::ComPtr<xaml::IDependencyObject> m_spSetterDO;
        std::wstring m_strName;
        std::wstring m_strValue;
        const CClassInfo* m_pType = nullptr;
        const CClassInfo* m_pOwnerType = nullptr;
        const CClassInfo* m_pValueType = nullptr;
        BaseValueSource m_baseValueSource = BaseValueSourceUnknown;
        bool m_nonDefaultValue = false;
        bool m_readOnly = false;
        bool m_collection = false;
        bool m_valueCollection = false;
        wrl::ComPtr<IDebugBindingExpression> m_spDebugBindingExpression;
        bool m_isExpression = false;
        wrl::ComPtr<IInspectable> m_spObjectValue;
    };

    class DebugEnumInfo
    {
    public:
        DebugEnumInfo() = default;
        DebugEnumInfo(const DebugEnumInfo& other)
        {
            *this = other;
        }
        DebugEnumInfo(DebugEnumInfo&& other)
        {
            *this = std::move(other);
        }

        DebugEnumInfo& operator=(const DebugEnumInfo& other)
        {
            if(this != &other)
            {
                m_index = other.m_index;
                m_strName = other.m_strName;
                m_values = other.m_values;
            }

            return *this;
        }
        DebugEnumInfo& operator=(DebugEnumInfo&& other)
        {
            if (this != &other)
            {
                m_index = std::move(other.m_index);
                m_strName = std::move(other.m_strName);
                m_values = std::move(other.m_values);
            }

            return *this;
        }

        HRESULT Initialize(
            _In_ KnownTypeIndex index,
            _In_ PCWSTR szName,
            _In_ const std::vector<std::pair<int, PCWSTR>>& values)
        {
            m_strName = szName;
            m_index = index;
            m_values = values;

            return S_OK;
        }

        inline KnownTypeIndex GetTypeIndex() const { return m_index; }
        inline PCWSTR GetName() const { return m_strName.c_str(); }
        inline const std::vector<std::pair<int, PCWSTR>>& GetValues() const { return m_values; }

    private:
        KnownTypeIndex m_index = KnownTypeIndex::UnknownType;
        std::wstring m_strName;
        std::vector<std::pair<int, PCWSTR>> m_values;
    };

    // This interface must be implemented by the debug tool (e.g., XamlSnoop.dll).
    interface IDebugTool
    {
        virtual void Launch(HWND hwndParent, _In_ const EnvironmentMap& env) = 0;
        virtual void TranslateAccelerator(_In_ MSG* pMsg, _Out_ bool* pbHandled) = 0;
        virtual HRESULT ValueToString(_In_opt_ IInspectable* /*pValue*/, _Out_ HSTRING* /*phstr*/) { return E_NOTIMPL; };
        virtual HRESULT ValueToString(_In_opt_ IInspectable* /*pValue*/, _In_opt_ IInspectable* /*pOwner*/, _Out_ HSTRING* /*phstr*/) { return E_NOTIMPL; };
        virtual HRESULT SignalMutation(_In_ xaml::IDependencyObject* /*pReference*/, _In_ bool /*isAdd*/) { return E_NOTIMPL; };
        virtual HRESULT OnDeinitializeCore() { return S_OK; };
        virtual HRESULT SetCoreDispatcher(_In_ wuc::ICoreDispatcher* /*dispatcher*/) { return S_OK; }
    };

    // This interface provides all of the private helper functions needed by the debug tool.
    interface __declspec(uuid("3c1e2929-9e66-4532-acd9-54bde204c635")) IInternalDebugInterop : IInspectable
    {
        #pragma region Type Methods

        virtual HRESULT GetBaseType(
            _In_ const CClassInfo* pType,
            _Outptr_ const CClassInfo** ppBaseType) = 0;

        virtual HRESULT GetBaseUri(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ HSTRING* phName) = 0;

        virtual HRESULT GetName(
            _In_ xaml::IFrameworkElement* pReference,
            _Out_ HSTRING* phName) = 0;

        virtual HRESULT GetTypeDisplayNameFromObject(
            _In_ IInspectable* pObject,
            _Out_ HSTRING* phDisplayName) = 0;

        virtual HRESULT GetTypeFor(
            _In_ xaml::IDependencyObject* pReference,
            _Outptr_ const CClassInfo** ppType) = 0;

        virtual PCWSTR GetTypeName(
            _In_ const CClassInfo* pType) = 0;

        virtual bool IsTypePublic(
            _In_ const CClassInfo* pType) = 0;

        #pragma endregion

        #pragma region Property Methods

        virtual HRESULT ClearPropertyValue(
            _In_ xaml::IDependencyObject* pReference,
            _In_ KnownPropertyIndex nPropertyIndex) = 0;

        virtual HRESULT GetAllDependencyProperties(
            _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection) = 0;

        virtual HRESULT GetCollectionSize(
            _In_ const DebugPropertyInfo& info,
            _Out_ UINT* pSize) = 0;

        virtual HRESULT GetCollectionItem(
            _In_ const DebugPropertyInfo& info,
            _In_ UINT index,
            _Outptr_ xaml::IDependencyObject** ppDO) = 0;

        virtual HRESULT GetContentProperty(
            _In_ xaml::IDependencyObject* pDO,
            _Out_ DebugTool::DebugPropertyInfo** ppPropertyInfo) = 0;

        virtual HRESULT GetContentPropertyName(
            _In_ xaml::IDependencyObject* pDO,
            _Out_ LPCWSTR* pName) = 0;

        virtual HRESULT GetDefaultValue(
            _In_ xaml::IDependencyObject* pReference,
            _In_ xaml::IDependencyProperty* pIDP,
            _Outptr_ IInspectable** ppValue) = 0;

        virtual DirectUI::PropertyPathStep* GetNextStep(
            _In_ DirectUI::PropertyPathStep* pStep) = 0;

        virtual PCWSTR GetPropertyName(
            _In_ DirectUI::PropertyPathStep* pStep) = 0;

        virtual HRESULT GetPropertyValue(
            _In_ DirectUI::PropertyPathStep* pStep, _Out_ IInspectable** ppValue) = 0;

        virtual HRESULT GetPropertiesForDO(
            _In_ xaml::IDependencyObject* pReference,
            _In_ bool bGetDefaultValues,
            _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection) = 0;

        virtual HRESULT GetValue(
            _In_ xaml::IDependencyObject* pReference,
            _In_ xaml::IDependencyProperty* pIDP,
            _Outptr_ IInspectable** ppValue) = 0;

        virtual bool IsCollection(
            _In_ const DebugPropertyInfo& info) = 0;

        virtual HRESULT SetPropertyValue(
            _In_ xaml::IDependencyObject* pReference,
            _In_ KnownPropertyIndex nPropertyIndex,
            _In_opt_ PCWSTR pszValue) = 0;

        virtual HRESULT SetPropertyValue(
            _In_ xaml::IDependencyObject* pReference,
            _In_ KnownPropertyIndex nPropertyIndex,
            _In_opt_ IInspectable* pValue) = 0;

        #pragma endregion

        #pragma region Tree Methods

        virtual HRESULT GetChildren(
            _In_ xaml::IDependencyObject* pReference,
            _Outptr_ xaml::IDependencyObject** ppDO) = 0;

        virtual HRESULT GetParent(
            _In_ xaml::IDependencyObject* pReference,
            _Outptr_ xaml::IDependencyObject** ppDO) = 0;

        virtual bool HasTemplatedParent(
            _In_ xaml::IDependencyObject* pReference) = 0;

        virtual HRESULT HitTest(
            _In_ RECT rect,
            _Inout_ ICollection<xaml::IDependencyObject*>* pElements) = 0;

        #pragma endregion

        #pragma region Framework Methods

        virtual HRESULT GetPopupRoot(
            _Outptr_ xaml::IDependencyObject** ppPopupRoot) = 0;

        virtual HRESULT GetVisualRoot(
            _Outptr_result_maybenull_ xaml::IDependencyObject** ppRoot) = 0;


        #pragma endregion

        #pragma region Layout Methods

        virtual HRESULT AdjustBoundingRectToRoot(
            _In_ xaml::IUIElement* pReference,
            _Inout_ XRECTF* pRect) = 0;

        virtual HRESULT GetGlobalBounds(
            _In_ xaml::IUIElement* pReference,
            _In_ bool ignoreClipping,
            _Out_ XRECTF* pBounds) = 0;

        virtual HRESULT GetLayoutInfo(
            _In_ xaml::IFrameworkElement* pReference,
            _Out_ XTHICKNESS* pMargin,
            _Out_ XTHICKNESS* pPadding) = 0;

        virtual bool GetRenderTransformLocal(
            _In_ xaml::IUIElement* pReference,
            _Inout_ CMILMatrix* pMatrix) = 0;

        virtual void GetScaleDimensions(
            _In_ CMILMatrix* pMatrix,
            _Out_ XFLOAT* prScaleX,
            _Out_ XFLOAT* prScaleY) = 0;

        virtual bool IsVisible(
            _In_ xaml::IUIElement* pReference) = 0;

        virtual HRESULT VisualRootUpdateLayout() = 0;

        #pragma endregion

        #pragma region Feature Methods

        virtual HRESULT GetAnimationSpeedFactor(
            _In_ INT64* piAnimationSpeed) = 0;

        virtual HRESULT GetShowingFrameCounter(
            _Out_ bool* pbFrameCounterEnabled) = 0;

        virtual HRESULT SetAnimationSpeedFactor(
            _In_ INT64 iAnimationSpeed) = 0;

        virtual HRESULT SetShowingFrameCounter(
            _In_ bool bFrameCounterEnabled) = 0;

        #pragma endregion

        #pragma region Other Methods

        virtual _Check_return_ HRESULT ConvertValueToString(
            _In_ wf::IPropertyValue* pPropertyValue,
            _In_ wf::PropertyType propertyType,
            _Out_ HSTRING* phstr) = 0;

        virtual DebugTool::IDebugTool* GetDebugToolNoRef() = 0;

        #pragma endregion
    };

    #pragma region Public Methods

    HRESULT GetPropertiesForDO(
        _In_ xaml::IDependencyObject* pReference,
        _In_ bool bGetDefaultValues,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _Inout_ std::vector<DebugTool::DebugPropertyInfo>& propInfoList);

    #pragma region Diagnostics Creation Methods

    typedef HRESULT(*PFNCreate)(
        _In_ IInternalDebugInterop* pInternalDebugInterop,
        _Outptr_ DebugTool::IDebugTool** ppInstance);

    HRESULT CreateDebugTool(
        _In_ CJupiterControl* pControl);
    HRESULT CreateDiagnostics(
        _In_ CJupiterControl* pControl,
        _In_ const Msg_ConnectToVisualTree * pMsg);
    HRESULT Create(_In_ CJupiterControl* pControl,
        _In_ const std::vector<LPCWSTR>& variables,
        _In_ const EnvironmentMap& env = EnvironmentMap());

    #pragma endregion

    #pragma endregion

} // namespace DebugTool
