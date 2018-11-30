$(OBJ_PATH)\$O\CppWinRTCodeGen:   makefile.inc sources
    <<$@.cmd
    echo ON
    setlocal ENABLEDELAYEDEXPANSION

    echo PROJECT_INTERNAL_SDK_METADATA_PATH=$(PROJECT_INTERNAL_SDK_METADATA_PATH)
    echo PROJECT_SDK_METADATA_PATH=$(PROJECT_SDK_METADATA_PATH)
    echo PROJECT_INTERIM_METADATA_PATH=$(PROJECT_INTERIM_METADATA_PATH)
    
    call cppwinrt.cmd -in $(PROJECT_INTERNAL_SDK_METADATA_PATH) -in $(PROJECT_SDK_METADATA_PATH)\windows.winmd -out $(OBJ_PATH)\$O\Platform

    call cppwinrt.cmd -component $(OBJ_PATH)\$O\Reference -overwrite -ref $(PROJECT_INTERNAL_SDK_METADATA_PATH) ^
        -in $(PROJECT_SDK_METADATA_PATH)\windows.winmd ^
        -in $(PROJECT_SDK_METADATA_PATH)\internal\windows.ui.winmd ^
        -in $(OBJ_PATH)\..\idl\$O\effects.winmd ^
        -out $(OBJ_PATH)\$O\Component ^
        @$(SDXROOT)\onecoreuap\windows\dxaml\controls\dev\dll\CppWinRTFilterTypes.txt ^
        -name Windows.UI.Xaml
    
    echo Done. >> $@
<<Keep
