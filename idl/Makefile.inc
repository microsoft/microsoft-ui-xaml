##############################################################################################################
# mdmerge windows.winmd+windows.ui.winmd+dependencies --> TestMergedWindows.winmd
# We do this because TreeView is in Windows.Ui.Xaml namespace but in windows.ui.winmd currently. This
# causes trouble with C# apps that want to consume it. They expect it to be in windows.winmd
##############################################################################################################

$(OBJ_PATH)\$O\TestMergedWindowsMetadata\windows.winmd: $(PROJECT_SDK_METADATA_PATH)\windows.winmd $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.ui.winmd $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.phone.winmd $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.graphics.winmd $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.devices.winmd $(PROJECT_INTERIM_METADATA_PATH)\windowmanagement.winmd $(PUBLIC_ROOT)\internal\onecore\internal\interimmetadata\windows.foundation.handles.internal.winmd
    -$(MKDIR) $(OBJ_PATH)\$O\TestWinmdsToMerge
    -$(MKDIR) $(OBJ_PATH)\$O\TestMergedWindowsMetadata
    $(COPY) /y $(PROJECT_SDK_METADATA_PATH)\windows.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.ui.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.phone.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.graphics.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.perception.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERNAL_SDK_METADATA_PATH)\windows.devices.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PROJECT_INTERIM_METADATA_PATH)\windowmanagement.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(COPY) /y $(PUBLIC_ROOT)\internal\onecore\internal\interimmetadata\windows.foundation.handles.internal.winmd $(OBJ_PATH)\$O\TestWinmdsToMerge
    $(MDMERGE) -i $(OBJ_PATH)\$O\TestWinmdsToMerge -o $(OBJ_PATH)\$O\TestMergedWindowsMetadata\ -n:1
