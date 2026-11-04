#include "pch.h"
#include "MainWindow.xaml.h"
#include "Person.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls::Tabular;

namespace winrt::TableViewAppCppPackaged::implementation
{
    namespace
    {
        TableViewAppCppPackaged::Person MakePerson(hstring const& name, int32_t age)
        {
            auto person = winrt::make<implementation::Person>();
            person.Name(name);
            person.Age(age);
            return person;
        }

        // Person is [bindable], so classic {Binding} resolves without an IXamlType.
        TableViewTextColumn MakeColumn(hstring const& header, hstring const& path)
        {
            TableViewTextColumn column;
            column.Header(box_value(header));
            Microsoft::UI::Xaml::Data::Binding binding;
            binding.Path(PropertyPath{ path });
            column.Binding(binding);
            return column;
        }
    }


    MainWindow::MainWindow()
    {
        // Populate before InitializeComponent so the markup x:Bind picks it up.
        m_people = single_threaded_observable_vector<Windows::Foundation::IInspectable>();
        m_people.Append(MakePerson(L"Alice", 30));
        m_people.Append(MakePerson(L"Bob", 25));
        m_people.Append(MakePerson(L"Carol", 40));

        InitializeComponent();

        // Second instance built entirely from code, to cover the non-markup activation path.
        auto codeTable = TableView{};
        codeTable.Height(160);
        codeTable.ItemsSource(m_people);
        codeTable.Columns().Append(MakeColumn(L"Name", L"Name"));
        codeTable.Columns().Append(MakeColumn(L"Age", L"Age"));
        CodeTableHost().Children().Append(codeTable);

        StatusText().Text(L"Both tables are bound to the same 3 items.");
    }

    void MainWindow::ToggleTheme_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto currentTheme = RootGrid().RequestedTheme();
        RootGrid().RequestedTheme(currentTheme == ElementTheme::Dark ? ElementTheme::Light : ElementTheme::Dark);
    }
}