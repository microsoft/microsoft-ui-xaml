"use strict";

// Description:
//    This file is a JavaScript debugger extension for WinDbg. Commands and data
//    exposed by this extension make it easy to find error codes and callstacks
//    for stowed exception stacks (exception code 0xC000027B), inspect the state
//    of core XAML objects on the thread, and more.
//
// Usage:
//    (1) Save this file to a convenient location, such as C:\temp\winui-dbgext.js
//    (2) With WinDbg either attached to a running process or debugging a dump
//        file, load this extension (replace path\filename as appropriate):
//           .scriptrun c:\temp\winui-dbgext.js
//    (3) Use the added commands and data, such as: !xamltriage
//
//    For some sample commands, run "!xamltools" and click the "[Show help]" link.

function GetTLSPointer(thread, module, variableName)
{
    try
    {
        var tlsSlot = host.getModuleSymbol(module, variableName, "int");

        if (tlsSlot < 64)
        {
            // TODO: check thread.NativeEnvironment if running WOW64
            return thread.Environment.EnvironmentBlock.TlsSlots[tlsSlot];
        }
        else
        {
            return thread.Environment.EnvironmentBlock.TlsExpansionSlots[tlsSlot - 64];
        }
    }
    catch (ex)
    {
    }
    return null;
}

var __showedModuleError = false;

function GetWinUI3MUXModule()
{
    var modules = host.currentProcess.Modules.Where(function(m)
        {
            if (m.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll"))
            {
                try
                {
                    return !m.Contents.Version.VersionInfo.FileVersion.startsWith("2.");
                }
                catch (e)
                {
                    if (!__showedModuleError)
                    {
                        host.diagnostics.debugLog("Unable to determine if this is WinUI2 or WinUI3: " + e + "\n");
                        host.diagnostics.debugLog("    Assuming WinUI3.\n");
                        __showedModuleError = true;
                    }
                    return true;
                }
            }
            return false;
        });
    if (modules.Count() == 0)
    {
        return null;
    }
    if (modules.Count() > 1)
    {
        host.diagnostics.debugLog(" Warning: GetWinUI3MUXModule seeing multiple non-WinUI2 microsoft.ui.xaml.dlls: " + modules.Count() + "\n");
    }
    return modules.First();
}

function GetSystemWUXModule()
{
    var modules = host.currentProcess.Modules.Where(function(m)
        {
            return m.Name.toLowerCase().endsWith("windows.ui.xaml.dll");
        });
    if (modules.Count() == 0)
    {
        return null;
    }
    return modules.First();
}

function XAML_DLL_Module_from_thread(thread, silentOnNonXAML)
{
    var ptr;

    var winui3MUXModule = GetWinUI3MUXModule();
    if (winui3MUXModule != null)
    {
        ptr = GetTLSPointer(thread, winui3MUXModule, "DXamlInstanceStorage::g_dwTlsIndex");
        if (ptr != null && !ptr.isNull)
        {
            return winui3MUXModule;
        }
        // It is possible XAML isn't initialized on the thread but there may still be stowed
        // exceptions, so check for that as well.
        ptr = GetTLSPointer(thread, winui3MUXModule, "g_dwErrorContextTlsIndex");
        if (ptr != null && !ptr.isNull)
        {
            return winui3MUXModule;
        }
    }

    var wuxModule = GetSystemWUXModule();
    if (wuxModule != null)
    {
        ptr = GetTLSPointer(thread, wuxModule, "DXamlInstanceStorage::g_dwTlsIndex");
        if (ptr != null && !ptr.isNull)
        {
            return wuxModule;
        }
        // It is possible XAML isn't initialized on the thread but there may still be stowed
        // exceptions, so check for that as well.
        ptr = GetTLSPointer(thread, wuxModule, "g_dwErrorContextTlsIndex");
        if (ptr != null && !ptr.isNull)
        {
            return wuxModule;
        }
    }

    if (silentOnNonXAML != true)
    {
        host.diagnostics.debugLog("No xaml on thread: " + thread + "\n");
    }
    return undefined;
}

function XAML_DLL_Module_from_targetType(targetType)
{
    try
    {
        var targetModule = targetType.containingModule;
        var lowercaseModule = targetModule.Name.toLowerCase();
        if (lowercaseModule.endsWith("windows.ui.xaml.dll") || lowercaseModule.endsWith("microsoft.ui.xaml.dll"))
        {
            return targetModule;
        }
    }
    catch (ex) {}
    throw new Error("targetType must be a core XAML type: " + targetType.toString());
}

function __debugDump(o)
{
    host.diagnostics.debugLog("Dumping " + o + " (targetType=" + o.targetType + ")\n");
    try
    {
        for (var fldName of Object.getOwnPropertyNames(o))
        {
            try
            {
                host.diagnostics.debugLog("  " + fldName + ": " + o[fldName] + "\n");
            }
            catch (e)
            {
                host.diagnostics.debugLog("  " + fldName + " error: " + e + "\n");
            }
        }
        host.diagnostics.debugLog("  ....\n");
    }
    catch (e) {}

    try
    {
        for (var v in o)
        {
            try
            {
                host.diagnostics.debugLog("  " + v + ": " + o[v] + "\n");
            }
            catch (e)
            {
                host.diagnostics.debugLog("  " + v + " error: " + e + "\n");
            }
        }
    }
    catch (e) {}

    host.diagnostics.debugLog("  done.\n");
}

function __IsXAMLLoaded()
{
    return (host.currentProcess.Modules.Where(function(m) { return m.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll") || 
                                                                   m.Name.toLowerCase().endsWith("windows.ui.xaml.dll") }).Count() != 0);
}

function __CheckXAMLSymbolsLoaded()
{
    if (!__IsXAMLLoaded())
    {
        throw new Error("XAML is not loaded.");
        //host.diagnostics.debugLog("XAML is not loaded.\n");
        return false;
    }

    var winui3MUXModule = GetWinUI3MUXModule();
    if (winui3MUXModule != null)
    {
        // Note: I think will break when host.apiVersionSupport(1,1) is used to get new behavior where
        //       getModuleSymbol returns null for unknown symbols.
        try
        {
            var v = host.getModuleSymbol(winui3MUXModule, "DXamlInstanceStorage::g_dwTlsIndex", "int");
        }
        catch (exp)
        {
            if (exp.toString().indexOf("Invalid argument to method 'getModuleSymbol'") > 0)
            {
                throw new Error("Symbols for XAML (microsoft.ui.xaml.dll) not loaded/unavailable.");
                //host.diagnostics.debugLog("Symbols for XAML (microsoft.ui.xaml.dll) not loaded/unavailable.\n");
                return false;
            }
            // else the error might be lack of heap or something else.
        }
    }

    return true;
}

function InferTypeNameForAddress(addr, requireCodePointer)
{
    try
    {
        // Run this command twice, to avoid any errors about loading symbols.
        var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + addr.toString(16) + " L1");

        // First check for good code pointers via .printf "%ly".
        var potentialObject = host.createPointerObject(addr, "combase.dll", "void**");
        var potentialVtable = potentialObject.dereference();
        //host.diagnostics.debugLog("vtable addr: " + potentialVtable.address.toString(16));
        // Note: The "0x" is added to the address to ensure this command works even if the user has
        //       the debugger in a mode where it doesn't auto-assume a hex value (via "n 10").
        output = host.namespace.Debugger.Utility.Control.ExecuteCommand(".printf \"%ly\", 0x" + potentialVtable.address.toString(16));
        for (var line of output)
        {
            index = line.indexOf("::`vftable'");
            if (index > 0)
            {
                line = line.substring(0, index);
                return line;
            }
            break;
        }

        // That didn't find anything, so check for a possible CLR type.
        output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + addr.toString(16) + " L1");
        for (var line of output)
        {
            //host.diagnostics.debugLog("line: " + line + " addr: " + addr.toString(16) + "\n");
            line = line.substring(line.indexOf(' '));
            line = line.trim();
            var index = line.indexOf(' ');
            if (index > 0)
            {
                line = line.substring(index);
                line = line.trim();
                if (line != null)
                {
                    if (line.indexOf("Vtable") > 0)
                    {
                        // Looks like a Clr Vtable
                        return line;
                    }

                    // If the caller doesn't require a known good code pointer,
                    // go ahead and return the line.
                    if (!requireCodePointer)
                    {
                        return line;
                    }

                    // We're done and didn't find anything which we're sure to be valid.
                    break;
                }
            }
        }
    }
    catch (ex)
    {
    }
    return null;
}

function CreateTypedPointerInferredForAddress(addr)
{
    var objType = InferTypeNameForAddress(addr);
    if (objType == null)
    {
        // Can't infer type, so return null
        return null;
    }
    //host.diagnostics.debugLog("type: " + objType + "\n");

    var bangIndex = objType.indexOf('!');
    if (bangIndex < 0)
    {
        host.diagnostics.debugLog("Error:  unable to find separator in type: " + objType + "\n");
        return null;
    }

    var moduleName = objType.substring(0, bangIndex);
    var typeName = objType.substring(bangIndex+1) + "*";
    //host.diagnostics.debugLog("Creating object for " + moduleName + "!" + typeName + " at " + addr.toString(16) + "\n");

    var ptr = host.createPointerObject(addr, moduleName, typeName);
    return ptr;
}

function __createRuntimeTypedObjectPointer(obj)
{
    try
    {
        var addr = obj.targetLocation.address;
        var objType = obj.targetType;
        var ptrObj = host.createPointerObject(addr, objType.createPointerTo());
        return ptrObj;
    }
    catch(e)
    {
        host.diagnostics.debugLog("__createRuntimeTypedObjectPointer failure: " + e + "\n");
        return obj;
    }
}

function __GetNameForEnumValue(value, enumType, module, unknownResult)
{
    var myType = host.getModuleType(module, enumType);
    var myFields = myType.fields;
    for (var fldName of Object.getOwnPropertyNames(myFields))
    {
        var fld = myFields[fldName];
        if (fld.value == value)
        {
            return fld.name;
        }
    }

    return unknownResult;
}

// Get the DirectUI::DependencyObject* for the given core CDependencyObject*.
function __GetPeerDOForCDO(cdo)
{
    if (cdo.isNull)
    {
        return null;
    }

    try
    {
        var peer = cdo.m_pDXAMLPeer;

        var peerAddress = peer.address;
        if (peerAddress & 1)
        {
            peerAddress--;
        }

        peer = host.createPointerObject(peerAddress, XAML_DLL_Module_from_targetType(cdo.targetType), "DirectUI::DependencyObject*");
        return peer;
    }
    catch (exp)
    {
        host.diagnostics.debugLog("__GetPeerDOForCDO error: " + exp + "\n");
        return null;
    }
}

// Get CCoreServices from the given CDO*
function __GetCoreForCDO(cdo)
{
    if (cdo.isNull)
    {
        return null;
    }

    try
    {
        return cdo.m_sharedState.m_ptr.m_value.m_coreServices;
    }
    catch (exp)
    {
        host.diagnostics.debugLog("__GetCoreForCDO error: " + exp + "\n");
        return null;
    }
}

class __List
{
    constructor(array, name)
    {
        this.__array = array;
        this.__name = name;
    }

    toString()
    {
        return this.__array.length + " " + this.__name;
    }

    get length()
    {
        return this.__array.length;
    }

    GetArray()
    {
        return this.__array;
    }

    *[Symbol.iterator]()
    {
        try
        {
            for (let item of this.__array)
            {
                yield item;
            }
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Error in iterator: " + exp);
        }
    }
}

class __codePointerInfo
{
    constructor(addr, requireName)
    {
        this.__addr = addr;
        this.__requireName = requireName;
        this.__initialized = false; // wait and initialize on demand for better perf
    }

    toString()
    {
        this.__Initialize();
        var source = this.__source;
        if (source != "")
        {
            source = " [" + source + "]";
        }

        var addrStr = this.__addr.address.toString(16);

        return this.__functionName + "  (" + addrStr + ")" + source;
    }

    Print(indentStr)
    {
        this.__Initialize();
        var addrStr = this.__addr.address.toString(16);
        var source = this.__source;
        if (source != "")
        {
            source = source.replace(/\\/g, '\\\\');

            var sourceNoLineNum = source.replace(/(.*) @ [0-9]*$/, '$1');
            var lineNum = source.replace(/.*( @ [0-9]*)$/, '$1');

            source = " [<link cmd=\\\".open -a " + addrStr + "\\\">" + sourceNoLineNum + "</link>" + lineNum + "]";
        }

        var cmd = ".printf /D \"" + indentStr + this.__functionName + "  (" + addrStr + ")" + source + "\\n\"";
        host.namespace.Debugger.Utility.Control.ExecuteCommand(cmd, false);
    }

    get Address()
    {
        return this.__addr.address.toString(16);
    }

    get Name()
    {
        this.__Initialize();
        return this.__functionName;
    }

    get Source()
    {
        this.__Initialize();
        return this.__source;
    }

    __Initialize()
    {
        if (this.__initialized)
        {
            return;
        }

        this.__source = "";
        this.__functionName = "";

        // Note: The "0x" is added to the address to ensure this command works even if the user has
        //       the debugger in a mode where it doesn't auto-assume a hex value (via "n 10").
        var output = host.namespace.Debugger.Utility.Control.ExecuteCommand(".printf \"%ly\", 0x" + this.__addr.address.toString(16));
        for (var line of output)
        {
            line = line.trim();

            this.__functionName = line;
            if (this.__functionName.includes('('))
            {
                this.__functionName = this.__functionName.substring(0, line.indexOf('(')).trim();
            }
            else if (this.__requireName)
            {
                this.__functionName = "";
            }

            if (line.indexOf("[") > 0)
            {
                this.__source = line.substring(line.indexOf('[')+1, line.indexOf(']'));
            }

            break;
        }
    }
}

class __Registers
{
    constructor(contextRecord)
    {
        this.__contextRecord = contextRecord;

        if (this.__contextRecord["Rax"] != undefined)
        {
            // amd64
            this.__registers = [ "Rax", "Rbx", "Rcx", "Rdx", "Rsi", "Rdi", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" ];
        }
        else if (this.__contextRecord["Eax"] != undefined)
        {
            // x86
            this.__registers = [ "Eax", "Ebx", "Ecx", "Edx", "Esi", "Edi" ];
        }
        else if (this.__contextRecord["R0"] != undefined)
        {
            // arm
            this.__registers = [ "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12" ];
        }
        else
        {
            // arm64
            this.__registers = [];
            for (var i = 0; i <= 28; i++)
            {
                this.__registers.push("X" + i);
            }
        }
    }

    toString()
    {
        return this.__registers.length + " Registers";
    }

    *[Symbol.iterator]()
    {
        var stack = [];

        var is64bit = false;
        if (host.evaluateExpression("sizeof(void*)*8") == 64)
        {
            is64bit = true;
        }

        for (var fldName of this.__registers)
        {
            var value = this.__contextRecord[fldName];
            fldName = fldName.toLowerCase();

            var regValue;

            var fldNameStr = fldName + (fldName.length < 3 ? " " : "");
            var valueStr = value.toString(16);
            if (valueStr.startsWith("0x"))
            {
                valueStr = valueStr.substring(2);
            }
            valueStr = "0x" + ("0000000000000000" + valueStr).slice(-(is64bit ? 16 : 8));
            var objType = InferTypeNameForAddress(value, true/*requireCodePointer*/);
            if (objType != null)
            {
                regValue = fldNameStr + " = " + valueStr + "  Obj: " + objType;
            }
            else
            {
                var pointer = host.createPointerObject(value, "combase.dll", "void*");
                var codePtr = new __codePointerInfo(pointer, true);
                if (codePtr.Name)
                {
                    regValue = fldNameStr + " = " + valueStr + "  code: " + codePtr;
                }
                else
                {
                    regValue = fldNameStr + " = " + valueStr;
                }
            }

            yield regValue;
        }
    }
}

class __xstringPtrVisualizer
{
    toString()
    {
        var handle = this.m_encodedStorage.Handle;
        if (!handle.isNull && (handle.address & 1))
        {
            handle = host.createPointerObject(handle.address.add(-1), "combase.dll", "STRING_OPAQUE*");
            var str = "????";
            try
            {
                str = host.memory.readWideString(handle.stringRef.address);
            }
            catch(e)
            {
                host.diagnostics.debugLog("   xstring error: " + e + "\n");
            }
            return str;
        }
        if (this.m_encodedStorage.Storage.Count == 0)
        {
            return "";
        }
        return this.m_encodedStorage.toString();
        //host.diagnostics.debugLog("   returning " + this.m_encodedStorage.Storage.Buffer + "\n");
        //__debugDump(this.m_encodedStorage.Storage.Buffer);
        //return this.m_encodedStorage.Storage.Buffer; // This function used to do this. Is there a scenario where this is better?
    }
}

class __xencodedStringPtrVisualizer
{
    toString()
    {
        var handle = this.Handle;
        if (!handle.isNull && (handle.address & 1))
        {
            handle = host.createPointerObject(handle.address.add(-1), "combase.dll", "STRING_OPAQUE*");
            var str = "????";
            try
            {
                str = host.memory.readWideString(handle.stringRef.address);
            }
            catch(e)
            {
                host.diagnostics.debugLog("   xstring error: " + e + "\n");
            }
            return str;
        }
        if (this.Storage.Count == 0)
        {
            return "";
        }
        try
        {
            return host.memory.readWideString(this.Storage.Buffer.address);
        }
        catch(e)
        {
            host.diagnostics.debugLog("   xstring error: " + e + "\n");
        }
    }

    get String()
    {
        return this.toString();
    }
}

function __GetErrorStringFromHRESULT(hr, includeHRESULT)
{
    if (hr == undefined)
        return undefined;

    var errorCodeString = "";
    hr = host.evaluateExpression("(unsigned int)" + hr); // make sure the value is unsigned
    if (includeHRESULT)
    {
        errorCodeString = hr.toString(16) + " - ";
    }
    switch (hr)
    {
    // First, handle some known error codes (especially ones !error doesn't handle)
    case 0xc000027b: errorCodeString += "Stowed exception"; break;
    case 0xc0000005: errorCodeString += "Access violation"; break;
    case 0x802b000a: errorCodeString += "E_XAMLPARSEFAILED"; break;
    case 0x802b0014: errorCodeString += "E_LAYOUTCYCLE"; break;
    case 0x88000fa8: errorCodeString += "AG_E_LAYOUT_CYCLE"; break;
    case 0x800f1000: errorCodeString += "E_NER_INVALID_OPERATION"; break;
    case 0x800f1001: errorCodeString += "E_NER_ARGUMENT_EXCEPTION"; break;
    case 0x80131513: errorCodeString += "COR_E_MISSING_METHOD"; break;

    default:
        var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("!error 0n" + hr);
        for (var line of output)
        {
            var msg = line.substring(line.indexOf('-') + 1).trim();
            errorCodeString += msg;
            break;
        }
        break;
    }

    if (errorCodeString == "")
    {
        errorCodeString = hr.toString(16);
    }

    return errorCodeString;
}

class __ErrorContextVisualizer
{
    toString()
    {
        return this.__GetHRESULTString() + " in " + this.ThrowingAddress.Name;
    }

    get HRESULT()
    {
        var hr = this.frameHRs[0];
        return __GetErrorStringFromHRESULT(hr, true /*includeHRESULT*/);
    }

    get ThrowingAddress()
    {
        return new __codePointerInfo(this.frameAddresses[0]);
    }

    get Stack()
    {
        var stack = [];
        for (var i = 0; i < this.frameCount; i++)
        {
            stack.push(new __codePointerInfo(this.frameAddresses[i]));
        }
        return new __List(stack, "Frames");
    }

    get Registers()
    {
        return new __Registers(this.contextRecord);
    }

    __GetHRESULTString()
    {
        var hr = this.frameHRs[0];
        var unsignedHR = host.evaluateExpression("(unsigned int)" + hr);
        return "0x" + unsignedHR.toString(16);
    }

    get NestedException()
    {
        if (this.threadErrorInfo != undefined && !this.threadErrorInfo.ptr_.isNull)
        {
            var errorInfo = this.threadErrorInfo.ptr_.dereference().runtimeTypedObject;
            if (errorInfo.targetType == "CRestrictedError")
            {
                return new __CRestrictedErrorWrapper(errorInfo);
            }
            return errorInfo;
        }
    }
}

class __WarningContextVisualizer
{
    toString()
    {
        return this.Type + " in " + this.ThrowingAddress.Name;
    }

    get Type()
    {
        return __GetNameForEnumValue(this.type, "WarningContextLog::WarningContextType", XAML_DLL_Module_from_targetType(this.targetType), "Unknown");
    }

    get ExtraInfo()
    {
        var strings = [];
        var count = this.extraInfo.size();
        for (var i = 0; i < count; i++)
        {
            strings.push(this.extraInfo[i]);
        }
        return strings;
    }

    get ThrowingAddress()
    {
        return new __codePointerInfo(this.frameAddresses[0]);
    }

    get Stack()
    {
        var stack = [];
        for (var i = 0; i < this.frameCount; i++)
        {
            stack.push(new __codePointerInfo(this.frameAddresses[i]));
        }
        return new __List(stack, "Frames");
    }
}

function __GetBlameFunctionFromStackFrames(stack)
{
    // Find a BlameFunction
    for (var frame of stack)
    {
        if (frame.Name.indexOf("!strlen") >= 0 ||
            frame.Name.indexOf("__abi_WinRTraiseCOMException") >= 0 ||
            frame.Name.indexOf("__abi_WinRTraiseException") >= 0 ||
            frame.Name.indexOf("__abi_WinRTraiseFailureException") >= 0 ||
            frame.Name.indexOf("__abi_WinRTraiseInvalidArgumentException") >= 0 ||
            frame.Name.indexOf("__CxxCallCatchBlock") >= 0 ||
            frame.Name.indexOf("_CallSettingFrame") >= 0 ||
            frame.Name.indexOf("_CxxThrowException") >= 0 ||
            frame.Name.indexOf("ClientToScreen") >= 0 ||
            frame.Name.indexOf("KiRaiseUserExceptionDispatcher") >= 0 ||
            frame.Name.indexOf("OriginateError") >= 0 ||
            frame.Name.indexOf("Platform::PrepareForThrow") >= 0 ||
            frame.Name.indexOf("RaiseFailFastException") >= 0 ||
            frame.Name.indexOf("RcConsolidateFrames") >= 0 ||
            frame.Name.indexOf("RealMsgWaitForMultipleObjectsEx") >= 0 ||
            frame.Name.indexOf("ReCreateException") >= 0 ||
            frame.Name.indexOf("ReCreateFromException") >= 0 ||
            frame.Name.indexOf("Return_CaughtException") >= 0 ||
            frame.Name.indexOf("RoFailFastWithErrorContextInternal2") >= 0 ||
            frame.Name.indexOf("RoGetMatchingRestrictedErrorInfo") >= 0 ||
            frame.Name.indexOf("RoOriginateLanguageException") >= 0 ||
            frame.Name.indexOf("ucrtbase!") >= 0 ||
            frame.Name.indexOf("wil::details::ReportFailure") >= 0 ||
            frame.Name.indexOf("WRL::EventSource") >= 0 ||
            frame.Name.indexOf("WRL::InvokeTraits") >= 0)
        {
            continue; // Don't blame this function.
        }

        return frame.Name;
    }

    return undefined;
}

class __ClrObject
{
    constructor(address)
    {
        this.__address = address;
        this.__Initialize();
    }

    toString()
    {
        var str = this.__address.toString(16);
        try { host.memory.readMemoryValues(this.__address, 1); } catch(e) { str += " (Target memory not available)"; }
        if (this.__dumpCommand != undefined)
        {
            str += " [" + this.__dumpCommand + "]";
            if (this.__targetExceptionHRESULT != undefined)
            {
                str += " (" + this.__targetExceptionHRESULT.toString(16) + ")";
            }
            if (this.__targetExceptionMessage != undefined)
            {
                str += ' "' + this.__targetExceptionMessage + '"';
            }
        }
        else if (this.__targetType != undefined)
        {
            str += " [" + this.__targetType;
            if (this.__targetExceptionHRESULT != undefined)
            {
                str += " (" + this.__targetExceptionHRESULT.toString(16) + ")";
            }
            str += "]";
            if (this.__targetExceptionMessage != undefined)
            {
                str += ' "' + this.__targetExceptionMessage + '"';
            }
        }

        return str;
    }

    get ManagedCCW()
    {
        return this.__managedCCW;
    }

    get InnerRCW()
    {
        return this.__innerRCW;
    }

    get Target()
    {
        return this.__m_target;
    }

    get TargetType()
    {
        return this.__targetType;
    }

    get TargetExceptionHRESULT()
    {
        return this.__targetExceptionHRESULT;
    }

    get TargetExceptionMessage()
    {
        return this.__targetExceptionMessage;
    }

    get TargetExceptionStack()
    {
        return this.__targetExceptionStack;
    }

    get TargetExceptionInnerException()
    {
        return this.__targetExceptionInnerException;
    }

    __Initialize()
    {
        var type = InferTypeNameForAddress(this.__address);
        if (type == null)
        {
            return; // unable to infer type. Possibly missing heap
        }
        if (type.toLowerCase().startsWith("coreclr"))
        {
            this.__InitializeCoreCLR(type);
        }
    }

    __InitializeCoreCLR(type)
    {
        //host.diagnostics.debugLog("clr: " + this.__address.toString(16) + " type: " + type + "\n");
        var clrtype = type.substring(type.indexOf('!') + 1);
        if (clrtype.startsWith("g_IErrorInfo"))
        {
            // First create the right type, then use its type to determine the correct offset to
            // the real object, and recreate at the corrected address.
            var obj1 = host.createPointerObject(this.__address, "CoreCLR.dll", "SimpleComCallWrapper*");
            var offset = obj1.targetType.baseType.fields.m_rgpVtable.offset + host.evaluateExpression("sizeof(void*)*4");
            obj1 = host.createPointerObject(this.__address.add(-offset), "CoreCLR.dll", "SimpleComCallWrapper*");

            var g_pExceptionClass = host.getModuleSymbol("CoreCLR.dll", "g_pExceptionClass", "MethodTable*");

            // Confirm it is an exception
            var isException = false;
            try
            {
                var mt = obj1.m_pMT;
                while (mt != undefined && !mt.isNull)
                {
                    if (mt.address == g_pExceptionClass.address)
                    {
                        isException = true;
                        break;
                    }

                    mt = mt.m_pParentMethodTable;
                    if (!mt.isNull)
                    {
                        mt = host.createPointerObject(mt, "CoreCLR.dll", "MethodTable*");
                    }
                }

                // TODO: Can we do better than this???
                this.__m_target = obj1;
            }
            catch (e)
            {
                // Sometimes the MethodTable* isn't right or is inaccessible, possibly
                // due to mscorlib_ni!* not being something we can work with.
            }
            this.__targetType = (isException ? "Exception" : type);
        }
        else if (clrtype.startsWith("ManagedObjectWrapper"))
        {
            try
            {
                var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("!dumpccw 0x" + this.__address.toString(16));
                var managedObject = null;
                for (var line of output)
                {
                    if (line.startsWith("Managed object:"))
                    {
                        managedObject = line.substring("Managed object:".length).trim();
                        break;
                    }
                }
                if (managedObject)
                {
                    // Determine the name of the sos extension (in case it isn't just "!sos")
                    var sosExtName = host.namespace.Debugger.Utility.Control.ExecuteCommand(".extmatch dumpccw")[0];
                    sosExtName = sosExtName.substring(0, sosExtName.indexOf('.'));
                    var dumpCommand = sosExtName + ".pe 0x" + managedObject;
                    output = host.namespace.Debugger.Utility.Control.ExecuteCommand(dumpCommand);
                    var stack = [];
                    var inStackSection = false;
                    var stackFunctionNameStartIndex = -1;
                    for (var line of output)
                    {
                        if (line.startsWith("StackTrace") && !line.startsWith("StackTraceString"))
                        {
                            inStackSection = true;
                            continue;
                        }
                        else if (inStackSection)
                        {
                            if (line == "") // stack stops on a blank line.
                            {
                                inStackSection = false;
                                continue;
                            }
                            if (stackFunctionNameStartIndex == -1)
                            {
                                // first line of the stack should be "    SP               IP               Function"
                                stackFunctionNameStartIndex = line.indexOf("Function");
                                if (stackFunctionNameStartIndex < 0)
                                {
                                    host.diagnostics.debugLog("Error parsing CLR stack.\n");
                                    inStackSection = false;
                                }
                                continue;
                            }
                            stack.push(line.substring(stackFunctionNameStartIndex).trim());
                        }
                        else if (line.startsWith("HResult:"))
                        {
                            this.__targetExceptionHRESULT = host.evaluateExpression("(unsigned int)0x" + line.substring("HResult:".length).trim());
                        }
                        else if (line.startsWith("Message:") && !line.endsWith("<none>"))
                        {
                            this.__targetExceptionMessage = line.substring("Message:".length).trim();
                        }
                        else if (line.startsWith("InnerException:") && !line.endsWith("<none>"))
                        {
                            this.__targetExceptionInnerException = line.substring("InnerException:".length).trim();
                        }
                    }
                    this.__targetExceptionStack = new __List(stack, "Frames");
                    this.__dumpCommand = dumpCommand;
                    this.__innerRCW = host.evaluateExpression("(void*)0x" + managedObject);
                }
            }
            catch (e)
            {
            }
            // Regardless of whether we were able to get more info, still include the base address and type.
            this.__m_target = host.createPointerObject(this.__address, "combase.dll", "void*");
            this.__targetType = type;
        }
        else
        {
            this.__m_target = host.createPointerObject(this.__address, "combase.dll", "void*");
            this.__targetType = type;
        }
    }

    static IsClrObject(addr)
    {
        // Verify the specified address looks like a CLR object.
        // Run this command twice, to avoid any errors about loading symbols.
        var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + addr.toString(16) + " L1");
            output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + addr.toString(16) + " L1");
        for (var line of output)
        {
            line = line.substring(line.indexOf(' '));
            line = line.trim();
            var index = line.indexOf(' ');
            if (index > 0)
            {
                line = line.substring(index);
                line = line.trim();
                if (line != null)
                {
                    index = line.indexOf(".s_theCcwVtable");
                    if (index > 0)
                    {
                        return true; // object found!
                    }

                    index = line.indexOf(".s_staticVtable");
                    if (index > 0)
                    {
                        return true; // object found!
                    }

                    index = line.toLowerCase().indexOf("coreclr!g_i");
                    if (index == 0)
                    {
                        return true; // object found!
                    }

                    index = line.toLowerCase().indexOf("coreclr!managedobjectwrapper_iunknownimpl");
                    if (index == 0)
                    {
                        return true; // object found!
                    }
                }
            }

            return false;
        }
        return false;
    }

    RunDumpCommand()
    {
        host.diagnostics.debugLog("> " + this.__dumpCommand + "\n");
        var output = host.namespace.Debugger.Utility.Control.ExecuteCommand(this.__dumpCommand);
        for (var line of output)
        {
            host.diagnostics.debugLog(line + "\n");
        }
    }
}

class __CPPWinRTExceptionErrorWrapper
{
    constructor(type, exceptionObject)
    {
        this.__type = type;
        this.__exceptionObject = exceptionObject;
    }

    toString()
    {
        return this.__type;
    }

    get Exception()
    {
        return this.__exceptionObject;
    }
}

class __CPPWinRTHResultErrorWrapper
{
    constructor(hresultErrorStruct)
    {
        this.__hresultError = hresultErrorStruct;
        this.Initialize();
    }

    toString()
    {
        return this.HRESULT;
    }

    get RawData()
    {
        return this.__hresultError;
    }

    get HRESULT()
    {
        if (this.__hresultError.m_code.value == undefined)
        {
            return __GetErrorStringFromHRESULT(this.__hresultError.m_code, true /*includeHRESULT*/);
        }
        return __GetErrorStringFromHRESULT(this.__hresultError.m_code.value, true /*includeHRESULT*/);
    }

    get RestrictedError()
    {
        return this.__restrictedError;
    }

    Initialize()
    {
        if (!this.__hresultError.m_info.m_ptr.isNull)
        {
            var restrictedError = __createRuntimeTypedObjectPointer(this.__hresultError.m_info.m_ptr.dereference().runtimeTypedObject);
            this.__restrictedError = new __CRestrictedErrorWrapper(restrictedError);
        }
    }
}

class __CombaseStowedExceptionWrapper
{
    constructor(stowedExceptionStruct)
    {
        this.__stowedExceptionStruct = stowedExceptionStruct;
        this.__Initialize();
    }

    toString()
    {
        var str = this.__stowedExceptionStruct.dereference().toString();
        if (this.__blameFunction != undefined)
        {
            str += " in " + this.__blameFunction;
        }
        return str;
    }

    get RawData()
    {
        return this.__stowedExceptionStruct;
    }

    get ResultCode()
    {
        if (this.__stowedExceptionStruct.ResultCode == undefined)
        {
            return "(--unavailable--)";
        }
        return __GetErrorStringFromHRESULT(this.__stowedExceptionStruct.ResultCode, true);
    }

    get ErrorMessage()
    {
        var STOWED_EXCEPTION_FORM_TEXT   = 0x02;
        if (this.__stowedExceptionStruct.ExceptionForm == STOWED_EXCEPTION_FORM_TEXT)
        {
            return host.memory.readWideString(this.__stowedExceptionStruct.ErrorText);
        }
        return this.__errorMessage;
    }
    set ErrorMessage(errorMessage)
    {
        this.__errorMessage = errorMessage;
    }

    get BlameFunction()
    {
        return this.__blameFunction;
    }

    get Stack()
    {
        if (this.__stack == undefined)
        {
            return new __List([], "Frames");
        }
        return this.__stack;
    }

    get NestedExceptionType()
    {
        return this.__nestedExceptionType;
    }

    get XamlErrorContext()
    {
        return this.__xamlErrorContext;
    }

    get ManagedException()
    {
        return this.__managedException;
    }

    __Initialize()
    {
        var STOWED_EXCEPTION_FORM_BINARY = 0x01;
        var STOWED_EXCEPTION_FORM_TEXT   = 0x02;

        if (this.__stowedExceptionStruct.ExceptionForm == STOWED_EXCEPTION_FORM_BINARY)
        {
            var stackEntry = host.createPointerObject(this.__stowedExceptionStruct.StackTrace.address, "combase.dll", "void**");

            var stack = [];
            for (var i = 0; i < this.__stowedExceptionStruct.StackTraceWords; i++)
            {
                stack.push(new __codePointerInfo(stackEntry[i]));
            }
            this.__stack = new __List(stack, "Frames");

            // Find a BlameFunction
            this.__blameFunction = __GetBlameFunctionFromStackFrames(this.__stack);
        }
        else
        {
            this.__stack = undefined; // no stack for TEXT exceptions
        }

        var NestedExceptionType_LE01 = 0x314F454C; // 'LE01'
        var NestedExceptionType_XAML = 0x4C4D4158; // 'XAML'
        var NestedExceptionType_STOW = 0x574F5453; // 'STOW'
        var NestedExceptionType_W32E = 0x45323357; // 'W32E'
        if (this.__stowedExceptionStruct.NestedExceptionType == NestedExceptionType_XAML)
        {
            this.__nestedExceptionType = "XAML";
            try
            {
                // get the thread for the stowed exception crash and the dll for that thread
                var thread = host.currentProcess.Threads.Where(function(t) { return t.Id == host.namespace.Debugger.State.PseudoRegisters.General.tid }).First();
                var module = XAML_DLL_Module_from_thread(thread);

                // This can fail if XAML symbols couldn't be loaded.
                this.__xamlErrorContext = host.createPointerObject(this.__stowedExceptionStruct.NestedException.address, module, "ErrorContext*");
            }
            catch (eErrorContext)
            {
                //this.__xamlErrorContext = host.createPointerObject(this.__stowedExceptionStruct.NestedException.address, "combase.dll", "void*");
            }
        }
        else if (this.__stowedExceptionStruct.NestedExceptionType == NestedExceptionType_LE01)
        {
            this.__nestedExceptionType = "Language exception";
            this.__managedException = new __ClrObject(this.__stowedExceptionStruct.NestedException.address);
        }
        else if (this.__stowedExceptionStruct.NestedExceptionType == NestedExceptionType_STOW)
        {
            // FUTURE: Expose the underlying exception info
            this.__nestedExceptionType = "Stowed exception!";
        }
        else if (this.__stowedExceptionStruct.NestedExceptionType == NestedExceptionType_W32E)
        {
            // FUTURE: Expose the underlying exception info
            this.__nestedExceptionType = "Win32 exception!";
        }
    }
}

class __NonStowedExceptionErrorWrapper
{
    constructor(errorCode, errorCodeString, stack)
    {
        this.__errorCode = errorCode;
        this.__errorCodeString = errorCodeString;
        this.__stack = stack;
    }

    toString()
    {
        var str = this.__errorCode.toString(16) + " (" + this.__errorCodeString + ")";
        if (this.__stack != undefined)
        {
            str += "  " + this.__stack;
        }
        return str;
    }

    get Stack()
    {
        return this.__stack;
    }
}

class __CRestrictedErrorWrapper
{
    constructor(restrictedError)
    {
        this.__restrictedError = restrictedError;
        this.__Initialize();
    }

    toString()
    {
        var str = this.HRESULT;
        if (this.__blameFunction != undefined)
        {
            str += " in " + this.__blameFunction;
        }
        return str;
    }

    get RawData()
    {
        return this.__restrictedError;
    }

    get HRESULT()
    {
        return __GetErrorStringFromHRESULT(this.__restrictedError._hrError);
    }

    get Description()
    {
        return this.__restrictedError._pszDescription;
    }

    get RestrictedDescription()
    {
        if (this.__restrictedError._pszRestrictedDescription != undefined && !this.__restrictedError._pszRestrictedDescription.isNull)
        {
            return this.__restrictedError._pszRestrictedDescription;
        }
        return undefined;
    }

    get BlameFunction()
    {
        return this.__blameFunction;
    }

    get Stack()
    {
        return this.__stack;
    }

    get LanguageException()
    {
        return this.__languageException;
    }

    __Initialize()
    {
        var stackEntry = this.__restrictedError._ppvStackBackTrace;

        var stack = [];
        for (var i = 0; i < this.__restrictedError._cStackBackTrace; i++)
        {
            stack.push(new __codePointerInfo(stackEntry[i]));
        }
        this.__stack = new __List(stack, "Frames");

        this.__blameFunction = __GetBlameFunctionFromStackFrames(this.__stack);

        if (this.__restrictedError._spLanguageException != undefined && !this.__restrictedError._spLanguageException.ptr_.isNull)
        {
            this.__languageException = this.__restrictedError._spLanguageException.ptr_;
            if (__ClrObject.IsClrObject(this.__languageException.address))
            {
                this.__languageException = new __ClrObject(this.__languageException.address);
            }
        }
    }
}

// Wrapper around a CValue
class __CValue
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        return this.__GetValueString();
    }

    get RawValue()
    {
        return this.__value;
    }

    get Type()
    {
        if (this.__value.isNull)
        {
            return null;
        }

        return __GetNameForEnumValue(this.__value.m_flags.m_state.m_type, "ValueType", XAML_DLL_Module_from_targetType(this.__value.targetType), "Unknown");
    }

    __ValueFieldToAddress(field)
    {
        var is64bit = (host.evaluateExpression("sizeof(void*)*8") == 64);
        var addr;
        if (is64bit)
        {
            addr = new host.Int64(field[0], field[1]);
        }
        else
        {
            addr = field.address;
        }
        return addr;
    }

    get Value()
    {
        var type = this.Type;
        if (type == "valueNull")
        {
            return null;
        }

        if (type == "valueObject" || type == "valuePointer")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_object);
            var ptr = CreateTypedPointerInferredForAddress(addr);
            if (ptr == null)
            {
                // null is returned when type can't be inferred. Just use void*.
                ptr = host.createPointerObject(addr, "combase.dll", "void*");
            }
            return ptr;
        }
        if (type == "valueIUnknown")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_iunknown);
            var ptr = host.createPointerObject(addr, "combase.dll", "IUnknown*");
            if (!ptr.isNull)
            {
                ptr = __createRuntimeTypedObjectPointer(ptr.dereference().runtimeTypedObject);
            }
            return ptr;
        }
        if (type == "valueIInspectable")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_iinspectable);
            var ptr = host.createPointerObject(addr, "combase.dll", "IInspectable*");
            if (!ptr.isNull)
            {
                ptr = __createRuntimeTypedObjectPointer(ptr.dereference().runtimeTypedObject);
            }
            return ptr;
        }
        if (type == "valueBool")
        {
            return this.__value.m_value.m_bool;
        }
        if (type == "valueEnum" || type == "valueSigned")
        {
            return this.__value.m_value.m_signed;
        }
        if (type == "valueEnum8")
        {
            return this.__value.m_value.m_unsigned & 0xFF;
        }
        if (type == "valueUnsigned")
        {
            return this.__value.m_value.m_unsigned;
        }
        if (type == "valueDouble")
        {
            return host.evaluateExpression("*((double*)" + this.__value.m_value.m_double.targetLocation + ")");
        }
        if (type == "valueCornerRadius")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_cornerRadius);
            var cornerRadius = host.createPointerObject(addr, XAML_DLL_Module_from_targetType(this.__value.targetType), "XCORNERRADIUS*");
            if (!cornerRadius.isNull)
            {
                return "" + cornerRadius.topLeft + "," + cornerRadius.topRight + "," + cornerRadius.bottomRight + "," + cornerRadius.bottomLeft;
            }
            return "null cornerRadius";
        }
        if (type == "valueRect")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_rect);
            var rect = host.createPointerObject(addr, XAML_DLL_Module_from_targetType(this.__value.targetType), "XRECTF*");
            if (!rect.isNull)
            {
                return "" + rect.X + "," + rect.Y + "," + rect.Width + "," + rect.Height;
            }
            return "null rect";
        }
        if (type == "valueThickness")
        {
            var addr = this.__ValueFieldToAddress(this.__value.m_value.m_thickness);
            var thickness = host.createPointerObject(addr, XAML_DLL_Module_from_targetType(this.__value.targetType), "XTHICKNESS*");
            if (!thickness.isNull)
            {
                return "" + thickness.left + "," + thickness.top + "," + thickness.right + "," + thickness.bottom;
            }
            return "null thickness";
        }
        if (type == "valueString")
        {
            var addr = this.__value.m_value.targetLocation.address;
            var xstring = host.createPointerObject(addr, XAML_DLL_Module_from_targetType(this.__value.targetType), "xstring_ptr_view*");
            return xstring.m_encodedStorage.toString();
        }
        return "TODO:" + type;
    }

    __GetValueString()
    {
        var value = this.Value;
        if (value == null)
        {
            return "null";
        }

        var type = this.Type;
        if (type == "valueObject" || type == "valuePointer" || type == "valueIInspectable" || type == "valueIUnknown")
        {
            if (value.isNull)
            {
                value = "null";
            }
            else
            {
                var targetType = value.targetType.toString();
                if (targetType.includes("Windows::Foundation::ValueScalar<bool>"))
                {
                    value = "ValueScalar<bool>(" + !!value.dereference().runtimeTypedObject._value + ")";
                }
                else if (targetType.includes("Windows::Foundation::ValueScalar<int>"))
                {
                    value = "ValueScalar<int>(" + value.dereference().runtimeTypedObject._value + ")";
                }
                else if (targetType.includes("Windows::Foundation::ValueScalar<__int64>"))
                {
                    value = "ValueScalar<__int64>(" + value.dereference().runtimeTypedObject._value + ")";
                }
                else if (targetType.includes("ValueScalar<HSTRING"))
                {
                    value = value.dereference().runtimeTypedObject.toString();
                }
                else
                {
                    value = "(" + value.targetType + ")0x" + value.address.toString(16);
                }
            }
        }
        else
        {
            value = value.toString();
        }

        return value;
    }
}

// Wrapper for a single sparse property entry
class XamlSparseProperty
{
    constructor(sparseProperty)
    {
        this.__sparseProperty = sparseProperty;
    }

    toString()
    {
        var propName = this.PropertyName;
        var valueStr = "";
        try
        {
            valueStr = this.Value.toString();
        }
        catch (e)
        {
            valueStr = "error getting value: " + e;
        }
        return propName + "=" + valueStr;
    }

    get Property()
    {
        return this.__GetDependencyProperty();
    }

    __GetDependencyProperty()
    {
        try
        {
            var module = XAML_DLL_Module_from_targetType(this.__sparseProperty.targetType);
            var metadataStorage = host.getModuleSymbol(module, "s_storage", "DirectUI::DynamicMetadataStorage*");
            if (!metadataStorage.isNull)
            {
                var propArrayRaw;
                if (metadataStorage.m_dpHandleCache._Myptr != undefined)
                {
                    propArrayRaw = metadataStorage.m_dpHandleCache._Myptr._Myfirst; // early RS3 and before
                }
                else
                {
                    propArrayRaw = metadataStorage.m_dpHandleCache._Mypair._Myval2._Mypair._Myval2._Myfirst; // late RS3+
                }
                var dptypename = "Windows::UI::Xaml::IDependencyProperty**";
                if (module.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll"))
                {
                    // Need to include the ABI namespace for WinUI3
                    dptypename = "ABI::Microsoft::UI::Xaml::IDependencyProperty**";
                }
//                host.diagnostics.debugLog("**  propArrayRaw.address: " + propArrayRaw.address.toString(16) + " module: " + module + " dptypename: " + dptypename + "\n");
//                host.diagnostics.debugLog("**  prop.first: " + this.__sparseProperty.first + "\n");
//                host.diagnostics.debugLog("**  prop: " + metadataStorage.m_dpHandleCache[this.__sparseProperty.first] + "\n");
                var propArray = host.createPointerObject(propArrayRaw.address, module, dptypename);
                var propHandleAddr = propArray[this.__sparseProperty.first];
                if (propHandleAddr.isNull) // build-in DP's are not not in the m_dpHandleCache list
                {
                    // Get a reference to the first property, then use its targetLocation to create
                    // the array pointer we can .add(index) off.
                    var firstPropObj = host.getModuleSymbol(module, "c_aProperties", "MetaDataProperty");
                    var knownPropsArray = host.createPointerObject(firstPropObj.targetLocation.address, module, "CDependencyProperty*");

                    var dp = knownPropsArray.add(this.__sparseProperty.first);
                    return dp;
                }

                var propHandle = propHandleAddr.dereference().runtimeTypedObject;
                var dp = propHandle.m_pDP;
                if (dp.m_nIndex != this.__sparseProperty.first)
                {
                    host.diagnostics.debugLog("*** Error in property lookup!\n");
                }

                var MetaDataPropertyInfoFlags_IsCustomDependencyProperty = 0x1000;
                if (dp.m_flags & MetaDataPropertyInfoFlags_IsCustomDependencyProperty)
                {
                    dp = host.createPointerObject(dp.address, module, "CCustomDependencyProperty*");
                }
                return dp;
            }
        }
        catch (e)
        {
            host.diagnostics.debugLog("Get DP Exception: " + e + "\n");
        }
        return null;
    }

    get TestPropertyName() { return this.__sparseProperty.first; }
    get PropertyName()
    {
        try
        {
            var module = XAML_DLL_Module_from_targetType(this.__sparseProperty.targetType);
            var name = __GetNameForEnumValue(this.__sparseProperty.first, "KnownPropertyIndex", module, null);
            if (name != null)
            {
                return name;
            }

            // Not a known name, so this must be a custom DependencyProperty
            var dp = this.__GetDependencyProperty();
            var MetaDataPropertyInfoFlags_IsCustomDependencyProperty = 0x1000;
            if (dp != null && (dp.m_flags & MetaDataPropertyInfoFlags_IsCustomDependencyProperty))
            {
                var name = dp.m_strName.toString();
                if (name.length > 0)
                {
                    return name;
                }
            }
        }
        catch (e)
        {
            return "Exception: " + e;
        }
        return "" + this.__sparseProperty.first + " (Unknown)";
    }

    get Value()
    {
        if (this.__cvalue == undefined) // cached for perf
        {
            this.__cvalue = new __CValue(this.__sparseProperty.second.value);
        }
        return this.__cvalue;
    }

    ValueToString()
    {
        var valueStr = "";
        try
        {
            valueStr = this.Value.toString();
        }
        catch (e)
        {
            valueStr = "error getting value: " + e;
        }
        return valueStr;
    }
}

// Wrapper class for "simple property" (non-DP-based property)
class XamlSimpleProperty
{
    constructor(name, value)
    {
        this.__name = name;
        this.__value = value;
    }

    toString()
    {
        return this.__name;
    }

    get PropertyName()
    {
        return this.__name;
    }

    get Value()
    {
        return this.__value;
    }
}

// Visualizer for a sparse property entry (not used via !xamlelement, but
// used when drilling down into an element's data directly).
class __SparsePropertyVisualizer
{
    toString()
    {
        return (new XamlSparseProperty(this)).toString();
    }

    get SparsePropertyVisualizerType()
    {
        return this.targetType;
    }
    get Property()
    {
        host.diagnostics.debugLog("** __SparsePropertyVisualizer.Property\n");
        return (new XamlSparseProperty(this)).Property;
    }

    get PropertyName()
    {
        return (new XamlSparseProperty(this)).PropertyName;
    }

    get Value()
    {
        return (new XamlSparseProperty(this)).Value;
    }
}

// ----------------------------------------------------------------------------
//                 Errors (and more) section
// ----------------------------------------------------------------------------
function __GetStowedExceptionsForThread(thread)
{
    var tlsSlot = host.getModuleSymbol(XAML_DLL_Module_from_thread(thread), "g_dwErrorContextTlsIndex", "int");

    var ptr;
    if (tlsSlot < 64)
    {
        // TODO: check thread.NativeEnvironment if running WOW64
        ptr = thread.Environment.EnvironmentBlock.TlsSlots[tlsSlot];
    }
    else
    {
        ptr = thread.Environment.EnvironmentBlock.TlsExpansionSlots[tlsSlot - 64];
    }

    var list = [];
    if (!ptr.isNull)
    {
        var errorContext = host.createPointerObject(ptr.address, XAML_DLL_Module_from_thread(thread), "ErrorContext*");
        while (!errorContext.isNull)
        {
            try { host.memory.readMemoryValues(ptr, 1); } catch(e) { throw new Error("Heap not available."); }
            list.push(errorContext);
            errorContext = errorContext.next;
        }
    }
    return new __List(list, "ErrorContexts");
}

class __xamlError
{
    constructor(error, thread)
    {
        this.__error = error;
        this.__thread = thread;
        error = error.dereference().runtimeTypedObject;
    }

    toString()
    {
        return this.ErrorString;
    }

    get Address()
    {
        return this.__error;
    }

    get Type()
    {
        return __GetNameForEnumValue(this.__error.m_eType, "DirectUI::ErrorType", XAML_DLL_Module_from_thread(this.__thread), "Unknown");
    }

    get ErrorString()
    {
        var errorStr = '"' + this.__error.m_strErrorDescription.toString() + '"';
        if (this.__error.m_uLineNumber != 0)
        {
            errorStr += " [Line: " + this.__error.m_uLineNumber + " Position: " + this.__error.m_uCharPosition + "]";
        }
        return errorStr;
    }

    get XamlFileName()
    {
        if (this.__error.targetType.name.startsWith("CParserError"))
        {
            var str = this.__error.m_strXamlFileName.m_encodedStorage.toString();
            // Only return this string if it is actually a string (starting with a quote).
            // Otherwise it probably is [object Object] due to NatVis output not returning
            // a string for an empty xstring_ptr.
            if (str.startsWith("\""))
            {
                return str;
            }
        }
        return undefined;
    }
}

class __xamlErrorInfo
{
    constructor(thread, coreServices)
    {
        this.__thread = thread;
        this.__coreServices = coreServices;
    }

    toString()
    {
        var str = "";

        var stowedExceptions = this.StowedExceptions;
        if (stowedExceptions != null && stowedExceptions.length > 0)
        {
            str += "" + stowedExceptions.length + " stowed exceptions";
        }

        var errors = this.Errors;
        if (errors != null && errors.length > 0)
        {
            if (str.length > 0) { str = str + ", "; }
            str += "" + errors.length + " errors";
        }

        if (str.length == 0) { return "No errors" }
        return str;
    }

    get StowedExceptions()
    {
        return __GetStowedExceptionsForThread(this.__thread);
    }

    get Errors()
    {
        var browserHost = this.__coreServices.m_pBrowserHost;
        browserHost = browserHost.dereference().runtimeTypedObject;

        var errorService = browserHost.m_pErrorService;
        errorService = errorService.dereference().runtimeTypedObject;
        var errorList = errorService.m_pErrorList;

        var results = [];

        if (!errorList.isNull)
        {
            var errorNode = errorList.m_pHead;
            while (!errorNode.isNull)
            {
                var error = errorNode.m_pData;
                errorNode = errorNode.m_pNext;

                error = error.dereference().runtimeTypedObject;
                error = __createRuntimeTypedObjectPointer(error);
                results.push(new __xamlError(error, this.__thread));
            }
        }

        return new __List(results, "Errors");
    }

    get WarningContexts()
    {
        var thread = this.__thread;
        var tlsSlot = host.getModuleSymbol(XAML_DLL_Module_from_thread(thread), "g_dwWarningContextTlsIndex", "int");

        var ptr;
        if (tlsSlot < 64)
        {
            // TODO: check thread.NativeEnvironment if running WOW64
            ptr = thread.Environment.EnvironmentBlock.TlsSlots[tlsSlot];
        }
        else
        {
            ptr = thread.Environment.EnvironmentBlock.TlsExpansionSlots[tlsSlot - 64];
        }

        var list = [];
        if (!ptr.isNull)
        {
            var warningContext = host.createPointerObject(ptr.address, XAML_DLL_Module_from_thread(thread), "WarningContext*");
            while (!warningContext.isNull)
            {
                try { host.memory.readMemoryValues(ptr, 1); } catch(e) { throw new Error("Heap not available."); }
                list.push(warningContext);
                warningContext = warningContext.next;
            }
        }
        return new __List(list, "WarningContexts");
    }
}

class __xamlDeferredInvoke
{
    constructor(deferredInvoke)
    {
        this.__deferredInvoke = deferredInvoke;
    }

    toString()
    {
        return this.Type + " (" + this.EventHandle + ")";
    }

    get DeferredInvoke()
    {
        return this.__deferredInvoke;
    }

    get Type()
    {
        switch (this.__deferredInvoke.Msg)
        {
        case 0x402: return "WM_INTERNAL_TICK";
        case 0x403: return "WM_SCRIPT_CALL_BACK";
        case 0x406: return "WM_APPLICATION_STARTUP_EVENT_COMPLETE";
        case 0x410: return "WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY";
        }

        return "???";
    }

    get EventInfo()
    {
        if (this.__deferredInvoke.Msg == 0x403)
        {
            return host.createPointerObject(this.__deferredInvoke.LParam, XAML_DLL_Module_from_targetType(this.__deferredInvoke.targetType), "CEventInfo*", host.currentProcess);
        }

        return undefined;
    }

    get EventHandle()
    {
        var e = this.EventInfo;
        if (e !== undefined)
        {
            return __GetNameForEnumValue(e.Event.index, "KnownEventIndex", XAML_DLL_Module_from_targetType(this.__deferredInvoke.targetType), "unknown");
        }

        return undefined;
    }
}

class __xamlRenderData
{
    constructor(thread, dxamlCore, coreServices)
    {
        this.__thread = thread;
        this.__dxamlCore = dxamlCore;
        this.__coreServices = coreServices;
    }

    toString()
    {
        var str = "Frame " + this.FrameNumber + " (render " + (this.RenderEnabled ? "enabled" : "disabled") + ")";
        if (!this.RenderEnabled)
        {
            if (this.Suspended)
            {
                str += " (suspended)";
            }
            else if (!this.WindowVisible)
            {
                str += " (window hidden)";
            }
        }

        return str;
    }

    get FrameNumber()
    {
        return this.__coreServices.m_uFrameNumber;
    }

    get RenderEnabled()
    {
        return !!this.__coreServices.m_isRenderEnabled;
    }

    get DeviceLost()
    {
        return !!this.__coreServices.m_deviceLost;
    }

    get Suspended()
    {
        return !!this.__coreServices.m_isSuspended;
    }

    get SuspendedReason()
    {
        var suspendReason = __GetNameForEnumValue(this.__coreServices.m_currentSuspendReason,
            "CCoreServices::SuspendReason", XAML_DLL_Module_from_targetType(this.__coreServices.targetType), this.__coreServices.m_currentSuspendReason);
        return suspendReason;
    }

    get SurfacesOffered()
    {
        var dcompTreeHost = this.DCompTreeHost;
        if (dcompTreeHost.m_offerTracker != undefined)
        {
            var offerTracker = this.DCompTreeHost.m_offerTracker.m_ptr;
            if (!offerTracker.isNull)
            {
                return !!offerTracker.m_offered; // Note: !! because m_offered is currently a BOOL
            }
            else
            {
                return false;
            }
        }
        else // on RS1, this was in a different variable
        {
            return dcompTreeHost.m_offered;
        }
    }

    get WindowVisible()
    {
        return !!this.__coreServices.m_isWindowVisible;
    }

    get WindowWasEverMadeVisible()
    {
        return !!this.__coreServices.m_wasWindowEverMadeVisible;
    }

    get CoreWindowVisible()
    {
        return this.__dxamlCore.m_pControl.m_pWindow.m_pCoreWindow.dereference().runtimeTypedObject._fWindowVisible;
    }

    get WindowState()
    {
        var state = "" + this.__dxamlCore.m_pControl.m_pWindow.m_windowActivationState + "=";
        var stateName = __GetNameForEnumValue(this.__dxamlCore.m_pControl.m_pWindow.m_windowActivationState,
            "JupiterWindowActivationState::Enum", XAML_DLL_Module_from_targetType(this.__dxamlCore.targetType), "**Unknown**");
        state += stateName;

        if (this.__dxamlCore.m_pControl.m_pWindow.m_wasWindowEverActivated)
        {
            state += " (has been activated)";
        }
        else
        {
            state += " (never activated)";
        }

        return state;
    }

    get DCompTreeHost()
    {
        var renderTarget = this.__coreServices.m_pNWWindowRenderTarget;
        var graphicsDeviceManager;
        if (renderTarget.m_graphicsDeviceManager != undefined)
        {
            graphicsDeviceManager = renderTarget.m_graphicsDeviceManager.m_ptr;
        }
        else // on RS1, this was in a different variable
        {
            graphicsDeviceManager = renderTarget.m_pIGraphicsDeviceManager.dereference().runtimeTypedObject;
        }
        return graphicsDeviceManager.m_pDCompTreeHost;
    }

    get DeferredInvokeList()
    {
        var browserHostInterface = this.__dxamlCore.m_hCore.m_pBrowserHost;
        var bh = browserHostInterface.dereference().runtimeTypedObject;

        var dispatcherInterface = bh.m_pDispatcherNoRef;
        var dispatcher = dispatcherInterface.dereference().runtimeTypedObject;

        var list = [];
        var deferredItem = dispatcher.m_DeferredInvoke.m_pHead;
        while (!deferredItem.isNull)
        {
            list.push(new __xamlDeferredInvoke(deferredItem));
            try
            {
                deferredItem = deferredItem.Next;
            }
            catch(exp)
            {
                host.diagnostics.debugLog("Warning: Error walking DeferredInvoke list.\n");
                break;
            }
        }

        return new __List(list, "PendingMessages");
    }
}

class ElementWrapper
{
    constructor(element)
    {
        this.__element = element;
    }

    toString()
    {
        return "0x" + this.__element.address.toString(16) + " [Type: " + this.__element.targetType + "]";
    }

    get Element()
    {
        return this.__element;
    }
}

class ContentRootWrapper
{
    constructor(contentRoot)
    {
        this.__contentRoot = contentRoot.m_ptr;
    }

    toString()
    {
        var id = this.Identifier;
        if (id != undefined)
        {
            return id;
        }
        return "0x" + this.__contentRoot.address.toString(16) + " [Type: " + this.__contentRoot.targetType + "]";
    }

    get ContentRoot()
    {
        return this.__contentRoot;
    }

    get RootType()
    {
        return __GetNameForEnumValue(this.__contentRoot.m_type, "CContentRoot::Type", XAML_DLL_Module_from_targetType(this.__contentRoot.targetType), "Unknown");
    }

    get RootElement()
    {
        return new ElementWrapper(this.__contentRoot.m_visualTree.m_rootElement);
    }

    get Identifier()
    {
        try
        {
            var element = this.__contentRoot.m_visualTree.m_publicRootVisual.m_ptr;
            if (element == null || element.address == 0)
            {
                return "Empty";
            }
            var name = element.m_strName.toString();
            var className = "";
            var checkChild = true; // check the child if the root doesn't have a good name/type
            element = element.dereference().runtimeTypedObject; // convert from CDependencyObject to subclass type
            if (element.m_strClassName != undefined)
            {
                className = element.m_strClassName.toString();
                if (className != "")
                {
                    // Check if it is a built-in XAML class name. The name may be quoted, so strip that
                    // off before checking the namespace.
                    var classNameUnquoted = className.startsWith('"') ? className.substring(1) : className;
                    if (!classNameUnquoted.startsWith("Windows.UI.Xaml.") && !classNameUnquoted.startsWith("Microsoft.UI.Xaml."))
                    {
                        checkChild = false; // Looks like a custom type. Use it.
                    }
                }
            }
            if (name == "" && checkChild && !element.m_pChildren.isNull)
            {
                var children = element.m_pChildren;
                element = element.m_pChildren.m_items[0];
                if (element != null)
                {
                    element = element.dereference().runtimeTypedObject; // convert from CDependencyObject to subclass type
                    name = element.m_strName.toString();

                    if (element.m_strClassName != undefined)
                    {
                        className = element.m_strClassName.toString();
                    }
                }
            }
            if (name != "")
            {
                name = "Name=" + name;
            }
            if (name != "" && className != "")
            {
                name += " ";
            }
            if (className != "")
            {
                return name + "Type=" + className;
            }
            if (name == "")
            {
                return element.runtimeTypedObject.targetType;
            }
            return name;
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Unable to determine ContentRoot Identifier error: " + exp + "\n");
            return;
        }
    }
}

class IslandsCollection
{
    constructor(contentRootCoordinator)
    {
        this.__contentRootCoordinator = contentRootCoordinator;
    }

    toString()
    {
        var count = this.__GetCount();
        var str = count + " Islands";

        if (count > 0)
        {
            str += " (";
            var idx = 0;
            for (var contentRoot of this)
            {
                if (idx > 0)
                {
                    str += "; ";
                }
                str += "#" + idx + ": " + contentRoot.toString();
                idx++;

                // only dump the first 2
                if (idx == 2 && count > 2)
                {
                    str += "; ...";
                    break;
                }
            }
            str += ")";
        }

        return str;
    }

    __GetVectorData()
    {
        return this.__contentRootCoordinator.m_contentRoots._Mypair._Myval2;
    }

    __GetCount()
    {
        var count = this.__GetRawCount();
        try
        {
            var vectorData = this.__GetVectorData();
            var contentRoot = vectorData._Myfirst;
            for (var i = 0; i < count; i++)
            {
                var wrapper = new ContentRootWrapper(contentRoot);
                var rootType = wrapper.RootType.toString();
                if (rootType == "CoreWindowAndXamlPresenter" || rootType == "CoreWindow")
                {
                    return count-1;
                }
                contentRoot = contentRoot.add(1);
            }
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Error in iterator: " + exp);
        }
        return count;
    }

    __GetRawCount()
    {
        var vectorData = this.__GetVectorData();

        var count = (vectorData._Mylast.address - vectorData._Myfirst.address) / vectorData._Myfirst.dereference().targetSize;
        return count;
    }

    *[Symbol.iterator]()
    {
        try
        {
            var vectorData = this.__GetVectorData();
            var count = (vectorData._Mylast.address - vectorData._Myfirst.address) / vectorData._Myfirst.dereference().targetSize;
            var contentRoot = vectorData._Myfirst;
            for (var i = 0; i < count; i++)
            {
                var wrapper = new ContentRootWrapper(contentRoot);
                var rootType = wrapper.RootType.toString();
                if (rootType != "CoreWindowAndXamlPresenter" && rootType != "CoreWindow")
                {
                    yield wrapper;
                }
                contentRoot = contentRoot.add(1);
            }
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Error in iterator: " + exp);
        }
    }
}

class XamlThread
{
    constructor(thread, dxamlCore)
    {
        this.__thread = thread;
        this.__dxamlCore = dxamlCore;
        this.__isHeapAvailable = true;
        try { host.memory.readMemoryValues(this.__dxamlCore, 1); } catch(e) { this.__isHeapAvailable = false; }
    }

    toString()
    {
        var str = "DXamlCore = " + this.__dxamlCore.address.toString(16);
        if (!this.__isHeapAvailable)
        {
            return str += " (-- heap not available or bad address --)";
        }
        try
        {
            var page = this.FindRootPage();
            if (page == null)
            {
                str += " (empty)";
            }
            else
            {
                // TODO: Should this just use XamlElement.DerivedType?
                str += " (" + this.GetSubclassTypeNameForElement(page) + ")";
            }
        }
        catch (exp)
        {
            host.diagnostics.debugLog("error in toString(): " + exp + "\n");
            return "unknown";
        }

        if (this.__thread.Id == host.currentThread.Id)
        {
            str += " (current thread)";
        }

        return str;
    }

    get DXamlCore()
    {
        return this.__dxamlCore;
    }

    get CCoreServices()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        return this.__dxamlCore.m_hCore;
    }

    get CJupiterWindow()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        return this.__dxamlCore.m_pControl.m_pWindow;
    }

    get RootElement()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        if (this.__dxamlCore.m_hCore.m_pMainVisualTree.m_pRootVisual != undefined)
        {
            return new ElementWrapper(this.__dxamlCore.m_hCore.m_pMainVisualTree.m_pRootVisual);
        }
        else
        {
            return new ElementWrapper(this.__dxamlCore.m_hCore.m_pMainVisualTree.m_rootVisual.m_ptr);
        }
    }

    get Islands()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        var islands = new IslandsCollection(this.__dxamlCore.m_hCore.m_contentRootCoordinator);
        return islands;
    }

    get RenderData()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        return new __xamlRenderData(this.__thread, this.__dxamlCore, this.__dxamlCore.m_hCore);
    }

    get ErrorInfo()
    {
        if (!this.__isHeapAvailable) { return "  -- not available --"; }
        return new __xamlErrorInfo(this.__thread, this.__dxamlCore.m_hCore);
    }

    GetSubclassTypeNameForElement(element)
    {
        try
        {
            if (element != null)
            {
                var peer = __GetPeerDOForCDO(element);
                peer = peer.dereference().runtimeTypedObject;

                var controllingUnknown = peer.m_pControllingUnknown;
                if (controllingUnknown.address != 0)
                {
                    type = InferTypeNameForAddress(controllingUnknown.address);
                    return type;
                }
            }

            var type = InferTypeNameForAddress(element.address);
            if (type != null)
            {
                return type;
            }

            return "???";
        }
        catch (exp)
        {
            host.diagnostics.debugLog("GetSubclassTypeNameForElement error: " + exp + "\n");
            return "unknown";
        }
    }

    FindRootPage()
    {
        try
        {
            var page = null;
            var element = this.__dxamlCore.m_hCore.m_pMainVisualTree.m_pPublicRootVisual;
            if (element == undefined)
            {
                element = this.__dxamlCore.m_hCore.m_pMainVisualTree.m_publicRootVisual.m_ptr;
            }
            if (element == null || element.address == 0)
            {
                return null;
            }
            page = element; // best element so far

            var type = InferTypeNameForAddress(element.address);
            if (type != null && (type == "Windows_UI_Xaml!CFrame" || type == "Microsoft_UI_Xaml!CFrame"))
            {
                element = element.m_pChildren.m_items[0];
                if (element != null)
                {
                    page = element; // best element so far
                    element = element.dereference().runtimeTypedObject; // dereference the pointer and switch to derived UDT

                    element = element.m_pChildren.m_items[0];
                    if (element != null)
                    {
                        page = element; // best element so far
                    }
                }
            }

            return page;
        }
        catch(exp)
        {
            host.diagnostics.debugLog("FindRootPage error: " + exp + "\n");
            return null;
        }
    }

    SwitchToThread()
    {
        this.__thread.SwitchTo();
    }

    get [Symbol.metadataDescriptor]()
    {
        var switchToThreadMetadata = { ActionName: "SwitchToThread", ActionDescription: "SwitchToThread(): Switch to this thread as the default context",  ActionIsDefault: true };
        return { SwitchToThread: switchToThreadMetadata };
    }
    
}

function listXamlThreads()
{
    // Check if symbols are loaded for XAML. If not, there is nothing we can do.
    if (!__CheckXAMLSymbolsLoaded())
    {
        return;
    }

    // Filter to XAML threads
    var threads = host.currentProcess.Threads.Where(function(t) { return t.UI != undefined && t.UI.Xaml != undefined });

    // Select the XAML object for those threads
    return threads.Select(function(t) { return t.UI.Xaml });
}

// Sparse properties list used by the XamlElement wrapper (for !xamlelement)
class XamlElementSparseProperties
{
    constructor(element)
    {
        this.__element = element;
    }

    toString()
    {
        var str = "" + this.__GetCount() + " sparse properties";
        return str;
    }

    __GetVectorData()
    {
        if (this.__element.m_pValueTable._Myptr != undefined)
        {
            return this.__element.m_pValueTable._Myptr.m_data.m_vector; // early RS3 and before
        }
        else
        {
            return this.__element.m_pValueTable._Mypair._Myval2.m_data.m_vector._Mypair._Myval2; // late RS3+
        }
    }

    __GetCount()
    {
        var vectorData = this.__GetVectorData();

        var count = (vectorData._Mylast.address - vectorData._Myfirst.address) / vectorData._Myfirst.dereference().targetSize;
        return count;
    }

    *[Symbol.iterator]()
    {
        try
        {
            var vectorData = this.__GetVectorData();
            if (vectorData.targetLocation == 0)
            {
                return;
            }
            var count = (vectorData._Mylast.address - vectorData._Myfirst.address) / vectorData._Myfirst.dereference().targetSize;
            var prop = vectorData._Myfirst;
            for (var i = 0; i < count; i++)
            {
                yield new XamlSparseProperty(prop);
                prop = prop.add(1);
            }
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Error in iterator: " + exp);
        }
    }
}

// Holds the list of "simple properties" (non-DP-based properties) for a given CDO*
// Used by !xamlelement and !xamlelementsp
class XamlElementSimpleProperties
{
    constructor(element)
    {
        this.__element = element;
        this.__simpleProperties = [];
        this.__FindSimpleProperties();
    }

    toString()
    {
        var count = this.__simpleProperties.length;
        var str = "" + count + " simple properties";
        return str;
    }

    NumSimpleProperties()
    {
        return this.__simpleProperties.length;
    }

    __TryAddSimpleProperty(propertyName, sparsetable)
    {
        try
        {
            var vectorData = sparsetable.m_data.m_vector._Mypair._Myval2;
            var count = (vectorData._Mylast.address - vectorData._Myfirst.address) / vectorData._Myfirst.dereference().targetSize;
            var prop = vectorData._Myfirst;
            for (var i = 0; i < count; i++)
            {
                if (prop.first.address == this.__element.address)
                {
                    this.__simpleProperties.push(new XamlSimpleProperty(propertyName, prop.second));
                    break;
                }
                prop = prop.add(1);
            }
        }
        catch(exp)
        {
            host.diagnostics.debugLog("Error adding simple property: " + exp);
        }
    }

    __FindSimpleProperties()
    {
        var sparsetables = __GetCoreForCDO(this.__element).m_sparseTables;

        if (sparsetables != undefined)
        {
            // Note:  This list of simple properties must be kept manually in-sync with XAML
            this.__TryAddSimpleProperty("Translation", sparsetables.m_UIElement_Translation);
            this.__TryAddSimpleProperty("Rotation", sparsetables.m_UIElement_Rotation);
            this.__TryAddSimpleProperty("Scale", sparsetables.m_UIElement_Scale);
            this.__TryAddSimpleProperty("TransformMatrix", sparsetables.m_UIElement_TransformMatrix);
            this.__TryAddSimpleProperty("RotationAxis", sparsetables.m_UIElement_RotationAxis);
            this.__TryAddSimpleProperty("CenterPoint", sparsetables.m_UIElement_CenterPoint);
            this.__TryAddSimpleProperty("KeepAliveCount", sparsetables.m_UIElement_KeepAliveCount);
            this.__TryAddSimpleProperty("AnimatedTranslation", sparsetables.m_UIElement_AnimatedTranslation);
            this.__TryAddSimpleProperty("AnimatedRotation", sparsetables.m_UIElement_AnimatedRotation);
            this.__TryAddSimpleProperty("AnimatedScale", sparsetables.m_UIElement_AnimatedScale);
            this.__TryAddSimpleProperty("AnimatedTransformMatrix", sparsetables.m_UIElement_AnimatedTransformMatrix);
            this.__TryAddSimpleProperty("AnimatedRotationAxis", sparsetables.m_UIElement_AnimatedRotationAxis);
            this.__TryAddSimpleProperty("AnimatedCenterPoint", sparsetables.m_UIElement_AnimatedCenterPoint);

            this.__TryAddSimpleProperty("Translation", sparsetables.m_LinearGradientBrush_Translation);
            this.__TryAddSimpleProperty("Rotation", sparsetables.m_LinearGradientBrush_Rotation);
            this.__TryAddSimpleProperty("Scale", sparsetables.m_LinearGradientBrush_Scale);
            this.__TryAddSimpleProperty("TransformMatrix", sparsetables.m_LinearGradientBrush_TransformMatrix);
            this.__TryAddSimpleProperty("CenterPoint", sparsetables.m_LinearGradientBrush_CenterPoint);
            this.__TryAddSimpleProperty("AnimatedTranslation", sparsetables.m_LinearGradientBrush_AnimatedTranslation);
            this.__TryAddSimpleProperty("AnimatedRotation", sparsetables.m_LinearGradientBrush_AnimatedRotation);
            this.__TryAddSimpleProperty("AnimatedScale", sparsetables.m_LinearGradientBrush_AnimatedScale);
            this.__TryAddSimpleProperty("AnimatedTransformMatrix", sparsetables.m_LinearGradientBrush_AnimatedTransformMatrix);
            this.__TryAddSimpleProperty("AnimatedCenterPoint", sparsetables.m_LinearGradientBrush_AnimatedCenterPoint);
        }
    }

    *[Symbol.iterator]()
    {
        for (var i = 0; i < this.__simpleProperties.length; i++)
        {
            yield this.__simpleProperties[i];
        }
    }
}

// Element wrapper, used by !xamlelement
class XamlElement
{
    constructor(element)
    {
        this.__element = element;
        this.__sparseProperties = new XamlElementSparseProperties(element);
        this.__simpleProperties = new XamlElementSimpleProperties(element);
    }

    toString()
    {
        var str = "Element = " + this.__element.address.toString(16) + " " + this.__element.targetType;
        return str;
    }

    HasSimpleProperties()
    {
        return this.__simpleProperties.NumSimpleProperties() > 0;
    }

    get Element()
    {
        // Note:  Output for this property can be super slow if the winrt.natvis is slow
        //        (such as for CContentControl).
        return this.__element;
    }

    get DXamlPeer()
    {
        var peer = __GetPeerDOForCDO(this.__element);
        peer = peer.dereference().runtimeTypedObject;
        peer = __createRuntimeTypedObjectPointer(peer);
        return peer;
    }

    get DerivedType()
    {
        if (this.__element.m_strClassName != undefined)
        {
            var str = this.__element.m_strClassName.toString();
            if (str != "" && str != "[object Object]")
            {
                return str;
            }
        }
        // else leave it undefined, to not show this irrelevant property
    }

    get DerivedObject()
    {
        var peer = this.DXamlPeer;
        if (!peer.isNull)
        {
            var obj = peer.m_pControllingUnknown;
            if (!obj.isNull)
            {
                obj = obj.dereference().runtimeTypedObject;
                obj = __createRuntimeTypedObjectPointer(obj);
                if (obj != null && obj.targetType == "IInspectable *")
                {
                    var type = InferTypeNameForAddress(obj.address, false);
                    if (type == null)
                    {
                        return "-- Error: no object found at m_pControllingUnknown = 0x" + peer.m_pControllingUnknown.address.toString(16);
                    }
                    // If this looks like a C++/WinRT type, such as "winrt::impl::produce<AcrylicBrush,winrt::Windows::UI::Xaml::Media::IAcrylicBrush>",
                    // then get to the real class.
                    if ((type.indexOf("winrt::impl::produce<") > 0) && (type.indexOf(",winrt::") > 0))
                    {
                        obj = this.__GetCppWinRTObject(obj, type);
                    }
                }
                return obj;
            }
        }
        // else leave it undefined, to not show this irrelevant property
    }

    __GetCppWinRTObject(obj, type)
    {
        // String out the class name from a type like: "winrt::impl::produce<AcrylicBrush,winrt::Windows::UI::Xaml::Media::IAcrylicBrush>"
        var cppwinrttemplate = "winrt::impl::produce<";
        var subname = type.substring(type.indexOf(cppwinrttemplate) + cppwinrttemplate.length);
        subname = subname.substring(0, subname.indexOf(','));
        var targetTypeName = type.substring(0, type.indexOf("!")+1) + subname;

        var typeInfo = host.namespace.Debugger.Utility.Control.ExecuteCommand("dt " + targetTypeName);
        var vtableName = type.substring(type.indexOf("!")+1); // vtable name, without module
        for (var line of typeInfo)
        {
            line = line.trim();
            if (line.includes(vtableName))
            {
                var offset = host.evaluateExpression(line.substring(0, line.indexOf(" ")));
                var indexSplit = targetTypeName.indexOf("!");
                return host.createPointerObject(obj.address - offset, targetTypeName.substring(0, indexSplit), targetTypeName.substring(indexSplit+1) + "*");
            }
        }

        return obj;
    }

    get SparseProperties()
    {
        return this.__sparseProperties;
    }

    get SimpleProperties()
    {
        return this.__simpleProperties;
    }

    GetShortDerivedName()
    {
        var className = this.DerivedType;
        if (className != undefined)
        {
            // Remove any quotes, and strip off namespace for platform types
            className = className.replace(/"/g, "");
            className = className.replace(/Windows.UI.Xaml.*\./, "");
            className = className.replace(/Microsoft.UI.Xaml.*\./, "");
        }
        else
        {
            // No DerivedType, but maybe we can glean one from the DerivedObject if there is one.
            var derivedObject = this.DerivedObject;
            if (derivedObject != undefined && derivedObject != null && derivedObject.targetType != undefined)
            {
                className = derivedObject.targetType.baseType.toString();
            }
        }
        return className;
    }
}

// !xamlelement function
function dumpXamlElement(elementAddr)
{
    if (!__CheckXAMLSymbolsLoaded())
    {
        return;
    }

    if (elementAddr == undefined)
    {
        host.diagnostics.debugLog("Usage: !xamlelement <pointer to element>\n");
        return null;
    }

    var element = CreateTypedPointerInferredForAddress(elementAddr);
    if (element == null)
    {
        host.diagnostics.debugLog("Error: Unable detect address as core or dxaml element pointer.\n");
        return null;
    }

    if (element.m_pDXAMLPeer == undefined)
    {
        // Maybe this is a pointer to the peer.
        //host.diagnostics.debugLog("Non-core type detected. Checking to see if it is a DXaml peer.\n");
        element = __createRuntimeTypedObjectPointer(element.dereference().runtimeTypedObject);
        if (element.isNull || element.m_pDO == undefined)
        {
            host.diagnostics.debugLog("Error: Unable detect address as core or dxaml element pointer.\n");
            return null;
        }
        if (element.m_bIsDisconnectedFromCore)
        {
            host.diagnostics.debugLog("Warning: Element disconnected from core.\n");
            return element;
        }
        element = element.m_pDO;
        if (!element.isNull)
        {
            // element is currently a CDependencyObject*. Convert to derived type.
            element = __createRuntimeTypedObjectPointer(element.dereference().runtimeTypedObject);
        }
    }

    return new XamlElement(element);
}

// !xamlstowed function
function dumpXamlStowed(dumpCount)
{
    if (!__CheckXAMLSymbolsLoaded())
    {
        return;
    }

    var thread = host.namespace.Debugger.State.DebuggerVariables.curthread;

    try
    {
        var module = XAML_DLL_Module_from_thread(thread);
        // If module is undefined, there is no Xaml on this thread and the above call
        // already printed that out in the window. Just return.
        if (module == undefined)
        {
            return;
        }
        var v = host.getModuleSymbol(module, "g_dwErrorContextTlsIndex", "int");
    }
    catch (exp)
    {
        throw new Error("Heap not available.");
        //return new class SimpleString { toString() { return "Heap not available." } };
    }

    var list = __GetStowedExceptionsForThread(thread);
    if (list.length == 0)
    {
        host.diagnostics.debugLog("No stowed exceptions on current thread [" + thread.Id.toString(16) + "]\n");
    }
    else if (dumpCount == undefined || dumpCount > 0)
    {
        for (var se of list)
        {
            host.diagnostics.debugLog("-------------------------\n");
            host.diagnostics.debugLog("Callstack for hr=" + se.HRESULT + "\n");
            host.diagnostics.debugLog("\n");
            for (var f of se.Stack)
            {
                if (f.Print != undefined)
                {
                    f.Print("    ");
                }
                else
                {
                    host.diagnostics.debugLog("    " + f + "\n");
                }
            }

            if (dumpCount != undefined && --dumpCount == 0)
            {
                break;
            }
        }
    }
    host.diagnostics.debugLog("\n");
    host.diagnostics.debugLog("=========================\n");
    return list;
}

// TriageData, used by !xamlelement
class TriageData
{
    constructor(element)
    {
        this.__Initialize();
    }

    toString()
    {
        return this.__errorCodeString;
    }

    get ErrorCode()
    {
        return this.__errorCode;
    }

    get ErrorCodeString()
    {
        return this.__errorCodeString;
    }

    get ErrorInfo()
    {
        return this.__blameError;
    }

    get ErrorMessage()
    {
        return this.__errorMessage;
    }

    get OriginateError()
    {
        return this.__originateError;
    }

    get StowedExceptions()
    {
        return this.__stowedExceptions;
    }

    get CPPWinRTException()
    {
        return this.__cppWinRTException;
    }

    get CXXPlatformException()
    {
        return this.__cxxPlatformException;
    }

    __Initialize()
    {
        var exrInfo = host.namespace.Debugger.State.PseudoRegisters.Errors;

        var errorCodeString = __GetErrorStringFromHRESULT(exrInfo.exr_code);

        this.__errorCode = exrInfo.exr_code;
        this.__errorCodeString = errorCodeString;

        // Check if we have symbols for combase, for the parts of this function which need that.
        try
        {
            var v = host.createPointerObject(0, "combase.dll", "_STOWED_EXCEPTION_INFORMATION_HEADER **", host.currentProcess);
        }
        catch (exp)
        {
            if (exp.toString().indexOf("Invalid argument to method 'createPointerObject'") > 0)
            {
                host.diagnostics.debugLog("*** WARNING: Symbols for combase.dll not loaded/unavailable.\n");
                // TODO: handle errors resulting from this
            }
            // else the error might be lack of heap or something else.
        }

        if (exrInfo.exr_code == 0xc000027b) // STATUS_STOWED_EXCEPTION
        {
            var rawArrayStowed = host.createPointerObject(exrInfo.exr_param0, "combase.dll", "_STOWED_EXCEPTION_INFORMATION_HEADER **", host.currentProcess);
            var count = exrInfo.exr_param1;

            try { host.memory.readMemoryValues(rawArrayStowed, 1); } catch(e) { throw new Error("Stowed exception memory not available."); }

            var STOWED_EXCEPTION_INFORMATION_V1_SIGNATURE = 0x53453031; // 'SE01'
            var STOWED_EXCEPTION_INFORMATION_V2_SIGNATURE = 0x53453032; // 'SE02'

            var array = [];
            for (var i = 0; i < count; i++)
            {
                var header = rawArrayStowed[i];

                // Check if we can read from the header struct. Some dumps don't
                // contain this memory.
                try
                {
                    host.memory.readMemoryValues(header.address, 1);
                }
                catch(e)
                {
                    var wrapper = new __CombaseStowedExceptionWrapper(header);
                    wrapper.ErrorMessage = "(-- heap not available --)";
                    array.push(wrapper);
                    continue;
                }

                // Can read from the struct. Handle it appropriately.
                if (header.Signature == STOWED_EXCEPTION_INFORMATION_V2_SIGNATURE)
                {
                    var se02 = host.createPointerObject(header.targetLocation.address, "combase.dll", "_STOWED_EXCEPTION_INFORMATION_V2*", host.currentProcess);
                    array.push(new __CombaseStowedExceptionWrapper(se02));
                }
                else if (header.Signature == STOWED_EXCEPTION_INFORMATION_V1_SIGNATURE)
                {
                    var se01 = host.createPointerObject(header.targetLocation.address, "combase.dll", "_STOWED_EXCEPTION_INFORMATION_V1*", host.currentProcess);
                    array.push(new __CombaseStowedExceptionWrapper(se01));
                }
                else
                {
                    array.push(header);
                }
            }
            this.__stowedExceptions = new __List(array, "StowedExceptions");

            if (count > 0)
            {
                this.__blameError = array[0];
            }
        }
        else
        {
            // Yuck: Switch to the exception context so we can get the correct stack
            host.namespace.Debugger.Utility.Control.ExecuteCommand(".ecxr");

            var stack = [];
            for (var frame of host.namespace.Debugger.State.DebuggerVariables.curstack.Frames)
            {
                //stack.push(new __codePointerInfo(frame)); // can't get the address from frame!
                stack.push(frame);
            }
            this.__blameError = new __NonStowedExceptionErrorWrapper(this.ErrorCode, this.ErrorCodeString, new __List(stack, "Frames"));
        }

        // OriginateError
        var thread = host.namespace.Debugger.State.DebuggerVariables.curthread;
        var reservedForOle = null;
        try
        {
            reservedForOle = thread.Environment.EnvironmentBlock.ReservedForOle;
        }
        catch (inaccessibleError)
        {
            // An error getting ReservedForOle likely means symbols weren't loaded.
            host.diagnostics.debugLog("Warning: Unable to read from environment block. Possible symbols issue.\n");
        }
        if (reservedForOle != null && !reservedForOle.isNull)
        {
            var oleTlsData = host.createPointerObject(reservedForOle.address, "combase.dll", "SOleTlsData*", host.currentProcess);
            var isHeapAvailable = true;
            try { oleTlsData.punkError.isNull; } catch(e) { isHeapAvailable = false; }
            if (isHeapAvailable && !oleTlsData.punkError.isNull)
            {
                var restrictedError = __createRuntimeTypedObjectPointer(oleTlsData.punkError.dereference().runtimeTypedObject);
                this.__originateError = new __CRestrictedErrorWrapper(restrictedError);
                this.__errorMessage = this.__originateError.RestrictedDescription;
                if (this.__errorMessage != undefined && !this.__errorMessage.isNull)
                {
                    this.__errorMessage = host.memory.readWideString(this.__errorMessage.address).trim();
                }

                if (this.__stowedExceptions != undefined && this.__stowedExceptions.length > 0)
                {
                    this.__stowedExceptions.GetArray()[0].ErrorMessage = this.__errorMessage;
                }
            }
        }

        // C++/WinRT exception
        var firstFrameName = host.namespace.Debugger.State.DebuggerVariables.curstack.Frames[0].ToDisplayString();
        if (firstFrameName.startsWith("ucrtbase!abort") || firstFrameName.startsWith("ucrtbased!abort"))
        {
            var frameNum = 0;
            for (var frame of host.namespace.Debugger.State.DebuggerVariables.curstack.Frames)
            {
                var frameName = frame.ToDisplayString();
                if (frameName.indexOf("!_CxxThrowException") > 0 || frameName.startsWith("ucrtbase!__CxxFrameHandler2"))
                {
                    // TODO: Should we also check for "!winrt::throw_hresult" in the frame name?
                    // Get the module symbol name of the next frame.
                    var nextFrameSymbol = host.namespace.Debugger.State.DebuggerVariables.curstack.Frames[frameNum+1].ToDisplayString();
                    if (nextFrameSymbol.indexOf("!") > 0)
                    {
                        nextFrameSymbol = nextFrameSymbol.substring(0, nextFrameSymbol.indexOf("!"));
                    }

                    // Convert the module name into the binary name by looking it up in the module list
                    var nextFrameModuleList = host.currentProcess.Modules.Where(function(m) { return m.Symbols.Name == nextFrameSymbol });
                    if (nextFrameModuleList.Count() == 0)
                    {
                        break;
                    }
                    var nextFrameModule = nextFrameModuleList.First();
                    var nextFrameModuleNameLastSlash = nextFrameModule.Name.lastIndexOf("\\");
                    var nextFrameBinaryName = nextFrameModule.Name.substring(nextFrameModuleNameLastSlash+1);
                    var dllName = nextFrameBinaryName;

                    try
                    {
                        // 9/10/2019: It looks like a change in ucrtbase has resulted in _CxxThrowException no longer being
                        //            on the stack, so we need to dig the exceptionAddress out from the _CxxFrameHandler2 frame instead.
                        var exceptionAddress;
                        if (frameName.indexOf("!_CxxThrowException") > 0)
                            exceptionAddress = frame.Parameters.pExceptionObject.address;
                        else // ucrtbase!__CxxFrameHandler2
                            exceptionAddress = frame.Parameters.pExcept.params.pExceptionObject.address;

                        var error = host.createPointerObject(exceptionAddress, dllName, "winrt::hresult_error*", host.currentProcess);
                        if (error.m_debug_magic == 0xaabbccdd) // confirm this really does look like a C++/WinRT hresult_error
                        {
                            this.__cppWinRTException = new __CPPWinRTHResultErrorWrapper(error);
                        }
                        else
                        {
                            // See if it is another known type
                            var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + exceptionAddress.toString(16) + " L1");
                            if (output[0].indexOf("::`vftable") > 0)
                            {
                                var type = output[0];
                                type = type.substring(type.indexOf('!')+1);
                                type = type.substring(0, type.indexOf("::`vftable"));
                                var error = host.createPointerObject(exceptionAddress, dllName, type + "*", host.currentProcess);
                                this.__cppWinRTException = new __CPPWinRTExceptionErrorWrapper(type, error);
                            }
                        }
                    }
                    catch (e)
                    {
                        if (e.toString().indexOf("Invalid argument to method 'createPointerObject'") > 0)
                        {
                            // Probably no symbols for WUCX/MUX
                            // FUTURE:  Just dig out the info based on expected locations???
                        }
                        else
                        {
                            host.diagnostics.debugLog("Error getting info for C++/WinRT exception.\n");
                        }
                    }
                    break;
                }
                else if (frameNum > 12)
                {
                    break;
                }
                else if (
                    frameName.startsWith("ucrtbase") ||
                    frameName.startsWith("VCRUNTIME") ||
                    frameName.indexOf("ExecuteHandler") > 0 ||
                    frameName.indexOf("ExceptionDispatcher") > 0 ||
                    frameName.indexOf("RaiseException"))
                {
                    frameNum++;
                }
            }
        }

        // If we didn't find a C++/WinRT exception above, look for a CxxCallCatchBlock frame to see if we can find one there.
        if (this.__cppWinRTException == undefined)
        {
            var frameNum = 0;
            for (var frame of host.namespace.Debugger.State.DebuggerVariables.curstack.Frames)
            {
                var frameName = frame.ToDisplayString();
                if (frameName.startsWith("ucrtbase!__FrameHandler4::CxxCallCatchBlock"))
                {
                    // Get the module symbol name of the next frame.
                    var nextFrameSymbol = host.namespace.Debugger.State.DebuggerVariables.curstack.Frames[frameNum+1].ToDisplayString();
                    if (nextFrameSymbol.indexOf("ntdll!") >= 0) // skip: ntdll!RcFrameConsolidation
                    {
                        nextFrameSymbol = host.namespace.Debugger.State.DebuggerVariables.curstack.Frames[frameNum+2].ToDisplayString();
                    }
                    if (nextFrameSymbol.indexOf("!") > 0)
                    {
                        nextFrameSymbol = nextFrameSymbol.substring(0, nextFrameSymbol.indexOf("!"));
                    }

                    // Convert the module name into the binary name by looking it up in the module list
                    var nextFrameModuleList = host.currentProcess.Modules.Where(function(m) { return m.Symbols.Name == nextFrameSymbol });
                    if (nextFrameModuleList.Count() == 0)
                    {
                        break;
                    }
                    var nextFrameModule = nextFrameModuleList.First();
                    var nextFrameModuleNameLastSlash = nextFrameModule.Name.lastIndexOf("\\");
                    var nextFrameBinaryName = nextFrameModule.Name.substring(nextFrameModuleNameLastSlash+1);
                    var dllName = nextFrameBinaryName;

                    try
                    {
                        var exceptionAddress = frame.LocalVariables.pFrameInfo.pExceptionObject.address;

                        var error = host.createPointerObject(exceptionAddress, dllName, "winrt::hresult_error*", host.currentProcess);
                        if (error.m_debug_magic == 0xaabbccdd) // confirm this really does look like a C++/WinRT hresult_error
                        {
                            this.__cppWinRTException = new __CPPWinRTHResultErrorWrapper(error);
                        }
                        else
                        {
                            // See if it is another known type
                            var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("dps 0x" + exceptionAddress.toString(16) + " L1");
                            if (output[0].indexOf("::`vftable") > 0)
                            {
                                var type = output[0];
                                type = type.substring(type.indexOf('!')+1);
                                type = type.substring(0, type.indexOf("::`vftable"));
                                var error = host.createPointerObject(exceptionAddress, dllName, type + "*", host.currentProcess);
                                this.__cppWinRTException = new __CPPWinRTExceptionErrorWrapper(type, error);
                            }
                        }
                    }
                    catch (e)
                    {
                        if (e.toString().indexOf("Invalid argument to method 'createPointerObject'") > 0)
                        {
                            // Perhaps no symbols for the target dll?
                            // FUTURE:  Just dig out the info based on expected locations???
                        }
                        else
                        {
                            host.diagnostics.debugLog("Error getting info for C++/WinRT exception.\n");
                        }
                    }
                    break;
                }
                else if (frameNum > 12)
                {
                    break;
                }
                else if (
                    frameName.startsWith("ucrtbase") ||
                    frameName.startsWith("VCRUNTIME") ||
                    frameName.indexOf("ExecuteHandler") > 0 ||
                    frameName.indexOf("ExceptionDispatcher") > 0 ||
                    frameName.indexOf("RaiseException"))
                {
                    frameNum++;
                }
            }
        }

        // wincorlib Platform::Exception
        if (host.namespace.Debugger.State.DebuggerVariables.curstack.Frames[2].ToDisplayString().startsWith("wincorlib!__abi_WinRTraise"))
        {
            var frameNum = 0;
            for (var frame of host.namespace.Debugger.State.DebuggerVariables.curstack.Frames)
            {
                var frameName = frame.ToDisplayString();
                if (frameName.indexOf("_CxxThrowException") > 0)
                {
                    try
                    {
                        var error = host.createPointerObject(frame.Parameters.pExceptionObject.address, "wincorlib.dll", "Platform::Exception**", host.currentProcess);
                        this.__cxxPlatformException = error[0];
                    }
                    catch (e)
                    {
                        if (e.toString().indexOf("Invalid argument to method 'createPointerObject'") > 0)
                        {
                            // Probably no symbols for wincorlib
                        }
                        else
                        {
                            host.diagnostics.debugLog("Error getting info for wincorlib Platform::Exception\n");
                        }
                    }
                    break;
                }
                else if (frameNum > 3)
                {
                    break;
                }
                else if (frameName.indexOf("RaiseException"))
                {
                    frameNum++;
                }
            }
        }

        // TODO:
        //   * OtherErrors
        //     * CLR exception
        //     * XamlStowed

        // TODO: Analyze correct stowed exception to use!
        if (this.__stowedExceptions != undefined && this.__stowedExceptions.length > 0)
        {
        }
    }

    Print(dumpCount)
    {
        if (this.__stowedExceptions == undefined || this.__stowedExceptions.length == 0)
        {
            host.diagnostics.debugLog("Not a stowed exception crash\n");
            if (this.__cppWinRTException != undefined)
            {
                host.diagnostics.debugLog("C++/WinRT exception found\n");
                host.diagnostics.debugLog("-------------------------\n");
                host.diagnostics.debugLog("Callstack for ErrorCode=" + this.CPPWinRTException.toString(16) + "\n");
                host.diagnostics.debugLog("\n");
                var restrictedError = this.CPPWinRTException.RestrictedError;
                if (restrictedError != undefined)
                {
                    for (var f of restrictedError.Stack)
                    {
                        host.diagnostics.debugLog("    " + f + "\n");
                    }
                }

                if (dumpCount != undefined)
                {
                    dumpCount--;
                }
            }

            // Dump the non-stowed-exception crash error code and stack
            if (dumpCount == undefined || dumpCount > 0)
            {
                host.diagnostics.debugLog("-------------------------\n");
                host.diagnostics.debugLog("Callstack for ErrorCode=" + this.ErrorCode.toString(16) + " - " + this.ErrorCodeString + "\n");
                host.diagnostics.debugLog("\n");

                if (this.ErrorInfo != undefined)
                {
                    for (var f of this.ErrorInfo.Stack)
                    {
                        if (f.Print != undefined)
                        {
                            f.Print("    ");
                        }
                        else
                        {
                            host.diagnostics.debugLog("    " + f + "\n");
                        }
                    }
                }
            }
        }
        else if (dumpCount == undefined || dumpCount > 0)
        {
            for (var se of this.__stowedExceptions)
            {
                host.diagnostics.debugLog("-------------------------\n");
                host.diagnostics.debugLog("Callstack for hr=" + se.ResultCode + "\n");
                if (se.ErrorMessage != undefined)
                {
                    host.diagnostics.debugLog("  Error message: \"" + se.ErrorMessage + "\"\n");
                }
                host.diagnostics.debugLog("\n");
                for (var f of se.Stack)
                {
                    if (f.Print != undefined)
                    {
                        f.Print("    ");
                    }
                    else
                    {
                        host.diagnostics.debugLog("    " + f + "\n");
                    }
                }

                if (se.NestedExceptionType != undefined)
                {
                    host.diagnostics.debugLog("\n    Nested Exception (" + se.NestedExceptionType + "):\n");
                    if (se.ManagedException != undefined)
                    {
                        host.diagnostics.debugLog("       " + se.ManagedException + "\n");
                        if (se.ManagedException.TargetExceptionStack != undefined)
                        {
                            host.diagnostics.debugLog("\n");
                            for (var stack of se.ManagedException.TargetExceptionStack)
                            {
                                host.diagnostics.debugLog("           " + stack + "\n");
                            }
                        }
                    }
                    else if (se.XamlErrorContext != undefined)
                    {
                        // get the thread for the stowed exception crash and the dll for that thread
                        var thread = host.currentProcess.Threads.Where(function(t) { return t.Id == host.namespace.Debugger.State.PseudoRegisters.General.tid }).First();
                        var module = XAML_DLL_Module_from_thread(thread);
                        var modulename = module.Symbols.Name; // use the symbol name, which might be something like "Microsoft_UI_Xaml_7ffeda4a0000"
                        // Note: The "0x" is added to the address to ensure this command works even if the user has
                        //       the debugger in a mode where it doesn't auto-assume a hex value (via "n 10").
                        host.diagnostics.debugLog("       dx ((" + modulename + "!ErrorContext*)0x" + se.XamlErrorContext.targetLocation.address.toString(16) + ")\n");
                    }
                }

                if (dumpCount != undefined && --dumpCount == 0)
                {
                    break;
                }
            }
        }
        host.diagnostics.debugLog("\n");
        host.diagnostics.debugLog("=========================\n");
    }

    get [Symbol.metadataDescriptor]()
    {
        // !xamltriage only dumps the stacks on the first call so they don't get repeatedly
        // dumped when viewing the properties from its returned object. Provide an easy way
        // to dump the stacks again. Alternatively, the developer can use "!xamltriage 5" to
        // explicitly dump the given number of stacks.
        var printMetadata = { PreferShow: false, ActionName: "Dump Stacks", ActionDescription: "Dump the stowed exception stacks. Or use '!xamltriage 5' to dump that many stacks.",  ActionIsDefault: true };
        return { Print: printMetadata };
    }
}

var __printTriageDataIfNoParam = true;

// !xamltriage function
function xamltriage(dumpCount)
{
    // !xamltriage should show whatever information is available, even if XAML symbols
    // aren't loaded. Print a warning if there aren't XAML symbols, though.
    try
    {
        __CheckXAMLSymbolsLoaded();
    }
    catch(e)
    {
        host.diagnostics.debugLog("Warning: XAML symbols not loaded.\n");
    }

    var triageData = new TriageData();

    if ((dumpCount == undefined && __printTriageDataIfNoParam) || dumpCount > 0)
    {
        __printTriageDataIfNoParam = false; // don't auto-print a second time.
        triageData.Print(dumpCount);
    }

    return triageData;
}

// !xamlclrobj function
function xamlclrobj(addr)
{
    if (addr == undefined)
    {
        host.diagnostics.debugLog("Usage: !xamlclrobj <pointer to clr obj>\n");
        return null;
    }

    if (!__ClrObject.IsClrObject(addr))
    {
        host.diagnostics.debugLog("No object found at address " + addr.toString(16) + "\n");
        return null;
    }

    var clrObj = new __ClrObject(addr);
    return clrObj;
}

// Visualizer for CDependencyProperty
class __CDependencyPropertyVisualizer
{
    toString()
    {
        var module = XAML_DLL_Module_from_targetType(this.targetType);
        if (module.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll"))
        {
            // There isn't a default visualizer for DPs in WinUI3, so implement that here:
            if (!this.__IsCustomProperty())
            {
                return __GetNameForEnumValue(this.m_nIndex, "KnownPropertyIndex", module, "???");
            }
            return this.PropertyName;
        }
        return this.Property;
    }

    get Property()
    {
        // TODO: Only return a custom property if it is a custom property.
        //if (this.__IsCustomProperty())
        return host.createPointerObject(this.address, XAML_DLL_Module_from_targetType(this.targetType), "CCustomDependencyProperty*");
    }

    get PropertyName()
    {
        return this.Property.m_strName.toString();
    }

    __IsCustomProperty()
    {
        var MetaDataPropertyInfoFlags_IsCustomDependencyProperty = 0x1000;
        return (this.m_flags & MetaDataPropertyInfoFlags_IsCustomDependencyProperty) ? true : false;
    }
}

class __CustomClassInfoWrapper
{
    constructor(customClassInfo, classIndex)
    {
        this.__customClassInfo = customClassInfo;
        this.__classIndex = classIndex;
    }

    toString()
    {
        return this.__classIndex + ": " + this.__customClassInfo.m_strFullName;
    }

    get RawClassInfo()
    {
        return this.__customClassInfo;
    }

    get ClassIndex()
    {
        return this.__classIndex;
    }

    get FullName()
    {
        return this.__customClassInfo.m_strFullName;
    }
}

class __XamlClassList
{
    constructor(array, startIndex)
    {
        this.__array = array;
        this.__startIndex = startIndex;
    }
    
    get length()
    {
        return this.__array.length;
    }

    *[Symbol.iterator]()
    {
        var index = this.__startIndex;
        for (let item of this.__array)
        {
            //host.diagnostics.debugLog(index + ": " + item.FullName + "\n");
            yield new host.indexedValue(item, [index++]);
        }
    }

    getDimensionality()
    {
        return 1;
    }

    getValueAt(index)
    {
        //host.diagnostics.debugLog("getValueAt: " + index + "\n");
        return this.__array[index-this.__startIndex];
    }
}

// TODO:  Expose full XAML metadata, perhaps with !xamlmetadata or somewhere off the
//        XamlThread, rather than this separate !xamlclass helper.
// !xamlclass function
function xamlclass(index, useWinUI3)
{
    if (index == undefined)
    {
        // TODO: return the array in this case!
        host.diagnostics.debugLog("Usage: !xamlclass <class index>\n");
        return null;
    }
    var modules = host.currentProcess.Modules.Where(function(m)
        {
            return (m.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll") && !m.Contents.Version.VersionInfo.FileVersion.startsWith("2.")) ||
                    m.Name.toLowerCase().endsWith("windows.ui.xaml.dll")
        });
    var module = modules.First();
    if (modules.Count() == 2)
    {
        if (useWinUI3 == undefined)
        {
            host.diagnostics.debugLog("Both system XAML and WinUI3 are loaded. Specify 0 (system XAML) or 1 (useWinUI3)\n");
            host.diagnostics.debugLog("Usage: !xamlclass <class index>,<useWinUI3>\n");
            return null;
        }
        else
        {
            module = useWinUI3 ? GetWinUI3MUXModule() : GetSystemWUXModule();
        }
    }

    var knownTypesArray = host.getModuleSymbol(module, "c_aTypes");
    var knownTypeCount = knownTypesArray.Count();

    var storage = host.getModuleSymbol(module, "s_storage", "DirectUI::DynamicMetadataStorage*");
    var customTypesCache = storage.m_customTypesCache;

    if (index == -1)
    {
        index = 0;
        var classesArray = [];
        while (index < customTypesCache.Count())
        {
            var c = customTypesCache[index]._Mypair._Myval2;
            classesArray.push(new __CustomClassInfoWrapper(c, index+knownTypeCount));
            index++;
        }
        return new __XamlClassList(classesArray, knownTypeCount);
    }
    if (index < knownTypeCount)
    {
        // Get a reference to the first class type, then use its targetLocation to create
        // the array pointer we can .add(index) off.
        var firstTypeObj = host.getModuleSymbol(module, "c_aTypes", "MetaDataType");
        var knownTypesArray = host.createPointerObject(firstTypeObj.targetLocation.address, module, "MetaDataType*");
        return knownTypesArray.add(index);
    }

    return customTypesCache[index-knownTypeCount];
}

function __GetObjectForTriageDump(thread)
{
    // See if we can detect XAML on the stack
    for (var frame of thread.Stack.Frames)
    {
        if (frame.toString().indexOf("Windows_UI_Xaml!CJupiterControl::RunMessageLoop") >= 0 ||
            frame.toString().indexOf("Microsoft_UI_Xaml!CJupiterControl::RunMessageLoop") >= 0)
        {
            return "XAML thread (heap not available)";
        }
    }
    return undefined; // doesn't appear to be a XAML thread
}

var XamlThreadExtension =
{
    get Xaml()
    {
        // Only do something if the XAML dll is loaded
        if (!__IsXAMLLoaded())
        {
            return undefined;
        }

        // "this" is the debugger Thread object
        var thread = this;

        var tlsSlot;
        try
        {
            var module = XAML_DLL_Module_from_thread(thread, true/*silentOnNonXAML*/);
            if (module == undefined)
            {
                // Either not a XAML thread, or symbols aren't loaded.
                if (!__CheckXAMLSymbolsLoaded())
                {
                    return "Symbols for XAML not loaded/unavailable.";
                }
                // Not a XAML thread, so don't extend this Thread.
                return undefined;
            }

            tlsSlot = host.getModuleSymbol(module, "DXamlInstanceStorage::g_dwTlsIndex", "int");
        }
        catch (exp)
        {
            host.diagnostics.debugLog("failure AAA: " + exp + "\n");
            if (exp.toString().indexOf("Invalid argument to method 'getModuleSymbol'") > 0)
            {
                return "Symbols for XAML not loaded/unavailable.";
            }
            return __GetObjectForTriageDump(thread);
        }

        var ptr;
        if (tlsSlot < 64)
        {
            // TODO: check thread.NativeEnvironment if running WOW64
            ptr = thread.Environment.EnvironmentBlock.TlsSlots[tlsSlot];
        }
        else
        {
            ptr = thread.Environment.EnvironmentBlock.TlsExpansionSlots[tlsSlot - 64];
        }

        if (!ptr.isNull)
        {
            var dxamlCore = host.createPointerObject(ptr.address, XAML_DLL_Module_from_thread(thread), "DirectUI::DXamlCore*");
            return new XamlThread(this, dxamlCore);
        }
        else
        {
            // Not a XAML thread, so don't extend this Thread.
            return undefined;
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                 XamlTools section
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class __CleanStackCmd
{
    toString() { return ""; }

    Execute()
    {
        try
        {
            host.diagnostics.debugLog("------\n");

            // Running this twice to ensure the first run takes care of any missing symbol errors:
                         host.namespace.Debugger.Utility.Control.ExecuteCommand("k");
            var output = host.namespace.Debugger.Utility.Control.ExecuteCommand("k");
            var trimToIndex = -1;
            for (var line of output)
            {
                if (trimToIndex == -1)
                {
                    trimToIndex = line.indexOf("Call Site");
                    if (trimToIndex == -1 && line.startsWith("00"))
                    {
                        // Must be on x86, which seems to not output Call Site.
                        if (line[20] == ' ')
                        {
                            trimToIndex = 21;
                        }
                    }
                }

                if (trimToIndex != -1)
                {
                    line = line.substring(trimToIndex);
                    line = line.trim();
                    host.diagnostics.debugLog(line + "\n");
                }
            }

            host.diagnostics.debugLog("------\n");
        }
        catch(exp)
        {
            host.diagnostics.debugLog("exception: " + exp + "\n");
        }
    }

    get [Symbol.metadataDescriptor]()
    {
        var executeMetadata = { PreferShow: true, ActionName: "Dump clean callstack", ActionDescription: "CleanStack(): print current callstack with just function name and source info",  ActionIsDefault: true };
        return { Execute: executeMetadata };
    }
}

class __ThreadErrorCmd
{
    toString() { return ""; }

    Execute()
    {
        try
        {
            host.diagnostics.debugLog("------\n");

            var punkError = undefined;
            try
            {
                punkError = host.currentThread.COM.ComTls.punkError;
            }
            catch(exp)
            {
            }

            var usingWorkaround = false;
            if (punkError == undefined) // Somtimes ComTls is unavailable or hits an issue. Try a fallback.
            {
                host.diagnostics.debugLog("Error reading @$curthread.COM.ComTls.punkError. Trying @$curthread.Environment.EnvironmentBlock.ReservedForOle.\n");
                var reservedForOle = host.currentThread.Environment.EnvironmentBlock.ReservedForOle;
                var oleTlsData = host.createPointerObject(reservedForOle.address, "combase.dll", "SOleTlsData*");
                punkError = oleTlsData.punkError;
                usingWorkaround = true;
            }

            host.diagnostics.debugLog("Thread punkError = 0x" + punkError.address.toString(16) + "\n");
            if (punkError != null)
            {
                // Dump the object, letting the output (with DML) go directly to the debugger output.
                if (!usingWorkaround)
                {
                    host.namespace.Debugger.Utility.Control.ExecuteCommand("dx @$curthread.COM.ComTls.punkError", false);
                }
                else
                {
                    host.namespace.Debugger.Utility.Control.ExecuteCommand("dx (IUnknown*)0x" + punkError.address.toString(16), false);
                }

                punkError = punkError.dereference().runtimeTypedObject;
                if (punkError._cStackBackTrace != undefined)
                {
                    host.diagnostics.debugLog("\n");
                    host.diagnostics.debugLog("Callstack (0x" + punkError._cStackBackTrace.toString(16) + " frames):\n");
                    host.diagnostics.debugLog("\n");

                    var stackEntry = punkError._ppvStackBackTrace;
                    for (var i = 0; i < punkError._cStackBackTrace; i++)
                    {
                        host.namespace.Debugger.Utility.Control.ExecuteCommand(".printf \"    %ly\\n\", 0x" + stackEntry[i].address.toString(16), false);
                    }
                }
            }

            host.diagnostics.debugLog("------\n");
        }
        catch(exp)
        {
            host.diagnostics.debugLog("exception: " + exp + "\n");
        }
    }

    get [Symbol.metadataDescriptor]()
    {
        var executeMetadata = { PreferShow: true, ActionName: "Dump thread error", ActionDescription: "ThreadError(): dump the error (such as OriginateError) stored on the thread",  ActionIsDefault: true };
        return { Execute: executeMetadata };
    }
}

class __HelpTextCmd
{
    toString() { return ""; }

    Execute()
    {
        try
        {
            this.__EscapeAndPrintf("*** General Commands ***\n");
            this.__EscapeAndPrintf("\n");
            this.__PrintfCommand("!xamltriage", "Dumps the errors and stacks for a stowed exception crash.");
            this.__PrintfCommand("!xamlstowed", "Dumps the XAML stowed exceptions on the thread.");
            this.__PrintfCommand("!xamlthreads", "Finds all the XAML threads and returns the primary XAML data for those threads.");
            this.__PrintfCommand("dx @$curthread.UI.Xaml", "Return the primary XAML data for the current thread.");
            this.__EscapeAndPrintf("\n");
            this.__EscapeAndPrintf("*** Islands Commands ***\n");
            this.__EscapeAndPrintf("\n");
            this.__PrintfCommand("dx -r3 @$xamlthreads().Select(x=>x.Islands)", "View all of the Islands in the process.");
            this.__EscapeAndPrintf("\n");
            this.__EscapeAndPrintf("*** WarningContexts ***\n");
            this.__EscapeAndPrintf("\n");
            this.__PrintfCommand("dx @$curthread.UI.Xaml.ErrorInfo.WarningContexts.Select(x=>x->ExtraInfo)", "View the extraInfo strings for WarningContexts on the current thread.");
        }
        catch (e)
        {
            host.diagnostics.debugLog("Help Text Exception: " + e + "\n");
        }
    }

    __PrintfCommand(command, description)
    {
        var str = "  <link cmd=\"" + command + "\">" + command + "</link>   - " + description + "\n";
        this.__EscapeAndPrintf(str);
    }

    __EscapeAndPrintf(str)
    {
        str = str.replace(/\\/g, '\\\\');
        str = str.replace(/"/g, '\\"');
        str = str.replace(/\n/g, '\\n');
        var cmd = ".printf /D \"" + str + "\"";
        var res = host.namespace.Debugger.Utility.Control.ExecuteCommand(cmd, false);
    }

    get [Symbol.metadataDescriptor]()
    {
        var executeMetadata = { PreferShow: true, ActionName: "Show help", ActionDescription: "ShowHelp(): show help text for some useful debugger extension commands",  ActionIsDefault: true };
        return { Execute: executeMetadata };
    }
}

class __ModuleWrapper
{
    constructor(module)
    {
        this.__module = module;
    }

    toString()
    {
        var version = "";
        try { version = "" + this.__module.Contents.Version.VersionInfo.FileVersion + " - "; } catch (ignore) {}
        return version + this.__module.Name;
    }

    get Module()
    {
        return this.__module;
    }

    get Version()
    {
        return this.__module.Contents.Version.VersionInfo.FileVersion;
    }

    get DetailedVersion()
    {
        return this.__module.Contents.Version.Children.getValueAt("StringFileInfo").Children.First().Children.getValueAt("FileVersion").Text;
    }
}

function __GetXAMLVersions()
{
    var versions = [];
    var summaryStringParts = [];
    var muxVersionDetail = null;
    var modules = host.currentProcess.Modules;
    var wasdkPaths = [];
    for (var i = 0; i < modules.Count(); i++)
    {
        var m = modules[i];
        var name = m.Name.toLowerCase();
        if (name.endsWith("microsoft.ui.xaml.dll") ||
            name.endsWith("microsoft.ui.xaml.controls.dll") ||
            name.endsWith("windows.ui.xaml.dll") ||
            name.endsWith("microsoft.ui.input.dll"))
        {
            versions.push(new __ModuleWrapper(m));
        }
        try
        {
            if (name.endsWith("microsoft.ui.xaml.dll") && !m.Contents.Version.VersionInfo.FileVersion.startsWith("2."))
            {
                var versionText = (new __ModuleWrapper(m)).DetailedVersion;
                muxVersionDetail = "MUX version: " + versionText;
                var summaryStr = "MUX: " + versionText;
                summaryStr = summaryStr.replace(/ *\(.*$/, "");
                summaryStringParts.push(summaryStr);
            }
        }
        catch (ignoring) {}

        // Do some work to check for the path(s) of WinAppSDK versions loaded.
        var warIndex = name.indexOf("\\microsoft.windowsappruntime.");
        if (warIndex > 0)
        {
            // Switch back to "m.Name" here to get the non-lowercased version.
            var nextSlashIndex = m.Name.indexOf("\\", warIndex+1);
            var path = m.Name.substring(0, nextSlashIndex);
            if (!wasdkPaths.includes(path))
            {
                wasdkPaths.push(path);
            }
        }
    }

    // Show an error if more than one WinAppSDK version is loaded.
    if (wasdkPaths.length > 1)
    {
        var cmd = ".printf /D \"\\n<b>Error: Detected multiple WinAppSDK packages loaded:</b>\\n\"";
        host.namespace.Debugger.Utility.Control.ExecuteCommand(cmd, false);
        for (var path of wasdkPaths)
        {
            host.diagnostics.debugLog("    " + path + "\n");
        }
        host.diagnostics.debugLog("\n");
    }

    // If we have it, add the more detailed MUX version after adding the modules above.
    if (muxVersionDetail != null)
    {
        versions.push(muxVersionDetail);
    }

    // Look for WinAppSDK package version, if heap is available.
    try
    {
        var cache = host.getModuleSymbol("combase.dll", "s_pCachePerUserRuntimeClassInfo", "CCache*");
        for (var i = 0; i < cache.m_cBuckets; i++)
        {
            if (!cache.m_paBuckets[i].isNull &&
                !cache.m_paBuckets[i].pUnknown.isNull)
            {
                var classInfo = cache.m_paBuckets[i].pUnknown.dereference().runtimeTypedObject;
                if (classInfo._packageMoniker != undefined)
                {
                    var packageMoniker = classInfo._packageMoniker.toString();
                    if (packageMoniker.indexOf("WindowsAppRuntime") >= 0)
                    {
                        versions.push(packageMoniker);
                        var wasdkPackageVersion = packageMoniker.substring(packageMoniker.indexOf("WindowsAppRuntime"));
                        wasdkPackageVersion = wasdkPackageVersion.replace(/_x86_.*$/g, "");
                        wasdkPackageVersion = wasdkPackageVersion.replace(/_x64_.*$/g, "");
                        wasdkPackageVersion = wasdkPackageVersion.replace(/__.*$/g, ""); // cover arm, leaving arm in there
                        summaryStringParts.push(wasdkPackageVersion);
                        break;
                    }
                }
            }
        }
    }
    catch (ignoring) {}

    var list = new __List(versions, "Versions");
    if (summaryStringParts.length > 0)
    {
        // Change the toString() to return a nice summary.
        list.toString = function() { return summaryStringParts.join("; "); };
    }
    return list;
}

class XamlTools
{
    toString() { return ""; }

    get Clean_Stack()
    {
        return new __CleanStackCmd();
    }

    CleanStack()
    {
        (new __CleanStackCmd).Execute();
    }

    get Thread_Error()
    {
        return new __ThreadErrorCmd();
    }

    ThreadError()
    {
        (new __ThreadErrorCmd).Execute();
    }

    get Help()
    {
        return new __HelpTextCmd();
    }

    ShowHelp()
    {
        (new __HelpTextCmd).Execute();
    }

    get Versions()
    {
        return __GetXAMLVersions();
    }
}

function xamltools()
{
    return new XamlTools();
}

function invokeScript()
{
    //
    // This method will be called whenever the script is invoked from a client.
    //
    // See the following for more details:
    //
    //     https://aka.ms/JsDbgExt

    // Debugging helper:
    //host.diagnostics.logUnhandledExceptions = true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                 initializeScript (defines main functionality -- keep last!)
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

function initializeScript()
{
    return [new host.namespacePropertyParent(XamlThreadExtension, "Debugger.Models.Thread", "Debugger.Models.Thread.UI", "UI"),
            new host.typeSignatureRegistration(__ErrorContextVisualizer, "ErrorContext", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__WarningContextVisualizer, "WarningContext", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__xstringPtrVisualizer, "xstring_ptr", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__xencodedStringPtrVisualizer, "xencoded_string_ptr", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__SparsePropertyVisualizer, "std::pair<enum KnownPropertyIndex,EffectiveValue>", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__CDependencyPropertyVisualizer, "CDependencyProperty*", "microsoft.ui.xaml.dll"),
            new host.typeSignatureRegistration(__CDependencyPropertyVisualizer, "CCustomDependencyProperty*", "microsoft.ui.xaml.dll"),
            new host.functionAlias(listXamlThreads, "xamlthreads"),
            new host.functionAlias(xamltriage, "xamltriage"),
            new host.functionAlias(dumpXamlStowed, "xamlstowed"),
            new host.functionAlias(dumpXamlElement, "xamlelement"),
            new host.functionAlias(xamlclrobj, "xamlclrobj"),
            new host.functionAlias(xamlclass, "xamlclass"),
            new host.functionAlias(xamltools, "xamltools")];
}
