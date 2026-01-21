#include "SettingsDialog.h"
#include "resource.h"
#include "Utils.h"
#include <commdlg.h>
#include <string>
#include <windowsx.h>

namespace PIME {

struct Settings {
    int fontSize = 12;
    COLORREF textColor = RGB(0, 0, 0);
    COLORREF bkColor = RGB(255, 255, 255);
    bool horizontal = true;
};

static Settings g_settings;
static std::wstring GetSettingsFilePath() {
    return getAppLocalDir() + L"\\PIME\\PIMESettings.json";
}

static void LoadSettings(Settings& s) {
    Json::Value root;
    if (loadJsonFile(GetSettingsFilePath(), root)) {
        s.fontSize = root.get("fontSize", 12).asInt();
        s.textColor = (COLORREF)root.get("textColor", (Json::UInt)RGB(0, 0, 0)).asUInt();
        s.bkColor = (COLORREF)root.get("bkColor", (Json::UInt)RGB(255, 255, 255)).asUInt();
        s.horizontal = root.get("horizontal", true).asBool();
    }
}

static void SaveSettings(const Settings& s) {
    Json::Value root;
    root["fontSize"] = s.fontSize;
    root["textColor"] = (Json::UInt)s.textColor;
    root["bkColor"] = (Json::UInt)s.bkColor;
    root["horizontal"] = s.horizontal;
    saveJsonFile(GetSettingsFilePath(), root);
}

static void ChooseColorHelper(HWND hwnd, COLORREF& color) {
    CHOOSECOLOR cc = { 0 };
    static COLORREF customColors[16] = { 0 };
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = customColors;
    cc.rgbResult = color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) {
        color = cc.rgbResult;
    }
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        LoadSettings(g_settings);
        SetDlgItemInt(hwnd, IDC_EDIT_FONT_SIZE, g_settings.fontSize, FALSE);
        Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_HORIZONTAL), g_settings.horizontal ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wp)) {
        case IDC_BTN_TEXT_COLOR:
            ChooseColorHelper(hwnd, g_settings.textColor);
            break;
        case IDC_BTN_BK_COLOR:
            ChooseColorHelper(hwnd, g_settings.bkColor);
            break;
        case IDOK: {
            BOOL translated;
            int fontSize = GetDlgItemInt(hwnd, IDC_EDIT_FONT_SIZE, &translated, FALSE);
            if (translated && fontSize > 0) {
                g_settings.fontSize = fontSize;
            }
            g_settings.horizontal = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_HORIZONTAL)) == BST_CHECKED);
            SaveSettings(g_settings);
            EndDialog(hwnd, IDOK);
            break;
        }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;
    }
    }
    return FALSE;
}

void ShowSettingsDialog(HINSTANCE hInst, HWND parent) {
    DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), parent, DialogProc);
}

}
