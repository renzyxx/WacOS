#pragma once
// Os instrumentos da folha de desenho (a mesma família do Quill): corpos brancos foscos, peças de aço escovado,
// a ponta e uma faixa na cor da tinta. Cada peça tem 40x100, ponta para cima. São SVGs desenhados pelo próprio
// Direct2D (ID2D1DeviceContext5::DrawSvgDocument), com cache por alvo + peça + cor.
#include <windows.h>
#include <d2d1_3.h>
#include <d2d1svg.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <cstdio>

namespace instrumentos {

enum { PEN, FOUNTAIN, MARKER, PENCIL, CRAYON, BRUSH, ERASER, LASSO, RULER, MONO, BRUSHPEN, REED, COUNT };

inline const char* Defs() {
    return R"SVG(<defs>
<linearGradient id="lac" x1="0" x2="1"><stop offset="0" stop-color="#d6d8de"/><stop offset="0.18" stop-color="#f4f5f7"/><stop offset="0.34" stop-color="#ffffff"/><stop offset="0.66" stop-color="#f2f3f6"/><stop offset="1" stop-color="#c9ccd4"/></linearGradient>
<linearGradient id="steel" x1="0" x2="1"><stop offset="0" stop-color="#7c808a"/><stop offset="0.25" stop-color="#c9ccd3"/><stop offset="0.42" stop-color="#f7f8fa"/><stop offset="0.6" stop-color="#aeb2bb"/><stop offset="1" stop-color="#6c707a"/></linearGradient>
<linearGradient id="round" x1="0" x2="1"><stop offset="0" stop-color="#000" stop-opacity="0.28"/><stop offset="0.3" stop-color="#fff" stop-opacity="0.28"/><stop offset="0.45" stop-color="#fff" stop-opacity="0.05"/><stop offset="1" stop-color="#000" stop-opacity="0.3"/></linearGradient>
<linearGradient id="cedar" x1="0" x2="1"><stop offset="0" stop-color="#caa278"/><stop offset="0.4" stop-color="#f0d9b8"/><stop offset="1" stop-color="#bd925f"/></linearGradient>
<linearGradient id="wrap" x1="0" x2="1"><stop offset="0" stop-color="#dcd9d2"/><stop offset="0.34" stop-color="#fffefb"/><stop offset="1" stop-color="#cfcbc2"/></linearGradient>
<linearGradient id="cane" x1="0" x2="1"><stop offset="0" stop-color="#b8925a"/><stop offset="0.35" stop-color="#e8cf9e"/><stop offset="0.6" stop-color="#d4b37a"/><stop offset="1" stop-color="#a07c46"/></linearGradient>
<linearGradient id="rub" x1="0" x2="1"><stop offset="0" stop-color="#c4c7cf"/><stop offset="0.35" stop-color="#eef0f4"/><stop offset="1" stop-color="#b5b9c2"/></linearGradient>
</defs>)SVG";
}

inline std::string Shine(float x, float y1, float y2) {
    char b[160];
    sprintf_s(b, R"SVG(<path d="M%g %g V %g" stroke="#fff" stroke-width="1.3" stroke-linecap="round" opacity="0.9"/>)SVG", x, y1, y2);
    return b;
}

inline std::string Body(int k, bool active) {
    switch (k) {
    case PEN: return R"SVG(<path d="M19 4.5 h2 l0.6 7 h-3.2 z" fill="{C}"/>
<path d="M18.2 11.5 h3.6 l0.9 9 h-5.4 z" fill="url(#steel)"/>
<path d="M17.3 20.5 h5.4 L 29 44 H 11 Z" fill="url(#lac)"/>
<rect x="11" y="44" width="18" height="62" rx="2" fill="url(#lac)"/>
<rect x="11" y="51" width="18" height="3" fill="{C}"/>
<rect x="11" y="51" width="18" height="3" fill="url(#round)"/>)SVG" + Shine(15.2f, 30, 100);
    case FOUNTAIN: return R"SVG(<path d="M20 3.5 C 23.5 10, 26.5 18, 26.5 28 H 13.5 C 13.5 18, 16.5 10, 20 3.5 Z" fill="url(#steel)"/>
<path d="M20 4.5 V 19" stroke="#3d4048" stroke-width="0.9"/>
<circle cx="20" cy="20.5" r="1.6" fill="#3d4048"/>
<path d="M20 3.5 l1.2 3.2 h-2.4 z" fill="{C}"/>
<path d="M14.5 21 C 15 14, 17 9, 19 6" stroke="#fff" stroke-width="1" fill="none" opacity="0.8"/>
<rect x="12.5" y="28" width="15" height="9" rx="1.5" fill="url(#lac)"/>
<rect x="11" y="37" width="18" height="3" fill="url(#steel)"/>
<rect x="10.5" y="40" width="19" height="66" rx="3" fill="url(#lac)"/>
<rect x="10.5" y="44" width="19" height="2.2" fill="{C}"/>
<rect x="10.5" y="44" width="19" height="2.2" fill="url(#round)"/>)SVG" + Shine(14.6f, 42, 100);
    case MARKER: return R"SVG(<path d="M14 10 L 26 4.5 V 20 H 14 Z" fill="{C}"/>
<path d="M14 10 L 26 4.5 V 20 H 14 Z" fill="url(#round)"/>
<path d="M12 20 H 28 L 29.5 30 H 10.5 Z" fill="#e9edf3"/>
<path d="M12 20 H 28 L 29.5 30 H 10.5 Z" fill="url(#round)" opacity="0.6"/>
<rect x="9" y="30" width="22" height="76" rx="4.5" fill="url(#lac)"/>
<rect x="9" y="38" width="22" height="9" fill="{C}"/>
<rect x="9" y="38" width="22" height="9" fill="url(#round)"/>)SVG" + Shine(13.4f, 50, 100);
    case PENCIL: return R"SVG(<path d="M20 4 L 23 15 H 17 Z" fill="{C}"/>
<path d="M20 4 L 23 15 H 17 Z" fill="url(#round)"/>
<path d="M17 15 H 23 L 29.5 36 H 10.5 Z" fill="url(#cedar)"/>
<path d="M10.5 36 l 3 -2.2 l 3.2 2.2 l 3.3 -2.2 l 3.3 2.2 l 3.2 -2.2 l 3 2.2" fill="none" stroke="#fff" stroke-width="0.8" opacity="0.7"/>
<rect x="10.5" y="36" width="19" height="70" fill="url(#lac)"/>
<path d="M16.8 36 V 106 M23.2 36 V 106" stroke="#000" stroke-opacity="0.09" stroke-width="0.8"/>
<rect x="10.5" y="98" width="19" height="2.4" fill="{C}"/>)SVG" + Shine(14, 40, 100);
    case CRAYON: return R"SVG(<path d="M12.5 27 L 18.2 8.6 Q 20 5 21.8 8.6 L 27.5 27 Z" fill="{C}"/>
<path d="M12.5 27 L 18.2 8.6 Q 20 5 21.8 8.6 L 27.5 27 Z" fill="url(#round)"/>
<path d="M16.2 20 L 18.9 10.5" stroke="#fff" stroke-width="1.2" stroke-linecap="round" opacity="0.55"/>
<rect x="11" y="27" width="18" height="79" rx="2" fill="url(#wrap)"/>
<rect x="11" y="31" width="18" height="4.5" fill="{C}"/>
<rect x="11" y="92" width="18" height="4.5" fill="{C}"/>
<ellipse cx="20" cy="63" rx="5" ry="12.5" fill="none" stroke="{C}" stroke-width="1.8"/>
<rect x="11" y="27" width="18" height="79" rx="2" fill="url(#round)" opacity="0.55"/>
<path d="M14.3 38 V 90" stroke="#fff" stroke-width="1.2" stroke-linecap="round" opacity="0.7"/>)SVG";
    case BRUSH: return R"SVG(<path d="M20 2.5 C 25 10, 26 19, 24.5 27 H 15.5 C 14 19, 15 10, 20 2.5 Z" fill="{C}"/>
<path d="M20 2.5 C 25 10, 26 19, 24.5 27 H 15.5 C 14 19, 15 10, 20 2.5 Z" fill="url(#round)"/>
<path d="M18 9 C 17 14, 17 19, 17.6 24" stroke="#fff" stroke-width="1.2" fill="none" stroke-linecap="round" opacity="0.6"/>
<path d="M15.3 27 H 24.7 L 26 42 H 14 Z" fill="url(#steel)"/>
<path d="M14 33 H 26" stroke="#000" stroke-opacity="0.15" stroke-width="0.8"/>
<path d="M14 42 H 26 L 24.6 106 H 15.4 Z" fill="url(#lac)"/>
<rect x="14.2" y="47" width="11.6" height="2.4" fill="{C}"/>)SVG" + Shine(17.4f, 50, 100);
    case ERASER: return R"SVG(<rect x="10.5" y="7" width="19" height="25" rx="6.5" fill="url(#rub)"/>
<path d="M13.5 12 Q 15 9.5 18 9.2" stroke="#fff" stroke-width="1.3" fill="none" stroke-linecap="round"/>
<rect x="10" y="29" width="20" height="10" fill="url(#steel)"/>
<path d="M10 32.5 H 30 M10 35.5 H 30" stroke="#000" stroke-opacity="0.18" stroke-width="0.7"/>
<rect x="10" y="39" width="20" height="67" rx="3" fill="url(#lac)"/>)SVG" + Shine(14, 43, 100);
    case LASSO: return R"SVG(<path d="M20 5 L 24 22 H 16 Z" fill="url(#steel)"/>
<path d="M17.5 21 C 17.8 15, 18.8 10, 20 6" stroke="#fff" stroke-width="0.9" fill="none" opacity="0.8"/>
<path d="M16 22 H 24 L 29 40 H 11 Z" fill="url(#lac)"/>
<rect x="11" y="40" width="18" height="66" rx="2.5" fill="url(#lac)"/>
<ellipse cx="20" cy="56" rx="6" ry="8" fill="none" stroke="#0a84ff" stroke-width="1.6" stroke-dasharray="2.6 2.1"/>
<path d="M17 63.5 q -1.5 3 1 5" stroke="#0a84ff" stroke-width="1.6" fill="none" stroke-linecap="round"/>)SVG" + Shine(14.6f, 44, 100);
    case REED: return R"SVG(<path d="M15.5 6 L 24.5 3 L 24.5 24 H 15.5 Z" fill="url(#cane)"/>
<path d="M15.5 6 L 24.5 3 L 24.2 8.6 L 15.8 11.4 Z" fill="{C}"/>
<path d="M15.5 6 L 24.5 3 L 24.2 8.6 L 15.8 11.4 Z" fill="url(#round)" opacity="0.6"/>
<path d="M20 5.2 V 15" stroke="#5a4426" stroke-width="0.7" opacity="0.7"/>
<path d="M15.5 24 H 24.5 L 26 30 H 14 Z" fill="url(#cane)"/>
<path d="M14 30 H 26" stroke="#7a5c32" stroke-width="1.1" opacity="0.8"/>
<rect x="14" y="30" width="12" height="76" rx="3" fill="url(#cane)"/>
<path d="M14 58 H 26 M14 86 H 26" stroke="#7a5c32" stroke-width="1.1" opacity="0.7"/>
<rect x="14" y="36" width="12" height="2" fill="{C}"/>
<path d="M17.4 34 V 104" stroke="#fff" stroke-width="1.1" stroke-linecap="round" opacity="0.45"/>)SVG";
    case BRUSHPEN: return R"SVG(<path d="M20 2.8 C 22.6 8, 24 14, 23.6 22 H 16.4 C 16 14, 17.4 8, 20 2.8 Z" fill="{C}"/>
<path d="M20 2.8 C 22.6 8, 24 14, 23.6 22 H 16.4 C 16 14, 17.4 8, 20 2.8 Z" fill="url(#round)"/>
<path d="M18.4 9 C 17.9 13, 17.8 17, 18 20.5" stroke="#fff" stroke-width="1" fill="none" stroke-linecap="round" opacity="0.55"/>
<path d="M16.2 22 H 23.8 L 25.2 31 H 14.8 Z" fill="url(#steel)"/>
<path d="M14.8 26.5 H 25.2" stroke="#000" stroke-opacity="0.15" stroke-width="0.7"/>
<rect x="12.5" y="31" width="15" height="75" rx="7.5" fill="url(#lac)"/>
<rect x="12.5" y="37" width="15" height="2.2" fill="{C}"/>
<rect x="12.5" y="37" width="15" height="2.2" fill="url(#round)"/>
<rect x="12.5" y="41" width="15" height="1" fill="{C}" opacity="0.6"/>)SVG" + Shine(16, 44, 100);
    case MONO: return R"SVG(<path d="M19.45 3 h1.1 l0.35 8.5 h-1.8 z" fill="{C}"/>
<path d="M18.6 11.5 h2.8 l2.3 11.5 h-7.4 z" fill="url(#steel)"/>
<path d="M19.2 12.5 L 17.8 21.8" stroke="#fff" stroke-width="0.8" opacity="0.7"/>
<rect x="15.2" y="23" width="9.6" height="5.5" rx="1.2" fill="url(#steel)"/>
<path d="M15.2 25.7 H 24.8" stroke="#000" stroke-opacity="0.15" stroke-width="0.7"/>
<rect x="12" y="28.5" width="16" height="78" rx="8" fill="url(#lac)"/>
<rect x="12" y="35" width="16" height="2.2" fill="{C}"/>
<rect x="12" y="35" width="16" height="2.2" fill="url(#round)"/>)SVG" + Shine(15.6f, 40, 100);
    case RULER: {
        std::string s = std::string(R"SVG(<defs><linearGradient id="rub2" x1="0" x2="1"><stop offset="0" stop-color=")SVG") + (active ? "#cfe3ff" : "#e3e5ea") +
            R"SVG("/><stop offset="0.4" stop-color="#ffffff"/><stop offset="1" stop-color=")SVG" + (active ? "#b6d3ff" : "#d2d5dc") + R"SVG("/></linearGradient></defs>
<rect x="9.5" y="6" width="21" height="100" rx="2.5" fill="url(#rub2)" stroke="#000" stroke-opacity="0.1"/>)SVG";
        for (int i = 0; i < 16; i++) {
            char b[200];
            sprintf_s(b, R"SVG(<rect x="9.5" y="%d" width="%g" height="1.1" fill="%s" opacity="0.7"/>)SVG", 11 + i * 6, i % 2 ? 4.5 : 8.5, active ? "#0a63d8" : "#3a3c44");
            s += b;
        }
        return s + R"SVG(<path d="M27.5 9 V 100" stroke="#fff" stroke-width="1.2" opacity="0.9"/>)SVG";
    }
    }
    return "";
}

inline std::string Svg(int k, D2D1_COLOR_F c, bool active) {
    char hex[16];
    sprintf_s(hex, "#%02x%02x%02x", (int)(c.r * 255 + 0.5f), (int)(c.g * 255 + 0.5f), (int)(c.b * 255 + 0.5f));
    std::string body = Body(k, active);
    for (size_t at; (at = body.find("{C}")) != std::string::npos;) body.replace(at, 3, hex);
    return std::string(R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 40 100" width="40" height="100">)SVG") + Defs() + body + "</svg>";
}

struct Entry { ID2D1DeviceContext5* dc; int k; UINT32 rgb; bool active; ID2D1SvgDocument* doc; };
inline std::vector<Entry>& Cache() { static std::vector<Entry> c; return c; }
inline void ClearCache() { for (auto& e : Cache()) e.doc->Release(); Cache().clear(); }

// desenha a peça k com o canto de cima à esquerda em (x, y), 40x100 pt. false: o alvo não desenha SVG (Windows antigo)
inline bool Draw(ID2D1DeviceContext* dc, int k, D2D1_COLOR_F c, float x, float y, bool active = false) {
    ID2D1DeviceContext5* d5 = nullptr;
    if (FAILED(dc->QueryInterface(__uuidof(ID2D1DeviceContext5), (void**)&d5)) || !d5) return false;
    UINT32 rgb = ((UINT32)(c.r * 255) << 16) | ((UINT32)(c.g * 255) << 8) | (UINT32)(c.b * 255);
    ID2D1SvgDocument* doc = nullptr;
    for (auto& e : Cache()) if (e.dc == d5 && e.k == k && e.rgb == rgb && e.active == active) { doc = e.doc; break; }
    if (!doc) {
        if (Cache().size() > 80) ClearCache();
        std::string s = Svg(k, c, active);
        IStream* st = SHCreateMemStream((const BYTE*)s.data(), (UINT)s.size());
        if (st && SUCCEEDED(d5->CreateSvgDocument(st, D2D1::SizeF(40, 100), &doc)) && doc) Cache().push_back({ d5, k, rgb, active, doc });
        if (st) st->Release();
    }
    if (doc) {
        D2D1_MATRIX_3X2_F old; d5->GetTransform(&old);
        d5->SetTransform(D2D1::Matrix3x2F::Translation(x, y) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
        d5->DrawSvgDocument(doc);
        d5->SetTransform(old);
    }
    d5->Release();
    return doc != nullptr;
}


// ícones da barra (os do Quill: traço de 1,9 num quadro de 24), na cor pedida, centrados em (cx, cy)
struct IconEntry { ID2D1DeviceContext5* dc; const char* d; UINT32 rgb; bool dashed; ID2D1SvgDocument* doc; };
inline std::vector<IconEntry>& Icons() { static std::vector<IconEntry> c; return c; }
inline bool Icon(ID2D1DeviceContext* dc, const char* d, D2D1_COLOR_F c, float cx, float cy, float size = 20, bool dashed = false, float sw = 1.9f) {
    ID2D1DeviceContext5* d5 = nullptr;
    if (FAILED(dc->QueryInterface(__uuidof(ID2D1DeviceContext5), (void**)&d5)) || !d5) return false;
    UINT32 rgb = ((UINT32)(c.r * 255) << 16) | ((UINT32)(c.g * 255) << 8) | (UINT32)(c.b * 255);
    ID2D1SvgDocument* doc = nullptr;
    for (auto& e : Icons()) if (e.dc == d5 && e.d == d && e.rgb == rgb && e.dashed == dashed) { doc = e.doc; break; }
    if (!doc) {
        if (Icons().size() > 80) { for (auto& e : Icons()) e.doc->Release(); Icons().clear(); }
        char hex[16]; sprintf_s(hex, "#%06x", rgb);
        char swb[16]; sprintf_s(swb, "%g", sw);
        std::string s = std::string(R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24"><path d=")SVG") + d +
            R"SVG(" fill="none" stroke=")SVG" + hex + R"SVG(" stroke-width=")SVG" + swb + R"SVG(" stroke-linecap="round" stroke-linejoin="round")SVG" +
            (dashed ? R"SVG( stroke-dasharray="2.6 2.6")SVG" : "") + "/></svg>";
        IStream* st = SHCreateMemStream((const BYTE*)s.data(), (UINT)s.size());
        if (st && SUCCEEDED(d5->CreateSvgDocument(st, D2D1::SizeF(24, 24), &doc)) && doc) Icons().push_back({ d5, d, rgb, dashed, doc });
        if (st) st->Release();
    }
    if (doc) {
        D2D1_MATRIX_3X2_F old; d5->GetTransform(&old);
        float k = size / 24;
        d5->SetTransform(D2D1::Matrix3x2F::Scale(k, k) * D2D1::Matrix3x2F::Translation(cx - size / 2, cy - size / 2) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
        d5->DrawSvgDocument(doc);
        d5->SetTransform(old);
    }
    d5->Release();
    return doc != nullptr;
}
inline void ClearIcons() { for (auto& e : Icons()) e.doc->Release(); Icons().clear(); }

}  // namespace instrumentos
