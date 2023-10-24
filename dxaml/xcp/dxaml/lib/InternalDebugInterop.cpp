// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InternalDebugInterop.h"
#include "FrameworkApplication.g.h"
#include "Setter.g.h"
#include "Style.g.h"
#include "DynamicMetadataStorage.h"
#include <CColor.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <functional>
#include "PropertyPathStep.h"
#include "ThemeResourceExpression.h"
#include "VisualTreeHelper.h"
#include "DynamicValueConverter.h"
#include "MetadataIterator.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

#pragma region Globals

namespace
{
    template <typename Item>
    static HRESULT
        GetCollectionSizeInternal(
        _In_ IInspectable* pValue,
        _Out_ UINT* pSize)
    {
        ctl::ComPtr<wfc::IVector<Item*>> spCollection;
        IFC_RETURN(ctl::do_query_interface(spCollection, pValue));
        IFC_RETURN(spCollection->get_Size(pSize));

        return S_OK;
    }

    template <typename Item, typename IItem>
    static HRESULT
        GetCollectionItemInternal(
        _In_ IInspectable* pValue,
        _In_ UINT index,
        _Outptr_ xaml::IDependencyObject** ppDO)
    {
        ctl::ComPtr<wfc::IVector<Item*>> spCollection;
        ctl::ComPtr<IItem> spItem;
        ctl::ComPtr<xaml::IDependencyObject> spItemAsDO;

        *ppDO = nullptr;

        IFC_RETURN(ctl::do_query_interface(spCollection, pValue));
        IFC_RETURN(spCollection->GetAt(index, &spItem));
        IFC_RETURN(spItem.As(&spItemAsDO));
        spItemAsDO.CopyTo(ppDO);

        return S_OK;
    }

    typedef HRESULT (*PFNGetSize)(_In_ IInspectable*, _In_ UINT*);
    typedef HRESULT (*PFNGetItem)(_In_ IInspectable*, _In_ UINT, _Outptr_ xaml::IDependencyObject**);

    struct CollectionAccessor
    {
        KnownTypeIndex m_index;
        PFNGetSize m_fnGetSize;
        PFNGetItem m_fnGetItem;
    };

    const CollectionAccessor s_collectionAccessors[] =
    {
        { KnownTypeIndex::DependencyObjectCollection,
        &GetCollectionSizeInternal<xaml::DependencyObject>,
        &GetCollectionItemInternal < xaml::DependencyObject, xaml::IDependencyObject > },

        { KnownTypeIndex::SetterBaseCollection,
        &GetCollectionSizeInternal<xaml::SetterBase>,
        &GetCollectionItemInternal < xaml::SetterBase, xaml::ISetterBase > },

        { KnownTypeIndex::TriggerActionCollection,
        &GetCollectionSizeInternal<xaml::TriggerAction>,
        &GetCollectionItemInternal < xaml::TriggerAction, xaml::ITriggerAction > },

        { KnownTypeIndex::TriggerCollection,
        &GetCollectionSizeInternal<xaml::TriggerBase>,
        &GetCollectionItemInternal < xaml::TriggerBase, xaml::ITriggerBase > },

        { KnownTypeIndex::GeometryCollection,
        &GetCollectionSizeInternal<xaml_media::Geometry>,
        &GetCollectionItemInternal < xaml_media::Geometry, xaml_media::IGeometry > },

        { KnownTypeIndex::GradientStopCollection,
        &GetCollectionSizeInternal<xaml_media::GradientStop>,
        &GetCollectionItemInternal < xaml_media::GradientStop, xaml_media::IGradientStop > },

        { KnownTypeIndex::PathFigureCollection,
        &GetCollectionSizeInternal<xaml_media::PathFigure>,
        &GetCollectionItemInternal < xaml_media::PathFigure, xaml_media::IPathFigure > },

        { KnownTypeIndex::PathSegmentCollection,
        &GetCollectionSizeInternal<xaml_media::PathSegment>,
        &GetCollectionItemInternal < xaml_media::PathSegment, xaml_media::IPathSegment > },

        { KnownTypeIndex::TransformCollection,
        &GetCollectionSizeInternal<xaml_media::Transform>,
        &GetCollectionItemInternal < xaml_media::Transform, xaml_media::ITransform > },

        { KnownTypeIndex::ColorKeyFrameCollection,
        &GetCollectionSizeInternal<xaml_animation::ColorKeyFrame>,
        &GetCollectionItemInternal < xaml_animation::ColorKeyFrame, xaml_animation::IColorKeyFrame > },

        { KnownTypeIndex::DoubleKeyFrameCollection,
        &GetCollectionSizeInternal<xaml_animation::DoubleKeyFrame>,
        &GetCollectionItemInternal < xaml_animation::DoubleKeyFrame, xaml_animation::IDoubleKeyFrame > },

        { KnownTypeIndex::ObjectKeyFrameCollection,
        &GetCollectionSizeInternal<xaml_animation::ObjectKeyFrame>,
        &GetCollectionItemInternal < xaml_animation::ObjectKeyFrame, xaml_animation::IObjectKeyFrame > },

        { KnownTypeIndex::PointKeyFrameCollection,
        &GetCollectionSizeInternal<xaml_animation::PointKeyFrame>,
        &GetCollectionItemInternal < xaml_animation::PointKeyFrame, xaml_animation::IPointKeyFrame > },


        { KnownTypeIndex::TransitionCollection,
        &GetCollectionSizeInternal<xaml_animation::Transition>,
        &GetCollectionItemInternal < xaml_animation::Transition, xaml_animation::ITransition > },

        { KnownTypeIndex::BlockCollection,
        &GetCollectionSizeInternal<xaml_docs::Block>,
        &GetCollectionItemInternal < xaml_docs::Block, xaml_docs::IBlock > },

        { KnownTypeIndex::InlineCollection,
        &GetCollectionSizeInternal<xaml_docs::Inline>,
        &GetCollectionItemInternal < xaml_docs::Inline, xaml_docs::IInline > },

        { KnownTypeIndex::ColumnDefinitionCollection,
        &GetCollectionSizeInternal<xaml_controls::ColumnDefinition>,
        &GetCollectionItemInternal < xaml_controls::ColumnDefinition, xaml_controls::IColumnDefinition > },

        { KnownTypeIndex::HubSectionCollection,
        &GetCollectionSizeInternal<xaml_controls::HubSection>,
        &GetCollectionItemInternal < xaml_controls::HubSection, xaml_controls::IHubSection > },

        { KnownTypeIndex::RowDefinitionCollection,
        &GetCollectionSizeInternal<xaml_controls::RowDefinition>,
        &GetCollectionItemInternal < xaml_controls::RowDefinition, xaml_controls::IRowDefinition > },

        { KnownTypeIndex::UIElementCollection,
        &GetCollectionSizeInternal<xaml::UIElement>,
        &GetCollectionItemInternal < xaml::UIElement, xaml::IUIElement > }
    };

    static const CollectionAccessor* const FindCollectionAccessor(KnownTypeIndex index)
    {
        const CollectionAccessor* const pEnd = s_collectionAccessors + ARRAYSIZE(s_collectionAccessors);

        const CollectionAccessor* const pResult = std::find_if(
            s_collectionAccessors,
            pEnd,
            [=](const CollectionAccessor& ca)
        {
            return ca.m_index == index;
        });

        return (pResult != pEnd) ? pResult : nullptr;
    }
}

#pragma endregion

#pragma region InternalDebugInterop

#pragma region IInternalDebugInterop

#pragma region Type Methods

HRESULT
InternalDebugInterop::CreateInstance(
    _In_ const CClassInfo* pType,
    _In_opt_ LPCWSTR value,
    _Outptr_ IInspectable** ppInstance)
{
    *ppInstance = nullptr;

    if (value)
    {
        wrl::ComPtr<IInspectable> spValueAsI;

        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(value).Get(), &spValueAsI));

        if (!m_spValueConverter)
        {
            IFC_RETURN(DynamicValueConverter::CreateConverter(&m_spValueConverter));
        }
        return m_spValueConverter->Convert(spValueAsI.Get(), pType, nullptr, ppInstance);
    }
    else
    {
        // If no value is provided, then the caller is creating a FrameworkElement so we go through our
        // ActivationAPI to create the object
        return ActivationAPI::ActivateInstance(pType, ppInstance);
    }
}

// Returns the base type of the given type.
HRESULT
InternalDebugInterop::GetBaseType(
    _In_ const CClassInfo* pType,
    _Outptr_ const CClassInfo** ppBaseType)
{
    *ppBaseType = pType->GetBaseType();

    return S_OK;
}

HRESULT
InternalDebugInterop::GetBaseUri(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ HSTRING* phName)
{
    CDependencyObject *pCDONoRef = nullptr;
    IPALUri* pUriNoRef = nullptr;
    xstring_ptr strName;

    IFCPTR_RETURN(pReference);

    pCDONoRef = static_cast<CDependencyObject*>(static_cast<DependencyObject*>(pReference)->GetHandle());
    pUriNoRef = pCDONoRef->GetBaseUri();

    if (!pUriNoRef)
    {
        IFC_RETURN(wrl_wrappers::HStringReference(L"").CopyTo(phName));
        return S_OK;
    }

    IFC_RETURN(pUriNoRef->GetCanonical(&strName));

    IFC_RETURN(wrl_wrappers::HStringReference(strName.GetBuffer()).CopyTo(phName));

    return S_OK;
}


HRESULT
InternalDebugInterop::GetName(
    _In_ xaml::IFrameworkElement* pReference,
    _Out_ HSTRING* phName)
{
    return pReference->get_Name(phName);
}

// Returns the type display name for the given object.
HRESULT
InternalDebugInterop::GetTypeDisplayNameFromObject(
    _In_ IInspectable* pObject,
    _Out_ HSTRING* phDisplayName)
{
    xruntime_string_ptr strRuntimeTypeName;
    xstring_ptr strTypeName;

    IFC_RETURN(MetadataAPI::GetFriendlyRuntimeClassName(pObject, &strTypeName));

    IFC_RETURN(strTypeName.Promote(&strRuntimeTypeName));

    *phDisplayName = strRuntimeTypeName.DetachHSTRING();

    return S_OK;
}

// Returns the type for the given DO.
HRESULT
InternalDebugInterop::GetTypeFor(
    _In_ xaml::IDependencyObject* pReference,
    _Outptr_ const CClassInfo** ppType)
{
    return MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pReference, ppType);
}

// Returns the full name of the given type.
PCWSTR
InternalDebugInterop::GetTypeName(
    _In_ const CClassInfo* pType)
{
    return pType->GetFullName().GetBuffer();
}

// Returns if the given type is a public type.
bool
InternalDebugInterop::IsTypePublic(
    _In_ const CClassInfo* pType)
{
    return !!pType->IsPublic();
}

#pragma endregion

#pragma region Property Methods

HRESULT
InternalDebugInterop::ClearPropertyValue(
    _In_ xaml::IDependencyObject* pReference,
    _In_ KnownPropertyIndex nPropertyIndex)
{
    CDependencyObject *pOwnerDO = static_cast<CDependencyObject*>(static_cast<DependencyObject*>(pReference)->GetHandle());
    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);

    IFC_RETURN(pOwnerDO->ClearValue(pDP));

    return S_OK;
}

HRESULT
InternalDebugInterop::GetAllDependencyProperties(
    _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection)
{
    CCoreServices* pCore = GetCore();

    IFC_RETURN(PopulateAllDependencyProperties(
        pCore,
        [this](IInspectable* pValue, HSTRING* phstr) -> HRESULT { return m_pDebugTool->ValueToString(pValue, phstr); },
        pPropertyCollection));

    return S_OK;
}

HRESULT
InternalDebugInterop::GetCollectionSize(
    _In_ const DebugTool::DebugPropertyInfo& info,
    _Out_ UINT* pSize)
{
    *pSize = 0;

    IInspectable* pValue = info.GetObjectValue();
    const CollectionAccessor* const pCa = FindCollectionAccessor(info.GetTypeIndex());

    if (!pCa)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(pCa->m_fnGetSize(pValue, pSize));

    return S_OK;
}

HRESULT
InternalDebugInterop::GetCollectionItem(
    _In_ const DebugTool::DebugPropertyInfo& info,
    _In_ UINT index,
    _Outptr_ xaml::IDependencyObject** ppDO)
{
    *ppDO = nullptr;

    IInspectable* pValue = info.GetObjectValue();
    const CollectionAccessor* const pCa = FindCollectionAccessor(info.GetTypeIndex());

    if (!pCa)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(pCa->m_fnGetItem(pValue, index, ppDO));

    return S_OK;
}



HRESULT
InternalDebugInterop::GetContentProperty(
    _In_ xaml::IDependencyObject* pDO,
    _Out_ DebugTool::DebugPropertyInfo** ppPropertyInfo)
{
    ctl::ComPtr<xaml::IDependencyObject> spDOAsI(pDO);
    ctl::ComPtr<DependencyObject> spDO;
    const CDependencyProperty* pContentProperty = nullptr;

    std::unique_ptr<DebugTool::DebugPropertyInfo> spPropertyInfo(new DebugTool::DebugPropertyInfo());

    IFC_RETURN(spDOAsI.As(&spDO));

    pContentProperty = spDO->GetHandle()->GetContentProperty();

    IFC_RETURN(PopulateDebugPropertyInfo(
        pContentProperty,
        nullptr,                            // pSetterDO
        nullptr,                            // pValue
        nullptr,
        BaseValueSourceUnknown,             // baseValueSource
        nullptr,                            // pDebugBindingExpression
        false,                              // isExpression
        false,                              // isNonDefaultValue
        spPropertyInfo.get())
        );

    *ppPropertyInfo = spPropertyInfo.release();

    return S_OK;
}

HRESULT
InternalDebugInterop::GetContentPropertyName(
    _In_ xaml::IDependencyObject* pDO,
    _Out_ LPCWSTR* pName)
{
    ctl::ComPtr<xaml::IDependencyObject> spDOAsI(pDO);
    ctl::ComPtr<DependencyObject> spDO;

    IFC_RETURN(spDOAsI.As(&spDO));

    *pName = spDO->GetHandle()->GetContentProperty()->GetName().GetBuffer();

    return S_OK;
}

HRESULT
InternalDebugInterop::GetDefaultValue(
    _In_ xaml::IDependencyObject* pReference,
    _In_ xaml::IDependencyProperty* pIDP,
    _Outptr_ IInspectable** ppValue)
{
    return GetDefaultValueInternal(pReference, pIDP, ppValue);
}

// Returns the next step of a binding after the given step.
DirectUI::PropertyPathStep*
InternalDebugInterop::GetNextStep(
    _In_ DirectUI::PropertyPathStep *pStep)
{
    return pStep->GetNextStep();
}



// Returns the name of the property being bound to by the given step.
PCWSTR
InternalDebugInterop::GetPropertyName(
    _In_ DirectUI::PropertyPathStep* pStep)
{
    return pStep->DebugGetPropertyName();
}

// Returns the value of the property at the given binding step.
HRESULT
InternalDebugInterop::GetPropertyValue(
    _In_ DirectUI::PropertyPathStep* pStep,
    _Out_ IInspectable** ppValue)
{
    return pStep->GetValue(ppValue);
}

// Get all of the properties on the given DO and add them to the given
// IPropertyCollection.
HRESULT
InternalDebugInterop::GetPropertiesForDO(
    _In_ xaml::IDependencyObject* pReference,
    _In_ bool bGetDefaultValue,
    _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pPropertyCollection)
{
    std::vector<DebugTool::DebugPropertyInfo> propInfoList;

    ASSERT(pPropertyCollection);
    ASSERT(pPropertyCollection->GetSize() == 0);

    IFC_RETURN(DebugTool::GetPropertiesForDO(
        pReference,
        bGetDefaultValue,
        [this](IInspectable* pValue, HSTRING* phstr) -> HRESULT { return m_pDebugTool->ValueToString(pValue, phstr); },
        propInfoList)
        );

    IFC_RETURN(pPropertyCollection->Initialize(propInfoList));

    return S_OK;
}


HRESULT
InternalDebugInterop::GetValue(
    _In_ xaml::IDependencyObject* pReference,
    _In_ xaml::IDependencyProperty* pIDP,
    _Outptr_ IInspectable** ppValue)
{
    return GetValueInternal(pReference, pIDP, ppValue);
}

bool
InternalDebugInterop::IsCollection(
    _In_ const DebugTool::DebugPropertyInfo& info)
{
    return FindCollectionAccessor(info.GetTypeIndex()) != nullptr;
}

// Sets the value of the given property on the given DO.
HRESULT
InternalDebugInterop::SetPropertyValue(
    _In_ xaml::IDependencyObject* pReference,
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_opt_ PCWSTR pszValue)
{
    wrl::ComPtr<IInspectable> spValue;
    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    const CClassInfo* pType = pDP->GetPropertyType();

    IFC_RETURN(CreateInstance(pType, pszValue, &spValue));

    return SetPropertyValue(pReference, nPropertyIndex, spValue.Get());
}

HRESULT
InternalDebugInterop::SetPropertyValue(
    _In_ xaml::IDependencyObject* pReference,
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_opt_ IInspectable* pValue) noexcept
{
    wrl::ComPtr<xaml::IDependencyObject> spDO(pReference);

    // Setting a style.
    wrl::ComPtr<xaml::IStyle> spDOAsStyle;
    if (SUCCEEDED(spDO.As(&spDOAsStyle)) && spDOAsStyle)
    {
        wrl::ComPtr<xaml::ISetterBaseCollection> spSetterCollection;
        wrl::ComPtr<wfc::IVector<xaml::SetterBase*>> spSetterVector;
        wxaml_interop::TypeName setterTargetType;
        unsigned int setterCount = 0;

        ctl::ComPtr<Style> spNewStyle;
        wrl::ComPtr<xaml::ISetterBaseCollection> spNewSetterCollection;
        wrl::ComPtr<wfc::IVector<xaml::SetterBase*>> spNewSetterVector;

        // Get the collection and vectors from the old style.
        IFC_RETURN(spDOAsStyle->get_Setters(&spSetterCollection));
        IFC_RETURN(spDOAsStyle->get_TargetType(&setterTargetType));
        IFC_RETURN(spSetterCollection.As(&spSetterVector));
        IFC_RETURN(spSetterVector->get_Size(&setterCount));

        // Make the new style and get the vectors from the new style.
        IFC_RETURN(ctl::make(&spNewStyle));
        IFC_RETURN(spNewStyle->put_TargetType(setterTargetType));
        IFC_RETURN(spNewStyle.Cast<xaml::IStyle>()->get_Setters(&spNewSetterCollection));
        IFC_RETURN(spNewSetterCollection.As(&spNewSetterVector));

        // Copy the setters from the old style.
        for (size_t k = 0; k < setterCount; k++)
        {
            wrl::ComPtr<xaml::ISetterBase> spSetterBase;
            wrl::ComPtr<xaml::ISetter> spSetter;
            wrl::ComPtr<xaml::IDependencyProperty> spProperty;

            IFC_RETURN(spSetterVector->GetAt(k, &spSetterBase));

            if (SUCCEEDED(spSetterBase.As(&spSetter)) && spSetter)
            {
                IFC_RETURN(spSetter->get_Property(&spProperty));
                auto index = static_cast<DependencyPropertyHandle*>(spProperty.Get())->GetDP()->GetIndex();

                // Only copy the setters that aren't the new one.
                if (index != nPropertyIndex)
                {
                    ctl::ComPtr<Setter> spNewSetter;
                    wrl::ComPtr<xaml::IDependencyProperty> spDP;
                    wrl::ComPtr<IInspectable> spValue;

                    IFC_RETURN(spSetter->get_Property(&spDP));
                    IFC_RETURN(spSetter->get_Value(&spValue));

                    IFC_RETURN(ctl::make(&spNewSetter));

                    IFC_RETURN(spNewSetter->put_Property(spDP.Get()));
                    IFC_RETURN(spNewSetter->put_Value(spValue.Get()));

                    IFC_RETURN(spNewSetterVector->Append(spNewSetter.Cast<xaml::ISetterBase>()));
                }
            }
        }

        // Now add the new setter.
        if (pValue)
        {
            ctl::ComPtr<Setter> spNewSetter;
            wrl::ComPtr<xaml::IDependencyProperty> spDP;

            IFC_RETURN(ctl::make(&spNewSetter));
            IFC_RETURN(MetadataAPI::GetIDependencyProperty(nPropertyIndex, &spDP));

            IFC_RETURN(spNewSetter->put_Property(spDP.Get()));
            IFC_RETURN(spNewSetter->put_Value(pValue));
            IFC_RETURN(spNewSetterVector->Append(spNewSetter.Cast<xaml::ISetterBase>()));
        }

        // Now, find all controls that used this style and restyle them.
        std::queue<wrl::ComPtr<xaml::IDependencyObject>> processingQueue;
        wrl::ComPtr<xaml::IDependencyObject> spRoot;

        IFC_RETURN(GetVisualRoot(&spRoot));
        processingQueue.push(spRoot);
        size_t updateCount = 0;
        while (!processingQueue.empty())
        {
            wrl::ComPtr<xaml::IDependencyObject> spCurrent;
            spCurrent = std::move(processingQueue.front());
            processingQueue.pop();

            // Enqueue the children for processing.
            wrl::ComPtr<xaml::IDependencyObject> spChildren;
            IFC_RETURN(GetChildren(spCurrent.Get(), &spChildren));
            if (spChildren)
            {
                wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildrenAsIVector;
                unsigned int numChildren = 0;

                IFC_RETURN(spChildren.As(&spChildrenAsIVector));
                IFC_RETURN(spChildrenAsIVector->get_Size(&numChildren));
                for (size_t k = 0; k < numChildren; k++)
                {
                    wrl::ComPtr<xaml::IUIElement> spChild;
                    wrl::ComPtr<xaml::IDependencyObject> spChildAsDO;

                    IFC_RETURN(spChildrenAsIVector->GetAt(k, &spChild));
                    IFC_RETURN(spChild.As(&spChildAsDO));
                    processingQueue.push(std::move(spChildAsDO));
                }
            }

            // Replace style, if possible.
            wrl::ComPtr<xaml::IFrameworkElement> spCurrentAsFE;
            if (SUCCEEDED(spCurrent.As(&spCurrentAsFE)) && spCurrentAsFE)
            {
                wrl::ComPtr<xaml::IStyle> spStyleOfCurrent;

                if (SUCCEEDED(spCurrentAsFE->get_Style(&spStyleOfCurrent)) && spStyleOfCurrent.Get() == spDOAsStyle.Get())
                {
                    IFC_RETURN(spCurrentAsFE->put_Style(spNewStyle.Cast<xaml::IStyle>()));
                    updateCount++;
                }
            }
        }

        // Let VS know that either:
        //   1. A style was updated with no consumers (unlikely), or
        //   2. An implicit style update was attempted, which is
        // invalid (implicit styles are applied and dropped at startup).
        if (updateCount == 0)
        {
            return E_INVALID_OPERATION;
        }

        return S_OK;
    }

    wrl::ComPtr<xaml::IDependencyProperty> spDP;
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(nPropertyIndex, &spDP));
    return spDO->SetValue(spDP.Get(), pValue);
}

#pragma endregion

#pragma region Tree Methods

// Return the children of the given DO.  The returned object should
// be QI-ed for an IVector<UIElement*>.
HRESULT
InternalDebugInterop::GetChildren(
    _In_ xaml::IDependencyObject* pReference,
    _Outptr_ xaml::IDependencyObject** ppDO)
{
    return VisualTreeHelper::GetChildrenStatic(pReference, ppDO);
}

HRESULT
InternalDebugInterop::GetParent(
    _In_ xaml::IDependencyObject* pReference,
    _Outptr_ xaml::IDependencyObject** ppDO)
{
    return VisualTreeHelper::GetParentStatic(pReference, ppDO);
}

// Return whether the given DO was generated by a control template.
bool
InternalDebugInterop::HasTemplatedParent(
    _In_ xaml::IDependencyObject* pReference)
{
    CDependencyObject *pObject = static_cast<CDependencyObject*>(static_cast<DependencyObject*>(pReference)->GetHandle());
    return !!pObject->GetTemplatedParent();
}

HRESULT
InternalDebugInterop::HitTest(
    _In_ RECT rect,
    _Inout_ DebugTool::ICollection<xaml::IDependencyObject*>* pElements)
{
    wrl::ComPtr<wfc::IIterable<xaml::UIElement*>> spElements;
    wrl::ComPtr<wfc::IIterator<xaml::UIElement*>> spElementsIterator;
    wf::Rect wfRect;
    boolean hasCurrent = false;

    wfRect.X = static_cast<float>(rect.left);
    wfRect.Y = static_cast<float>(rect.top);
    wfRect.Width = static_cast<float>(rect.right - rect.left);
    wfRect.Height = static_cast<float>(rect.bottom - rect.top);

    const BOOLEAN c_canHitDisabledElements = TRUE;
    const BOOLEAN c_canHitInvisibleElements = TRUE;
    IFC_RETURN(VisualTreeHelper::FindElementsInHostCoordinatesRectStatic(
        wfRect,
        nullptr,
        c_canHitDisabledElements,
        c_canHitInvisibleElements,
        &spElements));

    IFC_RETURN(spElements->First(&spElementsIterator));
    IFC_RETURN(spElementsIterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        wrl::ComPtr<xaml::IUIElement> spElement;
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;

        IFC_RETURN(spElementsIterator->get_Current(&spElement));
        IFC_RETURN(spElement.As(&spElementAsDO));

        // We should only be returning elements in the live tree
        if (static_cast<UIElement*>(spElement.Get())->IsInLiveTree())
        {
            IFC_RETURN(pElements->Append(spElementAsDO.Get()));
        }

        IFC_RETURN(spElementsIterator->MoveNext(&hasCurrent));
    }

    return S_OK;
}

#pragma endregion

#pragma region Framework Methods

HRESULT
InternalDebugInterop::GetPopupRoot(
    _Outptr_ xaml::IDependencyObject** ppPopupRoot)
{
    CCoreServices* pCS = GetCore();
    ctl::ComPtr<DependencyObject> spDO;

    *ppPopupRoot = nullptr;

    CPopupRoot* pPopupRoot = pCS->GetMainPopupRoot();
    if (!pPopupRoot)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pPopupRoot, &spDO));
    if (spDO)
    {
        *ppPopupRoot = static_cast<xaml::IDependencyObject*>(spDO.Detach());
    }

    return S_OK;
}

HRESULT
InternalDebugInterop::GetVisualRoot(
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppRoot)
{
    ctl::ComPtr<DirectUI::DependencyObject> spDO;
    ctl::ComPtr<xaml::IDependencyObject> spDOAsI;

    CCoreServices* pCore = GetCore();
    CDependencyObject *pVisualRoot = pCore->getRootScrollViewer();

    if (!pVisualRoot)
    {
        pVisualRoot = pCore->getVisualRoot();
        if (!pVisualRoot)
        {
            *ppRoot = nullptr;
            return S_OK;
        }
    }

    IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer(pVisualRoot, pVisualRoot->GetTypeIndex(), &spDO));

    IFC_RETURN(spDO.As(&spDOAsI));
    *ppRoot = spDOAsI.Detach();

    return S_OK;
}

#pragma endregion

#pragma region Layout Methods

// Adjusts the given rect from the specified element up to the root.
HRESULT
InternalDebugInterop::AdjustBoundingRectToRoot(
    _In_ xaml::IUIElement* pReference,
    _Inout_ XRECTF* pRect)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(static_cast<UIElement*>(pReference)->GetHandle());
    return pUIElement->AdjustBoundingRectToRoot(pRect);
}

// Returns the global bounds for the given DO.
HRESULT
InternalDebugInterop::GetGlobalBounds(
    _In_ xaml::IUIElement* pReference,
    _In_ bool ignoreClipping,
    _Out_ XRECTF *pBounds)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(static_cast<UIElement*>(pReference)->GetHandle());
    XRECTF_RB bounds = {};
    HRESULT hr = pUIElement->GetGlobalBounds(&bounds, ignoreClipping);
    if (SUCCEEDED(hr))
    {
        *pBounds = ToXRectF(bounds);
    }
    else
    {
        *pBounds = XRECTF();
    }
    return hr;
}

HRESULT
InternalDebugInterop::GetLayoutInfo(
    _In_ xaml::IFrameworkElement* pReference,
    _Out_ XTHICKNESS* pMargin,
    _Out_ XTHICKNESS* pPadding)
{
    CFrameworkElement* pFElement = static_cast<CFrameworkElement*>(static_cast<FrameworkElement*>(pReference)->GetHandle());
    CValue margin;
    const CDependencyProperty* pdp = nullptr;

    IFC_RETURN(pFElement->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Margin, &margin));
    *pMargin = *(margin.AsThickness());

    if ((pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::Control_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::Border_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::Grid_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::StackPanel_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::RelativePanel_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::ItemsPresenter_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::RichTextBlock_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::TextBlock_Padding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterPadding)) != nullptr ||
        (pdp = pFElement->GetPropertyByIndexInline(KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterPadding)) != nullptr)
    {
        CValue padding;
        IFC_RETURN(pFElement->GetValue(pdp, &padding));
        *pPadding = *(padding.AsThickness());
    }

    return S_OK;
}

// Gets the local render transform, if any, for the specified element.
bool
InternalDebugInterop::GetRenderTransformLocal(
    _In_ xaml::IUIElement* pReference,
    _Inout_ CMILMatrix* pMatrix)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(static_cast<UIElement*>(pReference)->GetHandle());
    CTransform *pTransform = pUIElement->GetRenderTransformLocal();
    if (pTransform)
    {
        pTransform->GetTransform(pMatrix);
        return true;
    }

    return false;
}

// Returns the x and y scale dimensions in the given matrix.
void
InternalDebugInterop::GetScaleDimensions(
    _In_ CMILMatrix* pMatrix,
    _Out_ XFLOAT* prScaleX,
    _Out_ XFLOAT* prScaleY)
{
    pMatrix->GetScaleDimensions(prScaleX, prScaleY);
}

// Returns if the given element is visible.
bool
InternalDebugInterop::IsVisible(
    _In_ xaml::IUIElement* pReference)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(static_cast<UIElement*>(pReference)->GetHandle());
    return !!pUIElement->IsVisible();
}

// This is also under discussion...basically, this is meant to "update"
// the app after the style gets updated...if we can't update the style,
// then we don't need this.
HRESULT
InternalDebugInterop::VisualRootUpdateLayout()
{
    wrl::ComPtr<xaml::IDependencyObject> spVisualRoot;
    wrl::ComPtr<xaml::IUIElement> spRootAsUI;

    IFC_RETURN(GetVisualRoot(&spVisualRoot));

    if (SUCCEEDED(spVisualRoot.As(&spRootAsUI)) && spRootAsUI)
    {
        IFC_RETURN(spRootAsUI->UpdateLayout());
    }

    return S_OK;
}

#pragma endregion

#pragma region Feature Methods

HRESULT
InternalDebugInterop::GetAnimationSpeedFactor(
    _Out_ INT64* piAnimationSpeed)
{
    CCoreServices *pCoreServices = GetCore();

    *piAnimationSpeed = 1;

    // If we are using the core slow factor get it, otherwise we must be using the registry key
    if (pCoreServices->GetAnimationSlowFactor() > 0)
    {
        *piAnimationSpeed = pCoreServices->GetAnimationSlowFactor();
    }
    else
    {
        HKEY hKey;
        DWORD dwExtendAnimations = 0;
        DWORD cbSize = sizeof(DWORD);

        IFC_RETURN(RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\DirectUI", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey));
        IFC_RETURN(RegQueryValueEx(hKey, L"ThemeAnimationSlowDownFactor", nullptr, nullptr, (LPBYTE)&dwExtendAnimations, &cbSize));

        *piAnimationSpeed = static_cast<INT64>(dwExtendAnimations);

        IGNOREHR(RegCloseKey(hKey));
    }

    return S_OK;
}

HRESULT
InternalDebugInterop::GetShowingFrameCounter(
    _Out_ bool* pbFrameCounterEnabled)
{
    bool bEnabled = false;
    CCoreServices *pCoreServices = GetCore();

    *pbFrameCounterEnabled = false;

    IFC_RETURN(CoreImports::Host_GetEnableFrameRateCounter(pCoreServices, &bEnabled));

    *pbFrameCounterEnabled = !!bEnabled;

    return S_OK;
}

HRESULT
InternalDebugInterop::SetAnimationSpeedFactor(
    _In_ INT64 iAnimationSpeed)
{
    CCoreServices *pCoreServices = GetCore();
    pCoreServices->SetAnimationSlowFactor(iAnimationSpeed);

    return S_OK;
}

HRESULT
InternalDebugInterop::SetShowingFrameCounter(
    _In_ bool bFrameCounterEnabled)
{
    bool bEnabled = bFrameCounterEnabled;
    CCoreServices *pCoreServices = GetCore();

    IFC_RETURN(CoreImports::Host_SetEnableFrameRateCounter(pCoreServices, bEnabled));

    return S_OK;
}

#pragma endregion

#pragma region Other Methods

// Converts the given property value of the specified type to a string.
HRESULT
InternalDebugInterop::ConvertValueToString(
    _In_ wf::IPropertyValue* pPropertyValue,
    _In_ wf::PropertyType propertyType,
    _Out_ HSTRING* phstr)
{
    if (!ValueConversionHelpers::CanConvertValueToString(propertyType))
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(ValueConversionHelpers::ConvertValueToString(pPropertyValue, propertyType, phstr));

    return S_OK;
}

DebugTool::IDebugTool*
InternalDebugInterop::GetDebugToolNoRef()
{
    return m_pDebugTool;
}

#pragma endregion

#pragma endregion

#pragma region Getters and Setters

void
InternalDebugInterop::SetDebugTool(_In_ DebugTool::IDebugTool* pDebugTool)
{
    m_pDebugTool = pDebugTool;
}

#pragma endregion

#pragma region Constructor and Destructor

InternalDebugInterop::InternalDebugInterop() :
    m_pDebugTool(nullptr)
{

}

HRESULT
InternalDebugInterop::RuntimeClassInitialize()
{
    return E_NOTIMPL;
}

HRESULT
InternalDebugInterop::RuntimeClassInitialize(
    _In_ HWND hwndParent)
{
    return S_OK;
}

#pragma endregion

#pragma region Statics

HRESULT
InternalDebugInterop::GetDefaultValueInternal(
    _In_ xaml::IDependencyObject* pReference,
    _In_ xaml::IDependencyProperty* pIDP,
    _Outptr_ IInspectable** ppValue)
{
    DependencyObject *pDO = static_cast<DependencyObject*>(pReference);
    const CDependencyProperty* pDP = static_cast<DependencyPropertyHandle*>(pIDP)->GetDP();
    const CDependencyProperty* pUnderlyingDP = nullptr;

    if (SUCCEEDED(MetadataAPI::GetUnderlyingDependencyProperty(pDP, &pUnderlyingDP)))
    {
        IFC_RETURN(pDO->GetDefaultValueInternal(pUnderlyingDP, ppValue));
    }
    else
    {
        // This is a custom regular property, but not a DP, so no default value.
        *ppValue = nullptr;
    }

    return S_OK;
}

HRESULT
InternalDebugInterop::GetValueInternal(
    _In_ xaml::IDependencyObject* pReference,
    _In_ xaml::IDependencyProperty* pIDP,
    _Outptr_ IInspectable** ppValue)
{
    DependencyObject *pDO = static_cast<DependencyObject*>(pReference);
    const CDependencyProperty* pDP = static_cast<DependencyPropertyHandle*>(pIDP)->GetDP();

    if (auto customProperty = pDP->AsOrNull<CCustomProperty>())
    {
        return customProperty->GetXamlPropertyNoRef()->GetValue(pReference, ppValue);
    }

    return pDO->GetValue(pDP, ppValue);
}

// Populates debug property info.  Bails early and didPopulate is false if
// the property info cannot be populated.
HRESULT
InternalDebugInterop::PopulateBaseValueDebugPropertyInfo(
    _In_ CCoreServices* pCore,
    _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
    _In_opt_ DependencyObject* pObject,
    _In_opt_ const CDependencyProperty* pDP,
    _In_ KnownPropertyIndex propertyIndex,
    _In_ bool bGetDefaultValue,
    _Inout_ bool& didPopulate,
    _Out_ DebugTool::DebugPropertyInfo* pDebugPropertyInfo)
{
    CDependencyObject *pDO = pObject ? static_cast<CDependencyObject*>(pObject->GetHandle()) : nullptr;

    ::BaseValueSource baseValueSource = ::BaseValueSourceUnknown; // Use the global BaseValueSource to escape DirectUI and get from corep.h.
    bool isNonDefaultValue = false;

    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<DebugTool::IDebugBindingExpression> spDebugBindingExpression;
    ctl::ComPtr<DependencyObject> spDO (pObject);
    ctl::ComPtr<xaml::IDependencyObject> spIDO;

    didPopulate = false;

    if (pDO && !pDP)
    {
        pDP = pDO->GetPropertyByIndexInline(propertyIndex);
    }

    // If the DP is null (such as when it doesn't apply for this DO), or it isn't public, we bail early
    // but don't set the 'didPopulate' flag.
    if (!pDP || !(pDP->IsPublic() || propertyIndex == KnownPropertyIndex::DependencyObject_Name))
    {
        return S_OK;
    }

    if (pObject)
    {
        EffectiveValueEntry* pValueEntry = pObject->TryGetEffectiveValueEntry(propertyIndex);

        IFC_RETURN(spDO.As(&spIDO));

        // Determine the source of the property.
        if (pValueEntry)
        {
            baseValueSource = static_cast<::BaseValueSource>(pValueEntry->GetBaseValueSource());
        }
        else
        {
            baseValueSource = pDO->GetBaseValueSource(pDP);

            // Unfortunately, for inherited values or ones obtained by method call, base value source is reported as Default.
            // In search scenario properties such as ActualWidth will not be returned even their values are not
            // default, hence forced inclusion).  Note: there might be other cases where this might need to be done.
            isNonDefaultValue = pDP->IsPropMethodCall() || pDP->IsInherited();
        }

        isNonDefaultValue |= (baseValueSource != ::BaseValueSourceDefault);

        // If the property value is the default value and we're not interested in default values,
        // then bail.  However, we're always interested in collections, regardless of whether they're
        // default or not.
        if (!isNonDefaultValue && !bGetDefaultValue && !pObject->IsCollection(pDP->GetPropertyType()->GetIndex()))
        {
            return S_OK;
        }

        // Query the binding expression, if available.
        IFC_RETURN(TryGetBindingExpression(pObject, propertyIndex, &spDebugBindingExpression));

        // Query the actual property value for this object.
        if (   !pDP->ShouldBindingGetValueUseCheckOnDemandProperty()
            || !pObject->GetHandle()->CheckOnDemandProperty(pDP).IsNull())
        {
            // Get the current value of the property
            IFC_RETURN(pObject->GetValue(pDP, &spValue));
        }
    }
    else
    {
        CValue defaultValue;

        // If no DO is specified, then we're interested in the default value for the property.  This
        // is used when dumping the default values for all the properties.
        pDP->GetDefaultValue(pCore, /* pReferenceObject */ pDO, pDP->GetTargetType(), &defaultValue);
        baseValueSource = ::BaseValueSourceDefault;

        IFC_RETURN(CValueBoxer::UnboxObjectValue(&defaultValue, nullptr /*pTargetType*/, &spValue));
    }

    IFC_RETURN(PopulateDebugPropertyInfo(
        pDP,
        spIDO.Get(),
        spValue.Get(),
        valueToString,
        baseValueSource,
        spDebugBindingExpression.Get(),
        spDebugBindingExpression.Get() != nullptr,
        isNonDefaultValue,
        pDebugPropertyInfo));

    didPopulate = true;

    return S_OK;
}

// Creates a DebugPropertyInfo instance from a DO and DP.
HRESULT
InternalDebugInterop::PopulateDebugPropertyInfo(
    _In_opt_ const CDependencyProperty* pDP,
    _In_opt_ xaml::IDependencyObject* pSetterDO,
    _Inout_opt_ IInspectable* pValue,
    _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
    _In_ BaseValueSource baseValueSource,
    _In_opt_ DebugTool::IDebugBindingExpression* pDebugBindingExpression,
    _In_ bool isExpression,
    _In_ bool isNonDefaultValue,
    _Out_ DebugTool::DebugPropertyInfo* pDebugPropertyInfo)
{
    wrl_wrappers::HString strValue;
    wrl_wrappers::HString strName;

    wrl::ComPtr<IInspectable> fixedValue;
    // Convert the value to a string.  For enums, we query the numeric value of the enum
    // and output that.
    if (pValue && pDP->GetPropertyType()->IsEnum())
    {
        std::array<wchar_t, 32> valueBuffer;
        UINT enumValue = 0;

        if (SUCCEEDED(GetEnumValueFromKnownWinRTBox(pValue, pDP->GetPropertyType(), &enumValue)))
        {
            swprintf_s(valueBuffer.data(), valueBuffer.size(), L"%d", enumValue);
            IFC_RETURN(strValue.Set(valueBuffer.data()));
        }
        else
        {
            IFC_RETURN(strValue.Set(L"(unknown)"));
        }
    }
    else if (pValue && ctl::is<IThemeResourceExpression>(pValue))
    {
        // If the value is a ThemeResourceExpression, we want to get the value of the expression (i.e. SolidColorBrush)
        // we we can then pass to the valueToString method.
        ctl::ComPtr<ThemeResourceExpression> spExpression;

        IFC_RETURN(ctl::do_query_interface(spExpression, pValue));
        IFC_RETURN(spExpression->GetValue(static_cast<DependencyObject*>(pSetterDO), pDP, &fixedValue));
        IFC_RETURN(valueToString(fixedValue.Get(), strValue.GetAddressOf()));
    }
    else if (pValue && ctl::is<wf::IPropertyValue>(pValue) && pDP->IsGridLengthProperty())
    {
        // Similar to the IThemeResourceExpression case, we want to rewrap the value inside a grid length
        // object, this way the valueToString method will know it is actually dealing with a length property and
        // not some random double. This is helpful because we interpret "NaN" to be "Auto" in the case of Width/Height.

        // The incoming property value could be a string or double so we should get the class info for the value
        const CClassInfo* propertyInfo = nullptr;
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pValue, &propertyInfo));

        // Now that we have the correct class info we can box the object to the correct CValue.
        CValue value;
        BoxerBuffer buffer;
        DependencyObject* pMOR = nullptr;
        IFC_RETURN(CValueBoxer::BoxObjectValue(&value, propertyInfo, pValue, &buffer, &pMOR));
        auto releaseMOR = wil::scope_exit([&pMOR]()
        {
            ctl::release_interface(pMOR);
        });

        // Now that the CValue we have reprents either a string or double representation of the value, we can unbox to an actual GridLength object.
        const CClassInfo* lengthInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::GridLength);
        IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, lengthInfo, lengthInfo->IsNullable(), __uuidof(wf::IReference<xaml::GridLength>), reinterpret_cast<void**>(fixedValue.GetAddressOf())));
        IFC_RETURN(valueToString(fixedValue.Get(), strValue.GetAddressOf()));
    }
    else if (valueToString)
    {
        IFC_RETURN(valueToString(pValue, strValue.GetAddressOf()));
    }
    else
    {
        IFC_RETURN(strValue.Set(L"(no value)"));
    }

    IFC_RETURN(strName.Set(pDP->GetName().IsNullOrEmpty() ? L"(unnamed)" : pDP->GetName().GetBuffer()));

    // For attached properties, the name in the DependencyPropertyInfo doesn't include the owner
    // (for example, the name is "Left" instead of "Canvas.Left".
    if (pDP->IsAttached())
    {
        XStringBuilder strAttachedPropertyNameBuilder;

        VERIFYHR(strAttachedPropertyNameBuilder.Initialize(pDP->GetDeclaringType()->GetName().GetCount() + /* '.' */ 1 + pDP->GetName().GetCount()));
        VERIFYHR(strAttachedPropertyNameBuilder.Append(pDP->GetDeclaringType()->GetName()));
        VERIFYHR(strAttachedPropertyNameBuilder.AppendChar(L'.'));
        VERIFYHR(strAttachedPropertyNameBuilder.Append(pDP->GetName()));

        strName.Set(strAttachedPropertyNameBuilder.GetBuffer());
    }

    IInspectable *reportedValue = fixedValue ? fixedValue.Get() : pValue;

    // Get value type.
    const CClassInfo* pValueType = nullptr;
    IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(reportedValue, &pValueType));

    // Get whether value is a collection.
    bool isValueCollection = false;
    if (pValue)
    {
        wrl::ComPtr<IInspectable> spValue(pValue);
        wrl::ComPtr<IUntypedVector> spValueAsUntyped;
        if (SUCCEEDED(spValue.As(&spValueAsUntyped)) && spValueAsUntyped)
        {
            isValueCollection = true;
        }
    }

    wrl::ComPtr<xaml::IDependencyProperty> spIDP;
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(pDP->GetIndex(), &spIDP));
    IFC_RETURN(pDebugPropertyInfo->Initialize(
        pDP->GetIndex(),
        pDP->GetPropertyType()->GetIndex(),
        spIDP.Get(),
        pSetterDO,
        strName.GetRawBuffer(nullptr),
        strValue.GetRawBuffer(nullptr),
        pDP->GetPropertyType(),
        pDP->GetTargetType(),
        pValueType,
        baseValueSource,
        isNonDefaultValue,
        pDP->IsExternalReadOnly(),
        pDP->GetPropertyType()->IsCollection(),
        isValueCollection,
        pDebugBindingExpression,
        isExpression,
        reportedValue));

    return S_OK;
}

HRESULT
InternalDebugInterop::PopulateAllDependencyProperties(
    _In_ CCoreServices* pCore,
    _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
    _Inout_ DebugTool::ICollection<DebugTool::DebugPropertyInfo>* pCollection)
{
    // PopulateBaseValueDebugPropertyInfo will end up calling CDependencyObject::GetPropertyByIndexInline
    // which can't handle custom properties, so only iterate over known property counts
    auto iterator = Diagnostics::EnumIterator<KnownPropertyIndex>(Diagnostics::EnumIterator<KnownPropertyIndex>::Begin());

    while (*iterator != static_cast<KnownPropertyIndex>(KnownDependencyPropertyCount))
    {
        KnownPropertyIndex prop = *iterator;
        const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(prop);

        if (pDP)
        {
            bool didPopulate = false;
            DebugTool::DebugPropertyInfo debugPropertyInfo;

            IFC_RETURN(PopulateBaseValueDebugPropertyInfo(
                pCore,                      // pCore
                valueToString,              // valueToString
                nullptr,                    // pObject
                pDP,                        // pProperty
                prop,                       // propertyIndex
                true,                       // pGetDefaultValue
                didPopulate,                // didPopulate
                &debugPropertyInfo)         // pDebugPropertyInfo
                );

            if (didPopulate)
            {
                pCollection->Append(std::move(debugPropertyInfo));
            }
        }
        iterator++;
    }

    return S_OK;
}

// Adds info to the given list for just the locally-set property values
// which have data stored here.  This is especially valuable to get
// any binding expression values.
HRESULT
InternalDebugInterop::PopulateDebugPropertyInfoCollection(
    _In_ DependencyObject* pDO,
    _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
    _In_ std::vector<DebugTool::DebugPropertyInfo>& collection,
    _In_ bool getDefaultValues)
{
    auto& pStore = pDO->GetHandle()->GetValueTable();

    if (pStore)
    {
        for (auto& prop : *pStore)
        {
            // Skip built-in properties. We already processed those.
            if (!MetadataAPI::IsKnownIndex(prop.first))
            {
                bool didPopulate = false;
                DebugTool::DebugPropertyInfo debugPropertyInfo;

                IFC_RETURN(PopulateBaseValueDebugPropertyInfo(
                    DXamlCore::GetCurrent()->GetHandle(),   // pCore
                    valueToString,                          // valueToString
                    pDO,                                    // pObject
                    nullptr,                                // pProperty
                    prop.first,                             // propertyIndex
                    getDefaultValues,                       // pGetDefaultValue
                    didPopulate,                            // didPopulate
                    &debugPropertyInfo)                     // pDebugPropertyInfo
                    );

                if (didPopulate)
                {
                    collection.push_back(std::move(debugPropertyInfo));
                }
            }
        }
    }

    return S_OK;
}

HRESULT InternalDebugInterop::TryGetBindingExpression(
    _In_ DirectUI::DependencyObject* pDO,
    _In_ KnownPropertyIndex propertyIndex,
    _COM_Outptr_result_maybenull_ DebugTool::IDebugBindingExpression** ppDebugBindingExpression)
{
    IFCPTR_RETURN(ppDebugBindingExpression);
    *ppDebugBindingExpression = nullptr;

    EffectiveValueEntry* pValueEntry = pDO->TryGetEffectiveValueEntry(propertyIndex);

    // Query the binding expression, if available.
    if (pValueEntry && pValueEntry->IsExpression())
    {
        ctl::ComPtr<IInspectable> spExpressionValue = pValueEntry->GetBaseValue();

        // We want to ignore if this fails since the QI can fail if this isn't a BindingExpression.
        auto spDebugExpression = spExpressionValue.AsOrNull<DebugTool::IDebugBindingExpression>();
        *ppDebugBindingExpression = spDebugExpression.Detach();
    }

    return S_OK;
}


#pragma endregion

#pragma region Private Member Methods

CCoreServices*
InternalDebugInterop::GetCore()
{
    CCoreServices* pCS = DXamlCore::GetCurrent()->GetHandle();
    ASSERT(pCS != nullptr);

    return pCS;
}

bool
InternalDebugInterop::IsInvalidProperty(_In_ const CDependencyProperty* const prop)
{
    return (prop->GetDeclaringType()->IsCollection() && prop->IsContentProperty()) ||
        // When an app creates a custom DP, we keep two types of properties in our
        // custom metadata cache, one of them is the custom DP and the other is
        // just a regular property. The regular property is not exposed and should
        // never go back to the app. Our metadata seems a little backwards because
        // CustomDependencyProperties don't have the "public" metadata bit set,
        // but otherwise we only want to hand public properties back.
        prop->Is<CCustomProperty>() ||
        (!prop->IsPublic() && !prop->Is<CCustomDependencyProperty>()) ||
        prop->GetIndex() == KnownPropertyIndex::UIElement_ChildrenInternal ||
        prop->GetIndex() == KnownPropertyIndex::FrameworkElement_Resources ||
        prop->GetIndex() == KnownPropertyIndex::UnknownType_UnknownProperty ||
        prop->GetTargetType()->GetIndex() == KnownTypeIndex::UnknownType ||
        prop->GetPropertyType()->GetIndex() == KnownTypeIndex::TransitionTarget;

}

#pragma endregion

#pragma endregion
