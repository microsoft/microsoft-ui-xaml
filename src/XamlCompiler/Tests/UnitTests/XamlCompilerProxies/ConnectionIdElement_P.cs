// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Collections;

namespace Win8Xaml.CompilerProxies
{
    public class ConnectionIdElement
    {
        static ProxyHelper _connectionIdElementType;
        static PropertyInfo _connectionIdProperty;
        static PropertyInfo _fieldDefinitionProperty;
        static PropertyInfo _hasEventAssignmentsProperty;
        static PropertyInfo _eventAssignmentsProperty;
        static PropertyInfo _lineNumberInfoProperty;
        static PropertyInfo _parentFileCodeInfoProperty;
        static PropertyInfo _bindAssignmentsProperty;

        object _instance;

        static ConnectionIdElement()
        {
            _connectionIdElementType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.ConnectionIdElement");
            _connectionIdProperty = _connectionIdElementType.GetProperty("ConnectionId");
            _fieldDefinitionProperty = _connectionIdElementType.GetProperty("FieldDefinition");
            _hasEventAssignmentsProperty = _connectionIdElementType.GetProperty("HasEventAssignments");
            _eventAssignmentsProperty = _connectionIdElementType.GetProperty("EventAssignments");
            _lineNumberInfoProperty = _connectionIdElementType.GetProperty("LineNumberInfo");
            _parentFileCodeInfoProperty = _connectionIdElementType.GetProperty("ParentFileCodeInfo");
            _bindAssignmentsProperty = _connectionIdElementType.GetProperty("BindAssignments");
        }

        public ConnectionIdElement() { }

        public ConnectionIdElement(object instance)
        {
            _instance = instance;
        }

        public int ConnectionId
        {
            get { return (int)_connectionIdProperty.GetValue(_instance, null); }
        }

        public LineNumberInfo LineNumberInfo
        {
            get
            {
                object inst = _lineNumberInfoProperty.GetValue(_instance, null);
                return new LineNumberInfo(inst);
            }
        }

        public FieldDefinition FieldDefinition
        {
            get
            {
                object instance = _fieldDefinitionProperty.GetValue(_instance, null);
                FieldDefinition fd = new FieldDefinition(instance);
                return fd;
            }
        }

        public XamlFileCodeInfo ParentFileCodeInfo
        {
            get
            {
                object inst = _parentFileCodeInfoProperty.GetValue(_instance, null);
                return new XamlFileCodeInfo(inst);
            }
        }

        public bool HasEventAssignments
        {
            get { return (bool)_eventAssignmentsProperty.GetValue(_instance, null); }
        }

        public List<EventAssignment> EventAssignments
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_eventAssignmentsProperty.GetValue(_instance, null);
                List<EventAssignment> EventAssignmentList = new List<EventAssignment>();
                foreach (Object obj in objectList)
                {
                    EventAssignment eventAssignment = new EventAssignment(obj);
                    EventAssignmentList.Add(eventAssignment);
                }
                return EventAssignmentList;
            }
        }

        public List<BindAssignment> BindAssignments
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_bindAssignmentsProperty.GetValue(_instance, null);
                List<BindAssignment> bindAssignmentList = new List<BindAssignment>();
                foreach (object obj in objectList)
                {
                    BindAssignment bindAssignment = new BindAssignment(obj);
                    bindAssignmentList.Add(bindAssignment);
                }
                return bindAssignmentList;
            }
        }
    }
}
