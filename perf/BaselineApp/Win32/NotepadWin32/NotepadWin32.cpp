// NotepadWin32.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "NotepadWin32.h"
#include <commdlg.h> // Add this include at the top of the file
#include <commctrl.h>
#include <shlobj.h> // For SHGetFolderPath
#include <tchar.h>  // For _tcslen

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hEdit;
HWND hStatus;
HWND hTreeView;
HFONT hFont;
WCHAR szFileName[MAX_PATH] = L"";
void LoadTextFileToEdit(HWND hEdit, LPCWSTR pszFileName);
void SaveTextFileFromEdit(HWND hEdit, LPCWSTR pszFileName);
void PopulateTreeView(HWND hTreeView, LPCWSTR pszPath, HTREEITEM hParent);
void GetUserFolderPath(LPWSTR path, int pathSize);


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOTEPADWIN32, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOTEPADWIN32));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOTEPADWIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NOTEPADWIN32);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   /*HWND*/ hEdit = CreateWindowEx(0, L"EDIT", NULL,
       WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
       0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
       hWnd, (HMENU)1, hInstance, NULL);

   hFont = CreateFont(
       20,                        // Height
       0,                         // Width
       0,                         // Escapement
       0,                         // Orientation
       FW_NORMAL,                 // Weight
       FALSE,                     // Italic
       FALSE,                     // Underline
       FALSE,                     // StrikeOut
       DEFAULT_CHARSET,           // CharSet
       OUT_DEFAULT_PRECIS,        // OutPrecision
       CLIP_DEFAULT_PRECIS,       // ClipPrecision
       DEFAULT_QUALITY,           // Quality
       DEFAULT_PITCH | FF_SWISS,  // PitchAndFamily
       L"Arial");                 // Facename

   // Set the font for the Edit control
   SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

   hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
       0, 0, 0, 0,
       hWnd, (HMENU)1, hInstance, NULL);

   hTreeView = CreateWindowEx(0, WC_TREEVIEW, NULL,
       WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
       0, 0, 200, CW_USEDEFAULT,
       hWnd, (HMENU)2, hInstance, NULL);


   WCHAR userFolderPath[MAX_PATH];
   GetUserFolderPath(userFolderPath, MAX_PATH);
   PopulateTreeView(hTreeView, userFolderPath, TVI_ROOT);


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void GetUserFolderPath(LPWSTR path, int pathSize)
{
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, path);
}


void PopulateTreeView(HWND hTreeView, LPCWSTR pszPath, HTREEITEM hParent)
{
    WIN32_FIND_DATA findFileData;
    WCHAR searchPath[MAX_PATH];
    wsprintf(searchPath, L"%s\\*", pszPath);

    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;

    do
    {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0)
        {
            tvis.item.pszText = findFileData.cFileName;
            HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvis);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                WCHAR subDir[MAX_PATH];
                wsprintf(subDir, L"%s\\%s", pszPath, findFileData.cFileName);
                PopulateTreeView(hTreeView, subDir, hItem);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}


void OnTreeViewItemSelect(HWND hTreeView, HWND hEdit)
{
    HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);
    if (hSelectedItem)
    {
        TVITEM tvi;
        tvi.mask = TVIF_TEXT;
        tvi.hItem = hSelectedItem;
        tvi.pszText = new WCHAR[MAX_PATH];
        tvi.cchTextMax = MAX_PATH;

        if (TreeView_GetItem(hTreeView, &tvi))
        {
            LoadTextFileToEdit(hEdit, tvi.pszText);
        }

        delete[] tvi.pszText;
    }
}

void UpdateStatusBar(HWND hEdit, HWND hStatus)
{
    int lineCount = SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);

    int textLength = GetWindowTextLength(hEdit);
    LPWSTR text = (LPWSTR)GlobalAlloc(GPTR, (textLength + 1) * sizeof(WCHAR));
    GetWindowText(hEdit, text, textLength + 1);

    int wordCount = 0;
    bool inWord = false;
    for (int i = 0; i < textLength; ++i)
    {
        if (iswspace(text[i]))
        {
            if (inWord)
            {
                inWord = false;
                ++wordCount;
            }
        }
        else
        {
            inWord = true;
        }
    }
    if (inWord)
    {
        ++wordCount;
    }

    GlobalFree(text);

    WCHAR statusText[256];
    wsprintf(statusText, L"Lines: %d Words: %d", lineCount, wordCount);
    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusText);
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        SetWindowPos(hTreeView, NULL, 0, 0, 200, rcClient.bottom - 20, SWP_NOZORDER);
        SetWindowPos(hEdit, NULL, 200, 0, rcClient.right - 200, rcClient.bottom - 20, SWP_NOZORDER);
        SendMessage(hStatus, WM_SIZE, 0, 0);
        UpdateStatusBar(hEdit, hStatus);
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDM_OPEN:
			{
				OPENFILENAME ofn;       // common dialog box structure
				WCHAR szFile[260];       // buffer for file name
				// Initialize OPENFILENAME
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = szFile;
				ofn.lpstrFile[0] = '\0';
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
				ofn.lpstrTitle = L"Open Text File";
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				// Display the Open dialog box 
				if (GetOpenFileName(&ofn) == TRUE)
				{
					LoadTextFileToEdit(hEdit, ofn.lpstrFile);
                    UpdateStatusBar(hEdit, hStatus);
				}
			}
			break;
            case IDM_SAVEAS:
            {
                OPENFILENAME ofn;       // common dialog box structure
                WCHAR szFile[260];       // buffer for file name
                // Initialize OPENFILENAME
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.lpstrFile[0] = '\0';
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
                ofn.lpstrTitle = L"Save Text File As";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                // Display the Save As dialog box 
                if (GetSaveFileName(&ofn) == TRUE)
                {
                    SaveTextFileFromEdit(hEdit, ofn.lpstrFile);
                    wcscpy_s(szFileName, ofn.lpstrFile); // Store the file name
                }
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case IDM_SAVE:
        if (wcslen(szFileName) > 0)
        {
            SaveTextFileFromEdit(hEdit, szFileName);
        }
        else
        {
            SendMessage(hWnd, WM_COMMAND, IDM_SAVEAS, 0);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    case WM_NOTIFY:
    {
        LPNMHDR lpnmhdr = (LPNMHDR)lParam;
        if (lpnmhdr->idFrom == 2 && lpnmhdr->code == TVN_SELCHANGED)
        {
            OnTreeViewItemSelect(hTreeView, hEdit);
        }
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
void LoadTextFileToEdit(HWND /*hEdit*/, LPCWSTR pszFileName)
{
    HANDLE hFile = CreateFileW(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize != INVALID_FILE_SIZE)
        {
            char* pszFileText = (char*)GlobalAlloc(GPTR, (dwFileSize + 1) * sizeof(WCHAR));
            if (pszFileText)
            {
                DWORD dwRead;
                if (ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL))
                {
                    int cchWideChar = MultiByteToWideChar(CP_UTF8, 0, pszFileText, -1, NULL, 0);
                    if (cchWideChar > 0)
                    {
                        LPWSTR pszWideText = (LPWSTR)GlobalAlloc(GPTR, cchWideChar * sizeof(WCHAR));
                        if (pszWideText)
                        {
                            MultiByteToWideChar(CP_UTF8, 0, pszFileText, -1, pszWideText, cchWideChar);
                            SetWindowText(hEdit, pszWideText);
                            GlobalFree(pszWideText);
                        }
                    }
                }
                GlobalFree(pszFileText);
            }
        }
        CloseHandle(hFile);
    }
}

void SaveTextFileFromEdit(HWND hEdit, LPCWSTR pszFileName)
{
    HANDLE hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwTextLength = GetWindowTextLength(hEdit);
        if (dwTextLength > 0)
        {
            LPWSTR pszText = (LPWSTR)GlobalAlloc(GPTR, (dwTextLength + 1) * sizeof(WCHAR));
            if (pszText)
            {
                if (GetWindowText(hEdit, pszText, dwTextLength + 1))
                {
                    DWORD dwWritten;
                    WriteFile(hFile, pszText, dwTextLength * sizeof(WCHAR), &dwWritten, NULL);
                }
                GlobalFree(pszText);
            }
        }
        CloseHandle(hFile);
    }
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
