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

#include "GeneralTransform.g.h"

#define __InternalTransform_GUID "2e93250c-816c-4575-9940-611cacd96e0c"

namespace DirectUI
{
    class InternalTransform;

    class __declspec(novtable) InternalTransformGenerated:
        public DirectUI::GeneralTransform
    {
        friend class DirectUI::InternalTransform;



    public:
        InternalTransformGenerated();
        ~InternalTransformGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::InternalTransform;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::InternalTransform;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "InternalTransform_Partial.h"

