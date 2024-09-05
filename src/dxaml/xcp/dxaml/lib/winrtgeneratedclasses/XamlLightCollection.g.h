// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once


#define __XamlLightCollection_GUID "deb37c56-5ccd-40a5-baa6-5c44f179fdb1"

namespace DirectUI
{
    class XamlLightCollection;

    class __declspec(novtable) __declspec(uuid(__XamlLightCollection_GUID)) XamlLightCollection:
        public DirectUI::PresentationFrameworkCollection<ABI::Microsoft::UI::Xaml::Media::XamlLight*>
    {



    public:
        XamlLightCollection();
        ~XamlLightCollection() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::XamlLightCollection;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::XamlLightCollection;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}

