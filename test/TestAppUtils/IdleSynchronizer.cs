// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using System.Threading;
using Windows.ApplicationModel.Core;
using Windows.Foundation;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

namespace MUXControls.TestAppUtils
{
    class IdleSynchronizer
    {
        private static IdleSynchronizer instance = null;
        
        public static IdleSynchronizer Instance
        {
            get
            {
                if (instance == null)
                {
                    throw new Exception("Init() must be called on the UI thread before retrieving Instance.");
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
                throw new Exception("Init() must be called on the UI thread.");
            }
            
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