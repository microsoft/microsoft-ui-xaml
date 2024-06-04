// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using MS.Internal.Xaml.Context;
using System.Xaml.Schema;
using System.Xaml.MS.Impl;

namespace System.Xaml
{
    internal enum DeferringMode
    {
        Off,
        TemplateStarting,
        TemplateDeferring,
        TemplateReady,
    }

    internal class DeferringWriter : XamlWriter, IXamlLineInfoConsumer
    {
        DeferringMode _mode;
        bool _handled;
        ObjectWriterContext _context;
        XamlNodeList _deferredList;
        XamlWriter _deferredWriter;
        int _deferredTreeDepth;

        public DeferringWriter(ObjectWriterContext context)
        {
            _context = context;
            _mode = DeferringMode.Off;
        }

        public void Clear()
        {
            _handled = false;
            _mode = DeferringMode.Off;
            _deferredList = null;
            _deferredTreeDepth = -1;
        }

        public bool Handled
        {
            get { return _handled; }
        }

        public DeferringMode Mode
        {
            get { return _mode; }
        }

        public XamlNodeList CollectTemplateList()
        {
            XamlNodeList retValue = _deferredList;
            _deferredList = null;
            _mode = DeferringMode.Off;
            return retValue;
        }

        #region XamlWriter Members

        public override void WriteGetObject()
        {
            WriteObject(null, true, "WriteGetObject");
        }

        public override void WriteStartObject(XamlType xamlType)
        {
            WriteObject(xamlType, false, "WriteStartObject");
        }

        void WriteObject(XamlType xamlType, bool fromMember, string methodName)
        {
            _handled = false;
            switch (_mode)
            {
                case DeferringMode.Off:
                    break;

                case DeferringMode.TemplateReady:
                    throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, methodName));

                case DeferringMode.TemplateStarting:
                    StartDeferredList();
                    _mode = DeferringMode.TemplateDeferring;
                    goto case DeferringMode.TemplateDeferring;

                case DeferringMode.TemplateDeferring:
                    if (fromMember)
                    {
                        _deferredWriter.WriteGetObject();
                    }
                    else
                    {
                        _deferredWriter.WriteStartObject(xamlType);
                    }
                    _deferredTreeDepth += 1;
                    _handled = true;
                    break;

                default:
                    throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), methodName));
            }
        }

        public override void WriteEndObject()
        {
            _handled = false;
            switch (_mode)
            {
            case DeferringMode.Off:
                break;

            case DeferringMode.TemplateReady:
                throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, "WriteEndObject"));

            case DeferringMode.TemplateDeferring:
                _deferredWriter.WriteEndObject();
                _handled = true;
                _deferredTreeDepth -= 1;

                if (_deferredTreeDepth == 0)
                {
                    _deferredWriter.Close();
                    _deferredWriter = null;
                    _mode = DeferringMode.TemplateReady;
                }
                break;

            default:
                throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), "WriteEndObject"));
            }
        }

        public override void WriteStartMember(XamlMember property)
        {
            _handled = false;
            switch (_mode)
            {
            case DeferringMode.Off:
                if (property.DeferringLoader != null)
                {
                    _mode = DeferringMode.TemplateStarting;

                    // We assume in WriteValue that this property can never be multi-valued
                    Debug.Assert(!property.IsDirective && !property.IsUnknown);
                }
                break;

            case DeferringMode.TemplateReady:
                throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, "WriteMember"));

            case DeferringMode.TemplateDeferring:
                _deferredWriter.WriteStartMember(property);
                _handled = true;
                break;

            default:
                throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), "WriteMember"));
            }
        }

        public override void WriteEndMember()
        {
            _handled = false;
            switch (_mode)
            {
            case DeferringMode.Off:
                break;

            case DeferringMode.TemplateReady:
                throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, "WriteEndMember"));

            case DeferringMode.TemplateDeferring:
                _deferredWriter.WriteEndMember();
                _handled = true;
                break;

            default:
                throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), "WriteEndMember"));
            }
        }

        public override void WriteValue(object value)
        {
            _handled = false;
            switch (_mode)
            {
            case DeferringMode.Off:
                break;

            case DeferringMode.TemplateReady:
                throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, "WriteValue"));

            case DeferringMode.TemplateStarting:
                // This handles the case of SM template; V object; EM
                Debug.Assert(_deferredTreeDepth == 0);
                if (value is XamlNodeList)
                {
                    _deferredList = (XamlNodeList)value;
                    _mode = DeferringMode.TemplateReady;
                    _handled = true;
                }
                else
                {
                    StartDeferredList();
                    _mode = DeferringMode.TemplateDeferring;
                    goto case DeferringMode.TemplateDeferring;
                }
                break;

            case DeferringMode.TemplateDeferring:
                _deferredWriter.WriteValue(value);
                _handled = true;
                break;

            default:
                throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), "WriteValue"));
            }
        }

        public override void  WriteNamespace(NamespaceDeclaration namespaceDeclaration)
        {
            switch (_mode)
            {
            case DeferringMode.Off:
                return;

            case DeferringMode.TemplateReady:
                throw new XamlInternalException(SR.Get(SRID.TemplateNotCollected, "WriteNamespace"));

            case DeferringMode.TemplateStarting:
                StartDeferredList();
                _mode = DeferringMode.TemplateDeferring;
                goto case DeferringMode.TemplateDeferring;

            case DeferringMode.TemplateDeferring:
                _deferredWriter.WriteNamespace(namespaceDeclaration);
                _handled = true;
                break;

            default:
                throw new XamlInternalException(SR.Get(SRID.MissingCase, _mode.ToString(), "WriteNamespace"));
            }
        }

        protected override void Dispose(bool disposing)
        {
            try
            {
                if (disposing && !IsDisposed)
                {
                    if (_deferredWriter != null)
                    {
                        _deferredWriter.Close();
                        _deferredWriter = null;
                    }
                }
            }
            finally
            {
                base.Dispose(disposing);
            }
        }

        public override XamlSchemaContext SchemaContext
        {
            get { return _context.SchemaContext; }
        }

        #endregion

        #region IConsumeXamlLineInfo Members

        // These SHOULD be implemented.

        public void SetLineInfo(int lineNumber, int linePosition)
        {
            throw new NotImplementedException();
        }

        public bool ShouldProvideLineInfo
        {
            get { throw new NotImplementedException(); }
        }

        #endregion

        private void StartDeferredList()
        {
            _deferredList = new XamlNodeList(_context.SchemaContext);
            _deferredWriter = _deferredList.Writer;
            _deferredTreeDepth = 0;
        }
    }
}
