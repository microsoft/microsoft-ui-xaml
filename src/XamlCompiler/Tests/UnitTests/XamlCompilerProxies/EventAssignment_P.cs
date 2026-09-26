// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class EventAssignment
    {
        static ProxyHelper _eventAssignmenType;

        static PropertyInfo _eventNameProperty;
        static PropertyInfo _eventTypeProperty;
        static PropertyInfo _declaringTypeProperty;
        static PropertyInfo _handlerNameProperty;
        static PropertyInfo _lineNumberInfoProperty;

        object _instance;

        static EventAssignment()
        {
            _eventAssignmenType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.EventAssignment");
            _eventNameProperty = _eventAssignmenType.GetProperty("EventName");
            _eventTypeProperty = _eventAssignmenType.GetProperty("EventType");
            _declaringTypeProperty = _eventAssignmenType.GetProperty("DeclaringType");
            _handlerNameProperty = _eventAssignmenType.GetProperty("HandlerName");
            _lineNumberInfoProperty = _eventAssignmenType.GetProperty("LineNumberInfo");
        }

        public EventAssignment(object instance)
        {
            _instance = instance;
        }

        public String EventName
        {
            get { return (string)_eventNameProperty.GetValue(_instance, null); }
        }

        public TypeForCodeGen EventType
        {
            get
            {
                object obj = _eventTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new TypeForCodeGen(obj);  // this is a proxied type.
            }
        }

        public TypeForCodeGen DeclaringType
        {
            get
            {
                object obj = _declaringTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new TypeForCodeGen(obj);  // this is a proxied type.
            }
        }

        public String HandlerName
        {
            get { return (string)_handlerNameProperty.GetValue(_instance, null); }
        }

        public LineNumberInfo LineNumberInfo
        {
            get
            {
                object inst = _lineNumberInfoProperty.GetValue(_instance, null);
                return new LineNumberInfo(inst);
            }
        }

    }
}
