// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.XBF;

namespace Microsoft.Xaml.WidgetSpinner.Writer
{
    internal partial class ObjectWriter
    {
        private void SetResourceDictionaryCustomRuntimeData(XbfNode<CustomRuntimeData, int> setCustomRuntimeDataNode)
        {
            var data = (ResourceDictionaryCustomRuntimeData)setCustomRuntimeDataNode.Values.Item1;

            var capturedContext = CaptureContext();
            var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
            var nodestream = m_xbfFile.NodeStreams[setCustomRuntimeDataNode.Values.Item2];

            // TODO: add true support for conditionally declared resources by checking each resource's token against the list
            // stored in data.ConditionallyDeclaredObjects and evaluating its associated conditional predicate

            foreach (var kvp in data.ExplicitKeyResources)
            {
                var realizedResource = objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(kvp.Item2));
                var resourceAsXamlObject = realizedResource as XamlObject;
                if (resourceAsXamlObject != null)
                {
                    // The realized object doesn't own a namescope, so transfer its contents to the actual namescope
                    TransferNamescopeFromObject(resourceAsXamlObject);
                }

                ((XamlObject)m_objectStack.Peek()).AddDictionaryItem(kvp.Item1, realizedResource);
            }

            foreach (var kvp in data.ImplicitKeyResources)
            {
                // Only Styles can be implicitly keyed resources
                var realizedResource = (XamlObject)objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(kvp.Item2));
                // The realized object doesn't own a namescope, so transfer its contents to the actual namescope
                TransferNamescopeFromObject(realizedResource);

                // The key for implicit resources (which can only be Styles) is a XamlType
                var key = XamlTypeRegistry.Instance.GetXamlTypeByFullName(kvp.Item1);

                ((XamlObject)m_objectStack.Peek()).AddDictionaryItem(key, realizedResource);
            }
        }

        private void SetDeferredElementCustomRuntimeData(XbfNode<CustomRuntimeData, int> setCustomRuntimeDataNode)
        {
            // A DeferredElement XamlObject will have been created and pushed onto the stack.
            // Pop it so we can replace it with the actual element
            m_objectStack.Pop();
            var capturedContext = CaptureContext();

            var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
            var realizedObject = objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2);

            var objectAsXamlObject = realizedObject as XamlObject;
            if (objectAsXamlObject != null)
            {
                objectAsXamlObject.SetValue(
                    XamlPropertyRegistry.Instance.GetPropertyByName(null, "x:Load"),
                    ((DeferredElementCustomRuntimeData)setCustomRuntimeDataNode.Values.Item1).Realize);

                // The realized object doesn't own a namescope, so transfer its contents to the actual namescope
                TransferNamescopeFromObject(objectAsXamlObject);
            }

            m_objectStack.Push(realizedObject);
        }

        private void SetStyleCustomRuntimeData(XbfNode<CustomRuntimeData, int> setCustomRuntimeDataNode)
        {
            var data = (StyleCustomRuntimeData)setCustomRuntimeDataNode.Values.Item1;
            var nodestream = m_xbfFile.NodeStreams[setCustomRuntimeDataNode.Values.Item2];

            var setterCollection = XamlObjectFactory.CreateObjectFromStableXbfTypeIndex(StableXbfTypeIndex.SetterBaseCollection);
            ((XamlObject)m_objectStack.Peek()).SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.Style_Setters), setterCollection);

            foreach (var setterEssence in data.Setters)
            {
                var setter = XamlObjectFactory.CreateObjectFromStableXbfTypeIndex(StableXbfTypeIndex.Setter);
                setter.SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.Setter_Property), setterEssence.Property);

                if (setterEssence.HasContainerValue)
                {
                    setter.SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.Setter_Value), setterEssence.Value);
                }
                else if (setterEssence.HasStaticResourceValue || setterEssence.HasThemeResourceValue)
                {
                    var capturedContext = CaptureContext();
                    capturedContext.ObjectStack.Push(setter);
                    var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
                    objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(setterEssence.Token));
                }
                else if (setterEssence.HasObjectValue)
                {
                    var capturedContext = CaptureContext();
                    var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
                    var value = objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(setterEssence.Token));
                    // The realized object doesn't own a namescope, so transfer its contents to the actual namescope
                    if (value is XamlObject xamlObject)
                    {
                        // Transfer namescope only when "value" can be cast into a XamlObject.
                        TransferNamescopeFromObject(xamlObject);
                    }

                    // Note that even if "value" is not a valid XamlObject, e.g., when it
                    // represents a scalar value of type Enum, System.Single (Float), string, etc.,
                    // we would still want to use it as a valid Style setter value.
                    // Below is such an example:
                    //
                    // <Setter>
                    //   <Setter.Value>
                    //      <x:String> lorem ipsum </x:String>
                    //   </Setter.Value>
                    // </Setter>
                    setter.SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.Setter_Value), value);
                }
                else if (setterEssence.HasTokenForSelf)
                {
                    var capturedContext = CaptureContext();
                    var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
                    setter = (XamlObject)objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(setterEssence.Token));
                    // TODO: Transfer scope here as well by calling method TransferNamescopeFromObject (VSO task # 17414810)
                }

                setterCollection.AddCollectionItem(setter);
            }
        }

        private void SetVisualStateGroupCollectionCustomRuntimeData(
            XbfNode<CustomRuntimeData, int> setCustomRuntimeDataNode)
        {
            var nodestream = m_xbfFile.NodeStreams[setCustomRuntimeDataNode.Values.Item2];
            var token = ((VisualStateGroupCollectionCustomRuntimeData)setCustomRuntimeDataNode.Values.Item1).EntireCollectionToken;
            // Because of the way VSGC is captured by the CustomWriter, the enclosing
            // PushScopeGetValue node is duplicated. We'll pop the top object off the
            // captured context's object stack (the original PushScopeGetValue object)
            // so that the duplicate node modifies the correct object
            var capturedContext = CaptureContext();
            capturedContext.ObjectStack.Pop();

            var objectWriter = new ObjectWriter(m_xbfFile, capturedContext);
            objectWriter.ProcessXbfFile(setCustomRuntimeDataNode.Values.Item2, nodestream.FindIndex(token));

            // The realized object doesn't own a namescope, so transfer its contents to the actual namescope
            TransferNamescopeFromObject((XamlObject)m_objectStack.Peek());
        }

        private void TransferNamescopeFromObject(XamlObject realizedObject)
        {
            foreach (var kvp in realizedObject.NameScope)
            {
                m_namescope.Add(kvp.Key, kvp.Value);
            }
            realizedObject.ClearRegisteredNames();
        }
    }
}