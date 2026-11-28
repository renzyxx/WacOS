// ---- folha de desenho (o Quill dentro do Notas) ---------------------------------------------------------------
// Incluído no meio de main.cpp (usa os ajudantes de lá: Text, TextW, Tr, ClientDip, In, Invalidate, DrawLights...).
// Como o Quill: o papel ocupa a janela inteira; em cima, os três botões da janela, a pílula do meio (caneta = mostra a
// paleta, laço, mão, zoom) e a da direita (foto, exportar, mais). Embaixo, a paleta de vidro com os instrumentos de
// papelaria (instrumentos.h), desfazer/refazer e as amostras; a roda abre o balão de cores (grade, espectro, conta-gotas).
// Tocar de novo no instrumento ativo abre os tamanhos (com o rabisco de cada um) e a opacidade.
// Ferramentas: caneta, caneta-tinteiro, marcador, lápis, giz de cera, aquarela (tinta.h), borracha (pixel ou traço),
// laço (selecionar, mover, apagar, duplicar, trocar a cor) e régua (o traço que começa na borda corre por ela;
// arrastar move, a roda gira). Segurar parado no fim de um traço o endireita ou vira forma (elipse, retângulo, triângulo).
// Papel branco, creme ou escuro; liso, pautado, quadriculado ou pontilhado. Aparência clara ou escura.
// Fechar (botão vermelho, Esc) põe o desenho na nota; o desenho fica salvo ao lado do PNG (desenho-*.tinta), e o
// clique duplo nele na nota abre a folha de novo para continuar.

const float SK_RULER_HW = 46, SK_RULER_HL = 640, SK_ERASE_R = 10;
const D2D1_COLOR_F SK_BLUE = D2D1::ColorF(10 / 255.f, 132 / 255.f, 1.f);
const D2D1_COLOR_F SK_PAPERS[3] = { D2D1::ColorF(1, 1, 1), D2D1::ColorF(0xfbf7ee), D2D1::ColorF(0x1c1c1e) };
enum { POP_MORE = 100, POP_COLORS = 200, POP_EXPORT = 300 };
// ícones do Quill (lucide-like, quadro de 24)
const char* IC_PEN = "M4 20l4.5-1L19 8.5a2.1 2.1 0 0 0-3-3L5.5 16 4 20zM14.5 7l2.5 2.5";
const char* IC_LASSO = "M12 4c-4.4 0-8 2.2-8 5s3.6 5 8 5 8-2.2 8-5-3.6-5-8-5zM7 13.5c-.6 1.2-.3 2.6 1 3.3 1.2.6 1.4 2 .4 3.2";
const char* IC_HAND = "M8 13V5.5a1.5 1.5 0 0 1 3 0V12M11 11.5v-7a1.5 1.5 0 0 1 3 0V12M14 12V6.5a1.5 1.5 0 0 1 3 0V14M8 12.5V10a1.5 1.5 0 0 0-3 0v4.5a7 7 0 0 0 7 6.5h.5a6.5 6.5 0 0 0 6.5-6.5v-2a1.5 1.5 0 0 0-3 0";
const char* IC_PHOTO = "M4 6.5A2.5 2.5 0 0 1 6.5 4h11A2.5 2.5 0 0 1 20 6.5v11a2.5 2.5 0 0 1-2.5 2.5h-11A2.5 2.5 0 0 1 4 17.5zM4 16l4.5-4.5 4 4 2.5-2.5L20 17.5M15.5 9.5h.01";
const char* IC_SHARE = "M12 15V4M8 8l4-4 4 4M5 13v5a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2v-5";
const char* IC_MORE = "M5 12h.01M12 12h.01M19 12h.01";
const char* IC_UNDO = "M9 14L4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11";
const char* IC_REDO = "M15 14l5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13";
const char* IC_DROPPER = "M14.5 6.5l3 3M4 20l1-4 9.5-9.5a2.1 2.1 0 0 1 3 3L8 19l-4 1zM16 5l1.5-1.5a2.1 2.1 0 0 1 3 3L19 8";

void SketchLayout();
void SkPushUndo();
void SkPhotoReload();
void SkGlassRelease();
inline bool SkInk() { return !g_sk.tempEraser && g_sk.tool < tinta::TOOL_COUNT; }
inline int SkTool() { return g_sk.tempEraser ? SK_ERASER : g_sk.tool; }
inline bool SkPanning() { return g_sk.hand || !g_sk.paletteOn; }
inline bool SameColor(D2D1_COLOR_F a, D2D1_COLOR_F b) { return std::fabs(a.r - b.r) < 0.004f && std::fabs(a.g - b.g) < 0.004f && std::fabs(a.b - b.b) < 0.004f; }
// aparência: a da pessoa (ou a do WacOS); o vidro também escurece no papel escuro
inline bool SkDark() { return g_sk.paper == 2; }  // o vidro mostra o papel: no papel escuro, letras e ícones claros
inline D2D1_COLOR_F SkText() { return SkDark() ? D2D1::ColorF(0xf2f2f5) : D2D1::ColorF(0x1c1c1f); }
inline D2D1_COLOR_F SkMuted() { return SkDark() ? D2D1::ColorF(0xa1a1aa) : D2D1::ColorF(0x6e6e78); }
inline D2D1_COLOR_F SkHover() { return SkDark() ? D2D1::ColorF(1, 1, 1, 0.09f) : D2D1::ColorF(0, 0, 0, 0.06f); }
inline D2D1_COLOR_F Alpha(D2D1_COLOR_F c, float a) { c.a = a; return c; }
// mundo (onde os traços moram) <-> tela
inline D2D1_POINT_2F SkWorld(D2D1_POINT_2F p) { return { (p.x - g_sk.canvas.left - g_sk.vx) / g_sk.vz, (p.y - g_sk.canvas.top - g_sk.vy) / g_sk.vz }; }
inline D2D1_POINT_2F SkScreen(float x, float y) { return { g_sk.canvas.left + g_sk.vx + x * g_sk.vz, g_sk.canvas.top + g_sk.vy + y * g_sk.vz }; }
inline D2D1_POINT_2F SkOrigin() { return { g_sk.canvas.left + g_sk.vx, g_sk.canvas.top + g_sk.vy }; }
inline void SkChanged() { g_sk.ver++; }

// a paleta mostra os instrumentos nesta ordem (a monolinha ao lado da caneta, como no Mac); a régua é o último
const int SK_NSLOTS = 8;
const int SK_SLOTS[SK_NSLOTS] = { tinta::PEN, tinta::FOUNTAIN, tinta::MARKER, tinta::PENCIL, tinta::CRAYON, SK_ERASER, SK_LASSO, SK_RULER_SLOT };
inline int SkSlotOf(int id) { for (int i = 0; i < SK_NSLOTS; i++) if (SK_SLOTS[i] == id) return i; return 0; }
inline int SkArt(int id) {
    if (id < tinta::MONOLINE) return id;
    if (id == tinta::MONOLINE) return instrumentos::MONO;
    if (id == tinta::BRUSHPEN) return instrumentos::BRUSHPEN;
    if (id == tinta::REEDPEN) return instrumentos::REED;
    if (id == SK_ERASER) return instrumentos::ERASER;
    if (id == SK_LASSO) return instrumentos::LASSO;
    return instrumentos::RULER;
}

// foto: centro, conversão mundo <-> foto (girada), cantos e a alça de girar
inline D2D1_POINT_2F PhC() { auto& P = g_sk.photo; return { P.x + P.w / 2, P.y + P.h / 2 }; }
inline D2D1_POINT_2F PhToLocal(D2D1_POINT_2F q) {
    auto c = PhC(); float s = std::sin(-g_sk.photo.rot), co = std::cos(-g_sk.photo.rot), dx = q.x - c.x, dy = q.y - c.y;
    return { dx * co - dy * s, dx * s + dy * co };
}
inline D2D1_POINT_2F PhToWorld(float lx, float ly) {
    auto c = PhC(); float s = std::sin(g_sk.photo.rot), co = std::cos(g_sk.photo.rot);
    return { c.x + lx * co - ly * s, c.y + lx * s + ly * co };
}
inline bool PhHit(D2D1_POINT_2F q) {
    auto& P = g_sk.photo;
    if (!P.on) return false;
    auto l = PhToLocal(q);
    if (std::fabs(l.x) > P.w / 2 || std::fabs(l.y) > P.h / 2) return false;
    return P.clip.size() < 3 || tinta::InPolygon((l.x + P.w / 2) / P.w, (l.y + P.h / 2) / P.h, P.clip);
}
inline D2D1_POINT_2F PhCorner(int i) { auto& P = g_sk.photo; return PhToWorld(i == 0 || i == 3 ? -P.w / 2 : P.w / 2, i < 2 ? -P.h / 2 : P.h / 2); }
inline D2D1_POINT_2F PhRotHandle() { return PhToWorld(0, -g_sk.photo.h / 2 - 30 / g_sk.vz); }

// hsl (0..360, 0..100, 0..100) como no Quill
inline D2D1_COLOR_F Hsl(float h, float s, float l) {
    s /= 100; l /= 100;
    auto k = [&](float n) { return std::fmod(n + h / 30, 12.f); };
    float a = s * std::min(l, 1 - l);
    auto f = [&](float n) { return l - a * std::max(-1.f, std::min(k(n) - 3, std::min(9 - k(n), 1.f))); };
    return D2D1::ColorF(f(0), f(8), f(4));
}
inline D2D1_COLOR_F GridColor(int r, int c) {
    if (r == 0) return Hsl(0, 0, 100 - c * 100.f / 11);
    static const float sl[6][2] = { { 90, 28 }, { 85, 40 }, { 80, 52 }, { 85, 64 }, { 80, 76 }, { 70, 87 } };
    return Hsl(c * 30.f, sl[r - 1][0], sl[r - 1][1]);
}
inline D2D1_COLOR_F SpectrumColor(float u, float v) {
    D2D1_COLOR_F c = Hsl(std::clamp(u, 0.f, 1.f) * 360, 100, 50);
    v = std::clamp(v, 0.f, 1.f);
    if (v < 0.5f) { float a = 0.88f * (1 - v / 0.5f); c = D2D1::ColorF(c.r + (1 - c.r) * a, c.g + (1 - c.g) * a, c.b + (1 - c.b) * a); }
    else { float a = 0.88f * ((v - 0.5f) / 0.5f); c = D2D1::ColorF(c.r * (1 - a), c.g * (1 - a), c.b * (1 - a)); }
    return c;
}

// ---------------------------------------------------------------------------------------------------------------
// foto de fundo e a camada guardada dos traços

ID2D1Bitmap* SkLoadBitmap(ID2D1RenderTarget* rt, const std::wstring& path, UINT* w = nullptr, UINT* h = nullptr) {
    IWICImagingFactory* wic = nullptr;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (!wic) return nullptr;
    ID2D1Bitmap* bmp = nullptr;
    IWICBitmapDecoder* dec = nullptr; IWICBitmapFrameDecode* fr = nullptr; IWICFormatConverter* cv = nullptr;
    if (SUCCEEDED(wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &dec)) &&
        SUCCEEDED(dec->GetFrame(0, &fr)) && SUCCEEDED(wic->CreateFormatConverter(&cv)) &&
        SUCCEEDED(cv->Initialize(fr, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut))) {
        if (w && h) cv->GetSize(w, h);
        if (rt) rt->CreateBitmapFromWicBitmap(cv, nullptr, &bmp);
    }
    if (cv) cv->Release(); if (fr) fr->Release(); if (dec) dec->Release(); wic->Release();
    return bmp;
}

void SkReleaseCache() {
    SkGlassRelease();
    if (g_sk.cache) { g_sk.cache->Release(); g_sk.cache = nullptr; }
    if (g_sk.photoBmp) { g_sk.photoBmp->Release(); g_sk.photoBmp = nullptr; }
    g_sk.cacheKey.clear();
}

// traços em ordem, com os de marcador por baixo da tinta (como o marca-texto no papel: a escrita continua nítida)
void SketchStrokes(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* br, D2D1_POINT_2F o, float k, bool skipLive = false) {
    tinta::DarkPaper() = g_sk.paper == 2;
    size_t n = g_sk.strokes.size() - (skipLive && g_sk.drawing && !g_sk.strokes.empty() ? 1 : 0);
    for (int pass = 0; pass < 2; pass++)
        for (size_t i = 0; i < n; i++) {
            bool marker = g_sk.strokes[i].tool == tinta::MARKER;
            if (marker != (pass == 0)) continue;
            tinta::Draw(rt, br, g_sk.strokes[i], o, k, false);
        }
}

// a foto girada e, se tiver, recortada pela volta desenhada
void SkDrawPhoto(ID2D1RenderTarget* rt, ID2D1Bitmap* bmp) {
    auto& P = g_sk.photo;
    if (!bmp || !P.on || P.w <= 0 || P.h <= 0) return;
    D2D1_MATRIX_3X2_F old; rt->GetTransform(&old);
    auto c = PhC();
    rt->SetTransform(D2D1::Matrix3x2F::Rotation(P.rot * 180 / 3.14159265f) * D2D1::Matrix3x2F::Translation(c.x, c.y) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
    ID2D1PathGeometry* g = nullptr;
    if (P.clip.size() >= 3) {
        ID2D1Factory* f = nullptr; rt->GetFactory(&f);
        if (f && SUCCEEDED(f->CreatePathGeometry(&g))) {
            ID2D1GeometrySink* k = nullptr;
            if (SUCCEEDED(g->Open(&k))) {
                auto L = [&](D2D1_POINT_2F u) { return D2D1::Point2F(u.x * P.w - P.w / 2, u.y * P.h - P.h / 2); };
                k->BeginFigure(L(P.clip[0]), D2D1_FIGURE_BEGIN_FILLED);
                for (size_t i = 1; i < P.clip.size(); i++) k->AddLine(L(P.clip[i]));
                k->EndFigure(D2D1_FIGURE_END_CLOSED); k->Close(); k->Release();
            }
        }
        if (f) f->Release();
    }
    if (g) rt->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), g), nullptr);
    rt->DrawBitmap(bmp, D2D1::RectF(-P.w / 2, -P.h / 2, P.w / 2, P.h / 2), 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    if (g) { rt->PopLayer(); g->Release(); }
    rt->SetTransform(old);
}

void SkPaintScene(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* br, D2D1_POINT_2F o, float k, D2D1_RECT_F worldVis, ID2D1Bitmap* photo, bool skipLive) {
    rt->Clear(SK_PAPERS[g_sk.paper]);
    D2D1_MATRIX_3X2_F old; rt->GetTransform(&old);
    rt->SetTransform(D2D1::Matrix3x2F::Scale(k, k) * D2D1::Matrix3x2F::Translation(o.x, o.y) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
    tinta::DrawPattern(rt, br, worldVis, g_sk.pattern, g_sk.paper == 2, 1 / k);
    SkDrawPhoto(rt, photo);
    rt->SetTransform(old);
    SketchStrokes(rt, br, o, k, skipLive);
}

// a camada do papel + traços prontos fica guardada num bitmap: enquanto se escreve, só o traço vivo é redesenhado
void SkEnsureCache(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    auto& C = g_sk.canvas;
    float W = C.right - C.left, H = C.bottom - C.top;
    if (W < 1 || H < 1) return;
    if (g_sk.cache && (g_sk.cacheDc != dc || std::fabs(g_sk.cacheW - W) > 0.5f || std::fabs(g_sk.cacheH - H) > 0.5f)) SkReleaseCache();
    if (!g_sk.cache) {
        if (FAILED(dc->CreateCompatibleRenderTarget(D2D1::SizeF(W, H), &g_sk.cache))) { g_sk.cache = nullptr; return; }
        g_sk.cacheDc = dc; g_sk.cacheW = W; g_sk.cacheH = H; g_sk.cacheKey.clear();
    }
    if (g_sk.photo.on && !g_sk.photoBmp) g_sk.photoBmp = SkLoadBitmap(g_sk.cache, g_sk.photo.path);
    wchar_t key[256];
    swprintf_s(key, L"%d|%zu|%d|%g|%g|%g|%d|%d|%d|%d", g_sk.ver, g_sk.strokes.size(), (int)g_sk.drawing, g_sk.vx, g_sk.vy, g_sk.vz, g_sk.paper, g_sk.pattern, (int)g_sk.photo.on, (int)(g_sk.photoBmp != nullptr));
    if (g_sk.cacheKey == key) return;
    g_sk.cacheKey = key;
    g_sk.cache->BeginDraw();
    D2D1_POINT_2F a = SkWorld({ C.left, C.top }), b = SkWorld({ C.right, C.bottom });
    SkPaintScene(g_sk.cache, br, { g_sk.vx, g_sk.vy }, g_sk.vz, D2D1::RectF(a.x, a.y, b.x, b.y), g_sk.photoBmp, true);
    if (FAILED(g_sk.cache->EndDraw())) SkReleaseCache();
}

// ---------------------------------------------------------------------------------------------------------------
// abrir/fechar e o arquivo do desenho (desenho-*.tinta, ao lado do PNG)

void SketchReset(bool open) {
    Sketch k;
    k.tool = g_sk.tool; k.eraseObject = g_sk.eraseObject; k.pattern = g_sk.pattern; k.paper = g_sk.paper; k.theme = g_sk.theme; k.custom = g_sk.custom;
    for (int i = 0; i < tinta::TOOL_COUNT; i++) { k.color[i] = g_sk.color[i]; k.size[i] = g_sk.size[i]; k.opacity[i] = g_sk.opacity[i]; }
    if (g_hwnd) KillTimer(g_hwnd, T_HOLD);
    if (GetCapture() == g_hwnd && g_sk.mode != Sketch::M_NONE) ReleaseCapture();
    SkReleaseCache();
    g_sk = std::move(k);
    g_sk.on = open;
    tinta::ClearCache();
    instrumentos::ClearCache();
    instrumentos::ClearIcons();
}

std::wstring SkDataPath(const std::wstring& png) {
    std::wstring s = png;
    size_t dot = s.rfind(L'.');
    if (dot != std::wstring::npos) s = s.substr(0, dot);
    return s + L".tinta";
}

bool SkSaveData(const std::wstring& png) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, SkDataPath(png).c_str(), L"w, ccs=UTF-8") || !f) return false;
    fwprintf(f, L"tinta 1\npapel %d %d\n", g_sk.paper, g_sk.pattern);
    if (g_sk.photo.on) {
        auto& P = g_sk.photo;
        fwprintf(f, L"foto2 %g %g %g %g %g %zu\n%ls\n", P.x, P.y, P.w, P.h, P.rot, P.clip.size(), P.path.c_str());
        for (auto& u : P.clip) fwprintf(f, L"%.4f %.4f ", u.x, u.y);
        fputwc(L'\n', f);
    }
    for (auto& s : g_sk.strokes) {
        fwprintf(f, L"traco2 %d %.4f %.4f %.4f %.3f %.3f %d %.1f %zu\n", (int)s.tool, s.color.r, s.color.g, s.color.b, s.size, s.opacity, (int)s.sim, s.nib, s.pts.size());
        for (auto& p : s.pts) fwprintf(f, L"%.2f %.2f %.3f ", p.x, p.y, p.p);
        fputwc(L'\n', f);
    }
    fclose(f);
    return true;
}

bool SkLoadData(const std::wstring& png) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, SkDataPath(png).c_str(), L"r, ccs=UTF-8") || !f) return false;
    wchar_t word[32];
    int ver = 0;
    bool ok = fwscanf_s(f, L"%31s %d", word, 32, &ver) == 2 && wcscmp(word, L"tinta") == 0;
    while (ok && fwscanf_s(f, L"%31s", word, 32) == 1) {
        if (!wcscmp(word, L"papel")) { if (fwscanf_s(f, L"%d %d", &g_sk.paper, &g_sk.pattern) != 2) break; g_sk.paper = std::clamp(g_sk.paper, 0, 2); g_sk.pattern = std::clamp(g_sk.pattern, 0, 3); }
        else if (!wcscmp(word, L"foto") || !wcscmp(word, L"foto2")) {  // foto2: com giro e recorte
            auto& P = g_sk.photo;
            bool v2 = !wcscmp(word, L"foto2");
            size_t nclip = 0;
            if (fwscanf_s(f, L"%f %f %f %f", &P.x, &P.y, &P.w, &P.h) != 4) break;
            if (v2 && fwscanf_s(f, L"%f %zu", &P.rot, &nclip) != 2) break;
            wchar_t line[1024];
            if (!fgetws(line, 1024, f) || !fgetws(line, 1024, f)) break;  // o fim da linha e a linha do caminho
            P.path = line;
            while (!P.path.empty() && (P.path.back() == L'\n' || P.path.back() == L'\r')) P.path.pop_back();
            P.clip.resize(std::min<size_t>(nclip, 20000));
            for (auto& u : P.clip) if (fwscanf_s(f, L"%f %f", &u.x, &u.y) != 2) { ok = false; break; }
            P.on = GetFileAttributesW(P.path.c_str()) != INVALID_FILE_ATTRIBUTES;
        }
        else if (!wcscmp(word, L"traco") || !wcscmp(word, L"traco2")) {  // traco2: com o ângulo da pena de junco
            tinta::Stroke s; int tool = 0, sim = 0; size_t n = 0;
            if (fwscanf_s(f, L"%d %f %f %f %f %f %d", &tool, &s.color.r, &s.color.g, &s.color.b, &s.size, &s.opacity, &sim) != 7) break;
            if (!wcscmp(word, L"traco2") && fwscanf_s(f, L"%f", &s.nib) != 1) break;
            if (fwscanf_s(f, L"%zu", &n) != 1) break;
            s.tool = (tinta::Tool)std::clamp(tool, 0, tinta::TOOL_COUNT - 1); s.color.a = 1; s.sim = sim != 0;
            s.pts.resize(std::min<size_t>(n, 200000));
            for (auto& p : s.pts) if (fwscanf_s(f, L"%f %f %f", &p.x, &p.y, &p.p) != 3) { ok = false; break; }
            if (ok) g_sk.strokes.push_back(std::move(s));
        }
        else break;
    }
    fclose(f);
    return ok;
}

tinta::Box SkContentBox() {
    tinta::Box b{ 1e9f, 1e9f, -1e9f, -1e9f };
    for (auto& s : g_sk.strokes) { auto o = tinta::BBox(s); b.x1 = std::min(b.x1, o.x1); b.y1 = std::min(b.y1, o.y1); b.x2 = std::max(b.x2, o.x2); b.y2 = std::max(b.y2, o.y2); }
    if (g_sk.photo.on) {
        auto& P = g_sk.photo;
        auto add = [&](D2D1_POINT_2F q) { b.x1 = std::min(b.x1, q.x); b.y1 = std::min(b.y1, q.y); b.x2 = std::max(b.x2, q.x); b.y2 = std::max(b.y2, q.y); };
        if (P.clip.size() >= 3) for (auto& u : P.clip) add(PhToWorld(u.x * P.w - P.w / 2, u.y * P.h - P.h / 2));
        else for (int i = 0; i < 4; i++) add(PhCorner(i));
    }
    return b;
}

// encaixa o desenho na janela (abaixo das pílulas, acima da paleta)
void SkZoomToFit() {
    auto b = SkContentBox();
    auto& C = g_sk.canvas;
    if (b.x1 > b.x2) { g_sk.vx = g_sk.vy = 0; g_sk.vz = 1; return; }
    float W = C.right - C.left - 120, H = C.bottom - C.top - TOOLBAR_H - g_sk.top - (g_sk.paletteOn ? 130 : 30) - 40;
    float z = std::clamp(std::min(W / std::max(1.f, b.x2 - b.x1), H / std::max(1.f, b.y2 - b.y1)), 0.25f, 2.f);
    g_sk.vz = z;
    g_sk.vx = (C.right - C.left) / 2 - (b.x1 + b.x2) / 2 * z;
    g_sk.vy = TOOLBAR_H + g_sk.top + 20 + H / 2 - (b.y1 + b.y2) / 2 * z;
}

void SkZoomAt(D2D1_POINT_2F p, float z) {
    z = std::clamp(z, 0.25f, 4.f);
    D2D1_POINT_2F w = SkWorld(p);
    g_sk.vz = z;
    g_sk.vx = p.x - g_sk.canvas.left - w.x * z;
    g_sk.vy = p.y - g_sk.canvas.top - w.y * z;
}

// o desenho vira imagem (2x, recortado em volta do que foi desenhado, no papel escolhido)
IWICBitmap* SkRender(IWICImagingFactory* wic, UINT& outW, UINT& outH) {
    if (g_sk.strokes.empty() && !g_sk.photo.on) return nullptr;
    auto b = SkContentBox();
    float m = 20, sc = 2;
    float x0 = b.x1 - m, y0 = b.y1 - m, W = std::max(40.f, b.x2 - b.x1 + 2 * m), H = std::max(30.f, b.y2 - b.y1 + 2 * m);
    float lim = 8000.f / std::max(W, H);  // não passa de 8000 px de lado
    if (sc > lim) sc = lim;
    outW = (UINT)(W * sc); outH = (UINT)(H * sc);
    ID2D1Factory* f = nullptr; IWICBitmap* bmp = nullptr; ID2D1RenderTarget* rt = nullptr;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), nullptr, (void**)&f);
    bool ok = false;
    if (f && SUCCEEDED(wic->CreateBitmap(outW, outH, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &bmp)) &&
        SUCCEEDED(f->CreateWicBitmapRenderTarget(bmp, D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), &rt))) {
        ID2D1SolidColorBrush* br = nullptr; rt->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &br);
        ID2D1Bitmap* photo = g_sk.photo.on ? SkLoadBitmap(rt, g_sk.photo.path) : nullptr;
        rt->BeginDraw();
        bool drawing = g_sk.drawing; g_sk.drawing = false;
        SkPaintScene(rt, br, { -x0 * sc, -y0 * sc }, sc, D2D1::RectF(x0, y0, x0 + W, y0 + H), photo, false);
        g_sk.drawing = drawing;
        ok = SUCCEEDED(rt->EndDraw());
        if (photo) photo->Release();
        br->Release();
    }
    tinta::ClearCache();  // os pincéis de textura estavam presos a este alvo
    if (rt) rt->Release(); if (f) f->Release();
    if (!ok && bmp) { bmp->Release(); bmp = nullptr; }
    return bmp;
}

bool SkSavePng(IWICImagingFactory* wic, IWICBitmap* bmp, UINT W, UINT H, const std::wstring& path) {
    IWICStream* st = nullptr; IWICBitmapEncoder* enc = nullptr; IWICBitmapFrameEncode* fe = nullptr;
    bool ok = false;
    if (SUCCEEDED(wic->CreateStream(&st)) && SUCCEEDED(st->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
        SUCCEEDED(wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) && SUCCEEDED(enc->Initialize(st, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(enc->CreateNewFrame(&fe, nullptr)) && SUCCEEDED(fe->Initialize(nullptr))) {
        fe->SetSize(W, H);
        WICPixelFormatGUID pf = GUID_WICPixelFormat32bppPBGRA; fe->SetPixelFormat(&pf);
        ok = SUCCEEDED(fe->WriteSource(bmp, nullptr)) && SUCCEEDED(fe->Commit()) && SUCCEEDED(enc->Commit());
    }
    if (fe) fe->Release(); if (enc) enc->Release(); if (st) st->Release();
    return ok;
}

// pronto: o desenho vai para a nota (ou atualiza o que já estava lá) e a folha fecha
void SketchInsert() {
    if (g_sk.strokes.empty() && !g_sk.photo.on) { SketchReset(false); FocusEditor(); Invalidate(); return; }
    IWICImagingFactory* wic = nullptr;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    std::wstring path = g_sk.editPath;
    if (path.empty()) { wchar_t name[64]; swprintf_s(name, L"\\desenho-%llx.png", (unsigned long long)store::Now()); path = store::AttachDir() + name; }
    UINT W = 0, H = 0;
    IWICBitmap* bmp = wic ? SkRender(wic, W, H) : nullptr;
    bool ok = bmp && SkSavePng(wic, bmp, W, H, path);
    if (ok) SkSaveData(path);
    if (bmp) bmp->Release();
    if (wic) wic->Release();
    Log(L"desenho: %ls %ls", path.c_str(), ok ? L"gravado" : L"FALHOU");
    bool editing = !g_sk.editPath.empty();
    SketchReset(false);
    g_sk.lastSaved = path;
    if (ok) { if (editing) editor::ForgetImage(path); else editor::InsertAttachment(path); }
    FocusEditor();
    Invalidate();
}

// clique duplo num desenho da nota: abre de novo para continuar (só os que têm o .tinta ao lado)
bool SkOpenDrawing(const std::wstring& png) {
    if (GetFileAttributesW(SkDataPath(png).c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    SketchReset(true);
    if (!SkLoadData(png)) { SketchReset(false); return false; }
    g_sk.editPath = png;
    editor::Focus(false);
    SketchLayout();
    SkZoomToFit();
    Invalidate();
    return true;
}
static const bool s_skHook = (editor::OpenDrawing = SkOpenDrawing, true);

void SkDiscard() {
    g_sheet = Sheet{};
    g_sheet.on = true;
    g_sheet.title = TrPick(L"Descartar as alterações?", L"Discard changes?");
    g_sheet.msg = TrPick(L"O que você desenhou agora não vai para a nota.", L"What you drew now won't be added to the note.");
    g_sheet.ok = TrPick(L"Descartar", L"Discard");
    g_sheet.act = [] { SketchReset(false); FocusEditor(); };
}

void SkSaveToPictures() {
    IWICImagingFactory* wic = nullptr;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (!wic) return;
    UINT W = 0, H = 0;
    IWICBitmap* bmp = SkRender(wic, W, H);
    PWSTR pics = nullptr;
    if (bmp && SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pics))) {
        std::wstring dir = std::wstring(pics) + L"\\" + TrPick(L"Desenhos do Notas", L"Notes Drawings");
        CreateDirectoryW(dir.c_str(), nullptr);
        SYSTEMTIME t; GetLocalTime(&t);
        wchar_t name[96]; swprintf_s(name, L"\\%ls %04d-%02d-%02d %02d.%02d.%02d.png", TrPick(L"Desenho", L"Drawing"), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
        std::wstring path = dir + name;
        if (SkSavePng(wic, bmp, W, H, path)) ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + path + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
    }
    if (pics) CoTaskMemFree(pics);
    if (bmp) bmp->Release();
    wic->Release();
}

void SkCopyImage() {
    IWICImagingFactory* wic = nullptr;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (!wic) return;
    UINT W = 0, H = 0;
    IWICBitmap* bmp = SkRender(wic, W, H);
    if (bmp) {
        std::vector<BYTE> px((size_t)W * H * 4);
        WICRect r{ 0, 0, (INT)W, (INT)H };
        if (SUCCEEDED(bmp->CopyPixels(&r, W * 4, (UINT)px.size(), px.data())) && OpenClipboard(g_hwnd)) {
            EmptyClipboard();
            HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + px.size());
            if (g) {
                BYTE* d = (BYTE*)GlobalLock(g);
                BITMAPINFOHEADER bi{ sizeof(bi), (LONG)W, (LONG)H, 1, 32, BI_RGB };
                memcpy(d, &bi, sizeof(bi));
                for (UINT y = 0; y < H; y++) memcpy(d + sizeof(bi) + (size_t)(H - 1 - y) * W * 4, px.data() + (size_t)y * W * 4, W * 4);  // DIB de baixo para cima
                GlobalUnlock(g);
                if (!SetClipboardData(CF_DIB, g)) GlobalFree(g);
            }
            CloseClipboard();
        }
        bmp->Release();
    }
    wic->Release();
}

void SkPlacePhoto(const std::wstring& file);
void SkPickPhoto() {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW of{ sizeof(of) };
    of.hwndOwner = g_hwnd;
    of.lpstrFilter = L"Imagens\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp;*.heic;*.tif;*.tiff\0";
    of.lpstrFile = file; of.nMaxFile = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&of)) return;
    SkPlacePhoto(file);
}

// põe a imagem como foto de fundo, no meio da tela, já escolhida (alças de tamanho e giro)
void SkPlacePhoto(const std::wstring& file) {
    std::wstring copy = store::AddAttachment(file);
    if (copy.empty()) return;
    UINT w = 0, h = 0;
    SkLoadBitmap(nullptr, copy, &w, &h);
    if (!w || !h) return;
    auto& C = g_sk.canvas;
    D2D1_POINT_2F a = SkWorld({ C.left, C.top + TOOLBAR_H }), b = SkWorld({ C.right, C.bottom - 120 });
    float vw = b.x - a.x, vh = b.y - a.y, k = std::min(vw * 0.7f / w, vh * 0.7f / h);
    SkPushUndo();
    g_sk.photo = {};
    g_sk.photo.on = true; g_sk.photo.path = copy; g_sk.photoSel = true; g_sk.crop = false;
    g_sk.photo.x = (a.x + b.x) / 2 - w * k / 2; g_sk.photo.y = (a.y + b.y) / 2 - h * k / 2; g_sk.photo.w = w * k; g_sk.photo.h = h * k;
    if (g_sk.photoBmp) { g_sk.photoBmp->Release(); g_sk.photoBmp = nullptr; }
    SkChanged();
}

// ---------------------------------------------------------------------------------------------------------------
// layout

// quanto do alto da janela fica por baixo da barra de menus do WacOS (janela ampliada até o topo da tela)
float SkTopInset() {
    if (!g_hwnd || IsIconic(g_hwnd)) return 0;
    POINT o{ 0, 0 }; ClientToScreen(g_hwnd, &o);
    MONITORINFO mi{ sizeof(mi) };
    if (!GetMonitorInfoW(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi)) return 0;
    int d = mi.rcWork.top - o.y;
    return d > 0 && d < 200 ? d / g_scale : 0;
}
inline D2D1_POINT_2F SkLightsPt(D2D1_POINT_2F p) { return { p.x, p.y - g_sk.top }; }  // os três botões descem junto

void SketchLayout() {
    auto c = ClientDip();
    g_sk.top = SkTopInset();
    const float TOOL_CY = ::TOOL_CY + g_sk.top;
    g_sk.panel = { 0, 0, c.width, c.height };
    g_sk.canvas = { 0, 0, c.width, c.height };
    auto& C = g_sk.canvas;
    if (g_sk.ruler.on && !g_sk.ruler.placed) {  // a régua aparece no meio da tela, um pouco inclinada
        D2D1_POINT_2F m = SkWorld({ (C.left + C.right) / 2, (C.top + C.bottom) / 2 - 40 });
        g_sk.ruler.cx = m.x; g_sk.ruler.cy = m.y;
        g_sk.ruler.angle = -12 * 3.14159265f / 180; g_sk.ruler.placed = true;
    }
    // pílulas de cima: [caneta laço mão zoom] no meio, [foto exportar mais] à direita
    float pw = 4 + 36 * 3 + 2 * 3 + 58 + 4;
    g_sk.pillC = { c.width / 2 - pw / 2, TOOL_CY - 22, c.width / 2 + pw / 2, TOOL_CY + 22 };
    float x = g_sk.pillC.left + 4;
    D2D1_RECT_F* b3[3] = { &g_sk.bPen, &g_sk.bLasso, &g_sk.bHand };
    for (auto r : b3) { *r = { x, TOOL_CY - 18, x + 36, TOOL_CY + 18 }; x += 38; }
    g_sk.bZoom = { x, TOOL_CY - 18, x + 58, TOOL_CY + 18 };
    float rw = 4 + 36 * 3 + 2 * 2 + 4;
    g_sk.pillR = { c.width - 16 - rw, TOOL_CY - 22, c.width - 16, TOOL_CY + 22 };
    x = g_sk.pillR.left + 4;
    D2D1_RECT_F* r3[3] = { &g_sk.bPhoto, &g_sk.bExport, &g_sk.bMore };
    for (auto r : r3) { *r = { x, TOOL_CY - 18, x + 36, TOOL_CY + 18 }; x += 38; }
    // paleta: [desfazer refazer] [9 instrumentos] | [amostras 3x2]
    const float PW = 18 + 36 + 6 + 36 + 14 + (SK_NSLOTS * 40 + (SK_NSLOTS - 1) * 2) + 14 + 1 + 14 + (3 * 28 + 2 * 10) + 18;
    float cx = (C.left + C.right) / 2, bottom = C.bottom - 18;
    g_sk.palette = g_sk.paletteOn ? D2D1::RectF(cx - PW / 2, bottom - 92, cx + PW / 2, bottom) : D2D1::RectF(-1e5f, -1e5f, -1e5f, -1e5f);
    float cy = bottom - 46;
    x = g_sk.palette.left + 18;
    g_sk.bUndo = { x, cy - 18, x + 36, cy + 18 }; x += 42;
    g_sk.bRedo = { x, cy - 18, x + 36, cy + 18 }; x += 36 + 14;
    for (int k = 0; k < SK_NSLOTS; k++) { g_sk.inst[k] = { x, g_sk.palette.top, x + 40, g_sk.palette.bottom }; x += 42; }
    x += -2 + 14 + 1 + 14;
    for (int i = 0; i < 6; i++) { float sx = x + (i % 3) * 38, sy = cy - 32 + (i / 3) * 36; g_sk.sw[i] = { sx, sy, sx + 28, sy + 28 }; }
    // balões
    g_sk.popR = {};
    g_sk.menuN = 0;
    if (g_sk.pop >= 0 && g_sk.pop < SK_LASSO && g_sk.paletteOn) {
        bool er = g_sk.pop == SK_ERASER;
        bool reed = g_sk.pop == tinta::REEDPEN;
        float w = er ? 2 * 128 + 6 + 32 : 5 * 48 + 4 * 6 + 32, h = er ? 14 + 22 + 38 + 14 : 14 + 22 + 48 + 12 + 26 + 14 + (reed ? 12 + 26 : 0);
        auto& slot = g_sk.inst[SkSlotOf(g_sk.pop)];
        float px = std::clamp((slot.left + slot.right) / 2, C.left + 10 + w / 2, C.right - 10 - w / 2);
        g_sk.popR = { px - w / 2, g_sk.palette.top - 12 - h, px + w / 2, g_sk.palette.top - 12 };
        float ix = g_sk.popR.left + 16, iy = g_sk.popR.top + 14 + 22;
        if (er) { for (int i = 0; i < 2; i++) g_sk.popItem[i] = { ix + 3 + i * 131, iy + 3, ix + 3 + i * 131 + 128, iy + 35 }; }
        else {
            for (int i = 0; i < 5; i++) g_sk.popItem[i] = { ix + i * 54, iy, ix + i * 54 + 48, iy + 48 };
            g_sk.opTrack = { ix, iy + 48 + 12, g_sk.popR.right - 16 - 52, iy + 48 + 12 + 26 };
            g_sk.angTrack = reed ? D2D1::RectF(ix, g_sk.opTrack.bottom + 12, g_sk.opTrack.right, g_sk.opTrack.bottom + 12 + 26) : D2D1::RectF(0, 0, 0, 0);
        }
    } else if (g_sk.pop == POP_COLORS && g_sk.paletteOn) {
        float w = 320, h = 14 + 22 + 154 + 12 + 44 + 12 + 30 + 14;
        float px = std::clamp((g_sk.sw[5].left + g_sk.sw[5].right) / 2, C.left + 10 + w / 2, C.right - 10 - w / 2);
        g_sk.popR = { px - w / 2, g_sk.palette.top - 12 - h, px + w / 2, g_sk.palette.top - 12 };
        auto& P = g_sk.popR;
        g_sk.gridR = { P.left + 16, P.top + 36, P.left + 16 + 288, P.top + 36 + 154 };
        g_sk.specR = { P.left + 16, g_sk.gridR.bottom + 12, P.left + 16 + 288, g_sk.gridR.bottom + 56 };
        float y = g_sk.specR.bottom + 12;
        g_sk.bEyedrop = { P.right - 16 - 34, y - 2, P.right - 16, y + 32 };
    } else if (g_sk.pop == POP_MORE || g_sk.pop == POP_EXPORT) {
        // menus de cima: itens de 32 (o "mais" começa com o papel)
        bool more = g_sk.pop == POP_MORE;
        float w = more ? 268.f : 240.f;
        auto& anchor = more ? g_sk.bMore : g_sk.bExport;
        float left = std::min(anchor.left - 8, c.width - 10 - w);
        float top = g_sk.pillR.bottom + 8, y = top + 6;
        if (more) {
            for (int i = 0; i < 3; i++) g_sk.paperSw[i] = { left + 14 + i * 40, y + 26, left + 14 + i * 40 + 30, y + 56 };
            for (int i = 0; i < 4; i++) g_sk.patChip[i] = { left + 12 + i * 62, y + 66, left + 12 + i * 62 + 58, y + 92 };
            y += 108;
        }
        auto add = [&](int id) { g_sk.menuId[g_sk.menuN] = id; g_sk.menuR[g_sk.menuN] = { left + 6, y, left + w - 6, y + 32 }; g_sk.menuN++; y += 32; };
        if (more) { add(1); add(2); if (g_sk.photo.on) add(4); add(5); add(6); }  // sem a opção de aparência
        else { add(10); add(11); add(12); }
        g_sk.popR = { left, top, left + w, y + 6 };
    }
    // foto escolhida: o menu (Recortar, Girar, Desfazer recorte, Apagar) em cima dela; recortando: a dica no alto
    g_sk.photoMenu = g_sk.bCrop = g_sk.bRot90 = g_sk.bUncrop = g_sk.bPhotoDel = g_sk.cropHint = g_sk.bCropCancel = {};
    if (g_sk.photo.on && g_sk.crop) {
        std::wstring hint = TrPick(L"Desenhe uma volta em torno do que fica", L"Draw a loop around the part to keep");
        float tw = TextW(g_f13, hint) + 28, cw = TextW(g_f13, Tr(L"Cancelar")) + 26, w = tw + cw + 10;
        float y = g_sk.pillC.bottom + 12;
        g_sk.cropHint = { c.width / 2 - w / 2, y, c.width / 2 + w / 2, y + 38 };
        g_sk.bCropCancel = { g_sk.cropHint.right - 5 - cw, y + 5, g_sk.cropHint.right - 5, y + 33 };
    } else if (g_sk.photo.on && g_sk.photoSel && g_sk.mode != Sketch::M_PHOTO_MOVE && g_sk.mode != Sketch::M_PHOTO_SCALE && g_sk.mode != Sketch::M_PHOTO_ROT) {
        float x1 = 1e9f, y1 = 1e9f, x2 = -1e9f, y2 = -1e9f;
        for (int i = 0; i < 4; i++) { auto s2 = SkScreen(PhCorner(i).x, PhCorner(i).y); x1 = std::min(x1, s2.x); y1 = std::min(y1, s2.y); x2 = std::max(x2, s2.x); y2 = std::max(y2, s2.y); }
        auto rh = PhRotHandle(); auto rs = SkScreen(rh.x, rh.y); y1 = std::min(y1, rs.y - 10);
        float wc = TextW(g_f13, TrPick(L"Recortar", L"Crop")) + 24, wr = TextW(g_f13, TrPick(L"Girar", L"Rotate")) + 24;
        float wu = g_sk.photo.clip.size() >= 3 ? TextW(g_f13, TrPick(L"Foto inteira", L"Uncrop")) + 24 : 0, wd = TextW(g_f13, Tr(L"Apagar")) + 24;
        float w = wc + wr + wu + wd + 12;
        float mx = std::clamp((x1 + x2) / 2, C.left + 10 + w / 2, C.right - 10 - w / 2);
        float my = y1 - 14 - 36;
        if (my < TOOLBAR_H + g_sk.top + 6) my = std::min(y2 + 14, C.bottom - (g_sk.paletteOn ? 150.f : 60.f));
        g_sk.photoMenu = { mx - w / 2, my, mx + w / 2, my + 36 };
        float bx = g_sk.photoMenu.left + 6;
        g_sk.bCrop = { bx, my + 4, bx + wc, my + 32 }; bx += wc;
        g_sk.bRot90 = { bx, my + 4, bx + wr, my + 32 }; bx += wr;
        if (wu > 0) { g_sk.bUncrop = { bx, my + 4, bx + wu, my + 32 }; bx += wu; }
        g_sk.bPhotoDel = { bx, my + 4, bx + wd, my + 32 };
    }
    // menu da seleção, em cima da caixa dos traços escolhidos
    g_sk.selMenu = {};
    if (!g_sk.sel.empty() && g_sk.mode != Sketch::M_MOVE) {
        tinta::Box b{ 1e9f, 1e9f, -1e9f, -1e9f };
        for (int i : g_sk.sel) { auto o = tinta::BBox(g_sk.strokes[i]); b.x1 = std::min(b.x1, o.x1); b.y1 = std::min(b.y1, o.y1); b.x2 = std::max(b.x2, o.x2); b.y2 = std::max(b.y2, o.y2); }
        D2D1_POINT_2F s1 = SkScreen(b.x1, b.y1), s2 = SkScreen(b.x2, b.y2);
        float wd = TextW(g_f13, Tr(L"Apagar")) + 24, wu = TextW(g_f13, Tr(L"Duplicar")) + 24, w = wd + wu + 12;
        float mx = std::clamp((s1.x + s2.x) / 2, C.left + 10 + w / 2, C.right - 10 - w / 2);
        float my = s1.y - 4 - 30 - 12 - 14 - 36;  // acima da alça de girar
        if (my < TOOLBAR_H + g_sk.top + 6) my = s2.y + 14;
        g_sk.selMenu = { mx - w / 2, my, mx + w / 2, my + 36 };
        g_sk.bSelDel = { g_sk.selMenu.left + 6, my + 4, g_sk.selMenu.left + 6 + wd, my + 32 };
        g_sk.bSelDup = { g_sk.bSelDel.right, my + 4, g_sk.bSelDel.right + wu, my + 32 };
    }
}

// a faixa de cima arrasta a janela (fora dos botões e das pílulas); o resto é da folha
bool SketchCaption(D2D1_POINT_2F p) {
    SketchLayout();
    return p.y < TOOLBAR_H + g_sk.top && !OverLights(SkLightsPt(p)) && !In(g_sk.pillC, p) && !In(g_sk.pillR, p) && !(g_sk.pop >= 0 && In(g_sk.popR, p));
}

// ---------------------------------------------------------------------------------------------------------------
// desenho

// vidro: sombra macia, translúcido (claro ou escuro), fio de luz na borda
// ---------------------------------------------------------------------------------------------------------------
// vidro líquido (o mesmo jeito do Quill): a borda arredondada de cada painel funciona como uma lente grossa — um mapa
// de deslocamento (feito para o tamanho e o canto exatos do painel) puxa o que está atrás para dentro perto da borda,
// então o papel e a tinta entortam ao passar por baixo dela. Por cima: fosco leve, cor mais viva, um véu fino, o
// brilho em diagonal e o fio de luz por dentro da borda (com a sombrinha embaixo, que dá espessura).

// o mapa: vermelho/verde = a normal para dentro da borda (128 = parado), mais forte rente à borda e sumindo na lente
ID2D1Bitmap* SkGlassMap(ID2D1DeviceContext* dc, float w, float h, float rad, float bezel) {
    float dpiX = 96, dpiY = 96; dc->GetDpi(&dpiX, &dpiY);
    wchar_t key[64]; swprintf_s(key, L"%.0f|%.0f|%.0f|%.1f", w, h, rad, dpiX);
    for (auto& e : g_sk.glassMaps) if (e.first == key) return e.second;
    if (g_sk.glassMaps.size() > 40) { for (auto& e : g_sk.glassMaps) e.second->Release(); g_sk.glassMaps.clear(); }
    const float k = dpiX / 96;  // na resolução da tela (o mapa e o desenho batem pixel a pixel)
    int W = std::max(2, (int)std::lround(w * k)), H = std::max(2, (int)std::lround(h * k));
    float R = std::min({ rad * k, W / 2.f, H / 2.f }), B = std::max(1.f, bezel * k);
    std::vector<UINT32> px((size_t)W * H);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float qx0 = x + 0.5f, qy0 = y + 0.5f;
            float qx = std::max({ R - qx0, 0.f, qx0 - (W - R) }), qy = std::max({ R - qy0, 0.f, qy0 - (H - R) });
            float d, nx, ny;
            if (qx > 0 && qy > 0) {  // canto: distância ao arco
                float cx = qx0 < R ? R : W - R, cy = qy0 < R ? R : H - R, dx = qx0 - cx, dy = qy0 - cy, len = std::max(0.0001f, std::hypot(dx, dy));
                d = R - len; nx = -dx / len; ny = -dy / len;
            } else {
                float dl = qx0, dr = W - qx0, dt = qy0, db = H - qy0;
                d = std::min({ dl, dr, dt, db });
                nx = d == dl ? 1.f : d == dr ? -1.f : 0.f;
                ny = d == dt ? 1.f : d == db ? -1.f : 0.f;
            }
            float t = d <= 0 ? 1 : std::max(0.f, 1 - d / B);
            float m = t * t * (3 - 2 * t);
            UINT32 r = (UINT32)std::clamp(128 + nx * m * 127, 0.f, 255.f), g = (UINT32)std::clamp(128 + ny * m * 127, 0.f, 255.f);
            px[(size_t)y * W + x] = 0xFF000000 | (r << 16) | (g << 8) | 128;
        }
    ID2D1Bitmap* bmp = nullptr;
    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiX);
    if (FAILED(dc->CreateBitmap(D2D1::SizeU(W, H), px.data(), W * 4, bp, &bmp)) || !bmp) return nullptr;
    g_sk.glassMaps.push_back({ key, bmp });
    return bmp;
}

void SkGlassEffects(ID2D1DeviceContext* dc) {
    if (g_sk.glassDc == dc && g_sk.glassDisp) return;
    if (g_sk.glassBlur) g_sk.glassBlur->Release();
    if (g_sk.glassXf) g_sk.glassXf->Release();
    if (g_sk.glassDisp) g_sk.glassDisp->Release();
    if (g_sk.glassSat) g_sk.glassSat->Release();
    g_sk.glassBlur = g_sk.glassXf = g_sk.glassDisp = g_sk.glassSat = nullptr;
    for (auto& e : g_sk.glassMaps) e.second->Release();
    g_sk.glassMaps.clear();
    g_sk.glassDc = dc;
    if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1GaussianBlur, &g_sk.glassBlur))) {
        g_sk.glassBlur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 1.2f);
        g_sk.glassBlur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    }
    dc->CreateEffect(CLSID_D2D12DAffineTransform, &g_sk.glassXf);
    if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1DisplacementMap, &g_sk.glassDisp))) {
        g_sk.glassDisp->SetValue(D2D1_DISPLACEMENTMAP_PROP_SCALE, 44.f);
        g_sk.glassDisp->SetValue(D2D1_DISPLACEMENTMAP_PROP_X_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_R);
        g_sk.glassDisp->SetValue(D2D1_DISPLACEMENTMAP_PROP_Y_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_G);
    }
    if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1Saturation, &g_sk.glassSat))) g_sk.glassSat->SetValue(D2D1_SATURATION_PROP_SATURATION, 1.75f);
}

void SkGlassRelease() {
    if (g_sk.glassBlur) g_sk.glassBlur->Release();
    if (g_sk.glassXf) g_sk.glassXf->Release();
    if (g_sk.glassDisp) g_sk.glassDisp->Release();
    if (g_sk.glassSat) g_sk.glassSat->Release();
    g_sk.glassBlur = g_sk.glassXf = g_sk.glassDisp = g_sk.glassSat = nullptr;
    for (auto& e : g_sk.glassMaps) e.second->Release();
    g_sk.glassMaps.clear();
    g_sk.glassDc = nullptr;
}

void Glass(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, D2D1_RECT_F r, float rad) {
    bool d = SkDark();
    float W = r.right - r.left, H = r.bottom - r.top;
    // sombra (o vidro flutua sobre o papel)
    for (int k = 8; k >= 1; k--) {
        br->SetColor(D2D1::ColorF(0, 0, 0, d ? 0.045f : 0.014f));
        dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left - k, r.top - k + 5, r.right + k, r.bottom + k + 5), rad + k, rad + k), br);
    }
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    ID2D1RoundedRectangleGeometry* clip = nullptr;
    if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(r, rad, rad), &clip);
    if (clip) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip), nullptr);
    br->SetColor(SK_PAPERS[g_sk.paper]); dc->FillRectangle(r, br);
    // o que está atrás: fosco leve -> refração pela borda -> cor mais viva. Efeitos próprios deste painel (o Direct2D
    // só calcula no fim do quadro) e sem trocar o alvo de desenho (a janela tem uma camada de recorte aberta)
    ID2D1Bitmap* bm = nullptr;
    if (g_sk.cache && SUCCEEDED(g_sk.cache->GetBitmap(&bm)) && bm) {
        ID2D1Effect *blur = nullptr, *xf = nullptr, *disp = nullptr, *sat = nullptr;
        if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1GaussianBlur, &blur))) {
            blur->SetInput(0, bm);
            blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 1.2f);
            blur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
            ID2D1Effect* top = blur;
            ID2D1Bitmap* map = SkGlassMap(dc, W, H, rad, std::min(22.f, std::min(W, H) * 0.42f));
            if (map && SUCCEEDED(dc->CreateEffect(CLSID_D2D12DAffineTransform, &xf)) && SUCCEEDED(dc->CreateEffect(CLSID_D2D1DisplacementMap, &disp))) {
                xf->SetInput(0, map);
                xf->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, D2D1::Matrix3x2F::Translation(r.left, r.top));  // a lente no lugar do painel
                disp->SetValue(D2D1_DISPLACEMENTMAP_PROP_SCALE, 44.f);
                disp->SetValue(D2D1_DISPLACEMENTMAP_PROP_X_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_R);
                disp->SetValue(D2D1_DISPLACEMENTMAP_PROP_Y_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_G);
                disp->SetInputEffect(0, blur);
                disp->SetInputEffect(1, xf);
                top = disp;
            }
            if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1Saturation, &sat))) { sat->SetValue(D2D1_SATURATION_PROP_SATURATION, 1.75f); sat->SetInputEffect(0, top); top = sat; }
            dc->DrawImage(top, D2D1::Point2F(r.left, r.top), r);
        }
        if (sat) sat->Release(); if (disp) disp->Release(); if (xf) xf->Release(); if (blur) blur->Release();
        bm->Release();
    }
    // véu do vidro (claro no papel claro, fumê no escuro)
    br->SetColor(d ? D2D1::ColorF(34 / 255.f, 34 / 255.f, 40 / 255.f, 0.32f) : D2D1::ColorF(1, 1, 1, 0.16f));  // véu fino: dá para ver através
    dc->FillRectangle(r, br);
    // brilho em diagonal (160°) e a sombrinha por dentro embaixo
    auto grad = [&](D2D1_POINT_2F a, D2D1_POINT_2F b, std::initializer_list<D2D1_GRADIENT_STOP> st, D2D1_RECT_F area) {
        ID2D1GradientStopCollection* sc = nullptr;
        std::vector<D2D1_GRADIENT_STOP> v(st);
        if (FAILED(dc->CreateGradientStopCollection(v.data(), (UINT32)v.size(), &sc))) return;
        ID2D1LinearGradientBrush* lg = nullptr;
        if (SUCCEEDED(dc->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(a, b), sc, &lg))) { dc->FillRectangle(area, lg); lg->Release(); }
        sc->Release();
    };
    float ang = 160 * 3.14159265f / 180, gx = std::sin(ang), gy = -std::cos(ang), half = (std::fabs(W * gx) + std::fabs(H * gy)) / 2;
    D2D1_POINT_2F mid{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
    D2D1_POINT_2F g0{ mid.x - gx * half, mid.y - gy * half }, g1{ mid.x + gx * half, mid.y + gy * half };
    if (d) grad(g0, g1, { { 0, D2D1::ColorF(1, 1, 1, 0.14f) }, { 0.45f, D2D1::ColorF(1, 1, 1, 0) } }, r);
    else grad(g0, g1, { { 0, D2D1::ColorF(1, 1, 1, 0.28f) }, { 0.38f, D2D1::ColorF(1, 1, 1, 0.04f) }, { 0.6f, D2D1::ColorF(1, 1, 1, 0) }, { 1, D2D1::ColorF(1, 1, 1, 0.1f) } }, r);
    grad({ 0, r.bottom - 14 }, { 0, r.bottom }, { { 0, D2D1::ColorF(0, 0, 0, 0) }, { 1, D2D1::ColorF(0, 0, 0, d ? 0.4f : 0.12f) } }, D2D1::RectF(r.left, r.bottom - 14, r.right, r.bottom));
    // fio de luz por dentro: forte em cima, mais fraco nos lados e embaixo
    ID2D1GradientStopCollection* rs = nullptr;
    D2D1_GRADIENT_STOP rst[3] = { { 0, D2D1::ColorF(1, 1, 1, d ? 0.38f : 0.85f) }, { 0.5f, D2D1::ColorF(1, 1, 1, d ? 0.14f : 0.4f) }, { 1, D2D1::ColorF(1, 1, 1, d ? 0.12f : 0.35f) } };
    if (SUCCEEDED(dc->CreateGradientStopCollection(rst, 3, &rs))) {
        ID2D1LinearGradientBrush* lg = nullptr;
        if (SUCCEEDED(dc->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({ 0, r.top }, { 0, r.bottom }), rs, &lg))) {
            dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 0.75f, r.top + 0.75f, r.right - 0.75f, r.bottom - 0.75f), rad - 0.75f, rad - 0.75f), lg, 1.5f);
            lg->Release();
        }
        rs->Release();
    }
    if (clip) { dc->PopLayer(); clip->Release(); }
    if (f) f->Release();
    br->SetColor(D2D1::ColorF(0, 0, 0, d ? 0.5f : 0.10f));
    dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left - 0.25f, r.top - 0.25f, r.right + 0.25f, r.bottom + 0.25f), rad, rad), br, 0.5f);
}

// botão redondo de pílula (36): realce ao passar, azul quando ligado
void PillBtn(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, D2D1_RECT_F r, const char* icon, bool on, bool dashed = false, bool bold = false) {
    D2D1_POINT_2F m{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
    if (on) { br->SetColor(SK_BLUE); dc->FillEllipse(D2D1::Ellipse(m, 18, 18), br); }
    else if (In(r, g_sk.hover)) { br->SetColor(SkHover()); dc->FillEllipse(D2D1::Ellipse(m, 18, 18), br); }
    instrumentos::Icon(dc, icon, on ? D2D1::ColorF(1, 1, 1) : SkText(), m.x, m.y, 20, dashed, bold ? 3.f : 1.9f);
}

void DrawSketchTop(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    const float TOOL_CY = ::TOOL_CY + g_sk.top;
    D2D1_MATRIX_3X2_F old; dc->GetTransform(&old);
    dc->SetTransform(D2D1::Matrix3x2F::Translation(0, g_sk.top) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
    DrawLights(dc, br);
    dc->SetTransform(old);
    Glass(dc, br, g_sk.pillC, 22);
    PillBtn(dc, br, g_sk.bPen, IC_PEN, g_sk.paletteOn && !g_sk.hand);
    PillBtn(dc, br, g_sk.bLasso, IC_LASSO, false, true);
    PillBtn(dc, br, g_sk.bHand, IC_HAND, g_sk.hand);
    if (In(g_sk.bZoom, g_sk.hover)) { br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(g_sk.bZoom, 18, 18), br); }
    wchar_t z[16]; swprintf_s(z, L"%d%%", (int)std::lround(g_sk.vz * 100));
    Text(dc, br, z, g_f13c, D2D1::RectF(g_sk.bZoom.left, TOOL_CY - 9, g_sk.bZoom.right, TOOL_CY + 10), SkMuted());
    Glass(dc, br, g_sk.pillR, 22);
    PillBtn(dc, br, g_sk.bPhoto, IC_PHOTO, false);
    PillBtn(dc, br, g_sk.bExport, IC_SHARE, g_sk.pop == POP_EXPORT);
    PillBtn(dc, br, g_sk.bMore, IC_MORE, g_sk.pop == POP_MORE, false, true);
}

void DrawRuler(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    auto& R = g_sk.ruler;
    bool d = g_sk.paper == 2;
    D2D1_POINT_2F m = SkScreen(R.cx, R.cy);
    D2D1_MATRIX_3X2_F old; dc->GetTransform(&old);
    dc->SetTransform(D2D1::Matrix3x2F::Rotation(R.angle * 180 / 3.14159265f) * D2D1::Matrix3x2F::Scale(g_sk.vz, g_sk.vz) * D2D1::Matrix3x2F::Translation(m.x, m.y) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
    D2D1_RECT_F b{ -SK_RULER_HL, -SK_RULER_HW, SK_RULER_HL, SK_RULER_HW };
    for (int k = 6; k >= 1; k--) { br->SetColor(D2D1::ColorF(0, 0, 0, 0.018f)); dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(b.left - k, b.top - k + 3, b.right + k, b.bottom + k + 3), 6.f + k, 6.f + k), br); }
    br->SetColor(d ? D2D1::ColorF(0.26f, 0.27f, 0.31f, 0.78f) : D2D1::ColorF(0.93f, 0.95f, 0.99f, 0.78f));
    dc->FillRoundedRectangle(D2D1::RoundedRect(b, 6, 6), br);
    br->SetColor(d ? D2D1::ColorF(1, 1, 1, 0.18f) : D2D1::ColorF(0, 0, 0, 0.14f));
    dc->DrawRoundedRectangle(D2D1::RoundedRect(b, 6, 6), br, 1 / g_sk.vz);
    br->SetColor(d ? D2D1::ColorF(1, 1, 1, 0.55f) : D2D1::ColorF(0x3a / 255.f, 0x3c / 255.f, 0x44 / 255.f, 0.6f));
    for (int i = 0; i * 8 <= 2 * SK_RULER_HL - 16; i++) {
        float x = -SK_RULER_HL + 8 + i * 8, l = i % 10 == 0 ? 16.f : i % 5 == 0 ? 11.f : 6.f;
        dc->DrawLine({ x, b.top }, { x, b.top + l }, br, 1 / g_sk.vz);
        dc->DrawLine({ x, b.bottom }, { x, b.bottom - l }, br, 1 / g_sk.vz);
    }
    dc->SetTransform(old);
    int deg = (int)std::lround(std::fmod(std::fmod(-R.angle * 180 / 3.14159265f, 180.f) + 180.f, 180.f));
    wchar_t t[16]; swprintf_s(t, L"%d°", deg);
    br->SetColor(d ? D2D1::ColorF(0.12f, 0.12f, 0.14f, 0.92f) : D2D1::ColorF(1, 1, 1, 0.92f));
    dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m.x - 26, m.y - 13, m.x + 26, m.y + 13), 13, 13), br);
    Text(dc, br, t, g_f13c, D2D1::RectF(m.x - 26, m.y - 9, m.x + 26, m.y + 13), d ? D2D1::ColorF(0xf2f2f5) : D2D1::ColorF(0x1c1c1f));
}

void ColorWheel(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, D2D1_POINT_2F c, float R) {
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    if (!f) return;
    static const UINT32 hues[] = { 0xff3b30, 0xff9500, 0xffcc00, 0x34c759, 0x00c7be, 0x0a84ff, 0x5e5ce6, 0xbf5af2, 0xff2d55 };
    const int N = 9;
    for (int s = 0; s < N; s++) {
        float a0 = -1.5708f + s * 6.2832f / N, a1 = a0 + 6.2832f / N + 0.02f;
        ID2D1PathGeometry* g = nullptr;
        if (FAILED(f->CreatePathGeometry(&g))) break;
        ID2D1GeometrySink* sk = nullptr; g->Open(&sk);
        sk->BeginFigure(c, D2D1_FIGURE_BEGIN_FILLED);
        sk->AddLine({ c.x + R * std::cos(a0), c.y + R * std::sin(a0) });
        sk->AddArc(D2D1::ArcSegment({ c.x + R * std::cos(a1), c.y + R * std::sin(a1) }, D2D1::SizeF(R, R), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
        sk->EndFigure(D2D1_FIGURE_END_CLOSED); sk->Close(); sk->Release();
        br->SetColor(D2D1::ColorF(hues[s])); dc->FillGeometry(g, br); g->Release();
    }
    f->Release();
}

void DrawPalette(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    if (!g_sk.paletteOn) return;
    auto& P = g_sk.palette;
    Glass(dc, br, P, 34);
    D2D1_COLOR_F text = SkText();
    for (int k = 0; k < 2; k++) {
        auto r = k ? g_sk.bRedo : g_sk.bUndo;
        bool en = k ? !g_sk.redo.empty() : !g_sk.undo.empty();
        D2D1_POINT_2F c{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
        br->SetColor(SkHover()); dc->FillEllipse(D2D1::Ellipse(c, 18, 18), br);
        instrumentos::Icon(dc, k ? IC_REDO : IC_UNDO, Alpha(text, en ? 1.f : 0.35f), c.x, c.y, 18);
    }
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    ID2D1RoundedRectangleGeometry* clip = nullptr;
    if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(P, 34, 34), &clip);
    if (clip) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip), nullptr);
    int active = SkTool();
    for (int k = 0; k < SK_NSLOTS; k++) {
        int id = SK_SLOTS[k];
        bool on = id == SK_RULER_SLOT ? g_sk.ruler.on : id == active && !g_sk.hand;
        float dy = on ? 8.f : k == g_sk.hoverInst ? 20.f : 26.f;
        auto r = g_sk.inst[k];
        D2D1_COLOR_F col = id < tinta::TOOL_COUNT ? g_sk.color[id] : D2D1::ColorF(0x1c1c1f);
        if (!instrumentos::Draw(dc, SkArt(id), col, r.left, r.top + dy, id == SK_RULER_SLOT && g_sk.ruler.on)) {
            br->SetColor(D2D1::ColorF(0.95f, 0.95f, 0.97f)); dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 11, r.top + dy + 30, r.right - 11, r.bottom + 20), 3, 3), br);
            br->SetColor(col); dc->FillRectangle(D2D1::RectF(r.left + 16, r.top + dy + 8, r.right - 16, r.top + dy + 30), br);
        }
    }
    if (clip) { dc->PopLayer(); clip->Release(); }
    if (f) f->Release();
    float sx = g_sk.inst[SK_NSLOTS - 1].right + 14;
    br->SetColor(SkHover());
    dc->FillRectangle(D2D1::RectF(sx, (P.top + P.bottom) / 2 - 26, sx + 1, (P.top + P.bottom) / 2 + 26), br);
    int ink = SkInk() ? g_sk.tool : -1;
    int selSw = -1;
    if (ink >= 0) { selSw = 5; for (int i = 0; i < 5; i++) if (SameColor(g_sk.color[ink], SK_COLORS[i])) selSw = i; }
    for (int i = 0; i < 6; i++) {
        auto r = g_sk.sw[i];
        D2D1_POINT_2F c{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
        if (i < 5) {
            br->SetColor(SK_COLORS[i]); dc->FillEllipse(D2D1::Ellipse(c, 14, 14), br);
            if (i == 0 && SkDark()) { br->SetColor(D2D1::ColorF(1, 1, 1, 0.35f)); dc->DrawEllipse(D2D1::Ellipse(c, 13.5f, 13.5f), br, 1); }  // o preto aparece no vidro escuro
        }
        else {
            ColorWheel(dc, br, c, 14);
            br->SetColor(ink >= 0 && selSw == 5 ? g_sk.color[ink] : g_sk.custom); dc->FillEllipse(D2D1::Ellipse(c, 7, 7), br);
            br->SetColor(D2D1::ColorF(1, 1, 1)); dc->DrawEllipse(D2D1::Ellipse(c, 8, 8), br, 2);
        }
        br->SetColor(D2D1::ColorF(0, 0, 0, 0.12f)); dc->DrawEllipse(D2D1::Ellipse(c, 13.5f, 13.5f), br, 1);
        if (i == selSw || (i == 5 && g_sk.pop == POP_COLORS)) { br->SetColor(SK_BLUE); dc->DrawEllipse(D2D1::Ellipse(c, 16.75f, 16.75f), br, 2.5f); }
    }
}

void DrawColors(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    auto& P = g_sk.popR;
    D2D1_COLOR_F text = SkText();
    Text(dc, br, TrPick(L"CORES", L"COLORS"), g_f11b, D2D1::RectF(P.left + 16, P.top + 14, P.right - 16, P.top + 32), SkMuted());
    int t = SkInk() ? g_sk.tool : tinta::PEN;
    D2D1_COLOR_F cur = g_sk.color[t];
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    auto& G = g_sk.gridR;
    ID2D1RoundedRectangleGeometry* clip = nullptr;
    if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(G, 10, 10), &clip);
    if (clip) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip), nullptr);
    int onR = -1, onC = -1;
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 12; c++) {
            D2D1_COLOR_F k = GridColor(r, c);
            if (SameColor(k, cur)) { onR = r; onC = c; }
            br->SetColor(k);
            dc->FillRectangle(D2D1::RectF(G.left + c * 24, G.top + r * 22, G.left + c * 24 + 24.5f, G.top + r * 22 + 22.5f), br);
        }
    if (clip) { dc->PopLayer(); clip->Release(); }
    br->SetColor(D2D1::ColorF(0, 0, 0, 0.08f)); dc->DrawRoundedRectangle(D2D1::RoundedRect(G, 10, 10), br, 1);
    if (onR >= 0) {
        D2D1_RECT_F cell{ G.left + onC * 24 + 2, G.top + onR * 22 + 2, G.left + onC * 24 + 22, G.top + onR * 22 + 20 };
        br->SetColor(onR == 0 && onC < 4 ? D2D1::ColorF(0, 0, 0, 0.6f) : D2D1::ColorF(1, 1, 1)); dc->DrawRoundedRectangle(D2D1::RoundedRect(cell, 3, 3), br, 2);
    }
    auto& S = g_sk.specR;
    ID2D1RoundedRectangleGeometry* sclip = nullptr;
    if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(S, 9, 9), &sclip);
    if (sclip) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), sclip), nullptr);
    for (float x = S.left; x < S.right; x += 2)
        for (float y = S.top; y < S.bottom; y += 2) {
            br->SetColor(SpectrumColor((x + 1 - S.left) / (S.right - S.left), (y + 1 - S.top) / (S.bottom - S.top)));
            dc->FillRectangle(D2D1::RectF(x, y, x + 2.4f, y + 2.4f), br);
        }
    if (sclip) { dc->PopLayer(); sclip->Release(); }
    br->SetColor(D2D1::ColorF(0, 0, 0, 0.08f)); dc->DrawRoundedRectangle(D2D1::RoundedRect(S, 9, 9), br, 1);
    if (f) f->Release();
    float y = S.bottom + 12;
    D2D1_POINT_2F pc{ P.left + 16 + 15, y + 15 };
    br->SetColor(cur); dc->FillEllipse(D2D1::Ellipse(pc, 15, 15), br);
    br->SetColor(D2D1::ColorF(0, 0, 0, 0.15f)); dc->DrawEllipse(D2D1::Ellipse(pc, 14.5f, 14.5f), br, 1);
    wchar_t hex[16]; swprintf_s(hex, L"#%02X%02X%02X", (int)(cur.r * 255 + 0.5f), (int)(cur.g * 255 + 0.5f), (int)(cur.b * 255 + 0.5f));
    Text(dc, br, hex, g_f13, D2D1::RectF(P.left + 56, y + 6, P.right - 60, y + 26), text);
    auto& E = g_sk.bEyedrop;
    br->SetColor(g_sk.eyedrop ? Alpha(SK_BLUE, 0.18f) : SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(E, 9, 9), br);
    instrumentos::Icon(dc, IC_DROPPER, g_sk.eyedrop ? SK_BLUE : text, (E.left + E.right) / 2, (E.top + E.bottom) / 2, 18);
}

// tamanhos (o rabisco de cada um, como no Quill) e a opacidade
void DrawToolPop(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    auto t = (tinta::Tool)g_sk.pop;
    tinta::DarkPaper() = SkDark();
    D2D1_COLOR_F col = tinta::Shown(g_sk.color[t]);
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    ID2D1StrokeStyle* rs = nullptr;
    if (f) f->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND), nullptr, 0, &rs);
    for (int i = 0; i < 5; i++) {
        auto r = g_sk.popItem[i];
        if (i == g_sk.size[t]) {
            br->SetColor(Alpha(SK_BLUE, 0.16f)); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 12, 12), br);
            br->SetColor(SK_BLUE); dc->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 0.75f, r.top + 0.75f, r.right - 0.75f, r.bottom - 0.75f), 11.25f, 11.25f), br, 1.5f);
        } else if (In(r, g_sk.hover)) { br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 12, 12), br); }
        ID2D1PathGeometry* g = nullptr;
        if (f && SUCCEEDED(f->CreatePathGeometry(&g))) {
            ID2D1GeometrySink* sk = nullptr; g->Open(&sk);
            float ox = r.left + 4, oy = r.top + 4;  // "M8 26 C 14 10, 22 32, 32 14" num quadro de 40
            sk->BeginFigure({ ox + 8, oy + 26 }, D2D1_FIGURE_BEGIN_HOLLOW);
            sk->AddBezier(D2D1::BezierSegment({ ox + 14, oy + 10 }, { ox + 22, oy + 32 }, { ox + 32, oy + 14 }));
            sk->EndFigure(D2D1_FIGURE_END_OPEN); sk->Close(); sk->Release();
            br->SetColor(Alpha(col, (t == tinta::MARKER ? 0.6f : 1.f) * std::max(0.2f, g_sk.opacity[t])));
            dc->DrawGeometry(g, br, std::max(1.2f, std::min(11.f, 1.2f + i * 2.3f)), rs);
            g->Release();
        }
    }
    if (rs) rs->Release();
    // opacidade: xadrez por baixo, a cor subindo, o botão branco e o número
    auto& T = g_sk.opTrack;
    ID2D1RoundedRectangleGeometry* clip = nullptr;
    if (f) f->CreateRoundedRectangleGeometry(D2D1::RoundedRect(T, 13, 13), &clip);
    if (clip) dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip), nullptr);
    br->SetColor(D2D1::ColorF(1, 1, 1)); dc->FillRectangle(T, br);
    br->SetColor(D2D1::ColorF(0xd4d4d8));
    int row = 0;
    for (float y = T.top; y < T.bottom; y += 6, row++)
        for (float x = T.left + (row % 2) * 6; x < T.right; x += 12) dc->FillRectangle(D2D1::RectF(x, y, x + 6, y + 6), br);
    ID2D1GradientStopCollection* stops = nullptr;
    D2D1_GRADIENT_STOP gs[2] = { { 0, Alpha(col, 0) }, { 1, Alpha(col, 1) } };
    if (SUCCEEDED(dc->CreateGradientStopCollection(gs, 2, &stops))) {
        ID2D1LinearGradientBrush* lg = nullptr;
        if (SUCCEEDED(dc->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({ T.left, 0 }, { T.right, 0 }), stops, &lg))) { dc->FillRectangle(T, lg); lg->Release(); }
        stops->Release();
    }
    if (clip) { dc->PopLayer(); clip->Release(); }
    if (f) f->Release();
    br->SetColor(D2D1::ColorF(0, 0, 0, 0.1f)); dc->DrawRoundedRectangle(D2D1::RoundedRect(T, 13, 13), br, 1);
    float op = g_sk.opacity[t], tx = T.left + 13 + (T.right - T.left - 26) * (op - 0.1f) / 0.9f;
    D2D1_POINT_2F th{ tx, (T.top + T.bottom) / 2 };
    for (int k = 4; k >= 1; k--) { br->SetColor(D2D1::ColorF(0, 0, 0, 0.06f)); dc->FillEllipse(D2D1::Ellipse({ th.x, th.y + 1 }, 13.f + k * 0.6f, 13.f + k * 0.6f), br); }
    br->SetColor(D2D1::ColorF(1, 1, 1)); dc->FillEllipse(D2D1::Ellipse(th, 13, 13), br);
    br->SetColor(D2D1::ColorF(0, 0, 0, 0.08f)); dc->DrawEllipse(D2D1::Ellipse(th, 12.5f, 12.5f), br, 1);
    wchar_t pct[8]; swprintf_s(pct, L"%d%%", (int)std::lround(op * 100));
    Text(dc, br, pct, g_f13, D2D1::RectF(T.right + 12, T.top + 4, g_sk.popR.right - 12, T.bottom), SkMuted());
    // pena de junco: o ângulo do corte (uma pena pequena mostra a inclinação no botão)
    if (g_sk.angTrack.right > 0) {
        auto& A = g_sk.angTrack;
        br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(A, 13, 13), br);
        float cy2 = (A.top + A.bottom) / 2;
        br->SetColor(Alpha(SkText(), 0.18f)); dc->FillRectangle(D2D1::RectF(A.left + 13, cy2 - 1, A.right - 13, cy2 + 1), br);
        float ax = A.left + 13 + (A.right - A.left - 26) * g_sk.nib / 180;
        D2D1_POINT_2F th2{ ax, cy2 };
        for (int k = 4; k >= 1; k--) { br->SetColor(D2D1::ColorF(0, 0, 0, 0.06f)); dc->FillEllipse(D2D1::Ellipse({ th2.x, th2.y + 1 }, 13.f + k * 0.6f, 13.f + k * 0.6f), br); }
        br->SetColor(D2D1::ColorF(1, 1, 1)); dc->FillEllipse(D2D1::Ellipse(th2, 13, 13), br);
        float ar = -g_sk.nib * 3.14159265f / 180;
        br->SetColor(col);
        dc->DrawLine({ th2.x - 7 * std::cos(ar), th2.y - 7 * std::sin(ar) }, { th2.x + 7 * std::cos(ar), th2.y + 7 * std::sin(ar) }, br, 2.5f);
        wchar_t deg[8]; swprintf_s(deg, L"%d°", (int)g_sk.nib);
        Text(dc, br, deg, g_f13, D2D1::RectF(A.right + 12, A.top + 4, g_sk.popR.right - 12, A.bottom), SkMuted());
    }
}

void DrawPopover(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br) {
    if (g_sk.pop < 0 || g_sk.popR.right <= 0) return;
    bool menu = g_sk.pop == POP_MORE || g_sk.pop == POP_EXPORT;
    Glass(dc, br, g_sk.popR, menu ? 14.f : 22.f);
    D2D1_COLOR_F text = SkText();
    if (g_sk.pop == POP_COLORS) { DrawColors(dc, br); return; }
    if (menu) {
        auto& P = g_sk.popR;
        if (g_sk.pop == POP_MORE) {
            Text(dc, br, TrPick(L"PAPEL", L"PAPER"), g_f11b, D2D1::RectF(P.left + 16, P.top + 12, P.right, P.top + 30), SkMuted());
            for (int i = 0; i < 3; i++) {
                auto r = g_sk.paperSw[i];
                D2D1_POINT_2F c{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
                br->SetColor(SK_PAPERS[i]); dc->FillEllipse(D2D1::Ellipse(c, 13, 13), br);
                br->SetColor(D2D1::ColorF(0, 0, 0, 0.18f)); dc->DrawEllipse(D2D1::Ellipse(c, 12.5f, 12.5f), br, 1);
                if (i == g_sk.paper) { br->SetColor(SK_BLUE); dc->DrawEllipse(D2D1::Ellipse(c, 16.5f, 16.5f), br, 2.5f); }
            }
            const wchar_t* pt[4] = { L"Liso", L"Pautado", L"Grade", L"Pontos" };
            const wchar_t* en[4] = { L"Plain", L"Lined", L"Grid", L"Dots" };
            for (int i = 0; i < 4; i++) {
                auto r = g_sk.patChip[i];
                bool on = i == g_sk.pattern;
                br->SetColor(on ? Alpha(SK_BLUE, 0.16f) : SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 13, 13), br);
                Text(dc, br, TrPick(pt[i], en[i]), g_fCardT, D2D1::RectF(r.left, r.top + 5, r.right, r.bottom), on ? SK_BLUE : text);
            }
            br->SetColor(SkHover()); dc->FillRectangle(D2D1::RectF(P.left + 12, g_sk.patChip[0].bottom + 8, P.right - 12, g_sk.patChip[0].bottom + 9), br);
        }
        for (int i = 0; i < g_sk.menuN; i++) {
            auto r = g_sk.menuR[i];
            if (In(r, g_sk.hover)) { br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 8, 8), br); }
            std::wstring label;
            D2D1_COLOR_F c = text;
            switch (g_sk.menuId[i]) {
            case 1: label = TrPick(L"Ajustar à janela", L"Zoom to Fit"); break;
            case 2: label = g_sk.ruler.on ? TrPick(L"Ocultar régua", L"Hide Ruler") : TrPick(L"Mostrar régua", L"Show Ruler"); break;
            case 3: label = SkDark() && g_sk.paper != 2 ? TrPick(L"Aparência clara", L"Light Appearance") : TrPick(L"Aparência escura", L"Dark Appearance"); break;
            case 4: label = TrPick(L"Remover foto", L"Remove Photo"); break;
            case 5: label = TrPick(L"Limpar desenho", L"Clear Drawing"); c = D2D1::ColorF(0.95f, 0.27f, 0.25f); break;
            case 6: label = TrPick(L"Descartar alterações", L"Discard Changes"); c = D2D1::ColorF(0.95f, 0.27f, 0.25f); break;
            case 10: label = g_sk.editPath.empty() ? TrPick(L"Pôr na nota", L"Add to Note") : TrPick(L"Atualizar na nota", L"Update in Note"); break;
            case 11: label = TrPick(L"Salvar PNG em Imagens", L"Save PNG to Pictures"); break;
            case 12: label = TrPick(L"Copiar imagem", L"Copy Image"); break;
            }
            Text(dc, br, label, g_f13, D2D1::RectF(r.left + 10, r.top + 7, r.right - 8, r.bottom), c);
        }
        return;
    }
    static const wchar_t* namesPt[] = { L"Caneta", L"Caneta-tinteiro", L"Marcador", L"Lápis", L"Giz de cera", L"Aquarela", L"Monolinha", L"Pincel de tinta", L"Pena de junco", L"Borracha" };
    static const wchar_t* namesEn[] = { L"Pen", L"Fountain pen", L"Marker", L"Pencil", L"Crayon", L"Watercolor", L"Monoline", L"Brush pen", L"Reed pen", L"Eraser" };
    std::wstring title = TrPick(namesPt[g_sk.pop], namesEn[g_sk.pop]);
    std::transform(title.begin(), title.end(), title.begin(), ::towupper);
    Text(dc, br, title, g_f11b, D2D1::RectF(g_sk.popR.left + 16, g_sk.popR.top + 14, g_sk.popR.right - 16, g_sk.popR.top + 32), SkMuted());
    if (g_sk.pop == SK_ERASER) {
        D2D1_RECT_F seg{ g_sk.popItem[0].left - 3, g_sk.popItem[0].top - 3, g_sk.popItem[1].right + 3, g_sk.popItem[1].bottom + 3 };
        br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(seg, 12, 12), br);
        for (int i = 0; i < 2; i++) {
            auto r = g_sk.popItem[i];
            bool on = (i == 1) == g_sk.eraseObject;
            if (on) { br->SetColor(SkDark() ? D2D1::ColorF(1, 1, 1, 0.16f) : D2D1::ColorF(1, 1, 1)); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 9, 9), br); }
            Text(dc, br, i ? TrPick(L"Borracha de traço", L"Object Eraser") : TrPick(L"Borracha de pixel", L"Pixel Eraser"), g_f13c, D2D1::RectF(r.left, r.top + 7, r.right, r.bottom), text);
        }
        return;
    }
    DrawToolPop(dc, br);
}

tinta::Box SkSelBox();
// alças da seleção do laço: os quatro cantos da caixa (com folga) e a de girar em cima, como na foto
inline D2D1_POINT_2F SelCorner(int i) {
    auto b = SkSelBox(); float pad = 4 / g_sk.vz;
    return { i == 0 || i == 3 ? b.x1 - pad : b.x2 + pad, i < 2 ? b.y1 - pad : b.y2 + pad };
}
inline D2D1_POINT_2F SelRotHandle() { auto b = SkSelBox(); return { (b.x1 + b.x2) / 2, b.y1 - 4 / g_sk.vz - 30 / g_sk.vz }; }

// o mesmo desenho para a foto e a seleção: contorno tracejado azul, alças brancas de aro azul e a de girar
void SkDrawFrame(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, const D2D1_POINT_2F cs[4], D2D1_POINT_2F topMid, D2D1_POINT_2F rot, bool handles) {
    ID2D1Factory* pf = nullptr; dc->GetFactory(&pf);
    ID2D1StrokeStyle* dsh = nullptr;
    if (pf) { float d[2] = { 4, 3 }; pf->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10, D2D1_DASH_STYLE_CUSTOM), d, 2, &dsh); }
    br->SetColor(SK_BLUE);
    for (int i = 0; i < 4; i++) dc->DrawLine(cs[i], cs[(i + 1) % 4], br, 1.5f, dsh);
    if (handles) {
        dc->DrawLine(topMid, rot, br, 1.5f);
        for (int i = 0; i <= 4; i++) {
            D2D1_POINT_2F h = i < 4 ? cs[i] : rot;
            for (int k = 3; k >= 1; k--) { br->SetColor(D2D1::ColorF(0, 0, 0, 0.06f)); dc->FillEllipse(D2D1::Ellipse({ h.x, h.y + 1 }, 7.f + k, 7.f + k), br); }
            br->SetColor(D2D1::ColorF(1, 1, 1)); dc->FillEllipse(D2D1::Ellipse(h, 7, 7), br);
            br->SetColor(SK_BLUE); dc->DrawEllipse(D2D1::Ellipse(h, 6.25f, 6.25f), br, 1.5f);
            if (i == 4) instrumentos::Icon(dc, "M20 11a8 8 0 1 0-2.3 5.7M20 4v7h-7", SK_BLUE, h.x, h.y, 9, false, 2.6f);
        }
    }
    if (dsh) dsh->Release();
    if (pf) pf->Release();
}

void DrawSketch(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* br, const Pal& pal) {
    if (!g_sk.on) return;
    SketchLayout();
    auto& C = g_sk.canvas;
    bool darkPaper = g_sk.paper == 2;
    SkEnsureCache(dc, br);
    ID2D1Bitmap* bm = nullptr;
    if (g_sk.cache && SUCCEEDED(g_sk.cache->GetBitmap(&bm)) && bm) { dc->DrawBitmap(bm, C, 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR); bm->Release(); }
    else {  // sem a camada guardada: desenha tudo direto
        D2D1_POINT_2F a = SkWorld({ C.left, C.top }), b = SkWorld({ C.right, C.bottom });
        SkPaintScene(dc, br, SkOrigin(), g_sk.vz, D2D1::RectF(a.x, a.y, b.x, b.y), nullptr, true);
    }
    if (g_sk.drawing && !g_sk.strokes.empty()) {  // o traço vivo
        tinta::DarkPaper() = darkPaper;
        tinta::Draw(dc, br, g_sk.strokes.back(), SkOrigin(), g_sk.vz, true);
    }
    ID2D1Factory* f = nullptr; dc->GetFactory(&f);
    ID2D1StrokeStyle* dash = nullptr;
    if (f) { float d[2] = { 4, 3 }; f->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10, D2D1_DASH_STYLE_CUSTOM), d, 2, &dash); }
    if (!g_sk.sel.empty()) {
        tinta::Box b{ 1e9f, 1e9f, -1e9f, -1e9f };
        for (int i : g_sk.sel) { auto o = tinta::BBox(g_sk.strokes[i]); b.x1 = std::min(b.x1, o.x1); b.y1 = std::min(b.y1, o.y1); b.x2 = std::max(b.x2, o.x2); b.y2 = std::max(b.y2, o.y2); }
        (void)b;
        D2D1_POINT_2F cs[4];
        for (int i = 0; i < 4; i++) { auto w2 = SelCorner(i); cs[i] = SkScreen(w2.x, w2.y); }
        auto rh = SelRotHandle();
        SkDrawFrame(dc, br, cs, D2D1::Point2F((cs[0].x + cs[1].x) / 2, cs[0].y), SkScreen(rh.x, rh.y), g_sk.mode != Sketch::M_MOVE);
    }
    if (g_sk.mode == Sketch::M_LASSO && g_sk.lasso.size() > 1 && f) {
        ID2D1PathGeometry* g = nullptr;
        if (SUCCEEDED(f->CreatePathGeometry(&g))) {
            ID2D1GeometrySink* sk = nullptr; g->Open(&sk);
            sk->BeginFigure(SkScreen(g_sk.lasso[0].x, g_sk.lasso[0].y), D2D1_FIGURE_BEGIN_HOLLOW);
            for (size_t i = 1; i < g_sk.lasso.size(); i++) sk->AddLine(SkScreen(g_sk.lasso[i].x, g_sk.lasso[i].y));
            sk->EndFigure(D2D1_FIGURE_END_OPEN); sk->Close(); sk->Release();
            br->SetColor(SK_BLUE); dc->DrawGeometry(g, br, 1.5f, dash);  // só a linha que está sendo feita
            g->Release();
        }
    }
    if (dash) dash->Release();
    if (f) f->Release();
    if (g_sk.ruler.on) DrawRuler(dc, br);
    // foto escolhida: contorno tracejado girado, as quatro alças de tamanho e a de girar
    if (g_sk.photo.on && (g_sk.photoSel || g_sk.crop)) {
        ID2D1Factory* pf = nullptr; dc->GetFactory(&pf);
        ID2D1StrokeStyle* dsh = nullptr;
        if (pf) { float d[2] = { 4, 3 }; pf->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10, D2D1_DASH_STYLE_CUSTOM), d, 2, &dsh); }
        D2D1_POINT_2F cs[4];
        for (int i = 0; i < 4; i++) { auto w2 = PhCorner(i); cs[i] = SkScreen(w2.x, w2.y); }
        br->SetColor(SK_BLUE);
        for (int i = 0; i < 4; i++) dc->DrawLine(cs[i], cs[(i + 1) % 4], br, 1.5f, dsh);
        if (!g_sk.crop) {
            auto rh = PhRotHandle(); auto rs = SkScreen(rh.x, rh.y);
            auto top = SkScreen(PhToWorld(0, -g_sk.photo.h / 2).x, PhToWorld(0, -g_sk.photo.h / 2).y);
            dc->DrawLine(top, rs, br, 1.5f);
            for (int i = 0; i <= 4; i++) {
                D2D1_POINT_2F h = i < 4 ? cs[i] : rs;
                for (int k = 3; k >= 1; k--) { br->SetColor(D2D1::ColorF(0, 0, 0, 0.06f)); dc->FillEllipse(D2D1::Ellipse({ h.x, h.y + 1 }, 7.f + k, 7.f + k), br); }
                br->SetColor(D2D1::ColorF(1, 1, 1)); dc->FillEllipse(D2D1::Ellipse(h, 7, 7), br);
                br->SetColor(SK_BLUE); dc->DrawEllipse(D2D1::Ellipse(h, 6.25f, 6.25f), br, 1.5f);
                if (i == 4) instrumentos::Icon(dc, "M20 11a8 8 0 1 0-2.3 5.7M20 4v7h-7", SK_BLUE, h.x, h.y, 9, false, 2.6f);
            }
        }
        // a volta do recorte sendo desenhada
        if (g_sk.mode == Sketch::M_CROP && g_sk.cropPts.size() > 1 && pf) {
            ID2D1PathGeometry* g = nullptr;
            if (SUCCEEDED(pf->CreatePathGeometry(&g))) {
                ID2D1GeometrySink* sk = nullptr; g->Open(&sk);
                sk->BeginFigure(SkScreen(g_sk.cropPts[0].x, g_sk.cropPts[0].y), D2D1_FIGURE_BEGIN_FILLED);
                for (size_t i = 1; i < g_sk.cropPts.size(); i++) sk->AddLine(SkScreen(g_sk.cropPts[i].x, g_sk.cropPts[i].y));
                sk->EndFigure(D2D1_FIGURE_END_CLOSED); sk->Close(); sk->Release();
                br->SetColor(Alpha(SK_BLUE, 0.10f)); dc->FillGeometry(g, br);
                br->SetColor(SK_BLUE); dc->DrawGeometry(g, br, 2, dsh);
                g->Release();
            }
        }
        if (dsh) dsh->Release();
        if (pf) pf->Release();
    }
    if (SkTool() == SK_ERASER && !SkPanning() && !g_sk.eraseObject && In(C, g_sk.hover) && g_sk.hover.y > TOOLBAR_H + g_sk.top && g_sk.pop < 0) {
        br->SetColor(darkPaper ? D2D1::ColorF(0, 0, 0, 0.35f) : D2D1::ColorF(1, 1, 1, 0.5f)); dc->FillEllipse(D2D1::Ellipse(g_sk.hover, SK_ERASE_R, SK_ERASE_R), br);
        br->SetColor(darkPaper ? D2D1::ColorF(1, 1, 1, 0.6f) : D2D1::ColorF(0, 0, 0, 0.45f)); dc->DrawEllipse(D2D1::Ellipse(g_sk.hover, SK_ERASE_R, SK_ERASE_R), br, 1);
    }
    if (g_sk.strokes.empty() && !g_sk.photo.on && g_sk.mode == Sketch::M_NONE)
        Text(dc, br, Tr(L"Desenhe aqui com o mouse ou a caneta"), g_f13c, D2D1::RectF(C.left, (C.top + C.bottom) / 2 - 60, C.right, (C.top + C.bottom) / 2 - 40), darkPaper ? D2D1::ColorF(1, 1, 1, 0.3f) : D2D1::ColorF(0, 0, 0, 0.28f));
    if (g_sk.photoMenu.right > 0) {
        Glass(dc, br, g_sk.photoMenu, 18);
        auto lab = [&](D2D1_RECT_F r, const std::wstring& t, D2D1_COLOR_F c) { if (In(r, g_sk.hover)) { br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(r, 14, 14), br); } Text(dc, br, t, g_f13c, D2D1::RectF(r.left, r.top + 5, r.right, r.bottom), c); };
        lab(g_sk.bCrop, TrPick(L"Recortar", L"Crop"), SkText());
        lab(g_sk.bRot90, TrPick(L"Girar", L"Rotate"), SkText());
        if (g_sk.bUncrop.right > 0) lab(g_sk.bUncrop, TrPick(L"Foto inteira", L"Uncrop"), SkText());
        lab(g_sk.bPhotoDel, Tr(L"Apagar"), D2D1::ColorF(0.95f, 0.25f, 0.25f));
    }
    if (g_sk.cropHint.right > 0) {
        Glass(dc, br, g_sk.cropHint, 19);
        Text(dc, br, TrPick(L"Desenhe uma volta em torno do que fica", L"Draw a loop around the part to keep"), g_f13, D2D1::RectF(g_sk.cropHint.left + 16, g_sk.cropHint.top + 10, g_sk.bCropCancel.left, g_sk.cropHint.bottom), SkText());
        br->SetColor(SkHover()); dc->FillRoundedRectangle(D2D1::RoundedRect(g_sk.bCropCancel, 14, 14), br);
        Text(dc, br, Tr(L"Cancelar"), g_f13c, D2D1::RectF(g_sk.bCropCancel.left, g_sk.bCropCancel.top + 5, g_sk.bCropCancel.right, g_sk.bCropCancel.bottom), SkText());
    }
    if (g_sk.selMenu.right > 0) {
        Glass(dc, br, g_sk.selMenu, 18);
        Text(dc, br, Tr(L"Apagar"), g_f13c, D2D1::RectF(g_sk.bSelDel.left, g_sk.bSelDel.top + 5, g_sk.bSelDel.right, g_sk.bSelDel.bottom), D2D1::ColorF(0.95f, 0.25f, 0.25f));
        Text(dc, br, Tr(L"Duplicar"), g_f13c, D2D1::RectF(g_sk.bSelDup.left, g_sk.bSelDup.top + 5, g_sk.bSelDup.right, g_sk.bSelDup.bottom), SkText());
    }
    DrawPalette(dc, br);
    DrawSketchTop(dc, br);
    DrawPopover(dc, br);
    (void)pal;
}

// ---------------------------------------------------------------------------------------------------------------
// entrada

void SkPhotoReload() { if (g_sk.photoBmp) { g_sk.photoBmp->Release(); g_sk.photoBmp = nullptr; } }
void SkPushUndo() {
    g_sk.undo.push_back({ g_sk.strokes, g_sk.photo });
    if (g_sk.undo.size() > 100) g_sk.undo.erase(g_sk.undo.begin());
    g_sk.redo.clear();
    SkChanged();
}
void SkUndo() {
    if (g_sk.undo.empty()) return;
    g_sk.redo.push_back({ std::move(g_sk.strokes), g_sk.photo });
    if (g_sk.undo.back().p.path != g_sk.photo.path) SkPhotoReload();
    g_sk.strokes = std::move(g_sk.undo.back().s); g_sk.photo = g_sk.undo.back().p; g_sk.undo.pop_back();
    if (!g_sk.photo.on) g_sk.photoSel = false;
    g_sk.sel.clear();
    SkChanged();
}
void SkRedo() {
    if (g_sk.redo.empty()) return;
    g_sk.undo.push_back({ std::move(g_sk.strokes), g_sk.photo });
    if (g_sk.redo.back().p.path != g_sk.photo.path) SkPhotoReload();
    g_sk.strokes = std::move(g_sk.redo.back().s); g_sk.photo = g_sk.redo.back().p; g_sk.redo.pop_back();
    if (!g_sk.photo.on) g_sk.photoSel = false;
    g_sk.sel.clear();
    SkChanged();
}

void SkArmHold() {
    if (g_hwnd && g_sk.mode == Sketch::M_INK && !g_sk.snap.on && !g_sk.straight && !g_sk.shaped) SetTimer(g_hwnd, T_HOLD, 550, nullptr);
}

void SketchHold() {
    if (g_hwnd) KillTimer(g_hwnd, T_HOLD);
    if (!g_sk.on || g_sk.mode != Sketch::M_INK || g_sk.strokes.empty()) return;
    auto& s = g_sk.strokes.back();
    if (s.pts.size() >= 3 && tinta::Straightness(s.pts) > 0.82f) {
        g_sk.straight = true;
        s.pts = tinta::Line(s.pts[0].x, s.pts[0].y, s.pts.back().x, s.pts.back().y, s.pts.back().p);
    } else {
        auto sh = tinta::RecognizeShape(s.pts);
        if (!sh.empty()) { g_sk.shaped = true; s.pts = std::move(sh); }
    }
    Invalidate();
}

void SkRulerSnap(D2D1_POINT_2F q, float size) {
    auto& R = g_sk.ruler;
    g_sk.snap.on = false;
    if (!R.on) return;
    float ux = std::cos(R.angle), uy = std::sin(R.angle), nx = -uy, ny = ux;
    float dx = q.x - R.cx, dy = q.y - R.cy, t = dx * ux + dy * uy, d = dx * nx + dy * ny;
    if (std::fabs(t) > SK_RULER_HL || std::fabs(std::fabs(d) - SK_RULER_HW) > 34 / g_sk.vz) return;
    float side = d < 0 ? -1.f : 1.f;
    g_sk.snap = { true, R.cx, R.cy, ux, uy, nx, ny, side * (SK_RULER_HW + size / 2 + 1) };
}

void SkAddInk(D2D1_POINT_2F q, float pr) {
    if (g_sk.strokes.empty()) return;
    auto& s = g_sk.strokes.back();
    if (g_sk.snap.on) {
        auto& n = g_sk.snap;
        float t = (q.x - n.cx) * n.ux + (q.y - n.cy) * n.uy;
        q = { n.cx + n.ux * t + n.nx * n.off, n.cy + n.uy * t + n.ny * n.off };
    }
    if (!s.pts.empty() && std::hypot(q.x - s.pts.back().x, q.y - s.pts.back().y) < 0.4f / g_sk.vz) return;
    s.pts.push_back({ q.x, q.y, pr });
}

void SkEraseAt(D2D1_POINT_2F q) {
    std::vector<tinta::Stroke> next;
    bool changed = false;
    for (auto& s : g_sk.strokes) {
        if (g_sk.eraseObject) {
            if (tinta::Hit(s, q.x, q.y, 6 / g_sk.vz)) { changed = true; continue; }
            next.push_back(std::move(s));
        } else {
            std::vector<tinta::Stroke> pieces;
            if (tinta::EraseFrom(s, q.x, q.y, SK_ERASE_R / g_sk.vz, pieces)) { changed = true; for (auto& p : pieces) next.push_back(std::move(p)); }
            else next.push_back(std::move(s));
        }
    }
    g_sk.strokes = std::move(next);
    if (changed) { g_sk.changed = true; SkChanged(); }
}

void SkFinishLasso() {
    g_sk.sel.clear();
    auto& poly = g_sk.lasso;
    if (poly.size() >= 3)
        for (int i = 0; i < (int)g_sk.strokes.size(); i++) {
            auto& s = g_sk.strokes[i];
            size_t in = 0;
            for (auto& p : s.pts) if (tinta::InPolygon(p.x, p.y, poly)) in++;
            if (in && in * 2 >= s.pts.size()) g_sk.sel.push_back(i);
        }
    poly.clear();
}

tinta::Box SkSelBox() {
    tinta::Box b{ 1e9f, 1e9f, -1e9f, -1e9f };
    for (int i : g_sk.sel) { auto o = tinta::BBox(g_sk.strokes[i]); b.x1 = std::min(b.x1, o.x1); b.y1 = std::min(b.y1, o.y1); b.x2 = std::max(b.x2, o.x2); b.y2 = std::max(b.y2, o.y2); }
    return b;
}

void SkDeleteSel() {
    if (g_sk.sel.empty()) return;
    SkPushUndo();
    std::vector<int> s = g_sk.sel;
    std::sort(s.rbegin(), s.rend());
    for (int i : s) g_sk.strokes.erase(g_sk.strokes.begin() + i);
    g_sk.sel.clear();
}
void SkDuplicateSel() {
    if (g_sk.sel.empty()) return;
    SkPushUndo();
    std::vector<int> fresh;
    for (int i : g_sk.sel) {
        tinta::Stroke c = g_sk.strokes[i];
        for (auto& p : c.pts) { p.x += 20; p.y += 20; }
        g_sk.strokes.push_back(std::move(c));
        fresh.push_back((int)g_sk.strokes.size() - 1);
    }
    g_sk.sel = fresh;
}

void SkPickColor(D2D1_COLOR_F c, bool custom) {
    c.a = 1;
    if (!g_sk.sel.empty()) {
        SkPushUndo();
        for (int i : g_sk.sel) { g_sk.strokes[i].color = c; g_sk.strokes[i].Free(); }
    }
    if (!SkInk()) g_sk.tool = tinta::PEN;
    g_sk.color[g_sk.tool] = c;
    if (custom) g_sk.custom = c;
}

bool SkOverRuler(D2D1_POINT_2F q, bool body) {
    auto& R = g_sk.ruler;
    if (!R.on) return false;
    float ux = std::cos(R.angle), uy = std::sin(R.angle);
    float dx = q.x - R.cx, dy = q.y - R.cy, t = dx * ux + dy * uy, d = -dx * uy + dy * ux;
    return std::fabs(t) <= SK_RULER_HL && std::fabs(d) <= (body ? SK_RULER_HW - 14 : SK_RULER_HW);
}

void SkMenuAction(int id) {
    g_sk.pop = -1;
    switch (id) {
    case 1: SkZoomToFit(); break;
    case 2: g_sk.ruler.on = !g_sk.ruler.on; break;
    case 3: g_sk.theme = SkDark() && g_sk.paper != 2 ? 0 : 1; break;
    case 4: SkPushUndo(); g_sk.photo = {}; g_sk.photoSel = g_sk.crop = false; SkPhotoReload(); SkChanged(); break;
    case 5: if (!g_sk.strokes.empty()) { SkPushUndo(); g_sk.strokes.clear(); g_sk.sel.clear(); } break;
    case 6: SkDiscard(); break;
    case 10: SketchInsert(); break;
    case 11: SkSaveToPictures(); break;
    case 12: SkCopyImage(); break;
    }
}

// clique na interface (pílulas, paleta, balões, menu da seleção); true: tratou
bool SkClickUi(D2D1_POINT_2F p) {
    if (OverLights(SkLightsPt(p))) {  // vermelho põe na nota e fecha; amarelo minimiza; verde amplia
        for (int k = 0; k < 3; k++) {
            auto c = LightCenter(k);
            if (std::fabs(p.x - c.x) <= 8 && std::fabs(p.y - g_sk.top - c.y) <= 8) {
                if (k == 0) SketchInsert();
                else if (k == 1) ShowWindow(g_hwnd, SW_MINIMIZE);
                else ampliacao::Alternar(g_hwnd);
            }
        }
        return true;
    }
    if (g_sk.pop >= 0) {
        if (g_sk.popR.right > 0 && In(g_sk.popR, p)) {
            if (g_sk.pop == POP_COLORS) {
                auto& G = g_sk.gridR;
                if (In(G, p)) { int c = std::clamp((int)((p.x - G.left) / 24), 0, 11), r = std::clamp((int)((p.y - G.top) / 22), 0, 6); SkPickColor(GridColor(r, c), true); }
                else if (In(g_sk.specR, p)) { g_sk.mode = Sketch::M_SPECTRUM; SetCapture(g_hwnd); auto& S = g_sk.specR; SkPickColor(SpectrumColor((p.x - S.left) / (S.right - S.left), (p.y - S.top) / (S.bottom - S.top)), true); }
                else if (In(g_sk.bEyedrop, p)) { g_sk.eyedrop = !g_sk.eyedrop; if (g_sk.eyedrop) g_sk.pop = -1; }
            }
            else if (g_sk.pop == POP_MORE || g_sk.pop == POP_EXPORT) {
                if (g_sk.pop == POP_MORE) {
                    for (int i = 0; i < 3; i++) if (In(g_sk.paperSw[i], p)) { g_sk.paper = i; SkChanged(); }
                    for (int i = 0; i < 4; i++) if (In(g_sk.patChip[i], p)) { g_sk.pattern = i; SkChanged(); }
                }
                for (int i = 0; i < g_sk.menuN; i++) if (In(g_sk.menuR[i], p)) { SkMenuAction(g_sk.menuId[i]); break; }
            }
            else if (g_sk.pop == SK_ERASER) { for (int i = 0; i < 2; i++) if (In(g_sk.popItem[i], p)) { g_sk.eraseObject = i == 1; g_sk.pop = -1; } }
            else {
                for (int i = 0; i < 5; i++) if (In(g_sk.popItem[i], p)) { g_sk.size[g_sk.pop] = i; g_sk.pop = -1; }
                if (g_sk.pop >= 0 && g_sk.angTrack.right > 0 && p.y >= g_sk.angTrack.top - 4 && p.y <= g_sk.angTrack.bottom + 4) {
                    g_sk.mode = Sketch::M_ANGLE; SetCapture(g_hwnd);
                    auto& T = g_sk.angTrack;
                    g_sk.nib = std::round(std::clamp(180.f * (p.x - T.left - 13) / (T.right - T.left - 26), 0.f, 180.f));
                }
                else if (g_sk.pop >= 0 && p.y >= g_sk.opTrack.top - 4 && p.y <= g_sk.opTrack.bottom + 4) {
                    g_sk.mode = Sketch::M_OPACITY; SetCapture(g_hwnd);
                    auto& T = g_sk.opTrack;
                    g_sk.opacity[g_sk.pop] = std::clamp(0.1f + 0.9f * (p.x - T.left - 13) / (T.right - T.left - 26), 0.1f, 1.f);
                }
            }
            return true;
        }
        int was = g_sk.pop;
        g_sk.pop = -1;
        if (was == POP_MORE && In(g_sk.bMore, p)) return true;  // tocar de novo no botão fecha
        if (was == POP_EXPORT && In(g_sk.bExport, p)) return true;
        if (was == POP_COLORS && In(g_sk.sw[5], p)) return true;
        if (!In(g_sk.palette, p) && !In(g_sk.pillC, p) && !In(g_sk.pillR, p)) return true;  // tocar fora só fecha o balão
    }
    if (In(g_sk.pillC, p)) {
        if (In(g_sk.bPen, p)) { if (g_sk.hand) { g_sk.hand = false; g_sk.paletteOn = true; } else g_sk.paletteOn = !g_sk.paletteOn; }
        else if (In(g_sk.bLasso, p)) { g_sk.tool = SK_LASSO; g_sk.hand = false; g_sk.paletteOn = true; }
        else if (In(g_sk.bHand, p)) g_sk.hand = !g_sk.hand;
        else if (In(g_sk.bZoom, p)) { auto& C = g_sk.canvas; SkZoomAt({ (C.left + C.right) / 2, (C.top + C.bottom) / 2 }, 1); }
        return true;
    }
    if (In(g_sk.pillR, p)) {
        if (In(g_sk.bPhoto, p)) SkPickPhoto();
        else if (In(g_sk.bExport, p)) g_sk.pop = POP_EXPORT;
        else if (In(g_sk.bMore, p)) g_sk.pop = POP_MORE;
        return true;
    }
    if (g_sk.cropHint.right > 0 && In(g_sk.cropHint, p)) { if (In(g_sk.bCropCancel, p)) g_sk.crop = false; return true; }
    if (g_sk.photoMenu.right > 0 && In(g_sk.photoMenu, p)) {
        if (In(g_sk.bCrop, p)) { g_sk.crop = true; g_sk.hand = false; }
        else if (In(g_sk.bRot90, p)) { SkPushUndo(); g_sk.photo.rot += 3.14159265f / 2; SkChanged(); }
        else if (g_sk.bUncrop.right > 0 && In(g_sk.bUncrop, p)) { SkPushUndo(); g_sk.photo.clip.clear(); SkChanged(); }
        else if (In(g_sk.bPhotoDel, p)) { SkPushUndo(); g_sk.photo = {}; g_sk.photoSel = false; SkPhotoReload(); SkChanged(); }
        return true;
    }
    if (g_sk.selMenu.right > 0 && In(g_sk.selMenu, p)) {
        if (In(g_sk.bSelDel, p)) SkDeleteSel();
        else if (In(g_sk.bSelDup, p)) SkDuplicateSel();
        return true;
    }
    if (!g_sk.paletteOn || !In(g_sk.palette, p)) return false;
    if (In(g_sk.bUndo, p)) { SkUndo(); return true; }
    if (In(g_sk.bRedo, p)) { SkRedo(); return true; }
    for (int k = 0; k < SK_NSLOTS; k++)
        if (In(g_sk.inst[k], p)) {
            int id = SK_SLOTS[k];
            g_sk.eyedrop = false;
            if (id == SK_RULER_SLOT) { g_sk.ruler.on = !g_sk.ruler.on; return true; }
            if (id == g_sk.tool && id != SK_LASSO && !g_sk.hand) g_sk.pop = id;  // tocar de novo no ativo: tamanhos / modo
            else { g_sk.tool = id; g_sk.hand = false; if (id != SK_LASSO) g_sk.sel.clear(); }
            return true;
        }
    for (int i = 0; i < 6; i++)
        if (In(g_sk.sw[i], p)) { if (i < 5) SkPickColor(SK_COLORS[i], false); else g_sk.pop = POP_COLORS; return true; }
    return true;  // vidro da paleta: não desenha por baixo
}

// cursores do papel, no jeito do Mac (o ponteiro sobre o papel no Freeform/Notas e o "fantasma" da Apple Pencil):
//  DOT   ponto cheio na cor da tinta com aro branco e um fio escuro — ferramentas finas (caneta, lápis, pena)
//  RING  só o contorno, fino e nítido, do tamanho da ponta, com um pontinho da cor no meio — ferramentas largas
//  CHISEL o contorno da ponta chanfrada inclinada do marcador, com um véu bem leve da cor
//  DASH  anel tracejado azul pequeno — laço
// O tamanho segue a ferramenta e o zoom (em pixels de tela).
enum { CUR_DOT, CUR_RING, CUR_CHISEL, CUR_DASH };
void SkCursorPixels(int kind, D2D1_COLOR_F c, float px, int N, UINT32* pxl, float ang = -84);
inline int SkCursorSize(float px) { return std::max(32, ((int)std::ceil(std::clamp(px, 4.f * g_scale, 140.f) + 8 * g_scale) + 1) & ~1); }
HCURSOR SkToolCursor(int kind, D2D1_COLOR_F c, float px, float ang = -84) {
    static std::vector<std::pair<std::wstring, HCURSOR>> cache;
    float sc = g_scale;
    px = std::clamp(px, 4.f * sc, 140.f);
    wchar_t key[96]; swprintf_s(key, L"%d|%.3f|%.3f|%.3f|%.0f|%.2f|%.0f", kind, c.r, c.g, c.b, std::round(px), sc, ang);
    for (auto& e : cache) if (e.first == key) return e.second;
    if (cache.size() > 48) { for (auto& e : cache) DestroyCursor(e.second); cache.clear(); }
    int N = SkCursorSize(px);
    BITMAPV5HEADER bi{}; bi.bV5Size = sizeof(bi); bi.bV5Width = N; bi.bV5Height = -N; bi.bV5Planes = 1; bi.bV5BitCount = 32; bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000; bi.bV5GreenMask = 0x0000FF00; bi.bV5BlueMask = 0x000000FF; bi.bV5AlphaMask = 0xFF000000;
    void* bits = nullptr;
    HDC sdc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(sdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, sdc);
    if (!color || !bits) return LoadCursorW(nullptr, IDC_ARROW);
    SkCursorPixels(kind, c, px, N, (UINT32*)bits, ang);
    HBITMAP mask = CreateBitmap(N, N, 1, 1, nullptr);
    ICONINFO ii{ FALSE, (DWORD)(N / 2), (DWORD)(N / 2), mask, color };
    HCURSOR h = CreateIconIndirect(&ii);
    DeleteObject(mask); DeleteObject(color);
    if (!h) return LoadCursorW(nullptr, IDC_ARROW);
    cache.push_back({ key, h });
    return h;
}

void SkCursorPixels(int kind, D2D1_COLOR_F c, float px, int N, UINT32* pxl, float ang) {
    float sc = g_scale;
    px = std::clamp(px, 4.f * sc, 140.f);
    const float cx = N / 2.f, cy = N / 2.f, A = ang * 3.14159265f / 180, ux = std::cos(A), uy = std::sin(A);
    const float line = 1.0f * sc, halo = 1.25f * sc;  // fio escuro e o aro branco por fora (aparece em qualquer papel)
    auto band = [](float d, float a, float b) {  // cobertura de a <= d <= b, com 1 px de suavização
        return std::clamp(std::min(d - a, b - d) + 0.5f, 0.f, 1.f);
    };
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            float r = 0, g = 0, bl = 0, al = 0;
            auto over = [&](float cr, float cg, float cb, float ca) { if (ca <= 0) return; r = r * (1 - ca) + cr * ca; g = g * (1 - ca) + cg * ca; bl = bl * (1 - ca) + cb * ca; al = al * (1 - ca) + ca; };
            if (kind == CUR_DOT) {
                float d = std::hypot(dx, dy), R = std::max(2.f * sc, px / 2);
                over(c.r, c.g, c.b, std::clamp(R - d + 0.5f, 0.f, 1.f));  // só a tinta, sem contorno
            } else if (kind == CUR_RING || kind == CUR_DASH) {
                float d = std::hypot(dx, dy), R = px / 2;
                float ring = band(d, R - line / 2, R + line / 2);
                if (kind == CUR_DASH) { float ang = std::atan2(dy, dx); if (std::fmod(ang + 6.2832f, 0.7854f) > 0.45f) ring = 0; }
                over(1, 1, 1, 0.9f * band(d, R - line / 2 - halo, R + line / 2 + halo));
                if (kind == CUR_DASH) over(SK_BLUE.r, SK_BLUE.g, SK_BLUE.b, ring);
                else over(0.11f, 0.11f, 0.12f, 0.85f * ring);
                // o pontinho da cor no meio
                float dot = std::clamp(1.6f * sc - d + 0.5f, 0.f, 1.f);
                if (kind == CUR_RING) { over(1, 1, 1, std::clamp(2.6f * sc - d + 0.5f, 0.f, 1.f)); over(c.r, c.g, c.b, dot); }
            } else {  // CHISEL: retângulo girado de cantos arredondados
                float hw = px / 2, ht = 0.75f * sc, rad = ht;  // um traço bem fino, quase em pé
                float qx = std::fabs(dx * ux + dy * uy) - (hw - rad), qy = std::fabs(-dx * uy + dy * ux) - (ht - rad);
                float d = std::hypot(std::max(qx, 0.f), std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - rad;  // < 0 dentro
                // ponta chanfrada cheia na cor (como o ponto da caneta), aro branco e um fio escuro por fora
                over(c.r, c.g, c.b, std::clamp(-d + 0.5f, 0.f, 1.f));  // só a tinta, sem contorno
            }
            // pré-multiplicado (cursor de 32 bits)
            pxl[y * N + x] = ((UINT32)(al * 255 + 0.5f) << 24) | ((UINT32)(r * al * 255 + 0.5f) << 16) | ((UINT32)(g * al * 255 + 0.5f) << 8) | (UINT32)(bl * al * 255 + 0.5f);
        }
}

// o cursor da ferramenta ativa: ponto para as finas, contorno do tamanho da ponta para as largas
HCURSOR SkInkCursor() {
    int t = g_sk.tool;
    D2D1_COLOR_F c = g_sk.color[t];
    if (g_sk.paper == 2) { tinta::DarkPaper() = true; c = tinta::Shown(c); }
    float size = tinta::Sizes((tinta::Tool)t)[g_sk.size[t]] * g_sk.vz * g_scale;
    switch (t) {
    case tinta::MARKER: return SkToolCursor(CUR_CHISEL, c, size);
    case tinta::FOUNTAIN: return SkToolCursor(CUR_DOT, c, size * 0.6f);
    case tinta::BRUSHPEN: return SkToolCursor(CUR_DOT, c, size * 0.55f);
    case tinta::REEDPEN: return SkToolCursor(CUR_CHISEL, c, size, -g_sk.nib);
    case tinta::PEN: case tinta::PENCIL: return SkToolCursor(CUR_DOT, c, size);
    default: return SkToolCursor(CUR_DOT, c, size);  // giz, aquarela
    }
}

// o mouse dá poucos WM_MOUSEMOVE por quadro; o Windows guarda os pontos do meio (até 64): traço mais redondo
void SkMouseHistory(D2D1_POINT_2F p, float pr) {
    POINT sp{ (LONG)std::lround(p.x * g_scale), (LONG)std::lround(p.y * g_scale) };
    ClientToScreen(g_hwnd, &sp);
    MOUSEMOVEPOINT in{}, out[64];
    in.x = sp.x & 0xFFFF; in.y = sp.y & 0xFFFF; in.time = GetMessageTime();
    int n = GetMouseMovePointsEx(sizeof(in), &in, out, 64, GMMP_USE_DISPLAY_POINTS);
    DWORD last = g_sk.lastMoveTime;
    g_sk.lastMoveTime = in.time;
    if (n <= 1) return;
    std::vector<D2D1_POINT_2F> pts;
    for (int i = 1; i < n; i++) {  // out[0] é o atual; os seguintes são os mais velhos
        if (out[i].time <= last) break;
        int x = out[i].x > 32767 ? out[i].x - 65536 : out[i].x, y = out[i].y > 32767 ? out[i].y - 65536 : out[i].y;
        POINT c{ x, y }; ScreenToClient(g_hwnd, &c);
        pts.push_back({ c.x / g_scale, c.y / g_scale });
    }
    for (size_t i = pts.size(); i-- > 0;) SkAddInk(SkWorld(pts[i]), pr);
}

// pressure < 0: mouse/touchpad (sem pressão); 0..1: caneta
bool SketchMouse(UINT m, D2D1_POINT_2F p, float pressure = -1) {  // true: a folha de desenho tratou
    if (!g_sk.on || g_sheet.on) return false;
    SketchLayout();
    auto& C = g_sk.canvas;
    D2D1_POINT_2F q = SkWorld(p);
    float pr = pressure < 0 ? 0.5f : std::clamp(pressure, 0.f, 1.f);
    if (m == WM_LBUTTONDOWN) {
        if (SkClickUi(p)) { Invalidate(); return true; }
        if (!In(C, p)) { Invalidate(); return true; }
        SetCapture(g_hwnd);
        g_sk.last = q; g_sk.lastScreen = p;
        if (g_sk.eyedrop) {
            for (int i = (int)g_sk.strokes.size() - 1; i >= 0; i--)
                if (tinta::Hit(g_sk.strokes[i], q.x, q.y, 4 / g_sk.vz)) { SkPickColor(g_sk.strokes[i].color, true); break; }
            g_sk.eyedrop = false;
            Invalidate();
            return true;
        }
        g_sk.downScreen = p; g_sk.moved = false; g_sk.downTick = GetTickCount();
        if (g_sk.crop && g_sk.photo.on) { g_sk.mode = Sketch::M_CROP; g_sk.cropPts = { q }; Invalidate(); return true; }  // a volta do recorte
        if (g_sk.photo.on && g_sk.photoSel) {  // alças da foto: girar, tamanho (cantos), mover (por dentro)
            float hr = 12 / g_sk.vz;
            auto rh = PhRotHandle();
            if (std::hypot(q.x - rh.x, q.y - rh.y) <= hr) { SkPushUndo(); g_sk.mode = Sketch::M_PHOTO_ROT; Invalidate(); return true; }
            for (int i = 0; i < 4; i++) {
                auto cn = PhCorner(i);
                if (std::hypot(q.x - cn.x, q.y - cn.y) <= hr) {
                    SkPushUndo(); g_sk.mode = Sketch::M_PHOTO_SCALE;
                    g_sk.gestC = PhC(); g_sk.gestA = std::max(1.f, std::hypot(q.x - g_sk.gestC.x, q.y - g_sk.gestC.y)); g_sk.gestW = g_sk.photo.w; g_sk.gestH = g_sk.photo.h;
                    Invalidate(); return true;
                }
            }
            if (PhHit(q)) { SkPushUndo(); g_sk.mode = Sketch::M_PHOTO_MOVE; Invalidate(); return true; }
            g_sk.photoSel = false;  // tocou fora: larga a foto
        }
        if (SkPanning() && !g_sk.tempEraser) { g_sk.mode = Sketch::M_PAN; Invalidate(); return true; }
        if (!g_sk.sel.empty()) {
            auto b = SkSelBox();
            float pad = 6 / g_sk.vz, hr = 12 / g_sk.vz;
            D2D1_POINT_2F c{ (b.x1 + b.x2) / 2, (b.y1 + b.y2) / 2 };
            auto start = [&](Sketch::Mode m) {
                SkPushUndo(); g_sk.mode = m; g_sk.gestC = c;
                g_sk.gestA = m == Sketch::M_SEL_ROT ? std::atan2(q.y - c.y, q.x - c.x) : std::max(1.f, std::hypot(q.x - c.x, q.y - c.y));
                g_sk.selOrig.clear(); for (int i : g_sk.sel) g_sk.selOrig.push_back(g_sk.strokes[i]);
                Invalidate();
            };
            auto rh = SelRotHandle();
            if (std::hypot(q.x - rh.x, q.y - rh.y) <= hr) { start(Sketch::M_SEL_ROT); return true; }
            for (int i = 0; i < 4; i++) { auto cn = SelCorner(i); if (std::hypot(q.x - cn.x, q.y - cn.y) <= hr) { start(Sketch::M_SEL_SCALE); return true; } }
            if (q.x >= b.x1 - pad && q.x <= b.x2 + pad && q.y >= b.y1 - pad && q.y <= b.y2 + pad) { SkPushUndo(); g_sk.mode = Sketch::M_MOVE; Invalidate(); return true; }
            g_sk.sel.clear();
        }
        if (SkOverRuler(q, true)) { g_sk.mode = Sketch::M_RULER; Invalidate(); return true; }
        int t = SkTool();
        if (t == SK_ERASER) { SkPushUndo(); g_sk.changed = false; g_sk.mode = Sketch::M_ERASE; SkEraseAt(q); }
        else if (t == SK_LASSO) { g_sk.mode = Sketch::M_LASSO; g_sk.lasso = { q }; }
        else {
            SkPushUndo();
            auto s = g_sk.Make(pressure < 0);
            g_sk.strokes.push_back(std::move(s));
            g_sk.mode = Sketch::M_INK; g_sk.drawing = true; g_sk.straight = g_sk.shaped = false;
            g_sk.lastMoveTime = GetMessageTime();
            SkRulerSnap(q, g_sk.strokes.back().size);
            SkAddInk(q, pr);
            g_sk.holdAt = q;
            SkArmHold();
        }
    } else if (m == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, g_hwnd, 0 }; TrackMouseEvent(&tme);
        g_sk.hover = p;
        bool hl = OverLights(SkLightsPt(p));
        ampliacao::Verde(g_hwnd, hl && p.x >= 58 && p.x < 81);
        g_hoverLights = hl;
        int hi = -1;
        if (g_sk.mode == Sketch::M_NONE && g_sk.paletteOn && In(g_sk.palette, p)) for (int k = 0; k < SK_NSLOTS; k++) if (In(g_sk.inst[k], p)) hi = k;
        g_sk.hoverInst = hi;
        switch (g_sk.mode) {
        case Sketch::M_INK:
            if (g_sk.shaped || g_sk.strokes.empty()) break;
            if (g_sk.straight) { auto& s = g_sk.strokes.back(); s.pts = tinta::Line(s.pts[0].x, s.pts[0].y, q.x, q.y, pr); }
            else { if (pressure < 0) SkMouseHistory(p, pr); SkAddInk(q, pr); }
            if (std::hypot(q.x - g_sk.holdAt.x, q.y - g_sk.holdAt.y) > 3 / g_sk.vz) { g_sk.holdAt = q; SkArmHold(); }
            break;
        case Sketch::M_ERASE: SkEraseAt(q); break;
        case Sketch::M_LASSO: g_sk.lasso.push_back(q); break;
        case Sketch::M_SEL_SCALE:
        case Sketch::M_SEL_ROT: {  // tamanho (cantos, sem deformar) ou giro (alça de cima) em volta do meio da seleção
            auto c = g_sk.gestC;
            float k = 1, a = 0;
            if (g_sk.mode == Sketch::M_SEL_SCALE) k = std::max(0.1f, std::hypot(q.x - c.x, q.y - c.y) / g_sk.gestA);
            else {
                a = std::atan2(q.y - c.y, q.x - c.x) - g_sk.gestA;
                float k90 = std::round(a / (3.14159265f / 2)) * (3.14159265f / 2);
                if (std::fabs(a - k90) < 3 * 3.14159265f / 180) a = k90;
            }
            float sn = std::sin(a), co = std::cos(a);
            for (size_t j = 0; j < g_sk.sel.size() && j < g_sk.selOrig.size(); j++) {
                tinta::Stroke s2 = g_sk.selOrig[j];
                for (auto& pt : s2.pts) {
                    float dx = (pt.x - c.x) * k, dy = (pt.y - c.y) * k;
                    pt.x = c.x + dx * co - dy * sn; pt.y = c.y + dx * sn + dy * co;
                }
                s2.size = g_sk.selOrig[j].size * k;
                g_sk.strokes[g_sk.sel[j]] = std::move(s2);
            }
            SkChanged();
            break;
        }
        case Sketch::M_MOVE: {
            float dx = q.x - g_sk.last.x, dy = q.y - g_sk.last.y;
            for (int i : g_sk.sel) { for (auto& pt : g_sk.strokes[i].pts) { pt.x += dx; pt.y += dy; } g_sk.strokes[i].Free(); }
            g_sk.last = q;
            SkChanged();
            break;
        }
        case Sketch::M_RULER:
            if (GetKeyState(VK_SHIFT) < 0) {  // Shift: gira a régua em volta do meio dela
                float a0 = std::atan2(g_sk.last.y - g_sk.ruler.cy, g_sk.last.x - g_sk.ruler.cx), a1 = std::atan2(q.y - g_sk.ruler.cy, q.x - g_sk.ruler.cx);
                g_sk.ruler.angle += a1 - a0;
            } else { g_sk.ruler.cx += q.x - g_sk.last.x; g_sk.ruler.cy += q.y - g_sk.last.y; }
            g_sk.last = q;
            break;
        case Sketch::M_PHOTO_MOVE:
            g_sk.photo.x += q.x - g_sk.last.x; g_sk.photo.y += q.y - g_sk.last.y;
            g_sk.last = q; SkChanged();
            break;
        case Sketch::M_PHOTO_SCALE: {  // pelos cantos: o tamanho muda em volta do meio, sem deformar
            float k = std::max(20.f / std::min(g_sk.gestW, g_sk.gestH), std::hypot(q.x - g_sk.gestC.x, q.y - g_sk.gestC.y) / g_sk.gestA);
            g_sk.photo.w = g_sk.gestW * k; g_sk.photo.h = g_sk.gestH * k;
            g_sk.photo.x = g_sk.gestC.x - g_sk.photo.w / 2; g_sk.photo.y = g_sk.gestC.y - g_sk.photo.h / 2;
            SkChanged();
            break;
        }
        case Sketch::M_PHOTO_ROT: {  // a alça fica em cima: o ângulo do arrasto em volta do meio; encaixa a cada 90°
            auto c = PhC();
            float a = std::atan2(q.y - c.y, q.x - c.x) + 3.14159265f / 2;
            float k90 = std::round(a / (3.14159265f / 2)) * (3.14159265f / 2);
            if (std::fabs(a - k90) < 3 * 3.14159265f / 180) a = k90;
            g_sk.photo.rot = a; SkChanged();
            break;
        }
        case Sketch::M_CROP: g_sk.cropPts.push_back(q); break;
        case Sketch::M_PAN:
            g_sk.vx += p.x - g_sk.lastScreen.x; g_sk.vy += p.y - g_sk.lastScreen.y;
            g_sk.lastScreen = p;
            break;
        case Sketch::M_SPECTRUM: {
            auto& S = g_sk.specR;
            SkPickColor(SpectrumColor((p.x - S.left) / (S.right - S.left), (p.y - S.top) / (S.bottom - S.top)), true);
            break;
        }
        case Sketch::M_ANGLE: {
            auto& T = g_sk.angTrack;
            g_sk.nib = std::round(std::clamp(180.f * (p.x - T.left - 13) / (T.right - T.left - 26), 0.f, 180.f));
            break;
        }
        case Sketch::M_OPACITY: {
            auto& T = g_sk.opTrack;
            if (g_sk.pop >= 0 && g_sk.pop < tinta::TOOL_COUNT) g_sk.opacity[g_sk.pop] = std::clamp(0.1f + 0.9f * (p.x - T.left - 13) / (T.right - T.left - 26), 0.1f, 1.f);
            break;
        }
        default: break;
        }
        bool overUi = OverLights(SkLightsPt(p)) || In(g_sk.pillC, p) || In(g_sk.pillR, p) || (g_sk.paletteOn && In(g_sk.palette, p)) || (g_sk.pop >= 0 && In(g_sk.popR, p)) || (g_sk.selMenu.right > 0 && In(g_sk.selMenu, p));
        // cursor: seta na interface; no papel, o pontinho da tinta (a borracha já desenha o círculo dela)
        HCURSOR cur = LoadCursorW(nullptr, IDC_ARROW);
        if (g_sk.mode == Sketch::M_MOVE || g_sk.mode == Sketch::M_RULER || g_sk.mode == Sketch::M_PAN || g_sk.mode == Sketch::M_PHOTO_MOVE || g_sk.mode == Sketch::M_PHOTO_SCALE) cur = LoadCursorW(nullptr, IDC_SIZEALL);
        else if (!overUi && In(C, p)) {
            bool onHandle = false;
            if (g_sk.photo.on && g_sk.photoSel && !g_sk.crop) {
                float hr = 12 / g_sk.vz; auto rh = PhRotHandle();
                if (std::hypot(q.x - rh.x, q.y - rh.y) <= hr) { cur = LoadCursorW(nullptr, IDC_HAND); onHandle = true; }
                for (int i = 0; i < 4 && !onHandle; i++) { auto cn = PhCorner(i); if (std::hypot(q.x - cn.x, q.y - cn.y) <= hr) { cur = LoadCursorW(nullptr, IDC_SIZEALL); onHandle = true; } }
                if (!onHandle && PhHit(q)) { cur = LoadCursorW(nullptr, IDC_SIZEALL); onHandle = true; }
            }
            if (onHandle) {}
            else if (g_sk.crop) cur = SkToolCursor(CUR_DASH, SK_BLUE, 14 * g_scale);
            else if (g_sk.eyedrop) cur = SkToolCursor(CUR_RING, D2D1::ColorF(0x1c1c1f), 18 * g_scale);
            else if (SkPanning()) cur = LoadCursorW(nullptr, IDC_HAND);
            else if (SkOverRuler(q, true)) cur = LoadCursorW(nullptr, IDC_SIZEALL);
            else if (!g_sk.sel.empty() && [&] { auto b = SkSelBox(); float pad = 6 / g_sk.vz; return q.x >= b.x1 - pad && q.x <= b.x2 + pad && q.y >= b.y1 - pad && q.y <= b.y2 + pad; }()) cur = LoadCursorW(nullptr, IDC_SIZEALL);
            else if (SkTool() == SK_ERASER) cur = g_sk.eraseObject ? SkToolCursor(CUR_RING, D2D1::ColorF(0x8e8e93), 14 * g_scale) : nullptr;
            else if (SkTool() == SK_LASSO) cur = SkToolCursor(CUR_DOT, SK_BLUE, 5 * g_scale);  // um ponto
            else cur = SkInkCursor();
        }
        SetCursor(cur);
    } else if (m == WM_LBUTTONUP) {
        if (g_hwnd) KillTimer(g_hwnd, T_HOLD);
        switch (g_sk.mode) {
        case Sketch::M_INK: {
            g_sk.drawing = false;
            // um toque rápido em cima da foto (sem arrastar) escolhe a foto em vez de deixar um ponto
            bool tap = std::hypot(p.x - g_sk.downScreen.x, p.y - g_sk.downScreen.y) < 5 && GetTickCount() - g_sk.downTick < 400;
            D2D1_POINT_2F at = !g_sk.strokes.empty() && !g_sk.strokes.back().pts.empty() ? D2D1::Point2F(g_sk.strokes.back().pts[0].x, g_sk.strokes.back().pts[0].y) : q;
            if (tap && !g_sk.straight && !g_sk.shaped && PhHit(at)) {
                if (!g_sk.strokes.empty()) g_sk.strokes.pop_back();
                if (!g_sk.undo.empty()) g_sk.undo.pop_back();
                g_sk.photoSel = true;
            }
            else if (!g_sk.strokes.empty() && g_sk.strokes.back().pts.empty()) { g_sk.strokes.pop_back(); if (!g_sk.undo.empty()) g_sk.undo.pop_back(); }
            SkChanged();
            break;
        }
        case Sketch::M_ERASE:
            if (!g_sk.changed && !g_sk.undo.empty()) g_sk.undo.pop_back();
            if (!g_sk.changed && std::hypot(p.x - g_sk.downScreen.x, p.y - g_sk.downScreen.y) < 5 && PhHit(q)) g_sk.photoSel = true;
            break;
        case Sketch::M_LASSO: {
            // um toque (sem arrastar) em cima da foto escolhe a foto
            bool tap = std::hypot(p.x - g_sk.downScreen.x, p.y - g_sk.downScreen.y) < 6;
            D2D1_POINT_2F start = g_sk.lasso.empty() ? q : g_sk.lasso[0];
            SkFinishLasso();
            if (tap && g_sk.sel.empty() && PhHit(start)) g_sk.photoSel = true;
            break;
        }
        case Sketch::M_PAN:
            if (std::hypot(p.x - g_sk.downScreen.x, p.y - g_sk.downScreen.y) < 6 && PhHit(q)) g_sk.photoSel = true;  // mão: tocar escolhe a foto
            break;
        case Sketch::M_CROP: {  // a volta vira o recorte (em coordenadas da foto, 0..1)
            auto& P = g_sk.photo;
            if (g_sk.cropPts.size() >= 8 && P.w > 0 && P.h > 0) {
                SkPushUndo();
                P.clip.clear();
                for (auto& w2 : g_sk.cropPts) { auto l = PhToLocal(w2); P.clip.push_back({ (l.x + P.w / 2) / P.w, (l.y + P.h / 2) / P.h }); }
                SkChanged();
            }
            g_sk.cropPts.clear(); g_sk.crop = false;
            break;
        }
        default: break;
        }
        g_sk.mode = Sketch::M_NONE;
        g_sk.snap.on = false;
        g_sk.tempEraser = false;
        if (GetCapture() == g_hwnd) ReleaseCapture();
    }
    Invalidate();
    return true;
}

// roda: em cima da régua gira; Ctrl (ou a pinça do touchpad) aproxima; senão rola o papel (Shift: para o lado)
bool SketchWheel(D2D1_POINT_2F p, float notches) {
    if (!g_sk.on || g_sheet.on) return false;
    SketchLayout();
    D2D1_POINT_2F q = SkWorld(p);
    if (GetKeyState(VK_CONTROL) < 0) SkZoomAt(p, g_sk.vz * std::pow(1.12f, notches));
    else if (SkOverRuler(q, false)) {
        float step = (GetKeyState(VK_SHIFT) < 0 ? 1.f : 5.f) * 3.14159265f / 180;
        float a = g_sk.ruler.angle - notches * step;
        float k45 = std::round(a / (3.14159265f / 4)) * (3.14159265f / 4);
        if (std::fabs(a - k45) < 0.6f * 3.14159265f / 180) a = k45;
        g_sk.ruler.angle = a;
    }
    else if (GetKeyState(VK_SHIFT) < 0) g_sk.vx += notches * 60;
    else g_sk.vy += notches * 60;
    Invalidate();
    return true;
}

// teclas da folha: Esc fecha (balão, conta-gotas, seleção; por fim põe na nota), Ctrl+Z/Y, Delete, Ctrl+D, Ctrl+A,
// 1-6 instrumentos, E borracha, L laço, R régua, H mão, Ctrl+0 100 %, Ctrl+1 ajustar, Ctrl+= / Ctrl+- zoom, setas rolam
bool SketchKey(WPARAM w, bool ctrl, bool shift) {
    if (!g_sk.on || g_sheet.on) return false;
    auto& C = g_sk.canvas;
    D2D1_POINT_2F mid{ (C.left + C.right) / 2, (C.top + C.bottom) / 2 };
    if (w == VK_ESCAPE) {
        if (g_sk.crop) g_sk.crop = false;
        else if (g_sk.photoSel) g_sk.photoSel = false;
        else if (g_sk.pop >= 0) g_sk.pop = -1;
        else if (g_sk.eyedrop) g_sk.eyedrop = false;
        else if (!g_sk.sel.empty()) g_sk.sel.clear();
        else SketchInsert();
    }
    else if (ctrl && w == 'Z') { if (shift) SkRedo(); else SkUndo(); }
    else if (ctrl && w == 'Y') SkRedo();
    else if (ctrl && w == 'D') SkDuplicateSel();
    else if (ctrl && w == 'A') { g_sk.sel.clear(); for (int i = 0; i < (int)g_sk.strokes.size(); i++) g_sk.sel.push_back(i); g_sk.tool = SK_LASSO; g_sk.hand = false; }
    else if (ctrl && w == '0') SkZoomAt(mid, 1);
    else if (ctrl && w == '1') SkZoomToFit();
    else if (ctrl && (w == VK_OEM_PLUS || w == VK_ADD)) SkZoomAt(mid, g_sk.vz * 1.25f);
    else if (ctrl && (w == VK_OEM_MINUS || w == VK_SUBTRACT)) SkZoomAt(mid, g_sk.vz / 1.25f);
    else if ((w == VK_DELETE || w == VK_BACK) && g_sk.photoSel && g_sk.sel.empty()) { SkPushUndo(); g_sk.photo = {}; g_sk.photoSel = false; SkPhotoReload(); SkChanged(); }
    else if (w == VK_DELETE || w == VK_BACK) SkDeleteSel();
    else if (!ctrl && w >= '1' && w <= '5') { g_sk.tool = SK_SLOTS[w - '1']; g_sk.hand = false; g_sk.paletteOn = true; g_sk.sel.clear(); }
    else if (!ctrl && w == 'E') { g_sk.tool = SK_ERASER; g_sk.hand = false; }
    else if (!ctrl && w == 'L') { g_sk.tool = SK_LASSO; g_sk.hand = false; }
    else if (!ctrl && w == 'R') g_sk.ruler.on = !g_sk.ruler.on;
    else if (!ctrl && w == 'H') g_sk.hand = !g_sk.hand;
    else if (w == VK_LEFT || w == VK_RIGHT || w == VK_UP || w == VK_DOWN) {
        float d = shift ? 320.f : 80.f;
        if (w == VK_LEFT) g_sk.vx += d; else if (w == VK_RIGHT) g_sk.vx -= d; else if (w == VK_UP) g_sk.vy += d; else g_sk.vy -= d;
    }
    Invalidate();
    return true;
}

// toque (tela touch): o dedo nunca desenha. Com a caneta perto da tela (ou usada há pouco), todo toque é ignorado —
// é a palma da mão apoiada. Sem a caneta: um dedo arrasta o papel, dois dedos aproximam/afastam; tocar nos botões
// (paleta, pílulas, balões) funciona como um clique.
bool SkTouch(UINT m, UINT32 id, D2D1_POINT_2F p) {
    auto& T = g_sk.touch;
    bool penNear = g_sk.penIn || GetTickCount() - g_sk.penTick < 700;
    auto find = [&](UINT32 i) { for (int k = 0; k < T.n; k++) if (T.id[k] == i) return k; return -1; };
    // começo de gesto: o que o(s) dedo(s) pegaram — régua, foto escolhida ou o papel
    auto start = [&]() {
        D2D1_POINT_2F w0 = SkWorld(T.p[0]);
        T.target = g_sk.ruler.on && SkOverRuler(w0, false) ? 1 : g_sk.photo.on && g_sk.photoSel && PhHit(w0) ? 2 : 0;
        if (T.target == 2) SkPushUndo();
        if (T.n == 2) {
            T.d0 = std::max(1.f, std::hypot(T.p[0].x - T.p[1].x, T.p[0].y - T.p[1].y)); T.z0 = g_sk.vz;
            T.a0 = std::atan2(T.p[1].y - T.p[0].y, T.p[1].x - T.p[0].x);
            T.w0 = SkWorld({ (T.p[0].x + T.p[1].x) / 2, (T.p[0].y + T.p[1].y) / 2 });
            T.r0 = T.target == 1 ? g_sk.ruler.angle : g_sk.photo.rot;
            T.c0 = T.target == 1 ? D2D1::Point2F(g_sk.ruler.cx, g_sk.ruler.cy) : PhC();
            T.pw0 = g_sk.photo.w; T.ph0 = g_sk.photo.h;
        }
    };
    if (m == WM_POINTERDOWN) {
        if (penNear || g_sk.mode == Sketch::M_INK || g_sk.mode == Sketch::M_ERASE || g_sk.mode == Sketch::M_LASSO || g_sk.mode == Sketch::M_CROP) return true;
        if (T.n == 0) {
            bool ui = OverLights(SkLightsPt(p)) || In(g_sk.pillC, p) || In(g_sk.pillR, p) || (g_sk.paletteOn && In(g_sk.palette, p)) || (g_sk.pop >= 0 && In(g_sk.popR, p)) ||
                      (g_sk.selMenu.right > 0 && In(g_sk.selMenu, p)) || (g_sk.photoMenu.right > 0 && In(g_sk.photoMenu, p)) || (g_sk.cropHint.right > 0 && In(g_sk.cropHint, p));
            if (ui || g_sk.pop >= 0) { T.ui = true; T.n = 1; T.id[0] = id; T.p[0] = p; SketchMouse(WM_LBUTTONDOWN, p); return true; }
            T.ui = false;
        }
        if (T.ui || T.n >= 2) return true;
        T.id[T.n] = id; T.p[T.n] = p; T.n++;
        start();
        return true;
    }
    int k = find(id);
    if (k < 0) return true;
    if (m == WM_POINTERUPDATE) {
        if (T.ui) { SketchMouse(WM_MOUSEMOVE, p); return true; }
        D2D1_POINT_2F old = T.p[k];
        T.p[k] = p;
        if (T.n == 1) {  // um dedo: arrasta a régua, a foto ou o papel
            D2D1_POINT_2F a = SkWorld(old), b = SkWorld(p);
            if (T.target == 1) { g_sk.ruler.cx += b.x - a.x; g_sk.ruler.cy += b.y - a.y; }
            else if (T.target == 2) { g_sk.photo.x += b.x - a.x; g_sk.photo.y += b.y - a.y; SkChanged(); }
            else { g_sk.vx += p.x - old.x; g_sk.vy += p.y - old.y; }
        } else {  // dois dedos: girar (régua, foto), aumentar (foto) ou aproximar o papel
            D2D1_POINT_2F mid{ (T.p[0].x + T.p[1].x) / 2, (T.p[0].y + T.p[1].y) / 2 };
            float d = std::hypot(T.p[0].x - T.p[1].x, T.p[0].y - T.p[1].y), a = std::atan2(T.p[1].y - T.p[0].y, T.p[1].x - T.p[0].x);
            D2D1_POINT_2F wm = SkWorld(mid);
            if (T.target == 1) {
                g_sk.ruler.angle = T.r0 + (a - T.a0);
                g_sk.ruler.cx = T.c0.x + wm.x - T.w0.x; g_sk.ruler.cy = T.c0.y + wm.y - T.w0.y;
            } else if (T.target == 2) {
                float s2 = std::max(20.f / std::min(T.pw0, T.ph0), d / T.d0);
                auto& P = g_sk.photo;
                P.rot = T.r0 + (a - T.a0); P.w = T.pw0 * s2; P.h = T.ph0 * s2;
                P.x = T.c0.x + wm.x - T.w0.x - P.w / 2; P.y = T.c0.y + wm.y - T.w0.y - P.h / 2;
                SkChanged();
            } else {
                float z = std::clamp(T.z0 * d / T.d0, 0.25f, 4.f);
                g_sk.vz = z;
                g_sk.vx = mid.x - g_sk.canvas.left - T.w0.x * z;
                g_sk.vy = mid.y - g_sk.canvas.top - T.w0.y * z;
            }
        }
        Invalidate();
        return true;
    }
    // soltou
    if (T.ui) { SketchMouse(WM_LBUTTONUP, p); T = {}; return true; }
    for (int j = k; j + 1 < T.n; j++) { T.id[j] = T.id[j + 1]; T.p[j] = T.p[j + 1]; }
    T.n--;
    if (T.n == 1 && T.target == 0) T.target = 0;
    Invalidate();
    return true;
}

// caneta (Surface Pen, Wacom...): WM_POINTER traz a pressão e todos os pontos entre um quadro e outro.
// A ponta de borracha (ou o botão lateral) apaga enquanto está encostada.
bool SketchPointer(UINT m, WPARAM w) {
    if (!g_sk.on || g_sheet.on) return false;
    UINT32 id = GET_POINTERID_WPARAM(w);
    POINTER_INPUT_TYPE type = PT_POINTER;
    if (!GetPointerType(id, &type)) return false;
    auto dip = [](POINT sp) { ScreenToClient(g_hwnd, &sp); return D2D1_POINT_2F{ sp.x / g_scale, sp.y / g_scale }; };
    if (type == PT_TOUCH) {
        if (m == WM_POINTERENTER || m == WM_POINTERLEAVE) return true;
        POINTER_INFO ti{};
        if (!GetPointerInfo(id, &ti)) return true;
        D2D1_POINT_2F tp = dip(ti.ptPixelLocation);
        if (m == WM_POINTERDOWN && g_sk.touch.n == 0 && SketchCaption(tp) && !(g_sk.penIn || GetTickCount() - g_sk.penTick < 700)) return false;  // arrastar a janela
        return SkTouch(m, id, tp);
    }
    if (type != PT_PEN) return false;
    // a caneta perto da tela: a palma da mão que encostar é ignorada
    g_sk.penTick = GetTickCount();
    if (m == WM_POINTERENTER) { g_sk.penIn = true; return true; }
    if (m == WM_POINTERLEAVE) { g_sk.penIn = false; return true; }
    if (m == WM_POINTERDOWN && g_sk.touch.n) {  // a mão chegou antes: cancela o arrasto do dedo
        if (g_sk.touch.ui) SketchMouse(WM_LBUTTONUP, g_sk.touch.p[0]);
        g_sk.touch = {};
    }
    auto press = [](const POINTER_PEN_INFO& pi) { return (pi.penMask & PEN_MASK_PRESSURE) ? pi.pressure / 1024.f : -1.f; };
    POINTER_PEN_INFO pi{};
    if (!GetPointerPenInfo(id, &pi)) return false;
    D2D1_POINT_2F at = dip(pi.pointerInfo.ptPixelLocation);
    if (m == WM_POINTERDOWN && SketchCaption(at)) return false;  // a faixa de cima continua sendo a barra da janela
    if (m == WM_POINTERUPDATE) {
        UINT32 n = 0;
        if (!GetPointerPenInfoHistory(id, &n, nullptr) || !n) return true;
        std::vector<POINTER_PEN_INFO> v(n);
        if (!GetPointerPenInfoHistory(id, &n, v.data())) return true;
        for (UINT32 i = n; i-- > 0;) SketchMouse(WM_MOUSEMOVE, dip(v[i].pointerInfo.ptPixelLocation), press(v[i]));
        return true;
    }
    if (m == WM_POINTERDOWN) g_sk.tempEraser = (pi.penFlags & (PEN_FLAG_ERASER | PEN_FLAG_INVERTED | PEN_FLAG_BARREL)) != 0;
    SketchMouse(m == WM_POINTERDOWN ? WM_LBUTTONDOWN : WM_LBUTTONUP, at, press(pi));
    return true;
}

