#include "i18n.h"

#include <windows.h>

// clang-format off

static const LocaleStrings kSimplifiedChinese = {
    /* windowTitle       */ "FilenameExchanger",
    /* file1Label        */ "文件一",
    /* file2Label        */ "文件二",
    /* startButton       */ "启动",
    /* pinTooltip        */ "置顶开关",
    /* aboutTooltip      */ L"关于",
    /* adminTooltip      */ "以管理员身份启动",
    /* sendToTooltip     */ "单击可新增右键菜单「发送到」选项\n右键点击以删除",
    /* aboutMessageW     */ L"拖入文件即可使用，软件将常驻任务栏，\n悬停鼠标于按钮上可获得提示。\n\n软件包含以下功能\n○管理员身份启动\n○创建/删除「发送到」菜单快捷方式\n○置顶",
    /* trayTooltip       */ L"FilenameExchanger\n左键显示/隐藏\n右键退出",
    /* shortcutCreated   */ L"已创建",
    /* shortcutRemoved   */ L"已删除",
    /* tipsTitle         */ L"提示",
    /* errorTitle        */ L"错误",
    /* warningTitle      */ L"警告",
    /* resultSuccess     */ "成功",
    /* resultNoExist     */ "文件不存在",
    /* resultPermDenied  */ "权限不足",
    /* resultAlreadyExists */ "目标文件已存在",
    /* resultSameFile    */ "两个路径指向同一文件",
    /* resultInvalidPath */ "无效路径",
    /* resultUnknown     */ "未知错误",
};

static const LocaleStrings kTraditionalChinese = {
    /* windowTitle       */ "FilenameExchanger",
    /* file1Label        */ "檔案一",
    /* file2Label        */ "檔案二",
    /* startButton       */ "啟動",
    /* pinTooltip        */ "置頂開關",
    /* aboutTooltip      */ L"關於",
    /* adminTooltip      */ "以系統管理員執行",
    /* sendToTooltip     */ "點擊新增至右鍵選單「傳送到」選項\n點擊右鍵取消",
    /* aboutMessageW     */ L"拖入檔案即可使用，軟件將常駐任務欄，\n懸停滑鼠於按鍵上可獲得提示。\n\n軟件包含以下功能\n○系統管理員身份啟動\n○建立/刪除「傳送到」選單快捷方式\n○置頂",
    /* trayTooltip       */ L"FilenameExchanger\n左鍵顯示/隱藏\n右鍵退出",
    /* shortcutCreated   */ L"已建立",
    /* shortcutRemoved   */ L"已刪除",
    /* tipsTitle         */ L"提示",
    /* errorTitle        */ L"錯誤",
    /* warningTitle      */ L"警告",
    /* resultSuccess     */ "成功",
    /* resultNoExist     */ "檔案不存在",
    /* resultPermDenied  */ "權限不足",
    /* resultAlreadyExists */ "目標檔案已存在",
    /* resultSameFile    */ "兩個路徑指向同一檔案",
    /* resultInvalidPath */ "無效路徑",
    /* resultUnknown     */ "未知錯誤",
};

static const LocaleStrings kEnglish = {
    /* windowTitle       */ "FilenameExchanger",
    /* file1Label        */ "File 1",
    /* file2Label        */ "File 2",
    /* startButton       */ "Exchange",
    /* pinTooltip        */ "Toggle Always on Top",
    /* aboutTooltip      */ L"About",
    /* adminTooltip      */ "Run as Administrator",
    /* sendToTooltip     */ "Click to add to 'Send To' context menu\nRight-click to remove",
    /* aboutMessageW     */ L"Drag and drop files to use.\nThe application stays in the system tray.\nHover over buttons for tooltips.\n\nFeatures:\n· Run as Administrator\n· Create/Remove 'Send To' shortcut\n· Always on Top",
    /* trayTooltip       */ L"FilenameExchanger\nLeft-click: Show/Hide\nRight-click: Exit",
    /* shortcutCreated   */ L"Shortcut created",
    /* shortcutRemoved   */ L"Shortcut removed",
    /* tipsTitle         */ L"Info",
    /* errorTitle        */ L"Error",
    /* warningTitle      */ L"Warning",
    /* resultSuccess     */ "Success",
    /* resultNoExist     */ "File does not exist",
    /* resultPermDenied  */ "Permission denied",
    /* resultAlreadyExists */ "Target file already exists",
    /* resultSameFile    */ "Both paths refer to the same file",
    /* resultInvalidPath */ "Invalid path (e.g. non-UTF-8)",
    /* resultUnknown     */ "Unknown error",
};

// clang-format on

Language DetectSystemLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLang = PRIMARYLANGID(langId);
    WORD subLang = SUBLANGID(langId);

    if (primaryLang == LANG_CHINESE) {
        // Simplified Chinese: mainland China, Singapore
        if (subLang == SUBLANG_CHINESE_SIMPLIFIED || subLang == SUBLANG_CHINESE_SINGAPORE) {
            return Language::SimplifiedChinese;
        }
        // Traditional Chinese: Taiwan, Hong Kong, Macau
        return Language::TraditionalChinese;
    }
    return Language::English;
}

const LocaleStrings& GetLocaleStrings(Language lang) {
    switch (lang) {
        case Language::SimplifiedChinese:
            return kSimplifiedChinese;
        case Language::TraditionalChinese:
            return kTraditionalChinese;
        case Language::English:
        default:
            return kEnglish;
    }
}

const LocaleStrings& GetCurrentLocale() {
    static Language cachedLang = DetectSystemLanguage();
    return GetLocaleStrings(cachedLang);
}

const char* GetOutputInfo(int id) {
    const auto& locale = GetCurrentLocale();
    switch (id) {
        case 0:
            return locale.resultSuccess;
        case 1:
            return locale.resultNoExist;
        case 2:
            return locale.resultPermissionDenied;
        case 3:
            return locale.resultAlreadyExists;
        case 4:
            return locale.resultSameFile;
        case 5:
            return locale.resultInvalidPath;
        default:
            return locale.resultUnknown;
    }
}
