// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI;
using Windows.ApplicationModel.Resources;
using Windows.ApplicationModel.Resources.Core;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class LocalizationTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyLanguages()
        {
            RunOnUIThread.Execute(() =>
            {
                ResourceContext context = new ResourceContext();
                context.QualifierValues["language"] = "en-US";
                var resource = ResourceManager.Current.MainResourceMap["/Microsoft.UI.Xaml/Resources/PersonName"];
                var candidate = resource.Resolve(context);
                Log.Comment("Candidate = {0}", candidate.ValueAsString);
                Verify.AreEqual("Person", candidate.ValueAsString, "Checking en-us string");

                context.QualifierValues["language"] = "fr-FR";
                candidate = resource.Resolve(context);
                Verify.AreEqual("Personne", candidate.ValueAsString, "Checking fr-FR string");
            });

            Log.Comment("LocalizationTests complete"); // Extra logging for infra issue with subsequent tests
        }
    }
}
