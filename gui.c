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
#define IDC_PIPELINE_OUTPUT 108
#define IDC_REGISTERS_OUTPUT 109
#define IDC_INSTRUCTION_OUTPUT 110
#define IDC_DATA_OUTPUT 111

static const char *PROGRAM_FILE = "program.txt";
static const char *OUTPUT_FILE = "gui_output.txt";
static const char *SIMULATOR_EXE = ".\\simulator.exe";

static HWND editorBox;
static HWND pipelineOutputBox;
static HWND registersOutputBox;
static HWND instructionOutputBox;
static HWND dataOutputBox;
static HWND runButton;
static HWND uploadButton;
static HWND saveButton;
static HWND reloadButton;
static HWND clearButton;
static HFONT fixedFont;
static HFONT uiFont;
static HFONT labelFont;
static HBRUSH backgroundBrush;
static HBRUSH editBrush;

static const COLORREF BACKGROUND_COLOR = RGB(244, 247, 251);
static const COLORREF BORDER_COLOR = RGB(207, 216, 228);
static const COLORREF LABEL_COLOR = RGB(37, 50, 69);
static const COLORREF EDIT_COLOR = RGB(252, 253, 255);
static const COLORREF TEXT_COLOR = RGB(24, 31, 42);
static const COLORREF EDITOR_PANEL_COLOR = RGB(232, 244, 255);
static const COLORREF PIPELINE_PANEL_COLOR = RGB(237, 247, 239);
static const COLORREF REGISTERS_PANEL_COLOR = RGB(255, 246, 226);
static const COLORREF INSTRUCTION_PANEL_COLOR = RGB(242, 237, 255);
static const COLORREF DATA_PANEL_COLOR = RGB(255, 236, 235);
static const COLORREF EDITOR_ACCENT_COLOR = RGB(43, 119, 191);
static const COLORREF PIPELINE_ACCENT_COLOR = RGB(34, 139, 84);
static const COLORREF REGISTERS_ACCENT_COLOR = RGB(190, 122, 18);
static const COLORREF INSTRUCTION_ACCENT_COLOR = RGB(103, 76, 194);
static const COLORREF DATA_ACCENT_COLOR = RGB(198, 67, 56);

static void showMessage(HWND hwnd, const char *text)
{
    MessageBoxA(hwnd, text, "Simulator GUI", MB_OK | MB_ICONINFORMATION);
}

static void setOutputFields(const char *pipeline, const char *registers,
                            const char *instruction, const char *data)
{
    SetWindowTextA(pipelineOutputBox, pipeline ? pipeline : "");
    SetWindowTextA(registersOutputBox, registers ? registers : "");
    SetWindowTextA(instructionOutputBox, instruction ? instruction : "");
    SetWindowTextA(dataOutputBox, data ? data : "");
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
    setOutputFields("Uploaded text file. Press Run to simulate it.", "", "", "");
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

static char *copyRange(const char *start, const char *end)
{
    size_t length = (size_t)(end - start);
    char *copy = (char *)GlobalAlloc(GPTR, length + 1);
    if (!copy)
        return NULL;

    memcpy(copy, start, length);
    copy[length] = '\0';
    return copy;
}

static void loadOutputFields(void)
{
    DWORD size = 0;
    char *text = readEntireFile(OUTPUT_FILE, &size);
    if (!text) {
        setOutputFields("No output yet.", "", "", "");
        return;
    }

    const char *registersHeader = strstr(text, "===== Registers =====");
    const char *instructionHeader = strstr(text, "===== Instruction Memory =====");
    const char *dataHeader = strstr(text, "===== Data Memory =====");

    const char *pipelineStart = text;
    const char *pipelineEnd = registersHeader ? registersHeader : text + size;
    const char *registersEnd = instructionHeader ? instructionHeader : text + size;
    const char *instructionEnd = dataHeader ? dataHeader : text + size;

    char *pipeline = copyRange(pipelineStart, pipelineEnd);
    char *registers = registersHeader ? copyRange(registersHeader, registersEnd) : NULL;
    char *instruction = instructionHeader ? copyRange(instructionHeader, instructionEnd) : NULL;
    char *data = dataHeader ? copyRange(dataHeader, text + size) : NULL;

    if (!pipeline) {
        setOutputFields("Could not allocate memory for output.", "", "", "");
    } else {
        setOutputFields(pipeline, registers, instruction, data);
    }

    if (pipeline)
        GlobalFree(pipeline);
    if (registers)
        GlobalFree(registers);
    if (instruction)
        GlobalFree(instruction);
    if (data)
        GlobalFree(data);
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
        setOutputFields(message, "", "", "");
        return;
    }

    setOutputFields("Running...", "", "", "");
    EnableWindow(runButton, FALSE);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    EnableWindow(runButton, TRUE);

    loadOutputFields();
    if (exitCode != 0)
        showMessage(hwnd, "The simulator finished with an error. Check the output fields.");
}

static HWND makeButton(HWND hwnd, const char *text, int id)
{
    HWND button = CreateWindowA("BUTTON", text,
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                0, 0, 90, 32, hwnd, (HMENU)(INT_PTR)id,
                                GetModuleHandle(NULL), NULL);

    SendMessageA(button, WM_SETFONT, (WPARAM)uiFont, TRUE);
    return button;
}

static HWND makeEdit(HWND hwnd, int id, DWORD extraStyle)
{
    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_AUTOHSCROLL | extraStyle,
        0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);

    SendMessageA(edit, WM_SETFONT, (WPARAM)fixedFont, TRUE);
    SendMessageA(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                 MAKELPARAM(8, 8));
    return edit;
}

static void drawPanel(HDC hdc, RECT rect, COLORREF fillColor, COLORREF accentColor)
{
    HPEN borderPen = CreatePen(PS_SOLID, 1, BORDER_COLOR);
    HBRUSH panelBrush = CreateSolidBrush(fillColor);
    HBRUSH accentBrush = CreateSolidBrush(accentColor);
    RECT accent = { rect.left + 12, rect.top + 8,
                    rect.left + 52, rect.top + 13 };
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, panelBrush);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    FillRect(hdc, &accent, accentBrush);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(accentBrush);
    DeleteObject(panelBrush);
    DeleteObject(borderPen);
}

static void layout(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 16;
    int toolbarHeight = 48;
    int labelHeight = 26;
    int gap = 12;
    int panelInset = 10;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int paneTop = padding + toolbarHeight + labelHeight;
    int paneHeight = height - paneTop - padding;
    int topPaneWidth = (width - padding * 2 - gap) / 2;
    int rightX = padding + topPaneWidth + gap;
    int topPaneHeight = (paneHeight * 3) / 5;
    int lowerLabelTop = paneTop + topPaneHeight + gap;
    int lowerPaneTop = lowerLabelTop + labelHeight;
    int lowerPaneHeight = height - lowerPaneTop - padding;
    int lowerWidth = width - padding * 2;
    int lowerColumnGap = 12;
    int lowerColumnWidth = (lowerWidth - lowerColumnGap * 2) / 3;

    MoveWindow(runButton, padding, padding, 90, 32, TRUE);
    MoveWindow(uploadButton, padding + 102, padding, 138, 32, TRUE);
    MoveWindow(saveButton, padding + 252, padding, 90, 32, TRUE);
    MoveWindow(reloadButton, padding + 354, padding, 90, 32, TRUE);
    MoveWindow(clearButton, rightX, padding, 118, 32, TRUE);

    MoveWindow(editorBox, padding + panelInset, paneTop + panelInset,
               topPaneWidth - panelInset * 2, topPaneHeight - panelInset * 2,
               TRUE);
    MoveWindow(pipelineOutputBox, rightX + panelInset, paneTop + panelInset,
               topPaneWidth - panelInset * 2, topPaneHeight - panelInset * 2,
               TRUE);
    MoveWindow(registersOutputBox, padding + panelInset,
               lowerPaneTop + panelInset,
               lowerColumnWidth - panelInset * 2,
               lowerPaneHeight - panelInset * 2, TRUE);
    MoveWindow(instructionOutputBox,
               padding + lowerColumnWidth + lowerColumnGap + panelInset,
               lowerPaneTop + panelInset,
               lowerColumnWidth - panelInset * 2,
               lowerPaneHeight - panelInset * 2, TRUE);
    MoveWindow(dataOutputBox,
               padding + (lowerColumnWidth + lowerColumnGap) * 2 + panelInset,
               lowerPaneTop + panelInset,
               lowerColumnWidth - panelInset * 2,
               lowerPaneHeight - panelInset * 2, TRUE);
}

static void paintLabels(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 16;
    int gap = 12;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int paneTop = padding + 48 + 26;
    int paneHeight = height - paneTop - padding;
    int topPaneWidth = (width - padding * 2 - gap) / 2;
    int rightX = padding + topPaneWidth + gap;
    int topPaneHeight = (paneHeight * 3) / 5;
    int lowerLabelTop = paneTop + topPaneHeight + gap;
    int lowerPaneTop = lowerLabelTop + 26;
    int lowerPaneHeight = height - lowerPaneTop - padding;
    int lowerWidth = width - padding * 2;
    int lowerColumnGap = 12;
    int lowerColumnWidth = (lowerWidth - lowerColumnGap * 2) / 3;
    RECT editorPanel = { padding, paneTop,
                         padding + topPaneWidth, paneTop + topPaneHeight };
    RECT pipelinePanel = { rightX, paneTop,
                           rightX + topPaneWidth, paneTop + topPaneHeight };
    RECT registersPanel = { padding, lowerPaneTop,
                            padding + lowerColumnWidth,
                            lowerPaneTop + lowerPaneHeight };
    RECT instructionPanel = {
        padding + lowerColumnWidth + lowerColumnGap, lowerPaneTop,
        padding + lowerColumnWidth * 2 + lowerColumnGap,
        lowerPaneTop + lowerPaneHeight
    };
    RECT dataPanel = {
        padding + (lowerColumnWidth + lowerColumnGap) * 2, lowerPaneTop,
        padding + lowerWidth, lowerPaneTop + lowerPaneHeight
    };

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, labelFont);
    SetTextColor(hdc, LABEL_COLOR);

    drawPanel(hdc, editorPanel, EDITOR_PANEL_COLOR, EDITOR_ACCENT_COLOR);
    drawPanel(hdc, pipelinePanel, PIPELINE_PANEL_COLOR, PIPELINE_ACCENT_COLOR);
    drawPanel(hdc, registersPanel, REGISTERS_PANEL_COLOR, REGISTERS_ACCENT_COLOR);
    drawPanel(hdc, instructionPanel, INSTRUCTION_PANEL_COLOR,
              INSTRUCTION_ACCENT_COLOR);
    drawPanel(hdc, dataPanel, DATA_PANEL_COLOR, DATA_ACCENT_COLOR);

    SetTextColor(hdc, EDITOR_ACCENT_COLOR);
    TextOutA(hdc, padding, 54, "Program Editor", 14);
    SetTextColor(hdc, PIPELINE_ACCENT_COLOR);
    TextOutA(hdc, rightX, 54, "Pipeline Output", 15);
    SetTextColor(hdc, REGISTERS_ACCENT_COLOR);
    TextOutA(hdc, padding, lowerLabelTop, "Registers", 9);
    SetTextColor(hdc, INSTRUCTION_ACCENT_COLOR);
    TextOutA(hdc, padding + lowerColumnWidth + lowerColumnGap, lowerLabelTop,
             "Instruction Memory", 18);
    SetTextColor(hdc, DATA_ACCENT_COLOR);
    TextOutA(hdc, padding + (lowerColumnWidth + lowerColumnGap) * 2,
             lowerLabelTop, "Data Memory", 11);

    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        backgroundBrush = CreateSolidBrush(BACKGROUND_COLOR);
        editBrush = CreateSolidBrush(EDIT_COLOR);
        uiFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        labelFont = CreateFontA(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        fixedFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                FIXED_PITCH | FF_MODERN, "Consolas");

        runButton = makeButton(hwnd, "Run", IDC_RUN);
        uploadButton = makeButton(hwnd, "Upload Text File", IDC_UPLOAD);
        saveButton = makeButton(hwnd, "Save", IDC_SAVE);
        reloadButton = makeButton(hwnd, "Reload", IDC_RELOAD);
        clearButton = makeButton(hwnd, "Clear Output", IDC_CLEAR);
        editorBox = makeEdit(hwnd, IDC_EDITOR, 0);
        pipelineOutputBox = makeEdit(hwnd, IDC_PIPELINE_OUTPUT, ES_READONLY);
        registersOutputBox = makeEdit(hwnd, IDC_REGISTERS_OUTPUT, ES_READONLY);
        instructionOutputBox = makeEdit(hwnd, IDC_INSTRUCTION_OUTPUT, ES_READONLY);
        dataOutputBox = makeEdit(hwnd, IDC_DATA_OUTPUT, ES_READONLY);

        loadProgramIntoEditor();
        setOutputFields("Press Run to simulate the program.", "", "", "");
        return 0;

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, backgroundBrush);
        return 1;
    }

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
            setOutputFields("", "", "", "");
            return 0;
        }
        break;

    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wParam, BACKGROUND_COLOR);
        return (LRESULT)backgroundBrush;

    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wParam, TEXT_COLOR);
        SetBkColor((HDC)wParam, EDIT_COLOR);
        return (LRESULT)editBrush;

    case WM_DESTROY:
        if (fixedFont)
            DeleteObject(fixedFont);
        if (uiFont)
            DeleteObject(uiFont);
        if (labelFont)
            DeleteObject(labelFont);
        if (backgroundBrush)
            DeleteObject(backgroundBrush);
        if (editBrush)
            DeleteObject(editBrush);
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
    wc.hbrBackground = NULL;
    wc.lpszClassName = "PipelineSimulatorGui";

    if (!RegisterClassA(&wc))
        return 1;

    HWND hwnd = CreateWindowA(
        wc.lpszClassName, "C Pipeline Simulator",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1220, 760,
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
