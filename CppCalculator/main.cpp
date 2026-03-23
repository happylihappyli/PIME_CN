#include <windows.h>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstdio>

void WriteHwndToTempFile(HWND hWnd) {
    char tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    std::string path = std::string(tempPath) + "pime_calc_hwnd.txt";
    FILE* f = fopen(path.c_str(), "w");
    if (f) {
        // Write as integer
        fprintf(f, "%lld", (long long)(uintptr_t)hWnd);
        fclose(f);
    }
}

void RemoveHwndTempFile() {
    char tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    std::string path = std::string(tempPath) + "pime_calc_hwnd.txt";
    remove(path.c_str());
}

void ForceForegroundWindow(HWND hWnd) {
    if (!hWnd) return;
    
    // 1. 尝试解除“前台锁定超时”限制
    DWORD dwTimeout = 0;
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);

    // 2. 尝试附加输入线程
    DWORD dwCurrentThread = GetCurrentThreadId();
    DWORD dwFGThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    
    if (dwCurrentThread != dwFGThread) {
        AttachThreadInput(dwCurrentThread, dwFGThread, TRUE);
        
        // 3. 各种激活操作组合拳
        // 核心技巧：最小化再还原，强制 Windows 刷新焦点状态
        ShowWindow(hWnd, SW_MINIMIZE);
        ShowWindow(hWnd, SW_RESTORE);
        
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
        BringWindowToTop(hWnd);
        
        AttachThreadInput(dwCurrentThread, dwFGThread, FALSE);
    } else {
        SetForegroundWindow(hWnd);
        BringWindowToTop(hWnd);
    }
    
    // 4. 恢复前台锁定限制
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)(size_t)dwTimeout, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
    
    // 5. 尝试 SwitchToThisWindow (虽然是旧 API，但在某些情况下有效)
    SwitchToThisWindow(hWnd, TRUE);

    // 6. 最后的模拟输入尝试 (如果上面的都失败了)
    if (GetForegroundWindow() != hWnd) {
        keybd_event(VK_MENU, 0, 0, 0); // 按下 ALT
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0); // 释放 ALT
        SetForegroundWindow(hWnd);
    }
}

#define IDC_INPUT 101
#define IDC_RESULT 102
#define ID_CALCULATE 103
#define ID_COPY 104
#define IDC_HISTORY 105
#define IDC_CLEAR 106
#define IDC_CLEAR_HISTORY 107
#define WM_USER_SHOW (WM_USER + 1)

HWND hHistory;

// 简单的焦点守护定时器
void CALLBACK FocusGuardTimer(HWND hWnd, UINT, UINT_PTR, DWORD) {
    static int retryCount = 0;
    if (GetForegroundWindow() != hWnd) {
        ForceForegroundWindow(hWnd);
        SetFocus(GetDlgItem(hWnd, IDC_INPUT));
    }
    retryCount++;
    if (retryCount > 20) { // 尝试 20 次 (约 2 秒) 后停止
        KillTimer(hWnd, 1);
    }
}

void CopyResultToClipboard(HWND hWnd) {
    // Check if user selected history item
    int sel = (int)SendMessage(hHistory, LB_GETCURSEL, 0, 0);
    std::string text;
    
    if (sel != LB_ERR) {
        // Copy selected history
        int len = (int)SendMessage(hHistory, LB_GETTEXTLEN, sel, 0);
        if (len > 0) {
            std::vector<char> buf(len + 1);
            SendMessage(hHistory, LB_GETTEXT, sel, (LPARAM)buf.data());
            text = buf.data();
        }
    } else {
        // Copy current result + input
        HWND hResult = GetDlgItem(hWnd, IDC_RESULT);
        HWND hInput = GetDlgItem(hWnd, IDC_INPUT);
        
        int lenRes = GetWindowTextLength(hResult);
        int lenIn = GetWindowTextLength(hInput);
        
        if (lenRes > 0 && lenIn > 0) {
            std::vector<char> bufRes(lenRes + 1);
            GetWindowText(hResult, bufRes.data(), lenRes + 1);
            
            std::vector<char> bufIn(lenIn + 1);
            GetWindowText(hInput, bufIn.data(), lenIn + 1);
            
            std::string sRes = bufRes.data();
            // Remove "= " prefix
            if (sRes.find("= ") == 0) sRes = sRes.substr(2);
            
            text = std::string(bufIn.data()) + " = " + sRes;
        }
    }
    
    if (text.empty()) return;

    if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (hGlob) {
            memcpy(GlobalLock(hGlob), text.c_str(), text.length() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
        CloseClipboard();
        
        SetWindowText(GetDlgItem(hWnd, ID_COPY), "已复制!");
        SetTimer(hWnd, 2, 1000, NULL);
    }
}

// ==========================================
// Calculator Logic (Preserved)
// ==========================================

enum CalcTokenType {
    TOKEN_NUMBER, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_END
};

struct CalcToken {
    CalcTokenType type;
    double value;
};

class Calculator {
public:
    Calculator(const std::string& expression) : expr(expression), pos(0) {}

    double evaluate() {
        tokens = tokenize(expr);
        currentTokenIndex = 0;
        return parseExpression();
    }

private:
    std::string expr;
    size_t pos;
    std::vector<CalcToken> tokens;
    size_t currentTokenIndex;

    std::vector<CalcToken> tokenize(const std::string& text) {
        std::vector<CalcToken> result;
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            if (isspace(c)) continue;

            if (isdigit(c) || c == '.') {
                std::string numStr;
                while (i < text.length() && (isdigit(text[i]) || text[i] == '.')) {
                    numStr += text[i];
                    i++;
                }
                i--;
                result.push_back({TOKEN_NUMBER, std::stod(numStr)});
            } else {
                switch (c) {
                    case '+': result.push_back({TOKEN_PLUS, 0}); break;
                    case '-': result.push_back({TOKEN_MINUS, 0}); break;
                    case '*': result.push_back({TOKEN_MULTIPLY, 0}); break;
                    case '/': result.push_back({TOKEN_DIVIDE, 0}); break;
                    case '(': result.push_back({TOKEN_LPAREN, 0}); break;
                    case ')': result.push_back({TOKEN_RPAREN, 0}); break;
                    default: throw std::runtime_error("未知字符");
                }
            }
        }
        result.push_back({TOKEN_END, 0});
        return result;
    }

    CalcToken peek() {
        if (currentTokenIndex < tokens.size()) return tokens[currentTokenIndex];
        return {TOKEN_END, 0};
    }

    CalcToken advance() {
        if (currentTokenIndex < tokens.size()) return tokens[currentTokenIndex++];
        return {TOKEN_END, 0};
    }

    double parseExpression() {
        double left = parseTerm();
        while (peek().type == TOKEN_PLUS || peek().type == TOKEN_MINUS) {
            CalcToken op = advance();
            double right = parseTerm();
            if (op.type == TOKEN_PLUS) left += right;
            else left -= right;
        }
        return left;
    }

    double parseTerm() {
        double left = parseFactor();
        while (peek().type == TOKEN_MULTIPLY || peek().type == TOKEN_DIVIDE) {
            CalcToken op = advance();
            double right = parseFactor();
            if (op.type == TOKEN_MULTIPLY) left *= right;
            else {
                if (right == 0) throw std::runtime_error("除数不能为零");
                left /= right;
            }
        }
        return left;
    }

    double parseFactor() {
        CalcToken token = advance();
        if (token.type == TOKEN_NUMBER) return token.value;
        if (token.type == TOKEN_LPAREN) {
            double result = parseExpression();
            if (advance().type != TOKEN_RPAREN) throw std::runtime_error("缺少右括号");
            return result;
        }
        if (token.type == TOKEN_MINUS) return -parseFactor();
        if (token.type == TOKEN_END) throw std::runtime_error("表达式意外结束");
        throw std::runtime_error("语法错误");
    }
};

// ==========================================
// GUI Logic
// ==========================================

HWND hEdit;
HWND hResult;
WNDPROC OldEditProc;
HFONT hFontInput;
HFONT hFontResult;

// Subclass procedure for Edit control to handle Enter and Escape
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            SendMessage(GetParent(hWnd), WM_COMMAND, ID_CALCULATE, 0);
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            SendMessage(GetParent(hWnd), WM_CLOSE, 0, 0);
            return 0;
        }
    }
    // Select all on Ctrl+A
    if (uMsg == WM_CHAR && wParam == 1) {
        SendMessage(hWnd, EM_SETSEL, 0, -1);
        return 0;
    }
    return CallWindowProc(OldEditProc, hWnd, uMsg, wParam, lParam);
}

void DoCalculate(HWND hWnd) {
    int len = GetWindowTextLength(hEdit);
    if (len == 0) return;

    std::vector<char> buf(len + 1);
    GetWindowText(hEdit, buf.data(), len + 1);
    std::string expr(buf.data());

    try {
        Calculator calc(expr);
        double result = calc.evaluate();
        
        std::stringstream ss;
        ss << result; // Simple formatting
        // Or remove trailing zeros if needed
        
        std::string resStr = "= " + ss.str();
        SetWindowText(hResult, resStr.c_str());
        
        // Add to history
        std::string historyItem = expr + " = " + ss.str();
        SendMessage(hHistory, LB_INSERTSTRING, 0, (LPARAM)historyItem.c_str());
        // Select input again
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        
    } catch (const std::exception& e) {
        std::string errStr = "错误: " + std::string(e.what());
        SetWindowText(hResult, errStr.c_str());
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Create Fonts
        hFontInput = CreateFont(26, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hFontResult = CreateFont(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        // Input Edit
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", 
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 
            10, 10, 365, 35, hWnd, (HMENU)IDC_INPUT, GetModuleHandle(NULL), NULL);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFontInput, TRUE);

        // Result Static
        hResult = CreateWindowEx(0, "STATIC", "准备就绪", 
            WS_CHILD | WS_VISIBLE, 
            12, 55, 360, 25, hWnd, (HMENU)IDC_RESULT, GetModuleHandle(NULL), NULL);
        SendMessage(hResult, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // Buttons Row (y=90)
        int btnY = 90;
        int btnH = 30;

        // Calculate Button
        HWND hBtnCalc = CreateWindow("BUTTON", "计算", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, btnY, 60, btnH, hWnd, (HMENU)ID_CALCULATE, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnCalc, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // Clear Input Button
        HWND hBtnClear = CreateWindow("BUTTON", "清空", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            80, btnY, 60, btnH, hWnd, (HMENU)IDC_CLEAR, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // Copy Button
        HWND hBtnCopy = CreateWindow("BUTTON", "复制结果", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            150, btnY, 100, btnH, hWnd, (HMENU)ID_COPY, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnCopy, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // Clear History Button
        HWND hBtnClearHist = CreateWindow("BUTTON", "清空历史", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, btnY, 110, btnH, hWnd, (HMENU)IDC_CLEAR_HISTORY, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnClearHist, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // History ListBox
        hHistory = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", NULL, 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
            10, 130, 365, 140, hWnd, (HMENU)IDC_HISTORY, GetModuleHandle(NULL), NULL);
        SendMessage(hHistory, WM_SETFONT, (WPARAM)hFontResult, TRUE);

        // Subclass Edit
        OldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        
        // Start Focus Guard Timer (100ms)
        SetTimer(hWnd, 1, 100, FocusGuardTimer);
        
        // Set Focus to Edit
        SetFocus(hEdit);
        break;
    }
    case WM_TIMER:
        if (wParam == 2) {
            KillTimer(hWnd, 2);
            SetWindowText(GetDlgItem(hWnd, ID_COPY), "复制结果");
        }
        break;
    case WM_SETFOCUS:
        SetFocus(hEdit);
        break;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_CALCULATE) {
            DoCalculate(hWnd);
            SetFocus(hEdit); // Return focus to edit
        } else if (LOWORD(wParam) == ID_COPY) {
            CopyResultToClipboard(hWnd);
            SetFocus(hEdit); // Return focus to edit after clicking
        } else if (LOWORD(wParam) == IDC_CLEAR) {
            SetWindowText(hEdit, "");
            SetWindowText(hResult, "准备就绪");
            SetFocus(hEdit);
        } else if (LOWORD(wParam) == IDC_CLEAR_HISTORY) {
            SendMessage(hHistory, LB_RESETCONTENT, 0, 0);
            SetFocus(hEdit);
        } else if (LOWORD(wParam) == IDC_HISTORY && HIWORD(wParam) == LBN_DBLCLK) {
            // Restore history item to input?
            int sel = (int)SendMessage(hHistory, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                int len = (int)SendMessage(hHistory, LB_GETTEXTLEN, sel, 0);
                if (len > 0) {
                    std::vector<char> buf(len + 1);
                    SendMessage(hHistory, LB_GETTEXT, sel, (LPARAM)buf.data());
                    std::string text = buf.data();
                    size_t eqPos = text.find(" = ");
                    if (eqPos != std::string::npos) {
                        std::string expr = text.substr(0, eqPos);
                        SetWindowText(hEdit, expr.c_str());
                        SetFocus(hEdit);
                    }
                }
            }
        }
        break;
    case WM_CLOSE:
        // Hide window instead of destroy
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    case WM_USER_SHOW:
        // Show window when requested by another instance
        ShowWindow(hWnd, SW_SHOW);
        ShowWindow(hWnd, SW_RESTORE);
        ForceForegroundWindow(hWnd);
        return 0;
    case WM_DESTROY:
        DeleteObject(hFontInput);
        DeleteObject(hFontResult);
        RemoveHwndTempFile();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Check for --hidden argument early
    std::string cmdLine = lpCmdLine;
    bool startHidden = (cmdLine.find("--hidden") != std::string::npos);

    // Single Instance Check
    HANDLE hMutex = CreateMutex(NULL, TRUE, "PIMECalculatorInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // If we are just trying to ensure it's running hidden, and it IS running, just exit.
        if (startHidden) {
             return 0;
        }

        // Find existing window and show it
        HWND hExisting = FindWindow("PIMECalculatorWindow", NULL);
        if (hExisting) {
            SendMessage(hExisting, WM_USER_SHOW, 0, 0);
        }
        return 0;
    }

    const char* CLASS_NAME = "PIMECalculatorWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClass(&wc);

    // Center Window logic
    int w = 400;
    int h = 320; // Increased height for better layout
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 3; // Slightly above center

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // Topmost and no taskbar icon (optional, maybe keep taskbar)
        CLASS_NAME,
        "PIME 计算器",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    // Write HWND to temp file for robust detection by Python
    WriteHwndToTempFile(hWnd);

    if (startHidden) {
        ShowWindow(hWnd, SW_HIDE);
    } else {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
        
        // Force focus
        ForceForegroundWindow(hWnd);
        SetFocus(hEdit);
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return 0;
}
