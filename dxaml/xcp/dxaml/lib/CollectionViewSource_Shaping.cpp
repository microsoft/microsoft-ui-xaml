// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Experimental WPF-style shaping (sort, filter, group) for CollectionViewSource.
//      See docs\design-notes\CollectionViewSourceShaping-POC.md.

#include "precomp.h"
#include <algorithm>
#include <vector>
#include "CollectionViewSource.g.h"
#include "SortDescription.g.h"
#include "PropertyGroupDescription.g.h"
#include "VectorCollectionView.g.h"
#include "GroupedDataCollectionView.g.h"
#include "FrameworkElement.g.h"
#include "CollectionViewManager.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"

using namespace DirectUI;

namespace
{
    // Listeners are created with fListenToChanges = false, so SourceChanged is never called.
    struct ShapingPropertyPathHost final : public IPropertyPathListenerHost
    {
        _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString) override
        {
            *pszTraceString = L"";
            return S_OK;
        }

        _Check_return_ HRESULT SourceChanged() override
        {
            return S_OK;
        }
    };

    ShapingPropertyPathHost s_shapingPropertyPathHost;

    // Returned by DeferRefresh. Holds the CollectionViewSource (as WPF's deferral holds its view) so a
    // pending refresh still runs; Close is idempotent.
    class ShapingRefreshDeferral final :
        public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::WinRt>, wf::IClosable>
    {
        InspectableClass(L"Microsoft.UI.Xaml.Data.CollectionViewSourceRefreshDeferral", BaseTrust);

    public:
        HRESULT RuntimeClassInitialize(_In_ CollectionViewSource* pOwner)
        {
            m_spOwner = static_cast<xaml_data::ICollectionViewSource*>(pOwner);
            return S_OK;
        }

        IFACEMETHODIMP Close() override
        {
            if (!m_spOwner)
            {
                return S_OK;
            }

            auto pOwner = static_cast<CollectionViewSource*>(m_spOwner.Get());
            IFC_RETURN(pOwner->CheckThread());

            // Released first so a Close re-entered from the refresh is a no-op.
            auto spOwner = std::move(m_spOwner);
            IFC_RETURN(pOwner->EndDeferRefresh());
            return S_OK;
        }

    private:
        Microsoft::WRL::ComPtr<xaml_data::ICollectionViewSource> m_spOwner;
    };

    // Descriptor collection that tells its owning CollectionViewSource to rebuild View when it changes.
    template <typename T>
    class __declspec(novtable) ShapingDescriptionCollection : public ObservableTrackerCollection<T>
    {
    public:
        _Check_return_ HRESULT SetOwner(_In_ CollectionViewSource* pOwner)
        {
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

    protected:
        ShapingDescriptionCollection() = default;

        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index) override
        {
            IFC_RETURN(ObservableTrackerCollection<T>::RaiseVectorChanged(action, index));

            if (m_ownerRef)
            {
                ctl::ComPtr<xaml_data::ICollectionViewSource> spOwner;
                IFC_RETURN(m_ownerRef.As(&spOwner));
                if (spOwner)
                {
                    IFC_RETURN(spOwner.Cast<CollectionViewSource>()->OnShapingDescriptionsChanged());
                }
            }

            return S_OK;
        }

    private:
        ctl::WeakRefPtr m_ownerRef;
    };

    // Read-only vector whose contents are replaced wholesale and announced with a single Reset.
    class __declspec(novtable) ShapedVector : public ReadOnlyObservableTrackerCollection<IInspectable*>
    {
    public:
        _Check_return_ HRESULT ReplaceAll(_In_ const std::vector<ctl::ComPtr<IInspectable>>& items)
        {
            IFC_RETURN(TrackerCollection<IInspectable*>::Clear());
            Reserve(static_cast<uint32_t>(items.size()));
            for (const auto& item : items)
            {
                IFC_RETURN(TrackerCollection<IInspectable*>::Append(item.Get()));
            }

            IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
            return S_OK;
        }

    protected:
        ShapedVector() = default;
    };

    // A computed group. It is its own item list (as with IsSourceGrouped and no ItemsPath) and exposes
    // Key (IGrouping<K,T> idiom) and Name (WPF CollectionViewGroup) to bindings.
    class __declspec(novtable) ShapedGroup :
        public ShapedVector,
        public xaml_data::ICustomPropertyProvider
    {
        BEGIN_INTERFACE_MAP(ShapedGroup, ShapedVector)
            INTERFACE_ENTRY(ShapedGroup, xaml_data::ICustomPropertyProvider)
        END_INTERFACE_MAP(ShapedGroup, ShapedVector)

    public:
        void SetKey(_In_opt_ IInspectable* pKey)
        {
            m_spKey = pKey;
        }

        IInspectable* GetKeyNoRef() const
        {
            return m_spKey.Get();
        }

        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override
        {
            wxaml_interop::TypeName typeName = {};
            typeName.Kind = wxaml_interop::TypeKind_Primitive;
            IFC_RETURN(WindowsCreateString(STR_LEN_PAIR(L"Object"), &typeName.Name));
            *pValue = typeName;
            return S_OK;
        }

        IFACEMETHODIMP GetCustomProperty(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** ppValue) override
        {
            *ppValue = nullptr;
            if (wrl_wrappers::HStringReference(L"Key") == name || wrl_wrappers::HStringReference(L"Name") == name)
            {
                IFC_RETURN(CustomProperty::CreateObjectProperty(name, &ShapedGroup::GetKeyPropertyValue, ppValue));
            }
            return S_OK;
        }

        IFACEMETHODIMP GetIndexedProperty(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** ppValue) override
        {
            *ppValue = nullptr;
            return S_OK;
        }

        IFACEMETHODIMP GetStringRepresentation(_Out_ HSTRING* pValue) override
        {
            *pValue = nullptr;
            if (m_spKey)
            {
                IFC_RETURN(FrameworkElement::GetStringFromObject(m_spKey.Get(), pValue));
            }
            return S_OK;
        }

    protected:
        ShapedGroup() = default;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICustomPropertyProvider)))
            {
                *ppObject = static_cast<xaml_data::ICustomPropertyProvider*>(this);
            }
            else
            {
                return ShapedVector::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    private:
        static _Check_return_ HRESULT GetKeyPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
        {
            ctl::ComPtr<IInspectable> spTarget(pTarget);
            ctl::ComPtr<xaml_data::ICustomPropertyProvider> spProvider;
            IFC_RETURN(spTarget.As(&spProvider));
            IFC_RETURN(spProvider.Cast<ShapedGroup>()->m_spKey.CopyTo(ppValue));
            return S_OK;
        }

        // Not a TrackerPtr: ObservableTrackerCollection::OnReferenceTrackerWalk is final.
        ctl::ComPtr<IInspectable> m_spKey;
    };

    struct ShapingKeySelector
    {
        ctl::ComPtr<PropertyPathListener> listener; // Null means "the item itself", as in WPF.
        bool isDescending = false;
    };

    _Check_return_ HRESULT CreateKeySelector(_In_opt_ HSTRING path, _Out_ ShapingKeySelector* pSelector)
    {
        UINT32 length = 0;
        LPCWSTR szPath = WindowsGetStringRawBuffer(path, &length);

        if (length == 0)
        {
            return S_OK;
        }

        PropertyPathParser parser{};
        IFC_RETURN(parser.SetSource(szPath, nullptr));
        IFC_RETURN(ctl::make<PropertyPathListener>(&s_shapingPropertyPathHost, &parser, false /* fListenToChanges */, false /* fUseWeakReferenceForSource */, &pSelector->listener));
        return S_OK;
    }

    _Check_return_ HRESULT EvaluateKey(_In_ const ShapingKeySelector& selector, _In_opt_ IInspectable* pItem, _Outptr_result_maybenull_ IInspectable** ppKey)
    {
        *ppKey = nullptr;

        if (!selector.listener)
        {
            *ppKey = pItem;
            ctl::addref_interface(pItem);
            return S_OK;
        }

        HRESULT hr = selector.listener->SetSource(pItem);
        if (SUCCEEDED(hr) && selector.listener->FullPathExists())
        {
            hr = selector.listener->GetValue(ppKey);
        }

        // Do not keep the item alive through the listener.
        IGNOREHR(selector.listener->SetSource(nullptr));
        return hr;
    }

    _Check_return_ HRESULT GetNumericValue(_In_ wf::IPropertyValue* pValue, wf::PropertyType type, _Out_ DOUBLE* pResult)
    {
        switch (type)
        {
        case wf::PropertyType_UInt8:  { BYTE v = 0;   IFC_RETURN(pValue->GetUInt8(&v));  *pResult = v; break; }
        case wf::PropertyType_Char16: { WCHAR v = 0;  IFC_RETURN(pValue->GetChar16(&v)); *pResult = v; break; }
        case wf::PropertyType_Int16:  { INT16 v = 0;  IFC_RETURN(pValue->GetInt16(&v));  *pResult = v; break; }
        case wf::PropertyType_UInt16: { UINT16 v = 0; IFC_RETURN(pValue->GetUInt16(&v)); *pResult = v; break; }
        case wf::PropertyType_Int32:  { INT32 v = 0;  IFC_RETURN(pValue->GetInt32(&v));  *pResult = v; break; }
        case wf::PropertyType_UInt32: { UINT32 v = 0; IFC_RETURN(pValue->GetUInt32(&v)); *pResult = v; break; }
        case wf::PropertyType_Int64:  { INT64 v = 0;  IFC_RETURN(pValue->GetInt64(&v));  *pResult = static_cast<DOUBLE>(v); break; }
        case wf::PropertyType_UInt64: { UINT64 v = 0; IFC_RETURN(pValue->GetUInt64(&v)); *pResult = static_cast<DOUBLE>(v); break; }
        case wf::PropertyType_Single: { FLOAT v = 0;  IFC_RETURN(pValue->GetSingle(&v)); *pResult = v; break; }
        case wf::PropertyType_Double: { IFC_RETURN(pValue->GetDouble(pResult)); break; }
        default: return E_INVALIDARG;
        }

        return S_OK;
    }

    // Values that cannot be ordered compare equal, so the stable sort keeps their source order.
    int CompareKeys(_In_opt_ IInspectable* pLeft, _In_opt_ IInspectable* pRight)
    {
        if (!pLeft || !pRight)
        {
            return (pLeft ? 1 : 0) - (pRight ? 1 : 0);
        }

        auto spLeft = ctl::query_interface_cast<wf::IPropertyValue>(pLeft);
        auto spRight = ctl::query_interface_cast<wf::IPropertyValue>(pRight);
        wf::PropertyType leftType = wf::PropertyType_Empty;
        wf::PropertyType rightType = wf::PropertyType_Empty;

        if (!spLeft || !spRight || FAILED(spLeft->get_Type(&leftType)) || FAILED(spRight->get_Type(&rightType)))
        {
            return 0;
        }

        if (leftType == wf::PropertyType_String && rightType == wf::PropertyType_String)
        {
            wrl_wrappers::HString left;
            wrl_wrappers::HString right;
            UINT32 leftLength = 0;
            UINT32 rightLength = 0;

            if (FAILED(spLeft->GetString(left.GetAddressOf())) || FAILED(spRight->GetString(right.GetAddressOf())))
            {
                return 0;
            }

            LPCWSTR szLeft = left.GetRawBuffer(&leftLength);
            LPCWSTR szRight = right.GetRawBuffer(&rightLength);
            const int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, 0, szLeft, static_cast<int>(leftLength), szRight, static_cast<int>(rightLength), nullptr, nullptr, 0);
            return (result == 0) ? 0 : result - CSTR_EQUAL;
        }

        if (DirectUI::PropertyValue::IsNumericPropertyType(leftType) && DirectUI::PropertyValue::IsNumericPropertyType(rightType))
        {
            DOUBLE left = 0;
            DOUBLE right = 0;

            if (FAILED(GetNumericValue(spLeft.Get(), leftType, &left)) || FAILED(GetNumericValue(spRight.Get(), rightType, &right)))
            {
                return 0;
            }

            return (left < right) ? -1 : ((left > right) ? 1 : 0);
        }

        if (leftType == rightType)
        {
            switch (leftType)
            {
            case wf::PropertyType_Boolean:
            {
                BOOLEAN left = FALSE;
                BOOLEAN right = FALSE;
                if (SUCCEEDED(spLeft->GetBoolean(&left)) && SUCCEEDED(spRight->GetBoolean(&right)))
                {
                    return static_cast<int>(!!left) - static_cast<int>(!!right);
                }
                break;
            }

            case wf::PropertyType_DateTime:
            {
                wf::DateTime left = {};
                wf::DateTime right = {};
                if (SUCCEEDED(spLeft->GetDateTime(&left)) && SUCCEEDED(spRight->GetDateTime(&right)))
                {
                    return (left.UniversalTime < right.UniversalTime) ? -1 : ((left.UniversalTime > right.UniversalTime) ? 1 : 0);
                }
                break;
            }

            case wf::PropertyType_TimeSpan:
            {
                wf::TimeSpan left = {};
                wf::TimeSpan right = {};
                if (SUCCEEDED(spLeft->GetTimeSpan(&left)) && SUCCEEDED(spRight->GetTimeSpan(&right)))
                {
                    return (left.Duration < right.Duration) ? -1 : ((left.Duration > right.Duration) ? 1 : 0);
                }
                break;
            }

            default:
                break;
            }
        }

        return 0;
    }

    template <typename T>
    _Check_return_ HRESULT CreateShapingDescriptionCollection(
        _In_ CollectionViewSource* pOwner,
        _Outptr_ ShapingDescriptionCollection<T>** ppCollection)
    {
        ctl::ComPtr<ShapingDescriptionCollection<T>> spCollection;
        IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<T>>::CreateInstance(spCollection.ReleaseAndGetAddressOf()));
        IFC_RETURN(spCollection->SetOwner(pOwner));
        *ppCollection = spCollection.Detach();
        return S_OK;
    }
}

_Check_return_ HRESULT SortDescriptionFactory::CreateInstanceImpl(
    _In_opt_ HSTRING propertyName,
    xaml_data::ListSortDirection direction,
    _Outptr_ xaml_data::ISortDescription** ppInstance)
{
    ctl::ComPtr<SortDescription> spDescription;

    IFCEXPECTRC_RETURN(direction == xaml_data::ListSortDirection_Ascending || direction == xaml_data::ListSortDirection_Descending, E_INVALIDARG);

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription->put_Direction(direction));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceImpl(
    _In_opt_ HSTRING propertyName,
    _Outptr_ xaml_data::IPropertyGroupDescription** ppInstance)
{
    ctl::ComPtr<PropertyGroupDescription> spDescription;

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT CollectionView::SetShapingOwner(_In_ CollectionViewSource* pOwner)
{
    SetPtrValue(m_tpShapingOwner, static_cast<xaml_data::ICollectionViewSource*>(pOwner));
    return S_OK;
}

_Check_return_ HRESULT CollectionView::GetShapingOwner(_Outptr_ CollectionViewSource** ppOwner)
{
    *ppOwner = nullptr;
    IFC_RETURN(CheckThread());
    IFCEXPECTRC_RETURN(m_tpShapingOwner, RO_E_CLOSED);
    *ppOwner = static_cast<CollectionViewSource*>(m_tpShapingOwner.Get());
    ctl::addref_interface(*ppOwner);
    return S_OK;
}

HRESULT CollectionView::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewShaping)) && m_tpShapingOwner)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewShaping*>(this);
        AddRefOuter();
        return S_OK;
    }

    return CollectionViewGenerated::QueryInterfaceImpl(iid, ppObject);
}

IFACEMETHODIMP CollectionView::get_SortDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->get_SortDescriptionsImpl(ppValue));
    return S_OK;
}

IFACEMETHODIMP CollectionView::get_GroupDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->get_GroupDescriptionsImpl(ppValue));
    return S_OK;
}

IFACEMETHODIMP CollectionView::get_Filter(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->get_FilterImpl(ppValue));
    return S_OK;
}

IFACEMETHODIMP CollectionView::put_Filter(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->put_FilterImpl(pValue));
    return S_OK;
}

IFACEMETHODIMP CollectionView::Refresh()
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->RefreshImpl());
    return S_OK;
}

IFACEMETHODIMP CollectionView::DeferRefresh(_Outptr_ wf::IClosable** ppReturnValue)
{
    ctl::ComPtr<CollectionViewSource> spOwner;
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue = nullptr;
    IFC_RETURN(GetShapingOwner(spOwner.ReleaseAndGetAddressOf()));
    IFC_RETURN(spOwner->DeferRefreshImpl(ppReturnValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    if (!m_tpSortDescriptions)
    {
        ctl::ComPtr<ShapingDescriptionCollection<xaml_data::SortDescription*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection(this, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpSortDescriptions, spCollection.Get());
    }

    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_GroupDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue)
{
    if (!m_tpGroupDescriptions)
    {
        ctl::ComPtr<ShapingDescriptionCollection<xaml_data::PropertyGroupDescription*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection(this, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpGroupDescriptions, spCollection.Get());
    }

    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_FilterImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    IFC_RETURN(m_tpFilter.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_FilterImpl(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    SetPtrValue(m_tpFilter, pValue);
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::RefreshImpl()
{
    IFC_RETURN(RefreshShapedView());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::DeferRefreshImpl(_Outptr_ wf::IClosable** ppReturnValue)
{
    Microsoft::WRL::ComPtr<ShapingRefreshDeferral> spDeferral;
    IFC_RETURN(wrl::MakeAndInitialize<ShapingRefreshDeferral>(&spDeferral, this));
    ++m_deferRefreshLevel;
    *ppReturnValue = spDeferral.Detach();
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::EndDeferRefresh()
{
    ASSERT(m_deferRefreshLevel > 0);
    if (m_deferRefreshLevel == 0 || --m_deferRefreshLevel > 0 || !m_isRefreshPending)
    {
        return S_OK;
    }

    m_isRefreshPending = false;
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::OnShapingDescriptionsChanged()
{
    ctl::ComPtr<IInspectable> spSource;
    bool isShapingRequested = false;

    if (m_isComputingShape)
    {
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(get_Source(spSource.ReleaseAndGetAddressOf()));
    if (!spSource)
    {
        return S_OK;
    }

    // A shaped View is reshaped in place, so a consumer holding View (for example through
    // ICollectionViewShaping) keeps a live view. Only switching between flat and grouped output
    // needs a different view class and therefore a new View.
    if (m_tpShapedOutput)
    {
        UINT groupCount = 0;
        if (m_tpGroupDescriptions)
        {
            IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
        }

        if ((groupCount > 0) == m_isShapedOutputGrouped)
        {
            IFC_RETURN(RefreshShapedView());
            return S_OK;
        }
    }

    // Leave an unshaped View untouched unless shaping is now requested.
    IFC_RETURN(IsShapingRequested(spSource.Get(), &isShapingRequested));
    if (!m_tpShapedOutput && !isShapingRequested)
    {
        return S_OK;
    }

    IFC_RETURN(EnsureView(spSource.Get()));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::IsShapingRequested(_In_ IInspectable* pSource, _Out_ bool* pIsRequested)
{
    BOOLEAN isSourceGrouped = FALSE;
    UINT sortCount = 0;
    UINT groupCount = 0;

    *pIsRequested = false;

    if (m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }

    if (sortCount == 0 && groupCount == 0 && !m_tpFilter)
    {
        return S_OK;
    }

    // IsSourceGrouped and ICollectionViewFactory sources keep their existing behavior.
    IFC_RETURN(get_IsSourceGrouped(&isSourceGrouped));
    *pIsRequested = !isSourceGrouped && !ctl::is<xaml_data::ICollectionViewFactory>(pSource);
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::CreateShapedView(_In_ IInspectable* pSource, _Outptr_ xaml_data::ICollectionView** ppView)
{
    ctl::ComPtr<wfc::IVector<IInspectable*>> spSourceVector;
    ctl::ComPtr<ShapedVector> spOutput;
    UINT groupCount = 0;
    HRESULT hr = S_OK;

    *ppView = nullptr;

    IFC_RETURN(CollectionViewManager::GetVectorFromSource(pSource, spSourceVector.ReleaseAndGetAddressOf()));
    IFC_RETURN(ctl::ComObject<ShapedVector>::CreateInstance(spOutput.ReleaseAndGetAddressOf()));

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }

    m_isShapedOutputGrouped = (groupCount > 0);
    SetPtrValue(m_tpShapedSource, spSourceVector.Get());
    SetPtrValue(m_tpShapedOutput, spOutput.Get());

    m_isComputingShape = true;
    hr = ComputeShapedOutput();
    m_isComputingShape = false;
    IFC_RETURN(hr);

    // The output is read-only, so index-based writes through View fail instead of reordering the source.
    if (m_isShapedOutputGrouped)
    {
        IFC_RETURN(GroupedDataCollectionView::CreateInstance(spOutput.Get(), nullptr /* pItemsPath */, ppView));
    }
    else
    {
        IFC_RETURN(VectorCollectionView::CreateInstance(spOutput.Get(), ppView));
    }

    IFC_RETURN(static_cast<CollectionView*>(*ppView)->SetShapingOwner(this));

    auto spObservable = spSourceVector.AsOrNull<wfc::IObservableVector<IInspectable*>>();
    if (spObservable)
    {
        SetPtrValue(m_tpShapedSourceObservable, spObservable.Get());
        IFC_RETURN(m_epShapedSourceVectorChangedHandler.AttachEventHandler(m_tpShapedSourceObservable.Get(),
            [this](wfc::IObservableVector<IInspectable*>* pSender, wfc::IVectorChangedEventArgs* pArgs)
            {
                auto pegThis = ctl::try_make_autopeg(this);
                if (pegThis)
                {
                    IFC_RETURN(RefreshShapedView());
                }
                return S_OK;
            }));
    }

    return S_OK;
}

// Full recompute: filter, then stable multi-key sort, then group by first appearance.
_Check_return_ HRESULT CollectionViewSource::ComputeShapedOutput()
{
    struct Entry
    {
        ctl::ComPtr<IInspectable> item;
        std::vector<ctl::ComPtr<IInspectable>> sortKeys;
        ctl::ComPtr<IInspectable> groupKey;
    };

    std::vector<ShapingKeySelector> sortSelectors;
    ShapingKeySelector groupSelector;
    bool hasGroupSelector = false;
    ctl::ComPtr<xaml_data::ICollectionViewFilter> spFilter = m_tpFilter.Get();
    std::vector<Entry> entries;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    UINT size = 0;

    if (m_tpSortDescriptions)
    {
        UINT count = 0;
        IFC_RETURN(m_tpSortDescriptions->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_data::ISortDescription> spDescription;
            IFC_RETURN(m_tpSortDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            if (!spDescription)
            {
                continue;
            }

            wrl_wrappers::HString path;
            xaml_data::ListSortDirection direction = xaml_data::ListSortDirection_Ascending;
            ShapingKeySelector selector;
            IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
            IFC_RETURN(spDescription->get_Direction(&direction));
            IFC_RETURN(CreateKeySelector(path.Get(), &selector));
            selector.isDescending = (direction == xaml_data::ListSortDirection_Descending);
            sortSelectors.push_back(std::move(selector));
        }
    }

    // ICollectionViewGroup is single level, so only the first group description is used.
    if (m_isShapedOutputGrouped)
    {
        UINT count = 0;
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&count));
        for (UINT i = 0; i < count && !hasGroupSelector; ++i)
        {
            ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
            IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            if (spDescription)
            {
                wrl_wrappers::HString path;
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                IFC_RETURN(CreateKeySelector(path.Get(), &groupSelector));
                hasGroupSelector = true;
            }
        }
    }

    IFC_RETURN(m_tpShapedSource->get_Size(&size));
    entries.reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        Entry entry;
        IFC_RETURN(m_tpShapedSource->GetAt(i, entry.item.ReleaseAndGetAddressOf()));

        if (spFilter)
        {
            boolean accepted = true;
            IFC_RETURN(spFilter->Invoke(entry.item.Get(), &accepted));
            if (!accepted)
            {
                continue;
            }
        }

        for (const auto& selector : sortSelectors)
        {
            ctl::ComPtr<IInspectable> spKey;
            IFC_RETURN(EvaluateKey(selector, entry.item.Get(), spKey.ReleaseAndGetAddressOf()));
            entry.sortKeys.push_back(std::move(spKey));
        }

        if (hasGroupSelector)
        {
            IFC_RETURN(EvaluateKey(groupSelector, entry.item.Get(), entry.groupKey.ReleaseAndGetAddressOf()));
        }

        entries.push_back(std::move(entry));
    }

    if (!sortSelectors.empty())
    {
        std::stable_sort(entries.begin(), entries.end(), [&sortSelectors](const Entry& left, const Entry& right)
        {
            for (size_t i = 0; i < sortSelectors.size(); ++i)
            {
                const int result = CompareKeys(left.sortKeys[i].Get(), right.sortKeys[i].Get());
                if (result != 0)
                {
                    return sortSelectors[i].isDescending ? (result > 0) : (result < 0);
                }
            }
            return false;
        });
    }

    if (!m_isShapedOutputGrouped)
    {
        outputItems.reserve(entries.size());
        for (auto& entry : entries)
        {
            outputItems.push_back(std::move(entry.item));
        }
    }
    else
    {
        std::vector<ctl::ComPtr<ShapedGroup>> groups;
        std::vector<std::vector<ctl::ComPtr<IInspectable>>> groupItems;

        for (auto& entry : entries)
        {
            size_t groupIndex = groups.size();
            for (size_t g = 0; g < groups.size(); ++g)
            {
                bool areEqual = false;
                IFC_RETURN(DirectUI::PropertyValue::AreEqual(groups[g]->GetKeyNoRef(), entry.groupKey.Get(), &areEqual));
                if (areEqual)
                {
                    groupIndex = g;
                    break;
                }
            }

            if (groupIndex == groups.size())
            {
                ctl::ComPtr<ShapedGroup> spGroup;
                IFC_RETURN(ctl::ComObject<ShapedGroup>::CreateInstance(spGroup.ReleaseAndGetAddressOf()));
                spGroup->SetKey(entry.groupKey.Get());
                groups.push_back(std::move(spGroup));
                groupItems.emplace_back();
            }

            groupItems[groupIndex].push_back(std::move(entry.item));
        }

        outputItems.reserve(groups.size());
        for (size_t g = 0; g < groups.size(); ++g)
        {
            IFC_RETURN(groups[g]->ReplaceAll(groupItems[g]));
            outputItems.push_back(ctl::as_iinspectable(groups[g].Get()));
        }
    }

    IFC_RETURN(static_cast<ShapedVector*>(m_tpShapedOutput.Get())->ReplaceAll(outputItems));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::RefreshShapedView()
{
    ctl::ComPtr<xaml_data::ICollectionView> spView;
    ctl::ComPtr<IInspectable> spCurrentItem;
    BOOLEAN isCurrentValid = FALSE;
    HRESULT hr = S_OK;

    // Changes raised while computing (for example from a Filter callback) are ignored.
    if (!m_tpShapedOutput || m_isComputingShape)
    {
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(get_View(spView.ReleaseAndGetAddressOf()));
    if (spView)
    {
        IFC_RETURN(spView->get_CurrentItem(spCurrentItem.ReleaseAndGetAddressOf()));
    }

    m_isComputingShape = true;
    hr = ComputeShapedOutput();
    m_isComputingShape = false;
    IFC_RETURN(hr);

    if (spView)
    {
        if (spCurrentItem)
        {
            IFC_RETURN(spView->MoveCurrentTo(spCurrentItem.Get(), &isCurrentValid));
        }

        if (!isCurrentValid)
        {
            IFC_RETURN(spView->MoveCurrentToFirst(&isCurrentValid));
        }
    }

    return S_OK;
}

void CollectionViewSource::DetachShapedSource()
{
    if (m_epShapedSourceVectorChangedHandler)
    {
        auto spSource = m_tpShapedSourceObservable.GetSafeReference();
        if (spSource)
        {
            VERIFYHR(m_epShapedSourceVectorChangedHandler.DetachEventHandler(spSource.Get()));
        }
        else
        {
            m_epShapedSourceVectorChangedHandler = ctl::EventPtr<VectorChangedEventCallback>();
        }
    }

    m_tpShapedSourceObservable.Clear();
    m_tpShapedSource.Clear();
    m_tpShapedOutput.Clear();
    m_isShapedOutputGrouped = false;
}
