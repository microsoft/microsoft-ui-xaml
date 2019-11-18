// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp.Utilities
{
    // Utility class used to turn on private logging and automatically turn it off when the instance gets disposed.
    public class PrivateLoggingHelper : IDisposable
    {
        public PrivateLoggingHelper() : this(null)
        {
        }

        public PrivateLoggingHelper(string type) : this(type, isLoggingInfoLevel: true, isLoggingVerboseLevel: true)
        {
        }

        public PrivateLoggingHelper(string type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            Types = new List<string>(1);
            Types.Add(type);
            Initialize(isLoggingInfoLevel, isLoggingVerboseLevel);
        }

        public PrivateLoggingHelper(string type1, string type2) : this(type1, type2, isLoggingInfoLevel: true, isLoggingVerboseLevel: true)
        {
        }

        public PrivateLoggingHelper(string type1, string type2, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            Types = new List<string>(2);
            Types.Add(type1);
            Types.Add(type2);
            Initialize(isLoggingInfoLevel, isLoggingVerboseLevel);
        }

        public PrivateLoggingHelper(List<string> types, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            Types = types;
            Initialize(isLoggingInfoLevel, isLoggingVerboseLevel);
        }

        public List<string> Types
        {
            get;
            set;
        }

        public void Dispose()
        {
            RunOnUIThread.Execute(() =>
            {
                string typeNames = Types.Count == 1 ? "type" : "types";

                foreach (string type in Types)
                {
                    typeNames += " " + type;
                }

                Log.Comment("PrivateLoggingHelper disposal: Turning off private logging for {0}.", typeNames);
                foreach (string type in Types)
                {
                    MUXControlsTestHooks.SetOutputDebugStringLevelForType(type, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                    MUXControlsTestHooks.SetLoggingLevelForType(type, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                }
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            });
        }

        private void Initialize(bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            System.Diagnostics.Debug.Assert(isLoggingInfoLevel || isLoggingVerboseLevel);

            RunOnUIThread.Execute(() =>
            {
                string typeNames = Types.Count == 1 ? "type " : "types ";

                foreach (string type in Types)
                {
                    typeNames += type + ", ";
                }

                Log.Comment("PrivateLoggingHelper creation: Turning on private logging for {0} isLoggingInfoLevel={1}, isLoggingVerboseLevel={2}.",
                    typeNames, isLoggingInfoLevel, isLoggingVerboseLevel);

                foreach (string type in Types)
                {
                    MUXControlsTestHooks.SetOutputDebugStringLevelForType(type, isLoggingInfoLevel, isLoggingVerboseLevel);
                    MUXControlsTestHooks.SetLoggingLevelForType(type, isLoggingInfoLevel, isLoggingVerboseLevel);
                }

                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
            });
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
            }

            if (args.IsVerboseLevel)
            {
                Log.Comment("  Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                Log.Comment("  Info:    " + senderName + "m:" + msg);
            }
        }
    }
}
