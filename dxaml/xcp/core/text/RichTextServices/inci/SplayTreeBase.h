// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    namespace Internal
    {
        class SplayTreeCollection;

        //---------------------------------------------------------------------------
        //
        //  SplayTreeBase
        //
        //  A parent object of a SplayTreeNode, either a SplayTreeCollection or a
        //  SplayTreeNode.
        //
        //                               SplayTreeBase
        //                                 /       \
        //                SplayTreeCollection    SplayTreeNode
        //
        //---------------------------------------------------------------------------
        class SplayTreeBase
        {
        public:

            // Tree index for the character dimension -- CharacterRunNode derived classes use this.
            static const XUINT32 CharTreeIndex = 0;

            // Tree index for the u pixel dimension, LayoutNodeImpl derivied classes use this.
            static const XUINT32 UTreeIndex    = 1;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      AsSplayTreeCollection
            //
            //  Returns:
            //      This object as a SplayTreeCollection, or NULL if this object is not
            //      a SplayTreeCollection.
            //
            //  Notes:
            //      If this method returns NULL, the object must be a SplayTreeNode.
            //      However, a SplayTreeNode may also be a SplayTreeCollection.  The two
            //      classes are not mutually exclusive.
            //
            //---------------------------------------------------------------------------
            virtual SplayTreeCollection *AsSplayTreeCollection() = 0;
        };
    }
}
