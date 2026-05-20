// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using Windows.Foundation;

namespace Private.Infrastructure.Hosting
{
    internal static class AppDomainExceptionHandler
    {
        private static ExceptionHandler exceptionHandlerAction;

        internal static void LogException(Exception e)
        {
            try
            {
                if (AppDomainExceptionHandler.exceptionHandlerAction!=null)
                {
                    AppDomainExceptionHandler.exceptionHandlerAction(e.ToString());
                }
            }
            catch
            {
            }
        }

        private static void OnException(object sender, UnhandledExceptionEventArgs args)
        {
            Exception e = (Exception)args?.ExceptionObject;
            LogException(e);
        }

        internal static void SetExceptionHandler(ExceptionHandler handler)
        {
            var currentDomain = AppDomain.CurrentDomain;
            if (handler!=null)
            {
                AppDomainExceptionHandler.exceptionHandlerAction = handler;
                currentDomain.UnhandledException += new UnhandledExceptionEventHandler(AppDomainExceptionHandler.OnException);
            }
            else
            {
                currentDomain.UnhandledException -= new UnhandledExceptionEventHandler(AppDomainExceptionHandler.OnException);
                AppDomainExceptionHandler.exceptionHandlerAction = null;
            }
        }
    }
}

