// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "DependentAsyncWorker.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemsPresenter.g.h"

using namespace DirectUI;

_Check_return_ HRESULT
ListViewBase::GetRelativeScrollPositionImpl(
    _In_ xaml_controls::IListViewItemToKeyHandler* itemToKeyHandler,
    _Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spItemsPanel;

    *returnValue = nullptr;

    IFC(get_ItemsPanelRoot(&spItemsPanel));
    if (spItemsPanel)
    {
        ctl::ComPtr<IModernCollectionBasePanel> spModernPanel = spItemsPanel.AsOrNull<IModernCollectionBasePanel>();

        if (spModernPanel)
        {
            ctl::ComPtr<wsts::IBuffer> spBuffer;
            ctl::ComPtr<xaml_controls::IItemsPresenter> spItemsPresenter;
            ItemsPresenter::ItemsPresenterParts firstVisiblePart = ItemsPresenter::ItemsPresenterParts_Header;
            ListSerializationData serializationData = {};
            serializationData.version = ListSerializationFormatVersions_PhoneBlue;
            serializationData.elementType = ElementType::Other;     // ElementType::Other is a no-op during deserialization.

            IFC(spItemsPanel.As(&spModernPanel));
            IFC(get_ItemsPresenter(&spItemsPresenter));

            ASSERT(spModernPanel);

            IFC(spItemsPresenter.Cast<ItemsPresenter>()->GetFirstVisiblePart(
                &firstVisiblePart,
                &serializationData.viewportOffset));

            if (firstVisiblePart == ItemsPresenter::ItemsPresenterParts_Header)
            {
                serializationData.elementType = ElementType::Header;
            }
            else if (firstVisiblePart == ItemsPresenter::ItemsPresenterParts_Footer)
            {
                serializationData.elementType = ElementType::Footer;
            }
            else
            {
                ctl::ComPtr<IUIElement> spFirstVisibleElement;
                xaml_controls::ElementType elementType = xaml_controls::ElementType_ItemContainer;

                IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->GetFirstVisibleElementForSerialization(
                    &elementType,
                    &serializationData.viewportOffset,
                    &spFirstVisibleElement));

                if (spFirstVisibleElement)
                {
                    ctl::ComPtr<IInspectable> spFirstVisibleObject;

                    if (elementType == xaml_controls::ElementType_ItemContainer)
                    {
                        ctl::ComPtr<IItemContainerMapping> spModernPanelAsIICM;
                        IFC(spModernPanel.As<IItemContainerMapping>(&spModernPanelAsIICM));
                        IFC(spModernPanelAsIICM->ItemFromContainer(spFirstVisibleElement.Cast<UIElement>(), &spFirstVisibleObject));

                        serializationData.elementType = ElementType::Item;
                    }
                    else
                    {
                        ctl::ComPtr<IInspectable> spCollectionViewGroupAsII;
                        ctl::ComPtr<xaml_data::ICollectionViewGroup> spCollectionViewGroup;

                        ctl::ComPtr<IGroupHeaderMapping> spModernPanelAsIGHM;
                        IFC(spModernPanel.As<IGroupHeaderMapping>(&spModernPanelAsIGHM));
                        IFC(spModernPanelAsIGHM->GroupFromHeader(spFirstVisibleElement.Cast<UIElement>(), &spCollectionViewGroupAsII));
                        IFC(spCollectionViewGroupAsII.As(&spCollectionViewGroup));
                        IFC(spCollectionViewGroup->get_Group(&spFirstVisibleObject));

                        serializationData.elementType = ElementType::GroupHeader;
                    }

                    if (spFirstVisibleObject)
                    {
                        IFC(itemToKeyHandler->Invoke(spFirstVisibleObject.Get(), serializationData.serializationKey.ReleaseAndGetAddressOf()));
                    }

                    // If, for some reason, we can't serialize (e.g. the serialization key is ...or empty, there is
                    // no way to tell the difference in WinRT), the backup plan is to return a no-op buffer.
                    if (serializationData.serializationKey.Get() == nullptr)
                    {
                        serializationData.elementType = ElementType::Other;
                        serializationData.viewportOffset = 0;
                    }
                }
            }

            // Creates the serialization buffer, encode it and returns it as a string.
            IFC(CreateSerializationBuffer(serializationData, &spBuffer));
            IFC(EncodeBuffer(spBuffer.Get(), returnValue));
        }
        else
        {
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_INCORRECT_PANEL_FOR_SERIALIZATION));
        }
    }
    else
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ITEMSPANELROOT_NOT_READY));
    }

Cleanup:
    RRETURN(hr);
}

extern __declspec(selectany) const WCHAR TryDeserializeListAsyncActionName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Controls.UIListSerialization.TryDeserializeListAsync";

_Check_return_ HRESULT
ListViewBase::SetRelativeScrollPositionAsyncImpl(
    _In_ HSTRING relativeScrollPosition,
    _In_ xaml_controls::IListViewKeyToItemHandler* keyToItemHandler,
    _Outptr_ wf::IAsyncAction** returnValue)
{

    typedef DependentAsyncWorker<AsyncActionWorkerTraits<wrl::AsyncCausalityOptions<TryDeserializeListAsyncActionName>>, AsyncOperationWorkerTraits<IInspectable*>>
        ListDeserializationAsyncOperation;

    ctl::ComPtr<wsts::IBuffer> spBuffer;
    wrl::ComPtr<ListDeserializationAsyncOperation> spListDeserializationAsyncOperation;
    ctl::WeakRefPtr wrThis;

    ListSerializationData serializationData = {};
    HSTRING serializationKey = nullptr;
    BOOLEAN parsingSucceeded = FALSE;

    auto guard = wil::scope_exit([&serializationKey]()
    {
        DELETE_STRING(serializationKey);
    });

    // Decodes and parse the buffer.
    IFC_RETURN(DecodeBuffer(relativeScrollPosition, &spBuffer));
    IFC_RETURN(ParseSerializationBuffer(spBuffer.Get(), &serializationData, &parsingSucceeded));

    const BOOLEAN isHeader = (serializationData.elementType == ElementType::Header);
    const BOOLEAN isFooter = (serializationData.elementType == ElementType::Footer);
    const DOUBLE offset = serializationData.viewportOffset;

    const BOOLEAN runChildAsyncOperation = parsingSucceeded && !isHeader && !isFooter;

    serializationKey = serializationData.serializationKey.Detach();

    IFC_RETURN(ctl::AsWeak(this, &wrThis));

    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<ListDeserializationAsyncOperation>(&spListDeserializationAsyncOperation));

    IFC_RETURN(spListDeserializationAsyncOperation->InitializeAsyncWorkers(
        // Returns the child async operation.
        [keyToItemHandler, runChildAsyncOperation, serializationKey](wf::IAsyncOperation<IInspectable*>** result)
        {
            *result = nullptr;
            if (runChildAsyncOperation)
            {
                IFC_RETURN(keyToItemHandler->Invoke(serializationKey, result));
            }
            return S_OK;
        },
        // Runs after the child operation has completed.
        [wrThis, parsingSucceeded, isHeader, isFooter, offset](wf::AsyncStatus status, IInspectable* item) mutable
        {
            ctl::ComPtr<ListViewBase> spThis = wrThis.AsOrNull<IListViewBase>().Cast<ListViewBase>();

            if (spThis && status == wf::AsyncStatus::Completed && parsingSucceeded)
            {
                std::function<HRESULT(IInspectable*)> scrollCommand =
                    spThis->GetDeserializationScrollCommand(isHeader, isFooter, offset);

                // Schedule the scroll operation right away if the panel is ready.
                // Otherwise, defer it.
                {
                    ctl::ComPtr<xaml_controls::IPanel> spItemsPanel;

                    IFC_RETURN(spThis->get_ItemsPanelRoot(&spItemsPanel));
                    if (spItemsPanel)
                    {
                        IFC_RETURN(scrollCommand(item));
                    }
                    else
                    {
                        const DeferPoint deferUntil = (isHeader || isFooter) ? DeferPoint::ListViewBaseLoaded : DeferPoint::ItemsHostAvailable;

                        if (isFooter)
                        {
                            spThis->m_isDeferredElementFooter = true;
                        }

                        spThis->ScheduleDeferredScrollCommand(std::move(scrollCommand), item, deferUntil);
                    }
                }
            }

            return S_OK;
        }));

    IFC_RETURN(spListDeserializationAsyncOperation->StartOperation());
    *returnValue = spListDeserializationAsyncOperation.Detach();

    return S_OK;
}

_Check_return_ HRESULT
ListViewBase::CreateSerializationBuffer(
    _In_ const ListSerializationData& serializationData,
    _Outptr_ wsts::IBuffer** returnValue) const
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wsts::IDataWriter> spDataWriter;
    UINT32 serializationKeyCodeUnitCount = 0;
    UINT32 serializationKeyCodeUnitCountWritten = 0;

    IFC(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
        &spDataWriter));

    static_assert(std::is_same<std::underlying_type<ListSerializationFormatVersions>::type, INT32>::value,
        "ListSerializationFormatVersions is serialized as an INT32.");
    static_assert(std::is_same<std::underlying_type<ElementType>::type, INT32>::value,
        "ElementType is serialized as an INT32.");

    IFC(spDataWriter->WriteInt32(static_cast<INT32>(serializationData.version)));
    IFC(spDataWriter->WriteInt32(static_cast<INT32>(serializationData.elementType)));
    IFC(spDataWriter->WriteDouble(serializationData.viewportOffset));

    IFC(spDataWriter->MeasureString(serializationData.serializationKey.Get(), &serializationKeyCodeUnitCount));
    IFC(spDataWriter->WriteUInt32(serializationKeyCodeUnitCount));
    IFC(spDataWriter->WriteString(serializationData.serializationKey.Get(), &serializationKeyCodeUnitCountWritten));

    ASSERT(serializationKeyCodeUnitCount == serializationKeyCodeUnitCountWritten);

    if (serializationKeyCodeUnitCount != serializationKeyCodeUnitCountWritten)
    {
        TraceLoggingWrite(
            g_hTraceProvider,
            "ListViewBaseSerializationInvalidWrite",
            TraceLoggingUInt32(serializationKeyCodeUnitCount, "serializationKeyCodeUnitCount"),
            TraceLoggingWideString(serializationData.serializationKey.GetRawBuffer(&serializationKeyCodeUnitCount), "serializationKey"),
            TraceLoggingUInt32(serializationKeyCodeUnitCountWritten, "serializationKeyCodeUnitCountWritten"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    }

    IFC(spDataWriter->DetachBuffer(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBase::ParseSerializationBuffer(
    _In_ wsts::IBuffer* pBuffer,
    _Out_ ListSerializationData* pSerializationData,
    _Out_ BOOLEAN* pSucceeded) const
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wsts::IDataReader> spDataReader;
    wrl::ComPtr<wsts::IDataReaderStatics> spDataReaderStatics;

    *pSucceeded = FALSE;

    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Storage_Streams_DataReader).Get(),
        &spDataReaderStatics));

    IFC(spDataReaderStatics->FromBuffer(pBuffer, &spDataReader));

    static_assert(std::is_same<std::underlying_type<ListSerializationFormatVersions>::type, INT32>::value,
        "ListSerializationFormatVersions is serialized as an INT32.");
    static_assert(std::is_same<std::underlying_type<ElementType>::type, INT32>::value,
        "ElementType is serialized as an INT32.");

    IFC(spDataReader->ReadInt32(reinterpret_cast<INT32*>(&pSerializationData->version)));

    if (pSerializationData->version == ListSerializationFormatVersions_PhoneBlue)
    {
        UINT32 serializationKeyCodeUnitCount = 0;

        IFC(spDataReader->ReadInt32(reinterpret_cast<INT32*>(&pSerializationData->elementType)));
        IFC(spDataReader->ReadDouble(&pSerializationData->viewportOffset));

        IFC(spDataReader->ReadUInt32(&serializationKeyCodeUnitCount));
        IFC(spDataReader->ReadString(serializationKeyCodeUnitCount, pSerializationData->serializationKey.ReleaseAndGetAddressOf()));

        *pSucceeded = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

std::function<HRESULT(IInspectable*)>
ListViewBase::GetDeserializationScrollCommand(
    _In_ BOOLEAN isHeader,
    _In_ BOOLEAN isFooter,
    _In_ DOUBLE offset)
{
    return [this, isHeader, isFooter, offset](IInspectable* scrollToItem) -> HRESULT
    {
        ctl::ComPtr<xaml_controls::IPanel> itemsPanel;
        IFC_RETURN(get_ItemsPanelRoot(&itemsPanel));
        if (itemsPanel)
        {
            ctl::ComPtr<IModernCollectionBasePanel> modernPanel;
            IFC_RETURN(itemsPanel.As(&modernPanel));

            if (modernPanel)
            {
                const double effectiveOffset =
                    isHeader || isFooter ?
                    offset :
                    modernPanel.Cast<ModernCollectionBasePanel>()->AdjustViewportOffsetForDeserialization(offset);

                IFC_RETURN(ScrollIntoViewInternal(
                    scrollToItem,
                    isHeader,
                    isFooter,
                    FALSE /* isFromPublicAPI */,
                    xaml_controls::ScrollIntoViewAlignment_Leading,
                    effectiveOffset));
            }
        }
        return S_OK;
    };
}

_Check_return_ HRESULT
ListViewBase::EncodeBuffer(
    _In_ wsts::IBuffer* pBuffer,
    _Out_ HSTRING* pBase64String)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wsec::Cryptography::ICryptographicBufferStatics> spCryptographicBufferStatics;

    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &spCryptographicBufferStatics));

    IFC(spCryptographicBufferStatics->EncodeToBase64String(pBuffer, pBase64String));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBase::DecodeBuffer(
    _In_ HSTRING base64String,
    _Outptr_ wsts::IBuffer** ppBuffer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wsec::Cryptography::ICryptographicBufferStatics> spCryptographicBufferStatics;

    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &spCryptographicBufferStatics));

    IFC(spCryptographicBufferStatics->DecodeFromBase64String(base64String, ppBuffer));

Cleanup:
    RRETURN(hr);
}
