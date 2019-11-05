// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if !USING_TAEF
using System;
using System.Collections.Generic;
using System.Text;

using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;

namespace Common
{
    // TAEF has a different terms for the same concepts as compared with MSTest.
    // In order to allow both to use the same test files, we'll define these helper classes
    // to translate TAEF into MSTest.
    public static class Log
    {
        public static void Comment(string format, params object[] args)
        {
            LogMessage(format, args);
        }

        public static void Warning(string format, params object[] args)
        {
            LogMessage(format, args);
        }

        public static void Error(string format, params object[] args)
        {
            LogMessage(format, args);
        }

        private static void LogMessage(string format, object[] args)
        {
            // string.Format() complains if we pass it something with braces, even if we have no arguments.
            // To account for that, we'll escape braces if we have no arguments.
            if (args.Length == 0)
            {
                format = format.Replace("{", "{{").Replace("}", "}}");
            }

            Logger.LogMessage(format, args);
        }
    }

    public static class LogController
    {
        public static void InitializeLogging()
        {
            // TODO: implement
        }
    }

    public static class Verify
    {
        public static bool DisableVerifyFailureExceptions
        {
            get;
            set;
        }

        private static void ThrowOrLogException(Exception e)
        {
            if (DisableVerifyFailureExceptions)
            {
                Log.Error(e.Message);
            }
            else
            {
                throw e;
            }
        }

        public static void AreEqual(object expected, object actual, string message = null)
        {
            try
            {
                Assert.AreEqual(expected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void AreEqual<T>(T expected, T actual, string message = null)
        {
            try
            {
                Assert.AreEqual(expected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void AreNotEqual(object notExpected, object actual, string message = null)
        {
            try
            { 
                Assert.AreNotEqual(notExpected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void AreNotEqual<T>(T notExpected, T actual, string message = null)
        {
            try
            { 
                Assert.AreNotEqual(notExpected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void AreSame(object expected, object actual, string message = null)
        {
            try
            {
                Assert.AreSame(expected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void AreNotSame(object notExpected, object actual, string message = null)
        {
            try
            { 
                Assert.AreNotSame(notExpected, actual, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsLessThan(IComparable expectedLess, IComparable expectedGreater, string message = null)
        {
            try
            { 
                Assert.IsTrue(expectedLess.CompareTo(expectedGreater) < 0, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsLessThanOrEqual(IComparable expectedLess, IComparable expectedGreater, string message = null)
        {
            try
            { 
                Assert.IsTrue(expectedLess.CompareTo(expectedGreater) <= 0, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsGreaterThan(IComparable expectedGreater, IComparable expectedLess, string message = null)
        {
            try
            {
                Assert.IsTrue(expectedGreater.CompareTo(expectedLess) > 0, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsGreaterThanOrEqual(IComparable expectedGreater, IComparable expectedLess, string message = null)
        {
            try
            {
                Assert.IsTrue(expectedGreater.CompareTo(expectedLess) >= 0, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsNull(object value, string message = null)
        {
            try
            { 
                Assert.IsNull(value, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsNotNull(object value, string message = null)
        {
            try
            { 
                Assert.IsNotNull(value, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsTrue(bool condition, string message = null)
        {
            try
            {
                Assert.IsTrue(condition, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void IsFalse(bool condition, string message = null)
        {
            try
            { 
                Assert.IsFalse(condition, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void Fail()
        {
            try 
            { 
                Assert.Fail();
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void Fail(string message, params object[] args)
        {
            try
            { 
                Assert.Fail(message, args);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void Throws<T>(Action action, string message) where T : Exception
        {
            try
            { 
                Assert.ThrowsException<T>(action, message);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }

        public static void Throws<T>(Action action) where T : Exception
        {
            try
            { 
                Assert.ThrowsException<T>(action);
            }
            catch (AssertFailedException e)
            {
                ThrowOrLogException(e);
            }
        }
    }
}
#endif