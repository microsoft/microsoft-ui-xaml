// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class XamlClassFlags
    {
        /// <summary>
        /// Causes generation of 'protected override sealed Size MeasureOverride(Size constraint)',
        /// 'protected override sealed Size ArrangeOverride(Size arrangeSize)' in JoltClasses.g.cs.
        /// Examples: INDEX_GRID / INDEX_STACK_PANEL / INDEX_CANVAS.
        /// </summary>
        public bool AreMeasureAndArrangeSealed { get; set; }

        /// <summary>
        /// Gets or sets a value indicating that this type's peer type in the core
        /// is a EOR, not the specified base type.
        /// </summary>
        public bool CoreTypeIsExternalObjectReference { get; set; }

        /// <summary>
        /// Causes use of managed TypeFlags.HasTypeConverter and native TypeFlags.tfHasTypeConverter when set to True.
        /// Example: ContentControl
        /// </summary>
        public bool CanConvertFromString { get; set; }

        /// <summary>
        /// Forces a InitializeDefaults() function stub to be generated in the core, allowing you to specify default values for generated fields.
        /// </summary>
        public bool ForceCoreFieldInitializer { get; set; }

        /// <summary>
        /// Only used for compat. Blue allowed access to FloatCollection, and we need to make sure that continues to work.
        /// </summary>
        public bool ForceIncludeInManifest { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the type is observable (now it is applicable to Collections only).
        /// Example: INDEX_ITEM_COLLECTION
        /// </summary>
        public bool IsObservable { get; set; }

        /// <summary>
        /// Generates type flag TYPE_HAS_TYPE_CONVERTER in XcpTypes.g.h when set to True.
        /// Example: INDEX_RUN
        /// </summary>
        public bool HasTypeConverter { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the type has a base type in the IDL.
        /// </summary>
        public bool HasBaseTypeInDXamlInterface { get; set; } = true;

        /// <summary>
        /// Determines whether this class is free threaded or not.
        /// </summary>
        public bool IsFreeThreaded { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the type implements the
        /// ICollection interface (or native equivalent).
        /// </summary>
        public bool IsICollection { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the type implements the
        /// IDictionary interface (or native equivalent).
        /// </summary>
        public bool IsIDictionary { get; set; }

        /// <summary>
        /// Gets or sets whether this type should not show up in IDL. When set to true, 
        /// all derived types will derive from this type's base type in IDL. This is useful 
        /// when you want to share code between multiple classes, without exposing a new 
        /// base class.
        /// Example: ListViewBaseItem
        /// </summary>
        public bool IsHiddenFromIdl { get; set; }

        /// <summary>
        /// Generates class flag TYPE_NOT_CREATEABLE_AFTER_V2 in XcpTypes.g.h when set to False.
        /// Example: INDEX_MOUSE_CURSOR
        /// </summary>
        public bool IsCreateableAfterV2 { get; set; } = true;

        /// <summary>
        /// Return True if this class implements ISupportInitialize.
        /// Example: INDEX_SELECTOR
        /// </summary>
        public bool IsISupportInitialize { get; set; }

        /// <summary>
        /// Generates class flag TYPE_IS_MARKUP_EXTENSION in XcpTypes.g.h when set to True.
        /// Example: INDEX_STATIC_RESOURCE_EXTENSION
        /// </summary>
        public bool IsMarkupExtension { get; set; }

        /// <summary>
        /// Generates class flag TYPE_SL4_IS_PUBLIC in XcpTypes.g.h when set to True.
        /// Example: INDEX_VISUAL_STATE_GROUP
        /// </summary>
        public bool IsPublicInSL4 { get; set; }

        /// <summary>
        /// Causes '!' prefix in generated 'struct KnownType.m_pwsz' name, in XcpTypes.g.h.
        /// Example: INDEX_EVENT_HANDLER
        /// </summary>
        public bool IsVisibleInXAML { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether the whitespace is considered
        /// significant when parsing this type in XAML.
        /// </summary>
        public bool IsWhitespaceSignificant { get; set; }

        /// <summary>
        /// Causes '?' prefix in generated 'struct KnownType.m_pwsz' name, in XcpTypes.g.h.
        /// Example: INDEX_ENUMERATED
        /// </summary>
        public bool IsTypeConverter { get; set; }

        /// <summary>
        /// Gets or sets whether the type requires a CCoreServices reference. This is currently only used for a small number of input-related EventArgs such as ManipulationStartedRoutedEventArgs.
        /// </summary>
        public bool RequiresCoreServices { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the whitespace is considered
        /// significant when parsing this type in XAML.
        /// </summary>
        public bool TrimSurroundingWhitespace { get; set; }
    }
}
