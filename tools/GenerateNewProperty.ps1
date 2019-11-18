Param(
    [string]$propertyName
    [string]$controlName;
)

$typeFormat1 = "<TYPEGOESHERE-1>"; # future proofing? :P
$typeFormat2 = "<TYPESGOESHERE-2>";

$controlName1 = "<CONTROL-NAME-GOES-HERE-1>";

Write-Host ".idl class";
Write-Host "    [propget] HRESULT $propertyName([out, retval] $($typeFormat1)* value);";
Write-Host "    [propput] HRESULT $propertyName([in] $($typeFormat1) value);";
Write-Host "";
Write-Host ".idl statics class";
Write-Host "    [propget] HRESULT $($propertyName)Property([out, retval] Windows.UI.Xaml.DependencyProperty** value);"
Write-Host "";
Write-Host "Factory.h methods";
Write-Host "    IFACEMETHOD(get_$($propertyName)Property)(abi::IDependencyProperty** value) override;"
Write-Host "";
Write-Host "Factory.h statics";
Write-Host "    static GlobalDependencyProperty s_$($propertyName)Property;";
Write-Host "";
Write-Host "Factory.cpp static definition";
Write-Host "winrt::DependencyProperty $($controlName1)Factory::s_$($propertyName)Property{ nullptr };";
Write-Host "";
Write-Host "Factory.cpp EnsureProperties";
Write-Host "    if (RatingControlFactory::s_$($propertyName)Property == nullptr)
    {
        $($controlName1)Factory::s_$($propertyName)Property =
            InitializeDependencyProperty(
                L"Text",
                L"Windows.Foundation.$($typeFormat2)",
                winrt::name_of<winrt::$($controlName1)>(),
                false /* isAttached */,
                <DEFAULT-VALUE-GOES-HERE>,
                winrt::PropertyChangedCallback(&$($controlName1)Factory::OnPropertyChanged));
    }";
Write-Host ""
Write-Host "Factory.cpp methods";
Write-Host "IFACEMETHODIMP RatingControlFactory::get_$($propertyName)Property(abi::IDependencyProperty** value) try
{
    winrt::copy_to(getAsABI(s_$($propertyName)Property), value);
    CATCH_RETURN;
}"