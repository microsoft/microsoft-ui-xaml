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


#define __ManipulationVelocities_GUID "da49c6f7-6615-464f-8c47-539e5f26bac1"

namespace DirectUI
{
    class ManipulationVelocities;

    class __declspec(novtable) __declspec(uuid(__ManipulationVelocities_GUID)) ManipulationVelocities:
        public DirectUI::DependencyObject
    {



    public:
        ManipulationVelocities();
        ~ManipulationVelocities() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ManipulationVelocities;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ManipulationVelocities;
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


