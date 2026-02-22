#pragma once

// Supported UI languages
enum class Language {
    SimplifiedChinese,
    TraditionalChinese,
    English,
};

// All localizable strings used in the UI
struct LocaleStrings {
    const char* file1Label;
    const char* file2Label;
    const char* startButton;
    const char* pinTooltip;
    const wchar_t* aboutTooltip;
    const char* adminTooltip;
    const char* sendToTooltip;
    const wchar_t* aboutMessageW;
    const wchar_t* trayTooltip;
    const wchar_t* shortcutCreated;
    const wchar_t* shortcutRemoved;
    const wchar_t* tipsTitle;
    const wchar_t* errorTitle;
    const wchar_t* warningTitle;

    // Result messages
    const char* resultSuccess;
    const char* resultNoExist;
    const char* resultPermissionDenied;
    const char* resultAlreadyExists;
    const char* resultSameFile;
    const char* resultInvalidPath;
    const char* resultUnknown;
};

// Detect the system UI language and return the appropriate Language enum
Language DetectSystemLanguage();

// Get the localized strings for the given language
const LocaleStrings& GetLocaleStrings(Language lang);

// Get the localized strings for the detected system language (cached)
const LocaleStrings& GetCurrentLocale();

// Get result message by exchange() return code
const char* GetOutputInfo(int id);
