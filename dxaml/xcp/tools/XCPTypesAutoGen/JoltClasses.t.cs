// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Markup;
using MS.Internal;

namespace MS.Internal
{
    internal static partial class CoreTypes
    {
        // Constants for known types
        //#comment  Generation tool will insert constants for known types
        //#comment  internal const uint <namespace>_<type>ID = <index>;
        //#insert TYPEID_CONSTANTS

        // Largest type ID
        //#comment  Generation tool will insert constant for largest type ID
        //#comment  internal const uint MaxTypeID = <largest_index>;
        //#insert MAXTYPEID_CONSTANT        

        // Type lookup: type name -> core type#
        private static uint CoreTypeIdFromString(string typeName)
        {
            switch(typeName)
            {
                //#comment  Generation tool will insert entries such as 
                //#comment    case "<namespace>.<type>": return <namespace>_<type>ID;
                //#comment  for each known type index.
                //#insert TYPEID_FROMSTRING_CASES
            }
        }

        // Create core wrapper: type name -> managed wrapper of core object#
        public static IManagedPeer GetCoreWrapper(uint typeId)
        {
            switch(typeId)
            {
                //#comment  Generation tool will insert entries such as 
                //#comment    case <namespace>_<type>ID: return new <namespace>.<type>();
                //#comment  for each known type index.
                //#insert TYPEID_TOWRAPPER_CASES
            }
        }

        // Return core type: type ID -> type of core object#
        public static Type GetCoreType(uint typeId)
        {
            if (typeId == 0 || typeId > MaxTypeID)
            {
                return null;
            }

            // If we don't have the type but we can create one, cache
            // that type. For example, a type outside of System.Windows
            if (_typeTable[typeId] == null)
            {
                object instance = GetCoreWrapper(typeId);
                if (instance != null)
                {
                    _typeTable[typeId] = instance.GetType();
                }
            }

            return _typeTable[typeId];
        }

        private delegate Type createCoreType();

        private static Type[] _typeTable = 
            new Type
                [
                    //#insert TYPETABLESIZE
                ]
        {
            //#comment  Generation tool will insert entries such as 
            //#comment    typeof(<namespace>.<type>),
            //#comment  for each known type index.
            //#insert TYPEID_TOTYPE_ELEMENTS
        };
    }

    internal static class CoreProperties
    {
        // Constants for known properties
        //#comment  Generation tool will insert constants for known properties
        //#comment  internal const uint <namespace>_<type>_<property>ID = <index>;
        //#insert PROPERTYID_CONSTANTS
    }

    internal static class CoreEvents
    {
        // Constants for known events
        //#comment  Generation tool will insert constants for known events
        //#comment  internal const uint <namespace>_<type>_<event>ID = <index>;
        //#insert EVENTID_CONSTANTS
    }

    internal static class CoreEventArgs
    {
        /// <summary>
        /// Creates an event args for the provided type index
        /// </summary>
        ///<SecurityNote> 
        ///Critical: Takes an IntPtr
        ///</SecurityNote> 
        [System.Security.SecurityCritical]
        public static EventArgs CreateEventArgs(uint typeIndex, IntPtr data)
        {
            EventArgs eventArgs = null;
            switch(typeIndex)
            {
                //#comment  Generation tool will insert entries such as 
                //#comment    case <namespace>_<type>ID:
                //#comment        <namespace>.<type> v_<type> = new <namespace>.<type>();
                //#comment        v_<type>.SetPtr(data);
                //#comment        return v_<type>;
                //#comment  for each known event args index.
                //#insert CREATE_EVENTARGS_CASES
                default:
                    eventArgs = null;
                    break;
            }
            if (eventArgs != null && data != IntPtr.Zero)
            {
                XcpImports.AddRefNativeObjectNative(data);
            }
            return eventArgs;
        }
    }


    internal static class CoreInvokeHandler
    {
        public static void InvokeEventHandler(uint typeIndex, Delegate handlerDelegate, object sender, object args)
        {
            switch(typeIndex)
            {
                case 0:
                    EventHandler handler = handlerDelegate as EventHandler;
                    handler.Invoke(sender, (EventArgs) args);
                    break;
                //#comment  Generation tool will insert entries such as 
                //#comment    case <index>:
                //#comment        <eventhandler> v_<event> = handlerDelegate as <eventhandler>;
                //#comment        if (v_<event> == null) {
                //#comment            System.EventHandler<eventargs> vt_<event> = handlerDelegate as System.EventHandler<eventargs>;
                //#comment            vt_<event>.Invoke(sender, (<eventargs>) args);
                //#comment            break;
                //#comment        }
                //#comment        v_<event>.Invoke(sender, (<eventargs>) args);
                //#comment        break;
                //#comment  for each known event args index.
                //#insert INVOKE_EVENT_CASES
            }
        }
    }
}

namespace System.Windows
{
    public abstract partial class PresentationFrameworkCollection<T> : System.Windows.DependencyObject, System.Collections.Generic.IList<T>
    {
        public static readonly System.Windows.DependencyProperty CountProperty =
            System.Windows.DependencyProperty.RegisterCoreProperty(CoreProperties.SystemWindows_PresentationFrameworkCollection_CountID, typeof(int));
    }
}

namespace System.Windows.Controls
{
    public abstract partial class Control : System.Windows.FrameworkElement
    {
        // Order of delegates is in sync with CControl::Delegates in Control.cpp
        private static readonly ControlDelegate[] _controlDelegates =
            new ControlDelegate[] { 
                //#comment  Generation tool will insert names for known events
                //#comment  On<eventname>,
                //#insert CONTROL_EVENT_METHODS
            };

        private enum Delegates
        {
            //#comment  Generation tool will insert values for known events
            //#comment  <eventname> = <index>,
            //#insert CONTROL_EVENT_INDICES
        }
    }
}

//#comment  Generation tool will insert namespaces with delegates, 
//#comment  enumerations and classes for each known namespace index.
//#insert NAMESPACES
