// Fonte para coreano, japonês e chinês em todo o WacOS.
// As fontes SF Pro do pacote não têm hangul, kana nem ideogramas: o DirectWrite caía na reserva do Windows
// (Malgun Gothic, Yu Gothic, Microsoft YaHei), com outro peso, outra altura e outro espaçamento que o SF ao
// lado, e o texto em coreano parecia de outro sistema. Aqui todo formato de texto ganha uma reserva própria:
// cada escrita vai para a fonte que o macOS usa para ela, se estiver instalada, e depois para as equivalentes
// livres e as do Windows (a primeira instalada da lista vence). Nenhuma fonte vem junto com o WacOS.
//   coreano:  Apple SD Gothic Neo -> Pretendard -> Noto Sans KR -> Malgun Gothic
//   japonês:  Hiragino Sans -> Noto Sans JP -> Yu Gothic UI -> Meiryo UI
//   chinês:   PingFang SC -> Noto Sans SC -> Microsoft YaHei UI
// Os ideogramas (hanja/kanji/hanzi) e a pontuação CJK seguem o idioma do WacOS (ajustes.txt idioma=ko/ja/zh);
// nos outros idiomas ficam com a reserva do Windows, como antes. fonte_cjk=<família> no ajustes.txt põe uma
// fonte escolhida na frente da lista do idioma (para o coreano nos idiomas que não são CJK).
// Uso: cjk::Format(fábrica, ...) no lugar de fábrica->CreateTextFormat(...), mesmos argumentos.
#pragma once
#include <windows.h>
#include <dwrite_2.h>
#include <cstdio>
#include <string>
#include <vector>

namespace cjk {

struct Prefs { std::wstring lang, font; };

// idioma e fonte escolhida, lidos uma vez do %APPDATA%\Shell\ajustes.txt (o mesmo arquivo dos Ajustes)
inline const Prefs& ReadPrefs() {
    static Prefs p = [] {
        Prefs r;
        wchar_t ad[MAX_PATH] = {};
        if (!GetEnvironmentVariableW(L"APPDATA", ad, MAX_PATH)) return r;
        FILE* f = nullptr;
        if (_wfopen_s(&f, (std::wstring(ad) + L"\\Shell\\ajustes.txt").c_str(), L"r, ccs=UTF-8") || !f) return r;
        wchar_t line[512];
        while (fgetws(line, 512, f)) {
            std::wstring l = line;
            while (!l.empty() && (l.back() == L'\n' || l.back() == L'\r' || l.back() == L' ')) l.pop_back();
            if (!l.empty() && l[0] == 0xFEFF) l.erase(0, 1);
            if (!l.compare(0, 7, L"idioma=")) r.lang = l.substr(7, 2);
            else if (!l.compare(0, 10, L"fonte_cjk=")) r.font = l.substr(10);
        }
        fclose(f);
        return r;
    }();
    return p;
}

// a reserva (uma por processo; os formatos guardam a referência deles)
inline IDWriteFontFallback* Fallback(IDWriteFactory* factory) {
    static IDWriteFontFallback* fb = nullptr;
    static bool tried = false;
    if (tried) return fb;
    tried = true;
    IDWriteFactory2* f2 = nullptr;
    if (!factory || FAILED(factory->QueryInterface(__uuidof(IDWriteFactory2), (void**)&f2)) || !f2) return nullptr;
    IDWriteFontFallbackBuilder* b = nullptr;
    IDWriteFontFallback* sys = nullptr;
    if (SUCCEEDED(f2->CreateFontFallbackBuilder(&b)) && b) {
        const Prefs& p = ReadPrefs();
        const bool ko = p.lang == L"ko", ja = p.lang == L"ja", zh = p.lang == L"zh";
        auto chain = [&](std::vector<const wchar_t*> v, bool mine) {
            if (mine && !p.font.empty()) v.insert(v.begin(), p.font.c_str());
            return v;
        };
        std::vector<const wchar_t*> kr = chain({ L"Apple SD Gothic Neo", L"Pretendard", L"Pretendard Variable", L"Noto Sans KR", L"Malgun Gothic" }, ko || (!ja && !zh));
        std::vector<const wchar_t*> jp = chain({ L"Hiragino Sans", L"Hiragino Kaku Gothic ProN", L"Noto Sans JP", L"Yu Gothic UI", L"Meiryo UI" }, ja);
        std::vector<const wchar_t*> cn = chain({ L"PingFang SC", L"Noto Sans SC", L"Microsoft YaHei UI" }, zh);
        const DWRITE_UNICODE_RANGE hangul[] = {
            { 0x1100, 0x11FF }, { 0x3130, 0x318F }, { 0xA960, 0xA97F }, { 0xAC00, 0xD7AF }, { 0xD7B0, 0xD7FF }, { 0xFFA0, 0xFFDC } };
        const DWRITE_UNICODE_RANGE kana[] = { { 0x3040, 0x30FF }, { 0x31F0, 0x31FF }, { 0xFF66, 0xFF9F } };
        // ideogramas, pontuação CJK e formas de largura cheia: a fonte do idioma do WacOS
        const DWRITE_UNICODE_RANGE han[] = {
            { 0x3000, 0x303F }, { 0x3400, 0x4DBF }, { 0x4E00, 0x9FFF }, { 0xF900, 0xFAFF }, { 0xFF00, 0xFF65 }, { 0xFFE0, 0xFFEF } };
        // sem idioma no mapeamento (issue #234, Beicxxxx): o DirectWrite só usa um mapeamento com idioma quando o texto
        // tem esse mesmo idioma, e os formatos de texto são criados com "pt-br"/"" : as listas daqui eram ignoradas
        // em quase todas as janelas. A escolha do idioma já foi feita acima (qual lista vai para o han).
        b->AddMapping(hangul, ARRAYSIZE(hangul), kr.data(), (UINT32)kr.size(), nullptr, nullptr, nullptr, 1.0f);
        b->AddMapping(kana, ARRAYSIZE(kana), jp.data(), (UINT32)jp.size(), nullptr, nullptr, nullptr, 1.0f);
        if (ko) b->AddMapping(han, ARRAYSIZE(han), kr.data(), (UINT32)kr.size(), nullptr, nullptr, nullptr, 1.0f);
        else if (ja) b->AddMapping(han, ARRAYSIZE(han), jp.data(), (UINT32)jp.size(), nullptr, nullptr, nullptr, 1.0f);
        else if (zh) b->AddMapping(han, ARRAYSIZE(han), cn.data(), (UINT32)cn.size(), nullptr, nullptr, nullptr, 1.0f);
        if (SUCCEEDED(f2->GetSystemFontFallback(&sys)) && sys) b->AddMappings(sys);  // o resto: a reserva do Windows
        b->CreateFontFallback(&fb);
    }
    if (sys) sys->Release();
    if (b) b->Release();
    f2->Release();
    return fb;
}

inline void Apply(IDWriteFactory* factory, IDWriteTextFormat* tf) {
    if (!tf) return;
    IDWriteFontFallback* fb = Fallback(factory);
    if (!fb) return;  // Windows sem IDWriteFactory2 (8.0 e antes): como antes
    IDWriteTextFormat1* t1 = nullptr;
    if (SUCCEEDED(tf->QueryInterface(__uuidof(IDWriteTextFormat1), (void**)&t1)) && t1) { t1->SetFontFallback(fb); t1->Release(); }
}

// factory->CreateTextFormat(...) com a reserva CJK
inline HRESULT Format(IDWriteFactory* factory, const wchar_t* family, IDWriteFontCollection* collection, DWRITE_FONT_WEIGHT weight,
                      DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size, const wchar_t* locale, IDWriteTextFormat** out) {
    if (!factory) return E_POINTER;
    HRESULT hr = factory->CreateTextFormat(family, collection, weight, style, stretch, size, locale, out);
    if (SUCCEEDED(hr) && out && *out) Apply(factory, *out);
    return hr;
}

}  // namespace cjk
