// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class AnnotatedScrollBarTests : ApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies the AnnotatedScrollBar default property values.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                AnnotatedScrollBar annotatedScrollBar = new AnnotatedScrollBar();
                Verify.IsNotNull(annotatedScrollBar);

                Log.Comment("Logging AnnotatedScrollBar default property values");
                LogAnnotatedScrollBarProperties(annotatedScrollBar);

                Log.Comment("Verifying AnnotatedScrollBar default property values");
                Verify.IsTrue(annotatedScrollBar.IsValueScrollOffset);
                Verify.IsNotNull(annotatedScrollBar.Labels);
                Verify.IsNull(annotatedScrollBar.LabelTemplate);
                Verify.IsNull(annotatedScrollBar.DetailLabelTemplate);
                Verify.AreEqual(annotatedScrollBar.LabelContentAreaSize, 0);
                Verify.AreEqual(annotatedScrollBar.ScrollOffsetToLabelOffsetFactor, 0);
                Verify.AreEqual(annotatedScrollBar.SmallChange, 0);
            });
        }

        private void LogAnnotatedScrollBarProperties(AnnotatedScrollBar annotatedScrollBar)
        {
            Log.Comment(" - annotatedScrollBar.IsValueScrollOffset: " + annotatedScrollBar.IsValueScrollOffset);
            Log.Comment(" - annotatedScrollBar.Labels.Count: " + annotatedScrollBar.Labels.Count);
            Log.Comment(" - annotatedScrollBar.LabelTemplate: " + (annotatedScrollBar.LabelTemplate == null ? "null" : "non-null"));
            Log.Comment(" - annotatedScrollBar.DetailLabelTemplate: " + (annotatedScrollBar.DetailLabelTemplate == null ? "null" : "non-null"));
            Log.Comment(" - annotatedScrollBar.LabelContentAreaSize: " + annotatedScrollBar.LabelContentAreaSize);
            Log.Comment(" - annotatedScrollBar.ScrollOffsetToLabelOffsetFactor: " + annotatedScrollBar.ScrollOffsetToLabelOffsetFactor);
            Log.Comment(" - annotatedScrollBar.SmallChange: " + annotatedScrollBar.SmallChange);
        }
    }
}
