#define STB_IMAGE_IMPLEMENTATION
#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <string>
#include <fstream>
#include <webp/encode.h>
#include "resource.h"
#include "stb_image.h"

#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")

// Globals for controls
HWND hInputPath, hInputBrowse;
HWND hOutputPath, hOutputBrowse;
HWND hExportButton;
HWND hMainWnd;

wchar_t inputFilePath[MAX_PATH] = L"";
wchar_t outputFilePath[MAX_PATH] = L"";

// Link against Common Controls library
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Helper: wide string to UTF-8 conversion
std::string WideToUtf8(const wchar_t* wstr) {
    if (!wstr) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed == 0) return {};
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, nullptr, nullptr);
    if (!strTo.empty() && strTo.back() == '\0') strTo.pop_back();
    return strTo;
}

bool ConvertToWebP(const char* inputPath, const char* outputPath, float quality =100.0f) {
    int width, height, channels;
    unsigned char* rgba = stbi_load(inputPath, &width, &height, &channels, 4);
    if (!rgba) {
        MessageBoxW(hMainWnd, L"Failed to load image.", L"Error", MB_ICONERROR);
        return false;
    }

    uint8_t* outputData = nullptr;
    size_t outputSize = WebPEncodeRGBA(rgba, width, height, width * 4, quality, &outputData);
    stbi_image_free(rgba);

    if (outputSize == 0) {
        MessageBoxW(hMainWnd, L"Failed to encode WebP image.", L"Error", MB_ICONERROR);
        return false;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        MessageBoxW(hMainWnd, L"Failed to open output file for writing.", L"Error", MB_ICONERROR);
        free(outputData);
        return false;
    }
    outFile.write(reinterpret_cast<char*>(outputData), outputSize);
    outFile.close();

    free(outputData);
    return true;
}

// Show file open dialog (for input)
bool OpenFileDialog(HWND owner, wchar_t* filePath, const wchar_t* filter) {
    OPENFILENAME ofn = { sizeof(ofn) };
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    return GetOpenFileName(&ofn) == TRUE;
}

// Show file save dialog (for output)
bool SaveFileDialog(HWND owner, wchar_t* filePath, const wchar_t* filter) {
    OPENFILENAME ofn = { sizeof(ofn) };
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileName(&ofn) == TRUE;
}

// Handle Browse buttons
void OnBrowseInput() {
    if (OpenFileDialog(hMainWnd, inputFilePath, L"Image Files\0*.jpg;*.jpeg;*.png\0All Files\0*.*\0")) {
        SetWindowTextW(hInputPath, inputFilePath);

        // Suggest output filename with .webp extension in same folder
        wcscpy_s(outputFilePath, MAX_PATH, inputFilePath);
        PathRenameExtensionW(outputFilePath, L".webp");
        SetWindowTextW(hOutputPath, outputFilePath);
    }
}

void OnBrowseOutput() {
    if (SaveFileDialog(hMainWnd, outputFilePath, L"WebP Image\0*.webp\0All Files\0*.*\0")) {
        SetWindowTextW(hOutputPath, outputFilePath);
    }
}

// Handle Export button click
void OnExport() {
    wchar_t inputPathBuf[MAX_PATH];
    wchar_t outputPathBuf[MAX_PATH];

    GetWindowTextW(hInputPath, inputPathBuf, MAX_PATH);
    GetWindowTextW(hOutputPath, outputPathBuf, MAX_PATH);

    if (wcslen(inputPathBuf) == 0 || wcslen(outputPathBuf) == 0) {
        MessageBoxW(hMainWnd, L"Please specify both input and output paths.", L"Error", MB_ICONERROR);
        return;
    }

    std::string inputPathUtf8 = WideToUtf8(inputPathBuf);
    std::string outputPathUtf8 = WideToUtf8(outputPathBuf);

    constexpr float quality = 100.0f; // fixed quality

    if (ConvertToWebP(inputPathUtf8.c_str(), outputPathUtf8.c_str(), quality)) {
        MessageBoxW(hMainWnd, L"Conversion successful!", L"Success", MB_OK);
    }
    else {
        MessageBoxW(hMainWnd, L"Conversion failed!", L"Error", MB_ICONERROR);
    }
}

// Window procedure with transparent static labels
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW)); // Cache background brush

    switch (msg) {
    case WM_CREATE:
    {
      
        HWND hInputLabel = CreateWindowW(L"STATIC", L"Input Image (JPG/PNG):",WS_VISIBLE | WS_CHILD  | SS_NOTIFY,10, 10, 150, 20, hWnd, NULL, NULL, NULL);

        hInputPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            10, 35, 350, 25, hWnd, NULL, NULL, NULL);
        hInputBrowse = CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD,
            370, 35, 80, 25, hWnd, (HMENU)1001, NULL, NULL);

        CreateWindowW(L"STATIC", L"Output WebP Path:",WS_VISIBLE | WS_CHILD | SS_NOTIFY, 10, 70, 150, 20, hWnd, NULL, NULL, NULL);
        hOutputPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,10, 95, 350, 25, hWnd, NULL, NULL, NULL);
        hOutputBrowse = CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD,370, 95, 80, 25, hWnd, (HMENU)1002, NULL, NULL);

        hExportButton = CreateWindowW(L"BUTTON", L"Convert to WebP",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 135, 440, 35, hWnd, (HMENU)1003, NULL, NULL);


        CreateWindowW(L"STATIC", L"Developed by Seyed Ahmad Parkhid", WS_VISIBLE | WS_CHILD | SS_NOTIFY, 10, 190, 470, 20, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"https://www.cafebit.ir", WS_VISIBLE | WS_CHILD | SS_NOTIFY, 10, 215, 470, 20, hWnd, NULL, NULL, NULL);
        // Force repaint of the input label to ensure no artifacts
        InvalidateRect(hInputLabel,NULL, TRUE);
        UpdateWindow(hInputLabel);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1001:
            OnBrowseInput();
            break;
        case 1002:
            OnBrowseOutput();
            break;
        case 1003:
            OnExport();
            break;
        }
        break;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, hBackgroundBrush);
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetBkColor(hdcStatic, GetSysColor(COLOR_WINDOW));
        // Ensure text color is clear to avoid blending issues
        SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR)hBackgroundBrush;
    }

    case WM_DESTROY:
        DeleteObject(hBackgroundBrush); // Clean up brush
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // Enable DPI awareness to handle high-DPI displays
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const wchar_t CLASS_NAME[] = L"WebPConverterWindowClass";

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_ICON)); // Large icon

    RegisterClassW(&wc);

    hMainWnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        CLASS_NAME,
        L"SAP WebP Converter 1.0",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_SIZEBOX) | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 290,
        NULL, NULL, hInstance, NULL);

    if (!hMainWnd) return 0;


    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Get window dimensions
    RECT windowRect;
    GetWindowRect(hMainWnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Calculate new position for centering
    int newX = (screenWidth - windowWidth) / 2;
    int newY = (screenHeight - windowHeight) / 2;

    // Set the window position
    SetWindowPos(hMainWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);


    ShowWindow(hMainWnd, nCmdShow);

    

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}