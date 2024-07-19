﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using System.Threading;
using Windows.ApplicationModel.Core;
using Windows.Foundation;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp.Utilities
{
    class IdleSynchronizer
    {
        public static DispatcherQueue DispatcherQueue
        {
            get;
            private set;
        }

        private static IdleSynchronizer instance = null;
        
        private static IdleSynchronizer Instance
        {
            get
            {
                if (instance == null)
                {
                    throw new Exception("IdleSynchronizer.Init() must be called on the UI thread before waiting.");
                }

                return instance;
            }
        }

        private AppTestAutomationHelpers.IdleSynchronizer _idleSynchronizerImpl;

        private IdleSynchronizer(DispatcherQueue dispatcherQueue)
        {
            _idleSynchronizerImpl = new AppTestAutomationHelpers.IdleSynchronizer(dispatcherQueue);
        }
        
        public static void Init()
        {
            DispatcherQueue dispatcherQueue = DispatcherQueue.GetForCurrentThread();

            if (dispatcherQueue == null)
            {
                throw new Exception("IdleSynchronizer.Init() must be called on the UI thread.");
            }

            DispatcherQueue = dispatcherQueue;
            instance = new IdleSynchronizer(dispatcherQueue);
        }

        public static void Wait()
        {
            string errorString = Instance.WaitInternal();

            if (errorString.Length > 0)
            {
                throw new Exception(errorString);
            }
        }

        public static string TryWait()
        {
            return Instance.WaitInternal();
        }

        private string WaitInternal()
        {
            return _idleSynchronizerImpl.TryWait();
        }
    }
}
