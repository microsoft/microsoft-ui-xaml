#include <windows.h>
#include <commctrl.h>
#include <string>
#include <windowsx.h>
#include <shobjidl.h>   // For IFileDialog


#define IDC_TABCONTROL 101
#define IDC_TREEVIEW 102
#define IDC_EDITCONTROL 103
#define IDC_STATUSBAR 104
#define IDC_OPENFILEBTN 105
#define ID_CONTEXT_CLOSE_TAB 201
#define ID_CONTEXT_OPEN_TAB    2001
#define ID_CONTEXT_RENAME_TAB  2002

HINSTANCE hInst;
HWND hTabCtrl, hTreeView, hEditCtrl, hStatusBar, hOpenFileBtn;

HWND hLoadTimeLabel;
HWND hLoadTimeListBox;


// Function declarations
void CreateChildControls(HWND hWnd);
void AddTab(HWND hTabCtrl, const wchar_t* tabName);
void AddFileToTreeView(HWND hTreeView, const wchar_t* fileName);

void CreateChildControls(HWND hWnd)
{

    // Create the TabControl with TCS_BUTTONS style for better spacing control
    hTabCtrl = CreateWindowW(WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | TCS_TABS | TCS_BUTTONS,
        0, 0, 600, 50,
        hWnd, (HMENU)IDC_TABCONTROL, hInst, nullptr);

    TabCtrl_SetItemSize(hTabCtrl, 120, 40);  // Set custom size

    // Create the "Open File" button
    hOpenFileBtn = CreateWindowW(L"BUTTON", L"Open Folder",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        0, 55, 100, 30,
        hWnd, (HMENU)IDC_OPENFILEBTN, hInst, NULL);

    // Create the TreeView
    hTreeView = CreateWindowW(WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES,
        0, 90, 200, 600,
        hWnd, (HMENU)IDC_TREEVIEW, hInst, nullptr);

    // Create the Edit Control (Text Editor)
    hEditCtrl = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        200, 90, 800, 560,
        hWnd, (HMENU)IDC_EDITCONTROL, hInst, nullptr);

    // Create the StatusBar
    hStatusBar = CreateWindowW(STATUSCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hWnd, (HMENU)IDC_STATUSBAR, hInst, NULL);


    hLoadTimeLabel = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        850, 10, 160, 20, // Adjust position and size as needed
        hWnd, NULL, hInst, NULL);


    hLoadTimeListBox = CreateWindowW(WC_LISTBOXW, NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        800, 10, 200, 50, // Adjust position and size as needed
        hWnd, NULL, hInst, NULL);


}

void AddTab(HWND hTabCtrl, const wchar_t* tabName)
{
    LARGE_INTEGER frequency, startTime, endTime;
    QueryPerformanceFrequency(&frequency); // Get the high-res timer frequency
    QueryPerformanceCounter(&startTime);   // Start the stopwatch
    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)tabName;
    TabCtrl_InsertItem(hTabCtrl, 0, &tie);
    QueryPerformanceCounter(&endTime);
    double elapsedTimeMs = (double)(endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

    wchar_t buffer[100];
    swprintf(buffer, 100, L"Tab Load Time: %.2f ms", elapsedTimeMs);

    SendMessageW(hLoadTimeListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
}

void AddFileToTreeView(HWND hTreeView, const wchar_t* fileName)
{
    
    TVINSERTSTRUCT tvInsert = { 0 };
    tvInsert.hParent = TVI_ROOT;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_TEXT | TVIF_PARAM;

    // Display only the file name, not the full path
    const wchar_t* displayName = wcsrchr(fileName, L'\\');
    if (displayName) displayName++; else displayName = fileName;

    tvInsert.item.pszText = (LPWSTR)displayName;
    tvInsert.item.lParam = (LPARAM)_wcsdup(fileName); // Store full path

    TreeView_InsertItem(hTreeView, &tvInsert);
}

void PopulateTreeViewWithFolder(HWND hTreeView, const wchar_t* folderPath)
{
    LARGE_INTEGER frequency, startTime, endTime;
    QueryPerformanceFrequency(&frequency); // Get the high-res timer frequency
    QueryPerformanceCounter(&startTime);   // Start the stopwatch
    WIN32_FIND_DATAW findFileData;
    wchar_t searchPath[MAX_PATH];
    swprintf_s(searchPath, MAX_PATH, L"%s\\*.txt", folderPath); // Limit to .txt files

    HANDLE hFind = FindFirstFileW(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        wchar_t fullPath[MAX_PATH];
        swprintf_s(fullPath, MAX_PATH, L"%s\\%s", folderPath, findFileData.cFileName);

        AddFileToTreeView(hTreeView, fullPath);  // ✅ Reuse your existing function
        

    } while (FindNextFileW(hFind, &findFileData));
    QueryPerformanceCounter(&endTime);
    double elapsedTimeMs = (double)(endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
    wchar_t buffer[100];
    swprintf(buffer, 100, L"TreeView Load Time: %.2f ms", elapsedTimeMs);
    SendMessageW(hLoadTimeListBox, LB_ADDSTRING, 0, (LPARAM)buffer);

    FindClose(hFind);  // ✅ Ensure we always close the handle
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateChildControls(hWnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_OPENFILEBTN:
        {
            IFileDialog* pFileDialog = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&pFileDialog));

            if (SUCCEEDED(hr)) {
                DWORD dwOptions;
                pFileDialog->GetOptions(&dwOptions);
                pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

                hr = pFileDialog->Show(hWnd);
                if (SUCCEEDED(hr)) {
                    IShellItem* pItem = nullptr;
                    hr = pFileDialog->GetResult(&pItem);
                    if (SUCCEEDED(hr)) {
                        PWSTR pszFolderPath = nullptr;
                        pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath);

                        // Display selected folder path in edit control
                        SetWindowTextW(hEditCtrl, pszFolderPath);

                        // Clear existing TreeView (if needed)
                        TreeView_DeleteAllItems(hTreeView);

                        // Populate TreeView with files from folder
                        PopulateTreeViewWithFolder(hTreeView, pszFolderPath);
                        const wchar_t* folderName = wcsrchr(pszFolderPath, L'\\');
                        if (folderName)
                            AddTab(hTabCtrl, folderName + 1);  // skip backslash
                        else
                            AddTab(hTabCtrl, pszFolderPath);   // fallback

                        CoTaskMemFree(pszFolderPath);
                        pItem->Release();
                    }
                }
                pFileDialog->Release();
            }
            break;
        }



        case ID_CONTEXT_CLOSE_TAB: // Close tab from context menu
        {
            int iSel = TabCtrl_GetCurSel(hTabCtrl);
            if (iSel != -1)
            {
                TabCtrl_DeleteItem(hTabCtrl, iSel);
            }
        }
        break;

        }
        break;

    case WM_CONTEXTMENU:
    {
        HWND hCtrl = (HWND)wParam;
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (hCtrl == hTabCtrl)
        {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_OPEN_TAB, L"Open");       // New
            AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_RENAME_TAB, L"Rename");     // New
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);                           // Optional separator
            AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_CLOSE_TAB, L"Close Tab");

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, xPos, yPos, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;
    }

    break;

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        int statusHeight = rect.bottom - 30;
        MoveWindow(hStatusBar, 0, statusHeight, rect.right, 30, TRUE);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == IDC_TREEVIEW && pnmh->code == TVN_SELCHANGED)
        {
            LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
            HTREEITEM hItem = pnmtv->itemNew.hItem;

            if (hItem)
            {
                TVITEM tvi = { 0 };
                tvi.mask = TVIF_PARAM;
                tvi.hItem = hItem;

                if (TreeView_GetItem(pnmh->hwndFrom, &tvi))
                {
                    const wchar_t* filePath = (const wchar_t*)tvi.lParam;
                    if (filePath)
                    {
                        // Load file content and show in hEditCtrl
                        HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            DWORD dwSize = GetFileSize(hFile, NULL);
                            if (dwSize != INVALID_FILE_SIZE && dwSize > 0)
                            {
                                char* buffer = (char*)GlobalAlloc(GPTR, dwSize + 1);
                                DWORD dwRead;
                                if (ReadFile(hFile, buffer, dwSize, &dwRead, NULL))
                                {
                                    buffer[dwRead] = '\0';

                                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
                                    if (wideLen > 0)
                                    {
                                        wchar_t* wideBuffer = (wchar_t*)GlobalAlloc(GPTR, wideLen * sizeof(wchar_t));
                                        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wideBuffer, wideLen);
                                        SetWindowTextW(hEditCtrl, wideBuffer);
                                        GlobalFree(wideBuffer);
                                    }
                                }
                                GlobalFree(buffer);
                            }
                            CloseHandle(hFile);
                        }
                    }
                }
            }
        }
        break;
    }


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    LARGE_INTEGER frequency, startTime, endTime;
    QueryPerformanceFrequency(&frequency); // Get the high-res timer frequency
    QueryPerformanceCounter(&startTime);   // Start the stopwatch

    // Register the window class
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"CommCtrlApp";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(NULL, L"Window class registration failed!", L"Error", MB_OK);
        return 0;
    }

    // Create the main window
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"CommCtrl App",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInst, NULL);

    if (hWnd == NULL)
    {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_OK);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    QueryPerformanceCounter(&endTime);

    double elapsedTimeMs = (double)(endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

    wchar_t buffer[100];
    swprintf(buffer, 100, L"Initialization Time: %.2f ms", elapsedTimeMs);
    SendMessageW(hLoadTimeListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
