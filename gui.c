#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>

#define IDC_EDITOR 101
#define IDC_OUTPUT 102
#define IDC_RUN 103
#define IDC_SAVE 104
#define IDC_RELOAD 105
#define IDC_CLEAR 106
#define IDC_UPLOAD 107

static const char *PROGRAM_FILE = "program.txt";
static const char *OUTPUT_FILE = "gui_output.txt";
static const char *SIMULATOR_EXE = ".\\simulator.exe";

static HWND editorBox;
static HWND outputBox;
static HWND runButton;
static HWND uploadButton;
static HWND saveButton;
static HWND reloadButton;
static HWND clearButton;
static HFONT fixedFont;

static void showMessage(HWND hwnd, const char *text)
{
    MessageBoxA(hwnd, text, "Simulator GUI", MB_OK | MB_ICONINFORMATION);
}

static char *readEntireFile(const char *path, DWORD *sizeOut)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    DWORD size = GetFileSize(file, NULL);
    char *buffer = (char *)GlobalAlloc(GPTR, size + 1);
    if (!buffer) {
        CloseHandle(file);
        return NULL;
    }

    DWORD bytesRead = 0;
    ReadFile(file, buffer, size, &bytesRead, NULL);
    buffer[bytesRead] = '\0';
    CloseHandle(file);

    if (sizeOut)
        *sizeOut = bytesRead;

    return buffer;
}

static int writeEntireFile(const char *path, const char *text, DWORD length)
{
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return 0;

    DWORD bytesWritten = 0;
    int ok = WriteFile(file, text, length, &bytesWritten, NULL) &&
             bytesWritten == length;
    CloseHandle(file);
    return ok;
}

static void loadProgramIntoEditor(void)
{
    DWORD size = 0;
    char *text = readEntireFile(PROGRAM_FILE, &size);
    if (!text) {
        SetWindowTextA(editorBox, "");
        return;
    }

    SetWindowTextA(editorBox, text);
    GlobalFree(text);
}

static void uploadTextFile(HWND hwnd)
{
    char fileName[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = "Upload Program Text File";

    if (!GetOpenFileNameA(&ofn))
        return;

    DWORD size = 0;
    char *text = readEntireFile(fileName, &size);
    if (!text) {
        showMessage(hwnd, "Could not read the selected text file.");
        return;
    }

    SetWindowTextA(editorBox, text);
    SetWindowTextA(outputBox, "Uploaded text file. Press Run to simulate it.");
    GlobalFree(text);
}

static int saveEditorToProgram(HWND hwnd)
{
    int length = GetWindowTextLengthA(editorBox);
    char *text = (char *)GlobalAlloc(GPTR, (SIZE_T)length + 1);
    if (!text) {
        showMessage(hwnd, "Could not allocate memory for the editor text.");
        return 0;
    }

    GetWindowTextA(editorBox, text, length + 1);
    if (!writeEntireFile(PROGRAM_FILE, text, (DWORD)length)) {
        GlobalFree(text);
        showMessage(hwnd, "Could not save program.txt.");
        return 0;
    }

    GlobalFree(text);
    return 1;
}

static void loadOutputPane(void)
{
    DWORD size = 0;
    char *text = readEntireFile(OUTPUT_FILE, &size);
    if (!text) {
        SetWindowTextA(outputBox, "No output yet.");
        return;
    }

    SetWindowTextA(outputBox, text);
    GlobalFree(text);
}

static void runSimulator(HWND hwnd)
{
    if (!saveEditorToProgram(hwnd))
        return;

    HANDLE outFile = CreateFileA(OUTPUT_FILE, GENERIC_WRITE, FILE_SHARE_READ,
                                 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (outFile == INVALID_HANDLE_VALUE) {
        showMessage(hwnd, "Could not create gui_output.txt.");
        return;
    }

    SetHandleInformation(outFile, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = outFile;
    si.hStdError = outFile;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    char commandLine[] = ".\\simulator.exe";
    BOOL created = CreateProcessA(SIMULATOR_EXE, commandLine, NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(outFile);

    if (!created) {
        char message[256];
        snprintf(message, sizeof(message),
                 "Could not run simulator.exe.\r\n\r\nWindows error code: %lu\r\n\r\nMake sure simulator.exe is in the same folder as simulator_gui.exe.",
                 GetLastError());
        SetWindowTextA(outputBox, message);
        return;
    }

    SetWindowTextA(outputBox, "Running...");
    EnableWindow(runButton, FALSE);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    EnableWindow(runButton, TRUE);

    loadOutputPane();
    if (exitCode != 0)
        showMessage(hwnd, "The simulator finished with an error. Check the output pane.");
}

static HWND makeButton(HWND hwnd, const char *text, int id)
{
    return CreateWindowA("BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         0, 0, 90, 28, hwnd, (HMENU)(INT_PTR)id,
                         GetModuleHandle(NULL), NULL);
}

static HWND makeEdit(HWND hwnd, int id, DWORD extraStyle)
{
    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_AUTOHSCROLL | extraStyle,
        0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);

    SendMessageA(edit, WM_SETFONT, (WPARAM)fixedFont, TRUE);
    return edit;
}

static void layout(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 12;
    int toolbarHeight = 42;
    int labelHeight = 22;
    int gap = 10;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int paneTop = padding + toolbarHeight + labelHeight;
    int paneHeight = height - paneTop - padding;
    int paneWidth = (width - padding * 2 - gap) / 2;
    int rightX = padding + paneWidth + gap;

    MoveWindow(runButton, padding, padding, 90, 28, TRUE);
    MoveWindow(uploadButton, padding + 98, padding, 130, 28, TRUE);
    MoveWindow(saveButton, padding + 236, padding, 90, 28, TRUE);
    MoveWindow(reloadButton, padding + 334, padding, 90, 28, TRUE);
    MoveWindow(clearButton, rightX, padding, 110, 28, TRUE);

    MoveWindow(editorBox, padding, paneTop, paneWidth, paneHeight, TRUE);
    MoveWindow(outputBox, rightX, paneTop, paneWidth, paneHeight, TRUE);
}

static void paintLabels(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 12;
    int gap = 10;
    int width = rc.right - rc.left;
    int paneWidth = (width - padding * 2 - gap) / 2;
    int rightX = padding + paneWidth + gap;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    TextOutA(hdc, padding, 48, "Program Editor", 14);
    TextOutA(hdc, rightX, 48, "Console Output", 14);

    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        fixedFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                FIXED_PITCH | FF_MODERN, "Consolas");

        runButton = makeButton(hwnd, "Run", IDC_RUN);
        uploadButton = makeButton(hwnd, "Upload Text File", IDC_UPLOAD);
        saveButton = makeButton(hwnd, "Save", IDC_SAVE);
        reloadButton = makeButton(hwnd, "Reload", IDC_RELOAD);
        clearButton = makeButton(hwnd, "Clear Output", IDC_CLEAR);
        editorBox = makeEdit(hwnd, IDC_EDITOR, 0);
        outputBox = makeEdit(hwnd, IDC_OUTPUT, ES_READONLY);

        loadProgramIntoEditor();
        SetWindowTextA(outputBox, "Press Run to simulate the program.");
        return 0;

    case WM_SIZE:
        layout(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_PAINT:
        paintLabels(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_RUN:
            runSimulator(hwnd);
            return 0;
        case IDC_UPLOAD:
            uploadTextFile(hwnd);
            return 0;
        case IDC_SAVE:
            if (saveEditorToProgram(hwnd))
                showMessage(hwnd, "Saved program.txt.");
            return 0;
        case IDC_RELOAD:
            loadProgramIntoEditor();
            return 0;
        case IDC_CLEAR:
            SetWindowTextA(outputBox, "");
            return 0;
        }
        break;

    case WM_DESTROY:
        if (fixedFont)
            DeleteObject(fixedFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance,
                   LPSTR commandLine, int showCommand)
{
    (void)prevInstance;
    (void)commandLine;

    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, modulePath, sizeof(modulePath))) {
        char *lastSlash = strrchr(modulePath, '\\');
        if (lastSlash) {
            *lastSlash = '\0';
            SetCurrentDirectoryA(modulePath);
        }
    }

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PipelineSimulatorGui";

    if (!RegisterClassA(&wc))
        return 1;

    HWND hwnd = CreateWindowA(
        wc.lpszClassName, "C Pipeline Simulator",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
        NULL, NULL, instance, NULL);

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
