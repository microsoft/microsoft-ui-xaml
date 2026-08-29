// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCaretBrowsingCaret final : public CPanel
{
protected:
    CCaretBrowsingCaret(_In_ CCoreServices *core) : CPanel( core )
    {
    }

   ~CCaretBrowsingCaret() override
   {
   }

public:
    DECLARE_CREATE(CCaretBrowsingCaret);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::CaretBrowsingCaret;
    }

};