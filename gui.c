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
#define IDC_IF_STAGE_OUTPUT 113
#define IDC_ID_STAGE_OUTPUT 114
#define IDC_EX_STAGE_OUTPUT 115
#define IDC_STEP 116

static const char *PROGRAM_FILE = "program.txt";
static const char *OUTPUT_FILE = "gui_output.txt";
static const char *SIMULATOR_EXE = ".\\simulator.exe";

static HWND editorBox;
static HWND pipelineOutputBox;
static HWND registersOutputBox;
static HWND instructionOutputBox;
static HWND dataOutputBox;
static HWND ifStageBox;
static HWND idStageBox;
static HWND exStageBox;
static HWND runButton;
static HWND stepButton;
static HWND uploadButton;
static HWND saveButton;
static HWND reloadButton;
static HWND clearButton;
static HFONT fixedFont;
static HFONT uiFont;
static HFONT labelFont;
static HBRUSH backgroundBrush;
static HBRUSH editBrush;

static const COLORREF BACKGROUND_COLOR = RGB(248, 253, 250);
static const COLORREF PANEL_COLOR = RGB(250, 255, 253);
static const COLORREF PANEL_HEADER_COLOR = RGB(199, 244, 224);
static const COLORREF BORDER_COLOR = RGB(166, 224, 199);
static const COLORREF LABEL_COLOR = RGB(0, 65, 62);
static const COLORREF EDIT_COLOR = RGB(255, 255, 255);
static const COLORREF TEXT_COLOR = RGB(3, 33, 34);
static const COLORREF MUTED_TEXT_COLOR = RGB(76, 104, 102);
static const COLORREF SHADOW_COLOR = RGB(221, 240, 233);
static const COLORREF PRIMARY_BUTTON_COLOR = RGB(20, 139, 111);
static const COLORREF PRIMARY_BUTTON_DOWN = RGB(13, 108, 91);
static const COLORREF SECONDARY_BUTTON_COLOR = RGB(226, 250, 240);
static const COLORREF SECONDARY_BUTTON_DOWN = RGB(204, 239, 224);
static const COLORREF DISABLED_BUTTON_COLOR = RGB(230, 237, 235);
static const COLORREF DISABLED_TEXT_COLOR = RGB(126, 143, 140);
static const COLORREF FIELD_BORDER_COLOR = RGB(168, 224, 201);

static char lastActionText[128] = "Ready";
static char lastRunText[128] = "-";
static char *stepPipelineText;
static char *stepRegistersText;
static char *stepInstructionText;
static char *stepDataText;
static int currentStepCycle;
static int totalStepCycles;

static void showMessage(HWND hwnd, const char *text)
{
    MessageBoxA(hwnd, text, "Simulator GUI", MB_OK | MB_ICONINFORMATION);
}

static void setStatus(HWND hwnd, const char *action, const char *lastRun)
{
    if (action) {
        snprintf(lastActionText, sizeof(lastActionText), "%s", action);
    }
    if (lastRun) {
        snprintf(lastRunText, sizeof(lastRunText), "%s", lastRun);
    }
    if (hwnd)
        InvalidateRect(hwnd, NULL, TRUE);
}

static void appendLog(const char *message)
{
    (void)message;
}

static void setOutputFields(const char *pipeline, const char *registers,
                            const char *instruction, const char *data)
{
    SetWindowTextA(pipelineOutputBox, pipeline ? pipeline : "");
    SetWindowTextA(registersOutputBox, registers ? registers : "");
    SetWindowTextA(instructionOutputBox, instruction ? instruction : "");
    SetWindowTextA(dataOutputBox, data ? data : "");
}

static void setStageFields(const char *fetch, const char *decode,
                           const char *execute)
{
    char fetchText[1200];
    char decodeText[1200];
    char executeText[1200];

    snprintf(fetchText, sizeof(fetchText), "FETCH (IF)\r\n%s",
             fetch ? fetch : "Waiting for a run.");
    snprintf(decodeText, sizeof(decodeText), "DECODE (ID)\r\n%s",
             decode ? decode : "Waiting for a run.");
    snprintf(executeText, sizeof(executeText), "EXECUTE (EX)\r\n%s",
             execute ? execute : "Waiting for a run.");

    SetWindowTextA(ifStageBox, fetchText);
    SetWindowTextA(idStageBox, decodeText);
    SetWindowTextA(exStageBox, executeText);
}

static void setControlsEnabled(BOOL enabled)
{
    EnableWindow(runButton, enabled);
    EnableWindow(stepButton, enabled);
    EnableWindow(uploadButton, enabled);
    EnableWindow(saveButton, enabled);
    EnableWindow(reloadButton, enabled);
    EnableWindow(clearButton, enabled);
}

static char *copyTextOrEmpty(const char *text)
{
    const char *source = text ? text : "";
    size_t length = strlen(source);
    char *copy = (char *)GlobalAlloc(GPTR, length + 1);

    if (!copy)
        return NULL;

    memcpy(copy, source, length);
    copy[length] = '\0';
    return copy;
}

static void clearStepCache(void)
{
    if (stepPipelineText)
        GlobalFree(stepPipelineText);
    if (stepRegistersText)
        GlobalFree(stepRegistersText);
    if (stepInstructionText)
        GlobalFree(stepInstructionText);
    if (stepDataText)
        GlobalFree(stepDataText);

    stepPipelineText = NULL;
    stepRegistersText = NULL;
    stepInstructionText = NULL;
    stepDataText = NULL;
    currentStepCycle = 0;
    totalStepCycles = 0;
}

static const char *findLastText(const char *text, const char *needle)
{
    const char *last = NULL;
    const char *found = strstr(text, needle);

    while (found) {
        last = found;
        found = strstr(found + 1, needle);
    }

    return last;
}

static int countCycles(const char *pipelineText)
{
    int count = 0;
    const char *found = pipelineText ? strstr(pipelineText, "================ Cycle") : NULL;

    while (found) {
        count++;
        found = strstr(found + 1, "================ Cycle");
    }

    return count;
}

static const char *findCycleStart(const char *pipelineText, int cycleNumber)
{
    int index = 1;
    const char *found = pipelineText ? strstr(pipelineText, "================ Cycle") : NULL;

    while (found && index < cycleNumber) {
        found = strstr(found + 1, "================ Cycle");
        index++;
    }

    return found;
}

static void appendStageLine(char *dest, size_t destSize, const char *lineStart,
                            size_t lineLength)
{
    size_t used = strlen(dest);
    size_t prefix = 0;

    while (prefix < lineLength &&
           (lineStart[prefix] == ' ' || lineStart[prefix] == '\t')) {
        prefix++;
    }

    if (lineLength > prefix && used + lineLength - prefix + 3 < destSize) {
        memcpy(dest + used, lineStart + prefix, lineLength - prefix);
        used += lineLength - prefix;
        dest[used++] = '\r';
        dest[used++] = '\n';
        dest[used] = '\0';
    }
}

static int lineContainsText(const char *lineStart, size_t lineLength,
                            const char *needle)
{
    size_t needleLength = strlen(needle);
    size_t i;

    if (needleLength == 0 || lineLength < needleLength)
        return 0;

    for (i = 0; i <= lineLength - needleLength; i++) {
        if (memcmp(lineStart + i, needle, needleLength) == 0)
            return 1;
    }

    return 0;
}

static void extractStageSummary(const char *cycleStart, const char *stageTag,
                                const char *fallback, char *dest,
                                size_t destSize)
{
    const char *line = cycleStart;
    dest[0] = '\0';

    while (line && *line) {
        const char *lineEnd = strchr(line, '\n');
        size_t lineLength = lineEnd ? (size_t)(lineEnd - line) : strlen(line);

        if (lineContainsText(line, lineLength, "================ Cycle") &&
            line != cycleStart)
            break;

        if (lineLength > 0 && line[lineLength - 1] == '\r')
            lineLength--;

        if (lineContainsText(line, lineLength, stageTag))
            appendStageLine(dest, destSize, line, lineLength);

        if (!lineEnd)
            break;
        line = lineEnd + 1;
    }

    if (dest[0] == '\0')
        snprintf(dest, destSize, "%s", fallback);
}

static void updateStageMonitorFromOutput(const char *pipelineText)
{
    char fetch[1024];
    char decode[1024];
    char execute[1024];
    const char *lastCycle = pipelineText ? findLastText(pipelineText, "================ Cycle") : NULL;

    if (!lastCycle) {
        setStageFields("IF: no completed cycle yet.",
                       "ID: no completed cycle yet.",
                       "EX: no completed cycle yet.");
        return;
    }

    extractStageSummary(lastCycle, "[IF]", "IF: no fetch activity in the latest cycle.",
                        fetch, sizeof(fetch));
    extractStageSummary(lastCycle, "[ID]", "ID: no decode activity in the latest cycle.",
                        decode, sizeof(decode));
    extractStageSummary(lastCycle, "[EX]", "EX: no execute activity in the latest cycle.",
                        execute, sizeof(execute));
    setStageFields(fetch, decode, execute);
}

static void buildStepRegisterText(const char *pipelineText, const char *limit,
                                  char *dest, size_t destSize)
{
    int registers[64] = { 0 };
    const char *line = pipelineText;
    size_t used = 0;
    int reg;
    int before;
    int after;
    int i;

    while (line && line < limit && *line) {
        const char *lineEnd = strchr(line, '\n');
        size_t lineLength = lineEnd ? (size_t)(lineEnd - line) : strlen(line);

        if (line + lineLength > limit)
            lineLength = (size_t)(limit - line);

        if (sscanf(line, "  [EX]     [REG] R%d: %d -> %d",
                   &reg, &before, &after) == 3 &&
            reg >= 0 && reg < 64) {
            registers[reg] = after;
        }

        if (!lineEnd || lineEnd >= limit)
            break;
        line = lineEnd + 1;
    }

    used += snprintf(dest + used, destSize - used,
                     "Registers after cycle %d\r\n", currentStepCycle);

    for (i = 0; i < 64 && used < destSize; i++) {
        used += snprintf(dest + used, destSize - used,
                         "R%-2d = %4d%s", i, registers[i],
                         ((i + 1) % 4 == 0) ? "\r\n" : "    ");
    }
}

static void buildStepDataText(const char *pipelineText, const char *limit,
                              char *dest, size_t destSize)
{
    int dataMemory[2048] = { 0 };
    unsigned char written[2048] = { 0 };
    const char *line = pipelineText;
    size_t used = 0;
    int address;
    int value;
    int i;
    int any = 0;

    while (line && line < limit && *line) {
        const char *lineEnd = strchr(line, '\n');
        size_t lineLength = lineEnd ? (size_t)(lineEnd - line) : strlen(line);

        if (line + lineLength > limit)
            lineLength = (size_t)(limit - line);

        if (sscanf(line, "  [EX]     [MEM] dataMemory[%d] <- %d",
                   &address, &value) == 2 &&
            address >= 0 && address < 2048) {
            dataMemory[address] = value;
            written[address] = 1;
        }

        if (!lineEnd || lineEnd >= limit)
            break;
        line = lineEnd + 1;
    }

    used += snprintf(dest + used, destSize - used,
                     "Data memory after cycle %d\r\n", currentStepCycle);

    for (i = 0; i < 2048 && used < destSize; i++) {
        if (written[i]) {
            any = 1;
            used += snprintf(dest + used, destSize - used,
                             "[%d] = %d\r\n", i, dataMemory[i]);
        }
    }

    if (!any && used < destSize)
        snprintf(dest + used, destSize - used,
                 "No data memory has been written yet.");
}

static void cacheStepOutput(const char *pipeline, const char *registers,
                            const char *instruction, const char *data)
{
    clearStepCache();
    stepPipelineText = copyTextOrEmpty(pipeline);
    stepRegistersText = copyTextOrEmpty(registers);
    stepInstructionText = copyTextOrEmpty(instruction);
    stepDataText = copyTextOrEmpty(data);

    if (!stepPipelineText || !stepRegistersText ||
        !stepInstructionText || !stepDataText) {
        clearStepCache();
        return;
    }

    totalStepCycles = countCycles(stepPipelineText);
}

static void showCurrentStep(HWND hwnd)
{
    const char *cycleStart;
    const char *cycleEnd;
    char *cycleText;
    size_t visibleLength;
    char registerText[4096];
    char dataText[4096];
    size_t length;

    if (!stepPipelineText || totalStepCycles <= 0) {
        setOutputFields("No step-through output is available. Click Step Cycle after loading a program.",
                        "", "", "");
        return;
    }

    if (currentStepCycle >= totalStepCycles) {
        setOutputFields(stepPipelineText, stepRegistersText,
                        stepInstructionText, stepDataText);
        updateStageMonitorFromOutput(stepPipelineText);
        setStatus(hwnd, "Step-through complete", "All cycles shown");
        return;
    }

    currentStepCycle++;
    cycleStart = findCycleStart(stepPipelineText, currentStepCycle);
    cycleEnd = cycleStart ? strstr(cycleStart + 1, "================ Cycle") : NULL;
    if (!cycleStart)
        return;

    length = cycleEnd ? (size_t)(cycleEnd - cycleStart) : strlen(cycleStart);
    visibleLength = (cycleEnd ? (size_t)(cycleEnd - stepPipelineText)
                              : strlen(stepPipelineText));
    cycleText = (char *)GlobalAlloc(GPTR, visibleLength + 128);
    if (!cycleText)
        return;

    snprintf(cycleText, visibleLength + 128,
             "Showing cycles 1 through %d of %d\r\n\r\n",
             currentStepCycle, totalStepCycles);
    strncat(cycleText, stepPipelineText, visibleLength);

    buildStepRegisterText(stepPipelineText, cycleStart + length,
                          registerText, sizeof(registerText));
    buildStepDataText(stepPipelineText, cycleStart + length,
                      dataText, sizeof(dataText));

    setOutputFields(cycleText, registerText, stepInstructionText, dataText);
    updateStageMonitorFromOutput(cycleText);
    setStatus(hwnd, "Stepping cycles", "Cycle view");
    GlobalFree(cycleText);
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
        appendLog("program.txt was not found; editor opened empty.");
        return;
    }

    SetWindowTextA(editorBox, text);
    appendLog("Loaded program.txt into the editor.");
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
        appendLog("Upload failed: could not read selected text file.");
        return;
    }

    SetWindowTextA(editorBox, text);
    setOutputFields("Uploaded text file. Press Run to simulate it.", "", "", "");
    setStageFields("Program loaded. Run to update fetch.",
                   "Program loaded. Run to update decode.",
                   "Program loaded. Run to update execute.");
    appendLog("Uploaded a text file into the editor.");
    setStatus(hwnd, "Program uploaded", NULL);
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
        setStageFields("IF: no output yet.", "ID: no output yet.", "EX: no output yet.");
        clearStepCache();
        appendLog("No simulator output file was found.");
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
        setStageFields("IF: output unavailable.",
                       "ID: output unavailable.",
                       "EX: output unavailable.");
        clearStepCache();
        appendLog("Could not allocate memory while loading simulator output.");
    } else {
        setOutputFields(pipeline, registers, instruction, data);
        cacheStepOutput(pipeline, registers, instruction, data);
        updateStageMonitorFromOutput(pipeline);
        appendLog("Simulator output loaded into the result sections.");
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

    appendLog("Saved editor contents to program.txt before running.");
    setStatus(hwnd, "Running simulator", "Running...");

    HANDLE outFile = CreateFileA(OUTPUT_FILE, GENERIC_WRITE, FILE_SHARE_READ,
                                 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (outFile == INVALID_HANDLE_VALUE) {
        showMessage(hwnd, "Could not create gui_output.txt.");
        setStageFields("IF: run did not start.",
                       "ID: run did not start.",
                       "EX: run did not start.");
        appendLog("Run failed: could not create gui_output.txt.");
        setStatus(hwnd, "Output file error", "Failed");
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
        setStageFields("IF: simulator did not launch.",
                       "ID: simulator did not launch.",
                       "EX: simulator did not launch.");
        appendLog("Run failed: simulator.exe could not be started.");
        setStatus(hwnd, "Simulator launch failed", "Failed");
        return;
    }

    setOutputFields("Running...", "", "", "");
    setStageFields("IF: simulator is running.",
                   "ID: simulator is running.",
                   "EX: simulator is running.");
    setControlsEnabled(FALSE);
    appendLog("Simulator process started.");
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    setControlsEnabled(TRUE);

    loadOutputFields();
    if (exitCode != 0) {
        appendLog("Simulator finished with an error.");
        setStatus(hwnd, "Simulator error", "Failed");
        showMessage(hwnd, "The simulator finished with an error. Check the output fields.");
    } else {
        appendLog("Simulator completed successfully.");
        setStatus(hwnd, "Simulation complete", "Success");
    }
}

static void stepSimulator(HWND hwnd)
{
    if (!stepPipelineText || currentStepCycle >= totalStepCycles) {
        runSimulator(hwnd);
        currentStepCycle = 0;
    }

    showCurrentStep(hwnd);
}

static HWND makeButton(HWND hwnd, const char *text, int id)
{
    HWND button = CreateWindowA("BUTTON", text,
                                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                0, 0, 90, 32, hwnd, (HMENU)(INT_PTR)id,
                                GetModuleHandle(NULL), NULL);

    SendMessageA(button, WM_SETFONT, (WPARAM)uiFont, TRUE);
    return button;
}

static HWND makeEdit(HWND hwnd, int id, DWORD extraStyle)
{
    HWND edit = CreateWindowExA(
        0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_AUTOHSCROLL | extraStyle,
        0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);

    SendMessageA(edit, WM_SETFONT, (WPARAM)fixedFont, TRUE);
    SendMessageA(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                 MAKELPARAM(8, 8));
    return edit;
}

static void drawButtonControl(const DRAWITEMSTRUCT *item)
{
    char text[96];
    RECT rect = item->rcItem;
    int id = (int)item->CtlID;
    int disabled = (item->itemState & ODS_DISABLED) != 0;
    int pressed = (item->itemState & ODS_SELECTED) != 0;
    int primary = (id == IDC_RUN || id == IDC_STEP);
    COLORREF fillColor = disabled ? DISABLED_BUTTON_COLOR :
                         pressed ? (primary ? PRIMARY_BUTTON_DOWN : SECONDARY_BUTTON_DOWN) :
                         primary ? PRIMARY_BUTTON_COLOR : SECONDARY_BUTTON_COLOR;
    COLORREF borderColor = primary ? PRIMARY_BUTTON_DOWN : BORDER_COLOR;
    COLORREF textColor = disabled ? DISABLED_TEXT_COLOR :
                         primary ? RGB(255, 255, 255) : LABEL_COLOR;
    HBRUSH brush = CreateSolidBrush(fillColor);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HPEN oldPen = (HPEN)SelectObject(item->hDC, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(item->hDC, brush);
    HFONT oldFont = (HFONT)SelectObject(item->hDC, uiFont);

    GetWindowTextA(item->hwndItem, text, sizeof(text));
    RoundRect(item->hDC, rect.left, rect.top, rect.right, rect.bottom, 14, 14);

    if (item->itemState & ODS_FOCUS) {
        RECT focusRect = { rect.left + 3, rect.top + 3, rect.right - 3, rect.bottom - 3 };
        HPEN focusPen = CreatePen(PS_SOLID, 1, RGB(75, 168, 142));
        SelectObject(item->hDC, focusPen);
        SelectObject(item->hDC, GetStockObject(NULL_BRUSH));
        RoundRect(item->hDC, focusRect.left, focusRect.top,
                  focusRect.right, focusRect.bottom, 10, 10);
        DeleteObject(focusPen);
        SelectObject(item->hDC, brush);
    }

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, textColor);
    if (pressed)
        OffsetRect(&rect, 0, 1);
    DrawTextA(item->hDC, text, -1, &rect,
              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);

    SelectObject(item->hDC, oldFont);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void drawTextFieldShell(HDC hdc, RECT rect)
{
    HPEN borderPen = CreatePen(PS_SOLID, 1, FIELD_BORDER_COLOR);
    HBRUSH fillBrush = CreateSolidBrush(EDIT_COLOR);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);
}

static void drawSection(HDC hdc, RECT rect, const char *title)
{
    HPEN borderPen = CreatePen(PS_SOLID, 1, BORDER_COLOR);
    HBRUSH panelBrush = CreateSolidBrush(PANEL_COLOR);
    HBRUSH headerBrush = CreateSolidBrush(PANEL_HEADER_COLOR);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, panelBrush);
    RECT shadow = { rect.left + 2, rect.top + 2, rect.right + 2, rect.bottom + 2 };
    HBRUSH shadowBrush = CreateSolidBrush(SHADOW_COLOR);
    RECT header = { rect.left + 1, rect.top + 1, rect.right - 1, rect.top + 32 };
    RECT titleRect = { rect.left + 12, rect.top + 6, rect.right - 12, rect.top + 30 };

    SelectObject(hdc, shadowBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));
    RoundRect(hdc, shadow.left, shadow.top, shadow.right, shadow.bottom, 18, 18);

    SelectObject(hdc, borderPen);
    SelectObject(hdc, panelBrush);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 18, 18);
    FillRect(hdc, &header, headerBrush);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, labelFont);
    SetTextColor(hdc, LABEL_COLOR);
    DrawTextA(hdc, title, -1, &titleRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(shadowBrush);
    DeleteObject(headerBrush);
    DeleteObject(panelBrush);
    DeleteObject(borderPen);
}

static void layout(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 18;
    int gap = 10;
    int panelInset = 10;
    int fieldPad = 6;
    int headerHeight = 32;
    int controlsHeight = 92;
    int stageHeight = 128;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int usableWidth = width - padding * 2;
    int controlTop = padding;
    int controlInnerTop = controlTop + headerHeight + 14;
    int stageTop = controlTop + controlsHeight + gap;
    int paneTop = stageTop + stageHeight + gap;
    int contentHeight = height - padding - paneTop;
    int minimumContentHeight = 280;
    int topPaneHeight = (contentHeight * 56) / 100;
    int lowerPaneTop = paneTop + topPaneHeight + gap;
    int lowerPaneHeight = contentHeight - topPaneHeight - gap;
    int topPaneWidth = (usableWidth - gap) / 2;
    int rightX = padding + topPaneWidth + gap;
    int lowerColumnWidth = (usableWidth - gap * 2) / 3;
    int editTopInset = panelInset + headerHeight;
    int stageBoxTop = stageTop + editTopInset;
    int stageBoxHeight = stageHeight - editTopInset - panelInset;
    int stageBoxWidth = (usableWidth - gap * 2 - panelInset * 2) / 3;

    if (contentHeight < minimumContentHeight) {
        contentHeight = minimumContentHeight;
        topPaneHeight = (contentHeight * 56) / 100;
        lowerPaneTop = paneTop + topPaneHeight + gap;
        lowerPaneHeight = contentHeight - topPaneHeight - gap;
    }

    MoveWindow(runButton, padding + 14, controlInnerTop, 92, 34, TRUE);
    MoveWindow(stepButton, padding + 116, controlInnerTop, 112, 34, TRUE);
    MoveWindow(uploadButton, padding + 238, controlInnerTop, 148, 34, TRUE);
    MoveWindow(saveButton, padding + 396, controlInnerTop, 92, 34, TRUE);
    MoveWindow(reloadButton, padding + 498, controlInnerTop, 98, 34, TRUE);
    MoveWindow(clearButton, padding + 606, controlInnerTop, 122, 34, TRUE);

    MoveWindow(ifStageBox, padding + panelInset + fieldPad,
               stageBoxTop + fieldPad,
               stageBoxWidth - fieldPad * 2,
               stageBoxHeight - fieldPad * 2, TRUE);
    MoveWindow(idStageBox, padding + panelInset + stageBoxWidth + gap + fieldPad,
               stageBoxTop + fieldPad,
               stageBoxWidth - fieldPad * 2,
               stageBoxHeight - fieldPad * 2, TRUE);
    MoveWindow(exStageBox,
               padding + panelInset + (stageBoxWidth + gap) * 2 + fieldPad,
               stageBoxTop + fieldPad,
               stageBoxWidth - fieldPad * 2,
               stageBoxHeight - fieldPad * 2, TRUE);

    MoveWindow(editorBox, padding + panelInset + fieldPad,
               paneTop + editTopInset + fieldPad,
               topPaneWidth - panelInset * 2 - fieldPad * 2,
               topPaneHeight - editTopInset - panelInset - fieldPad * 2, TRUE);
    MoveWindow(pipelineOutputBox, rightX + panelInset + fieldPad,
               paneTop + editTopInset + fieldPad,
               topPaneWidth - panelInset * 2 - fieldPad * 2,
               topPaneHeight - editTopInset - panelInset - fieldPad * 2, TRUE);
    MoveWindow(registersOutputBox, padding + panelInset + fieldPad,
               lowerPaneTop + editTopInset + fieldPad,
               lowerColumnWidth - panelInset * 2 - fieldPad * 2,
               lowerPaneHeight - editTopInset - panelInset - fieldPad * 2, TRUE);
    MoveWindow(instructionOutputBox,
               padding + lowerColumnWidth + gap + panelInset + fieldPad,
               lowerPaneTop + editTopInset + fieldPad,
               lowerColumnWidth - panelInset * 2 - fieldPad * 2,
               lowerPaneHeight - editTopInset - panelInset - fieldPad * 2, TRUE);
    MoveWindow(dataOutputBox,
               padding + (lowerColumnWidth + gap) * 2 + panelInset + fieldPad,
               lowerPaneTop + editTopInset + fieldPad,
               lowerColumnWidth - panelInset * 2 - fieldPad * 2,
               lowerPaneHeight - editTopInset - panelInset - fieldPad * 2, TRUE);
}

static void paintLabels(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    int padding = 18;
    int gap = 10;
    int panelInset = 10;
    int headerHeight = 32;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int usableWidth = width - padding * 2;
    int controlsHeight = 92;
    int stageHeight = 128;
    int controlTop = padding;
    int stageTop = controlTop + controlsHeight + gap;
    int paneTop = stageTop + stageHeight + gap;
    int contentHeight = height - padding - paneTop;
    int minimumContentHeight = 280;
    int topPaneWidth = (usableWidth - gap) / 2;
    int rightX = padding + topPaneWidth + gap;
    int topPaneHeight = (contentHeight * 56) / 100;
    int lowerPaneTop = paneTop + topPaneHeight + gap;
    int lowerPaneHeight = contentHeight - topPaneHeight - gap;
    int lowerColumnWidth = (usableWidth - gap * 2) / 3;
    int editTopInset = panelInset + headerHeight;
    int stageBoxTop = stageTop + editTopInset;
    int stageBoxHeight = stageHeight - editTopInset - panelInset;
    int stageBoxWidth = (usableWidth - gap * 2 - panelInset * 2) / 3;
    RECT controlsPanel = { padding, controlTop,
                           padding + usableWidth, controlTop + controlsHeight };
    RECT stagePanel = { padding, stageTop,
                        padding + usableWidth, stageTop + stageHeight };
    RECT editorPanel = { padding, paneTop,
                         padding + topPaneWidth, paneTop + topPaneHeight };
    RECT pipelinePanel = { rightX, paneTop,
                           rightX + topPaneWidth, paneTop + topPaneHeight };
    RECT registersPanel = { padding, lowerPaneTop,
                            padding + lowerColumnWidth,
                            lowerPaneTop + lowerPaneHeight };
    RECT instructionPanel = {
        padding + lowerColumnWidth + gap, lowerPaneTop,
        padding + lowerColumnWidth * 2 + gap,
        lowerPaneTop + lowerPaneHeight
    };
    RECT dataPanel = {
        padding + (lowerColumnWidth + gap) * 2, lowerPaneTop,
        padding + usableWidth, lowerPaneTop + lowerPaneHeight
    };
    RECT controlText = { padding + 746, controlTop + 40,
                         padding + usableWidth - 16, controlTop + 70 };
    RECT ifStageShell = { padding + panelInset, stageBoxTop,
                          padding + panelInset + stageBoxWidth,
                          stageBoxTop + stageBoxHeight };
    RECT idStageShell = { padding + panelInset + stageBoxWidth + gap,
                          stageBoxTop,
                          padding + panelInset + stageBoxWidth * 2 + gap,
                          stageBoxTop + stageBoxHeight };
    RECT exStageShell = { padding + panelInset + (stageBoxWidth + gap) * 2,
                          stageBoxTop,
                          padding + panelInset + stageBoxWidth * 3 + gap * 2,
                          stageBoxTop + stageBoxHeight };
    RECT editorShell = { padding + panelInset, paneTop + editTopInset,
                         padding + topPaneWidth - panelInset,
                         paneTop + topPaneHeight - panelInset };
    RECT pipelineShell = { rightX + panelInset, paneTop + editTopInset,
                           rightX + topPaneWidth - panelInset,
                           paneTop + topPaneHeight - panelInset };
    RECT registersShell = { padding + panelInset, lowerPaneTop + editTopInset,
                            padding + lowerColumnWidth - panelInset,
                            lowerPaneTop + lowerPaneHeight - panelInset };
    RECT instructionShell = { padding + lowerColumnWidth + gap + panelInset,
                              lowerPaneTop + editTopInset,
                              padding + lowerColumnWidth * 2 + gap - panelInset,
                              lowerPaneTop + lowerPaneHeight - panelInset };
    RECT dataShell = { padding + (lowerColumnWidth + gap) * 2 + panelInset,
                       lowerPaneTop + editTopInset,
                       padding + usableWidth - panelInset,
                       lowerPaneTop + lowerPaneHeight - panelInset };

    if (contentHeight < minimumContentHeight) {
        contentHeight = minimumContentHeight;
        topPaneHeight = (contentHeight * 56) / 100;
        lowerPaneTop = paneTop + topPaneHeight + gap;
        lowerPaneHeight = contentHeight - topPaneHeight - gap;
        editorPanel.bottom = editorPanel.top + topPaneHeight;
        pipelinePanel.bottom = pipelinePanel.top + topPaneHeight;
        registersPanel.top = lowerPaneTop;
        registersPanel.bottom = lowerPaneTop + lowerPaneHeight;
        instructionPanel.top = lowerPaneTop;
        instructionPanel.bottom = lowerPaneTop + lowerPaneHeight;
        dataPanel.top = lowerPaneTop;
        dataPanel.bottom = lowerPaneTop + lowerPaneHeight;
        editorShell.bottom = editorPanel.bottom - panelInset;
        pipelineShell.bottom = pipelinePanel.bottom - panelInset;
        registersShell.top = lowerPaneTop + editTopInset;
        registersShell.bottom = registersPanel.bottom - panelInset;
        instructionShell.top = lowerPaneTop + editTopInset;
        instructionShell.bottom = instructionPanel.bottom - panelInset;
        dataShell.top = lowerPaneTop + editTopInset;
        dataShell.bottom = dataPanel.bottom - panelInset;
    }

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, labelFont);
    SetTextColor(hdc, LABEL_COLOR);

    drawSection(hdc, controlsPanel, "Controls");
    drawSection(hdc, stagePanel, "Pipeline Stage Monitor - Latest Cycle");
    drawSection(hdc, editorPanel, "Program Editor");
    drawSection(hdc, pipelinePanel, "Pipeline Output");
    drawSection(hdc, registersPanel, "Registers");
    drawSection(hdc, instructionPanel, "Instruction Memory - Binary / Hex / Decimal");
    drawSection(hdc, dataPanel, "Data Memory");
    drawTextFieldShell(hdc, ifStageShell);
    drawTextFieldShell(hdc, idStageShell);
    drawTextFieldShell(hdc, exStageShell);
    drawTextFieldShell(hdc, editorShell);
    drawTextFieldShell(hdc, pipelineShell);
    drawTextFieldShell(hdc, registersShell);
    drawTextFieldShell(hdc, instructionShell);
    drawTextFieldShell(hdc, dataShell);

    SelectObject(hdc, uiFont);
    SetTextColor(hdc, MUTED_TEXT_COLOR);
    DrawTextA(hdc, "Run all cycles or step through them one cycle at a time.",
              -1, &controlText, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);

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

        runButton = makeButton(hwnd, "Run All", IDC_RUN);
        stepButton = makeButton(hwnd, "Step Cycle", IDC_STEP);
        uploadButton = makeButton(hwnd, "Upload Text File", IDC_UPLOAD);
        saveButton = makeButton(hwnd, "Save", IDC_SAVE);
        reloadButton = makeButton(hwnd, "Reload", IDC_RELOAD);
        clearButton = makeButton(hwnd, "Clear Output", IDC_CLEAR);
        editorBox = makeEdit(hwnd, IDC_EDITOR, 0);
        pipelineOutputBox = makeEdit(hwnd, IDC_PIPELINE_OUTPUT, ES_READONLY);
        registersOutputBox = makeEdit(hwnd, IDC_REGISTERS_OUTPUT, ES_READONLY);
        instructionOutputBox = makeEdit(hwnd, IDC_INSTRUCTION_OUTPUT, ES_READONLY);
        dataOutputBox = makeEdit(hwnd, IDC_DATA_OUTPUT, ES_READONLY);
        ifStageBox = makeEdit(hwnd, IDC_IF_STAGE_OUTPUT, ES_READONLY);
        idStageBox = makeEdit(hwnd, IDC_ID_STAGE_OUTPUT, ES_READONLY);
        exStageBox = makeEdit(hwnd, IDC_EX_STAGE_OUTPUT, ES_READONLY);

        loadProgramIntoEditor();
        setOutputFields("Press Run to simulate the program.", "", "", "");
        setStageFields("Waiting for a simulator run.",
                       "Waiting for a simulator run.",
                       "Waiting for a simulator run.");
        appendLog("GUI initialized and ready.");
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

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO *)lParam;
        mmi->ptMinTrackSize.x = 1180;
        mmi->ptMinTrackSize.y = 780;
        return 0;
    }

    case WM_PAINT:
        paintLabels(hwnd);
        return 0;

    case WM_DRAWITEM:
        drawButtonControl((const DRAWITEMSTRUCT *)lParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_EDITOR:
            if (HIWORD(wParam) == EN_CHANGE)
                clearStepCache();
            return 0;
        case IDC_RUN:
            runSimulator(hwnd);
            return 0;
        case IDC_STEP:
            stepSimulator(hwnd);
            return 0;
        case IDC_UPLOAD:
            uploadTextFile(hwnd);
            return 0;
        case IDC_SAVE:
            if (saveEditorToProgram(hwnd)) {
                appendLog("Saved editor contents to program.txt.");
                setStatus(hwnd, "Program saved", NULL);
                showMessage(hwnd, "Saved program.txt.");
            } else {
                appendLog("Save failed.");
                setStatus(hwnd, "Save failed", NULL);
            }
            return 0;
        case IDC_RELOAD:
            loadProgramIntoEditor();
            setStatus(hwnd, "Program reloaded", NULL);
            return 0;
        case IDC_CLEAR:
            setOutputFields("", "", "", "");
            setStageFields("Output cleared. Run again to refresh fetch.",
                           "Output cleared. Run again to refresh decode.",
                           "Output cleared. Run again to refresh execute.");
            clearStepCache();
            appendLog("Cleared output sections.");
            setStatus(hwnd, "Output cleared", NULL);
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

    case WM_CTLCOLORSTATIC:
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
        clearStepCache();
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
        CW_USEDEFAULT, CW_USEDEFAULT, 1320, 860,
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
