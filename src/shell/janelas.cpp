// Barra de título do Mac nas janelas dos outros programas (ver src\janelas\macjanela.cpp).
// O gancho é posto só na thread de cada janela elegível de um programa permitido: jogos, launchers
// e anti-cheat nunca recebem a DLL. Programas de 32 bits passam pelo MacJanela32Host.exe.
// Desligar: janelas_mac=0 em ajustes.txt. Excluir mais programas: %APPDATA%\Shell\janelas-excluidas.txt (os jogos
// de %APPDATA%\Shell\jogos.txt, a lista do Modo Jogo, também ficam de fora).
// Quem entra (20/11/2026, regra dura: nada de DLL em jogo ou anti-cheat; detalhes em src\comum\jogos.h):
//   - janela com a barra de título padrão do Windows, de programa sem NENHUMA marca de jogo (nome, pasta, arquivos de
//     motor/loja/anti-cheat ao lado do .exe, classe de motor de jogo);
//   - janela sem a barra do Windows (barra própria) só de app Chromium/Electron reconhecido pelos arquivos;
//   - nada novo enquanto um app roda em tela cheia exclusiva (Direct3D) ou em modo apresentação/jogo (o Windows
//     avisa); programa com janela sem borda cobrindo o monitor (jogo em "tela cheia sem borda") fica fora de vez.
// A decisão é guardada por processo; a DLL confere de novo ao carregar e recusa entrar em jogo.
#include "shell.h"
#include "../comum/jogos.h"
#include "../comum/gancho_mapa.h"
#include <shellapi.h>
#include <shlobj.h>
#include <map>
#include <set>
#include <thread>

namespace {

HMODULE g_dll;     // o módulo do gancho: a carregadora fixa (WacOSGancho.dll) ou, sem ela, a própria WacOSWindow.dll
HOOKPROC g_proc;
HANDLE g_mapaH;    // o mapa que diz à carregadora qual WacOSWindow.dll usar (gancho_mapa.h); fica aberto até o Shell sair
GanchoMapa* g_mapa;
HWINEVENTHOOK g_ev;
std::map<DWORD, HHOOK> g_hooks;      // thread -> gancho (64 bits)
std::set<DWORD> g_hooks32;           // threads entregues ao host de 32 bits
std::map<DWORD, ULONGLONG> g_nasc;   // thread -> quando nasceu (o Windows reaproveita os números de thread)
struct Permissao { bool ok = false, chromium = false; ULONGLONG nasc = 0; std::wstring exe; };
std::map<DWORD, Permissao> g_procOk; // processo -> permitido (chromium: Chromium/Electron, pode a barra própria)
std::vector<std::wstring> g_extra;   // exclusões do usuário
const UINT HOST_HOOK = WM_APP + 1, HOST_QUIT = WM_APP + 2;

// Jogos, launchers, anti-cheat, partes do próprio Windows e apps cuja janela a DLL quebra: nunca recebem a DLL
const wchar_t* EXCLUDED_EXE[] = {
    // apps Qt e de jogo cuja janela a DLL quebra (PRs #224 e #225 do 404-Page-Found, 28/11/2026):
    // Premiere Pro (issue #219): atalhos de teclado (espaço, Ctrl+S) e o fechar da barra paravam;
    // VLC (issue #220): barra quebrada e o duplo clique não ia para a tela cheia com a janela pequena;
    // GeForce NOW (issues #195 e #197): os botões do Windows apareciam na barra e o jogo abria numa janela pequena
    L"adobe premiere pro.exe", L"vlc.exe", L"geforcenow.exe",
    L"explorer.exe", L"dwm.exe", L"shell.exe", L"arquivos.exe", L"ajustes.exe", L"monitoratividade.exe", L"terminal.exe", L"calculadora.exe", L"notas.exe",
    L"wacosshell.exe", L"wacosfinder.exe", L"wacossettings.exe", L"wacosactivitymonitor.exe", L"wacosterminal.exe", L"wacoscalculator.exe", L"wacosnotes.exe", L"wacoswindow32host.exe", L"searchhost.exe", L"startmenuexperiencehost.exe",
    L"snippingtool.exe", L"screenclippinghost.exe",  // Ferramenta de Captura (gravação de tela parava: prints do Discord, 19/11/2026)
    L"applicationframehost.exe", L"textinputhost.exe", L"lockapp.exe", L"shellexperiencehost.exe", L"systemsettings.exe", L"taskmgr.exe",
    L"steam.exe", L"steamwebhelper.exe", L"gameoverlayui.exe", L"battle.net.exe", L"agent.exe", L"eadesktop.exe", L"eabackgroundservice.exe",
    L"origin.exe", L"epicgameslauncher.exe", L"epicwebhelper.exe", L"riotclientservices.exe", L"riotclientux.exe", L"riotclientuxrender.exe",
    L"leagueclient.exe", L"leagueclientux.exe", L"leagueclientuxrender.exe", L"league of legends.exe", L"valorant.exe", L"valorant-win64-shipping.exe",
    L"vgc.exe", L"vgtray.exe", L"faceit.exe", L"faceitclient.exe", L"easyanticheat.exe", L"easyanticheat_eos.exe", L"beservice.exe",
    L"beservice_x64.exe", L"ubisoftconnect.exe", L"upc.exe", L"galaxyclient.exe", L"xboxpcapp.exe", L"gamebar.exe", L"robloxplayerbeta.exe",
    L"minecraftlauncher.exe", L"javaw.exe", L"rockstarlauncher.exe", L"socialclubhelper.exe", L"overwolf.exe", L"medal.exe",
};
const wchar_t* EXCLUDED_PATH[] = {
    L"\\steamapps\\", L"\\riot games\\", L"\\epic games\\", L"\\ea games\\", L"\\electronic arts\\", L"\\battle.net\\", L"\\blizzard",
    L"\\ubisoft", L"\\gog galaxy\\", L"\\xboxgames\\",L"\\rockstar games\\", L"\\easyanticheat", L"\\battleye",
};

std::wstring Lower(std::wstring s) { for (auto& c : s) c = towlower(c); return s; }

// Tabela de DLLs de gancho do Windows cheia (erro 126: no máximo 32 DLLs diferentes por login; cada versão nova da
// WacOSWindow.dll ocupa um lugar até todos os apps que a carregaram fecharem). Antes o Shell tentava de novo a cada
// janela (176 falhas no log de um dia) e ninguém sabia o que fazer. Agora: para de tentar e avisa UMA vez por login.
bool g_cheio = false;
bool g_manter = false;  // reiniciando o Shell: as janelas ficam com a DLL que já têm (o Shell novo assume)

// identificador do login atual (muda a cada entrada no Windows): o aviso sai uma vez por login
std::wstring LoginId() {
    HANDLE t = nullptr;
    TOKEN_STATISTICS ts{}; DWORD got = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &t)) return L"";
    bool ok = GetTokenInformation(t, TokenStatistics, &ts, sizeof(ts), &got) != FALSE;
    CloseHandle(t);
    if (!ok) return L"";
    wchar_t b[40]; swprintf_s(b, L"%08lx%08lx", (unsigned long)ts.AuthenticationId.HighPart, ts.AuthenticationId.LowPart);
    return b;
}

// Os de 32 bits vão por outra carregadora (WacOSGancho32, pelo hospedeiro): com a tabela cheia ela pode ficar de fora
// enquanto a de 64 bits, que já estava na tabela, continua entrando. Por isso a marca é separada (os apps de 64 bits
// seguem ganhando a barra).
bool g_cheio32 = false;
void GanchoFalhou(DWORD tid, DWORD err, const wchar_t* onde, bool b32 = false) {
    if (err != ERROR_MOD_NOT_FOUND) { Log(L"janelas: gancho%ls falhou %ls, thread %lu (erro %lu)", b32 ? L" de 32 bits" : L"", onde, tid, err); return; }
    bool& cheio = b32 ? g_cheio32 : g_cheio;
    if (cheio) return;
    cheio = true;
    Log(L"janelas: tabela de DLLs de gancho do Windows cheia (erro 126, thread %lu%ls): sem novas barras%ls até sair e entrar no Windows",
        tid, b32 ? L", carregadora de 32 bits" : L"", b32 ? L" nos apps de 32 bits" : L"");
    std::wstring id = LoginId();
    if (!id.empty() && shell::Setting(L"aviso_ganchos", L"") == id) return;  // já avisou neste login
    shell::SetSetting(L"aviso_ganchos", id);
    atualizacao::Avisar(Tr(L"Barras de título do Mac"),
                        Tr(L"As barras de título do Mac precisam que você saia e entre de novo no Windows para voltar a aparecer nos apps."));
}

ULONGLONG NascThread(DWORD tid) {
    HANDLE th = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!th) return 0;
    FILETIME c{}, e{}, k{}, u{};
    ULONGLONG r = GetThreadTimes(th, &c, &e, &k, &u) ? ((ULONGLONG)c.dwHighDateTime << 32 | c.dwLowDateTime) : 0;
    CloseHandle(th);
    return r;
}

// A thread já recebeu o gancho? Só se ainda for a mesma: o número de uma thread que morreu volta em outra (outro
// programa) e antes ficava marcado como "já tem": aquele programa nunca ganhava a barra. O gancho antigo morreu com
// a thread dele; aqui só se esquece a marca (e o UnhookWindowsHookEx do antigo não faz mal).
bool JaTem(DWORD tid) {
    if (!g_hooks.count(tid) && !g_hooks32.count(tid)) return false;
    ULONGLONG n = NascThread(tid);
    auto it = g_nasc.find(tid);
    if (it != g_nasc.end() && it->second == n) return true;
    if (auto h = g_hooks.find(tid); h != g_hooks.end()) { UnhookWindowsHookEx(h->second); g_hooks.erase(h); }
    g_hooks32.erase(tid);
    g_nasc.erase(tid);
    return false;
}

void LoadList(const std::wstring& f, bool criar) {
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, f.c_str(), L"r, ccs=UTF-8") || !fp) {
        // cria o arquivo com a explicação, para o usuário saber onde pôr os jogos dele
        if (criar && !_wfopen_s(&fp, f.c_str(), L"w, ccs=UTF-8") && fp) {
            fwprintf(fp, L"# Um programa por linha (nome do .exe ou parte do caminho). Estes nunca recebem a barra de título do Mac.\n");
            fclose(fp);
        }
        return;
    }
    wchar_t line[512];
    while (fgetws(line, 512, fp)) {
        std::wstring s = line;
        while (!s.empty() && iswspace(s.back())) s.pop_back();
        if (!s.empty() && s[0] != L'#') g_extra.push_back(Lower(s));
    }
    fclose(fp);
}

void LoadExtra() {
    g_extra.clear();
    PWSTR p = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p))) return;
    std::wstring base = std::wstring(p) + L"\\Shell\\";
    CoTaskMemFree(p);
    LoadList(base + L"janelas-excluidas.txt", true);
    LoadList(base + L"jogos.txt", false);  // a lista do Modo Jogo (jogo.cpp): jogo nunca recebe a DLL
}

// Um app está em tela cheia exclusiva (Direct3D) ou em modo apresentação/jogo: nenhum gancho novo agora.
bool TelaCheiaExclusiva() {
    QUERY_USER_NOTIFICATION_STATE q{};
    return SUCCEEDED(SHQueryUserNotificationState(&q)) && (q == QUNS_RUNNING_D3D_FULL_SCREEN || q == QUNS_BUSY || q == QUNS_PRESENTATION_MODE);
}

bool CobreMonitor(HWND h) {
    MONITORINFO mi{ sizeof(mi) };
    RECT r;
    if (!GetWindowRect(h, &r) || !GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    return r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top && r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom;
}

// O próprio Shell roda como administrador? (aí o gancho entra também nos programas elevados)
bool Elevado() {
    static int r = -1;
    if (r < 0) {
        r = 0;
        HANDLE t = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &t)) {
            TOKEN_ELEVATION el{}; DWORD got = 0;
            if (GetTokenInformation(t, TokenElevation, &el, sizeof(el), &got) && el.TokenIsElevated) r = 1;
            CloseHandle(t);
        }
    }
    return r == 1;
}

ULONGLONG Criacao(HANDLE ph) {
    FILETIME c{}, e{}, k{}, u{};
    if (!GetProcessTimes(ph, &c, &e, &k, &u)) return 0;
    return ((ULONGLONG)c.dwHighDateTime << 32) | c.dwLowDateTime;
}

// Pode pôr o gancho na janela h (processo pid)? O resultado do programa fica guardado (pelo pid + hora de criação,
// para um jogo aberto depois com o mesmo número não herdar a permissão de outro programa).
Permissao Allowed(HWND h, DWORD pid, bool* is32) {
    HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!ph) {  // provavelmente elevado ou protegido: não dá para pôr o gancho
        if (!g_procOk.count(pid)) { g_procOk[pid] = {}; Log(L"janelas: processo %lu fora (não deu para abrir: protegido ou de administrador)", pid); }
        return {};
    }
    BOOL wow = FALSE;
    IsWow64Process(ph, &wow);
    *is32 = wow != FALSE;
    ULONGLONG nasc = Criacao(ph);
    auto it = g_procOk.find(pid);
    if (it != g_procOk.end() && it->second.nasc == nasc) {
        CloseHandle(ph);
        Permissao r = it->second;
        // janela de motor de jogo aparecendo depois (launcher que vira o jogo): o programa sai de vez
        wchar_t cls[64] = {}; GetClassNameW(h, cls, 64);
        if (r.ok && jogos::ClasseDeJogo(cls)) { it->second.ok = false; Log(L"janelas: %ls fora (janela de jogo)", it->second.exe.c_str()); return {}; }
        return r;
    }
    wchar_t path[MAX_PATH] = {}; DWORD n = MAX_PATH;
    QueryFullProcessImageNameW(ph, 0, path, &n);
    // apps da Loja: só os de confiança total (Claude, WhatsApp...); os em contêiner (UWP) não carregam a DLL
    bool container = false, admin = false;
    HANDLE tok = nullptr;
    if (OpenProcessToken(ph, TOKEN_QUERY, &tok)) {
        DWORD v = 0, got = 0;
        if (GetTokenInformation(tok, TokenIsAppContainer, &v, sizeof(v), &got)) container = v != 0;
        TOKEN_ELEVATION el{};
        if (GetTokenInformation(tok, TokenElevation, &el, sizeof(el), &got) && el.TokenIsElevated) admin = !Elevado();
        CloseHandle(tok);
    } else if (GetLastError() == ERROR_ACCESS_DENIED && !Elevado()) admin = true;  // a ficha de um processo elevado não abre daqui
    CloseHandle(ph);
    std::wstring full = Lower(path), exe = full.substr(full.find_last_of(L'\\') + 1);
    Permissao r;
    r.nasc = nasc;
    r.exe = exe;
    r.ok = !full.empty() && !container;
    const wchar_t* por = L"";
    // Xbox/jogos da Loja continuam fora (anti-cheat)
    if (full.find(L"\\windowsapps\\") != std::wstring::npos && (full.find(L"microsoft.gaming") != std::wstring::npos || full.find(L"xbox") != std::wstring::npos)) { r.ok = false; por = L"Xbox"; }
    for (auto e : EXCLUDED_EXE) if (exe == e) r.ok = false;
    // os apps do WacOS: os que usam esta barra carregam a DLL eles mesmos (src\comum\barra_mac.h), com ou sem o
    // Shell; os outros desenham a própria. O gancho aqui punha uma segunda cópia por cima
    // (também as cópias renomeadas na hora de instalar, "inst-...-wacosfinder.exe")
    if (exe.find(L"wacos") != std::wstring::npos) r.ok = false;
    // pastas de lojas de jogos: menos o OBS Studio (jogos.h ProgramaNaoJogo), que também vem pela Steam
    if (!jogos::ProgramaNaoJogo(path))
        for (auto e : EXCLUDED_PATH) if (full.find(e) != std::wstring::npos) r.ok = false;
    // programa aberto como administrador (o OBS, para capturar jogos, costuma ser): o Windows não deixa um Shell sem
    // elevação pôr gancho nele (UIPI). Fica a barra do Windows; o log diz por quê (antes era silencioso)
    if (r.ok && admin) { r.ok = false; por = L"aberto como administrador: o gancho de um Shell sem elevação não entra"; }
    for (auto& e : g_extra) if (exe == e || full.find(e) != std::wstring::npos) { r.ok = false; por = L"lista do usuário"; }
    // jogo, launcher ou anti-cheat (nome, pasta, arquivos ao lado do .exe): nunca
    if (r.ok && jogos::ProgramaDeJogo(path)) { r.ok = false; por = L"marca de jogo"; }
    wchar_t cls[64] = {}; GetClassNameW(h, cls, 64);
    if (r.ok && jogos::ClasseDeJogo(cls)) { r.ok = false; por = L"janela de motor de jogo"; }
    r.chromium = r.ok && jogos::ProgramaChromium(path);
    // janela sem borda cobrindo o monitor de um programa que não é navegador: jogo em "tela cheia sem borda"
    LONG st = GetWindowLongW(h, GWL_STYLE);
    if (r.ok && !r.chromium && IsWindowVisible(h) && (st & WS_CAPTION) != WS_CAPTION && CobreMonitor(h)) { r.ok = false; por = L"tela cheia sem borda"; }
    g_procOk[pid] = r;
    if (r.ok) Log(L"janelas: barra do Mac em %ls%ls", exe.c_str(), r.chromium ? L" (Chromium/Electron)" : L"");
    else if (*por) Log(L"janelas: %ls fora (%ls)", exe.c_str(), por);
    return r;
}

// 1: barra de título do Windows; 2: barra própria (só vale para Chromium/Electron); 0: não
int Candidate(HWND h) {
    if (!IsWindow(h) || GetAncestor(h, GA_ROOT) != h || !IsWindowVisible(h)) return 0;
    LONG st = GetWindowLongW(h, GWL_STYLE), ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((st & WS_CHILD) || (ex & WS_EX_TOOLWINDOW)) return 0;
    RECT wr; GetWindowRect(h, &wr);
    POINT c{ 0, 0 }; ClientToScreen(h, &c);
    if ((st & WS_CAPTION) == WS_CAPTION && c.y - wr.top >= 18) return 1;  // barra de título do Windows (a DLL confere de novo, com o DPI do programa)
    // barra própria (Chrome, Discord, Electron): a DLL anima o maximizar e acerta cantos e borda (LeveProc).
    // Só a janela do Chromium (classe Chrome_WidgetWin_1); o programa ainda precisa ser Chromium/Electron (Allowed)
    wchar_t cls[32] = {}; GetClassNameW(h, cls, 32);
    if (wcscmp(cls, L"Chrome_WidgetWin_1")) return 0;
    if ((st & WS_THICKFRAME) && (st & WS_MAXIMIZEBOX) && !(ex & WS_EX_NOACTIVATE) && wr.right - wr.left >= 300 && wr.bottom - wr.top >= 200) return 2;
    return 0;
}

void Consider(HWND h) {
    int tipo = g_proc ? Candidate(h) : 0;
    if (!tipo) return;
    DWORD pid = 0, tid = GetWindowThreadProcessId(h, &pid);
    if (!tid || pid == GetCurrentProcessId()) return;
    if (JaTem(tid)) { PostMessageW(h, WM_NULL, 0, 0); return; }
    if (TelaCheiaExclusiva()) return;  // jogo/apresentação em tela cheia: nada novo agora (volta na próxima janela)
    if (g_cheio) return;  // tabela de ganchos cheia: não adianta tentar (GanchoFalhou)
    bool is32 = false;
    Permissao pm = Allowed(h, pid, &is32);
    if (!pm.ok || (tipo == 2 && !pm.chromium)) return;
    if (is32) {
        HWND host = FindWindowExW(HWND_MESSAGE, nullptr, L"MacJanela32Host", nullptr);
        if (!host || g_cheio32) return;
        // esperando a resposta (antes era PostMessage e uma falha do gancho passava calada)
        DWORD_PTR r = 0;
        if (!SendMessageTimeoutW(host, HOST_HOOK, tid, (LPARAM)h, SMTO_ABORTIFHUNG, 200, &r)) return;
        if (r) { GanchoFalhou(tid, (DWORD)r, L"ao mostrar", true); return; }
        g_hooks32.insert(tid);
        g_nasc[tid] = NascThread(tid);
        return;
    }
    HHOOK hk = SetWindowsHookExW(WH_GETMESSAGE, g_proc, g_dll, tid);
    // Erro 126: o Windows guarda no máximo 32 DLLs de gancho diferentes, e uma só sai da lista quando
    // todos os programas que a carregaram fecham. NUNCA cair para o MacJanela.dll sem cópia: um programa
    // que já tem uma versão antiga carregada com esse mesmo caminho recebe o endereço do gancho da versão
    // nova e trava (o Ableton fechou assim).
    if (!hk) { GanchoFalhou(tid, GetLastError(), L"ao mostrar"); return; }
    g_hooks[tid] = hk;
    g_nasc[tid] = NascThread(tid);
    PostMessageW(h, WM_NULL, 0, 0);  // a thread processa uma mensagem e a DLL entra
}

void CALLBACK OnEvent(HWINEVENTHOOK, DWORD, HWND h, LONG obj, LONG, DWORD, DWORD) {
    if (obj == OBJID_WINDOW && h) Consider(h);
}

// Janela de app criada (ainda escondida): o gancho entra já, para a DLL estar lá quando ela aparecer (antes só
// no "mostrar": a primeira janela de um app recém-aberto aparecia sem a animação de abrir)
HWINEVENTHOOK g_evCreate;
void CALLBACK OnCreate(HWINEVENTHOOK, DWORD, HWND h, LONG obj, LONG, DWORD, DWORD) {
    if (obj != OBJID_WINDOW || !h || !g_proc || GetAncestor(h, GA_ROOT) != h) return;
    LONG st = GetWindowLongW(h, GWL_STYLE), ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((st & WS_CHILD) || (ex & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) || !((st & WS_CAPTION) == WS_CAPTION || (st & WS_THICKFRAME))) return;
    DWORD pid = 0, tid = GetWindowThreadProcessId(h, &pid);
    if (!tid || pid == GetCurrentProcessId() || JaTem(tid)) return;
    if (TelaCheiaExclusiva() || g_cheio) return;
    // sem a barra do Windows: só a janela do Chromium/Electron
    wchar_t cls[32] = {}; GetClassNameW(h, cls, 32);
    bool propria = (st & WS_CAPTION) != WS_CAPTION;
    if (propria && wcscmp(cls, L"Chrome_WidgetWin_1")) return;
    bool is32 = false;
    Permissao pm = Allowed(h, pid, &is32);
    if (!pm.ok || (propria && !pm.chromium)) return;
    if (is32) {
        // 32 bits: pelo hospedeiro, já na criação (só no "mostrar" a primeira janela aparecia antes de a DLL
        // entrar e abria sem a animação). Esperando o hospedeiro pôr o gancho: a janela ainda está escondida
        HWND host = FindWindowExW(HWND_MESSAGE, nullptr, L"MacJanela32Host", nullptr);
        if (!host || g_cheio32) return;
        DWORD_PTR r = 0;
        if (!SendMessageTimeoutW(host, HOST_HOOK, tid, (LPARAM)h, SMTO_ABORTIFHUNG, 200, &r)) return;
        if (r) { GanchoFalhou(tid, (DWORD)r, L"na criação", true); return; }
        g_hooks32.insert(tid);
        g_nasc[tid] = NascThread(tid);
        return;
    }
    HHOOK hk = SetWindowsHookExW(WH_GETMESSAGE, g_proc, g_dll, tid);
    // antes era silencioso: com os 32 lugares de DLL de gancho cheios (erro 126) nenhum app novo recebia a barra e o
    // log não dizia nada
    if (!hk) { GanchoFalhou(tid, GetLastError(), L"na criação"); return; }
    g_hooks[tid] = hk;
    g_nasc[tid] = NascThread(tid);
    PostMessageW(h, WM_NULL, 0, 0);  // a thread processa uma mensagem e a DLL entra antes de mostrar
}

// Cópia da carregadora com o nome pelo CONTEÚDO (não pela data): recompilar sem mudar o código dá o mesmo arquivo
// (/Brepro) e o mesmo nome, e o Windows continua usando o mesmo lugar da tabela de ganchos.
std::wstring CopiaPorConteudo(const std::wstring& file, const wchar_t* prefix) {
    std::wstring copies = shell::dir + L"\\obj\\mj";
    CreateDirectoryW((shell::dir + L"\\obj").c_str(), nullptr);
    CreateDirectoryW(copies.c_str(), nullptr);
    std::wstring src = shell::dir + L"\\" + file;
    HANDLE f = CreateFileW(src.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return L"";
    unsigned long long hsh = 1469598103934665603ull;
    BYTE buf[65536]; DWORD n = 0;
    while (ReadFile(f, buf, sizeof(buf), &n, nullptr) && n) for (DWORD i = 0; i < n; i++) { hsh ^= buf[i]; hsh *= 1099511628211ull; }
    CloseHandle(f);
    wchar_t b[80]; swprintf_s(b, L"%s-%016llx.dll", prefix, hsh);
    std::wstring copy = copies + L"\\" + b;
    if (GetFileAttributesW(copy.c_str()) == INVALID_FILE_ATTRIBUTES && !CopyFileW(src.c_str(), copy.c_str(), TRUE)) return L"";
    return copy;
}

// Cópia com nome próprio em bin\obj\mj: os apps que já têm uma versão antiga presa carregam esta como outro
// módulo. O nome vem da data do arquivo: reabrir o Shell reaproveita a mesma cópia (cada nome novo ocupa um
// dos 32 lugares de DLL de gancho do Windows até os apps fecharem); só uma compilação nova gera outro.
// Vale para a DLL de 64 bits e para a de 32 (o hospedeiro de 32 bits recebe o caminho pronto).
std::wstring OwnCopy(const std::wstring& file, const wchar_t* prefix) {
    std::wstring copies = shell::dir + L"\\obj\\mj";
    CreateDirectoryW((shell::dir + L"\\obj").c_str(), nullptr);
    CreateDirectoryW(copies.c_str(), nullptr);
    std::wstring src = shell::dir + L"\\" + file, name;
    WIN32_FILE_ATTRIBUTE_DATA fa{};
    if (!GetFileAttributesExW(src.c_str(), GetFileExInfoStandard, &fa)) return L"";
    wchar_t b[64]; swprintf_s(b, L"%s-%08lx%08lx.dll", prefix, fa.ftLastWriteTime.dwHighDateTime, fa.ftLastWriteTime.dwLowDateTime);
    name = b;
    WIN32_FIND_DATAW fd;
    HANDLE f = FindFirstFileW((copies + L"\\" + prefix + L"-*.dll").c_str(), &fd);
    if (f != INVALID_HANDLE_VALUE) {
        do if (_wcsicmp(fd.cFileName, name.c_str())) DeleteFileW((copies + L"\\" + fd.cFileName).c_str()); while (FindNextFileW(f, &fd));  // as em uso ficam
        FindClose(f);
    }
    std::wstring copy = copies + L"\\" + name;
    if (GetFileAttributesW(copy.c_str()) == INVALID_FILE_ATTRIBUTES && !CopyFileW(src.c_str(), copy.c_str(), TRUE)) return L"";
    return copy;
}

}  // namespace

void janelas::Start() {
    if (g_dll) return;
    // só as barras, sem o Shell (WacOSTitleBars.exe, src\titulos): o Shell assume. Fechar devolve as janelas
    // ao Windows e o gancho daqui entra logo depois (esperando ele sair: dois ganchos na mesma thread não)
    if (HWND only = FindWindowW(L"WacOSBarrasDeTitulo", nullptr)) {
        DWORD pid = 0; GetWindowThreadProcessId(only, &pid);
        if (pid != GetCurrentProcessId()) {
            HANDLE ph = OpenProcess(SYNCHRONIZE, FALSE, pid);
            PostMessageW(only, WM_CLOSE, 0, 0);
            if (ph) { WaitForSingleObject(ph, 3000); CloseHandle(ph); }
            Log(L"janelas: WacOSTitleBars fechado (o Shell assume as barras)");
        }
    }
    UINT undo = RegisterWindowMessageW(L"MacJanelaDesfazer");
    if (shell::Setting(L"janelas_mac", L"1") != L"1") {
        // desligado: o Shell anterior pode ter saído reiniciando (sem desfazer, ver Stop): as janelas voltam ao Windows agora
        EnumWindows([](HWND h, LPARAM undo) -> BOOL {
            if (GetPropW(h, L"MacJanela") || GetPropW(h, L"MacJanelaLeve")) PostMessageW(h, (UINT)undo, 0, 0);
            return TRUE;
        }, undo);
        return;
    }
    std::wstring copy = OwnCopy(DLL_WINDOW, L"WacOSWindow");
    // sem cópia própria, nada de gancho: o mesmo caminho com outra versão trava os programas (ver Consider)
    if (copy.empty()) {
        Log(L"janelas: não deu para copiar " DLL_WINDOW);
        return;
    }
    wchar_t wow[MAX_PATH]{};
    bool tem32 = GetSystemWow64DirectoryW(wow, MAX_PATH) > 0;
    std::wstring copy32 = tem32 ? OwnCopy(DLL_WINDOW32, L"WacOSWindow32") : L"";
    // A carregadora fixa (gancho_mapa.h): o gancho do Windows é sempre ela, e ela carrega a WacOSWindow.dll escrita no
    // mapa. Trocar de versão (atualizar, reiniciar o Shell) não gasta mais lugar na tabela de ganchos (erro 126).
    std::wstring gancho = CopiaPorConteudo(DLL_GANCHO, L"WacOSGancho"), gancho32 = tem32 ? CopiaPorConteudo(DLL_GANCHO32, L"WacOSGancho32") : L"";
    if (!g_mapaH) {
        g_mapaH = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(GanchoMapa), GANCHO_MAPA);
        if (g_mapaH) g_mapa = (GanchoMapa*)MapViewOfFile(g_mapaH, FILE_MAP_WRITE, 0, 0, sizeof(GanchoMapa));
    }
    if (g_mapa && !gancho.empty()) {
        g_mapa->tamanho = sizeof(GanchoMapa);
        wcsncpy_s(g_mapa->real64, copy.c_str(), _TRUNCATE);
        wcsncpy_s(g_mapa->real32, copy32.c_str(), _TRUNCATE);
        wcsncpy_s(g_mapa->gancho32, gancho32.c_str(), _TRUNCATE);
        MemoryBarrier();
        LONG gen = g_mapa->gen + 1; if (!gen) gen = 1;  // 0 = "ainda nada" para a carregadora
        InterlockedExchange(&g_mapa->gen, gen);
        g_dll = LoadLibraryW(gancho.c_str());
        g_proc = g_dll ? (HOOKPROC)GetProcAddress(g_dll, "GetMsgProc") : nullptr;
        if (g_proc) Log(L"janelas: gancho pela carregadora %ls (barras: %ls)", gancho.substr(gancho.find_last_of(L'\\') + 1).c_str(),
                        copy.substr(copy.find_last_of(L'\\') + 1).c_str());
    }
    if (!g_proc) {  // sem a carregadora (instalação antiga): o jeito de antes, a própria WacOSWindow.dll no gancho
        g_dll = LoadLibraryW(copy.c_str());
        g_proc = g_dll ? (HOOKPROC)GetProcAddress(g_dll, "GetMsgProc") : nullptr;
        if (g_mapa) { g_mapa->real64[0] = 0; g_mapa->gancho32[0] = 0; InterlockedIncrement(&g_mapa->gen); }
        Log(L"janelas: sem a carregadora " DLL_GANCHO L": gancho direto (cada versão nova gasta um lugar da tabela)");
    }
    if (!g_proc) { Log(L"janelas: " DLL_WINDOW L" não encontrada"); return; }
    LoadExtra();
    // programas de 32 bits: a DLL de 32 bits fica no rundll32.exe de 32 bits do Windows (entrada WacOSHost, ver
    // macjanela.cpp), que recebe os pedidos de gancho pela janela "MacJanela32Host" (e põe o gancho com a carregadora de 32)
    if (tem32 && !FindWindowExW(HWND_MESSAGE, nullptr, L"MacJanela32Host", nullptr) && !copy32.empty()) {
        // numa thread: abrir o processo e esperar ele ficar pronto parou a interface 2,9 s ao iniciar com o disco
        // ocupado (pilha de 22/11/2026 20:01, issue #32). Pronto o hospedeiro, as janelas de 32 bits já abertas são revistas.
        std::wstring cmd = L"\"" + std::wstring(wow) + L"\\rundll32.exe\" \"" + copy32 + L"\",WacOSHost";
        std::thread([cmd]() mutable {
            STARTUPINFOW si{ sizeof(si) }; PROCESS_INFORMATION pi{};
            if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) return;
            WaitForInputIdle(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            shell::NaInterface([] { if (g_proc) EnumWindows([](HWND h, LPARAM) -> BOOL { Consider(h); return TRUE; }, 0); });
        }).detach();
    }
    g_ev = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, nullptr, OnEvent, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    g_evCreate = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, nullptr, OnCreate, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    EnumWindows([](HWND h, LPARAM) -> BOOL { Consider(h); return TRUE; }, 0);
}

void janelas::Foreground(HWND h) { Consider(h); }
void janelas::ManterAoReiniciar() { g_manter = true; }

void janelas::Stop() {
    if (!g_dll) return;
    if (g_ev) { UnhookWinEvent(g_ev); g_ev = nullptr; }
    if (g_evCreate) { UnhookWinEvent(g_evCreate); g_evCreate = nullptr; }
    // devolve o visual do Windows a todas as janelas transformadas (a DLL desfaz na thread de cada uma).
    // Reiniciando (ShellReiniciar), não: a DLL fica carregada nos apps (presa) e continua cuidando das janelas; o Shell
    // novo põe o gancho e, se a DLL dele for outra versão, ela mesma pede à antiga que solte e assume (TakeOver na DLL).
    // Antes o reinício soltava todas e, com a tabela de ganchos cheia (erro 126), TODAS ficavam sem a barra do Mac até
    // sair e entrar no Windows (e às vezes sobravam os cantos).
    if (g_manter) {
        Log(L"janelas: reiniciando: as janelas ficam com a barra do Mac (o Shell novo assume)");
        for (auto& [t, hk] : g_hooks) UnhookWindowsHookEx(hk);
        g_hooks.clear();
        g_hooks32.clear();
        if (HWND host = FindWindowExW(HWND_MESSAGE, nullptr, L"MacJanela32Host", nullptr)) PostMessageW(host, HOST_QUIT, 0, 0);
        g_proc = nullptr;
        g_dll = nullptr;
        return;
    }
    UINT undo = RegisterWindowMessageW(L"MacJanelaDesfazer");
    EnumWindows([](HWND h, LPARAM undo) -> BOOL {
        if (GetPropW(h, L"MacJanela") || GetPropW(h, L"MacJanelaLeve")) SendMessageTimeoutW(h, (UINT)undo, 0, 0, SMTO_ABORTIFHUNG, 300, nullptr);
        return TRUE;
    }, undo);
    for (auto& [t, hk] : g_hooks) UnhookWindowsHookEx(hk);
    g_hooks.clear();
    g_hooks32.clear();
    g_nasc.clear();
    if (HWND host = FindWindowExW(HWND_MESSAGE, nullptr, L"MacJanela32Host", nullptr)) PostMessageW(host, HOST_QUIT, 0, 0);
    g_proc = nullptr;
    g_dll = nullptr;  // a DLL fica carregada (presa) no nosso processo também
}
