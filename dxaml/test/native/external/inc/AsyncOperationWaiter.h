// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        template<typename TResult>
        ref class AsyncOperationWaiter
        {
            typedef wf::IAsyncOperation<TResult> TOperation;
        public:
            AsyncOperationWaiter(TOperation^ operation)
                : m_operation(operation)
            {
                m_operation->Completed = ref new wf::AsyncOperationCompletedHandler<TResult>(
                    [this](TOperation^ operation, wf::AsyncStatus status)
                    {
                        this->OnOperationCompleted(status);
                    });
            }

            ~AsyncOperationWaiter()
            {
                Close();
            }

            TResult GetResult()
            {
                return m_operation->GetResults();
            }

            void Wait()
            {
                m_completedEvent.WaitForDefault();
            }

        private:
            TOperation^ m_operation;
            Event m_completedEvent;

            void Close()
            {
                if (m_operation->Completed!=nullptr)
                {
                    m_operation->Completed = nullptr;
                }
            }

            void OnOperationCompleted(wf::AsyncStatus asyncStatus)
            {
                if (asyncStatus == wf::AsyncStatus::Completed)
                {
                    LOG_OUTPUT(L"Async Operation completed");
                    m_completedEvent.Set();
                }
            }
        };
} } } } }
