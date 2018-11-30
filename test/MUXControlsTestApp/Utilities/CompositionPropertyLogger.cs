// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Threading;
using Windows.UI.Composition;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.Utilities
{
    /// <summary>
    /// Utility class used to log values of Composition properties that are being animated by ExpressionAnimation.
    /// The values are obtained via Comp's ICompositionNotifyPropertyChangedPartner which provides notifications 
    /// when a CompositionObject property changes in the DWM (via either animation or directly setting the value).
    ///
    /// Note: There is also a CompositionPropertySpy class in MUXControlsTestApp.Utilities. It also helps one observe 
    /// Comp animated values, but provides different information. Below is a comparison of the two helpers:
    ///
    /// 1) CompositionPropertySpy allows managed code to query current value of an animated CompositionObject property.
    ///    >> It does this by mapping the animated property of interest to a dummy PropertySet property, and 
    ///       calling Stop on the expression. That causes DComp/DWM to push the current value of the property being spied on into the PS.

    /// 2) CompositionPropertyLogger allows managed code to log a list of all values a property has transitioned through in the DWM. 
    ///    It also allows the app to be notified when a property has attained a certain value.
    ///     >> It does this by using COMP's internal ICompositionNotifyPropertyChangedPartner interface, which provides notificatons 
    ///        for animated property changes from the DWM.
    ///     >> Color animations not supported due to current ICompositionNotifyPropertyChangedPartner limitation.
    ///
    /// </summary>
    class CompositionPropertyLogger
    {
        // enum NotificationProperties - valid propertyIds to request callbacks for when the property changes in DWM.
        //
        // The same ids can be used for multiple types, when subscribing for property change notifications.
        //
        // For example:
        // - NotificationProperties.Offset: (Visual.Offset, InsetClip.Offset)
        // - NotificationProperties.AnchorPoint: (Visual.AnchorPoint, InsetClip.AnchorPoint)
        //-----------------------------------------------------------------------------------------------------------
        // Note: Corresponds to DCOMPOSITION_EXPRESSION_NOTIFICATION_PROPERTY in DCompTypes.w
        public enum NotificationProperties
        {
            Undefined = 0,

            Clip,
            Offset,
            Opacity,
            Size,
            RelativeOffset,
            RelativeSize,

            AnchorPoint,
            CenterPoint,
            Orientation,
            RotationAngle,
            RotationAxis,
            Scale,
            TransformMatrix,

            BottomInset,
            LeftInset,
            RightInset,
            TopInset,
        };

        // Currently supported loggable types
        public enum LoggableType
        {
            Float = 0,
            Vector3,
            // Color      <-- Color change notifications not supported by ICompositionNotifyPropertyChangedPartner. 
            //            <-- Color swizzling (eg colar.R) also not supported, so it's not possible to map a supported type.
            //            <-- COMP Deliverable 8278107 tracks full solution for expression/animation tacing.
        };

        struct ProxyPropertyInfo
        {
            public string proxyPropertyName;
            public NotificationProperties proxyPropertyType;
        }

        // Proxy mapping info indexed on LoggableType
        static ProxyPropertyInfo[] ProxyPropertyInfoList = new ProxyPropertyInfo[]
        {
            new ProxyPropertyInfo() {proxyPropertyName = "RotationAngle", proxyPropertyType = NotificationProperties.RotationAngle},
            new ProxyPropertyInfo() {proxyPropertyName = "Offset", proxyPropertyType = NotificationProperties.Offset}
        };

        struct LogData
        {
            public LoggableType type;
            public ExpressionAnimation expression;
            public Object defaultValue;
            public Object expectedValue;
            public float epsilon;
            public ManualResetEvent gotExpectedValue;
            public List<Object> values;
        };

        Dictionary<Tuple<CompositionObject, string>, Visual> _proxyVisualsMap;
        Dictionary<Visual, LogData> _logDataMap;
        PropertyChangedListenerHelper _propertyChangedListener;

        public CompositionPropertyLogger()
        {
            // Create a singleton listener object that will listen for property changes. 
            // We create a proxy Visual for each {CompositionObject, Property} combination that is being tracked,
            // so the various NotifyXXXChanged callbacks are completely disambiguated by the provided "target" object (visual).

            _proxyVisualsMap = new Dictionary<Tuple<CompositionObject, string>, Visual>();
            _logDataMap = new Dictionary<Visual, LogData>();
            _propertyChangedListener = new PropertyChangedListenerHelper(this);
        }

        public ManualResetEvent RegisterProperty(CompositionObject co, string propertyName, LoggableType type, Object defaultValue, Object expectedValue, float epsilon)
        {
            return _propertyChangedListener.AddPropertyChangedListener(co, propertyName, type, defaultValue, expectedValue, epsilon);
        }

        public void UnregisterProperty(CompositionObject co, string propertyName, LoggableType type)
        {
            _propertyChangedListener.RemovePropertyChangedListener(co, propertyName, type);
        }

        public List<Object> GetValues(CompositionObject co, string propertyName)
        {
            var proxyVisual = _proxyVisualsMap[new Tuple<CompositionObject, string>(co, propertyName)];
            return _logDataMap[proxyVisual].values;
        }

        [ComImport]
        [System.Runtime.InteropServices.Guid("5C495A03-13DE-4F33-B81F-36E0803B3919")]
        [ComVisible(true)]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ICompositionPropertyChangedListenerPartner
        {
            void NotifyColorPropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Windows.UI.Color value);

            void NotifyMatrix3x2PropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Matrix3x2 value);

            void NotifyMatrix4x4PropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Matrix4x4 value);

            // Bug 3647518:Property change callbacks need to support Quaternion properties
            // D2DQuaternion requires including an additional header file (shared\d3dxmath.h),
            // which doesn't play nicely with all of the places that include dcompp.h
            // [PreserveSig]
            // int NotifyQuaternionPropertyChanged(
            //     CompositionObject target,
            //     DCOMPOSITION_EXPRESSION_NOTIFICATION_PROPERTY propertyId,
            //     [In] ref Quaternion value);

            void NotifyReferencePropertyChanged(
          [In, MarshalAs(UnmanagedType.IInspectable)] object target,
          [In] uint propertyId);

            void NotifySinglePropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] float value);

            void NotifyVector2PropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Vector2 value);

            void NotifyVector3PropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Vector3 value);

            void NotifyVector4PropertyChanged(
              [In, MarshalAs(UnmanagedType.IInspectable)] object target,
              [In] uint propertyId,
              [In] Vector4 value);
        };


        //---------------------------------------------------------------------------------------------------
        // interface ICompositionNotifyPropertyChangedPartner - a partner interface used to request callbacks
        //                                                      when the specified property changes in DWM.
        //---------------------------------------------------------------------------------------------------

        [ComImport]
        [ComVisible(true)]
        [System.Runtime.InteropServices.Guid("FA92376B-164B-432A-BF53-C7FE2D51CABC")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ICompositionNotifyPropertyChangedPartner
        {
            void SetPropertyChangedListener(
                NotificationProperties propertyId,
                ICompositionPropertyChangedListenerPartner callback);
        }

        class PropertyChangedListenerHelper : ICompositionPropertyChangedListenerPartner
        {
            CompositionPropertyLogger _logger;

            public PropertyChangedListenerHelper(CompositionPropertyLogger logger)
            {
                _logger = logger;
            }

            public ManualResetEvent AddPropertyChangedListener(CompositionObject co, string propertyName, LoggableType type, Object defaultValue, Object expectedValue, float epsilon)
            {
                Compositor myCompositior = Window.Current.Compositor;
                var proxyVisual = myCompositior.CreateContainerVisual();

                // Add mapping from {CO, propertyName} to unique proxyVisual
                _logger._proxyVisualsMap.Add(new Tuple<CompositionObject, string>(co, propertyName), proxyVisual);

                LogData logData = new LogData();
                logData.type = type;
                logData.values = new List<Object>();
                logData.defaultValue = defaultValue;
                logData.expectedValue = expectedValue;
                logData.gotExpectedValue = new ManualResetEvent(false);
                logData.epsilon = epsilon;
                logData.expression = myCompositior.CreateExpressionAnimation("MySourceObject." + propertyName);
                logData.expression.SetReferenceParameter("MySourceObject", co);
                proxyVisual.StartAnimation(ProxyPropertyInfoList[(int)type].proxyPropertyName, logData.expression);

                // Add mapping from proxyVisual to logData, which cotains values and the connecting expression
                _logger._logDataMap.Add(proxyVisual, logData);

                // Manually populate the initial default value, since otherwise we will get no notification if the value never changes from default.
                PropertyChanged(proxyVisual, 0, defaultValue);

                // Start listening to CompositionPropertyChanged notifications on the proxy visual
                ICompositionNotifyPropertyChangedPartner notifyPartner = (ICompositionNotifyPropertyChangedPartner)(object)proxyVisual;
                notifyPartner.SetPropertyChangedListener(ProxyPropertyInfoList[(int)type].proxyPropertyType, this);

                return logData.gotExpectedValue;
            }

            public void RemovePropertyChangedListener(CompositionObject co, string propertyName, LoggableType type)
            {
                var proxyVisual = _logger._proxyVisualsMap[new Tuple<CompositionObject, string>(co, propertyName)];

                if (proxyVisual != null)
                {
                    // Stop listending to CompositionPropertyChanged notifications on the proxy visual
                    var notifyPropertyPartner = (ICompositionNotifyPropertyChangedPartner)proxyVisual;
                    notifyPropertyPartner.SetPropertyChangedListener(ProxyPropertyInfoList[(int)type].proxyPropertyType, null);

                    // Clean up logData and proxyVisual we've used.
                    LogData logData = _logger._logDataMap[proxyVisual];

                    ((CompositionObject)proxyVisual).StopAnimation(ProxyPropertyInfoList[(int)type].proxyPropertyName);
                    _logger._logDataMap.Remove(proxyVisual);
                    _logger._proxyVisualsMap.Remove(new Tuple<CompositionObject, string>(co, propertyName));
                }
            }

            private void
            PropertyChanged(
                object target,
                uint id,
                object value)
            {
                LogData logData = _logger._logDataMap[target as Visual];

                bool gotExpectedValue = false;
                if (value.GetType() == typeof(float))
                {
                    logData.values.Add((float)value);
                    gotExpectedValue = AreFloatsEqualWithTolerance((float)value, (float)(logData.expectedValue), logData.epsilon);
                }
                else if (value.GetType() == typeof(Vector3))
                {
                    logData.values.Add((Vector3)value);
                    bool gotExpectedValue_X = AreFloatsEqualWithTolerance(((Vector3)value).X, ((Vector3)logData.expectedValue).X, logData.epsilon);
                    bool gotExpectedValue_Y = AreFloatsEqualWithTolerance(((Vector3)value).Y, ((Vector3)logData.expectedValue).Y, logData.epsilon);
                    bool gotExpectedValue_Z = AreFloatsEqualWithTolerance(((Vector3)value).Z, ((Vector3)logData.expectedValue).Z, logData.epsilon);

                    gotExpectedValue = gotExpectedValue_X && gotExpectedValue_Y && gotExpectedValue_Z;
                }

                if (gotExpectedValue) { logData.gotExpectedValue.Set(); }
            }

            private void
            PropertyChanged(
                object target,
                uint id)
            {
            }

            public void
            NotifyColorPropertyChanged(
                object target,
                uint propertyId,
                Windows.UI.Color value)
            { /*PropertyChanged(target, propertyId, value);*/ }

            public void
            NotifyMatrix3x2PropertyChanged(
                object target,
                uint propertyId,
                Matrix3x2 value)
            { /*PropertyChanged(target, propertyId, value);*/ }

            public void
            NotifyMatrix4x4PropertyChanged(
                object target,
                uint propertyId,
                Matrix4x4 value)
            { /*PropertyChanged(target, propertyId, value);*/ }

            public void
            NotifyReferencePropertyChanged(
                object target,
                uint propertyId)
            { /*PropertyChanged(target, propertyId);*/ }

            public void
            NotifySinglePropertyChanged(
                object target,
                uint propertyId,
                float value)
            {
                PropertyChanged(target, propertyId, value as Object);
            }

            public void
            NotifyVector2PropertyChanged(
                object target,
                uint propertyId,
                Vector2 value)
            { /*PropertyChanged(target, propertyId, value);*/ }

            public void
            NotifyVector3PropertyChanged(
                object target,
                uint propertyId,
                Vector3 value)
            {
                PropertyChanged(target, propertyId, value as Object);
            }

            public void
            NotifyVector4PropertyChanged(
                object target,
                uint propertyId,
                Vector4 value)
            { /*PropertyChanged(target, propertyId, value); */}
        }

        public static bool AreFloatsEqualWithTolerance(float a, float b, float epsilon)
        {
            return Math.Abs(a - b) < epsilon;
        }
    }
}
