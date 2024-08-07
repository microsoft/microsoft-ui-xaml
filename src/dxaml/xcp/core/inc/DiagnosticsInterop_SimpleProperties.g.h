// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include <SimpleProperty.h>
#include <StringConversions.h>
#include <windows.foundation.numerics.h>
#include <Value.h>
#include <CValueBoxer.h>
#include <SimplePropertiesCommon.g.h>

// Our generated property types are the right ones publicly, but our internal storage may differ.  For instance, UIElement.Rotation
// is publicly a float, but internally stored and retrieved as a double.  We disable C4244 for this code, which covers the double -> float
// precision loss.
 #pragma warning( push )
 #pragma warning( disable: 4244 )

// Helper method that allows DiagnosticsInterop to get and set simple properties
namespace Diagnostics {
    bool IsCreatableSimplePropertyType(const CClassInfo* type)
    {
        switch (type->GetIndex())
        {
            case KnownTypeIndex::TimeSpan:
            case KnownTypeIndex::Vector3:
            case KnownTypeIndex::Matrix4x4:
            case KnownTypeIndex::Vector2:
            case KnownTypeIndex::Matrix3x2:
            case KnownTypeIndex::Quaternion:
                return true;
            default:
                return false;
        }
    }

    HRESULT CreateSimplePropertyType(const CClassInfo* type, const xstring_ptr_view& value, IInspectable** created)
    {
        *created = nullptr;

        switch (type->GetIndex())
        {
            case KnownTypeIndex::TimeSpan:
            {
                ABI::Windows::Foundation::TimeSpan typeConvertedValue{};
                IFC_RETURN(TimeSpanFromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::TimeSpan>(typeConvertedValue, created));
                break;
            }
            case KnownTypeIndex::Vector3:
            {
                ABI::Windows::Foundation::Numerics::Vector3 typeConvertedValue{};
                IFC_RETURN(Vector3FromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector3>(typeConvertedValue, created));
                break;
            }
            case KnownTypeIndex::Matrix4x4:
            {
                ABI::Windows::Foundation::Numerics::Matrix4x4 typeConvertedValue{};
                IFC_RETURN(Matrix4x4FromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Matrix4x4>(typeConvertedValue, created));
                break;
            }
            case KnownTypeIndex::Vector2:
            {
                ABI::Windows::Foundation::Numerics::Vector2 typeConvertedValue{};
                IFC_RETURN(Vector2FromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector2>(typeConvertedValue, created));
                break;
            }
            case KnownTypeIndex::Matrix3x2:
            {
                ABI::Windows::Foundation::Numerics::Matrix3x2 typeConvertedValue{};
                IFC_RETURN(Matrix3x2FromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Matrix3x2>(typeConvertedValue, created));
                break;
            }
            case KnownTypeIndex::Quaternion:
            {
                ABI::Windows::Foundation::Numerics::Quaternion typeConvertedValue{};
                IFC_RETURN(QuaternionFromString(value, typeConvertedValue));
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Quaternion>(typeConvertedValue, created));
                break;
            }
            default:
                MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
                return E_FAIL;
        }
        return S_OK;
    }

    HRESULT SetValueSimpleProperty(const CPropertyBase* property, SimpleProperty::objid_t obj, IInspectable* pValue)
    {
        switch (property->GetIndex())
        {
            case KnownPropertyIndex::BrushTransition_Duration:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>> referencePropValue;
                ABI::Windows::Foundation::TimeSpan propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::BrushTransition_Duration>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::ScalarTransition_Duration:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>> referencePropValue;
                ABI::Windows::Foundation::TimeSpan propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::ScalarTransition_Duration>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_CenterPoint:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector3 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_KeepAliveCount:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<INT>> referencePropValue;
                INT propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<INT>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_RasterizationScale:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<DOUBLE>> referencePropValue;
                DOUBLE propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<DOUBLE>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RasterizationScale>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_Rotation:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<FLOAT>> referencePropValue;
                FLOAT propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<FLOAT>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_RotationAxis:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector3 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_Scale:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector3 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_ThemeShadowReceiverCount:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<INT>> referencePropValue;
                INT propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<INT>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_TransformMatrix:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Matrix4x4>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Matrix4x4 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Matrix4x4>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::UIElement_Translation:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector3 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector3>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::Vector3Transition_Components:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Microsoft::UI::Xaml::Vector3TransitionComponents>> referencePropValue;
                ABI::Microsoft::UI::Xaml::Vector3TransitionComponents propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Microsoft::UI::Xaml::Vector3TransitionComponents>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Components>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::Vector3Transition_Duration:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>> referencePropValue;
                ABI::Windows::Foundation::TimeSpan propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Duration>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::LinearGradientBrush_CenterPoint:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector2 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::LinearGradientBrush_Rotation:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<DOUBLE>> referencePropValue;
                DOUBLE propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<DOUBLE>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::LinearGradientBrush_Scale:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector2 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::LinearGradientBrush_TransformMatrix:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Matrix3x2>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Matrix3x2 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Matrix3x2>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Set(obj, propertyValue);
                break;
             }

            case KnownPropertyIndex::LinearGradientBrush_Translation:
            {
                wrl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>> referencePropValue;
                ABI::Windows::Foundation::Numerics::Vector2 propertyValue;
                if (SUCCEEDED(pValue->QueryInterface<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Numerics::Vector2>>(&referencePropValue)))
                {
                    IFC_RETURN(referencePropValue->get_Value(&propertyValue));
                }
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Set(obj, propertyValue);
                break;
             }

            default:
                MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
                return E_FAIL;
        }

        return S_OK;
    }

    HRESULT GetValueSimpleProperty(const CPropertyBase* property, SimpleProperty::const_objid_t obj, _Outptr_ IInspectable** ppValue)
    {
        // If you see an error that a Box___ method is missing, it's likely due to a new simple property
        // using a type that isn't in the Box helper methods yet.  You'll need to define it earlier up this file.
        switch (property->GetIndex())
        {
            case KnownPropertyIndex::BrushTransition_Duration:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::TimeSpan>(SimpleProperty::Property::id<KnownPropertyIndex::BrushTransition_Duration>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::ScalarTransition_Duration:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::TimeSpan>(SimpleProperty::Property::id<KnownPropertyIndex::ScalarTransition_Duration>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_CenterPoint:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector3>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_KeepAliveCount:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<INT>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_RasterizationScale:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<DOUBLE>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RasterizationScale>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_Rotation:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<FLOAT>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_RotationAxis:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector3>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_Scale:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector3>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_ThemeShadowReceiverCount:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<INT>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_TransformMatrix:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Matrix4x4>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::UIElement_Translation:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector3>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::Vector3Transition_Components:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Microsoft::UI::Xaml::Vector3TransitionComponents>(SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Components>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::Vector3Transition_Duration:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::TimeSpan>(SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Duration>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::LinearGradientBrush_CenterPoint:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector2>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::LinearGradientBrush_Rotation:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<DOUBLE>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::LinearGradientBrush_Scale:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector2>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::LinearGradientBrush_TransformMatrix:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Matrix3x2>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Get(obj), ppValue));
                break;
            }
            case KnownPropertyIndex::LinearGradientBrush_Translation:
            {
                IFC_RETURN(DirectUI::PropertyValue::CreateReference<ABI::Windows::Foundation::Numerics::Vector2>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Get(obj), ppValue));
                break;
            }
            default:
                MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
                return E_FAIL;
        }
        
        return S_OK;
    }

    void ClearSimpleProperty(SimpleProperty::objid_t obj, const CPropertyBase* property)
    {
        switch (property->GetIndex())
        {
            case KnownPropertyIndex::BrushTransition_Duration:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::BrushTransition_Duration>();
                SimpleProperty::Property::id<KnownPropertyIndex::BrushTransition_Duration>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::ScalarTransition_Duration:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::ScalarTransition_Duration>();
                SimpleProperty::Property::id<KnownPropertyIndex::ScalarTransition_Duration>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_CenterPoint:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_CenterPoint>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_KeepAliveCount:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_KeepAliveCount>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_RasterizationScale:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_RasterizationScale>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RasterizationScale>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_Rotation:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_Rotation>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_RotationAxis:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_RotationAxis>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_Scale:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_Scale>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_ThemeShadowReceiverCount:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_TransformMatrix:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_TransformMatrix>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::UIElement_Translation:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_Translation>();
                SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::Vector3Transition_Components:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::Vector3Transition_Components>();
                SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Components>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::Vector3Transition_Duration:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::Vector3Transition_Duration>();
                SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Duration>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::LinearGradientBrush_CenterPoint:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::LinearGradientBrush_CenterPoint>();
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::LinearGradientBrush_Rotation:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::LinearGradientBrush_Rotation>();
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::LinearGradientBrush_Scale:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::LinearGradientBrush_Scale>();
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::LinearGradientBrush_TransformMatrix:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>();
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Set(obj, defaultValue);
                break;
            }

            case KnownPropertyIndex::LinearGradientBrush_Translation:
            {
                const auto defaultValue = SimpleProperty::Property::Default<KnownPropertyIndex::LinearGradientBrush_Translation>();
                SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Set(obj, defaultValue);
                break;
            }

            default:
                MICROSOFT_TELEMETRY_ASSERT_DISABLED(false);
        }
    }
}

#pragma warning(pop)