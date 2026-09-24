// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace BindTestbedCXModel
{
	using namespace ::Platform;
	using namespace ::Platform::Collections;
	using namespace ::Windows::Foundation::Collections;
    using namespace ::Microsoft::UI::Xaml;

    [::Windows::Foundation::Metadata::CreateFromString(MethodName = "BindTestbedCXModel.ModelCX.MakeStarCx")]
    public value struct Coordinates
    {
        int x;
        int y;
    };

	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class ModelCX sealed
	{
	public:
		ModelCX();

		property IObservableVector<String^>^ ObservableVectorOfStrings
		{
			IObservableVector<Platform::String^>^ get()
			{
				return m_vectorOfStrings;
			}
		}

        property Coordinates Location
        {
            Coordinates get()
            {
                return m_coords;
            }
        }

		void InitializeValues();
		void UpdateValues();

        static Coordinates MakeStarCx(Platform::String^ args)
        {
            Coordinates ret;
            ret.x = 1;
            ret.y = 2;
            return ret;
        }

	private:
		Vector<String^>^ m_vectorOfStrings;
        Coordinates m_coords;
	};

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class StarCx sealed : FrameworkElement
    {
    private:
        static DependencyProperty^ _CoordsProperty;
        static DependencyProperty^ _TestStringProperty;
    public:
        //static void RegisterDependencyProperties();
        static property DependencyProperty^ CoordsProperty
        {
            DependencyProperty^ get() { return _CoordsProperty; }
        }
        property Coordinates Coords
        {
            Coordinates get() {
                return (Coordinates)GetValue(CoordsProperty);
            }
            void set(Coordinates value) {
                SetValue(CoordsProperty, value);
            }
        }

        static property DependencyProperty^ TestStringProperty
        {
            DependencyProperty^ get() { return _TestStringProperty; }
        }
        property String^ TestString
        {
            String^ get() {
                return (String^)GetValue(TestStringProperty);
            }
            void set(String^ value) {
                SetValue(TestStringProperty, value);
            }
        }
    };
}