// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualTreeDumper.h"

#include <shlwapi.h>
#include <xmllite.h>
#include <array>

#include "WindowHelper.h"
#include "Utilities.h"
#include "IXamlTestHooks-win.h"

#include <XamlTailored.h>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

    void VisualTreeDumper::DumpToXml(_In_opt_ xaml::IDependencyObject* pRoot, _In_ HSTRING filenameWithPath, bool ignorePopups)
    {
        wrl::ComPtr<xaml::IDependencyObject> root(pRoot);

        wrl::ComPtr<IXmlWriter> spXmlWriter;
        CreateAndInitializeXmlWriter(filenameWithPath, &spXmlWriter);

        LogThrow_IfFailed(spXmlWriter->WriteStartDocument(XmlStandalone_Omit));
        LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"VisualTreeDump", nullptr /*pwszNamespaceUri */));

        RunOnUIThread([&]()
        {
            if (!root)
            {
                // If no root was specified, dump all ContentRoots.

                wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
                LogThrow_IfFailed(wf::GetActivationFactory(
                    wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
                    &testServicesStatics));

                wrl::ComPtr<test_infra::IWindowHelper> windowHelper;
                LogThrow_IfFailed(testServicesStatics->get_WindowHelper(&windowHelper));

                wrl::ComPtr<wfc::IVectorView<xaml::XamlRoot*>> xamlRoots;
                windowHelper->GetXamlRoots(&xamlRoots);

                unsigned int size;
                LogThrow_IfFailed(xamlRoots->get_Size(&size));

                for (unsigned int i = 0; i < size; i++)
                {
                    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
                    LogThrow_IfFailed(xamlRoots->GetAt(i, xamlRoot.ReleaseAndGetAddressOf()));

                    wrl::ComPtr<xaml::IUIElement> content;
                    LogThrow_IfFailed(xamlRoot->get_Content(content.ReleaseAndGetAddressOf()));

                    wrl::ComPtr<xaml::IDependencyObject> contentDO;
                    LogThrow_IfFailed(content.As(&contentDO));

                    DumpElement(
                        contentDO,
                        ignorePopups ? nullptr : xamlRoot,
                        spXmlWriter);
                }
            }
            else
            {
                wrl::ComPtr<xaml::IUIElement> rootAsUIE;
                root.As(&rootAsUIE);

                wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
                rootAsUIE->get_XamlRoot(&xamlRoot);

                DumpElement(
                    root,
                    ignorePopups ? nullptr : xamlRoot.Get(),
                    spXmlWriter);
            }
        });

        LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </VisualTreeDump>
        LogThrow_IfFailed(spXmlWriter->WriteEndDocument());
        LogThrow_IfFailed(spXmlWriter->Flush());
    }

    /* static */ void VisualTreeDumper::DumpElement(
        _In_ const wrl::ComPtr<xaml::IDependencyObject>& root,
        _In_ const wrl::ComPtr<xaml::IXamlRoot>& xamlRootForPopups,
        _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter)
    {
        wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVisualTreeHelperStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
            &spVisualTreeHelperStatics));

        if (root)
        {
            LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"VisualRoot", nullptr /*pwszNamespaceUri */));
            DumpToXmlWorker(root, spVisualTreeHelperStatics, spXmlWriter, false);
            LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </VisualRoot>
        }

        if (xamlRootForPopups != nullptr)
        {
            wrl::ComPtr<wfc::IVectorView<xaml_primitives::Popup*>> spOpenPopupsVectorView;
            unsigned int popupCount = 0;

            WindowHelper::GetOpenPopupsInXamlRootStatic(xamlRootForPopups.Get(), &spOpenPopupsVectorView);
            LogThrow_IfFailed(spOpenPopupsVectorView->get_Size(&popupCount));

            if (popupCount > 0)
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"PopupRoot", nullptr /*pwszNamespaceUri */));

                for (unsigned int popupIndex = 0; popupIndex < popupCount; ++popupIndex)
                {
                    wrl::ComPtr<xaml_primitives::IPopup> spPopup;
                    wrl::ComPtr<xaml::IDependencyObject> spPopupAsDO;

                    LogThrow_IfFailed(spOpenPopupsVectorView->GetAt(popupIndex, &spPopup));
                    LogThrow_IfFailed(spPopup.As(&spPopupAsDO));

                    DumpToXmlWorker(spPopupAsDO, spVisualTreeHelperStatics, spXmlWriter, true);
                }

                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </PopupRoot>
            }
        }
    }

    void VisualTreeDumper::DumpToXmlWorker(
        _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
        _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
        _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
        _In_ bool dumpPopupChildAsChildren
        )
    {
        wrl::ComPtr<IInspectable> spRootAsInsp;
        wrl::Wrappers::HString strRuntimeClassName;

        std::shared_ptr<std::map<std::wstring, std::wstring>> spPropertyValuesMap;

        LogThrow_IfFailed(spRoot.As(&spRootAsInsp));
        LogThrow_IfFailed(spRootAsInsp->GetRuntimeClassName(strRuntimeClassName.ReleaseAndGetAddressOf()));

        // Query the property values for this element.
        WindowHelper::GetTestHooks()->GetDependencyObjectPropertyValues(
            spRoot.Get(),
            true /* excludeDefaultPropertyValues */,
            &VisualTreeDumper::InspectableValueToString,
            spPropertyValuesMap);

        // Dump values for this element.
        LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Element", nullptr /*pwszNamespaceUri */));
        LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Type", nullptr /* pwszNamespaceUri */, strRuntimeClassName.GetRawBuffer(nullptr)));

        if (spPropertyValuesMap)
        {
            for (auto& iter : *spPropertyValuesMap)
            {
                // special case for Transform
                wrl::ComPtr<xaml::IUIElement> uiElement;
                spRoot.As(&uiElement);
                wrl::ComPtr<xaml_media::ITransform> transform;
                if (uiElement && (iter.first.compare(L"RenderTransform") == 0) )  //iter comparison necessary to remove noise
                {
                    LogThrow_IfFailed(uiElement->get_RenderTransform(&transform));  // Only doing for UI Element's RenderTransform
                }

                if (transform)  //Only doing for RenderTransform as of now for the sake of minimal complexity
                {
                    std::wstring outStr;
                    ProcessTransform(transform, spXmlWriter, iter.first, false /*partOfTransformGroup*/, outStr);
                }
                else
                {
                    // Write out the property name/value pair.
                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Property", nullptr /*pwszNamespaceUri */));
                    LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Name", nullptr /* pwszNamespaceUri */, iter.first.data()));
                    LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Value", nullptr /* pwszNamespaceUri */, iter.second.data()));
                    LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Property>
                }
            }
        }

        // Dump the child XAML elements.

        wrl::ComPtr<xaml_primitives::IPopup> spPopup;

        if (dumpPopupChildAsChildren && SUCCEEDED(spRoot.As(&spPopup)))
        {
            wrl::ComPtr<xaml::IUIElement> spPopupChild;

            LogThrow_IfFailed(spPopup->get_Child(&spPopupChild));

            if (spPopupChild)
            {
                wrl::ComPtr<xaml::IDependencyObject> spPopupChildAsDO;
                LogThrow_IfFailed(spPopupChild.As(&spPopupChildAsDO));
                DumpToXmlWorker(spPopupChildAsDO, spVisualTreeHelperStatics, spXmlWriter, dumpPopupChildAsChildren);
            }

            // Dump the overlay element as a child of the popup, if it has one.
            wrl::ComPtr<xaml::IFrameworkElement> popupOverlayElement;
            LogThrow_IfFailed(WindowHelper::GetTestHooks()->GetPopupOverlayElement(spPopup.Get(), &popupOverlayElement));
            if (popupOverlayElement)
            {
                wrl::ComPtr<xaml::IDependencyObject> popupOverlayElementAsDO;
                LogThrow_IfFailed(popupOverlayElement.As(&popupOverlayElementAsDO));
                DumpToXmlWorker(popupOverlayElementAsDO, spVisualTreeHelperStatics, spXmlWriter, dumpPopupChildAsChildren);
            }
        }
        else
        {
            //process children
            ProcessChildren(spRoot, spVisualTreeHelperStatics, spXmlWriter, false /* old format */, dumpPopupChildAsChildren );
        }

        LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Element>
    }

    void VisualTreeDumper::DumpToXmlOldFormat(_In_opt_ xaml::IDependencyObject* pRoot, _In_ HSTRING filenameWithPath)
    {
        wrl::ComPtr<xaml::IDependencyObject> spRoot(pRoot);
        wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVisualTreeHelperStatics;
        wrl::ComPtr<IXmlWriter> spXmlWriter;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
            &spVisualTreeHelperStatics)
            );

        CreateAndInitializeXmlWriter(filenameWithPath, &spXmlWriter);

        LogThrow_IfFailed(spXmlWriter->WriteStartDocument(XmlStandalone_Omit));
        LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"VisualTreeDump", nullptr /*pwszNamespaceUri */));
        RunOnUIThread([&]()
        {
            wrl::ComPtr<wfc::IVectorView<xaml_primitives::Popup*>> spOpenPopupsVectorView;
            unsigned int popupCount = 0;

            // If no root was specified, default to the window content as the root.
            if (!spRoot)
            {
                wrl::ComPtr<xaml::IUIElement> spWindowContent;

                wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
                wrl::ComPtr<test_infra::IWindowHelper> windowHelper;
                LogThrow_IfFailed(wf::GetActivationFactory(
                    wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
                    &testServicesStatics
                ));
                LogThrow_IfFailed(testServicesStatics->get_WindowHelper(&windowHelper));
                LogThrow_IfFailed(windowHelper->get_WindowContent(&spWindowContent));

                // If we don't have Window content, then we'll just dump the open popups.
                if (spWindowContent)
                {
                    LogThrow_IfFailed(spWindowContent.As(&spRoot));
                }
            }


            if (spRoot)
            {
                wrl::ComPtr<xaml::IUIElement> spRootAsUIE;
                spRoot.As(&spRootAsUIE);
                if (spRootAsUIE)
                {
                    wrl::ComPtr<xaml::IXamlRoot> spXamlRoot;
                    LogThrow_IfFailed(spRootAsUIE->get_XamlRoot(&spXamlRoot));
                    if (spXamlRoot)
                    {
                        WindowHelper::GetOpenPopupsInXamlRootStatic(spXamlRoot.Get(), &spOpenPopupsVectorView);
                        LogThrow_IfFailed(spOpenPopupsVectorView->get_Size(&popupCount));
                    }
                }
            }

            // Dump the visual elements.
            if (spRoot)
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"VisualElements", nullptr /*pwszNamespaceUri */));
                DumpToXmlWorkerOldFormat(spRoot, spVisualTreeHelperStatics, spXmlWriter);
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </VisualElements>
            }

            // Dump the open popups.
            if (popupCount > 0)
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"PopupElements", nullptr /*pwszNamespaceUri */));
                for (unsigned int popupIndex = 0; popupIndex < popupCount; ++popupIndex)
                {
                    wrl::ComPtr<xaml_primitives::IPopup> spPopup;
                    wrl::ComPtr<xaml::IDependencyObject> spPopupAsDO;
                    wrl::ComPtr<xaml::IUIElement> spPopupChild;
                    wrl::ComPtr<xaml::IDependencyObject> spPopupChildAsDO;

                    LogThrow_IfFailed(spOpenPopupsVectorView->GetAt(popupIndex, &spPopup));
                    LogThrow_IfFailed(spPopup.As(&spPopupAsDO));

                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"PopupElement", nullptr /*pwszNamespaceUri */));
                    DumpToXmlWorkerOldFormat(spPopupAsDO, spVisualTreeHelperStatics, spXmlWriter);

                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"PopupElement.Child", nullptr /*pwszNamespaceUri */));
                    LogThrow_IfFailed(spPopup->get_Child(&spPopupChild));
                    LogThrow_IfFailed(spPopupChild.As(&spPopupChildAsDO));
                    DumpToXmlWorkerOldFormat(spPopupChildAsDO, spVisualTreeHelperStatics, spXmlWriter);
                    LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </PopupElement.Child>

                    LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </PopupElement>
                }
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </PopupElements>
            }
        });
        LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </VisualTreeDump>
        LogThrow_IfFailed(spXmlWriter->WriteEndDocument());
        LogThrow_IfFailed(spXmlWriter->Flush());
    }

    void VisualTreeDumper::DumpToXmlWorkerOldFormat(
        _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
        _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
        _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter
        )
    {
        wrl::ComPtr<IInspectable> spRootAsInsp;
        wrl::Wrappers::HString strRuntimeClassName;

        std::shared_ptr<std::map<std::wstring, std::wstring>> spPropertyValuesMap;

        LogThrow_IfFailed(spRoot.As(&spRootAsInsp));
        LogThrow_IfFailed(spRootAsInsp->GetRuntimeClassName(strRuntimeClassName.ReleaseAndGetAddressOf()));

        // Query the property values for this element.
        WindowHelper::GetTestHooks()->GetDependencyObjectPropertyValues(
            spRoot.Get(),
            true /* excludeDefaultPropertyValues */,
            &VisualTreeDumper::InspectableValueToString,
            spPropertyValuesMap);

        // Dump values for this element.
        LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Element", nullptr /*pwszNamespaceUri */));
        LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Type", nullptr /* pwszNamespaceUri */, strRuntimeClassName.GetRawBuffer(nullptr)));

        if (spPropertyValuesMap && spPropertyValuesMap->size() > 0)
        {
            bool added = false;

            for (auto& iter : *spPropertyValuesMap)
            {
                if (iter.first.compare(L"ActualHeight") == 0 ||
                    iter.first.compare(L"ActualWidth") == 0 ||
                    iter.first.compare(L"RenderSize") == 0 ||
                    iter.first.compare(L"Name") == 0)
                {
                    continue;
                }

                if (!added)
                {
                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Properties", nullptr /*pwszNamespaceUri */));
                    added = true;
                }

                // special case for Transform
                wrl::ComPtr<xaml::IUIElement> uiElement;
                spRoot.As(&uiElement);
                wrl::ComPtr<xaml_media::ITransform> transform;
                if (uiElement && (iter.first.compare(L"RenderTransform") == 0) )  //iter comparison necessary to remove noise
                {
                    LogThrow_IfFailed(uiElement->get_RenderTransform(&transform));  // Only doing for UI Element's RenderTransform
                }

                if (transform)  //Only doing for RenderTransform as of now for the sake of minimal complexity
                {
                    std::wstring outStr;
                    ProcessTransform(transform, spXmlWriter, iter.first, false/*partOfTransformGroup*/, outStr);
                }
                else
                {
                    // Write out the property name/value pair.
                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Property", nullptr /*pwszNamespaceUri */));
                    LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Name", nullptr /* pwszNamespaceUri */, iter.first.data()));
                    LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Value", nullptr /* pwszNamespaceUri */, iter.second.data()));
                    LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Property>
                }
            }

            if (added)
            {
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Properties>
            }
        }
        
        //process children
        ProcessChildren(spRoot, spVisualTreeHelperStatics, spXmlWriter, true /* old format */);
        LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Element>
    }

    void VisualTreeDumper::CreateAndInitializeXmlWriter(_In_ HSTRING filenameWithPath, _Outptr_ IXmlWriter** ppXmlWriter)
    {
        wrl::ComPtr<IXmlWriter> spXmlWriter;
        wrl::ComPtr<IStream> spOutputStream;

        LogThrow_IfFailed(SHCreateStreamOnFileEx(
            ::WindowsGetStringRawBuffer(filenameWithPath, nullptr),
            STGM_CREATE | STGM_WRITE,
            FILE_ATTRIBUTE_NORMAL,
            TRUE,
            nullptr /* pstmTemplate - reserved */,
            &spOutputStream)
            );

        LogThrow_IfFailed(CreateXmlWriter(__uuidof(IXmlWriter), &spXmlWriter, nullptr /* pMalloc */));
        LogThrow_IfFailed(spXmlWriter->SetOutput(spOutputStream.Get()));
        LogThrow_IfFailed(spXmlWriter->SetProperty(XmlWriterProperty_Indent, TRUE));

        LogThrow_IfFailed(spXmlWriter.CopyTo(ppXmlWriter));
    }

    HRESULT VisualTreeDumper::InspectableValueToString(_In_ IInspectable* pValue, _Out_ HSTRING* phstrValue)
    {
        COM_START
        {
            wrl::ComPtr<IInspectable> spValue(pValue);

            if (spValue)
            {
                wrl::ComPtr<wf::IPropertyValue> spPropertyValue;
                wrl::ComPtr<xaml_media::IFontFamily> spFontFamily;
                wrl::ComPtr<xaml_media::ISolidColorBrush> spSolidColorBrush;
                wrl::ComPtr<xaml_media::IBrush> spBrush;

                if (SUCCEEDED(spValue.As(&spPropertyValue)))
                {
                    wf::PropertyType valueType = wf::PropertyType_Empty;

                    if (SUCCEEDED(spPropertyValue->get_Type(&valueType)))
                    {
                        if (valueType == wf::PropertyType_String)
                        {
                            if (SUCCEEDED(spPropertyValue->GetString(phstrValue)))
                            {
                                if (*phstrValue == nullptr)
                                {
                                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(L"NULL").CopyTo(phstrValue));
                                }
                            }
                        }
                        else
                        {
                            PropertyValueToString(spPropertyValue.Get(), valueType, phstrValue);
                        }
                    }
                }
                else if (SUCCEEDED(spValue.As(&spFontFamily)))
                {
                    LogThrow_IfFailed(spFontFamily->get_Source(phstrValue));
                }
                else if (SUCCEEDED(spValue.As(&spSolidColorBrush)))
                {
                    wu::Color color = {};
                    double opacity;
                    std::array<wchar_t, 50> buffer{};

                    LogThrow_IfFailed(spSolidColorBrush->get_Color(&color));
                    LogThrow_IfFailed(spSolidColorBrush.As(&spBrush));
                    LogThrow_IfFailed(spBrush->get_Opacity(&opacity));

                    if (opacity < 1.0)
                    {
                        Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"#%02X%02X%02X%02X(Opacity=%.2f)", color.A, color.R, color.G, color.B, opacity) > 0, E_FAIL);
                    }
                    else
                    {
                        Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"#%02X%02X%02X%02X", color.A, color.R, color.G, color.B) > 0, E_FAIL);
                    }
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(buffer.data()).CopyTo(phstrValue));
                }
                else
                {
                    wrl::Wrappers::HString strValue;
                    LogThrow_IfFailed(spValue->GetRuntimeClassName(strValue.GetAddressOf()));

                    // Value: "[RUNTIMECLASSNAME]"
                    auto wstrValue = std::wstring(L"[") + strValue.GetRawBuffer(nullptr) + std::wstring(L"]");
                    strValue.Set(wstrValue.c_str());

                    LogThrow_IfFailed(strValue.CopyTo(phstrValue));
                }
            }
            else
            {
                LogThrow_IfFailed(wrl::Wrappers::HStringReference(L"[NULL]").CopyTo(phstrValue));
            }

            if (*phstrValue == nullptr)
            {
                LogThrow_IfFailed(wrl::Wrappers::HStringReference(L"???").CopyTo(phstrValue));
            }
        }
        COM_END
    }

    void VisualTreeDumper::PropertyValueToString(_In_ wf::IPropertyValue* pPropertyValue, wf::PropertyType propertyType, _Out_ HSTRING* phstrValue)
    {
        std::array<wchar_t, 128> buffer{};
        bool createString = true;

        wrl::ComPtr<wf::IPropertyValue> spPropertyValue(pPropertyValue);

        switch (propertyType)
        {
            case wf::PropertyType_UInt8:
            {
                BYTE value = 0;
                LogThrow_IfFailed(pPropertyValue->GetUInt8(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%u", static_cast<UINT32>(value)) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Int16:
            {
                INT16 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetInt16(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%hd", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_UInt16:
            {
                UINT16 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetUInt16(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%hu", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Int32:
            {
                INT32 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetInt32(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I32d", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_UInt32:
            {
                UINT32 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetUInt32(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I32u", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Int64:
            {
                INT64 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetInt64(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I64d", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_UInt64:
            {
                UINT64 value = 0;
                LogThrow_IfFailed(pPropertyValue->GetUInt64(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I64u", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Single:
            {
                FLOAT value = 0;
                LogThrow_IfFailed(pPropertyValue->GetSingle(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Double:
            {
                DOUBLE value = 0;
                LogThrow_IfFailed(pPropertyValue->GetDouble(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%lg", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Char16:
            {
                WCHAR value = 0;
                LogThrow_IfFailed(pPropertyValue->GetChar16(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%c", value) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Boolean:
            {
                BOOLEAN value = FALSE;
                LogThrow_IfFailed(pPropertyValue->GetBoolean(&value));
                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), (!!value ? L"True" : L"False")) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_String:
            {
                LogThrow_IfFailed(pPropertyValue->GetString(phstrValue));
                createString = false;
                break;
            }

            case wf::PropertyType_Guid:
            {
                GUID value;
                LogThrow_IfFailed(pPropertyValue->GetGuid(&value));
                GuidToString(value, phstrValue);
                createString = false;
                break;
            }

            case wf::PropertyType_Point:
            {
                wrl::ComPtr<wf::IReference<wf::Point>> spRefPoint;
                wf::Point point = {};

                LogThrow_IfFailed(spPropertyValue.As(&spRefPoint));
                LogThrow_IfFailed(spRefPoint->get_Value(&point));

                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g,%g", point.X, point.Y) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Size:
            {
                wrl::ComPtr<wf::IReference<wf::Size>> spRefSize;
                wf::Size size = {};

                LogThrow_IfFailed(spPropertyValue.As(&spRefSize));
                LogThrow_IfFailed(spRefSize->get_Value(&size));

                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%gx%g", size.Width, size.Height) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_Rect:
            {
                wrl::ComPtr<wf::IReference<wf::Rect>> spRefRect;
                wf::Rect rect = {};

                LogThrow_IfFailed(spPropertyValue.As(&spRefRect));
                LogThrow_IfFailed(spRefRect->get_Value(&rect));

                Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g,%g %gx%g", rect.X, rect.Y, rect.Width, rect.Height) > 0, E_FAIL);
                break;
            }

            case wf::PropertyType_OtherType:
            {
                // Try to get value strings for common ref types.
                wrl::ComPtr<wf::IReference<xaml::Visibility>> spRefVisibility;
                wrl::ComPtr<wf::IReference<xaml::Thickness>> spRefThickness;
                wrl::ComPtr<wf::IReference<xaml::CornerRadius>> spRefCornerRadius;
                wrl::ComPtr<wf::IReference<xaml_controls::Orientation>> spRefOrientation;
                wrl::ComPtr<wf::IReference<xaml_controls::ScrollBarVisibility>> spRefScrollBarVisibility;
                wrl::ComPtr<wf::IReference<xaml_controls::SnapPointsType>> spRefSnapPointsType;
                wrl::ComPtr<wf::IReference<xaml_controls::ScrollMode>> spRefScrollMode;
                wrl::ComPtr<wf::IReference<xaml_controls::ZoomMode>> spRefZoomMode;
                wrl::ComPtr<wf::IReference<xaml_controls::ClickMode>> spRefClickMode;
                wrl::ComPtr<wf::IReference<wut::FontWeight>> spRefFontWeight;
                wrl::ComPtr<wf::IReference<wf::TimeSpan>> spRefTimeSpan;
                wrl::ComPtr<wf::IReference<wf::DateTime>> spRefDateTime;
                wrl::ComPtr<wf::IReference<xaml::GridLength>> spGridLength;

                if (SUCCEEDED(spPropertyValue.As(&spRefVisibility)))
                {
                    xaml::Visibility visibility = xaml::Visibility_Visible;

                    LogThrow_IfFailed(spRefVisibility->get_Value(&visibility));
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(visibility == xaml::Visibility_Visible ? L"Visible" : L"Collapsed").CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefThickness)))
                {
                    xaml::Thickness thickness = {};

                    LogThrow_IfFailed(spRefThickness->get_Value(&thickness));
                    Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g,%g,%g,%g", thickness.Left, thickness.Top, thickness.Right, thickness.Bottom) > 0, E_FAIL);
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefCornerRadius)))
                {
                    xaml::CornerRadius cornerRadius = {};

                    LogThrow_IfFailed(spRefCornerRadius->get_Value(&cornerRadius));
                    Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g,%g,%g,%g", cornerRadius.TopLeft, cornerRadius.TopRight, cornerRadius.BottomRight, cornerRadius.BottomLeft) > 0, E_FAIL);
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefOrientation)))
                {
                    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

                    LogThrow_IfFailed(spRefOrientation->get_Value(&orientation));
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(orientation == xaml_controls::Orientation_Vertical ? L"Vertical" : L"Horizontal").CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefScrollBarVisibility)))
                {
                    xaml_controls::ScrollBarVisibility visibility = xaml_controls::ScrollBarVisibility_Disabled;

                    LogThrow_IfFailed(spRefScrollBarVisibility->get_Value(&visibility));

                    const wchar_t* visibilityStringArray[] = {
                        L"Disabled",
                        L"Auto",
                        L"Hidden",
                        L"Visible"
                    };

                    Throw::IfFalse(static_cast<unsigned int>(visibility) < ARRAYSIZE(visibilityStringArray), E_UNEXPECTED);
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(visibilityStringArray[static_cast<unsigned int>(visibility)]).CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefSnapPointsType)))
                {
                    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;

                    LogThrow_IfFailed(spRefSnapPointsType->get_Value(&snapPointsType));

                    const wchar_t* snapPointsTypeStringArray[] = {
                        L"None",
                        L"Optional",
                        L"Mandatory",
                        L"OptionalSingle",
                        L"MandatorySingle"
                    };

                    Throw::IfFalse(static_cast<unsigned int>(snapPointsType) < ARRAYSIZE(snapPointsTypeStringArray), E_UNEXPECTED);
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(snapPointsTypeStringArray[static_cast<unsigned int>(snapPointsType)]).CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefScrollMode)))
                {
                    xaml_controls::ScrollMode scrollMode = xaml_controls::ScrollMode_Disabled;

                    LogThrow_IfFailed(spRefScrollMode->get_Value(&scrollMode));

                    const wchar_t* scrollModeStringArray[] = {
                        L"Disabled",
                        L"Enabled",
                        L"Auto"
                    };

                    Throw::IfFalse(static_cast<unsigned int>(scrollMode) < ARRAYSIZE(scrollModeStringArray), E_UNEXPECTED);
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(scrollModeStringArray[static_cast<unsigned int>(scrollMode)]).CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefZoomMode)))
                {
                    xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;

                    LogThrow_IfFailed(spRefZoomMode->get_Value(&zoomMode));
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(zoomMode == xaml_controls::ZoomMode_Disabled ? L"Disabled" : L"Enabled").CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefClickMode)))
                {
                    xaml_controls::ClickMode clickMode = xaml_controls::ClickMode_Release;

                    LogThrow_IfFailed(spRefClickMode->get_Value(&clickMode));

                    const wchar_t* clickModeStringArray[] = {
                        L"Release",
                        L"Press",
                        L"Hover"
                    };

                    Throw::IfFalse(static_cast<unsigned int>(clickMode) < ARRAYSIZE(clickModeStringArray), E_UNEXPECTED);
                    LogThrow_IfFailed(wrl::Wrappers::HStringReference(clickModeStringArray[static_cast<unsigned int>(clickMode)]).CopyTo(phstrValue));
                    createString = false;
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefFontWeight)))
                {
                    wut::FontWeight fontWeight = {};

                    LogThrow_IfFailed(spRefFontWeight->get_Value(&fontWeight));
                    Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%d", fontWeight.Weight) > 0, E_FAIL);
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefTimeSpan)))
                {
                    wf::TimeSpan timespan = {};

                    LogThrow_IfFailed(spRefTimeSpan->get_Value(&timespan));
                    Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I64d", timespan.Duration) > 0, E_FAIL);
                }
                else if (SUCCEEDED(spPropertyValue.As(&spRefDateTime)))
                {
                    wf::DateTime datetime = {};

                    LogThrow_IfFailed(spRefDateTime->get_Value(&datetime));
                    Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%I64d", datetime.UniversalTime) > 0, E_FAIL);
                }
                else if (SUCCEEDED(spPropertyValue.As(&spGridLength)))
                {
                    xaml::GridLength gridLength = {};

                    LogThrow_IfFailed(spGridLength->get_Value(&gridLength));

                    switch (gridLength.GridUnitType)
                    {
                    case xaml::GridUnitType_Auto:
                        Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"Auto") > 0, E_FAIL);
                        break;

                    case xaml::GridUnitType_Pixel:
                        Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g", gridLength.Value) > 0, E_FAIL);
                        break;

                    case xaml::GridUnitType_Star:
                        Throw::IfFalse(swprintf_s(buffer.data(), buffer.size(), L"%g*", gridLength.Value) > 0, E_FAIL);
                        break;
                    }
                }
                else
                {
                    wrl::Wrappers::HString strValue;
                    LogThrow_IfFailed(pPropertyValue->GetRuntimeClassName(strValue.GetAddressOf()));

                    // Value: "[RUNTIMECLASSNAME]"
                    auto wstrValue = std::wstring(L"[") + strValue.GetRawBuffer(nullptr) + std::wstring(L"]");
                    strValue.Set(wstrValue.c_str());
                    LogThrow_IfFailed(strValue.CopyTo(phstrValue));
                    createString = false;
                }
                break;
            }
        }

        if (createString)
        {
            LogThrow_IfFailed(wrl::Wrappers::HStringReference(buffer.data()).CopyTo(phstrValue));
        }
    }

    void VisualTreeDumper::GuidToString(GUID guid, _Out_ HSTRING* phstrValue)
    {
        HRESULT hr = S_OK;
        LPOLESTR pGuidString = nullptr;

        if (SUCCEEDED(StringFromCLSID(guid, &pGuidString)))
        {
            hr = wrl::Wrappers::HStringReference(pGuidString).CopyTo(phstrValue);
            CoTaskMemFree(pGuidString);
            LogThrow_IfFailed(hr);
        }
    }
    void VisualTreeDumper::ProcessChildren(
        _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
        _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
        _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
        _In_ const bool oldFormat,
        _In_ const bool dumpPopupChildAsChildren
        )
    {
        wrl::ComPtr<xaml_controls::ITextBlock> textBlock;
        spRoot.As(&textBlock);
        // special case for Transform
        wrl::ComPtr<xaml_docs::ISpan> spanElement;
        spRoot.As(&spanElement);
        wrl::ComPtr<xaml_docs::IParagraph> paragraphElement;
        spRoot.As(&paragraphElement);
        // special case : treat textblock/span/paragraph differently because of inlines
        if(textBlock || spanElement || paragraphElement)
        {
            wrl::ComPtr<wfc::IVector<xaml_docs::Inline*>> inlines;
            if(textBlock)
            {
                LogThrow_IfFailed(textBlock->get_Inlines(&inlines));
            }
            else if(spanElement)
            {
                LogThrow_IfFailed(spanElement->get_Inlines(&inlines));
            }
            else if(paragraphElement)
            {
                LogThrow_IfFailed(paragraphElement->get_Inlines(&inlines));
            }
            
            UINT childCount = 0;
            LogThrow_IfFailed(inlines->get_Size(&childCount));
            if(childCount > 0)
            {
                if(oldFormat)
                {
                    LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"ChildElements", nullptr /*pwszNamespaceUri */));
                }
                for (UINT i = 0; i < childCount; i++)
                {
                    wrl::ComPtr<xaml_docs::IInline> inlineElement = nullptr;
                    LogThrow_IfFailed(inlines->GetAt(i, &inlineElement));
                    wrl::ComPtr<xaml::IDependencyObject> inlineElementDO;
                    LogThrow_IfFailed(inlineElement.As(&inlineElementDO));
                    if(oldFormat)
                    {
                        DumpToXmlWorkerOldFormat(inlineElementDO, spVisualTreeHelperStatics, spXmlWriter);
                    }
                    else
                    {
                        DumpToXmlWorker(inlineElementDO, spVisualTreeHelperStatics, spXmlWriter, dumpPopupChildAsChildren);
                    }
                }

                if(oldFormat)
                {
                    LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </ChildElements>
                }
            }
        }

        int childCount = 0;

        wrl::ComPtr<xaml::IUIElement> spRootAsUIE;
        spRoot.As(&spRootAsUIE);
        if (spRootAsUIE)
        {
            // GetChildrenCount takes in a DependencyObject parameter, but actually expects it to be a UIElement.
            spVisualTreeHelperStatics->GetChildrenCount(spRoot.Get(), &childCount);
        }

        if (childCount > 0)
        {
            if(oldFormat)
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"ChildElements", nullptr /*pwszNamespaceUri */));
            }
            for (int childIndex = 0; childIndex < childCount; ++childIndex)
            {
                wrl::ComPtr<xaml::IDependencyObject> spChild;
                LogThrow_IfFailed(spVisualTreeHelperStatics->GetChild(spRoot.Get(), childIndex, &spChild));
                if(oldFormat)
                {
                    DumpToXmlWorkerOldFormat(spChild, spVisualTreeHelperStatics, spXmlWriter);
                }
                else
                {
                    DumpToXmlWorker(spChild, spVisualTreeHelperStatics, spXmlWriter, dumpPopupChildAsChildren);
                }
            }

            if(oldFormat)
            {
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </ChildElements>
            }
        }
    }

    void VisualTreeDumper::ProcessTransform(
        _In_ const wrl::ComPtr<xaml_media::ITransform>& tranform,
        _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
        _In_ const std::wstring& name,
        _In_ const bool partOfTransformGroup,
        _Out_ std::wstring& outString
        )
    {
        // if this transform is transform group, then process all of its children transforms and add to parent transformgroup's output
        wrl::ComPtr<xaml_media::ITransformGroup> transformGroup;
        tranform.As(&transformGroup);
        if(transformGroup)
        {
            ProcessTransformGroup(transformGroup, spXmlWriter, name, partOfTransformGroup, outString);
        }
        else
        {   // when transform is not a transform group, make outString or print to xml its values
            wrl::ComPtr<IInspectable> tranformAsInsp;
            wrl::Wrappers::HString strRuntimeClassName;
            LogThrow_IfFailed(tranform.As(&tranformAsInsp));
            LogThrow_IfFailed(tranformAsInsp->GetRuntimeClassName(strRuntimeClassName.ReleaseAndGetAddressOf()));
            
            wrl::ComPtr<xaml::IDependencyObject> transformElementDO;
            LogThrow_IfFailed(tranform.As(&transformElementDO));
            std::shared_ptr<std::map<std::wstring, std::wstring>> transformPropertyValuesMap;
            // Query the property values for this element.
            WindowHelper::GetTestHooks()->GetDependencyObjectPropertyValues(
                transformElementDO.Get(),
                true /* excludeDefaultPropertyValues */,
                &VisualTreeDumper::InspectableValueToString,
                transformPropertyValuesMap);

            std::wstring strValue;
            if ( transformPropertyValuesMap->size() > 0 )
            {
                for (auto& iter : *transformPropertyValuesMap)
                {
                    
                    std::wstring processedValue;
                    // flooring numerical values to integer for properties X and Y
                    if( (iter.first.compare(L"X") == 0) || (iter.first.compare(L"Y") == 0) )
                    {
                           // convert wstring to wchar_t and use in _wtof and round to nearest integer
                           double convertedValue = _wtof(iter.second.c_str());
                           if( convertedValue == 0.0L ) // if the value was NaN
                           {
                                processedValue = iter.second;
                           }
                           else
                           {
                                int intVal = (int)round(convertedValue); 
                                processedValue = std::to_wstring(intVal);
                           }
                    }
                    else
                    {
                        processedValue = iter.second;
                    }

                    strValue = strValue + L" " + iter.first + L"=" + processedValue + L" "; 
                }
            }
            
            // find the class name of transform
            std::wstring longClassName = strRuntimeClassName.GetRawBuffer(nullptr);
            
            // shorten the class name of transform
            std::wstringstream ss(longClassName);
            std::wstring token;
            while (std::getline(ss, token, L'.')); //take the last block after token A.B.C.D returns D
            std::wstring shortClassName = token;
            std::wstring finalStr = shortClassName + L"(" + strValue + L")";
            
            //only build and send string if it is called from another transform group else just print xml to the file
            if(partOfTransformGroup)
            {
                outString = finalStr;
            }
            else
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Property", nullptr /*pwszNamespaceUri */));
                LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Name", nullptr /* pwszNamespaceUri */, name.data()));
                LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Value", nullptr /* pwszNamespaceUri */, finalStr.data()));
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Property>
            }
        }
    }

    void VisualTreeDumper::ProcessTransformGroup(
    _In_ const wrl::ComPtr<xaml_media::ITransformGroup>& transformGroup,
    _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
    _In_ const std::wstring& name,
    _In_ const bool partOfTransformGroup,
    _Out_ std::wstring& outString
    )
    {
        wrl::ComPtr<wfc::IVector<xaml_media::Transform*>> transforms;
        LogThrow_IfFailed(transformGroup->get_Children(&transforms));
        UINT childCount = 0;
        LogThrow_IfFailed(transforms->get_Size(&childCount));

        if(childCount > 0)
        {
            std::wstring finalStr = L"TransformGroup(";
            for (UINT i = 0; i < childCount; i++)
            {
                wrl::ComPtr<xaml_media::ITransform> transformElement = nullptr;
                LogThrow_IfFailed(transforms->GetAt(i, &transformElement));
                wrl::ComPtr<xaml::IDependencyObject> transformElementDO;
                LogThrow_IfFailed(transformElement.As(&transformElementDO));
                
                std::wstring outputStr;
                ProcessTransform(transformElement, spXmlWriter, L""/*name*/, true /*partOfTransformGroup*/, outputStr);
                finalStr = finalStr + outputStr + L" ";
            }

            finalStr = finalStr + L")";
            
            //only build and send string if it is called from another transform group else just print xml to the file
            if(partOfTransformGroup)
            {
                outString = finalStr;
            }
            else
            {
                LogThrow_IfFailed(spXmlWriter->WriteStartElement(nullptr /* pwszPrefix */, L"Property", nullptr /*pwszNamespaceUri */));
                LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Name", nullptr /* pwszNamespaceUri */, name.data()));
                LogThrow_IfFailed(spXmlWriter->WriteAttributeString(nullptr /* pwszPrefix */, L"Value", nullptr /* pwszNamespaceUri */, finalStr.data()));
                LogThrow_IfFailed(spXmlWriter->WriteEndElement()); // </Property>
            }
                
        }
    }

} } // namespace Private::Infrastructure
