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


#define __TextHighlighterCollection_GUID "924121f4-cb70-4966-9748-6b6a90d47a99"

#pragma region forwarders
namespace ctl
{
    
}
#pragma endregion

namespace DirectUI
{
    class TextHighlighterCollection;

    class __declspec(novtable) TextHighlighterCollectionGenerated:
        public DirectUI::PresentationFrameworkCollection<ABI::Microsoft::UI::Xaml::Documents::TextHighlighter*>
    {
        friend class DirectUI::TextHighlighterCollection;



    public:
        TextHighlighterCollectionGenerated();
        ~TextHighlighterCollectionGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TextHighlighterCollection;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::TextHighlighterCollection;
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

#include "TextHighlighterCollection_Partial.h"

