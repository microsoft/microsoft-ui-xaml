// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace DependencyObject {

        class MockDependencyObjectWithObjectProperty : public CDependencyObject
        {
        public:
            MockDependencyObjectWithObjectProperty() :
                value(nullptr)
            {
            }

            CDependencyObject* value;
        };

        class MockDependencyObjectWithStringProperty : public CDependencyObject
        {
        public:
            MockDependencyObjectWithStringProperty() :
                value()
            {
            }

            xstring_ptr value;
        };

        class MockDependencyObjectTrackingLayoutFlag : public CDependencyObject
        {
        public:
            MockDependencyObjectTrackingLayoutFlag() :
                affectedMeasure(false),
                affectedArrange(false)
            {
            }

            void PropagateLayoutDirty(bool affectsMeasure, bool affectsArrange) override
            {
                affectedMeasure = affectsMeasure;
                affectedArrange = affectsArrange;
            }

            void Reset()
            {
                affectedMeasure = affectedArrange = false;
            }

            bool affectedMeasure;
            bool affectedArrange;
        };

        class MockDependencyObjectTrackingInvoke : public CDependencyObject
        {
        public:
            MockDependencyObjectTrackingInvoke() :
                calledInvoke(false)
            {
            }

            _Check_return_ HRESULT InvokeImpl(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner) override
            {
                calledInvoke = true;
                return S_OK;
            }

            void Reset()
            {
                calledInvoke = false;
            }

            bool calledInvoke;
        };

        class MockDependencyObjectTrackingEnterLeave : public CDependencyObject
        {
        public:
            MockDependencyObjectTrackingEnterLeave() :
                calledEnter(false),
                calledLeave(false)
            {
            }

            _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params) override
            {
                calledEnter = true;
                return S_OK;
            }

            _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params) override
            {
                calledLeave = true;
                return S_OK;
            }

            void Reset()
            {
                calledEnter = calledLeave = false;
            }

            bool calledEnter;
            bool calledLeave;
        };

        class MockDependencyObjectTrackingThemeChanged : public CDependencyObject
        {
        public:
            MockDependencyObjectTrackingThemeChanged() :
                hasThemeChanged(false)
            {
            }

            _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false)
            {
                hasThemeChanged = true;
                return S_OK;
            }

            bool hasThemeChanged;
        };

        class MockDependencyObjectWithPropertyChangedHandler: public CDependencyObject
        {
        public:
            MockDependencyObjectWithPropertyChangedHandler() = default;

            std::function<HRESULT(const PropertyChangedParams&)> propertyChangedCallback;

        protected:
            _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args)
            {
                if (propertyChangedCallback)
                {
                    IFC_RETURN(propertyChangedCallback(args));
                }
                return CDependencyObject::OnPropertyChanged(args);
            }
        };
    }}
} } } }
