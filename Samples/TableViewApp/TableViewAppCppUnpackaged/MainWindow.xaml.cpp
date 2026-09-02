#include "pch.h"
#include "MainWindow.xaml.h"
#include "Person.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls::Tabular;

namespace winrt::TableViewAppCppUnpackaged::implementation
{
    namespace
    {
        TableViewAppCppUnpackaged::Person MakePerson(hstring const& name, int32_t age)
        {
            auto person = winrt::make<implementation::Person>();
            person.Name(name);
            person.Age(age);
            return person;
        }
    }

    TableViewTemplateColumn MainWindow::MakeColumn(hstring const& header, hstring const& templateKey)
    {
        TableViewTemplateColumn column;
        column.Header(box_value(header));
        column.CellTemplate(RootGrid().Resources().Lookup(box_value(templateKey)).as<DataTemplate>());
        return column;
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
        codeTable.Columns().Append(MakeColumn(L"Name", L"NameCell"));
        codeTable.Columns().Append(MakeColumn(L"Age", L"AgeCell"));
        CodeTableHost().Children().Append(codeTable);

        StatusText().Text(L"Both tables are bound to the same 3 items.");
    }

    void MainWindow::ToggleTheme_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto currentTheme = RootGrid().RequestedTheme();
        RootGrid().RequestedTheme(currentTheme == ElementTheme::Dark ? ElementTheme::Light : ElementTheme::Dark);
    }
}