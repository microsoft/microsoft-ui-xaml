// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>

class CXamlLightCollection final : public CDOCollection
{
private:
    explicit CXamlLightCollection(_In_ CCoreServices* core);

public:
    ~CXamlLightCollection() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** object,
        _In_ CREATEPARAMETERS* create);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override;

    // CCollection overrides...
public:
    _Check_return_ HRESULT Clear() override;

    void SetChangeCallback(_In_ const std::weak_ptr<ICollectionChangeCallback>& callback) override;

    // CDOCollectionOverrides ...
    _Check_return_ bool NeedsOwnerInfo() override;

public:
    _Check_return_ HRESULT Append(_In_ CDependencyObject* object, _Out_opt_ XUINT32* index = nullptr) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 index, _In_ CDependencyObject* object) override;
    _Check_return_ void* RemoveAt(_In_ XUINT32 index) override;

private:
    std::weak_ptr<ICollectionChangeCallback> m_wrChangeCallback;
};
