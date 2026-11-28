// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Hierarchy in a CollectionView (ICollectionViewHierarchy): a tree built from a flat source by key and parent key
//      (PropertyHierarchyDescription), sorted per sibling list and filtered with the ancestors of matches kept as
//      context. The view's own list is every kept node in pre-order. See
//      docs\design-notes\CollectionViewSourceShaping-POC.md.

#include "precomp.h"
#include "VectorCollectionView_HierarchyInternal.h"
#include "HierarchyDescription.g.h"
#include "PropertyHierarchyDescription.g.h"
#include "DebugOutput.h"
#include <XStringUtils.h>
#include <XamlTraceLogging.h>

using namespace DirectUI;
using namespace DirectUI::ShapingEngine;

namespace
{
    // A local diagnostic event (like CollectionViewContainedFailure): no telemetry keyword, so it is not uploaded.
    void TraceHierarchyDataError(UINT duplicateCount, UINT cycleCount)
    {
        TraceLoggingWrite(
            g_hTraceProvider,
            "CollectionViewHierarchyDataError",
            TraceLoggingDescription("A hierarchical CollectionView found duplicate keys or parent-key cycles."),
            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
            TraceLoggingUInt32(duplicateCount, "DuplicateCount"),
            TraceLoggingUInt32(cycleCount, "CycleCount"));
    }

    void LogHierarchyDataError(UINT duplicateCount, UINT cycleCount)
    {
        if (duplicateCount == 0 && cycleCount == 0)
        {
            return;
        }

        TraceHierarchyDataError(duplicateCount, cycleCount);
        if (DebugOutput::IsLoggingForBindingEnabled())
        {
            if (duplicateCount > 0)
            {
                DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
                    L"CollectionView hierarchy: %u item(s) have a key an earlier item has; they are shown as roots and have no children.\n", duplicateCount));
            }
            if (cycleCount > 0)
            {
                DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
                    L"CollectionView hierarchy: %u parent-key cycle(s); the item of each cycle that comes first in the source is shown as a root.\n", cycleCount));
            }
        }
    }

    _Check_return_ HRESULT BoxNodeKey(const NodeKey& key, _Outptr_result_maybenull_ IInspectable** ppValue)
    {
        *ppValue = nullptr;
        switch (key.type)
        {
        case wf::PropertyType_Empty:
            return S_OK;
        case wf::PropertyType_String:
            {
                wrl_wrappers::HString text;
                IFC_RETURN(text.Set(key.text.c_str(), static_cast<UINT32>(key.text.size())));
                return PropertyValue::CreateFromString(text.Get(), ppValue);
            }
        case wf::PropertyType_Int32:
            return PropertyValue::CreateFromInt32(static_cast<INT32>(key.bits), ppValue);
        case wf::PropertyType_UInt32:
            return PropertyValue::CreateFromUInt32(static_cast<UINT32>(key.bits), ppValue);
        case wf::PropertyType_Int64:
            return PropertyValue::CreateFromInt64(static_cast<INT64>(key.bits), ppValue);
        case wf::PropertyType_UInt64:
            return PropertyValue::CreateFromUInt64(key.bits, ppValue);
        case wf::PropertyType_Int16:
            return PropertyValue::CreateFromInt16(static_cast<INT16>(key.bits), ppValue);
        case wf::PropertyType_UInt16:
            return PropertyValue::CreateFromUInt16(static_cast<UINT16>(key.bits), ppValue);
        case wf::PropertyType_UInt8:
            return PropertyValue::CreateFromUInt8(static_cast<BYTE>(key.bits), ppValue);
        case wf::PropertyType_Char16:
            return PropertyValue::CreateFromChar16(static_cast<WCHAR>(key.bits), ppValue);
        case wf::PropertyType_Boolean:
            return PropertyValue::CreateFromBoolean(key.bits != 0, ppValue);
        case wf::PropertyType_Double:
            {
                DOUBLE value = 0;
                memcpy(&value, &key.bits, sizeof(value));
                return PropertyValue::CreateFromDouble(value, ppValue);
            }
        case wf::PropertyType_Single:
            {
                FLOAT value = 0;
                const UINT32 bits = static_cast<UINT32>(key.bits);
                memcpy(&value, &bits, sizeof(value));
                return PropertyValue::CreateFromSingle(value, ppValue);
            }
        case wf::PropertyType_Guid:
            {
                GUID value = {};
                memcpy(&value, &key.bits, sizeof(UINT64));
                memcpy(reinterpret_cast<BYTE*>(&value) + sizeof(UINT64), &key.bits2, sizeof(UINT64));
                return PropertyValue::CreateFromGuid(value, ppValue);
            }
        case wf::PropertyType_DateTime:
            return PropertyValue::CreateFromDateTime(wf::DateTime{ static_cast<INT64>(key.bits) }, ppValue);
        case wf::PropertyType_TimeSpan:
            return PropertyValue::CreateFromTimeSpan(wf::TimeSpan{ static_cast<INT64>(key.bits) }, ppValue);
        default:
            return S_OK;
        }
    }
}

_Check_return_ HRESULT DirectUI::ShapingEngine::MakeNodeKey(_In_opt_ IInspectable* pValue, _Out_ NodeKey* pKey)
{
    *pKey = NodeKey();
    if (!pValue)
    {
        return S_OK;
    }

    auto spValue = ctl::query_interface_cast<wf::IPropertyValue>(pValue);
    wf::PropertyType type = wf::PropertyType_OtherType;
    if (spValue)
    {
        IFC_RETURN(spValue->get_Type(&type));
    }

    pKey->type = type;
    switch (type)
    {
    case wf::PropertyType_Empty:
        return S_OK;
    case wf::PropertyType_String:
        {
            wrl_wrappers::HString text;
            UINT32 length = 0;
            IFC_RETURN(spValue->GetString(text.GetAddressOf()));
            LPCWSTR szText = text.GetRawBuffer(&length);
            if (length == 0)
            {
                pKey->type = wf::PropertyType_Empty;
                return S_OK;
            }
            pKey->text.assign(szText, length);
            return S_OK;
        }
    case wf::PropertyType_Int32:
        {
            INT32 value = 0;
            IFC_RETURN(spValue->GetInt32(&value));
            pKey->bits = static_cast<UINT64>(static_cast<INT64>(value));
            return S_OK;
        }
    case wf::PropertyType_UInt32:
        {
            UINT32 value = 0;
            IFC_RETURN(spValue->GetUInt32(&value));
            pKey->bits = value;
            return S_OK;
        }
    case wf::PropertyType_Int64:
        {
            INT64 value = 0;
            IFC_RETURN(spValue->GetInt64(&value));
            pKey->bits = static_cast<UINT64>(value);
            return S_OK;
        }
    case wf::PropertyType_UInt64:
        IFC_RETURN(spValue->GetUInt64(&pKey->bits));
        return S_OK;
    case wf::PropertyType_Int16:
        {
            INT16 value = 0;
            IFC_RETURN(spValue->GetInt16(&value));
            pKey->bits = static_cast<UINT64>(static_cast<INT64>(value));
            return S_OK;
        }
    case wf::PropertyType_UInt16:
        {
            UINT16 value = 0;
            IFC_RETURN(spValue->GetUInt16(&value));
            pKey->bits = value;
            return S_OK;
        }
    case wf::PropertyType_UInt8:
        {
            BYTE value = 0;
            IFC_RETURN(spValue->GetUInt8(&value));
            pKey->bits = value;
            return S_OK;
        }
    case wf::PropertyType_Char16:
        {
            WCHAR value = 0;
            IFC_RETURN(spValue->GetChar16(&value));
            pKey->bits = value;
            return S_OK;
        }
    case wf::PropertyType_Boolean:
        {
            boolean value = false;
            IFC_RETURN(spValue->GetBoolean(&value));
            pKey->bits = value ? 1 : 0;
            return S_OK;
        }
    case wf::PropertyType_Double:
        {
            DOUBLE value = 0;
            IFC_RETURN(spValue->GetDouble(&value));
            value = (value == 0) ? 0 : value;
            memcpy(&pKey->bits, &value, sizeof(value));
            return S_OK;
        }
    case wf::PropertyType_Single:
        {
            FLOAT value = 0;
            IFC_RETURN(spValue->GetSingle(&value));
            value = (value == 0) ? 0 : value;
            UINT32 bits = 0;
            memcpy(&bits, &value, sizeof(value));
            pKey->bits = bits;
            return S_OK;
        }
    case wf::PropertyType_Guid:
        {
            GUID value = {};
            IFC_RETURN(spValue->GetGuid(&value));
            memcpy(&pKey->bits, &value, sizeof(UINT64));
            memcpy(&pKey->bits2, reinterpret_cast<BYTE*>(&value) + sizeof(UINT64), sizeof(UINT64));
            return S_OK;
        }
    case wf::PropertyType_DateTime:
        {
            wf::DateTime value = {};
            IFC_RETURN(spValue->GetDateTime(&value));
            pKey->bits = static_cast<UINT64>(value.UniversalTime);
            return S_OK;
        }
    case wf::PropertyType_TimeSpan:
        {
            wf::TimeSpan value = {};
            IFC_RETURN(spValue->GetTimeSpan(&value));
            pKey->bits = static_cast<UINT64>(value.Duration);
            return S_OK;
        }
    default:
        {
            ctl::ComPtr<IUnknown> spIdentity;
            IFC_RETURN(ctl::do_query_interface(spIdentity, pValue));
            pKey->type = wf::PropertyType_OtherType;
            pKey->isObject = true;
            pKey->bits = reinterpret_cast<UINT64>(spIdentity.Get());
            return S_OK;
        }
    }
}

// ---- CollectionViewNode ----

HRESULT CollectionViewNode::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(CollectionViewNode)))
    {
        *ppObject = static_cast<CollectionViewNode*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewNode)))
    {
        *ppObject = static_cast<xaml_data::ICollectionViewNode*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICustomPropertyProvider)))
    {
        *ppObject = static_cast<xaml_data::ICustomPropertyProvider*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::INotifyPropertyChanged)))
    {
        *ppObject = static_cast<xaml_data::INotifyPropertyChanged*>(this);
    }
    else
    {
        return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

void CollectionViewNode::OnReferenceTrackerWalk(INT walkType)
{
    m_propertyChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
    ctl::WeakReferenceSource::OnReferenceTrackerWalk(walkType);
}

_Check_return_ HRESULT CollectionViewNode::Initialize(_In_ const std::shared_ptr<HierarchyStore>& spStore, NodeId id, _In_opt_ IInspectable* pItem, _In_opt_ IInspectable* pKey, bool isContext)
{
    m_store = spStore;
    m_id = id;
    m_isContext = isContext;
    SetPtrValue(m_tpItem, pItem);
    SetPtrValue(m_tpKey, pKey);
    return S_OK;
}

void CollectionViewNode::Detach(bool isRaised)
{
    m_store.reset();
    m_id = c_noNode;
    if (m_tpChildren)
    {
        IGNOREHR(m_tpChildren->Detach(isRaised));
    }
}

_Check_return_ HRESULT CollectionViewNode::SetItem(_In_opt_ IInspectable* pItem)
{
    if (m_tpItem.Get() == pItem)
    {
        return S_OK;
    }
    SetPtrValue(m_tpItem, pItem);
    IFC_RETURN(RaisePropertyChanged(L"Item"));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewNode::SetIsContext(bool isContext)
{
    if (m_isContext == isContext)
    {
        return S_OK;
    }
    m_isContext = isContext;
    IFC_RETURN(RaisePropertyChanged(L"IsContext"));
    return S_OK;
}

bool CollectionViewNode::IsBoundTo(const HierarchyStore* pStore) const
{
    return m_id != c_noNode && m_store.lock().get() == pStore;
}

IFACEMETHODIMP CollectionViewNode::get_Item(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpItem.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::get_Key(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpKey.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::get_Parent(_Outptr_result_maybenull_ xaml_data::ICollectionViewNode** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue = nullptr;
    IFC_RETURN(CheckThread());
    auto spStore = m_store.lock();
    if (spStore && m_id != c_noNode)
    {
        IFC_RETURN(spStore->GetParentProjection(m_id, ppValue));
    }
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::get_Children(_Outptr_ wfc::IObservableVector<IInspectable*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    if (!m_tpChildren)
    {
        ctl::ComPtr<NodeListVector> spChildren;
        IFC_RETURN(ctl::ComObject<NodeListVector>::CreateInstance(spChildren.ReleaseAndGetAddressOf()));
        auto spStore = m_store.lock();
        if (spStore && m_id != c_noNode)
        {
            spChildren->Bind(spStore, m_id);
            IFC_RETURN(spStore->RegisterChildren(m_id, spChildren.Get()));
        }
        SetPtrValue(m_tpChildren, spChildren.Get());
    }

    *ppValue = static_cast<wfc::IObservableVector<IInspectable*>*>(m_tpChildren.Get());
    (*ppValue)->AddRef();
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::get_IsContext(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    IFC_RETURN(CheckThread());
    *pValue = m_isContext;
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::get_HasChildren(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = FALSE;
    IFC_RETURN(CheckThread());
    auto spStore = m_store.lock();
    if (spStore && m_id != c_noNode)
    {
        *pValue = spStore->KeptCount(m_id) > 0;
    }
    return S_OK;
}

void CollectionViewNode::SetChildren(_In_ NodeListVector* pChildren)
{
    SetPtrValue(m_tpChildren, pChildren);
}

IFACEMETHODIMP CollectionViewNode::add_PropertyChanged(_In_ xaml_data::IPropertyChangedEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ARG_NOTNULL_RETURN(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(pToken);
    IFC_RETURN(CheckThread());
    m_propertyChangedHandlers.AddHandler(pHandler, pToken);
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::remove_PropertyChanged(EventRegistrationToken token)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_propertyChangedHandlers.RemoveHandler(token));
    return S_OK;
}

// A name of its own, so a binding's accessor cache (keyed by this TypeName) never applies a node's accessors to
// another provider.
IFACEMETHODIMP CollectionViewNode::get_Type(_Out_ wxaml_interop::TypeName* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    IFC_RETURN(CheckThread());
    wxaml_interop::TypeName typeName = {};
    typeName.Kind = wxaml_interop::TypeKind_Metadata;
    IFC_RETURN(WindowsCreateString(STR_LEN_PAIR(L"Microsoft.UI.Xaml.Data.ICollectionViewNode"), &typeName.Name));
    *pValue = typeName;
    return S_OK;
}

_Check_return_ HRESULT CollectionViewNode::FromTarget(_In_opt_ IInspectable* pTarget, _Outptr_ CollectionViewNode** ppNode)
{
    *ppNode = nullptr;
    auto spNode = ctl::query_interface_cast<CollectionViewNode>(pTarget);
    if (!spNode)
    {
        return E_INVALIDARG;
    }
    *ppNode = spNode.Detach();
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::GetCustomProperty(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue = nullptr;
    IFC_RETURN(CheckThread());
    if (wrl_wrappers::HStringReference(L"Item") == name)
    {
        IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_Item(ppResult));
            return S_OK;
        }, ppValue));
    }
    else if (wrl_wrappers::HStringReference(L"Key") == name)
    {
        IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_Key(ppResult));
            return S_OK;
        }, ppValue));
    }
    else if (wrl_wrappers::HStringReference(L"Parent") == name)
    {
        IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_Parent(reinterpret_cast<xaml_data::ICollectionViewNode**>(ppResult)));
            return S_OK;
        }, ppValue));
    }
    else if (wrl_wrappers::HStringReference(L"Children") == name)
    {
        IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_Children(reinterpret_cast<wfc::IObservableVector<IInspectable*>**>(ppResult)));
            return S_OK;
        }, ppValue));
    }
    else if (wrl_wrappers::HStringReference(L"HasChildren") == name)
    {
        IFC_RETURN(CustomProperty::CreateBooleanProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            BOOLEAN value = FALSE;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_HasChildren(&value));
            IFC_RETURN(DirectUI::PropertyValue::CreateFromBoolean(value, ppResult));
            return S_OK;
        }, ppValue));
    }
    else if (wrl_wrappers::HStringReference(L"IsContext") == name)
    {
        IFC_RETURN(CustomProperty::CreateBooleanProperty(name, [](IInspectable* pTarget, IInspectable** ppResult) -> HRESULT
        {
            ctl::ComPtr<CollectionViewNode> spNode;
            BOOLEAN value = FALSE;
            IFC_RETURN(FromTarget(pTarget, spNode.ReleaseAndGetAddressOf()));
            IFC_RETURN(spNode->get_IsContext(&value));
            IFC_RETURN(DirectUI::PropertyValue::CreateFromBoolean(value, ppResult));
            return S_OK;
        }, ppValue));
    }
    return S_OK;
}
IFACEMETHODIMP CollectionViewNode::GetIndexedProperty(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** ppValue)
{
    *ppValue = nullptr;
    return S_OK;
}

IFACEMETHODIMP CollectionViewNode::GetStringRepresentation(_Out_ HSTRING* pValue)
{
    *pValue = nullptr;
    if (m_tpItem)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(m_tpItem.Get(), pValue));
    }
    return S_OK;
}

_Check_return_ HRESULT CollectionViewNode::RaisePropertyChanged(_In_z_ const WCHAR* pName)
{
    if (m_propertyChangedHandlers.Empty())
    {
        return S_OK;
    }

    ctl::ComPtr<DirectUI::PropertyChangedEventArgs> spArgs;
    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(spArgs->put_PropertyName(wrl_wrappers::HStringReference(pName).Get()));
    IFC_RETURN(ContainRaiseFailure(m_propertyChangedHandlers.Raise(ctl::as_iinspectable(this), spArgs.Get())));
    return S_OK;
}

// ---- NodeListVector ----

HRESULT NodeListVector::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(NodeListVector)))
    {
        *ppObject = static_cast<NodeListVector*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IObservableVector<IInspectable*>)))
    {
        *ppObject = static_cast<wfc::IObservableVector<IInspectable*>*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable*>)))
    {
        *ppObject = static_cast<wfc::IVector<IInspectable*>*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable*>)))
    {
        *ppObject = static_cast<wfc::IIterable<IInspectable*>*>(this);
    }
    else
    {
        return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

void NodeListVector::OnReferenceTrackerWalk(INT walkType)
{
    m_handlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
    ctl::WeakReferenceSource::OnReferenceTrackerWalk(walkType);
}

void NodeListVector::Bind(_In_ const std::shared_ptr<HierarchyStore>& spStore, NodeId owner)
{
    m_store = spStore;
    m_owner = owner;
}

_Check_return_ HRESULT NodeListVector::Detach(bool isRaised)
{
    const bool wasBound = !m_store.expired();
    m_store.reset();
    m_owner = c_noNode;
    if (isRaised && wasBound)
    {
        IFC_RETURN(RaiseChanged(wfc::CollectionChange_Reset, 0));
    }
    return S_OK;
}

_Check_return_ HRESULT NodeListVector::RaiseChanged(wfc::CollectionChange change, UINT index)
{
    if (m_handlers.Empty())
    {
        return S_OK;
    }

    ctl::ComPtr<wfc::IVectorChangedEventArgs> spArgs;
    VectorChangedEventArgs* pArgs = nullptr;
    IFC_RETURN(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(spArgs.ReleaseAndGetAddressOf()));
    spArgs.CastTo(&pArgs);
    IFC_RETURN(pArgs->put_CollectionChange(change));
    IFC_RETURN(pArgs->put_Index(index));
    IFC_RETURN(ContainRaiseFailure(m_handlers.Raise(this, spArgs.Get())));
    return S_OK;
}

IFACEMETHODIMP NodeListVector::add_VectorChanged(_In_ wfc::VectorChangedEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ARG_NOTNULL_RETURN(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(pToken);
    m_handlers.AddHandler(pHandler, pToken);
    return S_OK;
}

IFACEMETHODIMP NodeListVector::remove_VectorChanged(EventRegistrationToken token)
{
    IFC_RETURN(m_handlers.RemoveHandler(token));
    return S_OK;
}

IFACEMETHODIMP NodeListVector::GetAt(UINT index, _Outptr_ IInspectable** ppItem)
{
    ARG_VALIDRETURNPOINTER(ppItem);
    *ppItem = nullptr;
    IFC_RETURN(CheckThread());
    auto spStore = m_store.lock();
    IFCEXPECTRC_RETURN(spStore && index < spStore->KeptCount(m_owner), E_BOUNDS);
    ctl::ComPtr<xaml_data::ICollectionViewNode> spNode;
    IFC_RETURN(spStore->GetProjection(spStore->KeptAt(m_owner, index), spNode.ReleaseAndGetAddressOf()));
    *ppItem = spNode.Detach();
    return S_OK;
}

IFACEMETHODIMP NodeListVector::get_Size(_Out_ UINT* pSize)
{
    ARG_VALIDRETURNPOINTER(pSize);
    IFC_RETURN(CheckThread());
    auto spStore = m_store.lock();
    *pSize = spStore ? spStore->KeptCount(m_owner) : 0;
    return S_OK;
}

_Check_return_ HRESULT NodeListVector::Snapshot(_Outptr_ InternalList** ppList)
{
    ctl::ComPtr<InternalList> spList;
    std::vector<ctl::ComPtr<IInspectable>> items;
    UINT size = 0;
    IFC_RETURN(get_Size(&size));
    items.reserve(size);
    for (UINT i = 0; i < size; ++i)
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC_RETURN(GetAt(i, spItem.ReleaseAndGetAddressOf()));
        items.push_back(std::move(spItem));
    }
    IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spList.ReleaseAndGetAddressOf()));
    IFC_RETURN(spList->ReplaceAll(items));
    *ppList = spList.Detach();
    return S_OK;
}

IFACEMETHODIMP NodeListVector::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** ppView)
{
    ARG_VALIDRETURNPOINTER(ppView);
    ctl::ComPtr<InternalList> spList;
    IFC_RETURN(Snapshot(spList.ReleaseAndGetAddressOf()));
    IFC_RETURN(spList->GetView(ppView));
    return S_OK;
}

// Only a node object of this list's nodes is found; it is located through the store, so no other node object is made.
IFACEMETHODIMP NodeListVector::IndexOf(_In_opt_ IInspectable* pValue, _Out_ UINT* pIndex, _Out_ BOOLEAN* pFound)
{
    ARG_VALIDRETURNPOINTER(pIndex);
    ARG_VALIDRETURNPOINTER(pFound);
    *pIndex = 0;
    *pFound = FALSE;
    IFC_RETURN(CheckThread());
    auto spStore = m_store.lock();
    auto spNode = ctl::query_interface_cast<CollectionViewNode>(pValue);
    UINT position = 0;
    if (spStore && spNode && spStore->TryGetPosition(m_owner, spNode.Get(), &position))
    {
        *pIndex = position;
        *pFound = TRUE;
    }
    return S_OK;
}
IFACEMETHODIMP NodeListVector::SetAt(UINT, _In_opt_ IInspectable*)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::InsertAt(UINT, _In_opt_ IInspectable*)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::RemoveAt(UINT)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::Append(_In_opt_ IInspectable*)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::RemoveAtEnd()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::Clear()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP NodeListVector::First(_Outptr_ wfc::IIterator<IInspectable*>** ppIterator)
{
    ARG_VALIDRETURNPOINTER(ppIterator);
    ctl::ComPtr<InternalList> spList;
    IFC_RETURN(Snapshot(spList.ReleaseAndGetAddressOf()));
    IFC_RETURN(spList->First(ppIterator));
    return S_OK;
}

// ---- HierarchyStore ----

namespace
{
    // T answers its own IID with itself (CollectionViewNode, NodeListVector).
    template <typename T>
    ctl::ComPtr<T> ResolveWeak(ctl::WeakRefPtr& ref)
    {
        ctl::ComPtr<T> result;
        if (ref.Get())
        {
            ctl::ComPtr<IInspectable> spResolved;
            IGNOREHR(ref.AsIID(__uuidof(T), &spResolved));
            if (spResolved)
            {
                result.Attach(reinterpret_cast<T*>(spResolved.Detach()));
            }
        }
        return result;
    }
}

HierarchyStore::~HierarchyStore()
{
    DetachProjections(false /* isRaised */);
}

UINT HierarchyStore::KeptCount(NodeId owner) const
{
    if (owner == c_noNode)
    {
        return static_cast<UINT>(roots.size());
    }
    return (owner < nodes.size() && nodes[owner].isKept) ? static_cast<UINT>(nodes[owner].kept.size()) : 0;
}

NodeId HierarchyStore::KeptAt(NodeId owner, UINT index) const
{
    return (owner == c_noNode) ? roots[index] : nodes[owner].kept[index];
}

void HierarchyStore::ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
{
    m_items.ReferenceTrackerWalk(walkType);
    m_keyObjects.ReferenceTrackerWalk(walkType);
    if (sort)
    {
        sort->references.ReferenceTrackerWalk(walkType);
    }
}

void HierarchyStore::Reserve(UINT count)
{
    nodes.reserve(count);
    sourceIndexOf.reserve(count);
    bySource.reserve(count);
    m_items.Reserve(count);
    owners.reserve(count);
}

UINT HierarchyStore::AllocateKeyObject(_In_opt_ IInspectable* pObject)
{
    if (!m_freeKeyObjects.empty())
    {
        const UINT slot = m_freeKeyObjects.back();
        m_freeKeyObjects.pop_back();
        VERIFYHR(m_keyObjects.SetAt(slot, pObject));
        return slot;
    }
    m_keyObjects.Append(pObject);
    return m_keyObjects.Size() - 1;
}

void HierarchyStore::FreeKeyObject(UINT slot)
{
    if (slot != c_noTrackerSlot)
    {
        VERIFYHR(m_keyObjects.SetAt(slot, nullptr));
        m_freeKeyObjects.push_back(slot);
    }
}

_Check_return_ HRESULT HierarchyStore::AllocateNode(
    _In_opt_ IInspectable* pItem,
    UINT sourceIndex,
    NodeKey&& key,
    _In_opt_ IInspectable* pKeyObject,
    NodeKey&& parentKey,
    _In_opt_ IInspectable* pParentKeyObject,
    bool isMatch,
    SortKeys&& keys,
    _Out_ NodeId* pId)
{
    NodeId id = c_noNode;
    if (!freeNodes.empty())
    {
        id = freeNodes.back();
        freeNodes.pop_back();
        nodes[id] = HierarchyNode();
        sourceIndexOf[id] = sourceIndex;
        IFC_RETURN(m_items.SetAt(id, pItem));
    }
    else
    {
        id = static_cast<NodeId>(nodes.size());
        nodes.emplace_back();
        sourceIndexOf.push_back(sourceIndex);
        m_items.Append(pItem);
    }

    auto& node = nodes[id];
    node.isAllocated = true;
    node.isMatch = isMatch;
    node.key = std::move(key);
    node.parentKey = std::move(parentKey);
    if (node.key.isObject)
    {
        node.keyObjectSlot = AllocateKeyObject(pKeyObject);
    }
    if (node.parentKey.isObject)
    {
        node.parentKeyObjectSlot = AllocateKeyObject(pParentKeyObject);
    }
    if (sort && sort->keys.FieldCount() > 0)
    {
        node.slot = sort->keys.Allocate(std::move(keys));
    }
    if (m_isIdentityIndexed)
    {
        IndexIdentity(id, pItem, true /* isAdded */);
    }
    *pId = id;
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::FreeNode(NodeId id)
{
    auto& node = nodes[id];
    ReleaseProjection(id);
    if (m_isIdentityIndexed)
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC_RETURN(m_items.GetAt(id, spItem.ReleaseAndGetAddressOf()));
        IndexIdentity(id, spItem.Get(), false /* isAdded */);
    }
    FreeKeyObject(node.keyObjectSlot);
    FreeKeyObject(node.parentKeyObjectSlot);
    if (sort && node.slot != SortKeyStore::c_noSlot)
    {
        sort->keys.Free(node.slot);
    }
    node = HierarchyNode();
    freeNodes.push_back(id);
    IFC_RETURN(m_items.SetAt(id, nullptr));
    return S_OK;
}

void HierarchyStore::IndexIdentity(NodeId id, _In_opt_ IInspectable* pItem, bool isAdded)
{
    ctl::ComPtr<IUnknown> spIdentity;
    if (pItem)
    {
        IGNOREHR(ctl::do_query_interface(spIdentity, pItem));
    }
    if (isAdded)
    {
        m_byIdentity.emplace(spIdentity.Get(), id);
        return;
    }
    auto range = m_byIdentity.equal_range(spIdentity.Get());
    for (auto entry = range.first; entry != range.second; ++entry)
    {
        if (entry->second == id)
        {
            m_byIdentity.erase(entry);
            return;
        }
    }
}

ctl::ComPtr<CollectionViewNode> HierarchyStore::LiveProjection(NodeId id)
{
    const UINT slot = nodes[id].projectionSlot;
    return (slot == c_noTrackerSlot) ? nullptr : ResolveWeak<CollectionViewNode>(m_projections[slot].node);
}

ctl::ComPtr<NodeListVector> HierarchyStore::LiveChildren(NodeId id)
{
    const UINT slot = nodes[id].projectionSlot;
    return (slot == c_noTrackerSlot) ? nullptr : ResolveWeak<NodeListVector>(m_projections[slot].children);
}

UINT HierarchyStore::EnsureProjectionSlot(NodeId id)
{
    auto& node = nodes[id];
    if (node.projectionSlot == c_noTrackerSlot)
    {
        if (!m_freeProjections.empty())
        {
            node.projectionSlot = m_freeProjections.back();
            m_freeProjections.pop_back();
        }
        else
        {
            node.projectionSlot = static_cast<UINT>(m_projections.size());
            m_projections.emplace_back();
        }
    }
    return node.projectionSlot;
}

_Check_return_ HRESULT HierarchyStore::RegisterChildren(NodeId id, _In_ NodeListVector* pChildren)
{
    const UINT slot = EnsureProjectionSlot(id);
    IFC_RETURN(ctl::AsWeak(ctl::as_iinspectable(pChildren), &m_projections[slot].children));
    return S_OK;
}

void HierarchyStore::ReleaseProjection(NodeId id)
{
    auto& node = nodes[id];
    if (node.projectionSlot == c_noTrackerSlot)
    {
        return;
    }

    const UINT slot = std::exchange(node.projectionSlot, c_noTrackerSlot);
    auto refs = std::exchange(m_projections[slot], ProjectionRefs());
    m_freeProjections.push_back(slot);
    if (auto spChildren = ResolveWeak<NodeListVector>(refs.children))
    {
        IGNOREHR(spChildren->Detach(true /* isRaised */));
    }
    if (auto spProjection = ResolveWeak<CollectionViewNode>(refs.node))
    {
        spProjection->Detach(true /* isRaised */);
    }
}

void HierarchyStore::DetachProjections(bool isRaised)
{
    // By index, and each slot cleared before its objects are told: a handler may read the old nodes.
    for (NodeId id = 0; id < nodes.size(); ++id)
    {
        const UINT slot = std::exchange(nodes[id].projectionSlot, c_noTrackerSlot);
        if (slot == c_noTrackerSlot)
        {
            continue;
        }
        auto refs = std::exchange(m_projections[slot], ProjectionRefs());
        m_freeProjections.push_back(slot);
        if (auto spChildren = ResolveWeak<NodeListVector>(refs.children))
        {
            IGNOREHR(spChildren->Detach(isRaised));
        }
        if (auto spProjection = ResolveWeak<CollectionViewNode>(refs.node))
        {
            spProjection->Detach(isRaised);
        }
    }
}

void HierarchyStore::BeginFlatBatch()
{
    if (m_flatBatchDepth++ == 0)
    {
        m_flatBatchRows = 0;
        m_isFlatBatchDirty = false;
    }
}

_Check_return_ HRESULT HierarchyStore::EndFlatBatch(const HierarchyOutput& output, bool isPublished)
{
    ASSERT(m_flatBatchDepth > 0);
    if (m_flatBatchDepth == 0 || --m_flatBatchDepth > 0)
    {
        return S_OK;
    }
    m_flatBatchRows = 0;
    if (std::exchange(m_isFlatBatchDirty, false) && isPublished && output.pFlat)
    {
        IFC_RETURN(RepublishFlat(output));
    }
    return S_OK;
}

bool HierarchyStore::IsFlatPublished(UINT count)
{
    if (m_flatBatchDepth == 0)
    {
        return true;
    }
    m_flatBatchRows += count;
    m_isFlatBatchDirty = m_isFlatBatchDirty || m_flatBatchRows > c_maxFlatEvents;
    return !m_isFlatBatchDirty;
}
_Check_return_ HRESULT HierarchyStore::CompareNodes(NodeId left, NodeId right, _Out_ int* pResult)
{
    ctl::ComPtr<IInspectable> spLeft;
    ctl::ComPtr<IInspectable> spRight;
    if (sort->comparer.HasCustomSort())
    {
        IFC_RETURN(m_items.GetAt(left, spLeft.ReleaseAndGetAddressOf()));
        IFC_RETURN(m_items.GetAt(right, spRight.ReleaseAndGetAddressOf()));
    }
    IFC_RETURN(sort->comparer.Compare(
        spLeft.Get(), sort->keys.At(nodes[left].slot), sourceIndexOf[left],
        spRight.Get(), sort->keys.At(nodes[right].slot), sourceIndexOf[right], pResult));
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::CompareWithKeys(NodeId id, const SortKeys& keys, NodeId other, _Out_ int* pResult)
{
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<IInspectable> spOther;
    if (sort->comparer.HasCustomSort())
    {
        IFC_RETURN(m_items.GetAt(id, spItem.ReleaseAndGetAddressOf()));
        IFC_RETURN(m_items.GetAt(other, spOther.ReleaseAndGetAddressOf()));
    }
    IFC_RETURN(sort->comparer.Compare(
        spItem.Get(), keys.data(), sourceIndexOf[id],
        spOther.Get(), sort->keys.At(nodes[other].slot), sourceIndexOf[other], pResult));
    return S_OK;
}

// Ties fall back to the source index, so a list is in source order when nothing sorts it.
_Check_return_ HRESULT HierarchyStore::SortKept(std::vector<NodeId>& list)
{
    if (list.size() < 2)
    {
        return S_OK;
    }

    if (!sort || sort->comparer.IsEmpty())
    {
        std::sort(list.begin(), list.end(), [this](NodeId left, NodeId right) { return sourceIndexOf[left] < sourceIndexOf[right]; });
        return S_OK;
    }

    if (sort->comparer.HasCustomSort())
    {
        IFC_RETURN(SafeStableSort(list, [this](NodeId left, NodeId right, bool* pIsBefore) -> HRESULT
        {
            int result = 0;
            IFC_RETURN(CompareNodes(left, right, &result));
            *pIsBefore = result < 0;
            return S_OK;
        }));
        return S_OK;
    }

    std::sort(list.begin(), list.end(), [this](NodeId left, NodeId right)
    {
        int result = 0;
        VERIFYHR(CompareNodes(left, right, &result));
        return result < 0;
    });
    return S_OK;
}

// Full build, after AllocateNode for every source item in source order. Duplicate keys and cycles degrade, as
// the design note describes; nothing here throws for the app's data.
_Check_return_ HRESULT HierarchyStore::BuildTree(_Out_ HierarchyDiagnostics* pDiagnostics)
{
    *pDiagnostics = HierarchyDiagnostics();
    const NodeId count = static_cast<NodeId>(nodes.size());

    for (NodeId id = 0; id < count; ++id)
    {
        auto& node = nodes[id];
        if (node.key.IsEmpty())
        {
            continue;
        }
        auto inserted = owners.emplace(node.key, id);
        if (inserted.second)
        {
            node.isOwner = true;
        }
        else
        {
            node.isDuplicate = true;
            duplicates[node.key].push_back(id);
            ++pDiagnostics->duplicateCount;
        }
    }

    for (NodeId id = 0; id < count; ++id)
    {
        auto& node = nodes[id];
        if (node.isDuplicate || node.parentKey.IsEmpty())
        {
            continue;
        }
        auto found = owners.find(node.parentKey);
        if (found != owners.end())
        {
            node.parent = found->second;
            nodes[found->second].children.push_back(id);
        }
        else
        {
            node.isWaiting = true;
            waiting[node.parentKey].push_back(id);
        }
    }

    // Cycles: walk up from each node; a walk that meets a node of its own path closed a cycle, whose member that
    // comes first in the source becomes a root.
    {
        enum : UINT8 { Unvisited, OnPath, Done };
        std::vector<UINT8> state(count, Unvisited);
        std::vector<NodeId> path;
        for (NodeId start = 0; start < count; ++start)
        {
            path.clear();
            NodeId id = start;
            while (id != c_noNode && state[id] == Unvisited)
            {
                state[id] = OnPath;
                path.push_back(id);
                id = nodes[id].parent;
            }

            if (id != c_noNode && state[id] == OnPath)
            {
                auto first = std::find(path.begin(), path.end(), id);
                NodeId broken = *first;
                for (auto member = first; member != path.end(); ++member)
                {
                    if (sourceIndexOf[*member] < sourceIndexOf[broken])
                    {
                        broken = *member;
                    }
                }
                auto& siblings = nodes[nodes[broken].parent].children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), broken), siblings.end());
                nodes[broken].parent = c_noNode;
                nodes[broken].isCycleBroken = true;
                ++cycleBrokenCount;
                ++pDiagnostics->cycleCount;
            }

            for (NodeId member : path)
            {
                state[member] = Done;
            }
        }
    }

    // Filter counts, children before parents.
    std::vector<NodeId> postOrder;
    postOrder.reserve(count);
    {
        std::vector<std::pair<NodeId, UINT>> stack;
        for (NodeId root = 0; root < count; ++root)
        {
            if (nodes[root].parent != c_noNode)
            {
                continue;
            }
            stack.emplace_back(root, 0);
            while (!stack.empty())
            {
                auto& top = stack.back();
                const auto& children = nodes[top.first].children;
                if (top.second < children.size())
                {
                    const NodeId child = children[top.second++];
                    stack.emplace_back(child, 0);
                }
                else
                {
                    postOrder.push_back(top.first);
                    stack.pop_back();
                }
            }
        }
    }

    for (NodeId id : postOrder)
    {
        auto& node = nodes[id];
        node.isKept = node.IsKeptByRule();
        if (node.parent != c_noNode)
        {
            nodes[node.parent].matchingBelow += node.matchingBelow + (node.isMatch ? 1 : 0);
        }
    }

    for (NodeId id : postOrder)
    {
        auto& node = nodes[id];
        node.kept.clear();
        if (!node.isKept)
        {
            continue;
        }
        for (NodeId child : node.children)
        {
            if (nodes[child].isKept)
            {
                node.kept.push_back(child);
            }
        }
        IFC_RETURN(SortKept(node.kept));
        node.keptSubtree = 1;
        for (NodeId child : node.kept)
        {
            node.keptSubtree += nodes[child].keptSubtree;
        }
    }

    roots.clear();
    for (NodeId id = 0; id < count; ++id)
    {
        if (nodes[id].parent == c_noNode && nodes[id].isKept)
        {
            roots.push_back(id);
        }
    }
    IFC_RETURN(SortKept(roots));
    isDuplicateLogged = pDiagnostics->duplicateCount > 0;
    return S_OK;
}

void HierarchyStore::CollectPreOrder(std::vector<NodeId>& order) const
{
    order.clear();
    std::vector<std::pair<NodeId, UINT>> stack;
    for (NodeId root : roots)
    {
        order.push_back(root);
        stack.emplace_back(root, 0);
        while (!stack.empty())
        {
            auto& top = stack.back();
            const auto& kept = nodes[top.first].kept;
            if (top.second < kept.size())
            {
                const NodeId child = kept[top.second++];
                order.push_back(child);
                stack.emplace_back(child, 0);
            }
            else
            {
                stack.pop_back();
            }
        }
    }
}

_Check_return_ HRESULT HierarchyStore::FlatItems(const std::vector<NodeId>& order, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pItems)
{
    pItems->clear();
    pItems->reserve(order.size());
    for (NodeId id : order)
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC_RETURN(m_items.GetAt(id, spItem.ReleaseAndGetAddressOf()));
        pItems->push_back(std::move(spItem));
    }
    return S_OK;
}

UINT HierarchyStore::PositionOf(NodeId id)
{
    const auto& list = KeptList(nodes[id].parent);
    return static_cast<UINT>(std::find(list.begin(), list.end(), id) - list.begin());
}

// The parent's index, plus one, plus the kept subtrees of the siblings before it, up to the roots.
UINT HierarchyStore::FlatIndexOf(NodeId id)
{
    UINT index = 0;
    for (NodeId current = id; current != c_noNode; current = nodes[current].parent)
    {
        const NodeId parent = nodes[current].parent;
        for (NodeId sibling : KeptList(parent))
        {
            if (sibling == current)
            {
                break;
            }
            index += nodes[sibling].keptSubtree;
        }
        if (parent != c_noNode)
        {
            ++index;
        }
    }
    return index;
}

void HierarchyStore::ShiftSourceIndices(UINT index, int delta)
{
    // c_noNode: a slot whose node a failed change already removed (the store is stale then).
    if (delta > 0)
    {
        bySource.insert(bySource.begin() + index, c_noNode);
        for (UINT sourceIndex = index + 1; sourceIndex < bySource.size(); ++sourceIndex)
        {
            if (bySource[sourceIndex] != c_noNode)
            {
                sourceIndexOf[bySource[sourceIndex]] = sourceIndex;
            }
        }
    }
    else
    {
        bySource.erase(bySource.begin() + index);
        for (UINT sourceIndex = index; sourceIndex < bySource.size(); ++sourceIndex)
        {
            if (bySource[sourceIndex] != c_noNode)
            {
                sourceIndexOf[bySource[sourceIndex]] = sourceIndex;
            }
        }
    }
}

bool HierarchyStore::IsInsertComplex(UINT sourceIndex, const NodeKey& key, const NodeKey& parentKey, NodeId replacing) const
{
    if (!key.IsEmpty())
    {
        if (key == parentKey)
        {
            return true;
        }
        auto owner = owners.find(key);
        if (owner != owners.end() && owner->second != replacing)
        {
            // The new item comes first and would take the key from the item that has it.
            return sourceIndexOf[owner->second] >= sourceIndex;
        }
    }

    if (key.IsEmpty() || parentKey.IsEmpty())
    {
        return false;
    }

    // The new node would adopt the root that waits for its key; if that root is above its own parent, a cycle. A
    // replaced node's children wait for its key once it is gone.
    auto parent = owners.find(parentKey);
    if (parent == owners.end() || parent->second == replacing)
    {
        return false;
    }
    NodeId top = parent->second;
    for (NodeId up = nodes[top].parent; up != c_noNode; up = nodes[top].parent)
    {
        if (up == replacing)
        {
            return nodes[replacing].key == key;
        }
        top = up;
    }
    return nodes[top].isWaiting && nodes[top].parentKey == key;
}

bool HierarchyStore::IsRemoveComplex(NodeId id) const
{
    const auto& node = nodes[id];
    return cycleBrokenCount > 0 || (node.isOwner && duplicates.find(node.key) != duplicates.end());
}

ctl::ComPtr<NodeListVector> HierarchyStore::NodeVector(NodeId parent, const HierarchyOutput& output)
{
    if (parent == c_noNode)
    {
        return output.pRoots;
    }
    return LiveChildren(parent);
}

_Check_return_ HRESULT HierarchyStore::FindInsertPosition(NodeId id, const std::vector<NodeId>& list, _Out_ UINT* pPosition)
{
    *pPosition = static_cast<UINT>(list.size());
    if (!sort || sort->comparer.IsEmpty())
    {
        const UINT sourceIndex = sourceIndexOf[id];
        *pPosition = static_cast<UINT>(std::lower_bound(list.begin(), list.end(), sourceIndex,
            [this](NodeId other, UINT value) { return sourceIndexOf[other] < value; }) - list.begin());
        return S_OK;
    }

    SortKeys keys;
    if (nodes[id].slot != SortKeyStore::c_noSlot)
    {
        const SortKey* pKeys = sort->keys.At(nodes[id].slot);
        keys.assign(pKeys, pKeys + sort->keys.FieldCount());
    }
    IFC_RETURN(FindSortedRange(
        static_cast<UINT>(list.size()),
        UINT_MAX,
        [&](UINT position, int* pResult) { return CompareWithKeys(id, keys, list[position], pResult); },
        [&](UINT position) { return sourceIndexOf[list[position]]; },
        [this](UINT sourceIndex) { return isSortSkipped && isSortSkipped(sourceIndex); },
        pPosition));
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::GetProjection(NodeId id, _Outptr_ xaml_data::ICollectionViewNode** ppNode)
{
    auto spProjection = LiveProjection(id);
    if (!spProjection)
    {
        const auto& node = nodes[id];
        ctl::ComPtr<IInspectable> spItem;
        ctl::ComPtr<IInspectable> spKey;
        IFC_RETURN(m_items.GetAt(id, spItem.ReleaseAndGetAddressOf()));
        if (node.isDuplicate)
        {
            // Only the owner reports the key, so a control that keys rows by Key never takes a duplicate for the
            // owner. A duplicate becomes the owner only through a recompute, which makes new nodes.
        }
        else if (node.key.isObject)
        {
            IFC_RETURN(m_keyObjects.GetAt(node.keyObjectSlot, spKey.ReleaseAndGetAddressOf()));
        }
        else
        {
            IFC_RETURN(BoxNodeKey(node.key, spKey.ReleaseAndGetAddressOf()));
        }
        IFC_RETURN(ctl::ComObject<CollectionViewNode>::CreateInstance(spProjection.ReleaseAndGetAddressOf()));
        IFC_RETURN(spProjection->Initialize(shared_from_this(), id, spItem.Get(), spKey.Get(), !nodes[id].isMatch));
        if (auto spChildren = LiveChildren(id))
        {
            spProjection->SetChildren(spChildren.Get());
        }
        const UINT slot = EnsureProjectionSlot(id);
        IFC_RETURN(ctl::AsWeak(ctl::as_iinspectable(spProjection.Get()), &m_projections[slot].node));
    }

    IFC_RETURN(ctl::do_query_interface(*ppNode, ctl::as_iinspectable(spProjection.Get())));
    return S_OK;
}

bool HierarchyStore::TryGetPosition(NodeId owner, _In_ CollectionViewNode* pNode, _Out_ UINT* pPosition)
{
    *pPosition = 0;
    if (!pNode->IsBoundTo(this))
    {
        return false;
    }
    const NodeId id = pNode->Id();
    if (id >= nodes.size() || !nodes[id].isKept || nodes[id].parent != owner || LiveProjection(id).Get() != pNode)
    {
        return false;
    }
    *pPosition = PositionOf(id);
    return *pPosition < KeptCount(owner);
}
_Check_return_ HRESULT HierarchyStore::GetParentProjection(NodeId id, _Outptr_result_maybenull_ xaml_data::ICollectionViewNode** ppNode)
{
    *ppNode = nullptr;
    const NodeId parent = nodes[id].parent;
    if (nodes[id].isKept && parent != c_noNode && nodes[parent].isKept)
    {
        IFC_RETURN(GetProjection(parent, ppNode));
    }
    return S_OK;
}

// By COM identity, through an index built on the first call. An item the source holds twice: its kept node that comes
// first in the source.
_Check_return_ HRESULT HierarchyStore::FindKeptNode(_In_opt_ IInspectable* pItem, _Out_ NodeId* pId)
{
    *pId = c_noNode;
    if (!m_isIdentityIndexed)
    {
        m_byIdentity.reserve(nodes.size());
        for (NodeId id = 0; id < nodes.size(); ++id)
        {
            if (nodes[id].isAllocated)
            {
                ctl::ComPtr<IInspectable> spItem;
                IFC_RETURN(m_items.GetAt(id, spItem.ReleaseAndGetAddressOf()));
                IndexIdentity(id, spItem.Get(), true /* isAdded */);
            }
        }
        m_isIdentityIndexed = true;
    }

    ctl::ComPtr<IUnknown> spIdentity;
    if (pItem)
    {
        IFC_RETURN(ctl::do_query_interface(spIdentity, pItem));
    }
    auto range = m_byIdentity.equal_range(spIdentity.Get());
    for (auto entry = range.first; entry != range.second; ++entry)
    {
        const NodeId id = entry->second;
        if (nodes[id].isKept && (*pId == c_noNode || sourceIndexOf[id] < sourceIndexOf[*pId]))
        {
            *pId = id;
        }
    }
    return S_OK;
}
_Check_return_ HRESULT HierarchyStore::RepublishFlat(const HierarchyOutput& output)
{
    std::vector<NodeId> order;
    std::vector<ctl::ComPtr<IInspectable>> items;
    CollectPreOrder(order);
    IFC_RETURN(FlatItems(order, &items));
    IFC_RETURN(output.pFlat->ReplaceAll(items));
    return S_OK;
}

// Puts a kept node (with its kept subtree already complete) into its parent's kept list, at its sorted place, and
// publishes it: the parent's node vector, then the flat list.
_Check_return_ HRESULT HierarchyStore::Attach(NodeId id, const HierarchyOutput& output)
{
    const NodeId parent = nodes[id].parent;
    auto& list = KeptList(parent);
    UINT position = 0;
    IFC_RETURN(FindInsertPosition(id, list, &position));
    list.insert(list.begin() + position, id);
    const bool isFirstChild = (parent != c_noNode) && list.size() == 1;
    const UINT count = nodes[id].keptSubtree;
    for (NodeId ancestor = parent; ancestor != c_noNode; ancestor = nodes[ancestor].parent)
    {
        nodes[ancestor].keptSubtree += count;
    }

    if (auto spVector = NodeVector(parent, output))
    {
        IFC_RETURN(spVector->RaiseChanged(wfc::CollectionChange_ItemInserted, position));
    }
    if (isFirstChild)
    {
        IFC_RETURN(NotifyHasChildrenChanged(parent));
    }

    if (!output.pFlat || !IsFlatPublished(count))
    {
        return S_OK;
    }

    if (count > c_maxFlatEvents)
    {
        IFC_RETURN(RepublishFlat(output));
        return S_OK;
    }

    const UINT base = FlatIndexOf(id);
    std::vector<NodeId> subtree;
    subtree.reserve(count);
    {
        std::vector<std::pair<NodeId, UINT>> stack;
        subtree.push_back(id);
        stack.emplace_back(id, 0);
        while (!stack.empty())
        {
            auto& top = stack.back();
            const auto& kept = nodes[top.first].kept;
            if (top.second < kept.size())
            {
                const NodeId child = kept[top.second++];
                subtree.push_back(child);
                stack.emplace_back(child, 0);
            }
            else
            {
                stack.pop_back();
            }
        }
    }

    for (UINT i = 0; i < subtree.size(); ++i)
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC_RETURN(m_items.GetAt(subtree[i], spItem.ReleaseAndGetAddressOf()));
        IFC_RETURN(output.pFlat->InternalInsertAt(base + i, spItem.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::Detach(NodeId id, const HierarchyOutput& output)
{
    const NodeId parent = nodes[id].parent;
    auto& list = KeptList(parent);
    const UINT position = static_cast<UINT>(std::find(list.begin(), list.end(), id) - list.begin());
    if (position >= list.size())
    {
        ASSERT(false);
        return E_UNEXPECTED;
    }

    const UINT count = nodes[id].keptSubtree;
    const bool isPublished = output.pFlat && IsFlatPublished(count);
    const UINT base = isPublished ? FlatIndexOf(id) : 0;
    list.erase(list.begin() + position);
    const bool isLastChild = (parent != c_noNode) && list.empty();
    for (NodeId ancestor = parent; ancestor != c_noNode; ancestor = nodes[ancestor].parent)
    {
        nodes[ancestor].keptSubtree -= count;
    }

    if (auto spVector = NodeVector(parent, output))
    {
        IFC_RETURN(spVector->RaiseChanged(wfc::CollectionChange_ItemRemoved, position));
    }
    if (isLastChild)
    {
        IFC_RETURN(NotifyHasChildrenChanged(parent));
    }

    if (!isPublished)
    {
        return S_OK;
    }

    if (count > c_maxFlatEvents)
    {
        IFC_RETURN(RepublishFlat(output));
        return S_OK;
    }

    for (UINT i = 0; i < count; ++i)
    {
        IFC_RETURN(output.pFlat->InternalRemoveAt(base));
    }
    return S_OK;
}
void HierarchyStore::AddMatches(NodeId from, int delta)
{
    for (NodeId ancestor = from; ancestor != c_noNode && delta != 0; ancestor = nodes[ancestor].parent)
    {
        nodes[ancestor].matchingBelow = static_cast<UINT>(static_cast<int>(nodes[ancestor].matchingBelow) + delta);
    }
}

// child became kept: the ancestors that were not kept become kept as context, each with that one kept child, and the
// topmost of them is attached with the chain below it.
_Check_return_ HRESULT HierarchyStore::PromoteAncestors(NodeId child, const HierarchyOutput& output)
{
    NodeId top = child;
    for (NodeId ancestor = nodes[child].parent; ancestor != c_noNode && !nodes[ancestor].isKept; ancestor = nodes[ancestor].parent)
    {
        ASSERT(nodes[ancestor].IsKeptByRule());
        auto& node = nodes[ancestor];
        node.isKept = true;
        node.kept.assign(1, top);
        node.keptSubtree = 1 + nodes[top].keptSubtree;
        top = ancestor;
    }
    IFC_RETURN(Attach(top, output));
    return S_OK;
}

// The ancestors from 'from' up that no longer have a matching descendant (and do not match) stop being kept; their
// kept lists hold only that chain.
_Check_return_ HRESULT HierarchyStore::DemoteAncestors(NodeId from, const HierarchyOutput& output)
{
    NodeId top = c_noNode;
    for (NodeId ancestor = from; ancestor != c_noNode && nodes[ancestor].isKept && !nodes[ancestor].IsKeptByRule(); ancestor = nodes[ancestor].parent)
    {
        top = ancestor;
    }
    if (top == c_noNode)
    {
        return S_OK;
    }

    IFC_RETURN(Detach(top, output));
    for (NodeId ancestor = from; ; ancestor = nodes[ancestor].parent)
    {
        auto& node = nodes[ancestor];
        node.isKept = false;
        node.kept.clear();
        node.keptSubtree = 0;
        ReleaseProjection(ancestor);
        if (ancestor == top)
        {
            break;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::SetIsContext(NodeId id)
{
    if (auto spProjection = LiveProjection(id))
    {
        IFC_RETURN(spProjection->SetIsContext(!nodes[id].isMatch));
    }
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::NotifyHasChildrenChanged(NodeId owner)
{
    if (auto spProjection = LiveProjection(owner))
    {
        IFC_RETURN(spProjection->RaisePropertyChanged(L"HasChildren"));
    }
    return S_OK;
}

// A node allocated (AllocateNode) for a source item already in bySource: links it by its keys, adopts the roots that
// wait for its key, and publishes what becomes visible.
_Check_return_ HRESULT HierarchyStore::InsertNode(NodeId id, const HierarchyOutput& output)
{
    NodeId parent = c_noNode;
    {
        auto& node = nodes[id];
        if (!node.key.IsEmpty())
        {
            auto inserted = owners.emplace(node.key, id);
            if (inserted.second)
            {
                node.isOwner = true;
            }
            else
            {
                node.isDuplicate = true;
                duplicates[node.key].push_back(id);
                if (!isDuplicateLogged)
                {
                    isDuplicateLogged = true;
                    LogHierarchyDataError(1, 0);
                }
            }
        }

        if (!node.isDuplicate && !node.parentKey.IsEmpty())
        {
            auto found = owners.find(node.parentKey);
            if (found != owners.end() && found->second != id)
            {
                parent = found->second;
            }
            else
            {
                node.isWaiting = true;
                waiting[node.parentKey].push_back(id);
            }
        }
    }

    // The roots that waited for this key move under the new node: unpublished first, published with it.
    std::vector<NodeId> adopted;
    if (nodes[id].isOwner)
    {
        auto found = waiting.find(nodes[id].key);
        if (found != waiting.end())
        {
            adopted = std::move(found->second);
            waiting.erase(found);
        }
    }

    for (NodeId orphan : adopted)
    {
        if (nodes[orphan].isKept)
        {
            IFC_RETURN(Detach(orphan, output));
        }
    }

    {
        auto& node = nodes[id];
        node.parent = parent;
        for (NodeId orphan : adopted)
        {
            auto& child = nodes[orphan];
            child.isWaiting = false;
            child.parent = id;
            node.children.push_back(orphan);
            node.matchingBelow += child.matchingBelow + (child.isMatch ? 1 : 0);
            if (child.isKept)
            {
                node.kept.push_back(orphan);
            }
        }
    }
    IFC_RETURN(SortKept(nodes[id].kept));

    auto& node = nodes[id];
    if (parent != c_noNode)
    {
        nodes[parent].children.push_back(id);
    }
    node.isKept = node.IsKeptByRule();
    if (node.isKept)
    {
        node.keptSubtree = 1;
        for (NodeId child : node.kept)
        {
            node.keptSubtree += nodes[child].keptSubtree;
        }
        AddMatches(parent, static_cast<int>(node.matchingBelow + (node.isMatch ? 1 : 0)));
        IFC_RETURN(PromoteAncestors(id, output));
    }
    IFC_RETURN(NotifyParentChanged(adopted));
    return S_OK;
}

// Unpublishes the node, re-roots its children (they wait for its key again) and frees it. The caller has removed it
// from bySource.
_Check_return_ HRESULT HierarchyStore::RemoveNode(NodeId id, const HierarchyOutput& output)
{
    const NodeId parent = nodes[id].parent;
    if (nodes[id].isKept)
    {
        IFC_RETURN(Detach(id, output));
    }

    const int matches = static_cast<int>(nodes[id].matchingBelow + (nodes[id].isMatch ? 1 : 0));
    if (matches > 0)
    {
        AddMatches(parent, -matches);
        IFC_RETURN(DemoteAncestors(parent, output));
    }

    if (parent != c_noNode)
    {
        auto& siblings = nodes[parent].children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());
    }

    if (nodes[id].isWaiting)
    {
        auto found = waiting.find(nodes[id].parentKey);
        if (found != waiting.end())
        {
            auto& list = found->second;
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            if (list.empty())
            {
                waiting.erase(found);
            }
        }
    }

    if (nodes[id].isDuplicate)
    {
        auto found = duplicates.find(nodes[id].key);
        if (found != duplicates.end())
        {
            auto& list = found->second;
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            if (list.empty())
            {
                duplicates.erase(found);
            }
        }
    }

    const auto children = std::move(nodes[id].children);
    if (nodes[id].isOwner)
    {
        owners.erase(nodes[id].key);
    }

    for (NodeId child : children)
    {
        auto& orphan = nodes[child];
        orphan.parent = c_noNode;
        orphan.isWaiting = true;
        waiting[orphan.parentKey].push_back(child);
    }

    for (NodeId child : children)
    {
        if (nodes[child].isKept)
        {
            IFC_RETURN(Attach(child, output));
        }
    }

    IFC_RETURN(FreeNode(id));
    IFC_RETURN(NotifyParentChanged(children));
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::NotifyParentChanged(const std::vector<NodeId>& ids)
{
    for (NodeId id : ids)
    {
        if (auto spProjection = LiveProjection(id))
        {
            IFC_RETURN(spProjection->RaisePropertyChanged(L"Parent"));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT HierarchyStore::UpdateNode(NodeId id, _In_opt_ IInspectable* pItem, bool isMatch, _In_opt_ const SortKeys* pKeys, const HierarchyOutput& output, bool isReplace)
{
    const NodeId parent = nodes[id].parent;
    const bool wasKept = nodes[id].isKept;

    ctl::ComPtr<IInspectable> spOldItem;
    IFC_RETURN(m_items.GetAt(id, spOldItem.ReleaseAndGetAddressOf()));
    if (spOldItem.Get() != pItem)
    {
        if (m_isIdentityIndexed)
        {
            IndexIdentity(id, spOldItem.Get(), false /* isAdded */);
        }
        IFC_RETURN(m_items.SetAt(id, pItem));
        if (m_isIdentityIndexed)
        {
            IndexIdentity(id, pItem, true /* isAdded */);
        }
        // The node first, so a handler of the flat change finds the item on it.
        if (auto spProjection = LiveProjection(id))
        {
            IFC_RETURN(spProjection->SetItem(pItem));
        }
        if (wasKept && output.pFlat)
        {
            IFC_RETURN(output.pFlat->InternalSetAt(FlatIndexOf(id), pItem));
        }
    }

    if (nodes[id].isMatch != isMatch)
    {
        nodes[id].isMatch = isMatch;
        AddMatches(parent, isMatch ? 1 : -1);
    }
    const bool isKept = nodes[id].IsKeptByRule();

    bool isMoving = false;
    if (wasKept && isKept && pKeys && sort && !sort->comparer.IsEmpty())
    {
        auto& list = KeptList(parent);
        const UINT position = PositionOf(id);
        UINT newPosition = 0;
        UINT first = 0;
        UINT last = 0;
        IFC_RETURN(FindSortedRange(
            static_cast<UINT>(list.size()),
            position,
            [&](UINT other, int* pResult) { return CompareWithKeys(id, *pKeys, list[other], pResult); },
            [&](UINT other) { return sourceIndexOf[list[other]]; },
            [this](UINT sourceIndex) { return isSortSkipped && isSortSkipped(sourceIndex); },
            &newPosition, &first, &last));
        isMoving = position < first || position > last;
    }

    if (isMoving)
    {
        IFC_RETURN(Detach(id, output));
    }

    if (pKeys && nodes[id].slot != SortKeyStore::c_noSlot)
    {
        sort->keys.Set(nodes[id].slot, *pKeys);
    }

    if (wasKept && isKept)
    {
        if (isMoving)
        {
            IFC_RETURN(Attach(id, output));
        }
        else if (isReplace && spOldItem.Get() == pItem && output.pFlat)
        {
            // A same-object Replace is a refresh; consumers redraw the row in place.
            IFC_RETURN(output.pFlat->InternalSetAt(FlatIndexOf(id), pItem));
        }
        IFC_RETURN(SetIsContext(id));
    }
    else if (wasKept)
    {
        IFC_RETURN(Detach(id, output));
        nodes[id].isKept = false;
        nodes[id].kept.clear();
        nodes[id].keptSubtree = 0;
        ReleaseProjection(id);
        IFC_RETURN(DemoteAncestors(parent, output));
    }
    else if (isKept)
    {
        auto& node = nodes[id];
        node.isKept = true;
        node.kept.clear();
        node.keptSubtree = 1;
        IFC_RETURN(PromoteAncestors(id, output));
    }
    return S_OK;
}

// ---- VectorCollectionView ----

HierarchyOutput VectorCollectionView::GetHierarchyOutput() const
{
    HierarchyOutput output;
    output.pFlat = static_cast<InternalList*>(m_tpInternalList.Get());
    output.pRoots = static_cast<NodeListVector*>(m_tpNodeList.Get());
    return output;
}

IFACEMETHODIMP VectorCollectionView::get_HierarchyDescription(_Outptr_result_maybenull_ xaml_data::IHierarchyDescription** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpHierarchyDescription.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_HierarchyDescription(_In_opt_ xaml_data::IHierarchyDescription* pValue)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(SetHierarchyDescription(pValue));
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::SetHierarchyDescription(_In_opt_ xaml_data::IHierarchyDescription* pValue)
{
    if (pValue)
    {
        UINT groupCount = 0;
        if (m_tpGroupDescriptions)
        {
            IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
        }
        IFC_RETURN(CheckHierarchyWithoutGrouping(groupCount));
        auto spDescription = ctl::query_interface_cast<HierarchyDescription>(pValue);
        if (spDescription)
        {
            IFC_RETURN(spDescription->AddOwner(this));
        }
    }
    SetPtrValue(m_tpHierarchyDescription, pValue);
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::CheckHierarchyWithoutGrouping(UINT groupCount)
{
    if (groupCount > 0)
    {
        static constexpr WCHAR c_message[] = L"A CollectionView cannot group its items and build a hierarchy at the same time.";
        IFC_RETURN(ErrorHelper::OriginateError(E_ILLEGAL_METHOD_CALL, ARRAYSIZE(c_message) - 1, c_message));
    }
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnHierarchyDescriptionChanged(_In_ HierarchyDescription* pDescription)
{
    auto spCurrent = ctl::query_interface_cast<HierarchyDescription>(m_tpHierarchyDescription.Get());
    if (spCurrent.Get() == pDescription)
    {
        IFC_RETURN(OnShapingRulesChanged());
    }
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CanBuildHierarchy(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    UINT groupCount = 0;
    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }
    *pValue = groupCount == 0;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CollectionNodes(_Outptr_result_maybenull_ wfc::IObservableVector<IInspectable*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue = nullptr;
    IFC_RETURN(CheckThread());
    if (!m_isHierarchical || !m_spHierarchy)
    {
        return S_OK;
    }

    IFC_RETURN(EnsureNodeList());
    *ppValue = static_cast<wfc::IObservableVector<IInspectable*>*>(static_cast<NodeListVector*>(m_tpNodeList.Get()));
    (*ppValue)->AddRef();
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::NodeFromItem(_In_opt_ IInspectable* pItem, _Outptr_result_maybenull_ xaml_data::ICollectionViewNode** ppNode)
{
    ARG_VALIDRETURNPOINTER(ppNode);
    *ppNode = nullptr;
    IFC_RETURN(CheckThread());
    auto spStore = m_isHierarchical ? m_spHierarchy : nullptr;
    if (!spStore)
    {
        return S_OK;
    }

    NodeId id = c_noNode;
    IFC_RETURN(spStore->FindKeptNode(pItem, &id));
    if (id != c_noNode)
    {
        IFC_RETURN(spStore->GetProjection(id, ppNode));
    }
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EnsureNodeList()
{
    if (!m_tpNodeList)
    {
        ctl::ComPtr<NodeListVector> spList;
        IFC_RETURN(ctl::ComObject<NodeListVector>::CreateInstance(spList.ReleaseAndGetAddressOf()));
        spList->Bind(m_spHierarchy, c_noNode);
        SetPtrValue(m_tpNodeList, static_cast<wfc::IVector<IInspectable*>*>(spList.Get()));
    }
    return S_OK;
}

// Full build (RefreshOverride while hierarchical): every source item gets a node; keys, filter and sort keys are read
// once per item. The flat list and CollectionNodes are each announced with one Reset.
_Check_return_ HRESULT VectorCollectionView::RefreshHierarchy()
{
    auto spSort = std::make_shared<ShapingSortState>();
    auto spStore = std::make_shared<HierarchyStore>();
    ShapingFilterEvents filterEvents;
    HierarchyDiagnostics diagnostics;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    std::vector<NodeId> order;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT size = 0;

    IFC_RETURN(spSort->comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get(), spSort->references));
    spSort->keys.Reset(spSort->comparer.FieldCount());
    spStore->sort = spSort;

    {
        wrl_wrappers::HString itemKeyPath;
        wrl_wrappers::HString parentKeyPath;
        auto spDescription = m_tpHierarchyDescription.AsOrNull<xaml_data::IPropertyHierarchyDescription>();
        if (spDescription)
        {
            IFC_RETURN(spDescription->get_ItemKeyPath(itemKeyPath.GetAddressOf()));
            IFC_RETURN(spDescription->get_ParentKeyPath(parentKeyPath.GetAddressOf()));
        }
        IFC_RETURN(CreateKeySelector(itemKeyPath.Get(), &spStore->keySelector));
        IFC_RETURN(CreateKeySelector(parentKeyPath.Get(), &spStore->parentKeySelector));
    }

    IFC_RETURN(m_tpSourceList->get_Size(&size));
    IFC_RETURN(GetFilterEvents(&filterEvents));
    spStore->Reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC_RETURN(m_tpSourceList->GetAt(i, spItem.ReleaseAndGetAddressOf()));
        NodeId id = c_noNode;
        IFC_RETURN(AllocateHierarchyNode(*spStore, filterEvents, spItem.Get(), i, &id));
        spStore->bySource.push_back(id);
    }

    IFC_RETURN(spStore->BuildTree(&diagnostics));
    spStore->CollectPreOrder(order);
    IFC_RETURN(spStore->FlatItems(order, &outputItems));
    IFC_RETURN(RebuildLiveShapingItems());
    LogHierarchyDataError(diagnostics.duplicateCount, diagnostics.cycleCount);

    const bool wasHierarchical = m_isHierarchical;
    std::shared_ptr<HierarchyStore> spPrevious = spStore;
    ctl::ComPtr<xaml_data::ICollectionViewGroup> spPreviousRoot;
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        std::swap(m_spSortState, spSort);
        std::swap(m_spHierarchy, spPrevious);
        std::swap(m_spGroupRoot, spPreviousRoot);
    }

    m_sourceIndices.clear();
    m_isHierarchical = true;
    m_sourceListSize = size;
    m_areSourceIndicesValid = (sourceChangeCount == m_sourceListChangeCount);

    if (m_tpNodeList)
    {
        auto pRoots = static_cast<NodeListVector*>(m_tpNodeList.Get());
        pRoots->Bind(spStore, c_noNode);
        IFC_RETURN(pRoots->RaiseChanged(wfc::CollectionChange_Reset, 0));
    }
    if (!wasHierarchical)
    {
        IFC_RETURN(ContainRaiseFailure(RaisePropertyChanged(STR_LEN_PAIR(L"CollectionNodes"))));
    }
    IFC_RETURN(static_cast<InternalList*>(m_tpInternalList.Get())->ReplaceAll(outputItems));

    // After the roots' Reset, so a consumer that rebuilt from the new roots has already let go of these: the old
    // nodes' Children lists empty, and each one somebody still follows says so.
    if (spPrevious)
    {
        spPrevious->DetachProjections(true /* isRaised */);
    }
    return S_OK;
}

// The view stops being hierarchical: CollectionNodes turns null; consumers holding the old lists see them emptied.
_Check_return_ HRESULT VectorCollectionView::LeaveHierarchy()
{
    if (!m_isHierarchical)
    {
        return S_OK;
    }

    std::shared_ptr<HierarchyStore> spPrevious;
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        spPrevious = std::move(m_spHierarchy);
    }
    m_isHierarchical = false;
    if (m_tpNodeList)
    {
        IFC_RETURN(static_cast<NodeListVector*>(m_tpNodeList.Get())->Detach(true /* isRaised */));
    }
    IFC_RETURN(ContainRaiseFailure(RaisePropertyChanged(STR_LEN_PAIR(L"CollectionNodes"))));
    if (spPrevious)
    {
        spPrevious->DetachProjections(true /* isRaised */);
    }
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::AllocateHierarchyNode(
    _In_ HierarchyStore& store,
    const ShapingFilterEvents& filterEvents,
    _In_opt_ IInspectable* pItem,
    UINT sourceIndex,
    _Out_ NodeId* pId)
{
    ctl::ComPtr<IInspectable> spKey;
    ctl::ComPtr<IInspectable> spParentKey;
    NodeKey key;
    NodeKey parentKey;
    SortKeys keys;
    bool isMatch = true;

    IFC_RETURN(EvaluateKey(store.keySelector, pItem, spKey.ReleaseAndGetAddressOf()));
    IFC_RETURN(MakeNodeKey(spKey.Get(), &key));
    IFC_RETURN(EvaluateKey(store.parentKeySelector, pItem, spParentKey.ReleaseAndGetAddressOf()));
    IFC_RETURN(MakeNodeKey(spParentKey.Get(), &parentKey));
    IFC_RETURN(PassesFilter(pItem, filterEvents, &isMatch));
    if (store.sort->keys.FieldCount() > 0)
    {
        IFC_RETURN(store.sort->comparer.EvaluateKeys(pItem, &keys));
    }
    IFC_RETURN(store.AllocateNode(pItem, sourceIndex, std::move(key), spKey.Get(), std::move(parentKey), spParentKey.Get(), isMatch, std::move(keys), pId));
    return S_OK;
}

// A source Add, Remove or Replace on a hierarchical view. App code (key and sort getters, the filter) runs before
// anything changes, and so does the check whether the change needs a recompute (a key taken from a later duplicate, a
// cycle), which leaves *pIsApplied false and the store untouched. Then the tree is updated in place and each vector
// raises what changed in it. A change that fails or is overtaken once it began to change the store leaves the store
// stale and the source indices invalid: only a recompute uses the store again.
_Check_return_ HRESULT VectorCollectionView::ProcessHierarchyCollectionChanged(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied)
{
    auto spStore = m_spHierarchy;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    const HierarchyOutput output = GetHierarchyOutput();
    const UINT previousSize = static_cast<UINT>(spStore ? spStore->bySource.size() : 0);
    HRESULT hr = S_OK;
    bool isMutating = false;

    *pIsApplied = false;
    if (!spStore || spStore->isStale || previousSize != m_sourceListSize
        + ((change == wfc::CollectionChange_ItemRemoved) ? 1 : 0)
        - ((change == wfc::CollectionChange_ItemInserted) ? 1 : 0))
    {
        return S_OK;
    }
    if (change == wfc::CollectionChange_ItemInserted ? index > previousSize : (index >= previousSize || spStore->bySource[index] == c_noNode))
    {
        return S_OK;
    }

    spStore->isSortSkipped = [this, change, index](UINT sourceIndex)
    {
        if (change == wfc::CollectionChange_ItemRemoved)
        {
            return IsSortDirtyAfterSourceChange(wfc::CollectionChange_ItemChanged, index, (sourceIndex >= index) ? sourceIndex + 1 : sourceIndex);
        }
        return IsSortDirtyAfterSourceChange(change, index, sourceIndex);
    };
    auto clearSkip = wil::scope_exit([&spStore]() noexcept { spStore->isSortSkipped = nullptr; });
    auto invalidate = wil::scope_exit([&]() noexcept
    {
        if (isMutating && !*pIsApplied)
        {
            spStore->isStale = true;
            m_areSourceIndicesValid = false;
        }
    });

    if (change == wfc::CollectionChange_ItemRemoved)
    {
        const NodeId id = spStore->bySource[index];
        if (spStore->IsRemoveComplex(id))
        {
            return S_OK;
        }
        BusyScope busy(m_shapeBusyDepth);
        isMutating = true;
        spStore->ShiftSourceIndices(index, -1);
        spStore->BeginFlatBatch();
        hr = spStore->RemoveNode(id, output);
        const HRESULT hrEnd = spStore->EndFlatBatch(output, SUCCEEDED(hr));
        IFC_RETURN(hr);
        IFC_RETURN(hrEnd);
        *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
        return S_OK;
    }

    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<IInspectable> spKey;
    ctl::ComPtr<IInspectable> spParentKey;
    NodeKey key;
    NodeKey parentKey;
    SortKeys keys;
    bool isMatch = true;
    IFC_RETURN(m_tpSourceList->GetAt(index, spItem.ReleaseAndGetAddressOf()));
    {
        BusyScope busy(m_shapeBusyDepth);
        hr = [&]() -> HRESULT
        {
            ShapingFilterEvents filterEvents;
            IFC_RETURN(GetFilterEvents(&filterEvents));
            IFC_RETURN(EvaluateKey(spStore->keySelector, spItem.Get(), spKey.ReleaseAndGetAddressOf()));
            IFC_RETURN(MakeNodeKey(spKey.Get(), &key));
            IFC_RETURN(EvaluateKey(spStore->parentKeySelector, spItem.Get(), spParentKey.ReleaseAndGetAddressOf()));
            IFC_RETURN(MakeNodeKey(spParentKey.Get(), &parentKey));
            IFC_RETURN(PassesFilter(spItem.Get(), filterEvents, &isMatch));
            if (spStore->sort->keys.FieldCount() > 0)
            {
                IFC_RETURN(spStore->sort->comparer.EvaluateKeys(spItem.Get(), &keys));
            }
            return S_OK;
        }();
    }
    IFC_RETURN(hr);
    if (sourceChangeCount != m_sourceListChangeCount || spStore->isStale)
    {
        return S_OK;
    }

    BusyScope busy(m_shapeBusyDepth);
    NodeId replaced = c_noNode;
    if (change == wfc::CollectionChange_ItemChanged)
    {
        const NodeId id = spStore->bySource[index];
        const auto& node = spStore->nodes[id];
        if (node.key == key && node.parentKey == parentKey)
        {
            isMutating = true;
            spStore->BeginFlatBatch();
            hr = spStore->UpdateNode(id, spItem.Get(), isMatch, &keys, output, true);
            const HRESULT hrEnd = spStore->EndFlatBatch(output, SUCCEEDED(hr));
            IFC_RETURN(hr);
            IFC_RETURN(hrEnd);
            *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
            return S_OK;
        }

        if (spStore->IsRemoveComplex(id) || spStore->IsInsertComplex(index, key, parentKey, id))
        {
            return S_OK;
        }
        replaced = id;
    }
    else if (spStore->IsInsertComplex(index, key, parentKey))
    {
        return S_OK;
    }

    isMutating = true;
    spStore->BeginFlatBatch();
    hr = [&]() -> HRESULT
    {
        if (replaced != c_noNode)
        {
            IFC_RETURN(spStore->RemoveNode(replaced, output));
            spStore->bySource[index] = c_noNode;
        }
        else
        {
            spStore->ShiftSourceIndices(index, 1);
        }

        NodeId id = c_noNode;
        IFC_RETURN(spStore->AllocateNode(spItem.Get(), index, std::move(key), spKey.Get(), std::move(parentKey), spParentKey.Get(), isMatch, std::move(keys), &id));
        spStore->bySource[index] = id;
        IFC_RETURN(spStore->InsertNode(id, output));
        return S_OK;
    }();
    const HRESULT hrEnd = spStore->EndFlatBatch(output, SUCCEEDED(hr));
    IFC_RETURN(hr);
    IFC_RETURN(hrEnd);
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}

// A live restore of one item while hierarchical: its filter result and sort keys are read again, and the node is
// moved among its siblings, or shown or hidden with its context ancestors.
_Check_return_ HRESULT VectorCollectionView::RestoreHierarchyItem(UINT sourceIndex, bool isFiltering, bool isResorting, _In_opt_ IInspectable* pItem, const std::function<bool(UINT)>& isSkipped, _Out_ bool* pIsApplied)
{
    auto spStore = m_spHierarchy;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    *pIsApplied = false;
    if (!spStore || spStore->isStale || sourceIndex >= spStore->bySource.size() || spStore->bySource[sourceIndex] == c_noNode)
    {
        return S_OK;
    }

    const NodeId id = spStore->bySource[sourceIndex];
    bool isMatch = spStore->nodes[id].isMatch;
    SortKeys keys;
    HRESULT hr = S_OK;
    {
        BusyScope busy(m_shapeBusyDepth);
        hr = [&]() -> HRESULT
        {
            if (isFiltering)
            {
                ShapingFilterEvents filterEvents;
                IFC_RETURN(GetFilterEvents(&filterEvents));
                IFC_RETURN(PassesFilter(pItem, filterEvents, &isMatch));
            }
            if (isResorting)
            {
                IFC_RETURN(spStore->sort->comparer.EvaluateKeys(pItem, &keys));
            }
            return S_OK;
        }();
    }
    IFC_RETURN(hr);
    if (sourceChangeCount != m_sourceListChangeCount || spStore->isStale)
    {
        return S_OK;
    }

    BusyScope busy(m_shapeBusyDepth);
    spStore->isSortSkipped = isSkipped;
    auto clearSkip = wil::scope_exit([&spStore]() noexcept { spStore->isSortSkipped = nullptr; });
    auto invalidate = wil::scope_exit([&]() noexcept
    {
        if (!*pIsApplied)
        {
            spStore->isStale = true;
            m_areSourceIndicesValid = false;
        }
    });
    const HierarchyOutput output = GetHierarchyOutput();
    spStore->BeginFlatBatch();
    hr = spStore->UpdateNode(id, pItem, isMatch, isResorting ? &keys : nullptr, output);
    const HRESULT hrEnd = spStore->EndFlatBatch(output, SUCCEEDED(hr));
    IFC_RETURN(hr);
    IFC_RETURN(hrEnd);
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}
void VectorCollectionView::WalkHierarchy(EReferenceTrackerWalkType walkType)
{
    if (m_spHierarchy)
    {
        m_spHierarchy->ReferenceTrackerWalk(walkType);
    }
}

void VectorCollectionView::ReleaseHierarchy()
{
    std::shared_ptr<HierarchyStore> spHierarchy;
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        spHierarchy = std::move(m_spHierarchy);
    }
}

// ---- HierarchyDescription / PropertyHierarchyDescription ----

_Check_return_ HRESULT HierarchyDescription::AddOwner(_In_ VectorCollectionView* pView)
{
    bool isOwner = false;

    m_owners.erase(std::remove_if(m_owners.begin(), m_owners.end(), [pView, &isOwner](ctl::WeakRefPtr& ownerRef)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spOwner;
        if (FAILED(ownerRef.As(&spOwner)) || !spOwner)
        {
            return true;
        }
        isOwner = isOwner || (static_cast<CollectionView*>(spOwner.Get()) == static_cast<CollectionView*>(pView));
        return false;
    }), m_owners.end());

    if (!isOwner)
    {
        ctl::WeakRefPtr ownerRef;
        IFC_RETURN(ctl::AsWeak(static_cast<CollectionView*>(pView), &ownerRef));
        m_owners.push_back(std::move(ownerRef));
    }
    return S_OK;
}

// As GroupDescription.NotifyOwners: every view that builds its tree with this description rebuilds it.
_Check_return_ HRESULT HierarchyDescription::NotifyOwners()
{
    std::vector<ctl::ComPtr<xaml_data::ICollectionView>> owners;
    for (auto& ownerRef : m_owners)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spOwner;
        IFC_RETURN(ownerRef.As(&spOwner));
        if (spOwner)
        {
            owners.push_back(std::move(spOwner));
        }
    }

    for (const auto& spOwner : owners)
    {
        IFC_RETURN(static_cast<VectorCollectionView*>(static_cast<CollectionView*>(spOwner.Get()))->OnHierarchyDescriptionChanged(this));
    }
    return S_OK;
}

_Check_return_ HRESULT PropertyHierarchyDescriptionFactory::CreateInstanceWithKeyPathsImpl(
    _In_opt_ HSTRING itemKeyPath,
    _In_opt_ HSTRING parentKeyPath,
    _Outptr_ xaml_data::IPropertyHierarchyDescription** ppInstance)
{
    ctl::ComPtr<PropertyHierarchyDescription> spDescription;
    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_ItemKeyPath(itemKeyPath));
    IFC_RETURN(spDescription->put_ParentKeyPath(parentKeyPath));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyHierarchyDescription::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(PropertyHierarchyDescriptionGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::PropertyHierarchyDescription_ItemKeyPath:
    case KnownPropertyIndex::PropertyHierarchyDescription_ParentKeyPath:
        IFC_RETURN(NotifyOwners());
        break;
    }
    return S_OK;
}
