#pragma once
// Pastas na nuvem no Windows 11 (OneDrive pessoal, OneDrive corporativo / múltiplas contas, iCloud Drive, Dropbox...):
//  1. O filtro de arquivos na nuvem do Windows (cldflt.sys) marca a raiz ativa do OneDrive e cada subpasta sincronizada
//     com FILE_ATTRIBUTE_REPARSE_POINT e IO_REPARSE_TAG_CLOUD..CLOUD_F (0x9000001A..0x9000F01A). Diferente de junções e
//     symlinks, elas são pastas reais no lugar (sem desvio nem risco de laço) — pular todo reparse point deixava o
//     OneDrive ativo inteiro fora do índice do Spotlight, do vigia de mudanças, da busca e do "Obter Informações".
//  2. O FOLDERID_SkyDrive só devolve a pasta do OneDrive pessoal (%USERPROFILE%\OneDrive), ignorando contas corporativas
//     e de ensino (HKCU\Software\Microsoft\OneDrive\Accounts\*\UserFolder, OneDriveCommercial e "OneDrive - *").
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

namespace nuvem {

inline bool TagNuvem(DWORD tag) {
    return (tag & ~0x0000F000u) == 0x9000001Au /* IO_REPARSE_TAG_CLOUD..CLOUD_F */ ||
           tag == 0x80000021u /* IO_REPARSE_TAG_ONEDRIVE */;
}

// Reparse point que redireciona para outro caminho (junção, symlink, ponto de montagem): evita voltas ao varrer pastas.
// Pastas do filtro de nuvem (TagNuvem) devolvem false para entrarem normalmente na varredura.
inline bool ReparseDeDesvio(const WIN32_FIND_DATAW& fd) {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    return !TagNuvem(fd.dwReserved0);
}

// Arquivo desidratado ("só na nuvem", Files On-Demand): não ocupa blocos no disco local e abri-lo sem
// FILE_FLAG_OPEN_REPARSE_POINT dispara o download completo pelo cldflt.sys.
inline bool SoNaNuvem(DWORD attr) {
    return (attr & (FILE_ATTRIBUTE_OFFLINE | 0x00400000u /*FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS*/ |
                    0x00040000u /*FILE_ATTRIBUTE_RECALL_ON_OPEN*/)) != 0;
}

struct Conta {
    std::wstring rotulo;
    std::wstring caminho;
};

inline std::vector<Conta> PastasOneDrive() {
    std::vector<Conta> out;
    auto limpar = [](std::wstring p) {
        while (p.size() > 3 && (p.back() == L'\\' || p.back() == L'/')) p.pop_back();
        return p;
    };
    auto nomeFolha = [](const std::wstring& p) {
        size_t b = p.find_last_of(L"\\/");
        return b == std::wstring::npos ? p : p.substr(b + 1);
    };
    auto adicionar = [&](std::wstring p, std::wstring rotulo) {
        p = limpar(std::move(p));
        if (p.empty()) return;
        DWORD a = GetFileAttributesW(p.c_str());
        if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) return;
        for (auto& e : out) if (!_wcsicmp(e.caminho.c_str(), p.c_str())) return;
        if (rotulo.empty()) {
            std::wstring folha = nomeFolha(p);
            rotulo = !_wcsnicmp(folha.c_str(), L"OneDrive", 8) ? folha : L"OneDrive";
        }
        out.push_back({ std::move(rotulo), std::move(p) });
    };

    // 1. Contas configuradas no cliente do OneDrive (Personal, Business1, Business2...)
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\OneDrive\\Accounts", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        wchar_t sub[128];
        for (DWORD idx = 0;; idx++) {
            DWORD subLen = 128;
            if (RegEnumKeyExW(hk, idx, sub, &subLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            HKEY ak = nullptr;
            if (RegOpenKeyExW(hk, sub, 0, KEY_READ, &ak) != ERROR_SUCCESS) continue;
            wchar_t pasta[MAX_PATH] = {}, disp[256] = {};
            DWORD cbPasta = sizeof(pasta), cbDisp = sizeof(disp), tp = 0, bus = 0, cbBus = sizeof(bus);
            bool temPasta = RegQueryValueExW(ak, L"UserFolder", nullptr, &tp, (BYTE*)pasta, &cbPasta) == ERROR_SUCCESS &&
                            (tp == REG_SZ || tp == REG_EXPAND_SZ) && pasta[0];
            if (temPasta) {
                RegQueryValueExW(ak, L"DisplayName", nullptr, &tp, (BYTE*)disp, &cbDisp);
                RegQueryValueExW(ak, L"Business", nullptr, &tp, (BYTE*)&bus, &cbBus);
                std::wstring rot;
                if (bus && disp[0]) rot = std::wstring(L"OneDrive - ") + disp;
                adicionar(pasta, std::move(rot));
            }
            RegCloseKey(ak);
        }
        RegCloseKey(hk);
    }

    // 2. Variáveis de ambiente (OneDriveCommercial, OneDriveConsumer, OneDrive)
    for (const wchar_t* ev : { L"OneDriveCommercial", L"OneDriveConsumer", L"OneDrive" }) {
        wchar_t buf[MAX_PATH] = {};
        DWORD n = GetEnvironmentVariableW(ev, buf, MAX_PATH);
        if (n > 0 && n < MAX_PATH) adicionar(buf, L"");
    }

    // 3. Pastas "OneDrive" e "OneDrive - *" na pasta do usuário (inclui pastas corporativas mantidas localmente após logout)
    PWSTR prof = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &prof)) && prof) {
        std::wstring base = limpar(prof);
        CoTaskMemFree(prof);
        WIN32_FIND_DATAW fd{};
        HANDLE f = FindFirstFileExW((base + L"\\OneDrive*").c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
        if (f != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) continue;
                const wchar_t* nm = fd.cFileName;
                if (!_wcsicmp(nm, L"OneDrive") || !_wcsnicmp(nm, L"OneDrive - ", 11) || !_wcsnicmp(nm, L"OneDrive ", 9))
                    adicionar(base + L"\\" + nm, nm);
            } while (FindNextFileW(f, &fd));
            FindClose(f);
        }
    }

    // 4. FOLDERID_SkyDrive (caso o OneDrive pessoal tenha sido movido para outro disco/caminho)
    PWSTR sky = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SkyDrive, 0, nullptr, &sky)) && sky) {
        adicionar(sky, L"");
        CoTaskMemFree(sky);
    }

    return out;
}

}  // namespace nuvem
