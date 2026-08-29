// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Handle.h>
#include <TestEvent.h>

interface IXmlWriter;

namespace Private { namespace Infrastructure {

    class VisualTreeDumper
    {
    public:
        // Dump visual tree in format expected by XmlValidation tool.
        static void DumpToXml(_In_opt_ xaml::IDependencyObject* pRoot, _In_ HSTRING filenameWithPath, bool ignorePopups = false);

        // Dump visual tree in legacy format.  Once all masters are updated to the new format, this method and ones
        // which are exclusively called by it can be removed.
        static void DumpToXmlOldFormat(_In_opt_ xaml::IDependencyObject* pRoot, _In_ HSTRING filenameWithPath);

    private:
        VisualTreeDumper();
        ~VisualTreeDumper();

        static void DumpElement(
            _In_ const wrl::ComPtr<xaml::IDependencyObject>& root,
            _In_ const wrl::ComPtr<xaml::IXamlRoot>& xamlRootForPopups,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter);

        static void DumpToXmlWorker(
            _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
            _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
            _In_ bool dumpPopupChildAsChildren
            );

        static void DumpToXmlWorkerOldFormat(
            _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
            _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter
            );

        static void ProcessChildren(
            _In_ const wrl::ComPtr<xaml::IDependencyObject>& spRoot,
            _In_ const wrl::ComPtr<xaml_media::IVisualTreeHelperStatics>& spVisualTreeHelperStatics,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
            _In_ const bool oldFormat,
            _In_ const bool dumpPopupChildAsChildren = false
            );

        static void ProcessTransform(
            _In_ const wrl::ComPtr<xaml_media::ITransform>& tranform,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
            _In_ const std::wstring& name,
            _In_ const bool partOfTransformGroup,
            _Out_ std::wstring& outString
            );

        static void ProcessTransformGroup(
            _In_ const wrl::ComPtr<xaml_media::ITransformGroup>& tranformGroup,
            _In_ const wrl::ComPtr<IXmlWriter>& spXmlWriter,
            _In_ const std::wstring& name,
            _In_ const bool partOfTransformGroup,
            _Out_ std::wstring& outString
            );
        static void CreateAndInitializeXmlWriter(_In_ HSTRING filenameWithPath, _Outptr_ IXmlWriter** ppXmlWriter);

        // Internal debug interop callback to convert property values to strings.
        static HRESULT InspectableValueToString(_In_ IInspectable* pValue, _Out_ HSTRING* phstrValue);

        // Value to string conversion utilities.
        static void PropertyValueToString(_In_ wf::IPropertyValue* pPropertyValue, wf::PropertyType propertyType, _Out_ HSTRING* phstrValue);
        static void GuidToString(GUID guid, _Out_ HSTRING* phstrValue);

    }; // class VisualTreeDumper

} } // namespace Private::Infrastructure
