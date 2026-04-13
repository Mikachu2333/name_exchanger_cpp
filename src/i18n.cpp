#include "i18n.h"

#include <windows.h>

// clang-format off

static const LocaleStrings kSimplifiedChinese = {
    /* file1Label        */  "路径一",
    /* file2Label        */  "路径二",
    /* preserveExtLabel  */  "保留扩展名交换",
    /* swapFullNameLabel */  "完整交换文件名",
    /* startButton       */  "启动",
    /* pinTooltip        */  "窗口置顶",
    /* aboutTooltip      */ L"关于",
    /* adminTooltip      */  "切换管理员权限",
    /* sendToTooltip     */  "左键：添加「发送到」快捷方式\n右键：删除「发送到」快捷方式",
    /* aboutMessageW     */ L"拖入 1 个或 2 个文件/文件夹，或手动输入路径。\n程序可常驻系统托盘，悬停按钮可查看提示。\n\n功能：\n○ 置顶窗口\n○ 切换管理员权限\n○ 创建/删除「发送到」快捷方式\n○ 保留扩展名或完整交换文件名",
    /* trayTooltip       */ L"FilenameExchanger\n左键：显示/隐藏\n右键：退出",
    /* shortcutCreated   */ L"已添加到「发送到」",
    /* shortcutRemoved   */ L"已从「发送到」移除",
    /* tipsTitle         */ L"提示",
    /* errorTitle        */ L"错误",
    /* warningTitle      */ L"警告",
    /* cmdErrorPrefix    */ L"交换失败：",
    /* cmdUsage          */ L"用法：\n  name_exchanger <path1> <path2> [preserve]\n\n参数说明：\n  preserve 为可选参数，默认 true（保留扩展名），可选 false（完整交换文件名）。",
    /* resultSuccess     */  "成功",
    /* resultNoExist     */  "文件或目录不存在",
    /* resultPermDenied  */  "权限不足",
    /* resultAlreadyExists */"目标路径已存在",
    /* resultSameFile    */ "两个路径指向同一项",
    /* resultInvalidPath */ "路径无效",
    /* resultUnknown     */ "未知错误",
};

static const LocaleStrings kTraditionalChinese = {
    /* file1Label        */  "檔案一",
    /* file2Label        */  "檔案二",
    /* preserveExtLabel  */  "保留副檔名",
    /* swapFullNameLabel */  "交換完整檔名",
    /* startButton       */  "啟動",
    /* pinTooltip        */  "置頂開關",
    /* aboutTooltip      */ L"關於",
    /* adminTooltip      */  "以系統管理員執行",
    /* sendToTooltip     */  "點擊新增至右鍵選單「傳送到」選項\n點擊右鍵取消",
    /* aboutMessageW     */ L"拖入檔案即可使用，軟件將常駐任務欄，\n懸停滑鼠於按鍵上可獲得提示。\n\n軟件包含以下功能\n○以系統管理員執行\n○建立/刪除「傳送到」選單快捷方式\n○置頂",
    /* trayTooltip       */ L"FilenameExchanger\n左鍵顯示/隱藏\n右鍵退出",
    /* shortcutCreated   */ L"已建立",
    /* shortcutRemoved   */ L"已刪除",
    /* tipsTitle         */ L"提示",
    /* errorTitle        */ L"錯誤",
    /* warningTitle      */ L"警告",
    /* cmdErrorPrefix    */ L"交換失敗：",
    /* cmdUsage          */ L"用法：\n  name_exchanger <path1> <path2> [preserve]\n\n參數說明：\n  preserve 為可選參數，默認 true（保留副檔名），可選 false（完整交換檔名）。",
    /* resultSuccess     */  "成功",
    /* resultNoExist     */  "檔案不存在",
    /* resultPermDenied  */  "權限不足",
    /* resultAlreadyExists */"目標檔案已存在",
    /* resultSameFile    */ "兩個路徑指向同一檔案",
    /* resultInvalidPath */ "無效路徑",
    /* resultUnknown     */ "未知錯誤",
};

static const LocaleStrings kEnglish = {
    /* file1Label        */  "Path 1",
    /* file2Label        */  "Path 2",
    /* preserveExtLabel  */  "Swap BASE name only",
    /* swapFullNameLabel */  "Swap FULL names",
    /* startButton       */  "Exchange",
    /* pinTooltip        */  "Always on top",
    /* aboutTooltip      */ L"About",
    /* adminTooltip      */  "Toggle administrator mode",
    /* sendToTooltip     */  "Left-click: add a Send To shortcut\nRight-click: remove the Send To shortcut",
    /* aboutMessageW     */ L"Drop one or two files/folders, or type paths manually.\nThe app can stay in the system tray.\nHover toolbar buttons to view tips.\n\nFeatures:\n- Always on top\n- Toggle administrator mode\n- Create/remove Send To shortcut\n- Preserve extensions or swap full names",
    /* trayTooltip       */ L"FilenameExchanger\nLeft-click: Show/Hide\nRight-click: Exit",
    /* shortcutCreated   */ L"Added to Send To",
    /* shortcutRemoved   */ L"Removed from Send To",
    /* tipsTitle         */ L"Info",
    /* errorTitle        */ L"Error",
    /* warningTitle      */ L"Warn",
    /* cmdErrorPrefix    */ L"Exchange failed: ",
    /* cmdUsage          */ L"Usage:\n  name_exchanger <path1> <path2> [preserve]\n\n[preserve] is optional and defaults to true (preserve extensions), you can set it to false (swap full names).",
    /* resultSuccess     */  "Success",
    /* resultNoExist     */  "File or directory does not exist",
    /* resultPermDenied  */  "Permission denied",
    /* resultAlreadyExists */"Target file already exists",
    /* resultSameFile    */  "Both paths refer to the same item",
    /* resultInvalidPath */  "Invalid path",
    /* resultUnknown     */  "Unknown error",
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
