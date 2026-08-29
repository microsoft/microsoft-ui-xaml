// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CharacterRunCollection.h"

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  CountedCharacterRunCollection
        //
        //  Holds a collection of CharacterRunNodes and tracks the total character
        //  count contained.
        //
        //---------------------------------------------------------------------------
        class CountedCharacterRunCollection : public CharacterRunCollection
        {
        public:

            // Initializes a new CountedCharacterRunCollection instance.
            CountedCharacterRunCollection();

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      CountedCharacterRunCollection::GetCount
            //
            //  Returns:
            //      The character count of this collection.
            //
            //---------------------------------------------------------------------------
            virtual XUINT32 GetCount() const = 0;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      CountedCharacterRunCollection::SetCount
            //
            //  Synopsis:
            //      Sets the character count of this collection.
            //
            //---------------------------------------------------------------------------
            virtual void SetCount(_In_ XUINT32 count) = 0;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      CountedCharacterRunCollection::AdjustCount
            //
            //  Synopsis:
            //      Modifies the character count of this collection.
            //
            //---------------------------------------------------------------------------
            virtual void AdjustCount(
                _In_ XINT32 delta
                    // Character count to add to existing value, negative values are legal.
                ) = 0;

            // Modifies the character count of this CountedCharacterRunCollection and any containing
            // CountedCharacterRunCollections.
            void AdjustCollectionCounts(_In_ XINT32 delta);

            // Attaches a tree of CharacterRunNodes to a CountedCharacterRunCollection.
            static void Connect(
                _In_opt_ CountedCharacterRunCollection *pContainingCollection, 
                _In_opt_ CharacterRunNode *pRootNode, 
                _In_ XINT32 delta
                    // Change in character count value, negative values are legal.
                );
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CountedCharacterRunCollection::CountedCharacterRunCollection
        //
        //  Synopsis:
        //      Initializes a new CountedCharacterRunCollection instance.
        //
        //---------------------------------------------------------------------------
        inline 
        CountedCharacterRunCollection::CountedCharacterRunCollection()
            : CharacterRunCollection()
        {
        }
    }
}
