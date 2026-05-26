// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using System;

namespace CustomTypes
{
    public sealed partial class MainPage : Page, IComponentConnector
    {
        public MainPage(Action<int, object> iccDel, Func<int, object, IComponentConnector> icc2Del)
        {
            _iccDelegate = iccDel;
            _iccDelegate2 = icc2Del;
        }

        public MainPage(Func<int, object, IComponentConnector> icc2Del)
        {
            _iccDelegate = (id, target) => {}; // No_op: MainPage connect isn't that useful anymore. if the user cares they can use the other constructor
            _iccDelegate2 = icc2Del;
        }

        private Action<int, object> _iccDelegate;
        private Func<int, object, IComponentConnector> _iccDelegate2;

        public void Connect(int connectionId, object target)
        {
            _iccDelegate(connectionId, target);
        }

        public IComponentConnector GetBindingConnector(int connectionId, object target)
        {
            return _iccDelegate2(connectionId, target);
        }
    }

    public sealed partial class MainPageBindings : IComponentConnector, IDataTemplateComponent
    {
        public MainPageBindings(Action<int, object> iccDel)
        {
            _iccDelegate = iccDel;
        }

        public MainPageBindings(Func<int, object, IComponentConnector> icc2Del)
        {
            _iccDelegate2 = icc2Del;
        }

        private Action<int, object> _iccDelegate;
        private Func<int, object, IComponentConnector> _iccDelegate2;

        public void Connect(int connectionId, object target)
        {
            _iccDelegate(connectionId, target);
        }

        public IComponentConnector GetBindingConnector(int connectionId, object target)
        {
            return _iccDelegate2(connectionId, target);
        }

        public void ProcessBindings(object item, int itemIndex, int phase, out int nextPhase)
        {
            // Remove if adding new test that is using ProcessBindings
            throw new NotImplementedException("ProcessBindings shouldn't be called");
        }
        
        public void Recycle()
        {
            // Remove if adding new test that is using Recycle
            throw new NotImplementedException("Recycle shouldn't be called");
        }
    }
}
