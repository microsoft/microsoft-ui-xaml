// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class APLock
{
    private:
        CAutomationPeer *ptr_;
        APLock(const APLock&);
        APLock& operator=(const APLock&);
            
    public:
        explicit APLock(
            _In_ CAutomationPeer* const other)
            : ptr_(other)
        {
            if (ptr_ != nullptr)
            {
                ptr_->AddRef();
            }
        }
            
        ~APLock()
        {
            CAutomationPeer* const temp = ptr_;
            if (temp != nullptr)
            {
                ptr_ = nullptr;
                temp->Release();
            }
        }
};
