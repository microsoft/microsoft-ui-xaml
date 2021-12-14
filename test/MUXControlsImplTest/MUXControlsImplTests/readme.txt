========================================================================
    C++/WinRT MUXControlsImplTests Project Overview
========================================================================

This project demonstrates how to get started authoring Windows Runtime 
classes directly with standard C++, using the C++/WinRT SDK component 
to generate implementation headers from interface (IDL) files.  The
generated static library should be consumed by a single Runtime Component 
or App project and the types will automatically be added to that binary.

To be able to instantiate types from the static library you need to 
update the activation factory in the consuming binary to call the 
activation factory exposed by this static library using code similar to:

void* __stdcall MUXControlsImplTests_get_activation_factory(
    std::wstring_view const& name);

void* __stdcall winrt_get_activation_factory(
    std::wstring_view const& name)
{
    void* factory = MUXControlsImplTests_get_activation_factory(name);
    if (factory)
    {
        return factory;
    }
    
    /* call other activation factories */

    return nullptr;
}

Steps:
1. Create an interface (IDL) file to define your Windows Runtime class, 
    its default interface, and any other interfaces it implements.
2. Build the project once to generate module.g.cpp, module.h.cpp, and
    implementation templates under the "Generated Files" folder, as 
    well as skeleton class definitions under "Generated Files\sources".  
3. Use the skeleton class definitions for reference to implement your
    Windows Runtime classes.

========================================================================
Learn more about C++/WinRT here:
http://aka.ms/cppwinrt/
========================================================================
