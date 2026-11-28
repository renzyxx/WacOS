#include "shell.h"
#include "installed_apps.h"
#include "installed_apps_text.h"
#include "../comum/semaforo.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <map>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {
namespace words = installed_apps_text;
using words::Get;

struct App {
    std::wstring name, publisher, version, command, productCode;
    std::wstring directory, iconFile;
    int iconIndex = 0;
    HICON icon = nullptr;
    bool triedIcon = false;
    DWORD sizeKiB = 0;
    FILETIME lastLaunch{};
};

enum : int { ID_LIST = 101, ID_REMOVE, ID_OPEN_APPS, ID_EXPLORER, ID_DONE, ID_CLOSE, ID_MINIMIZE, ID_ZOOM };
HWND window, list, removeButton, openAppsButton, explorerButton, doneButton, closeButton;
HWND minimizeButton, zoomButton, hoveredButton;
RECT normalBounds{};
bool zoomed, darkTheme;
unsigned windowGeneration;
HFONT font, headingFont;
HIMAGELIST rowMetrics;
HANDLE uninstallProcess;
std::vector<App> appsList;
std::wstring preferred;
HICON headerIcon;
std::wstring totalSize;

bool LocalPath(const std::wstring& path) {
    return path.size() > 2 && iswalpha(path[0]) && path[1] == L':' && path[2] == L'\\';
}

std::wstring ReadText(HKEY key, const wchar_t* name) {
    DWORD size = 0;
    constexpr DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;
    if (RegGetValueW(key, nullptr, name, flags, nullptr, nullptr, &size) != ERROR_SUCCESS ||
        size < sizeof(wchar_t) || size > 128 * 1024) return {};
    std::wstring value(size / sizeof(wchar_t), L'\0');
    if (RegGetValueW(key, nullptr, name, flags, nullptr, value.data(), &size) != ERROR_SUCCESS) return {};
    value.resize(wcslen(value.c_str()));
    return value;
}

DWORD ReadNumber(HKEY key, const wchar_t* name) {
    DWORD value = 0, size = sizeof(value);
    if (RegGetValueW(key, nullptr, name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return 0;
    return value;
}

void ReadRegistry(HKEY root, REGSAM view) {
    HKEY entries;
    if (RegOpenKeyExW(root, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0,
                      KEY_READ | view, &entries) != ERROR_SUCCESS) return;
    for (DWORD i = 0; ; ++i) {
        wchar_t id[256];
        DWORD length = ARRAYSIZE(id);
        LONG status = RegEnumKeyExW(entries, i, id, &length, nullptr, nullptr, nullptr, nullptr);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status == ERROR_MORE_DATA) continue;
        if (status != ERROR_SUCCESS) break;
        HKEY entry;
        if (RegOpenKeyExW(entries, id, 0, KEY_READ | view, &entry) != ERROR_SUCCESS) continue;
        if (!ReadNumber(entry, L"SystemComponent") && !ReadNumber(entry, L"NoRemove") &&
            ReadText(entry, L"ParentKeyName").empty()) {
            App app;
            app.name = ReadText(entry, L"DisplayName");
            app.publisher = ReadText(entry, L"Publisher");
            app.version = ReadText(entry, L"DisplayVersion");
            app.command = ReadText(entry, L"UninstallString");
            app.directory = ReadText(entry, L"InstallLocation");
            app.iconFile = ReadText(entry, L"DisplayIcon");
            auto comma = app.iconFile.find_last_of(L',');
            if (comma != std::wstring::npos) {
                wchar_t* end = nullptr;
                auto suffix = app.iconFile.c_str() + comma + 1;
                long index = wcstol(suffix, &end, 10);
                if (end != suffix && !*end) {
                    app.iconIndex = index;
                    app.iconFile.resize(comma);
                }
            }
            if (app.iconFile.size() >= 2 && app.iconFile.front() == 34 && app.iconFile.back() == 34)
                app.iconFile = app.iconFile.substr(1, app.iconFile.size()-2);
            if (!LocalPath(app.directory) && LocalPath(app.iconFile))
                app.directory = app.iconFile.substr(0, app.iconFile.find_last_of(wchar_t(92)));
            app.sizeKiB = ReadNumber(entry, L"EstimatedSize");
            GUID product;
            if (ReadNumber(entry, L"WindowsInstaller") && length == 38 && SUCCEEDED(CLSIDFromString(id, &product)))
                app.productCode = id;
            if (!app.name.empty() && (!app.command.empty() || !app.productCode.empty())) {
                auto same = [&](const App& other) {
                    return app.name == other.name && app.version == other.version &&
                           app.publisher == other.publisher && app.command == other.command &&
                           app.productCode == other.productCode;
                };
                if (std::none_of(appsList.begin(), appsList.end(), same)) appsList.push_back(std::move(app));
            }
        }
        RegCloseKey(entry);
    }
    RegCloseKey(entries);
}

struct PathLess {
    bool operator()(const std::wstring& a, const std::wstring& b) const {
        return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    }
};

using LaunchHistory = std::map<std::wstring, FILETIME, PathLess>;

std::wstring KnownFolder(REFKNOWNFOLDERID id) {
    PWSTR path = nullptr;
    std::wstring folder;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DONT_VERIFY, nullptr, &path))) folder = path;
    CoTaskMemFree(path);
    return folder;
}

using KnownFolders = std::map<std::wstring, std::wstring, PathLess>;

std::wstring DecodeHistoryPath(std::wstring value, KnownFolders& folders) {
    for (auto& ch : value) {
        if (ch >= L'A' && ch <= L'Z') ch = L'A' + (ch - L'A' + 13) % 26;
        else if (ch >= L'a' && ch <= L'z') ch = L'a' + (ch - L'a' + 13) % 26;
    }
    if (value.size() > 38 && value.front() == L'{' && value[38] == L'\\') {
        auto id = value.substr(0, 38);
        auto [entry, inserted] = folders.try_emplace(id);
        if (inserted) {
            GUID guid;
            if (SUCCEEDED(CLSIDFromString(id.c_str(), &guid))) entry->second = KnownFolder(guid);
        }
        if (!entry->second.empty()) value.replace(0, 38, entry->second);
    }
    return value;
}

std::wstring ShortcutTarget(const std::wstring& path) {
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)))) return {};
    wchar_t target[MAX_PATH]{};
    IPersistFile* file = nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
        if (SUCCEEDED(file->Load(path.c_str(), STGM_READ)))
            link->GetPath(target, ARRAYSIZE(target), nullptr, SLGP_RAWPATH);
        file->Release();
    }
    link->Release();
    wchar_t expanded[MAX_PATH]{};
    const DWORD length = ExpandEnvironmentStringsW(target, expanded, ARRAYSIZE(expanded));
    if (length && length <= ARRAYSIZE(expanded)) return expanded;
    return target;
}

bool IsUnderDirectory(std::wstring_view path, std::wstring_view directory) {
    while (!directory.empty() && directory.back() == L'\\') directory.remove_suffix(1);
    return !directory.empty() && path.size() > directory.size() && path[directory.size()] == L'\\' &&
        CompareStringOrdinal(path.data(), int(directory.size()), directory.data(), int(directory.size()), TRUE) == CSTR_EQUAL;
}

void AddLaunch(LaunchHistory& history, std::wstring executable, FILETIME time) {
    if (!LocalPath(executable) || _wcsicmp(PathFindExtensionW(executable.c_str()), L".exe")) return;
    auto& previous = history[std::move(executable)];
    if (CompareFileTime(&time, &previous) > 0) previous = time;
}

bool ReadLaunchTime(const BYTE* data, DWORD length, FILETIME now, FILETIME& time) {
    // Windows 7+ UserAssist layout. Restrict reads to this known record version.
    if (length != 72) return false;
    DWORD focusTime = 0;
    std::memcpy(&focusTime, data + 12, sizeof(focusTime));
    std::memcpy(&time, data + 60, sizeof(time));
    return focusTime && (time.dwHighDateTime || time.dwLowDateTime) && CompareFileTime(&time, &now) <= 0;
}

void CollectLaunches(HKEY key, bool shortcuts, LaunchHistory& history, KnownFolders& folders) {
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    const std::wstring programs = shortcuts ? KnownFolder(FOLDERID_Programs) : L"";
    const std::wstring common = shortcuts ? KnownFolder(FOLDERID_CommonPrograms) : L"";
    const std::wstring desktop = shortcuts ? KnownFolder(FOLDERID_Desktop) : L"";
    const std::wstring publicDesktop = shortcuts ? KnownFolder(FOLDERID_PublicDesktop) : L"";
    std::vector<wchar_t> name(16384);
    for (DWORD i = 0; ; ++i) {
        BYTE data[72]{};
        DWORD nameLength = DWORD(name.size()), size = sizeof(data), type = 0;
        const LSTATUS status = RegEnumValueW(key, i, name.data(), &nameLength, nullptr, &type, data, &size);
        if (status == ERROR_MORE_DATA) continue;
        if (status != ERROR_SUCCESS) break;
        FILETIME time{};
        if (type != REG_BINARY || !ReadLaunchTime(data, size, now, time)) continue;
        auto path = DecodeHistoryPath(std::wstring(name.data(), nameLength), folders);
        if (shortcuts) {
            if (!LocalPath(path) || _wcsicmp(PathFindExtensionW(path.c_str()), L".lnk")) continue;
            if (!IsUnderDirectory(path, programs) && !IsUnderDirectory(path, common) &&
                !IsUnderDirectory(path, desktop) && !IsUnderDirectory(path, publicDesktop)) continue;
            path = ShortcutTarget(path);
        }
        AddLaunch(history, std::move(path), time);
    }
}

LaunchHistory ReadLaunchHistory() {
    constexpr wchar_t userAssist[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\";
    const wchar_t* buckets[] = {
        L"{CEBFF5CD-ACE2-4F4F-9178-9926F41749EA}\\Count",
        L"{F4E57C4B-2036-45F0-A9AB-443BCFE33D9F}\\Count",
    };
    LaunchHistory history;
    KnownFolders folders;
    for (int i = 0; i < ARRAYSIZE(buckets); ++i) {
        HKEY key;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, (std::wstring(userAssist) + buckets[i]).c_str(),
                          0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) continue;
        CollectLaunches(key, i == 1, history, folders);
        RegCloseKey(key);
    }
    return history;
}

FILETIME LastLaunch(const App& app, const LaunchHistory& history) {
    FILETIME latest{};
    auto icon = history.find(app.iconFile);
    if (icon != history.end()) latest = icon->second;
    if (!LocalPath(app.directory)) return latest;
    // Avoid assigning launch times of other programs in a shared installation directory.
    for (const auto& [path, time] : history) {
        if (!IsUnderDirectory(path, app.directory)) continue;
        std::wstring executable = PathFindFileNameW(path.c_str());
        executable.resize(executable.size() - 4);
        if (CompareStringOrdinal(app.name.c_str(), -1, executable.c_str(), -1, TRUE) == CSTR_EQUAL &&
            CompareFileTime(&time, &latest) > 0) latest = time;
    }
    return latest;
}

std::wstring FormatLaunch(FILETIME stamp) {
    SYSTEMTIME utc{}, local{};
    if ((!stamp.dwHighDateTime && !stamp.dwLowDateTime) || !FileTimeToSystemTime(&stamp, &utc) ||
        !SystemTimeToTzSpecificLocalTimeEx(nullptr, &utc, &local)) return L"—";
    wchar_t date[64]{}, clock[32]{};
    const auto locale = idioma::Locale();
    if (!GetDateFormatEx(locale, DATE_SHORTDATE, &local, nullptr, date, ARRAYSIZE(date), nullptr) ||
        !GetTimeFormatEx(locale, TIME_NOSECONDS, &local, nullptr, clock, ARRAYSIZE(clock))) return L"—";
    return std::wstring(date) + L" " + clock;
}

std::wstring Lower(std::wstring text) {
    for (auto& ch : text) ch = towlower(ch);
    return text;
}

int Selected() {
    int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    if (row < 0) return -1;
    LVITEMW item{};
    item.mask = LVIF_PARAM;
    item.iItem = row;
    return ListView_GetItem(list, &item) ? (int)item.lParam : -1;
}

void UpdateButtons() {
    const int index = Selected();
    EnableWindow(explorerButton, index >= 0 && size_t(index) < appsList.size() &&
        LocalPath(appsList[index].directory) && PathIsDirectoryW(appsList[index].directory.c_str()));
    if (window) InvalidateRect(window, nullptr, FALSE);
    EnableWindow(removeButton, !uninstallProcess && index >= 0);
}

void FillList() {
    int exactRow = -1, exactCount = 0, row = 0;
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    for (size_t i = 0; i < appsList.size(); ++i) {
        auto& app = appsList[i];
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = row;
        item.pszText = app.name.data();
        item.lParam = (LPARAM)i;
        ListView_InsertItem(list, &item);
        ListView_SetItemText(list, row, 1, const_cast<wchar_t*>(app.productCode.empty() ? L"Windows" : L"Windows Installer"));
        auto lastLaunch = FormatLaunch(app.lastLaunch);
        ListView_SetItemText(list, row, 2, lastLaunch.data());
        wchar_t size[64] = L"—";
        if (app.sizeKiB) StrFormatByteSizeW((LONGLONG)app.sizeKiB * 1024, size, ARRAYSIZE(size));
        ListView_SetItemText(list, row, 3, size);
        if (!preferred.empty() && !_wcsicmp(app.name.c_str(), preferred.c_str())) {
            exactRow = row;
            ++exactCount;
        }
        ++row;
    }
    if (exactCount == 1) {
        ListView_SetItemState(list, exactRow, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(list, exactRow, FALSE);
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, TRUE);
    UpdateButtons();
}

void ReleaseIcons() {
    for (auto& app : appsList) if (app.icon) DestroyIcon(app.icon);
}

void Layout(HWND h);

void Reload() {
    ReleaseIcons();
    appsList.clear();
    for (auto view : { KEY_WOW64_64KEY, KEY_WOW64_32KEY }) {
        ReadRegistry(HKEY_LOCAL_MACHINE, view);
        ReadRegistry(HKEY_CURRENT_USER, view);
    }
    const auto history = ReadLaunchHistory();
    for (auto& app : appsList) app.lastLaunch = LastLaunch(app, history);
    std::stable_sort(appsList.begin(), appsList.end(), [](const App& a, const App& b) {
        if (a.sizeKiB != b.sizeKiB) return a.sizeKiB > b.sizeKiB;
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    LONGLONG bytes = 0;
    for (const auto& app : appsList) bytes += LONGLONG(app.sizeKiB) * 1024;
    wchar_t size[64]{};
    StrFormatByteSizeW(bytes, size, ARRAYSIZE(size));
    totalSize = size;
    FillList();
    Layout(window);
}

bool CommandParts(const std::wstring& command, std::wstring& file, std::wstring& args) {
    const auto start = command.find_first_not_of(L" \t");
    if (start == std::wstring::npos) return false;
    size_t end;
    if (command[start] == L'"') {
        end = command.find(L'"', start + 1);
        if (end == std::wstring::npos) return false;
        file = command.substr(start + 1, end++ - start - 1);
    } else {
        auto lower = Lower(command);
        end = lower.find(L".exe", start);
        while (end != std::wstring::npos && end + 4 < command.size() && !iswspace(command[end + 4]))
            end = lower.find(L".exe", end + 4);
        if (end == std::wstring::npos) return false;
        end += 4;
        file = command.substr(start, end - start);
    }
    if (file.size() < 7 || !iswalpha(file[0]) || file[1] != L':' || file[2] != L'\\' ||
        _wcsicmp(file.c_str() + file.size() - 4, L".exe") || (end < command.size() && !iswspace(command[end]))) return false;
    const DWORD attrs = GetFileAttributesW(file.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) return false;
    args = command.substr(end);
    return true;
}

void RunUninstaller(const std::wstring& file, const std::wstring& args, const std::wstring& icon) {
    SHELLEXECUTEINFOW run{ sizeof(run) };
    run.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    run.hwnd = window;
    run.lpFile = file.c_str();
    run.lpParameters = args.c_str();
    run.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&run)) {
        if (GetLastError() != ERROR_CANCELLED)
            alertas::Prompt(window, Get(words::UninstallError), L"", Get(words::Ok), [] {}, icon, L"");
        return;
    }
    uninstallProcess = run.hProcess;
    if (uninstallProcess) SetTimer(window, 1, 400, nullptr);
    else Reload();
    UpdateButtons();
}

void RemoveSelected() {
    const int index = Selected();
    if (uninstallProcess || index < 0 || (size_t)index >= appsList.size()) return;
    const auto app = appsList[index];
    std::wstring file, args;
    if (!app.productCode.empty()) {
        wchar_t system[MAX_PATH];
        UINT size = GetSystemDirectoryW(system, ARRAYSIZE(system));
        if (size && size < ARRAYSIZE(system)) {
            file = std::wstring(system) + L"\\msiexec.exe";
            args = L"/x " + app.productCode;
        }
    } else if (!CommandParts(app.command, file, args)) {
        file.clear();
    }
    const auto icon = LocalPath(app.iconFile) ? app.iconFile : L"pack:App Store";
    const auto generation = windowGeneration;
    if (file.empty()) {
        alertas::Prompt(window, app.name, Get(words::WindowsFallback), Get(words::OpenSettings),
            [generation] {
                if (window && generation == windowGeneration)
                    ShellExecuteW(window, L"open", L"ms-settings:appsfeatures", nullptr, nullptr, SW_SHOWNORMAL);
            }, icon, Get(words::Cancel));
        return;
    }
    std::wstring question = Get(words::ConfirmUninstall);
    question.replace(question.find(L"%ls"), 3, app.name);
    auto details = app.publisher;
    if (!app.version.empty()) details += (details.empty() ? L"" : L"\n") + app.version;
    alertas::Prompt(window, question, details, Get(words::Delete),
        [generation, file, args, icon] {
            if (window && generation == windowGeneration && !uninstallProcess)
                RunUninstaller(file, args, icon);
        }, icon, Get(words::Cancel));
}

struct Palette {
    COLORREF background, panel, stripe, field, text, muted, border, accent, pressed;
};

Palette Colors() {
    if (darkTheme) return { RGB(31,32,34), RGB(38,39,41), RGB(44,45,47), RGB(52,53,55),
        RGB(239,239,241), RGB(166,168,172), RGB(59,60,63), RGB(0,102,209), RGB(0,84,181) };
    return { RGB(246,246,248), RGB(255,255,255), RGB(245,245,247), RGB(255,255,255),
        RGB(30,30,32), RGB(96,98,104), RGB(219,220,223), RGB(0,102,209), RGB(0,84,181) };
}

int Px(int value) { return MulDiv(value, GetDpiForWindow(window), 96); }

void Fill(HDC dc, const RECT& rect, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
}

void Rounded(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF edge) {
    auto pen = SelectObject(dc, GetStockObject(DC_PEN));
    auto brush = SelectObject(dc, GetStockObject(DC_BRUSH));
    SetDCPenColor(dc, edge);
    SetDCBrushColor(dc, fill);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, pen);
    SelectObject(dc, brush);
}

void Label(HDC dc, std::wstring_view value, RECT rect, COLORREF color, UINT align = DT_LEFT) {
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, value.data(), (int)value.size(), &rect, align | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

const wchar_t* ColumnName(int index) {
    constexpr words::Message columns[]{words::Name, words::Kind, words::LastLaunch, words::Size};
    return Get(columns[index]);
}

void PaintWindow(HWND h) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(h, &paint);
    RECT area{};
    GetClientRect(h, &area);
    const auto theme = Colors();
    Fill(dc, area, theme.background);
    const auto saved = SelectObject(dc, headingFont);
    Fill(dc, { Px(20), Px(39), area.right - Px(20), Px(39) + 1 }, theme.border);
    if (headerIcon) DrawIconEx(dc,Px(22),Px(53),headerIcon,Px(30),Px(30),0,nullptr,DI_NORMAL);
    auto title = std::wstring(Get(words::Applications)) + L" — " + totalSize;
    Label(dc,title,{Px(64),Px(48),area.right-Px(220),Px(70)},theme.text);
    SelectObject(dc,font);
    Label(dc,Get(words::Description),{Px(64),Px(69),area.right-Px(24),Px(89)},theme.muted);
    int bottom = area.bottom - Px(111);
    Rounded(dc,{Px(20),Px(96),area.right-Px(20),bottom},Px(7),theme.panel,theme.border);
    int x = Px(21);
    for (int i=0;i<4;++i) {
        int width = ListView_GetColumnWidth(list,i);
        std::wstring caption = ColumnName(i);
        if (i == 3) caption += L"  ⌄";
        Label(dc,caption,{x+Px(8),Px(97),x+width-Px(6),Px(122)},theme.text,i==3 ? DT_RIGHT : DT_LEFT);
        x += width;
        if(i<3) Fill(dc,{x,Px(102),x+1,Px(117)},theme.border);
    }
    Fill(dc,{Px(21),Px(122),area.right-Px(21),Px(123)},theme.border);
    RECT path{Px(21),bottom-Px(22),area.right-Px(21),bottom-1};
    Fill(dc,path,theme.stripe);
    path.left+=Px(7);
    int index=Selected();
    std::wstring crumb = Get(words::Applications);
    if(index>=0 && size_t(index)<appsList.size()) crumb += L"  ›  " + appsList[index].name;
    Label(dc,crumb,path,theme.muted);
    Fill(dc,{0,area.bottom-Px(61),area.right,area.bottom-Px(61)+1},theme.border);
    SelectObject(dc, saved);
    EndPaint(h, &paint);
}

LRESULT PaintRow(NMLVCUSTOMDRAW* draw) {
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
    if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) return CDRF_DODEFAULT;
    const int row = (int)draw->nmcd.dwItemSpec;
    RECT line{}, client{};
    if (!ListView_GetItemRect(list, row, &line, LVIR_BOUNDS)) return CDRF_DODEFAULT;
    GetClientRect(list, &client);
    line.left = 0;
    line.right = client.right;
    HDC dc = draw->nmcd.hdc;
    const auto theme = Colors();
    const bool selected = (ListView_GetItemState(list, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    Fill(dc, line, row % 2 ? theme.stripe : theme.panel);
    if (selected) {
        RECT selection{line.left + Px(3), line.top + Px(1), line.right - Px(3), line.bottom - Px(1)};
        Rounded(dc, selection, Px(8), theme.accent, theme.accent);
    }
    LVITEMW item{};
    item.mask=LVIF_PARAM;
    item.iItem=row;
    if (ListView_GetItem(list,&item) && size_t(item.lParam)<appsList.size()) {
        auto& app=appsList[item.lParam];
        if (!app.triedIcon) {
            app.triedIcon=true;
            if (LocalPath(app.iconFile)) ExtractIconExW(app.iconFile.c_str(),app.iconIndex,nullptr,&app.icon,1);
        }
        HICON icon=app.icon ? app.icon : LoadIconW(nullptr,IDI_APPLICATION);
        DrawIconEx(dc,Px(9),(line.top+line.bottom-Px(16))/2,icon,Px(16),Px(16),0,nullptr,DI_NORMAL);
    }
    const auto prev = SelectObject(dc, font);
    int x = 0;
    for (int i = 0; i < 4; ++i) {
        wchar_t cell[512]{};
        ListView_GetItemText(list, row, i, cell, ARRAYSIZE(cell));
        const int width = ListView_GetColumnWidth(list, i);
        Label(dc, cell, { x + Px(i==0 ? 32 : 8), line.top, x + width - Px(6), line.bottom },
              selected ? RGB(255,255,255) : i == 0 ? theme.text : theme.muted,
              i == 3 ? DT_RIGHT : DT_LEFT);
        x += width;
    }
    SelectObject(dc, prev);
    return CDRF_SKIPDEFAULT;
}

void PaintButton(const DRAWITEMSTRUCT& draw) {
    const auto theme = Colors();
    const bool primary = draw.CtlID == ID_DONE && !(draw.itemState & ODS_DISABLED);
    const bool pressed = (draw.itemState & ODS_SELECTED) != 0;
    const bool hover = hoveredButton == draw.hwndItem;
    const COLORREF fill = primary ? (pressed ? theme.pressed : theme.accent) :
                         (pressed || hover ? theme.stripe : theme.field);
    Fill(draw.hDC, draw.rcItem, theme.background);
    if (draw.CtlID >= ID_CLOSE && draw.CtlID <= ID_ZOOM) {
        static const COLORREF normal[]{ RGB(255,95,86), RGB(255,189,46), RGB(39,201,63) };
        static const COLORREF down[]{ RGB(218,75,68), RGB(220,157,35), RGB(29,168,50) };
        const auto color = semaforo::Grafite() ? theme.muted : (pressed ? down : normal)[draw.CtlID - ID_CLOSE];
        RECT dot{Px(3), Px(3), Px(17), Px(17)};
        Rounded(draw.hDC, dot, Px(14), color, color);
        if (hover || (draw.itemState & ODS_FOCUS)) {
            const wchar_t* glyph = draw.CtlID == ID_CLOSE ? L"×" : draw.CtlID == ID_MINIMIZE ? L"−" : zoomed ? L"↙" : L"↗";
            const auto previous = SelectObject(draw.hDC, font);
            Label(draw.hDC, glyph, dot, RGB(48,44,44), DT_CENTER);
            SelectObject(draw.hDC, previous);
        }
        return;
    }
    Rounded(draw.hDC, draw.rcItem, Px(12), fill, primary ? fill : theme.border);
    wchar_t text[128]{};
    GetWindowTextW(draw.hwndItem, text, ARRAYSIZE(text));
    const auto prev = SelectObject(draw.hDC, font);
    Label(draw.hDC, text, draw.rcItem, primary ? RGB(255,255,255) :
          (draw.itemState & ODS_DISABLED ? theme.muted : theme.text), DT_CENTER);
    SelectObject(draw.hDC, prev);
    if (draw.itemState & ODS_FOCUS) {
        RECT focus = draw.rcItem;
        InflateRect(&focus, -Px(4), -Px(4));
        DrawFocusRect(draw.hDC, &focus);
    }
}

void SetMetrics() {
    if (font) DeleteObject(font);
    if (headingFont) DeleteObject(headingFont);
    const auto family = ui::family.empty() ? L"Segoe UI" : ui::family.c_str();
    font = CreateFontW(-Px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, family);
    headingFont = CreateFontW(-Px(16), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, family);
    for (auto control : { list, removeButton, openAppsButton, explorerButton, doneButton, closeButton, minimizeButton, zoomButton })
        SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
    auto image = ImageList_Create(1, Px(22), ILC_COLOR32, 1, 1);
    ListView_SetImageList(list, image, LVSIL_SMALL);
    if (rowMetrics) ImageList_Destroy(rowMetrics);
    rowMetrics = image;
}

void Layout(HWND h) {
    RECT area{};
    GetClientRect(h, &area);
    int width = area.right, height = area.bottom;
    MoveWindow(closeButton,Px(17),Px(10),Px(20),Px(20),TRUE);
    MoveWindow(minimizeButton,Px(39),Px(10),Px(20),Px(20),TRUE);
    MoveWindow(zoomButton,Px(61),Px(10),Px(20),Px(20),TRUE);
    MoveWindow(openAppsButton,width-Px(214),Px(49),Px(194),Px(27),TRUE);
    MoveWindow(list,Px(21),Px(123),width-Px(42),height-Px(257),TRUE);
    MoveWindow(explorerButton,Px(20),height-Px(102),Px(194),Px(27),TRUE);
    MoveWindow(removeButton,width-Px(123),height-Px(102),Px(103),Px(27),TRUE);
    MoveWindow(doneButton,width-Px(108),height-Px(43),Px(88),Px(27),TRUE);
    RECT client{};
    GetClientRect(list, &client);
    const int available = (std::min)(int(client.right), width - Px(42) - GetSystemMetricsForDpi(SM_CXVSCROLL,GetDpiForWindow(h)));
    const int kind = Px(105);
    const int lastLaunch = Px(159);
    const int size = Px(91);
    ListView_SetColumnWidth(list, 0, available - kind - lastLaunch - size);
    ListView_SetColumnWidth(list, 1, kind);
    ListView_SetColumnWidth(list, 2, lastLaunch);
    ListView_SetColumnWidth(list, 3, size);
    InvalidateRect(h, nullptr, FALSE);
}

void ToggleZoom(HWND h) {
    RECT bounds = normalBounds;
    if (!zoomed) {
        MONITORINFO monitor{sizeof(monitor)};
        if (!GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &monitor)) return;
        GetWindowRect(h, &normalBounds);
        bounds = monitor.rcWork;
    }
    zoomed = !zoomed;
    SetWindowTextW(zoomButton, Get(zoomed ? words::Restore : words::Zoom));
    SetWindowPos(h, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void ApplyTheme(HWND h) {
    darkTheme = ui::Dark();
    const BOOL dark = darkTheme;
    const auto theme = Colors();
    DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    DwmSetWindowAttribute(h, DWMWA_BORDER_COLOR, &theme.border, sizeof(theme.border));
    if (list) {
        SetWindowTheme(list, darkTheme ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        ListView_SetBkColor(list, theme.panel);
        ListView_SetTextBkColor(list, theme.panel);
        InvalidateRect(list, nullptr, TRUE);
    }
    RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK HandlePanelKeys(HWND child, UINT message, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR) {
    if (message == WM_KEYDOWN && w == VK_ESCAPE) {
        PostMessageW(GetParent(child), WM_CLOSE, 0, 0);
        return 0;
    }
    if (message == WM_KEYDOWN && w == VK_TAB) {
        HWND next = GetNextDlgTabItem(GetParent(child), child, GetKeyState(VK_SHIFT) < 0);
        if (next) SetFocus(next);
        return 0;
    }
    if (id == 2 && message == WM_MOUSEMOVE && hoveredButton != child) {
        HWND previous = hoveredButton;
        hoveredButton = child;
        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, child, 0};
        TrackMouseEvent(&track);
        if (previous) InvalidateRect(previous, nullptr, FALSE);
        InvalidateRect(child, nullptr, FALSE);
    }
    if (id == 2 && message == WM_MOUSELEAVE && hoveredButton == child) {
        hoveredButton = nullptr;
        InvalidateRect(child, nullptr, FALSE);
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(child, HandlePanelKeys, id);
    return DefSubclassProc(child, message, w, l);
}

LRESULT CALLBACK Proc(HWND h, UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_NCHITTEST: {
        POINT pt{short(LOWORD(l)),short(HIWORD(l))};
        ScreenToClient(h,&pt);
        if (window && pt.y >= 0 && pt.y < Px(90) && ChildWindowFromPointEx(h, pt, CWP_SKIPINVISIBLE) == h)
            return HTCAPTION;
        break;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: PaintWindow(h); return 0;
    case WM_DRAWITEM: PaintButton(*reinterpret_cast<const DRAWITEMSTRUCT*>(l)); return TRUE;
    case WM_ACTIVATE:
        if (window && LOWORD(w) != WA_INACTIVE && ui::Dark() != darkTheme) ApplyTheme(h);
        break;
    case WM_SETTINGCHANGE:
        if (window) ApplyTheme(h);
        break;
    case WM_NCLBUTTONDBLCLK:
        if (w == HTCAPTION) { ToggleZoom(h); return 0; }
        break;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(l);
        info->ptMinTrackSize = { (LONG)(590 * ui::Scale(h)), (LONG)(350 * ui::Scale(h)) };
        return 0;
    }
    case WM_SIZE:
        if (list && w != SIZE_MINIMIZED) Layout(h);
        return 0;
    case WM_DPICHANGED: {
        if (list) SetMetrics();
        const auto* rect = reinterpret_cast<const RECT*>(l);
        SetWindowPos(h, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == ID_REMOVE) RemoveSelected();
        if (LOWORD(w) == ID_OPEN_APPS) ShellExecuteW(h,L"open",L"shell:AppsFolder",nullptr,nullptr,SW_SHOWNORMAL);
        if (LOWORD(w) == ID_DONE || LOWORD(w) == ID_CLOSE) PostMessageW(h, WM_CLOSE, 0, 0);
        if (LOWORD(w) == ID_MINIMIZE) ShowWindow(h, SW_MINIMIZE);
        if (LOWORD(w) == ID_ZOOM) ToggleZoom(h);
        if (LOWORD(w) == ID_EXPLORER && IsWindowEnabled(explorerButton)) {
            int index=Selected();
            if(index>=0 && size_t(index)<appsList.size()) ShellExecuteW(h,L"explore",appsList[index].directory.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        }
        return 0;
    case WM_NOTIFY:
        if (reinterpret_cast<const NMHDR*>(l)->hwndFrom == list) {
            const auto code = reinterpret_cast<const NMHDR*>(l)->code;
            if (code == NM_CUSTOMDRAW) return PaintRow(reinterpret_cast<NMLVCUSTOMDRAW*>(l));
            if (code == LVN_ITEMCHANGED) UpdateButtons();
        }
        return 0;
    case WM_TIMER:
        if (uninstallProcess && WaitForSingleObject(uninstallProcess, 0) != WAIT_TIMEOUT) {
            KillTimer(h, 1);
            CloseHandle(uninstallProcess);
            uninstallProcess = nullptr;
            Reload();
        }
        return 0;
    case WM_DESTROY:
        if (uninstallProcess) CloseHandle(uninstallProcess);
        uninstallProcess = nullptr;
        if (font) DeleteObject(font);
        if (headingFont) DeleteObject(headingFont);
        if (rowMetrics) ImageList_Destroy(rowMetrics);
        font = headingFont = nullptr;
        rowMetrics = nullptr;
        window = list = removeButton = openAppsButton = explorerButton = doneButton = closeButton = nullptr;
        minimizeButton = zoomButton = hoveredButton = nullptr;
        zoomed = false;
        ReleaseIcons();
        if(headerIcon) DestroyIcon(headerIcon);
        headerIcon=nullptr;
        appsList.clear();
        return 0;
    }
    return DefWindowProcW(h, message, w, l);
}
}

const wchar_t* installed_apps::MenuLabel() {
    return Get(words::UninstallApp);
}

void installed_apps::Open(const std::wstring& preferredName) {
    preferred = preferredName;
    if (!window) {
        INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&controls);
        WNDCLASSW wc{};
        wc.lpfnWndProc = Proc;
        wc.style = CS_DBLCLKS;
        wc.hInstance = shell::inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"WacOSInstalledApps";
        RegisterClassW(&wc);
        auto title = Get(words::Title);
        const UINT dpi = GetDpiForSystem();
        window = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, title, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
                                0, 0, MulDiv(650, dpi, 96), MulDiv(482, dpi, 96),
                                nullptr, nullptr, shell::inst, nullptr);
        if (!window) return;
        ++windowGeneration;
        POINT cursor{};
        GetCursorPos(&cursor);
        MONITORINFO mon{sizeof(mon)};
        GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mon);
        RECT bounds{};
        GetWindowRect(window,&bounds);
        SetWindowPos(window,nullptr,(mon.rcWork.left+mon.rcWork.right-bounds.right)/2,
            (mon.rcWork.top+mon.rcWork.bottom-bounds.bottom)/2,0,0,SWP_NOSIZE|SWP_NOZORDER);
        DWM_WINDOW_CORNER_PREFERENCE corners=DWMWCP_ROUND;
        DwmSetWindowAttribute(window,DWMWA_WINDOW_CORNER_PREFERENCE,&corners,sizeof(corners));
        SHSTOCKICONINFO icon{sizeof(icon)};
        if (SUCCEEDED(SHGetStockIconInfo(SIID_APPLICATION, SHGSI_ICON, &icon))) headerIcon = icon.hIcon;
        auto child = [&](const wchar_t* cls, const wchar_t* label, DWORD style, int id) {
            HWND control = CreateWindowExW(0, cls, label, WS_CHILD | WS_VISIBLE | style,
                                           0, 0, 0, 0, window, (HMENU)(INT_PTR)id, shell::inst, nullptr);
            SetWindowSubclass(control, HandlePanelKeys, id == ID_LIST ? 1 : 2, 0);
            return control;
        };
        closeButton = child(WC_BUTTONW, Get(words::Close), BS_OWNERDRAW | WS_TABSTOP, ID_CLOSE);
        minimizeButton = child(WC_BUTTONW, Get(words::Minimize), BS_OWNERDRAW | WS_TABSTOP, ID_MINIMIZE);
        zoomButton = child(WC_BUTTONW, Get(words::Zoom), BS_OWNERDRAW | WS_TABSTOP, ID_ZOOM);
        openAppsButton = child(WC_BUTTONW, Get(words::OpenApplications), BS_OWNERDRAW | WS_TABSTOP, ID_OPEN_APPS);
        list = child(WC_LISTVIEWW, L"", LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP, ID_LIST);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        for (int i = 0; i < 4; ++i) {
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_FMT;
            column.pszText = const_cast<wchar_t*>(ColumnName(i));
            column.fmt = i == 3 ? LVCFMT_RIGHT : LVCFMT_LEFT;
            ListView_InsertColumn(list, i, &column);
        }
        explorerButton = child(WC_BUTTONW, Get(words::ShowInExplorer), BS_OWNERDRAW | WS_TABSTOP, ID_EXPLORER);
        removeButton = child(WC_BUTTONW, Get(words::Delete), BS_OWNERDRAW | WS_TABSTOP, ID_REMOVE);
        doneButton = child(WC_BUTTONW, Get(words::Done), BS_OWNERDRAW | WS_TABSTOP, ID_DONE);
        SetMetrics();
    }
    SetWindowTextW(window, Get(words::Title));
    SetWindowTextW(closeButton, Get(words::Close));
    SetWindowTextW(minimizeButton, Get(words::Minimize));
    SetWindowTextW(zoomButton, Get(zoomed ? words::Restore : words::Zoom));
    SetWindowTextW(openAppsButton, Get(words::OpenApplications));
    SetWindowTextW(explorerButton, Get(words::ShowInExplorer));
    SetWindowTextW(removeButton, Get(words::Delete));
    SetWindowTextW(doneButton, Get(words::Done));
    ApplyTheme(window);
    Reload();
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
    SetFocus(list);
}
