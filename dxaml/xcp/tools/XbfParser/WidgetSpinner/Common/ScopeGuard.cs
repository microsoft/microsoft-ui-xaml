// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.Xaml.WidgetSpinner.Common
{
    internal sealed class ScopeGuard : IDisposable
    {
        internal ScopeGuard(Action scopeExitAction)
        {
            m_scopeExitAction = scopeExitAction;
        }

        public void Dispose()
        {
            if (m_disposed)
            {
                return;
            }
            m_scopeExitAction?.Invoke();
            m_disposed = true;
        }

        internal void Dismiss()
        {
            m_scopeExitAction = null;
        }

        private Action m_scopeExitAction;
        private bool m_disposed = false;
    }
}
