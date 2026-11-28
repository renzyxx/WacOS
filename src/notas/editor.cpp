// Notas: o editor de texto rico. Cada parágrafo vira um IDWriteTextLayout (estilo, negrito/itálico/
// sublinhado/tachado, realces, variáveis e resultados das Notas Matemáticas); cursor amarelo, seleção,
// listas (marcadores, traços, números, verificação com bolinhas), seções recolhíveis nos títulos,
// desfazer/refazer e área de transferência com formatação (formato próprio + texto puro).
#include "notas.h"
#include "../comum/fonte_cjk.h" // a reserva para coreano, japonês e chinês
#include <cwctype>
#include <algorithm>
#include <cmath>
#include "../comum/simbolos.h"
#include "../comum/semaforo.h"  // tema Leopard: o papel amarelo pautado das Notas do 10.5
#include <wincodec.h>
#include <shellapi.h>
#include <map>
// Tabelas, anexos e Ferramentas de Escrita (botões da barra do macOS 26/27):
// - tabela: cada linha é um parágrafo K_ROW com as células separadas por TAB; o layout usa uma parada de tabulação por
//   coluna (largura da tabela / colunas), sem quebra de linha, e a grade é desenhada por cima. Tab/Shift+Tab andam de
//   célula em célula, Enter abre uma linha nova e Enter numa linha vazia no fim sai da tabela.
// - anexo: parágrafo K_FILE sem texto, com o arquivo em `src` (copiado para Notas\anexos). Imagens aparecem na nota
//   (WIC -> bitmap do Direct2D, até 400 pt de altura); outros arquivos viram um cartão; clique duplo abre no programa
//   padrão. O cursor fica à esquerda do anexo; Backspace/Delete apagam o anexo inteiro.
// - Ferramentas de Escrita: transformações locais, sem IA (revisar espaços/pontuação/maiúsculas, lista, tabela, caixa).

using editor::Pos;

namespace {

Note* N;
Pos caret, anchor;
bool focused, blinkOn = true;
float scrollY, desiredX = -1;
D2D1_RECT_F area{};
std::wstring family = L"Segoe UI", monoFamily = L"Consolas";
bool dark;
std::wstring query;
uint8_t typing; bool typingSet;
bool wantReveal;
int hoverChevron = -1;
int clickMode;   // 1 caractere, 2 palavra, 3 parágrafo (arrastar estende nessa unidade)
Pos clickA, clickB;

struct PL { IDWriteTextLayout* tl = nullptr; float x = 0, y = 0, h = 0, w = 0, base = 0; bool hidden = false; int number = 0, resultAt = -1;
             float pad = 0;                       // linha de tabela: folga em cima e embaixo do texto
             int cols = 0; float colW = 0, tableW = 0;  // linha de tabela
             float imgW = 0, imgH = 0; };         // anexo: tamanho na nota
std::vector<PL> pl;
bool dirtyLayout = true;
float layoutW = -1;
float contentH;
std::vector<MathLine> math;
ID2D1DeviceContext* lastDc;
ID2D1SolidColorBrush *brAccent, *brHi[HI_COUNT + 1];
IDWriteTextFormat* fmt[5];

const float MARGIN = 24, INDENT = 26, TOP_PAD = 34, CELL_PAD = 8, ROW_PAD = 7;

// medidas de cada estilo (tamanho, altura da linha, espaço depois)
struct StyleM { float size, line, after; DWRITE_FONT_WEIGHT w; };
const StyleM SM[5] = {
    { 24, 30, 6, DWRITE_FONT_WEIGHT_BOLD },        // Título
    { 18, 23, 4, DWRITE_FONT_WEIGHT_BOLD },        // Cabeçalho
    { 14, 18, 3, DWRITE_FONT_WEIGHT_BOLD },        // Subcabeçalho
    { 13, 17.5f, 0, DWRITE_FONT_WEIGHT_NORMAL },   // Corpo
    { 12, 16.5f, 0, DWRITE_FONT_WEIGHT_NORMAL },   // Monoespaçado
};

D2D1_COLOR_F TextColor() { return dark ? D2D1::ColorF(1, 1, 1, 0.88f) : D2D1::ColorF(0.11f, 0.11f, 0.12f); }
D2D1_COLOR_F Text2() { return dark ? D2D1::ColorF(1, 1, 1, 0.45f) : D2D1::ColorF(0, 0, 0, 0.45f); }

// realces do Sequoia: roxo, rosa, laranja, menta, azul (texto na cor, fundo claro da mesma cor)
D2D1_COLOR_F HiText(int h) {
    static const D2D1_COLOR_F L[HI_COUNT] = { D2D1::ColorF(0.55f, 0.27f, 0.85f), D2D1::ColorF(0.86f, 0.18f, 0.45f), D2D1::ColorF(0.85f, 0.42f, 0.0f), D2D1::ColorF(0.0f, 0.55f, 0.48f), D2D1::ColorF(0.0f, 0.45f, 0.90f) };
    static const D2D1_COLOR_F D[HI_COUNT] = { D2D1::ColorF(0.80f, 0.62f, 1.0f), D2D1::ColorF(1.0f, 0.50f, 0.68f), D2D1::ColorF(1.0f, 0.68f, 0.35f), D2D1::ColorF(0.40f, 0.90f, 0.80f), D2D1::ColorF(0.45f, 0.72f, 1.0f) };
    return dark ? D[h - 1] : L[h - 1];
}
D2D1_COLOR_F HiBack(int h) {
    static const D2D1_COLOR_F c[HI_COUNT] = { D2D1::ColorF(0.69f, 0.42f, 0.98f), D2D1::ColorF(1.0f, 0.30f, 0.55f), D2D1::ColorF(1.0f, 0.58f, 0.15f), D2D1::ColorF(0.20f, 0.82f, 0.72f), D2D1::ColorF(0.25f, 0.60f, 1.0f) };
    D2D1_COLOR_F k = c[h - 1]; k.a = dark ? 0.30f : 0.22f; return k;
}

bool Less(Pos a, Pos b) { return a.p < b.p || (a.p == b.p && a.o < b.o); }
bool Same(Pos a, Pos b) { return a.p == b.p && a.o == b.o; }
Pos SelStart() { return Less(caret, anchor) ? caret : anchor; }
Pos SelEnd() { return Less(caret, anchor) ? anchor : caret; }
bool HasSel() { return !Same(caret, anchor); }
Para& P(int i) { return N->paras[i]; }
int Count() { return N ? (int)N->paras.size() : 0; }
int Level(const Para& p) { return p.kind == K_TEXT && p.style <= S_SUBHEADING ? p.style : 99; }
bool IsRow(int i) { return i >= 0 && i < Count() && P(i).kind == K_ROW; }
bool IsFile(int i) { return i >= 0 && i < Count() && P(i).kind == K_FILE; }
int Tabs(const std::wstring& s) { return (int)std::count(s.begin(), s.end(), L'\t'); }
int TableCols(int i) {  // colunas do bloco de linhas que contém i (a linha mais larga manda)
    int a = i, b = i, n = 1;
    while (IsRow(a - 1)) a--;
    while (IsRow(b + 1)) b++;
    for (int k = a; k <= b; k++) n = std::max(n, Tabs(P(k).text) + 1);
    return n;
}
float After(int i) { return IsFile(i) ? 10.f : IsRow(i) ? (IsRow(i + 1) ? 0.f : 10.f) : SM[P(i).style].after; }
bool IsImageFile(const std::wstring& path) {
    std::wstring e = path.substr(path.find_last_of(L'.') + 1);
    for (auto& c : e) c = towlower(c);
    return e == L"png" || e == L"jpg" || e == L"jpeg" || e == L"gif" || e == L"bmp" || e == L"tif" || e == L"tiff" || e == L"webp" || e == L"heic" || e == L"ico";
}
// nome mostrado: sem o prefixo único que o AddAttachment põe ("1dd7...-foto.png" -> "foto.png")
std::wstring ShownName(const std::wstring& path) {
    std::wstring n = path.substr(path.find_last_of(L"\\/") + 1);
    size_t d = n.find(L'-');
    if (d != std::wstring::npos && d >= 8 && n.find_first_not_of(L"0123456789abcdef") == d) n = n.substr(d + 1);
    return n;
}

// imagens dos anexos (por contexto do Direct2D; nullptr guardado = não é imagem ou não abriu)
std::map<std::wstring, ID2D1Bitmap*> imgs;
IWICImagingFactory* wic;
void ReleaseImages() { for (auto& k : imgs) if (k.second) k.second->Release(); imgs.clear(); }
ID2D1Bitmap* Img(ID2D1DeviceContext* dc, const std::wstring& path) {
    auto it = imgs.find(path);
    if (it != imgs.end()) return it->second;
    ID2D1Bitmap* bmp = nullptr;
    if (IsImageFile(path)) {
        if (!wic) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
        IWICBitmapDecoder* dec = nullptr; IWICBitmapFrameDecode* fr = nullptr; IWICFormatConverter* cv = nullptr;
        if (wic && SUCCEEDED(wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &dec)) &&
            SUCCEEDED(dec->GetFrame(0, &fr)) && SUCCEEDED(wic->CreateFormatConverter(&cv)) &&
            SUCCEEDED(cv->Initialize(fr, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut)))
            dc->CreateBitmapFromWicBitmap(cv, nullptr, &bmp);
        if (cv) cv->Release(); if (fr) fr->Release(); if (dec) dec->Release();
    }
    imgs[path] = bmp;
    return bmp;
}

void ReleaseLayouts() { for (auto& l : pl) if (l.tl) l.tl->Release(); pl.clear(); dirtyLayout = true; }
void ReleaseBrushes() {
    ReleaseImages();
    if (brAccent) brAccent->Release(); brAccent = nullptr;
    for (auto& b : brHi) { if (b) b->Release(); b = nullptr; }
}

void Clamp() {
    if (!N) return;
    auto fix = [](Pos& q) { q.p = std::clamp(q.p, 0, Count() - 1); q.o = std::clamp(q.o, 0, (int)P(q.p).text.size()); };
    fix(caret); fix(anchor);
}

// ---------------------------------------------------------------------------
// layout
void ComputeHidden() {
    int hideLevel = -1;  // escondendo até achar um título de nível <= hideLevel
    for (int i = 0; i < Count(); i++) {
        int lv = Level(P(i));
        if (hideLevel >= 0) {
            if (lv <= hideLevel) hideLevel = -1;
            else { pl[i].hidden = true; continue; }
        }
        pl[i].hidden = false;
        if (P(i).collapsed && lv < 99) hideLevel = lv;
    }
}

void BuildLayouts(ID2D1DeviceContext* dc, float width) {
    if (dc != lastDc) { ReleaseBrushes(); ReleaseLayouts(); lastDc = dc; }
    if (!brAccent) {
        dc->CreateSolidColorBrush(AccentText(), &brAccent);
        for (int h = 1; h <= HI_COUNT; h++) dc->CreateSolidColorBrush(HiText(h), &brHi[h]);
    }
    if (!dirtyLayout && width == layoutW && (int)pl.size() == Count()) return;
    for (auto& l : pl) if (l.tl) l.tl->Release();
    pl.assign(Count(), PL{});
    layoutW = width;
    dirtyLayout = false;
    math = EvalMath(N->paras);
    int counters[10] = {};
    float y = TOP_PAD;
    for (int i = 0; i < Count(); i++) {
        Para& p = P(i);
        PL& l = pl[i];
        // numeração das listas numeradas
        if (p.list == L_NONE) std::fill(std::begin(counters), std::end(counters), 0);
        else {
            for (int k = p.indent + 1; k < 10; k++) counters[k] = 0;
            if (p.list == L_NUMBER) l.number = ++counters[std::min<int>(p.indent, 9)];
            else counters[std::min<int>(p.indent, 9)] = 0;
        }
        float base = MARGIN + p.indent * INDENT;
        if (p.kind == K_FILE) {  // anexo: imagem na largura possível (até 400 de altura) ou cartão de 52
            l.x = l.base = base; l.w = std::max(40.f, width - base - MARGIN);
            gfx::dw->CreateTextLayout(L"", 0, fmt[S_BODY], l.w, 10000, &l.tl);
            if (l.tl) l.tl->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, SM[S_BODY].line, SM[S_BODY].line * 0.8f);
            if (ID2D1Bitmap* b = Img(dc, p.src)) {
                D2D1_SIZE_U px = b->GetPixelSize();
                // desenhos do Notas são gravados em 2x; o resto, 1 pixel = 1 pt
                float k = ShownName(p.src).rfind(L"desenho-", 0) == 0 || p.src.find(L"\\desenho-") != std::wstring::npos ? 0.5f : 1.f;
                float w = std::min(l.w, std::max(1.f, px.width * k)), h = w * px.height / std::max(1u, px.width);
                if (h > 400) { w *= 400 / h; h = 400; }
                l.imgW = w; l.imgH = h;
            } else { l.imgW = std::min(l.w, 320.f); l.imgH = 52; }
            l.h = l.imgH; l.y = y; y += l.h + After(i);
            continue;
        }
        if (p.kind == K_ROW) {  // linha de tabela: uma parada de tabulação por coluna, sem quebra
            l.cols = TableCols(i); l.base = base;
            l.tableW = std::max(120.f, width - base - MARGIN); l.colW = l.tableW / l.cols;
            l.x = base + CELL_PAD; l.w = l.tableW - CELL_PAD; l.pad = ROW_PAD;
        }
        float lead = p.kind == K_ROW ? 0.f : p.list == L_CHECK ? 28.f : p.list != L_NONE ? 24.f : 0.f;
        if (p.kind != K_ROW) { l.x = base + lead; l.base = base; l.w = std::max(40.f, width - l.x - MARGIN); }
        std::wstring s = p.text;
        int resultAt = -1;
        if (i < (int)math.size() && !math[i].result.empty()) {
            if (s.size() >= 2 && s.back() == L'=' && s[s.size() - 2] == L' ') s += L' ';  // "x =" -> "x = 5"
            resultAt = (int)s.size(); s += math[i].result;
        }
        l.resultAt = resultAt;
        const StyleM& m = SM[p.style];
        gfx::dw->CreateTextLayout(s.c_str(), (UINT32)s.size(), fmt[p.style], l.w, 10000, &l.tl);
        if (!l.tl) continue;
        l.tl->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, m.line, m.line * 0.8f);
        if (p.kind == K_ROW) { l.tl->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP); l.tl->SetIncrementalTabStop(l.colW); }
        UINT32 n = (UINT32)p.text.size();
        for (UINT32 k = 0; k < n;) {  // corridas de atributos
            UINT32 j = k; uint8_t a = p.at[k];
            while (j < n && p.at[j] == a) j++;
            DWRITE_TEXT_RANGE r{ k, j - k };
            if (a & A_BOLD) l.tl->SetFontWeight(p.style == S_BODY || p.style == S_MONO ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_HEAVY, r);
            if (a & A_ITALIC) l.tl->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, r);
            if (a & A_UNDER) l.tl->SetUnderline(TRUE, r);
            if (a & A_STRIKE) l.tl->SetStrikethrough(TRUE, r);
            if (HiOf(a)) l.tl->SetDrawingEffect(brHi[HiOf(a)], r);
            k = j;
        }
        if (i < (int)math.size())
            for (auto& v : math[i].vars) l.tl->SetDrawingEffect(brAccent, { (UINT32)v.first, (UINT32)v.second });
        if (resultAt >= 0) l.tl->SetDrawingEffect(brAccent, { (UINT32)resultAt, (UINT32)math[i].result.size() });
        DWRITE_TEXT_METRICS tm{}; l.tl->GetMetrics(&tm);
        l.h = std::max(tm.height, m.line);
        l.y = y + l.pad;
        y += l.h + 2 * l.pad + After(i);
    }
    ComputeHidden();
    // recolhidos não ocupam espaço
    y = TOP_PAD;
    for (int i = 0; i < Count(); i++) {
        pl[i].y = y + pl[i].pad;
        if (!pl[i].hidden) y += pl[i].h + 2 * pl[i].pad + After(i);
    }
    contentH = y + 40;
}

void Changed() {
    if (!N) return;
    N->modified = store::Now();
    N->dirty = true;
    dirtyLayout = true;
    wantReveal = true;
    blinkOn = true;
    NoteEdited(N);
}

// cursor em coordenadas do conteúdo (antes da rolagem)
D2D1_RECT_F CaretRectAt(Pos q) {
    if (q.p >= (int)pl.size() || !pl[q.p].tl) return { 0, 0, 0, 0 };
    if (P(q.p).kind == K_FILE) return { pl[q.p].x - 4, pl[q.p].y, pl[q.p].x - 2, pl[q.p].y + pl[q.p].h };  // à esquerda do anexo
    float x, y; DWRITE_HIT_TEST_METRICS m;
    pl[q.p].tl->HitTestTextPosition((UINT32)q.o, FALSE, &x, &y, &m);
    return { pl[q.p].x + x, pl[q.p].y + m.top, pl[q.p].x + x + 2, pl[q.p].y + m.top + m.height };
}

Pos HitContent(float x, float y) {  // ponto no conteúdo -> posição
    Pos r{ 0, 0 };
    if (!N || pl.empty()) return r;
    int best = -1;
    for (int i = 0; i < Count(); i++) {
        if (pl[i].hidden) continue;
        best = i;
        if (y < pl[i].y + pl[i].h + pl[i].pad + After(i)) break;
    }
    if (best < 0) return r;
    r.p = best;
    if (!pl[best].tl) return r;
    BOOL trailing, inside; DWRITE_HIT_TEST_METRICS m;
    pl[best].tl->HitTestPoint(x - pl[best].x, y - pl[best].y, &trailing, &inside, &m);
    r.o = std::min((int)(m.textPosition + (trailing ? m.length : 0)), (int)P(best).text.size());
    return r;
}

void Reveal() {
    if (!N || pl.empty()) return;
    D2D1_RECT_F c = CaretRectAt(caret);
    float h = area.bottom - area.top;
    if (c.top - 8 < scrollY) scrollY = c.top - 8;
    if (c.bottom + 24 > scrollY + h) scrollY = c.bottom + 24 - h;
    scrollY = std::clamp(scrollY, 0.f, std::max(0.f, contentH - h));
}

// ---------------------------------------------------------------------------
// edição
void Checkpoint(int kind) {
    if (!N) return;
    ULONGLONG now = GetTickCount64();
    if (kind == 3 || kind != N->lastEditKind || now - N->lastEditAt > 1500) {
        N->undo.push_back({ N->paras, caret.p, caret.o, anchor.p, anchor.o });
        if (N->undo.size() > 200) N->undo.erase(N->undo.begin());
        N->redo.clear();
    }
    N->lastEditKind = kind; N->lastEditAt = now;
}

uint8_t AttrAtCaret() {
    if (typingSet) return typing;
    Para& p = P(caret.p);
    if (caret.o > 0 && caret.o <= (int)p.at.size()) return p.at[caret.o - 1];
    if (!p.at.empty()) return p.at[0];
    return 0;
}

void DeleteRange(Pos a, Pos b) {
    if (!Less(a, b)) return;
    Para& pa = P(a.p);
    if (a.p == b.p) {
        pa.text.erase(a.o, b.o - a.o);
        pa.at.erase(pa.at.begin() + a.o, pa.at.begin() + b.o);
    } else if (P(b.p).kind == K_FILE && b.o == 0) {  // a seleção para antes do anexo: ele fica inteiro
        if (pa.kind == K_FILE) { N->paras.erase(N->paras.begin() + a.p, N->paras.begin() + b.p); caret = anchor = { a.p, 0 }; return; }
        pa.text.resize(a.o); pa.at.resize(a.o);
        N->paras.erase(N->paras.begin() + a.p + 1, N->paras.begin() + b.p);
    } else {
        Para& pb = P(b.p);
        if (pa.kind == K_FILE) { pa.kind = pb.kind; pa.src = pb.src; pa.style = pb.style; pa.list = pb.list; pa.indent = pb.indent; pa.checked = pb.checked; }  // o anexo selecionado some
        pa.text = pa.text.substr(0, a.o) + pb.text.substr(b.o);
        std::vector<uint8_t> at(pa.at.begin(), pa.at.begin() + a.o);
        at.insert(at.end(), pb.at.begin() + b.o, pb.at.end());
        pa.at = at;
        if (a.o == 0 && pa.text.size() == pb.text.size() - b.o && b.o == 0) { pa.style = pb.style; pa.list = pb.list; pa.indent = pb.indent; pa.checked = pb.checked; }
        N->paras.erase(N->paras.begin() + a.p + 1, N->paras.begin() + b.p + 1);
    }
    caret = anchor = a;
}

void DeleteSel() { if (HasSel()) { Pos a = SelStart(), b = SelEnd(); DeleteRange(a, b); } }

void InsertPlain(const std::wstring& s, uint8_t at) {
    Para& p = P(caret.p);
    p.text.insert(caret.o, s);
    p.at.insert(p.at.begin() + caret.o, s.size(), at);
    caret.o += (int)s.size();
    anchor = caret;
}

void SplitPara() {
    Para& p = P(caret.p);
    Para np;
    np.text = p.text.substr(caret.o);
    np.at.assign(p.at.begin() + caret.o, p.at.end());
    p.text.resize(caret.o);
    p.at.resize(caret.o);
    np.list = p.list; np.indent = p.indent;
    np.style = p.style <= S_SUBHEADING ? (uint8_t)S_BODY : p.style;
    if (p.collapsed) p.collapsed = false;
    N->paras.insert(N->paras.begin() + caret.p + 1, np);
    caret = anchor = { caret.p + 1, 0 };
}

void InsertParas(std::vector<Para> v) {
    if (v.empty()) return;
    DeleteSel();
    if (v.size() == 1) {
        Para& p = P(caret.p);
        p.text.insert(caret.o, v[0].text);
        p.at.insert(p.at.begin() + caret.o, v[0].at.begin(), v[0].at.end());
        caret.o += (int)v[0].text.size(); anchor = caret;
        return;
    }
    Para& p = P(caret.p);
    std::wstring tail = p.text.substr(caret.o);
    std::vector<uint8_t> tailAt(p.at.begin() + caret.o, p.at.end());
    p.text.resize(caret.o); p.at.resize(caret.o);
    p.text += v[0].text; p.at.insert(p.at.end(), v[0].at.begin(), v[0].at.end());
    if (caret.o == 0) { p.style = v[0].style; p.list = v[0].list; p.indent = v[0].indent; p.checked = v[0].checked; }
    int at = caret.p + 1;
    for (size_t k = 1; k < v.size(); k++) N->paras.insert(N->paras.begin() + at++, v[k]);
    Para& last = P(at - 1);
    caret = anchor = { at - 1, (int)last.text.size() };
    last.text += tail; last.at.insert(last.at.end(), tailAt.begin(), tailAt.end());
}

bool IsWordChar(wchar_t c) { return iswalnum(c) || c == L'_'; }
int PrevWord(const std::wstring& t, int i) { while (i > 0 && !IsWordChar(t[i - 1])) i--; while (i > 0 && IsWordChar(t[i - 1])) i--; return i; }
int NextWord(const std::wstring& t, int i) { int n = (int)t.size(); while (i < n && !IsWordChar(t[i])) i++; while (i < n && IsWordChar(t[i])) i++; return i; }

void UncollapseAround(int i) {  // o cursor entrou numa seção recolhida: abre os títulos que a escondem
    if (pl.size() != (size_t)Count() || !pl[i].hidden) return;
    for (int k = i - 1; k >= 0; k--) if (P(k).collapsed && Level(P(k)) < Level(P(i))) { P(k).collapsed = false; }
    dirtyLayout = true;
}

int PrevVisible(int i) { for (int k = i - 1; k >= 0; k--) if (k >= (int)pl.size() || !pl[k].hidden) return k; return -1; }
int NextVisible(int i) { for (int k = i + 1; k < Count(); k++) if (k >= (int)pl.size() || !pl[k].hidden) return k; return -1; }

// parágrafos atingidos pela seleção (ou o do cursor)
void SelParas(int& a, int& b) {
    Pos s = SelStart(), e = SelEnd();
    a = s.p; b = e.p;
    if (HasSel() && e.o == 0 && e.p > s.p) b--;  // seleção que termina no começo do parágrafo não o inclui
}

std::vector<Para> SelectionParas() {
    std::vector<Para> v;
    Pos s = SelStart(), e = SelEnd();
    for (int i = s.p; i <= e.p; i++) {
        Para p = P(i);
        int a = i == s.p ? s.o : 0, b = i == e.p ? e.o : (int)p.text.size();
        p.text = p.text.substr(a, b - a);
        p.at = std::vector<uint8_t>(P(i).at.begin() + a, P(i).at.begin() + b);
        p.collapsed = false;
        v.push_back(p);
    }
    return v;
}

UINT ClipFormat() { static UINT f = RegisterClipboardFormatW(L"NotasParagrafos"); return f; }

}  // namespace

// ---------------------------------------------------------------------------
namespace editor {
// um desenho editado de novo regravou o PNG: solta a imagem guardada para ela ser lida outra vez
void ForgetImage(const std::wstring& path) {
    auto it = imgs.find(path);
    if (it != imgs.end()) { if (it->second) it->second->Release(); imgs.erase(it); }
    dirtyLayout = true;
}
bool (*OpenDrawing)(const std::wstring& path) = nullptr;

void SetFormats(const std::wstring& fam, const std::wstring& mono) {
    family = fam; monoFamily = mono;
    for (int s = 0; s < 5; s++) {
        if (fmt[s]) fmt[s]->Release();
        cjk::Format(gfx::dw, s == S_MONO ? monoFamily.c_str() : family.c_str(), nullptr, SM[s].w, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, SM[s].size, L"pt-br", &fmt[s]);
        if (fmt[s]) fmt[s]->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    }
    ReleaseLayouts();
}
void SetDark(bool d) { dark = d; ReleaseBrushes(); ReleaseLayouts(); }
void SetSearch(const std::wstring& q) { query = q; for (auto& c : query) c = towlower(c); }

Note* Current() { return N; }

void SetNote(Note* n) {
    if (n == N) return;
    N = n;
    caret = anchor = { 0, 0 };
    scrollY = 0; desiredX = -1; typingSet = false;
    ReleaseLayouts();
    if (N && N->paras.empty()) N->paras.push_back(Para{});
}

void PlaceCaretAtEnd() {
    if (!N) return;
    caret.p = Count() - 1; caret.o = (int)P(caret.p).text.size(); anchor = caret;
    wantReveal = true;
}

void Focus(bool on) { focused = on; blinkOn = true; }
bool Focused() { return focused; }
void BlinkReset() { blinkOn = true; }
void ToggleBlink() { blinkOn = !blinkOn; }
void Invalidate() { dirtyLayout = true; }
bool HasSelection() { return N && HasSel(); }

// grade da tabela que começa no parágrafo i: contorno arredondado, linhas entre as linhas e entre as colunas
void DrawTableGrid(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, int i, float ox, float oy) {
    int j = i;
    while (IsRow(j + 1)) j++;
    PL& a = pl[i]; PL& b = pl[j];
    float L = ox + a.base, R = L + a.tableW, T = oy + a.y - a.pad, B = oy + b.y + b.h + b.pad;
    br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.20f) : D2D1::ColorF(0, 0, 0, 0.16f));
    for (int k = i; k < j; k++) { if (pl[k].hidden) continue; float y = oy + pl[k].y + pl[k].h + pl[k].pad; dc->DrawLine({ L, y }, { R, y }, br, 1); }
    for (int c = 1; c < a.cols; c++) { float x = L + c * a.colW; dc->DrawLine({ x, T }, { x, B }, br, 1); }
    dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(L + 0.5f, T + 0.5f, R - 0.5f, B - 0.5f), 6, 6), br, 1);
}

// anexo: a imagem (cantos de 8) ou um cartão com a folha, o nome e o tamanho; clique duplo abre
void DrawFile(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, const Para& p, const PL& l, float ox, float oy, bool inSel, D2D1_COLOR_F accent) {
    D2D1_RECT_F r = D2D1::RectF(ox + l.x, oy + l.y, ox + l.x + l.imgW, oy + l.y + l.imgH);
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    if (ID2D1Bitmap* b = imgs.count(p.src) ? imgs[p.src] : nullptr) {
        ID2D1RoundedRectangleGeometry* g = nullptr;
        if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(r, 8, 8), &g);
        if (g) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), g), nullptr);
        dc->DrawBitmap(b, r, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (g) { dc->PopLayer(); g->Release(); }
        br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.12f) : D2D1::ColorF(0, 0, 0, 0.10f));
        dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 0.5f, r.top + 0.5f, r.right - 0.5f, r.bottom - 0.5f), 7.5f, 7.5f), br, 1);
    } else {
        br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.07f) : D2D1::ColorF(0, 0, 0, 0.035f));
        dc->FillRoundedRectangle(D2D1::RoundedRect(r, 10, 10), br);
        br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.12f) : D2D1::ColorF(0, 0, 0, 0.10f));
        dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 0.5f, r.top + 0.5f, r.right - 0.5f, r.bottom - 0.5f), 9.5f, 9.5f), br, 1);
        // folha com a ponta dobrada
        float fx = r.left + 14, fy = r.top + 11, fw = 22, fh = 30, d = 7;
        ID2D1PathGeometry* g = nullptr;
        if (f && SUCCEEDED(f->CreatePathGeometry(&g))) {
            ID2D1GeometrySink* k = nullptr; g->Open(&k);
            k->BeginFigure({ fx, fy }, D2D1_FIGURE_BEGIN_FILLED);
            k->AddLine({ fx + fw - d, fy }); k->AddLine({ fx + fw, fy + d }); k->AddLine({ fx + fw, fy + fh }); k->AddLine({ fx, fy + fh });
            k->EndFigure(D2D1_FIGURE_END_CLOSED); k->Close(); k->Release();
            br->SetColor(dark ? D2D1::ColorF(0.30f, 0.30f, 0.32f) : D2D1::ColorF(1, 1, 1));
            dc->FillGeometry(g, br);
            br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.45f) : D2D1::ColorF(0, 0, 0, 0.35f));
            dc->DrawGeometry(g, br, 1);
            dc->DrawLine({ fx + fw - d, fy }, { fx + fw - d, fy + d }, br, 1); dc->DrawLine({ fx + fw - d, fy + d }, { fx + fw, fy + d }, br, 1);
            g->Release();
        }
        std::wstring name = ShownName(p.src), info;
        WIN32_FILE_ATTRIBUTE_DATA fa{};
        if (GetFileAttributesExW(p.src.c_str(), GetFileExInfoStandard, &fa)) {
            double kb = (((ULONGLONG)fa.nFileSizeHigh << 32) | fa.nFileSizeLow) / 1024.0;
            wchar_t b[64];
            if (kb < 1024) swprintf_s(b, L"%.0f KB", std::max(1.0, kb)); else swprintf_s(b, L"%.1f MB", kb / 1024);
            info = b;
        } else info = Tr(L"Arquivo não encontrado");
        std::wstring ext = name.substr(name.find_last_of(L'.') + 1);
        if (ext.size() < name.size() && ext.size() <= 5) { for (auto& c : ext) c = towupper(c); info = ext + L" · " + info; }
        IDWriteTextFormat *t1 = nullptr, *t2 = nullptr;
        cjk::Format(gfx::dw, family.c_str(), nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13, L"pt-br", &t1);
        cjk::Format(gfx::dw, family.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11, L"pt-br", &t2);
        DWRITE_TRIMMING tr{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        if (t1) { t1->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP); t1->SetTrimming(&tr, nullptr); br->SetColor(TextColor()); dc->DrawTextW(name.c_str(), (UINT32)name.size(), t1, D2D1::RectF(r.left + 48, r.top + 9, r.right - 10, r.top + 27), br); t1->Release(); }
        if (t2) { br->SetColor(Text2()); dc->DrawTextW(info.c_str(), (UINT32)info.size(), t2, D2D1::RectF(r.left + 48, r.top + 28, r.right - 10, r.top + 44), br); t2->Release(); }
    }
    if (f) f->Release();
    if (inSel) { br->SetColor(D2D1::ColorF(accent.r, accent.g, accent.b, focused ? 0.38f : 0.18f)); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 8, 8), br); }
}

void Draw(ID2D1DeviceContext* dc, D2D1_RECT_F a) {
    area = a;
    if (!N) return;
    Clamp();
    BuildLayouts(dc, a.right - a.left);
    if (wantReveal) { Reveal(); wantReveal = false; }
    float h = a.bottom - a.top;
    scrollY = std::clamp(scrollY, 0.f, std::max(0.f, contentH - h));
    float ox = a.left, oy = a.top - scrollY;
    dc->PushAxisAlignedClip(a, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1SolidColorBrush* br = nullptr;
    dc->CreateSolidColorBrush(TextColor(), &br);
    if (semaforo::Leopard()) {
        // Leopard: o bloco amarelo das Notas do Mail — amarelo-claro, as pautas finas a cada 24 (rolam com o texto) e a
        // margem dupla avermelhada à esquerda
        br->SetColor(D2D1::ColorF(1, 0.976f, 0.73f));
        dc->FillRectangle(a, br);
        br->SetColor(D2D1::ColorF(0.55f, 0.62f, 0.80f, 0.35f));
        float y0 = a.top + 40 - std::fmod(scrollY, 24.f);
        for (float y = y0; y < a.bottom; y += 24) dc->DrawLine({ a.left, y }, { a.right, y }, br, 1);
        br->SetColor(D2D1::ColorF(0.85f, 0.35f, 0.30f, 0.45f));
        dc->DrawLine({ a.left + 10, a.top }, { a.left + 10, a.bottom }, br, 1);
        dc->DrawLine({ a.left + 13, a.top }, { a.left + 13, a.bottom }, br, 1);
        br->SetColor(TextColor());
    }

    // data no alto, centralizada e cinza
    {
        std::wstring d = LongDate(N->modified);
        IDWriteTextFormat* tf = nullptr;
        cjk::Format(gfx::dw, family.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12, L"pt-br", &tf);
        if (tf) {
            tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            br->SetColor(Text2());
            dc->DrawTextW(d.c_str(), (UINT32)d.size(), tf, D2D1::RectF(ox, oy + 6, a.right, oy + 24), br);
            tf->Release();
        }
    }
    Pos ss = SelStart(), se = SelEnd();
    bool sel = HasSel();
    D2D1_COLOR_F accent = Accent();
    for (int i = 0; i < Count(); i++) {
        PL& l = pl[i];
        if (l.hidden || !l.tl) continue;
        float py = oy + l.y;
        if (py > a.bottom || py + l.h < a.top) continue;
        Para& p = P(i);
        float px = ox + l.x;
        bool inSel = sel && i >= ss.p && i <= se.p && !(i == se.p && se.o == 0 && se.p > ss.p);
        if (p.kind == K_FILE) { DrawFile(dc, br, p, l, ox, oy, inSel, accent); continue; }
        if (p.kind == K_ROW && !IsRow(i - 1)) DrawTableGrid(dc, br, i, ox, oy);
        auto rangeRects = [&](int from, int len, D2D1_COLOR_F c, float pad) {
            if (len <= 0) return;
            UINT32 cnt = 0;
            l.tl->HitTestTextRange(from, len, 0, 0, nullptr, 0, &cnt);
            std::vector<DWRITE_HIT_TEST_METRICS> ms(cnt);
            l.tl->HitTestTextRange(from, len, 0, 0, ms.data(), cnt, &cnt);
            br->SetColor(c);
            for (auto& m : ms) dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(px + m.left - pad, py + m.top + 1, px + m.left + m.width + pad, py + m.top + m.height), 3, 3), br);
        };
        // realces
        for (int k = 0; k < (int)p.at.size();) {
            int j = k; int hcol = HiOf(p.at[k]);
            while (j < (int)p.at.size() && HiOf(p.at[j]) == hcol) j++;
            if (hcol) rangeRects(k, j - k, HiBack(hcol), 1);
            k = j;
        }
        // resultados das contas: fundo amarelado discreto
        if (i < (int)math.size() && !math[i].result.empty())
            rangeRects(l.resultAt, (int)math[i].result.size(), D2D1::ColorF(accent.r, accent.g, accent.b, dark ? 0.16f : 0.13f), 1);
        // busca
        if (!query.empty() && query.size() <= p.text.size()) {
            std::wstring low = p.text; for (auto& c : low) c = towlower(c);
            for (size_t f = low.find(query); f != std::wstring::npos; f = low.find(query, f + query.size()))
                rangeRects((int)f, (int)query.size(), D2D1::ColorF(1.0f, 0.85f, 0.1f, dark ? 0.55f : 0.6f), 0.5f);
        }
        // seleção
        if (sel && i >= ss.p && i <= se.p) {
            int from = i == ss.p ? ss.o : 0, to = i == se.p ? se.o : (int)p.text.size();
            D2D1_COLOR_F sc = focused ? D2D1::ColorF(accent.r, accent.g, accent.b, dark ? 0.40f : 0.38f) : (dark ? D2D1::ColorF(1, 1, 1, 0.14f) : D2D1::ColorF(0, 0, 0, 0.10f));
            rangeRects(from, to - from, sc, 0);
            if (i < se.p) {  // a quebra de linha também está selecionada
                D2D1_RECT_F c = CaretRectAt({ i, (int)p.text.size() });
                br->SetColor(sc);
                dc->FillRectangle(D2D1::RectF(ox + c.left, oy + c.top + 1, ox + c.left + 6, oy + c.bottom), br);
            }
        }
        // marcador da lista
        float x0, y0; DWRITE_HIT_TEST_METRICS m0;
        l.tl->HitTestTextPosition(0, FALSE, &x0, &y0, &m0);
        float cy = py + m0.top + m0.height / 2;
        float mx = ox + l.base;
        if (p.list == L_CHECK) {
            D2D1_POINT_2F c{ mx + 9, cy };
            if (p.checked) {
                br->SetColor(accent);
                dc->FillEllipse(D2D1::Ellipse(c, 8.5f, 8.5f), br);
                br->SetColor(D2D1::ColorF(1, 1, 1));
                simbolo::Draw(dc, L"checkmark", D2D1::RectF(c.x - 4.6f, c.y - 4.2f, c.x + 4.6f, c.y + 4.2f), br);
            } else {
                br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.38f) : D2D1::ColorF(0, 0, 0, 0.30f));
                dc->DrawEllipse(D2D1::Ellipse(c, 8, 8), br, 1.1f);
            }
        } else if (p.list != L_NONE && p.kind == K_TEXT) {
            std::wstring mk = p.list == L_BULLET ? L"•" : p.list == L_DASH ? L"–" : std::to_wstring(l.number) + L".";
            IDWriteTextLayout* ml = nullptr;
            gfx::dw->CreateTextLayout(mk.c_str(), (UINT32)mk.size(), fmt[p.style], 40, 100, &ml);
            if (ml) {
                ml->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, SM[p.style].line, SM[p.style].line * 0.8f);
                DWRITE_TEXT_METRICS tm{}; ml->GetMetrics(&tm);
                float x = p.list == L_NUMBER ? mx + 19 - tm.width : mx + 4;
                br->SetColor(TextColor());
                dc->DrawTextLayout(D2D1::Point2F(x, py), ml, br);
                ml->Release();
            }
        }
        // seta das seções recolhíveis (títulos): aparece com o mouse em cima ou quando recolhida
        if (Level(p) < 99 && (p.collapsed || hoverChevron == i)) {
            br->SetColor(Text2());
            float cx = ox + 11, s = 4.2f;
            ID2D1PathGeometry* g = nullptr; ID2D1Factory* f = nullptr; dc->GetFactory(&f);
            if (f && SUCCEEDED(f->CreatePathGeometry(&g))) {
                ID2D1GeometrySink* k = nullptr; g->Open(&k);
                if (p.collapsed) { k->BeginFigure({ cx - s / 2, cy - s }, D2D1_FIGURE_BEGIN_HOLLOW); k->AddLine({ cx + s / 2, cy }); k->AddLine({ cx - s / 2, cy + s }); }
                else { k->BeginFigure({ cx - s, cy - s / 2 }, D2D1_FIGURE_BEGIN_HOLLOW); k->AddLine({ cx, cy + s / 2 }); k->AddLine({ cx + s, cy - s / 2 }); }
                k->EndFigure(D2D1_FIGURE_END_OPEN); k->Close(); k->Release();
                ID2D1StrokeStyle* rs = nullptr;
                f->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND), nullptr, 0, &rs);
                dc->DrawGeometry(g, br, 1.6f, rs);
                if (rs) rs->Release();
                g->Release();
            }
            if (f) f->Release();
        }
        br->SetColor(TextColor());
        dc->DrawTextLayout(D2D1::Point2F(px, py), l.tl, br, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }
    // cursor amarelo
    if (focused && blinkOn && !sel && caret.p < (int)pl.size() && !pl[caret.p].hidden) {
        D2D1_RECT_F c = CaretRectAt(caret);
        br->SetColor(accent);
        dc->FillRectangle(D2D1::RectF(ox + c.left - 0.5f, oy + c.top, ox + c.left + 1.5f, oy + c.bottom), br);
    }
    br->Release();
    dc->PopAxisAlignedClip();
}

void CaretScreenRect(D2D1_RECT_F& r) {
    D2D1_RECT_F c = CaretRectAt(caret);
    r = { area.left + c.left, area.top - scrollY + c.top, area.left + c.right, area.top - scrollY + c.bottom };
}

void OnWheel(float dy) { scrollY = std::max(0.f, scrollY - dy); }

static int ChevronAt(D2D1_POINT_2F p) {
    if (!N || pl.size() != (size_t)Count() || p.x < area.left || p.x > area.left + MARGIN) return -1;
    float y = p.y - area.top + scrollY;
    for (int i = 0; i < Count(); i++)
        if (!pl[i].hidden && Level(P(i)) < 99 && y >= pl[i].y && y < pl[i].y + SM[P(i).style].line + 2) return i;
    return -1;
}
bool OverChevronArea(D2D1_POINT_2F p) { return ChevronAt(p) >= 0; }

void OnMouseMove(D2D1_POINT_2F p, bool dragging) {
    if (!N) return;
    if (!dragging) {
        // a seta aparece com o mouse em qualquer lugar da linha do título
        int hc = -1;
        if (pl.size() == (size_t)Count() && p.x >= area.left && p.x < area.right) {
            float y = p.y - area.top + scrollY;
            for (int i = 0; i < Count(); i++)
                if (!pl[i].hidden && Level(P(i)) < 99 && y >= pl[i].y && y < pl[i].y + pl[i].h) { hc = i; break; }
        }
        if (hc != hoverChevron) { hoverChevron = hc; RequestPaint(); }
        return;
    }
    Pos q = HitContent(p.x - area.left, p.y - area.top + scrollY);
    if (clickMode == 2) {
        const std::wstring& t = P(q.p).text;
        if (Less(q, clickA)) { caret = { q.p, PrevWord(t, std::min(q.o + 1, (int)t.size())) }; anchor = clickB; }
        else { caret = { q.p, NextWord(t, q.o) }; anchor = clickA; }
    } else if (clickMode == 3) {
        if (q.p < clickA.p) { caret = { q.p, 0 }; anchor = clickB; }
        else { caret = { q.p, (int)P(q.p).text.size() }; anchor = clickA; }
    } else caret = q;
    if (p.y < area.top) scrollY = std::max(0.f, scrollY - 12);
    if (p.y > area.bottom) scrollY += 12;
    desiredX = -1;
    RequestPaint();
}

void OnMouseDown(D2D1_POINT_2F p, bool shift, int clicks) {
    if (!N) return;
    focused = true; blinkOn = true;
    int ch = ChevronAt(p);
    if (ch >= 0 && clicks == 1) {
        Checkpoint(3);
        P(ch).collapsed = !P(ch).collapsed;
        if (P(ch).collapsed && caret.p > ch) { caret = anchor = { ch, (int)P(ch).text.size() }; }
        N->dirty = true; dirtyLayout = true; NoteEdited(N);
        return;
    }
    float cx = p.x - area.left, cy = p.y - area.top + scrollY;
    // bolinha da lista de verificação
    for (int i = 0; i < Count() && pl.size() == (size_t)Count(); i++) {
        if (pl[i].hidden || P(i).list != L_CHECK || !pl[i].tl) continue;
        float x0, y0; DWRITE_HIT_TEST_METRICS m0;
        pl[i].tl->HitTestTextPosition(0, FALSE, &x0, &y0, &m0);
        float ccx = pl[i].base + 9, ccy = pl[i].y + m0.top + m0.height / 2;
        if (std::fabs(cx - ccx) <= 11 && std::fabs(cy - ccy) <= 11) {
            Checkpoint(3);
            P(i).checked = !P(i).checked;
            Changed();
            wantReveal = false;
            return;
        }
    }
    Pos q = HitContent(cx, cy);
    typingSet = false; desiredX = -1;
    if (P(q.p).kind == K_FILE) {  // anexo: clique põe o cursor nele; clique duplo abre no programa padrão
        caret = q; if (!shift) anchor = q;
        clickMode = 1;
        if (clicks == 2 && !(OpenDrawing && OpenDrawing(P(q.p).src))) ShellExecuteW(nullptr, L"open", P(q.p).src.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        RequestPaint();
        return;
    }
    const std::wstring& t = P(q.p).text;
    if (clicks == 2) {
        int a = q.o, b = q.o;
        if (a < (int)t.size() && IsWordChar(t[a])) { while (a > 0 && IsWordChar(t[a - 1])) a--; while (b < (int)t.size() && IsWordChar(t[b])) b++; }
        else if (b < (int)t.size()) b++;
        anchor = { q.p, a }; caret = { q.p, b };
        clickA = anchor; clickB = caret; clickMode = 2;
    } else if (clicks >= 3) {
        anchor = { q.p, 0 }; caret = { q.p, (int)t.size() };
        clickA = anchor; clickB = caret; clickMode = 3;
    } else {
        caret = q;
        if (!shift) anchor = q;
        clickMode = 1;
    }
    RequestPaint();
}

void SelectAll() { if (!N) return; anchor = { 0, 0 }; caret = { Count() - 1, (int)P(Count() - 1).text.size() }; }

void Copy(bool cut) {
    if (!N || !HasSel()) return;
    auto v = SelectionParas();
    std::wstring plain;
    for (size_t i = 0; i < v.size(); i++) { if (i) plain += L"\r\n"; plain += v[i].text; }
    std::wstring rich = store::Serialize(v);
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        auto put = [](UINT fmtId, const std::wstring& s) {
            HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, (s.size() + 1) * sizeof(wchar_t));
            if (!g) return;
            memcpy(GlobalLock(g), s.c_str(), (s.size() + 1) * sizeof(wchar_t));
            GlobalUnlock(g);
            if (!SetClipboardData(fmtId, g)) GlobalFree(g);
        };
        put(CF_UNICODETEXT, plain);
        put(ClipFormat(), rich);
        CloseClipboard();
    }
    if (cut) { Checkpoint(3); DeleteSel(); Changed(); }
}

void Paste() {
    if (!N || !OpenClipboard(nullptr)) return;
    std::vector<Para> v;
    if (HANDLE h = GetClipboardData(ClipFormat())) {
        if (auto* p = (const wchar_t*)GlobalLock(h)) { v = store::Deserialize(p); GlobalUnlock(h); }
    } else if (HANDLE h2 = GetClipboardData(CF_UNICODETEXT)) {
        if (auto* p = (const wchar_t*)GlobalLock(h2)) {
            std::wstring s = p; GlobalUnlock(h2);
            uint8_t at = AttrAtCaret() & ~0x70;  // texto colado sem realce
            size_t a = 0;
            for (;;) {
                size_t e = s.find(L'\n', a);
                std::wstring line = s.substr(a, e == std::wstring::npos ? std::wstring::npos : e - a);
                if (!line.empty() && line.back() == L'\r') line.pop_back();
                Para q; q.text = line; q.at.assign(line.size(), at);
                q.style = S_BODY;
                v.push_back(q);
                if (e == std::wstring::npos) break;
                a = e + 1;
            }
            if (v.size() == 1) v[0].style = P(caret.p).style;
        }
    }
    CloseClipboard();
    if (v.empty()) return;
    Checkpoint(3);
    InsertParas(v);
    typingSet = false;
    Changed();
}

void Undo() {
    if (!N || N->undo.empty()) return;
    N->redo.push_back({ N->paras, caret.p, caret.o, anchor.p, anchor.o });
    auto s = N->undo.back(); N->undo.pop_back();
    N->paras = s.paras; caret = { s.cp, s.co }; anchor = { s.ap, s.ao };
    N->lastEditKind = 0;
    Clamp(); Changed();
}
void Redo() {
    if (!N || N->redo.empty()) return;
    N->undo.push_back({ N->paras, caret.p, caret.o, anchor.p, anchor.o });
    auto s = N->redo.back(); N->redo.pop_back();
    N->paras = s.paras; caret = { s.cp, s.co }; anchor = { s.ap, s.ao };
    N->lastEditKind = 0;
    Clamp(); Changed();
}

bool InTable() { return N && IsRow(caret.p); }

void InsertTable(int rows, int cols) {
    if (!N) return;
    Checkpoint(3);
    DeleteSel();
    Para row; row.kind = K_ROW; row.text.assign(std::max(1, cols) - 1, L'\t'); row.at.assign(row.text.size(), 0);
    int i = caret.p, first;
    if (P(i).kind == K_TEXT && P(i).text.empty()) { P(i) = row; first = i; }
    else {
        if (P(i).kind == K_TEXT && caret.o > 0 && caret.o < (int)P(i).text.size()) { SplitPara(); i = caret.p - 1; }
        else if (P(i).kind == K_TEXT && caret.o == 0 && !P(i).text.empty()) i--;  // no começo do parágrafo: a tabela vem antes
        N->paras.insert(N->paras.begin() + i + 1, row);
        first = i + 1;
    }
    for (int k = 1; k < rows; k++) N->paras.insert(N->paras.begin() + first + k, row);
    int last = first + rows - 1;
    if (last + 1 >= Count() || P(last + 1).kind != K_TEXT) N->paras.insert(N->paras.begin() + last + 1, Para{});
    caret = anchor = { first, 0 };
    typingSet = false;
    Changed();
}

// linha/coluna a mais ou a menos e "Converter em Texto" (menu do botão da tabela com o cursor dentro dela)
void Table(int op) {
    if (!N || !IsRow(caret.p)) return;
    Checkpoint(3);
    anchor = caret;
    int a = caret.p, b = caret.p;
    while (IsRow(a - 1)) a--;
    while (IsRow(b + 1)) b++;
    int col = (int)std::count(P(caret.p).text.begin(), P(caret.p).text.begin() + caret.o, L'\t');
    int cols = TableCols(caret.p);
    auto cellEnd = [](const std::wstring& t, int c) {  // posição do fim da célula c (onde fica o TAB seguinte)
        size_t k = 0;
        for (int n = 0; n < c; n++) { k = t.find(L'\t', k); if (k == std::wstring::npos) return t.size(); k++; }
        size_t e = t.find(L'\t', k);
        return e == std::wstring::npos ? t.size() : e;
    };
    auto cellStart = [](const std::wstring& t, int c) {
        size_t k = 0;
        for (int n = 0; n < c; n++) { k = t.find(L'\t', k); if (k == std::wstring::npos) return t.size(); k++; }
        return k;
    };
    switch (op) {
    case T_ADD_ROW: {
        Para nr; nr.kind = K_ROW; nr.text.assign(cols - 1, L'\t'); nr.at.assign(nr.text.size(), 0);
        N->paras.insert(N->paras.begin() + caret.p + 1, nr);
        caret = anchor = { caret.p + 1, 0 };
        break;
    }
    case T_ADD_COL:
        for (int i = a; i <= b; i++) {
            Para& p = P(i);
            while (Tabs(p.text) < cols - 1) { p.text += L'\t'; p.at.push_back(0); }  // linhas curtas completam
            size_t e = cellEnd(p.text, col);
            p.text.insert(e, 1, L'\t'); p.at.insert(p.at.begin() + e, 0);
        }
        caret = anchor = { caret.p, (int)cellEnd(P(caret.p).text, col) + 1 };
        break;
    case T_DEL_ROW:
        if (a == b) { P(a).kind = K_TEXT; P(a).text.clear(); P(a).at.clear(); caret = anchor = { a, 0 }; break; }
        N->paras.erase(N->paras.begin() + caret.p);
        caret.p = std::min(caret.p, b - 1); caret.o = 0; anchor = caret;
        break;
    case T_DEL_COL:
        if (cols <= 1) break;
        for (int i = a; i <= b; i++) {
            Para& p = P(i);
            size_t s = cellStart(p.text, col), e = cellEnd(p.text, col);
            if (s > p.text.size()) continue;
            // leva junto um TAB: o do fim (ou o de antes, na última coluna)
            if (e < p.text.size()) e++; else if (s > 0) s--;
            p.text.erase(s, e - s); p.at.erase(p.at.begin() + s, p.at.begin() + e);
        }
        caret = anchor = { caret.p, 0 };
        break;
    case T_TO_TEXT:
        for (int i = a; i <= b; i++) P(i).kind = K_TEXT;
        break;
    }
    Clamp();
    Changed();
}

void InsertAttachment(const std::wstring& path) {
    if (!N || path.empty()) return;
    Checkpoint(3);
    DeleteSel();
    Para f; f.kind = K_FILE; f.src = path;
    int i = caret.p, at;
    if (P(i).kind == K_TEXT && P(i).text.empty() && P(i).list == L_NONE) { P(i) = f; at = i; }
    else {
        if (P(i).kind == K_TEXT && caret.o > 0 && caret.o < (int)P(i).text.size()) { SplitPara(); i = caret.p - 1; }
        else if (P(i).kind == K_TEXT && caret.o == 0 && !P(i).text.empty()) i--;
        N->paras.insert(N->paras.begin() + i + 1, f);
        at = i + 1;
    }
    if (at + 1 >= Count() || P(at + 1).kind != K_TEXT) N->paras.insert(N->paras.begin() + at + 1, Para{});
    caret = anchor = { at + 1, 0 };
    typingSet = false;
    Changed();
}

// Revisar (sem IA): espaços repetidos, espaço antes de pontuação, espaço depois de , ; ! ? (e de . antes de maiúscula),
// maiúscula no começo do parágrafo e depois de . ! ?, "i" sozinho vira "I" em inglês. Os atributos andam junto.
static bool Proof(Para& p) {
    const std::wstring& s = p.text;
    std::wstring t; std::vector<uint8_t> at;
    bool row = p.kind == K_ROW;
    auto punct = [](wchar_t c) { return c == L',' || c == L'.' || c == L';' || c == L':' || c == L'!' || c == L'?' || c == L')'; };
    bool capNext = !row;
    for (size_t i = 0; i < s.size(); i++) {
        wchar_t c = s[i];
        uint8_t a = i < p.at.size() ? p.at[i] : 0;
        wchar_t nx = i + 1 < s.size() ? s[i + 1] : 0;
        if (c == L' ') {
            if (t.empty() || t.back() == L' ' || t.back() == L'\t' || nx == 0 || nx == L'\t' || (punct(nx) && nx != L')')) continue;
            t += c; at.push_back(a); continue;
        }
        if (c == L'\t') { while (!t.empty() && t.back() == L' ') { t.pop_back(); at.pop_back(); } t += c; at.push_back(a); continue; }
        if (iswalpha(c)) {
            if (capNext) c = towupper(c);
            capNext = false;
            if (idioma::En() && c == L'i' && (t.empty() || t.back() == L' ') && (nx == 0 || nx == L' ' || nx == L'\'' || nx == L'\u2019' || punct(nx))) c = L'I';
        } else if (iswdigit(c)) capNext = false;
        t += c; at.push_back(a);
        if (c == L'.' || c == L'!' || c == L'?') {
            bool sentence = c != L'.' || (nx == L' ' || nx == 0);
            if (!row && sentence) capNext = true;
        }
        if ((c == L',' || c == L';' || c == L'!' || c == L'?') && iswalpha(nx)) { t += L' '; at.push_back(a); }
        else if (c == L'.' && iswupper(nx) && !(i > 0 && iswupper(s[i - 1]))) { t += L' '; at.push_back(a); if (!row) capNext = true; }
    }
    while (!t.empty() && t.back() == L' ') { t.pop_back(); at.pop_back(); }
    if (t == s) return false;
    p.text = t; p.at = at;
    return true;
}

bool Transform(int kind) {
    if (!N) return false;
    Checkpoint(3);
    size_t undoN = N->undo.size();
    bool sel = HasSel();
    Pos ss = SelStart(), se = SelEnd();
    int a, b;
    if (kind == W_LIST || kind == W_TABLE || sel) SelParas(a, b); else { a = 0; b = Count() - 1; }
    bool changed = false;
    for (int i = a; i <= b; i++) {
        Para& p = P(i);
        if (p.kind == K_FILE) continue;
        int from = 0, to = (int)p.text.size();
        if (sel && (kind == W_UPPER || kind == W_LOWER || kind == W_TITLE)) { if (i == ss.p) from = ss.o; if (i == se.p) to = se.o; }
        switch (kind) {
        case W_UPPER: case W_LOWER: case W_TITLE: {
            bool start = true;
            for (int k = from; k < to; k++) {
                wchar_t c = p.text[k], n = c;
                if (kind == W_UPPER) n = towupper(c);
                else if (kind == W_LOWER) n = towlower(c);
                else { n = start ? towupper(c) : towlower(c); }
                start = !iswalnum(c) && c != L'\'' && c != L'\u2019';
                if (n != c) { p.text[k] = n; changed = true; }
            }
            break;
        }
        case W_LIST:
            if (p.kind == K_TEXT && !p.text.empty() && p.list != L_BULLET) { p.list = L_BULLET; if (p.style <= S_SUBHEADING) p.style = S_BODY; changed = true; }
            break;
        case W_TABLE: {
            if (p.kind != K_TEXT || p.text.empty()) break;
            // separador: TAB, senão ";", "|" ou ","; os espaços em volta somem
            wchar_t sep = 0;
            for (wchar_t c : { L'\t', L';', L'|', L',' }) if (p.text.find(c) != std::wstring::npos) { sep = c; break; }
            std::wstring t; std::vector<uint8_t> at;
            for (size_t k = 0; k < p.text.size(); k++) {
                wchar_t c = p.text[k] == sep ? L'\t' : p.text[k];
                if (c == L' ' && (t.empty() || t.back() == L'\t' || (k + 1 < p.text.size() && p.text[k + 1] == sep))) continue;
                t += c; at.push_back(k < p.at.size() ? p.at[k] : 0);
            }
            p.text = t; p.at = at; p.kind = K_ROW; p.list = L_NONE; p.style = S_BODY; p.collapsed = false;
            changed = true;
            break;
        }
        case W_PROOF:
            if (Proof(p)) changed = true;
            break;
        }
    }
    if (!changed) { if (N->undo.size() > undoN - 1 && N->undo.size() == undoN) N->undo.pop_back(); return false; }
    if (kind == W_TABLE && (b + 1 >= Count() || P(b + 1).kind != K_TEXT)) N->paras.insert(N->paras.begin() + b + 1, Para{});
    Clamp();
    Changed();
    return true;
}

// ---- Ferramentas de Escrita com IA
// Markdown simples nos dois sentidos: o modelo entende e devolve listas, títulos e tabelas que viram parágrafos de verdade
static std::wstring ParaMd(const Para& p, int numero) {
    if (p.kind == K_FILE) return L"";
    if (p.kind == K_ROW) {
        std::wstring t = L"| ";
        for (wchar_t c : p.text) t += c == L'\t' ? std::wstring(L" | ") : std::wstring(1, c);
        return t + L" |";
    }
    std::wstring pre(p.indent * 2, L' ');
    if (p.style == S_TITLE) pre += L"# ";
    else if (p.style == S_HEADING) pre += L"## ";
    else if (p.style == S_SUBHEADING) pre += L"### ";
    if (p.list == L_BULLET || p.list == L_DASH) pre += L"- ";
    else if (p.list == L_NUMBER) pre += std::to_wstring(numero) + L". ";
    else if (p.list == L_CHECK) pre += p.checked ? L"- [x] " : L"- [ ] ";
    return pre + p.text;
}

editor::AiRange AiTake() {
    editor::AiRange r;
    if (!N) return r;
    r.n = N;
    r.all = !HasSel();
    if (r.all) { r.a = 0; r.b = Count() - 1; } else SelParas(r.a, r.b);
    int num = 0;
    for (int i = r.a; i <= r.b; i++) {
        const Para& p = P(i);
        num = p.list == L_NUMBER ? num + 1 : 0;
        std::wstring l = ParaMd(p, num);
        if (p.kind == K_FILE) continue;
        r.antes += l + L"\n";
    }
    while (!r.antes.empty() && r.antes.back() == L'\n') r.antes.pop_back();
    return r;
}

static std::wstring SemMarcas(std::wstring t) {  // **negrito**, __x__ e `código` viram texto puro
    for (const wchar_t* m : { L"**", L"__", L"`" }) for (size_t i; (i = t.find(m)) != std::wstring::npos;) t.erase(i, wcslen(m));
    return t;
}

bool AiReplace(const editor::AiRange& r, const std::wstring& md) {
    if (!N || N != r.n || r.b >= Count()) return false;
    {   // a nota mudou enquanto a IA escrevia: não troca (o texto novo iria para o lugar errado)
        editor::AiRange agora; agora.n = N; agora.a = r.a; agora.b = r.b;
        int num = 0; std::wstring t;
        for (int i = r.a; i <= r.b; i++) { const Para& p = P(i); num = p.list == L_NUMBER ? num + 1 : 0; if (p.kind != K_FILE) t += ParaMd(p, num) + L"\n"; }
        while (!t.empty() && t.back() == L'\n') t.pop_back();
        if (t != r.antes) return false;
    }
    std::vector<Para> novos;
    std::wstring s = md;
    // o modelo às vezes embrulha tudo num bloco ```
    if (s.rfind(L"```", 0) == 0) { size_t q = s.find(L'\n'); s = q == std::wstring::npos ? L"" : s.substr(q + 1); }
    if (s.size() >= 3 && s.compare(s.size() - 3, 3, L"```") == 0) s.resize(s.size() - 3);
    size_t i = 0;
    bool brancoAntes = false;
    while (i <= s.size()) {
        size_t f = s.find(L'\n', i); if (f == std::wstring::npos) f = s.size();
        std::wstring l = s.substr(i, f - i);
        i = f + 1;
        if (!l.empty() && l.back() == L'\r') l.pop_back();
        size_t ind = 0; while (ind < l.size() && (l[ind] == L' ' || l[ind] == L'\t')) ind++;
        std::wstring t = l.substr(ind);
        if (t.empty()) { brancoAntes = true; if (f >= s.size()) break; continue; }
        Para p;
        p.indent = (uint8_t)std::min<size_t>(ind / 2, 4);
        if (t[0] == L'|') {  // tabela; a linha de traços (|---|) some
            bool tracos = t.find_first_not_of(L"|-: ") == std::wstring::npos;
            if (tracos) continue;
            std::wstring c = t.substr(1);
            if (!c.empty() && c.back() == L'|') c.pop_back();
            std::wstring cel, o;
            for (size_t k = 0; k <= c.size(); k++) {
                if (k == c.size() || c[k] == L'|') {
                    size_t x = cel.find_first_not_of(L' '), y = cel.find_last_not_of(L' ');
                    o += (x == std::wstring::npos ? L"" : cel.substr(x, y - x + 1));
                    if (k < c.size()) o += L'\t';
                    cel.clear();
                } else cel += c[k];
            }
            p.kind = K_ROW; p.text = SemMarcas(o); p.indent = 0;
        } else {
            if (t.rfind(L"### ", 0) == 0) { p.style = S_SUBHEADING; t = t.substr(4); }
            else if (t.rfind(L"## ", 0) == 0) { p.style = S_HEADING; t = t.substr(3); }
            else if (t.rfind(L"# ", 0) == 0) { p.style = S_TITLE; t = t.substr(2); }
            if (t.rfind(L"- [ ] ", 0) == 0 || t.rfind(L"* [ ] ", 0) == 0) { p.list = L_CHECK; t = t.substr(6); }
            else if (t.rfind(L"- [x] ", 0) == 0 || t.rfind(L"- [X] ", 0) == 0) { p.list = L_CHECK; p.checked = true; t = t.substr(6); }
            else if (t.rfind(L"- ", 0) == 0 || t.rfind(L"* ", 0) == 0 || t.rfind(L"• ", 0) == 0) { p.list = L_BULLET; t = t.substr(2); }
            else {
                size_t d = 0; while (d < t.size() && iswdigit(t[d])) d++;
                if (d > 0 && d < 4 && d + 1 < t.size() && (t[d] == L'.' || t[d] == L')') && t[d + 1] == L' ') { p.list = L_NUMBER; t = t.substr(d + 2); }
            }
            p.text = SemMarcas(t);
            if (p.list != L_NONE && p.style != S_BODY) p.style = S_BODY;
        }
        // um parágrafo em branco entre blocos de texto comum (como o modelo separa): fica um vazio na nota também
        if (brancoAntes && !novos.empty() && p.list == L_NONE && p.kind == K_TEXT && novos.back().list == L_NONE && novos.back().kind == K_TEXT)
            novos.push_back(Para{});
        brancoAntes = false;
        p.at.assign(p.text.size(), 0);
        novos.push_back(std::move(p));
        if (f >= s.size()) break;
    }
    if (novos.empty()) return false;
    // a nota inteira: o primeiro parágrafo continua título (o nome da nota na lista)
    if (r.all && r.a == 0 && P(0).style == S_TITLE && novos[0].kind == K_TEXT && novos[0].list == L_NONE) novos[0].style = S_TITLE;
    // anexos dentro do trecho ficam (no fim do texto novo)
    for (int k = r.a; k <= r.b; k++) if (P(k).kind == K_FILE) novos.push_back(P(k));
    Checkpoint(3);
    N->paras.erase(N->paras.begin() + r.a, N->paras.begin() + r.b + 1);
    N->paras.insert(N->paras.begin() + r.a, novos.begin(), novos.end());
    if (N->paras.empty()) N->paras.push_back(Para{});
    // a seleção cobre o texto novo (como no Mac: dá para ver o que mudou e desfazer com Ctrl+Z)
    int fim = r.a + (int)novos.size() - 1;
    anchor = { r.a, 0 };
    caret = { fim, (int)P(fim).text.size() };
    Clamp();
    Changed();
    return true;
}

int CurrentStyle() { return N ? P(caret.p).style : S_BODY; }
int CurrentList() { return N ? P(caret.p).list : L_NONE; }
uint8_t CurrentAttrs() {
    if (!N) return 0;
    if (HasSel()) { Pos s = SelStart(); return s.o < (int)P(s.p).at.size() ? P(s.p).at[s.o] : 0; }
    return AttrAtCaret();
}

void ApplyStyle(int style) {
    if (!N) return;
    Checkpoint(3);
    int a, b; SelParas(a, b);
    for (int i = a; i <= b; i++) { P(i).style = (uint8_t)style; if (style > S_SUBHEADING) P(i).collapsed = false; }
    Changed();
}

void ToggleAttr(uint8_t flag) {
    if (!N) return;
    if (!HasSel()) { typing = AttrAtCaret() ^ flag; typingSet = true; return; }
    Checkpoint(3);
    Pos s = SelStart(), e = SelEnd();
    bool all = true;
    for (int i = s.p; i <= e.p; i++) {
        int from = i == s.p ? s.o : 0, to = i == e.p ? e.o : (int)P(i).text.size();
        for (int k = from; k < to; k++) if (!(P(i).at[k] & flag)) all = false;
    }
    for (int i = s.p; i <= e.p; i++) {
        int from = i == s.p ? s.o : 0, to = i == e.p ? e.o : (int)P(i).text.size();
        for (int k = from; k < to; k++) P(i).at[k] = all ? (P(i).at[k] & ~flag) : (P(i).at[k] | flag);
    }
    Changed();
}

void SetHighlight(int h) {
    if (!N) return;
    if (!HasSel()) { typing = WithHi(AttrAtCaret(), h); typingSet = true; return; }
    Checkpoint(3);
    Pos s = SelStart(), e = SelEnd();
    for (int i = s.p; i <= e.p; i++) {
        int from = i == s.p ? s.o : 0, to = i == e.p ? e.o : (int)P(i).text.size();
        for (int k = from; k < to; k++) P(i).at[k] = WithHi(P(i).at[k], h);
    }
    Changed();
}

void ToggleList(int kind) {
    if (!N) return;
    Checkpoint(3);
    int a, b; SelParas(a, b);
    bool all = true;
    for (int i = a; i <= b; i++) if (P(i).list != kind) all = false;
    for (int i = a; i <= b; i++) {
        if (P(i).kind != K_TEXT) continue;
        P(i).list = all ? (uint8_t)L_NONE : (uint8_t)kind;
        if (!all && P(i).style != S_MONO) P(i).style = P(i).style <= S_SUBHEADING && P(i).text.empty() ? (uint8_t)S_BODY : P(i).style;
        P(i).checked = false;
    }
    Changed();
}

void Indent(int d) {
    if (!N) return;
    Checkpoint(3);
    int a, b; SelParas(a, b);
    for (int i = a; i <= b; i++) P(i).indent = (uint8_t)std::clamp(P(i).indent + d, 0, 8);
    Changed();
}

void ToggleChecked() {
    if (!N) return;
    Checkpoint(3);
    int a, b; SelParas(a, b);
    for (int i = a; i <= b; i++) if (P(i).list == L_CHECK) P(i).checked = !P(i).checked;
    Changed();
}

void OnChar(wchar_t c) {
    if (!N || !focused) return;
    if ((c == L'\r' || c == L'\n') && !HasSel() && (IsFile(caret.p) || IsRow(caret.p))) {
        Checkpoint(3);
        int i = caret.p;
        if (IsFile(i)) { N->paras.insert(N->paras.begin() + i + 1, Para{}); caret = anchor = { i + 1, 0 }; }
        else {
            Para& r = P(i);
            bool empty = r.text.find_first_not_of(L'\t') == std::wstring::npos;
            if (empty && !IsRow(i + 1) && IsRow(i - 1)) { r.kind = K_TEXT; r.text.clear(); r.at.clear(); caret = anchor = { i, 0 }; }  // sai da tabela
            else {
                Para nr; nr.kind = K_ROW; nr.text.assign(TableCols(i) - 1, L'\t'); nr.at.assign(nr.text.size(), 0);
                N->paras.insert(N->paras.begin() + i + 1, nr);
                caret = anchor = { i + 1, 0 };
            }
        }
        typingSet = false;
        Changed();
        return;
    }
    if (c == L'\r' || c == L'\n') {
        Checkpoint(3);
        DeleteSel();
        Para& p = P(caret.p);
        if (p.list != L_NONE && p.text.empty()) {  // Enter na linha vazia da lista: sai da lista (ou volta um nível)
            if (p.indent > 0) p.indent--; else p.list = L_NONE;
        } else if (caret.o == 0 && !p.text.empty()) {  // no começo: abre uma linha em cima com a mesma formatação
            Para np = p; np.text.clear(); np.at.clear(); np.checked = false; np.collapsed = false;
            N->paras.insert(N->paras.begin() + caret.p, np);
            caret = anchor = { caret.p + 1, 0 };
        } else SplitPara();
        typingSet = false;
        Changed();
        return;
    }
    if (c == L'\t') return;  // tratado em OnKey
    if (c < 32 || c == 127) return;
    Checkpoint(1);
    if (!HasSel() && IsFile(caret.p)) {  // digitou com o cursor no anexo: o texto vai num parágrafo novo depois dele
        Checkpoint(3);
        N->paras.insert(N->paras.begin() + caret.p + 1, Para{});
        caret = anchor = { caret.p + 1, 0 };
    }
    uint8_t at = AttrAtCaret();
    DeleteSel();
    Para& p = P(caret.p);
    // atalhos de digitação: "* " / "- " / "1. " viram listas, como no Mac
    if (c == L' ' && p.list == L_NONE && caret.o == (int)p.text.size() && p.style != S_MONO) {
        int kind = -1;
        if (p.text == L"*" || p.text == L"•") kind = L_BULLET;
        else if (p.text == L"-" || p.text == L"–") kind = L_DASH;
        else if (p.text == L"1." || p.text == L"1)") kind = L_NUMBER;
        else if (p.text == L"[]" || p.text == L"[ ]") kind = L_CHECK;
        if (kind >= 0 && p.kind == K_TEXT) {
            Checkpoint(3);
            p.text.clear(); p.at.clear(); p.list = (uint8_t)kind; caret.o = 0; anchor = caret;
            if (p.style <= S_SUBHEADING) p.style = S_BODY;
            Changed();
            return;
        }
    }
    InsertPlain(std::wstring(1, c), at);
    typing = at;  // continua com o mesmo atributo
    Changed();
}

bool OnKey(UINT vk) {
    if (!N || !focused) return false;
    bool ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0;
    blinkOn = true;
    auto move = [&](Pos q, bool keepX = false) {
        caret = q;
        if (!shift) anchor = q;
        if (!keepX) desiredX = -1;
        typingSet = false;
        if (q.p < (int)pl.size() && pl[q.p].hidden) UncollapseAround(q.p);
        wantReveal = true;
    };
    // atalhos de formatação (Ctrl no lugar do Cmd)
    if (ctrl && shift) {
        switch (vk) {
        case 'T': ApplyStyle(S_TITLE); return true;
        case 'H': ApplyStyle(S_HEADING); return true;
        case 'J': ApplyStyle(S_SUBHEADING); return true;
        case 'B': ApplyStyle(S_BODY); return true;
        case 'M': ApplyStyle(S_MONO); return true;
        case '7': ToggleList(L_BULLET); return true;
        case '8': ToggleList(L_DASH); return true;
        case '9': ToggleList(L_NUMBER); return true;
        case 'L': ToggleList(L_CHECK); return true;
        case 'U': ToggleChecked(); return true;
        case 'X': ToggleAttr(A_STRIKE); return true;
        case 'Z': Redo(); return true;
        }
    }
    if (ctrl && !shift) {
        switch (vk) {
        case 'B': ToggleAttr(A_BOLD); return true;
        case 'I': ToggleAttr(A_ITALIC); return true;
        case 'U': ToggleAttr(A_UNDER); return true;
        case 'A': SelectAll(); return true;
        case 'C': Copy(false); return true;
        case 'X': Copy(true); return true;
        case 'V': Paste(); return true;
        case 'Z': Undo(); return true;
        case 'Y': Redo(); return true;
        case VK_OEM_6: Indent(1); return true;   // Ctrl+]
        case VK_OEM_4: Indent(-1); return true;  // Ctrl+[
        }
    }
    Para& p = P(caret.p);
    auto jump = [&](Pos q) { caret = anchor = q; desiredX = -1; typingSet = false; wantReveal = true; };
    auto eraseFile = [&](int i) {  // apaga o anexo inteiro (fica pelo menos um parágrafo)
        Checkpoint(3);
        if (Count() == 1) { P(0) = Para{}; caret = anchor = { 0, 0 }; }
        else { N->paras.erase(N->paras.begin() + i); int k = std::min(i, Count() - 1); caret = anchor = { k, k < i ? (int)P(k).text.size() : 0 }; }
        Changed();
    };
    if (!HasSel() && !ctrl) {
        // tabela: Tab/Shift+Tab de célula em célula (Tab na última célula abre uma linha nova)
        if (vk == VK_TAB && IsRow(caret.p)) {
            const std::wstring& t = p.text;
            if (!shift) {
                size_t k = t.find(L'\t', caret.o);
                if (k != std::wstring::npos) jump({ caret.p, (int)k + 1 });
                else if (IsRow(caret.p + 1)) jump({ caret.p + 1, 0 });
                else {
                    Checkpoint(3);
                    Para nr; nr.kind = K_ROW; nr.text.assign(TableCols(caret.p) - 1, L'\t'); nr.at.assign(nr.text.size(), 0);
                    N->paras.insert(N->paras.begin() + caret.p + 1, nr);
                    jump({ caret.p + 1, 0 });
                    Changed();
                }
            } else {
                int s0 = caret.o == 0 ? 0 : (int)(t.rfind(L'\t', caret.o - 1) == std::wstring::npos ? 0 : t.rfind(L'\t', caret.o - 1) + 1);
                if (s0 > 0) { size_t k = s0 >= 2 ? t.rfind(L'\t', s0 - 2) : std::wstring::npos; jump({ caret.p, k == std::wstring::npos ? 0 : (int)k + 1 }); }
                else if (IsRow(caret.p - 1)) { const std::wstring& u = P(caret.p - 1).text; size_t k = u.rfind(L'\t'); jump({ caret.p - 1, k == std::wstring::npos ? 0 : (int)k + 1 }); }
            }
            return true;
        }
        if (vk == VK_BACK || vk == VK_DELETE) {
            if (IsFile(caret.p)) { eraseFile(caret.p); return true; }
            if (IsRow(caret.p)) {
                const std::wstring& t = p.text;
                if (vk == VK_BACK && caret.o > 0 && t[caret.o - 1] == L'\t') { jump({ caret.p, caret.o - 1 }); return true; }  // não junta células
                if (vk == VK_DELETE && (caret.o >= (int)t.size() || t[caret.o] == L'\t')) return true;
                if (vk == VK_BACK && caret.o == 0) {
                    if (t.find_first_not_of(L'\t') == std::wstring::npos) {  // linha vazia: some (a última vira texto)
                        Checkpoint(3);
                        if (IsRow(caret.p - 1) || IsRow(caret.p + 1)) {
                            N->paras.erase(N->paras.begin() + caret.p);
                            int k = std::max(0, caret.p - 1);
                            caret = anchor = { k, (int)P(k).text.size() };
                        } else { p.kind = K_TEXT; p.text.clear(); p.at.clear(); }
                        Changed();
                    }
                    return true;
                }
            }
            if (vk == VK_BACK && caret.o == 0 && caret.p > 0) {
                int k = PrevVisible(caret.p);
                if (IsFile(k)) { eraseFile(k); return true; }
                if (IsRow(k)) { jump({ k, (int)P(k).text.size() }); return true; }
            }
            if (vk == VK_DELETE && caret.o == (int)p.text.size()) {
                int k = NextVisible(caret.p);
                if (IsFile(k)) { int keep = caret.p; eraseFile(k); caret = anchor = { keep, (int)P(keep).text.size() }; return true; }
                if (IsRow(k)) return true;
            }
        }
    }
    switch (vk) {
    case VK_LEFT:
        if (!shift && HasSel()) { move(SelStart()); return true; }
        if (caret.o > 0) move({ caret.p, ctrl ? PrevWord(p.text, caret.o) : caret.o - 1 });
        else { int k = PrevVisible(caret.p); if (k >= 0) move({ k, (int)P(k).text.size() }); }
        return true;
    case VK_RIGHT:
        if (!shift && HasSel()) { move(SelEnd()); return true; }
        if (caret.o < (int)p.text.size()) move({ caret.p, ctrl ? NextWord(p.text, caret.o) : caret.o + 1 });
        else { int k = NextVisible(caret.p); if (k >= 0) move({ k, 0 }); }
        return true;
    case VK_UP: case VK_DOWN: {
        if (pl.size() != (size_t)Count()) return true;
        if (ctrl) { move(vk == VK_UP ? Pos{ 0, 0 } : Pos{ Count() - 1, (int)P(Count() - 1).text.size() }); return true; }
        D2D1_RECT_F c = CaretRectAt(caret);
        if (desiredX < 0) desiredX = c.left;
        float y = vk == VK_UP ? c.top - 2 : c.bottom + 2;
        if (vk == VK_UP && caret.p == 0 && c.top <= pl[0].y + 1) { move({ 0, 0 }, true); return true; }
        if (vk == VK_DOWN && y >= contentH - 40) { move({ Count() - 1, (int)P(Count() - 1).text.size() }, true); return true; }
        // pula o espaço entre parágrafos
        Pos q = HitContent(desiredX, y);
        if (Same(q, caret)) q = HitContent(desiredX, vk == VK_UP ? y - 6 : y + 6);
        move(q, true);
        return true;
    }
    case VK_HOME: case VK_END: {
        if (ctrl) { move(vk == VK_HOME ? Pos{ 0, 0 } : Pos{ Count() - 1, (int)P(Count() - 1).text.size() }); return true; }
        if (pl.size() != (size_t)Count() || !pl[caret.p].tl) return true;
        D2D1_RECT_F c = CaretRectAt(caret);
        BOOL tr, in; DWRITE_HIT_TEST_METRICS m;
        pl[caret.p].tl->HitTestPoint(vk == VK_HOME ? -1000.f : 100000.f, (c.top + c.bottom) / 2 - pl[caret.p].y, &tr, &in, &m);
        int o = (int)m.textPosition + (tr ? (int)m.length : 0);
        o = std::min(o, (int)p.text.size());
        if (vk == VK_END && o > 0 && o < (int)p.text.size() && p.text[o - 1] == L' ') o--;  // fim da linha quebrada
        move({ caret.p, o });
        return true;
    }
    case VK_PRIOR: case VK_NEXT: {
        float h = area.bottom - area.top;
        D2D1_RECT_F c = CaretRectAt(caret);
        scrollY += vk == VK_PRIOR ? -h : h;
        move(HitContent(c.left, (c.top + c.bottom) / 2 + (vk == VK_PRIOR ? -h : h)));
        return true;
    }
    case VK_BACK:
        if (HasSel()) { Checkpoint(2); DeleteSel(); Changed(); return true; }
        if (caret.o == 0) {
            Checkpoint(3);
            if (p.list != L_NONE) p.list = L_NONE;
            else if (p.indent > 0) p.indent--;
            else if (caret.p > 0) {
                int k = PrevVisible(caret.p);
                if (k < 0) k = caret.p - 1;
                if (P(k).text.empty() && !p.text.empty()) {  // linha vazia em cima: some ela, este fica como está
                    N->paras.erase(N->paras.begin() + k);
                    caret = anchor = { k, 0 };
                } else {
                    for (int z = k; z < caret.p; z++) P(z).collapsed = false;
                    Pos a{ k, (int)P(k).text.size() };
                    DeleteRange(a, { caret.p, 0 });
                }
            } else if (p.style != S_BODY && p.text.empty() && Count() > 1) p.style = S_BODY;
            Changed();
            return true;
        }
        Checkpoint(2);
        DeleteRange({ caret.p, ctrl ? PrevWord(p.text, caret.o) : caret.o - 1 }, caret);
        Changed();
        return true;
    case VK_DELETE:
        if (HasSel()) { Checkpoint(2); DeleteSel(); Changed(); return true; }
        Checkpoint(2);
        if (caret.o < (int)p.text.size()) DeleteRange(caret, { caret.p, ctrl ? NextWord(p.text, caret.o) : caret.o + 1 });
        else if (caret.p + 1 < Count()) { P(caret.p).collapsed = false; DeleteRange(caret, { caret.p + 1, 0 }); }
        Changed();
        return true;
    case VK_TAB: {
        int a, b; SelParas(a, b);
        if (p.list != L_NONE || a != b) { Indent(shift ? -1 : 1); return true; }
        if (!shift) { Checkpoint(1); uint8_t at = AttrAtCaret(); DeleteSel(); InsertPlain(L"\t", at); Changed(); }
        return true;
    }
    case VK_ESCAPE:
        if (HasSel()) { anchor = caret; return true; }
        return false;
    }
    return false;
}

}  // namespace editor
