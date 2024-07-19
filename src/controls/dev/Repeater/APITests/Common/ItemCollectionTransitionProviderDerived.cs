﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    public class ItemCollectionTransitionProviderDerived : ItemCollectionTransitionProvider
    {
        public Func<ItemCollectionTransition, bool> ShouldAnimateFunc { get; set; }

        public Action<IList<ItemCollectionTransition>> StartTransitionsFunc { get; set; }
     
        protected override bool ShouldAnimateCore(ItemCollectionTransition transition)
        {
            return ShouldAnimateFunc != null && ShouldAnimateFunc(transition);
        }

        protected override void StartTransitions(IList<ItemCollectionTransition> transitions)
        {
            StartTransitionsFunc?.Invoke(transitions);
        }
    }
}
