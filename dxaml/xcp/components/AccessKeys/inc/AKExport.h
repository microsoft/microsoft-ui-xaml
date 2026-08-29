// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <paltypes.h>
#include <EnumDefs.g.h>
#include "ModeContainer.h"

namespace AccessKeys
{
    struct AccessKeyExportImpl;
    class AccessKeyExport;
}

class CCoreSerivces;
class VisualTree;

namespace AccessKeys
{
    class AccessKeyExport
    {
    public:
        AccessKeyExport(_In_ CCoreServices* const core);
        ~AccessKeyExport();

        _Check_return_ HRESULT TryProcessInputForAccessKey(_In_ const InputMessage* const inputMessage, _Out_ bool* keyProcessed) const;
        _Check_return_ HRESULT ProcessPointerInput(_In_ const InputMessage* const inputMessage) const;
        _Check_return_ HRESULT TryProcessInputForCharacterReceived(_In_ mui::ICharacterReceivedEventArgs* args, _Out_ bool* keyProcessed) const;
        _Check_return_ HRESULT UpdateScope() const;
        _Check_return_ HRESULT AddElementToAKMode(_In_ CDependencyObject* const element) const;
        _Check_return_ HRESULT RemoveElementFromAKMode(_In_ CDependencyObject* const element) const;
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ CDependencyObject* const element, bool isEnabled) const;
        _Check_return_ HRESULT OnVisibilityChanged(_In_ CDependencyObject* const element, const DirectUI::Visibility& visibility) const;
        _Check_return_ HRESULT CleanupAndExitCurrentScope() const;
        _Check_return_ HRESULT ExitAccessKeyMode() const;
        _Check_return_ HRESULT EnterAccessKeyMode() const;

        bool IsActive() const;
        AKModeContainer& GetModeContainer() const;

        void SetVisualTree(_In_opt_ VisualTree* tree) const;
        void SetFocusManager(_In_opt_ CFocusManager* focusManager) const;

    private:
        std::unique_ptr<AccessKeyExportImpl> impl;
    };
};
