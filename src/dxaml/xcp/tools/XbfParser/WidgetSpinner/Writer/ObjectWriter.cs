// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Common;
using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.XBF;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.Writer
{
    // Takes an XbfFile as input and transforms it into a XamlObject graph
    internal partial class ObjectWriter
    {
        internal class SavedContext
        {
            internal Stack<object> ObjectStack { get; } = new Stack<object>();
            internal Stack<List<Tuple<string, XamlXmlNamespace>>> NamespaceStack { get; } = new Stack<List<Tuple<string, XamlXmlNamespace>>>();

            internal SavedContext()
            {
            }

            internal SavedContext(Stack<object> objectStack, Stack<List<Tuple<string, XamlXmlNamespace>>> namespaceStack)
            {
                ObjectStack = objectStack;
                NamespaceStack = namespaceStack;
            }
        }

        internal object Result { get; set; }
        internal bool ConditionalXamlEncountered { get; set; }

        internal ObjectWriter(XbfFile xbfFile)
        {
            m_xbfFile = xbfFile;
        }

        /// <summary>
        /// Instantiate an ObjectWriter with the provided SavedContext. Note that the contents of the
        /// SavedContext are immediately copied out, and then no longer used thereafter, so modifying
        /// it after the ObjectWriter has been constructed has no effect.
        /// </summary>
        /// <param name="xbfFile"></param>
        /// <param name="savedContext"></param>
        internal ObjectWriter(XbfFile xbfFile, SavedContext savedContext)
            : this(xbfFile)
        {
            m_savedContext = savedContext;
            m_objectStack = savedContext.ObjectStack.Clone();
            m_namespaceStack = savedContext.NamespaceStack.Clone();
        }

        internal SavedContext CaptureContext()
        {
            var objectStackClone = m_objectStack.Clone();
            var namespaceStackClone = m_namespaceStack.Clone();

            return new SavedContext(objectStackClone, namespaceStackClone);
        }

        internal object ProcessXbfFile()
        {
            return ProcessXbfFile(0);
        }

        internal object ProcessXbfFile(int streamIndex)
        {
            return ProcessXbfFile(streamIndex, 0);
        }

        internal object ProcessXbfFile(int streamIndex, int startNodeIndex)
        {
            var nodeStream = m_xbfFile.NodeStreams[streamIndex];

            var finalIndex = VisitNodes(nodeStream, startNodeIndex);

            // Remaining object on the object stack is the root object
            Result = m_objectStack.Pop();
            var resultAsXamlObject = Result as XamlObject;
            if (resultAsXamlObject != null)
            {
                if (!string.IsNullOrEmpty(m_rootXClass))
                {
                    resultAsXamlObject.SetValue(XamlPropertyRegistry.Instance.GetPropertyByName(null, "x:Class"), m_rootXClass);
                }
                foreach (var name in m_namescope.Keys)
                {
                    var namedObject = m_namescope[name];
                    resultAsXamlObject.RegisterName(name, namedObject);
                }
            }

            Reset();
            return Result;
        }

        private void Reset()
        {
            m_objectStack = m_savedContext.ObjectStack.Clone();
            m_namespaceStack = m_savedContext.NamespaceStack.Clone();
            m_namescope.Clear();
            m_rootXClass = null;
        }

        // An XBF node stream can be thought of as a lineraized tree of XbfNodes. 
        // The XAML framework treats this as a purely pre-order traversal, but 
        // bookkeeping can be simplified by instead performing a mix of pre- and 
        // post-order operations on the nodes. If one considers (potential) interior 
        // nodes to be those that push a new "scope" (in the parlance of XML, this would 
        // be the scope within which an XML namespace definition is valid), and their 
        // children to be those nodes following after until an EndInitPopScope/PopScope 
        // node is reached, then it's straightforward to derive the algorithm 
        // [visit each node by 
        //      1) performing the pre-order operation, 
        //      2) if it is an interior node type, visit each subsequent node (these are 
        //         the child nodes) until an EndInitPopScope/EndInitProvideValuePopScope/PopScope node is reached, 
        //      3) perform the post-order operation, 
        //      4) visit the node immediately following the final descendant (or the next 
        //         node if there were no children)
        // ]
        // and requisite data structures [a stack to hold namespace scopes, and a stack 
        // to hold created objects]. 
        // Once the algorithm has been derived, it is simply a matter of determining what 
        // each specific node's pre- and post-order operations must do. For the vast 
        // majority of node types (these "leaf" nodes can never have children) only 
        // a single operation is required, so by convention that single operation is a
        // post-order operation.
        private int VisitNodes(IReadOnlyList<XbfNode> nodes, int startIndex)
        {
            var currentIndex = startIndex;
            // Because of the way XAML semantics work, we are done doing work
            // once the namespace stack returns to its starting size
            var startDepth = m_namespaceStack.Count;

            do
            {
                var currentNode = nodes[currentIndex];
                // Consume the current node
                var newIndex = currentIndex + 1;

                // Perform pre-order operation
                PreorderVisit(currentNode);

                // Visit "children" if this isn't a "leaf" node
                // This can consume multiple nodes (i.e. the next node visited might be
                // significantly further down the nodestream than the node after this one)
                if (!IsLeafNode(currentNode))
                {
                    // Added "newIndex < nodes.Count" to avoid IndexOutOfRange Error in Flotsam (Task # 20688139)
                    while (newIndex < nodes.Count && nodes[newIndex].NodeType != XbfNodeType.EndInitPopScope &&
                           nodes[newIndex].NodeType != XbfNodeType.EndInitProvideValuePopScope &&
                           nodes[newIndex].NodeType != XbfNodeType.PopScope)
                    {
                        newIndex = VisitNodes(nodes, newIndex);
                    }
                }

                // Perform post-order operation
                PostOrderVisit(currentNode);

                // Visit next node (might be "sibling" or "parent")
                currentIndex = newIndex;
            } while (m_namespaceStack.Count > startDepth && currentIndex < nodes.Count);

            return currentIndex;
        }

        private void PreorderVisit(XbfNode currentNode)
        {
            switch (currentNode.NodeType)
            {
                case XbfNodeType.AddNamespace:
                case XbfNodeType.EndInitPopScope:
                case XbfNodeType.EndInitProvideValuePopScope:
                case XbfNodeType.AddToCollection:
                case XbfNodeType.AddToDictionary:
                case XbfNodeType.AddToDictionaryWithKey:
                case XbfNodeType.CheckPeerType:
                case XbfNodeType.CreateTypeBeginInit:
                case XbfNodeType.CreateTypeWithConstantBeginInit:
                case XbfNodeType.CreateTypeWithTypeConvertedConstantBeginInit:
                case XbfNodeType.EndConditionalScope:
                case XbfNodeType.GetResourcePropertyBag:
                case XbfNodeType.PopScope:
                case XbfNodeType.PushConstant:
                case XbfNodeType.SetValue:
                case XbfNodeType.SetValueFromMarkupExtension:
                case XbfNodeType.SetConnectionId:
                case XbfNodeType.SetName:
                case XbfNodeType.SetDeferredProperty:
                case XbfNodeType.SetCustomRuntimeData:
                case XbfNodeType.SetValueConstant:
                case XbfNodeType.SetValueTypeConvertedConstant:
                case XbfNodeType.SetValueTypeConvertedResolvedType:
                case XbfNodeType.SetValueTypeConvertedResolvedProperty:
                case XbfNodeType.ProvideStaticResourceValue:
                case XbfNodeType.SetValueFromStaticResource:
                case XbfNodeType.ProvideThemeResourceValue:
                case XbfNodeType.SetValueFromThemeResource:
                case XbfNodeType.SetValueFromTemplateBinding:
                    return;

                case XbfNodeType.PushScope:
                    {
                        PreOrderVisitPushScope(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeAddNamespace:
                    {
                        PreOrderVisitPushScopeAddNamespace(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeGetValue:
                    {
                        PreOrderVisitPushScopeGetValue(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeBeginInit:
                    {
                        PreOrderVisitPushScopeCreateTypeBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeWithConstantBeginInit:
                    {
                        PreOrderVisitPushScopeCreateTypeWithConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
                    {
                        PreOrderVisitPushScopeCreateTypeWithTypeConvertedConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.BeginConditionalScope:
                    {
                        PreOrderVisitBeginConditionalScope(currentNode);
                    }
                    break;

                default:
                    // Treat unrecognized node types as a no-op, but raise warning
                    // 1) orphaned ProvideValue can appear if a Binding is used as a resource in a ResourceDictionary
                    // 2) CreateTypeWithConstant can also appear, but it is currently unknown what markup triggers its appearance
                    {
                        ErrorService.Instance.RaiseWarning($"Unexpected node with type {currentNode.NodeType} appeared in the node stream at offset {currentNode.NodeStreamOffset} during object graph creation. This is harmless as it is treated as a no-op, but may indicate a problem in the parser during XBF generation.");
                    }
                    return;
            }
        }

        private void PostOrderVisit(XbfNode currentNode)
        {
            switch (currentNode.NodeType)
            {
                case XbfNodeType.AddNamespace:
                    {
                        PostOrderVisitAddNamespace(currentNode);
                    }
                    break;

                case XbfNodeType.EndInitPopScope:
                    {
                        PostOrderVisitEndInitPopScope(currentNode);
                    }
                    break;

                case XbfNodeType.EndInitProvideValuePopScope:
                    {
                        PostOrderVisitEndInitProvideValuePopScope(currentNode);
                    }
                    break;

                case XbfNodeType.AddToCollection:
                    {
                        PostOrderVisitAddToCollection(currentNode);
                    }
                    break;

                case XbfNodeType.AddToDictionary:
                    {
                        PostOrderVisitAddToDictionary(currentNode);
                    }
                    break;

                case XbfNodeType.AddToDictionaryWithKey:
                    {
                        PostOrderVisitAddToDictionaryWithKey(currentNode);
                    }
                    break;

                case XbfNodeType.CheckPeerType:
                    {
                        PostOrderVisitCheckPeerType(currentNode);
                    }
                    break;

                case XbfNodeType.CreateTypeBeginInit:
                    {
                        PostOrderVisitCreateTypeBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.CreateTypeWithConstantBeginInit:
                    {
                        PostOrderVisitCreateTypeWithConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.CreateTypeWithTypeConvertedConstantBeginInit:
                    {
                        PostOrderVisitCreateTypeWithTypeConvertedConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.EndConditionalScope:
                    {
                        PostOrderVisitEndConditionalScope(currentNode);
                    }
                    break;

                case XbfNodeType.GetResourcePropertyBag:
                    {
                        PostOrderVisitGetResourcePropertyBag(currentNode);
                    }
                    break;

                case XbfNodeType.PopScope:
                    {
                        PostOrderVisitPopScope(currentNode);
                    }
                    break;

                case XbfNodeType.PushConstant:
                    {
                        PostOrderVisitPushConstant(currentNode);
                    }
                    break;

                case XbfNodeType.SetValue:
                    {
                        PostOrderVisitSetValue(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueFromMarkupExtension:
                    {
                        PostOrderVisitSetValueFromMarkupExtension(currentNode);
                    }
                    break;

                case XbfNodeType.SetConnectionId:
                    {
                        PostOrderVisitSetConnectionId(currentNode);
                    }
                    break;

                case XbfNodeType.SetName:
                    {
                        PostOrderVisitSetName(currentNode);
                    }
                    break;

                case XbfNodeType.SetDeferredProperty:
                    {
                        PostOrderVisitSetDeferredProperty(currentNode);
                    }
                    break;

                case XbfNodeType.SetCustomRuntimeData:
                    {
                        PostOrderVisitSetCustomRuntimeData(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueConstant:
                    {
                        PostOrderVisitSetValueConstant(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueTypeConvertedConstant:
                    {
                        PostOrderVisitSetValueTypeConvertedConstant(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueTypeConvertedResolvedType:
                    {
                        PostOrderVisitSetValueTypeConvertedResolvedType(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueTypeConvertedResolvedProperty:
                    {
                        PostOrderVisitSetValueTypeConvertedResolvedProperty(currentNode);
                    }
                    break;

                case XbfNodeType.ProvideStaticResourceValue:
                    {
                        PostOrderVisitProvideStaticResourceValue(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueFromStaticResource:
                    {
                        PostOrderVisitSetValueFromStaticResource(currentNode);
                    }
                    break;

                case XbfNodeType.ProvideThemeResourceValue:
                    {
                        PostOrderVisitProvideThemeResourceValue(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueFromThemeResource:
                    {
                        PostOrderVisitSetValueFromThemeResource(currentNode);
                    }
                    break;

                case XbfNodeType.SetValueFromTemplateBinding:
                    {
                        PostOrderVisitSetValueFromTemplateBinding(currentNode);
                    }
                    break;

                case XbfNodeType.PushScope:
                    {
                        PostOrderVisitPushScope(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeAddNamespace:
                    {
                        PostOrderVisitPushScopeAddNamespace(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeGetValue:
                    {
                        PostOrderVisitPushScopeGetValue(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeBeginInit:
                    {
                        PostOrderVisitPushScopeCreateTypeBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeWithConstantBeginInit:
                    {
                        PostOrderVisitPushScopeCreateTypeWithConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
                    {
                        PostOrderVisitPushScopeCreateTypeWithTypeConvertedConstantBeginInit(currentNode);
                    }
                    break;

                case XbfNodeType.BeginConditionalScope:
                    {
                        PostOrderVisitBeginConditionalScope(currentNode);
                    }
                    break;

                // these are temporary nodes that should have been optimized away,
                // but there are legal node sequences they can appear in which
                // result in the node list optimizer leaving them in place
                case XbfNodeType.ProvideValue:
                case XbfNodeType.CreateTypeWithConstant:
                    return;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        #region Node visitors

        private void PostOrderVisitAddNamespace(XbfNode currentNode)
        {
            var addNamespaceNode = (XbfNode<string, XamlXmlNamespace>)currentNode;
            m_namespaceStack.Peek().Add(new Tuple<string, XamlXmlNamespace>(addNamespaceNode.Values.Item1, addNamespaceNode.Values.Item2));
        }

        // Consumes top of namespace stack
        private void PostOrderVisitEndInitPopScope(XbfNode currentNode)
        {
            var currentObject = m_objectStack.Peek();
            var namespaces = m_namespaceStack.Pop();

            var currentObjectAsXamlObject = currentObject as XamlObject;
            if (currentObjectAsXamlObject != null)
            {
                foreach (var ns in namespaces)
                {
                    currentObjectAsXamlObject.AddNamespace(ns.Item1, ns.Item2);
                }
            }
        }

        // Consumes top of namespace stack
        private void PostOrderVisitEndInitProvideValuePopScope(XbfNode currentNode)
        {
            var currentObject = m_objectStack.Peek();
            var namespaces = m_namespaceStack.Pop();

            var currentObjectAsXamlObject = currentObject as XamlObject;
            if (currentObjectAsXamlObject != null)
            {
                foreach (var ns in namespaces)
                {
                    currentObjectAsXamlObject.AddNamespace(ns.Item1, ns.Item2);
                }
            }
        }

        // Consumes top of object stack
        private void PostOrderVisitAddToCollection(XbfNode currentNode)
        {
            // Prefer the last seen constant value over the top of the object stack
            var value = m_lastSeenConstantValue ?? m_objectStack.Pop();
            m_lastSeenConstantValue = null;
            ((XamlObject)m_objectStack.Peek()).AddCollectionItem(value);
            ((XamlObject)m_objectStack.Peek()).Type.IsCollection = true;
        }

        // Consumes top of object stack
        private void PostOrderVisitAddToDictionary(XbfNode currentNode)
        {
            var value = m_objectStack.Pop() as XamlObject;
            object key;

            if (value != null)
            {
                if (value.Type.Equals(XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.Style)))
                {
                    key = value.PropertyStore[XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.Style_TargetType)];
                }
                else
                {
                    key = value.PropertyStore[XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.DependencyObject_Name)];
                }
            }
            else
            {
                throw new Exception();
            }

            ((XamlObject)m_objectStack.Peek()).AddDictionaryItem(key, value);
            ((XamlObject)m_objectStack.Peek()).Type.IsDictionary = true;
        }

        // Consumes top of object stack
        private void PostOrderVisitAddToDictionaryWithKey(XbfNode currentNode)
        {
            var addToDictionaryWithKeyNode = (XbfNode<string, object>)currentNode;
            var key = (string)addToDictionaryWithKeyNode.Values.Item2;
            var value = m_objectStack.Pop();

            ((XamlObject)m_objectStack.Peek()).AddDictionaryItem(key, value);
            ((XamlObject)m_objectStack.Peek()).Type.IsDictionary = true;
        }

        private void PostOrderVisitCheckPeerType(XbfNode currentNode)
        {
            var checkPeerTypeNode = (XbfNode<string>)currentNode;
            m_rootXClass = checkPeerTypeNode.Values.Item1;
        }

        // Creates a new XamlObject and pushes it onto the object stack
        private void PostOrderVisitCreateTypeBeginInit(XbfNode currentNode)
        {
            var createTypeBeginInitNode = (XbfNode<XamlType>)currentNode;
            var xamlType = createTypeBeginInitNode.Values.Item1;

            var xamlObject = XamlObjectFactory.CreateObjectFromXamlType(xamlType);
            m_objectStack.Push(xamlObject);
        }

        // Copies the constant value from the input node and pushes it onto the object stack
        private void PostOrderVisitCreateTypeWithConstantBeginInit(XbfNode currentNode)
        {
            var createTypeWithConstantBeginInitNode = (XbfNode<XamlType, string, object>)currentNode;
            var value = createTypeWithConstantBeginInitNode.Values.Item3;

            m_objectStack.Push(value);
        }

        // Creates a TypeConvertedValue and pushes it onto the object stack
        private void PostOrderVisitCreateTypeWithTypeConvertedConstantBeginInit(XbfNode currentNode)
        {
            var createTypeWithTypeConvertedConstantBeginInitNode = (XbfNode<XamlType, string, object>)currentNode;
            var targetType = createTypeWithTypeConvertedConstantBeginInitNode.Values.Item1;
            var value = createTypeWithTypeConvertedConstantBeginInitNode.Values.Item3;

            var typeConvertedValue = new TypeConvertedValue(targetType, value);
            m_objectStack.Push(typeConvertedValue);
        }

        private void PostOrderVisitEndConditionalScope(XbfNode currentNode)
        {
            // TODO: implement true support for conditional XAML
            // no-op
        }

        // Penultimate item on object stack must be XamlObject
        private void PostOrderVisitGetResourcePropertyBag(XbfNode currentNode)
        {
            var getResourcePropertyBagNode = (XbfNode<string, object>)currentNode;
            var xUid = (string)getResourcePropertyBagNode.Values.Item2;

            ((XamlObject)m_objectStack.Peek()).SetValue(XamlPropertyRegistry.Instance.GetPropertyByName(null, "x:Uid"), xUid);
        }

        // Removes top of namespaces stack, modifies top of object stack if it is
        // a XamlObject
        private void PostOrderVisitPopScope(XbfNode currentNode)
        {
            var currentObject = m_objectStack.Peek();
            var namespaces = m_namespaceStack.Pop();

            var currentObjectAsXamlObject = currentObject as XamlObject;
            if (currentObjectAsXamlObject != null)
            {
                foreach (var ns in namespaces)
                {
                    currentObjectAsXamlObject.AddNamespace(ns.Item1, ns.Item2);
                }
            }
        }

        // Pushes object onto object stack
        private void PostOrderVisitPushConstant(XbfNode currentNode)
        {
            // A bug in the parser during XBF generation results in a CheckPeerType
            // node failing to be emitted for the 'x:Class' directive if there is
            // also a 'x:ConnectionId' directive present (e.g. x:Bind is present in 
            // the markup); code inspection suggests that this will also happen if the
            // 'x:Uid' directive is present.
            // To deal with this, we'll employ the following heuristic:
            // If there is only one object and only one namespace scope has been created, 
            // then transform the PushConstant node into a CheckPeerTypeNode and Visit it.
            var pushConstantNode = (XbfNode<string, object>)currentNode;
            if (m_namespaceScopeCreationCount == 1 && m_objectStack.Count == 1)
            {
                var checkPeerTypeNode = new XbfNode<string>(XbfNodeType.CheckPeerType, currentNode.NodeStreamOffset,
                    (string)pushConstantNode.Values.Item2);
                PostOrderVisitCheckPeerType(checkPeerTypeNode);
            }
            else
            {
                // Just ignore the node entirely.
                // PushConstant nodes should only exist right before an AddToCollection node
                // (and ideally we'd have an optimized node to replace that node pair), but sometimes
                // they can get emitted anyway (e.g. event handler subscription that XamlCompiler
                // didn't strip out because the element wasn't assigned a x:ConnectionId). 
                // We don't want to push them onto the stack because
                // they'll mess things up if they *shouldn't* have been emitted, but we still need
                // to remember them so they can be consumed in legitimate scenarios.
                m_lastSeenConstantValue = pushConstantNode.Values.Item2;
            }
        }

        // Consumes top of object stack
        // Penultimate item on object stack must be XamlObject
        private void PostOrderVisitSetValue(XbfNode currentNode)
        {
            var setValueNode = (XbfNode<XamlProperty>)currentNode;
            var xamlProperty = setValueNode.Values.Item1;
            var value = m_objectStack.Pop();

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, value);
        }

        // Consumes top of object stack, which must be a XamlObject and whose type
        // will be marked as a markup extension
        // Penultimate item on object stack must be XamlObject
        private void PostOrderVisitSetValueFromMarkupExtension(XbfNode currentNode)
        {
            var setValueFromMarkupExtensionNode = (XbfNode<XamlProperty>)currentNode;
            var xamlProperty = setValueFromMarkupExtensionNode.Values.Item1;
            var markupExtension = (XamlObject)m_objectStack.Pop();
            markupExtension.Type.IsMarkupExtension = true;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, markupExtension);
        }

        // Modifies top of object stack if it is a XamlObject
        private void PostOrderVisitSetConnectionId(XbfNode currentNode)
        {
            var setConnectionIdNode = (XbfNode<string, object>)currentNode;
            var xamlObject = m_objectStack.Peek() as XamlObject;
            var connectionId = setConnectionIdNode.Values.Item2;

            xamlObject?.SetValue(XamlPropertyRegistry.Instance.GetPropertyByName(null, "x:ConnectionId"), connectionId);
        }

        // Modifies top of object stack if it is a XamlObject
        // Registers the top of the object stack with the specified name in the namescope
        private void PostOrderVisitSetName(XbfNode currentNode)
        {
            var setNameNode = (XbfNode<string, object>)currentNode;
            var name = (string)setNameNode.Values.Item2;
            var namedObject = m_objectStack.Peek();
            var xamlObject = namedObject as XamlObject;

            xamlObject?.SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.DependencyObject_Name), name);
            m_namescope[name] = namedObject;
        }

        // Realizes the object represented by the specified nodestream, and sets it as the value of the specified
        // property on the object (must be XamlObject) on the top of the object stack
        private void PostOrderVisitSetDeferredProperty(XbfNode currentNode)
        {
            var setDeferredPropertyNode = (XbfNode<XamlProperty, int, List<string>, List<string>>)currentNode;
            var xamlProperty = setDeferredPropertyNode.Values.Item1;

            // Realize the deferred object
            var objectWriter = new ObjectWriter(m_xbfFile, CaptureContext());
            var content = objectWriter.ProcessXbfFile(setDeferredPropertyNode.Values.Item2);

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, content);
        }

        private void PostOrderVisitSetCustomRuntimeData(XbfNode currentNode)
        {
            var setCustomRuntimeDataNode = (XbfNode<CustomRuntimeData, int>)currentNode;

            switch (setCustomRuntimeDataNode.Values.Item1.Version)
            {
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v1:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v2:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v3:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v4:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v5:
                    {
                        SetVisualStateGroupCollectionCustomRuntimeData(setCustomRuntimeDataNode);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.Style_v1:
                case CustomWriterRuntimeDataTypeIndex.Style_v2:
                    {
                        SetStyleCustomRuntimeData(setCustomRuntimeDataNode);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v1:
                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v2:
                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v3:
                    {
                        SetDeferredElementCustomRuntimeData(setCustomRuntimeDataNode);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v1:
                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v2:
                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v3:
                    {
                        SetResourceDictionaryCustomRuntimeData(setCustomRuntimeDataNode);
                    }
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueConstant(XbfNode currentNode)
        {
            var setValueConstantNode = (XbfNode<XamlProperty, string, object>)currentNode;
            var xamlProperty = setValueConstantNode.Values.Item1;
            var value = setValueConstantNode.Values.Item3;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, value);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueTypeConvertedConstant(XbfNode currentNode)
        {
            var setValueTypeConvertedConstantNode = (XbfNode<XamlProperty, string, object>)currentNode;
            var xamlProperty = setValueTypeConvertedConstantNode.Values.Item1;
            var value = setValueTypeConvertedConstantNode.Values.Item3;
            var typeConvertedValue = new TypeConvertedValue(xamlProperty.PropertyType, value);

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, typeConvertedValue);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueTypeConvertedResolvedType(XbfNode currentNode)
        {
            var setValueTypeConvertedResolvedTypeNode = (XbfNode<XamlProperty, XamlType>)currentNode;
            var xamlProperty = setValueTypeConvertedResolvedTypeNode.Values.Item1;
            var resolvedXamlType = setValueTypeConvertedResolvedTypeNode.Values.Item2;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, resolvedXamlType);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueTypeConvertedResolvedProperty(XbfNode currentNode)
        {
            var setValueTypeConvertedResolvedPropertyNode = (XbfNode<XamlProperty, XamlProperty>)currentNode;
            var xamlProperty = setValueTypeConvertedResolvedPropertyNode.Values.Item1;
            var resolvedXamlProperty = setValueTypeConvertedResolvedPropertyNode.Values.Item2;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, resolvedXamlProperty);
        }

        // Creates a XamlStaticResourceExtensions and pushes it onto the object stack
        private void PostOrderVisitProvideStaticResourceValue(XbfNode currentNode)
        {
            var provideStaticResourceValueNode = (XbfNode<string, object>)currentNode;
            var key = (string)provideStaticResourceValueNode.Values.Item2;

            var extension = XamlObjectFactory.CreateXamlStaticResourceExtension();
            extension.ResourceKey = key;

            m_objectStack.Push(extension);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueFromStaticResource(XbfNode currentNode)
        {
            var setValueFromStaticResourceNode = (XbfNode<XamlProperty, string, object>)currentNode;
            var xamlProperty = setValueFromStaticResourceNode.Values.Item1;
            var key = (string)setValueFromStaticResourceNode.Values.Item3;

            var extension = XamlObjectFactory.CreateXamlStaticResourceExtension();
            extension.ResourceKey = key;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, extension);
        }

        // Creates a XamlThemeResourceExtensions and pushes it onto the object stack
        private void PostOrderVisitProvideThemeResourceValue(XbfNode currentNode)
        {
            var provideThemeResourceValueNode = (XbfNode<string, object>)currentNode;
            var key = (string)provideThemeResourceValueNode.Values.Item2;

            var extension = XamlObjectFactory.CreateXamlThemeResourceExtension();
            extension.ResourceKey = key;

            m_objectStack.Push(extension);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueFromThemeResource(XbfNode currentNode)
        {
            var setValueFromThemeResourceNode = (XbfNode<XamlProperty, string, object>)currentNode;
            var xamlProperty = setValueFromThemeResourceNode.Values.Item1;
            var key = (string)setValueFromThemeResourceNode.Values.Item3;

            var extension = XamlObjectFactory.CreateXamlThemeResourceExtension();
            extension.ResourceKey = key;

            ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, extension);
        }

        // Modifies top of object stack, which must be a XamlObject
        private void PostOrderVisitSetValueFromTemplateBinding(XbfNode currentNode)
        {
            var setValueFromTemplateBindingNode = (XbfNode<XamlProperty, XamlProperty>)currentNode;
            var targetProperty = setValueFromTemplateBindingNode.Values.Item1;
            var value = setValueFromTemplateBindingNode.Values.Item2;

            var templateBinding = XamlObjectFactory.CreateXamlTemplateBindingExtension();
            templateBinding.Property = value;

            ((XamlObject)m_objectStack.Peek()).SetValue(targetProperty, templateBinding);
        }

        // Pushes empty namespace scope onto namespace stack
        private void PreOrderVisitPushScope(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;
        }

        private void PostOrderVisitPushScope(XbfNode currentNode)
        {
            // no-op
        }

        // Pushes empty namespace scope onto namespace stack and adds a namespace to it
        private void PreOrderVisitPushScopeAddNamespace(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;

            var addNamespaceNode = (XbfNode<string, XamlXmlNamespace>)currentNode;
            m_namespaceStack.Peek().Add(new Tuple<string, XamlXmlNamespace>(addNamespaceNode.Values.Item1, addNamespaceNode.Values.Item2));
        }

        private void PostOrderVisitPushScopeAddNamespace(XbfNode currentNode)
        {
            // no-op
        }

        // Pushes empty namespace scope onto namespace stack, and then retrieves
        // the value of the specified property from the object of the top of the object stack
        // (creating it if necessary) and pushes that onto the object stack.
        private void PreOrderVisitPushScopeGetValue(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;

            var pushScopeGetValueNode = (XbfNode<XamlProperty>)currentNode;
            var xamlProperty = pushScopeGetValueNode.Values.Item1;

            object value;
            if (!((XamlObject)m_objectStack.Peek()).PropertyStore.TryGetValue(xamlProperty, out value))
            {
                // GetValue nodes are only emitted for properties that are a XamlObject type.
                value = XamlObjectFactory.CreateObjectFromXamlType(xamlProperty.PropertyType);
                ((XamlObject)m_objectStack.Peek()).SetValue(xamlProperty, value);
            }

            m_objectStack.Push(value);
        }

        // Pops the top of the object stack
        private void PostOrderVisitPushScopeGetValue(XbfNode currentNode)
        {
            m_objectStack.Pop();
        }

        // Pushes empty namespace scope onto namespace stack, then creates an object of the
        // specified type and pushes it onto the object stack
        private void PreOrderVisitPushScopeCreateTypeBeginInit(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;

            var pushScopeCreateTypeBeginInitNode = (XbfNode<XamlType>)currentNode;
            var xamlType = pushScopeCreateTypeBeginInitNode.Values.Item1;

            var xamlObject = XamlObjectFactory.CreateObjectFromXamlType(xamlType);
            m_objectStack.Push(xamlObject);
        }

        private void PostOrderVisitPushScopeCreateTypeBeginInit(XbfNode currentNode)
        {
            // no-op
        }

        // Pushes empty namespace scope onto namespace stack
        // Pushes a new object onto the object stack
        private void PreOrderVisitPushScopeCreateTypeWithConstantBeginInit(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;

            var pushScopeCreateTypeWithConstantBeginInitNode = (XbfNode<XamlType, string, object>)currentNode;
            var value = pushScopeCreateTypeWithConstantBeginInitNode.Values.Item3;

            m_objectStack.Push(value);
        }

        private void PostOrderVisitPushScopeCreateTypeWithConstantBeginInit(XbfNode currentNode)
        {
            // no-op
        }

        // Pushes empty namespace scope onto namespace stack
        // Pushes a new object onto the object stack
        private void PreOrderVisitPushScopeCreateTypeWithTypeConvertedConstantBeginInit(XbfNode currentNode)
        {
            m_namespaceStack.Push(new List<Tuple<string, XamlXmlNamespace>>());
            ++m_namespaceScopeCreationCount;

            var pushScopeCreateTypeWithTypeConvertedConstantBeginInitNode = (XbfNode<XamlType, string, object>)currentNode;
            var xamlType = pushScopeCreateTypeWithTypeConvertedConstantBeginInitNode.Values.Item1;
            var value = pushScopeCreateTypeWithTypeConvertedConstantBeginInitNode.Values.Item3;
            var typeConvertedValue = new TypeConvertedValue(xamlType, value);

            m_objectStack.Push(typeConvertedValue);
        }

        private void PostOrderVisitPushScopeCreateTypeWithTypeConvertedConstantBeginInit(XbfNode currentNode)
        {
            // no-op
        }

        private void PreOrderVisitBeginConditionalScope(XbfNode currentNode)
        {
            // TODO: implement true support for conditional XAML. For now just indicate it was encountered.
            ConditionalXamlEncountered = true;
        }

        private void PostOrderVisitBeginConditionalScope(XbfNode currentNode)
        {
            // TODO: implement true support for conditional XAML
            // no-op
        }

        #endregion

        private static bool IsLeafNode(XbfNode node)
        {
            switch (node.NodeType)
            {
                case XbfNodeType.AddNamespace:
                case XbfNodeType.EndInitPopScope:
                case XbfNodeType.EndInitProvideValuePopScope:
                case XbfNodeType.AddToCollection:
                case XbfNodeType.AddToDictionary:
                case XbfNodeType.AddToDictionaryWithKey:
                case XbfNodeType.CheckPeerType:
                case XbfNodeType.CreateTypeBeginInit:
                case XbfNodeType.CreateTypeWithConstantBeginInit:
                case XbfNodeType.CreateTypeWithTypeConvertedConstantBeginInit:
                case XbfNodeType.EndConditionalScope:
                case XbfNodeType.GetResourcePropertyBag:
                case XbfNodeType.PopScope:
                case XbfNodeType.PushConstant:
                case XbfNodeType.SetValue:
                case XbfNodeType.SetValueFromMarkupExtension:
                case XbfNodeType.SetConnectionId:
                case XbfNodeType.SetName:
                case XbfNodeType.SetDeferredProperty:
                case XbfNodeType.SetCustomRuntimeData:
                case XbfNodeType.SetValueConstant:
                case XbfNodeType.SetValueTypeConvertedConstant:
                case XbfNodeType.SetValueTypeConvertedResolvedType:
                case XbfNodeType.SetValueTypeConvertedResolvedProperty:
                case XbfNodeType.ProvideStaticResourceValue:
                case XbfNodeType.SetValueFromStaticResource:
                case XbfNodeType.ProvideThemeResourceValue:
                case XbfNodeType.SetValueFromThemeResource:
                case XbfNodeType.SetValueFromTemplateBinding:
                case XbfNodeType.ProvideValue:
                    {
                        return true;
                    }
                case XbfNodeType.PushScope:
                case XbfNodeType.PushScopeAddNamespace:
                case XbfNodeType.PushScopeGetValue:
                case XbfNodeType.PushScopeCreateTypeBeginInit:
                case XbfNodeType.PushScopeCreateTypeWithConstantBeginInit:
                case XbfNodeType.PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
                case XbfNodeType.BeginConditionalScope:
                    {
                        return false;
                    }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private Stack<List<Tuple<string, XamlXmlNamespace>>> m_namespaceStack = new Stack<List<Tuple<string, XamlXmlNamespace>>>();
        private Stack<object> m_objectStack = new Stack<object>();
        private readonly Dictionary<string, object> m_namescope = new Dictionary<string, object>();
        private readonly SavedContext m_savedContext = new SavedContext();
        private readonly XbfFile m_xbfFile;
        private string m_rootXClass;

        private int m_namespaceScopeCreationCount = 0;
        private object m_lastSeenConstantValue;
    }
}
