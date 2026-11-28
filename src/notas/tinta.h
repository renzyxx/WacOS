#pragma once
// Tinta: o motor de traço da folha de desenho do Notas (vindo do Quill).
// O contorno de cada traço é calculado a partir dos pontos (x, y, pressão) e preenchido como uma forma só,
// então a espessura acompanha a pressão da caneta (ou a velocidade, no mouse).
// A geração do contorno é uma tradução para C++ do perfect-freehand (MIT, Steve Ruiz) - ver THIRD-PARTY-NOTICES.md.
#include <windows.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#pragma comment(lib, "dxguid.lib")
#include <vector>
#include <cmath>
#include <algorithm>

namespace tinta {

// ângulo da ponta de feltro do marcador: quase em pé, como o marca-texto segurado para sublinhar texto
const float MARKER_DEG = -76;

enum Tool { PEN = 0, FOUNTAIN, MARKER, PENCIL, CRAYON, WATERCOLOR, MONOLINE, BRUSHPEN, REEDPEN, TOOL_COUNT };  // a ordem fica (vai nos .tinta)

struct Pt { float x, y, p; };

struct Stroke {
    Tool tool = PEN;
    D2D1_COLOR_F color{ 0, 0, 0, 1 };
    float size = 3, opacity = 1, nib = 40;  // nib: ângulo da pena de junco (graus a partir da horizontal)
    bool sim = true;  // sem pressão de verdade (mouse/touchpad): a pressão vem da velocidade
    std::vector<Pt> pts;
    // contorno pronto (só para traços terminados), preso à fábrica que o criou
    ID2D1PathGeometry* geo = nullptr;
    ID2D1Factory* geoFactory = nullptr;
    Stroke() = default;
    Stroke(const Stroke& o) : tool(o.tool), color(o.color), size(o.size), opacity(o.opacity), nib(o.nib), sim(o.sim), pts(o.pts) {}
    Stroke& operator=(const Stroke& o) { if (this != &o) { Free(); tool = o.tool; color = o.color; size = o.size; opacity = o.opacity; nib = o.nib; sim = o.sim; pts = o.pts; } return *this; }
    Stroke(Stroke&& o) noexcept : tool(o.tool), color(o.color), size(o.size), opacity(o.opacity), nib(o.nib), sim(o.sim), pts(std::move(o.pts)), geo(o.geo), geoFactory(o.geoFactory) { o.geo = nullptr; o.geoFactory = nullptr; }
    Stroke& operator=(Stroke&& o) noexcept { if (this != &o) { Free(); tool = o.tool; color = o.color; size = o.size; opacity = o.opacity; nib = o.nib; sim = o.sim; pts = std::move(o.pts); geo = o.geo; geoFactory = o.geoFactory; o.geo = nullptr; o.geoFactory = nullptr; } return *this; }
    ~Stroke() { Free(); }
    void Free() { if (geo) geo->Release(); geo = nullptr; geoFactory = nullptr; }
};

// tamanhos de cada ferramenta (pt), do fino ao grosso, e o padrão de cada uma (como no Quill)
inline const float* Sizes(Tool t) {
    static const float z[TOOL_COUNT][5] = { { 1.5f, 3, 5, 8, 13 }, { 3, 5, 8, 12, 18 }, { 10, 16, 24, 34, 48 },
                                            { 2, 3.5f, 5, 8, 12 }, { 6, 10, 15, 22, 30 }, { 10, 18, 28, 40, 56 }, { 1, 2, 3.5f, 6, 10 }, { 3, 6, 10, 16, 24 }, { 4, 7, 11, 16, 24 } };
    return z[t];
}
inline int DefaultSize(Tool t) { static const int d[TOOL_COUNT] = { 1, 2, 2, 1, 1, 2, 1, 2, 2 }; return d[t]; }

// ---------------------------------------------------------------------------
// perfect-freehand
namespace pf {

struct V { double x, y; };
inline V add(V a, V b) { return { a.x + b.x, a.y + b.y }; }
inline V sub(V a, V b) { return { a.x - b.x, a.y - b.y }; }
inline V mul(V a, double n) { return { a.x * n, a.y * n }; }
inline V per(V a) { return { a.y, -a.x }; }
inline V neg(V a) { return { -a.x, -a.y }; }
inline double dpr(V a, V b) { return a.x * b.x + a.y * b.y; }
inline double len(V a) { return std::hypot(a.x, a.y); }
inline V uni(V a) { double l = len(a); return { a.x / l, a.y / l }; }
inline double dist(V a, V b) { return std::hypot(a.y - b.y, a.x - b.x); }
inline double dist2(V a, V b) { double dx = a.x - b.x, dy = a.y - b.y; return dx * dx + dy * dy; }
inline V lrp(V a, V b, double t) { return add(a, mul(sub(b, a), t)); }
inline V prj(V a, V b, double c) { return add(a, mul(b, c)); }
inline V rotAround(V a, V c, double r) {
    double s = std::sin(r), co = std::cos(r), px = a.x - c.x, py = a.y - c.y;
    return { px * co - py * s + c.x, px * s + py * co + c.y };
}

const double RATE_OF_PRESSURE_CHANGE = 0.275, FIXED_PI = 3.14159265358979323846 + 0.0001;
const int START_CAP_SEGMENTS = 13, END_CAP_SEGMENTS = 29, CORNER_CAP_SEGMENTS = 13;
const double END_NOISE_THRESHOLD = 3, MIN_STREAMLINE_T = 0.15, STREAMLINE_T_RANGE = 0.85, MIN_RADIUS = 0.01;
const double DEFAULT_FIRST_PRESSURE = 0.25, DEFAULT_PRESSURE = 0.5;

struct Options {
    double size = 16, thinning = 0.5, smoothing = 0.5, streamline = 0.5;
    bool simulatePressure = true;
    double taperStart = 0, taperEnd = 0;  // 0 = sem afinar
    bool capStart = true, capEnd = true, last = false;
};

struct SP { V point; double pressure; V vector; double distance, runningLength; };

inline double StrokeRadius(double size, double thinning, double pressure) { return size * (0.5 - thinning * (0.5 - pressure)); }
inline double SimulatePressure(double prev, double distance, double size) {
    double sp = std::min(1.0, distance / size), rp = std::min(1.0, 1 - sp);
    return std::min(1.0, prev + (rp - prev) * (sp * RATE_OF_PRESSURE_CHANGE));
}

struct In { V v; double p; };  // p < 0: sem pressão

inline std::vector<SP> StrokePoints(std::vector<In> pts, const Options& o) {
    std::vector<SP> out;
    if (pts.empty()) return out;
    double t = MIN_STREAMLINE_T + (1 - o.streamline) * STREAMLINE_T_RANGE;
    if (pts.size() == 2) {
        In last = pts[1];
        pts.pop_back();
        for (int i = 1; i < 5; i++) pts.push_back({ lrp(pts[0].v, last.v, i / 4.0), -1 });
    }
    if (pts.size() == 1) pts.push_back({ add(pts[0].v, { 1, 1 }), pts[0].p });
    out.push_back({ pts[0].v, pts[0].p >= 0 ? pts[0].p : DEFAULT_FIRST_PRESSURE, { 1, 1 }, 0, 0 });
    bool reached = false;
    double running = 0;
    size_t max = pts.size() - 1;
    for (size_t i = 1; i < pts.size(); i++) {
        const SP& prev = out.back();
        V point = o.last && i == max ? pts[i].v : lrp(prev.point, pts[i].v, t);
        if (point.x == prev.point.x && point.y == prev.point.y) continue;
        double d = dist(point, prev.point);
        running += d;
        if (i < max && !reached) {
            if (running < o.size) continue;
            reached = true;
        }
        V vec = uni(sub(prev.point, point));
        out.push_back({ point, pts[i].p >= 0 ? pts[i].p : DEFAULT_PRESSURE, vec, d, running });
    }
    out[0].vector = out.size() > 1 ? out[1].vector : V{ 0, 0 };
    return out;
}

inline std::vector<V> OutlinePoints(const std::vector<SP>& points, const Options& o) {
    std::vector<V> left, right;
    if (points.empty() || o.size <= 0) return {};
    auto easeStart = [](double t) { return t * (2 - t); };
    auto easeEnd = [](double t) { t -= 1; return t * t * t + 1; };
    double total = points.back().runningLength;
    double taperStart = o.taperStart, taperEnd = o.taperEnd;
    double minDistance = std::pow(o.size * o.smoothing, 2);
    double prevPressure = points[0].pressure;
    for (size_t i = 0; i < points.size() && i < 10; i++) {
        double pr = points[i].pressure;
        if (o.simulatePressure) pr = SimulatePressure(prevPressure, points[i].distance, o.size);
        prevPressure = (prevPressure + pr) / 2;
    }
    double radius = StrokeRadius(o.size, o.thinning, points.back().pressure);
    double firstRadius = -1;
    V prevVector = points[0].vector, prevLeft = points[0].point, prevRight = prevLeft, tmpL = prevLeft, tmpR = prevRight;
    bool prevSharp = false;
    for (size_t i = 0; i < points.size(); i++) {
        double pressure = points[i].pressure;
        const V point = points[i].point, vector = points[i].vector;
        double distance = points[i].distance, running = points[i].runningLength;
        bool isLast = i == points.size() - 1;
        if (!isLast && total - running < END_NOISE_THRESHOLD) continue;
        if (o.thinning) {
            if (o.simulatePressure) pressure = SimulatePressure(prevPressure, distance, o.size);
            radius = StrokeRadius(o.size, o.thinning, pressure);
        } else radius = o.size / 2;
        if (firstRadius < 0) firstRadius = radius;
        double ts = running < taperStart ? easeStart(running / taperStart) : 1;
        double te = total - running < taperEnd ? easeEnd((total - running) / taperEnd) : 1;
        radius = std::max(MIN_RADIUS, radius * std::min(ts, te));
        V nextVector = (!isLast ? points[i + 1] : points[i]).vector;
        double nextDpr = !isLast ? dpr(vector, nextVector) : 1.0;
        double prevDpr = dpr(vector, prevVector);
        bool sharp = prevDpr < 0 && !prevSharp, nextSharp = nextDpr < 0;
        if (sharp || nextSharp) {
            V off = mul(per(prevVector), radius);
            double step = 1.0 / CORNER_CAP_SEGMENTS;
            for (double t = 0; t <= 1; t += step) {
                tmpL = rotAround(sub(point, off), point, FIXED_PI * t); left.push_back(tmpL);
                tmpR = rotAround(add(point, off), point, FIXED_PI * -t); right.push_back(tmpR);
            }
            prevLeft = tmpL; prevRight = tmpR;
            if (nextSharp) prevSharp = true;
            continue;
        }
        prevSharp = false;
        if (isLast) {
            V off = mul(per(vector), radius);
            left.push_back(sub(point, off));
            right.push_back(add(point, off));
            continue;
        }
        V off = mul(per(lrp(nextVector, vector, nextDpr)), radius);
        tmpL = sub(point, off);
        if (i <= 1 || dist2(prevLeft, tmpL) > minDistance) { left.push_back(tmpL); prevLeft = tmpL; }
        tmpR = add(point, off);
        if (i <= 1 || dist2(prevRight, tmpR) > minDistance) { right.push_back(tmpR); prevRight = tmpR; }
        prevPressure = pressure;
        prevVector = vector;
    }
    V first = points[0].point;
    V lastP = points.size() > 1 ? points.back().point : add(points[0].point, { 1, 1 });
    std::vector<V> startCap, endCap;
    if (points.size() == 1) {
        if (!(taperStart || taperEnd) || o.last) {
            std::vector<V> dot;
            double r = firstRadius >= 0 ? firstRadius : radius;
            V start = prj(first, uni(per(sub(first, add(first, { 1, 1 })))), -r);
            double step = 1.0 / START_CAP_SEGMENTS;
            for (double t = step; t <= 1; t += step) dot.push_back(rotAround(start, first, FIXED_PI * 2 * t));
            return dot;
        }
    } else {
        if (taperStart) {
        } else if (o.capStart && !right.empty()) {
            double step = 1.0 / START_CAP_SEGMENTS;
            for (double t = step; t <= 1; t += step) startCap.push_back(rotAround(right[0], first, FIXED_PI * t));
        } else if (!left.empty() && !right.empty()) {
            V c = sub(left[0], right[0]), a = mul(c, 0.5), b = mul(c, 0.51);
            startCap = { sub(first, a), sub(first, b), add(first, b), add(first, a) };
        }
        V dir = per(neg(points.back().vector));
        if (taperEnd) endCap.push_back(lastP);
        else if (o.capEnd) {
            V start = prj(lastP, dir, radius);
            double step = 1.0 / END_CAP_SEGMENTS;
            for (double t = step; t < 1; t += step) endCap.push_back(rotAround(start, lastP, FIXED_PI * 3 * t));
        } else endCap = { add(lastP, mul(dir, radius)), add(lastP, mul(dir, radius * 0.99)), sub(lastP, mul(dir, radius * 0.99)), sub(lastP, mul(dir, radius)) };
    }
    std::vector<V> out = left;
    out.insert(out.end(), endCap.begin(), endCap.end());
    out.insert(out.end(), right.rbegin(), right.rend());
    out.insert(out.end(), startCap.begin(), startCap.end());
    return out;
}

}  // namespace pf

// ---------------------------------------------------------------------------
// ferramentas

inline pf::Options OptionsFor(const Stroke& s, bool last) {
    pf::Options o;
    o.size = s.size; o.last = last;
    switch (s.tool) {
    case FOUNTAIN:  // a pressão já vem do ângulo da pena (NibInput)
        o.size = s.size * 1.15; o.thinning = 0.82; o.smoothing = 0.6; o.streamline = 0.48; o.simulatePressure = false;
        o.taperStart = s.size * 1.2; o.taperEnd = s.size * 2.2;
        break;
    case PENCIL:
        o.thinning = s.sim ? 0.35 : 0.6; o.smoothing = 0.5; o.streamline = 0.35; o.simulatePressure = s.sim;
        o.taperStart = s.size * 1.5; o.taperEnd = s.size * 2;
        break;
    case CRAYON:
        o.thinning = s.sim ? 0.1 : 0.22; o.smoothing = 0.4; o.streamline = 0.28; o.simulatePressure = s.sim;
        o.taperStart = s.size * 0.25; o.taperEnd = s.size * 0.35;
        break;
    case WATERCOLOR:  // pincel redondo molhado: pontas redondas (sem afinar), largura quase constante
        o.thinning = s.sim ? 0.12 : 0.3; o.smoothing = 0.8; o.streamline = 0.6; o.simulatePressure = s.sim;
        o.taperStart = 0; o.taperEnd = 0; o.capStart = o.capEnd = true;
        break;
    case BRUSHPEN:  // pincel de tinta (ponta de pincel firme): fino com pouca pressão, cheio apertando, pontas afinando
        o.size = s.size * 1.1; o.thinning = 0.78; o.smoothing = 0.65; o.streamline = 0.5; o.simulatePressure = s.sim;
        o.taperStart = s.size * 1.6; o.taperEnd = s.size * 2.6;
        break;
    case MONOLINE:  // traço de largura igual do começo ao fim (sem fino e grosso), pontas redondas
        o.thinning = 0; o.smoothing = 0.7; o.streamline = 0.5; o.simulatePressure = false;
        break;
    default:  // caneta
        o.thinning = s.sim ? 0.25 : 0.45; o.smoothing = 0.62; o.streamline = 0.42; o.simulatePressure = s.sim;
        break;
    }
    return o;
}

// caneta-tinteiro: a largura depende da direção do traço em relação a uma pena fixa a 40°
inline std::vector<pf::In> NibInput(const Stroke& s) {
    std::vector<pf::In> out;
    size_t n = s.pts.size();
    const double nib = -40 * 3.14159265358979323846 / 180;
    double smooth = 0.6;
    for (size_t i = 0; i < n; i++) {
        size_t a = i >= 2 ? i - 2 : 0, b = std::min(n - 1, i + 2);
        double dx = s.pts[b].x - s.pts[a].x, dy = s.pts[b].y - s.pts[a].y;
        double f = dx == 0 && dy == 0 ? smooth : 0.18 + 0.82 * std::fabs(std::sin(std::atan2(dy, dx) - nib));
        smooth = smooth * 0.65 + f * 0.35;
        double p = s.sim ? 0.62 : 0.35 + s.pts[i].p * 0.65;
        out.push_back({ { s.pts[i].x, s.pts[i].y }, std::min(1.0, smooth * p) });
    }
    return out;
}

inline std::vector<pf::In> Input(const Stroke& s) {
    if (s.tool == FOUNTAIN) return NibInput(s);
    std::vector<pf::In> out;
    out.reserve(s.pts.size());
    for (auto& p : s.pts) out.push_back({ { p.x, p.y }, s.sim ? 0.5 : p.p });
    return out;
}

// ---------------------------------------------------------------------------
// marcador: uma ponta chanfrada (retângulo fino a -38°) arrastada pelo caminho. Cada par de pontos vira o
// fecho convexo das duas pegadas da ponta; com o preenchimento "winding" tudo vira uma forma só (sem sobreposição escura).

namespace chisel {
using P3 = Pt;
inline std::vector<P3> Smooth(const std::vector<Pt>& raw, int passes = 1, float step = 2.5f) {
    if (raw.size() < 3) return raw;
    std::vector<P3> out = raw;
    for (int k = 0; k < passes; k++) {
        std::vector<P3> o{ out[0] };
        for (size_t i = 1; i + 1 < out.size(); i++) {
            auto& a = out[i - 1]; auto& b = out[i]; auto& c = out[i + 1];
            o.push_back({ (a.x + 2 * b.x + c.x) / 4, (a.y + 2 * b.y + c.y) / 4, (a.p + 2 * b.p + c.p) / 4 });
        }
        o.push_back(out.back());
        out.swap(o);
    }
    std::vector<P3> res{ out[0] };
    for (size_t i = 1; i < out.size(); i++) {
        P3 a = res.back(), b = out[i];
        int steps = (int)std::ceil(std::hypot(b.x - a.x, b.y - a.y) / step);
        for (int k = 1; k <= steps; k++) { float t = (float)k / steps; res.push_back({ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.p + (b.p - a.p) * t }); }
    }
    return res;
}
struct Q { float x, y; };
inline std::vector<Q> Hull(std::vector<Q> p) {
    std::sort(p.begin(), p.end(), [](const Q& a, const Q& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    auto cross = [](const Q& o, const Q& a, const Q& b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    std::vector<Q> lo, up;
    for (auto& q : p) { while (lo.size() >= 2 && cross(lo[lo.size() - 2], lo.back(), q) <= 0) lo.pop_back(); lo.push_back(q); }
    for (size_t i = p.size(); i-- > 0;) { auto& q = p[i]; while (up.size() >= 2 && cross(up[up.size() - 2], up.back(), q) <= 0) up.pop_back(); up.push_back(q); }
    lo.pop_back(); up.pop_back();
    lo.insert(lo.end(), up.begin(), up.end());
    return lo;
}
}  // namespace chisel

inline void AddPolygon(ID2D1GeometrySink* k, const std::vector<chisel::Q>& h) {
    if (h.size() < 3) return;
    k->BeginFigure({ h[0].x, h[0].y }, D2D1_FIGURE_BEGIN_FILLED);
    for (size_t i = 1; i < h.size(); i++) k->AddLine({ h[i].x, h[i].y });
    k->EndFigure(D2D1_FIGURE_END_CLOSED);
}

// marcador: ponta de feltro larga, quase em pé (MARKER_DEG). Caneta-tinteiro: pena fina a -40° (largura pela pressão), então o traço
// começa e termina com o corte inclinado da pena, grosso na descida e fino na subida, como a caligrafia de verdade.
inline void ChiselFigures(ID2D1GeometrySink* k, const Stroke& s) {
    bool pena = s.tool == FOUNTAIN, junco = s.tool == REEDPEN;
    auto pts = pena ? chisel::Smooth(s.pts) : junco ? chisel::Smooth(s.pts, 2, 1.5f) : chisel::Smooth(s.pts, 3, 1.25f);
    const float A = (pena ? -40 : junco ? -s.nib : MARKER_DEG) * 3.14159265f / 180, ux = std::cos(A), uy = std::sin(A), vx = -uy, vy = ux;
    auto nib = [&](const Pt& p) {
        // junco: corte reto e largo, a largura quase não muda com a pressão (a força está no ângulo)
        float w = pena ? s.size * (s.sim ? 0.95f : 0.5f + 0.75f * p.p) / 2 : junco ? s.size * (s.sim ? 1 : 0.85f + 0.3f * p.p) / 2 : s.size * (s.sim ? 1 : 0.85f + 0.3f * p.p) / 2;
        float t = pena ? std::max(0.7f, s.size * 0.11f) / 2 : junco ? std::max(0.6f, s.size * 0.07f) / 2 : std::max(1.2f, s.size * 0.3f) / 2;
        if (!pena && !junco) {  // feltro: cantos levemente gastos
            float e = std::min(t * 0.8f, w * 0.3f), wi = w - e, ti = t * 0.35f;
            return std::vector<chisel::Q>{ { p.x + ux * wi + vx * t, p.y + uy * wi + vy * t }, { p.x + ux * w + vx * ti, p.y + uy * w + vy * ti },
                                           { p.x + ux * w - vx * ti, p.y + uy * w - vy * ti }, { p.x + ux * wi - vx * t, p.y + uy * wi - vy * t },
                                           { p.x - ux * wi - vx * t, p.y - uy * wi - vy * t }, { p.x - ux * w - vx * ti, p.y - uy * w - vy * ti },
                                           { p.x - ux * w + vx * ti, p.y - uy * w + vy * ti }, { p.x - ux * wi + vx * t, p.y - uy * wi + vy * t } };
        }
        return std::vector<chisel::Q>{ { p.x + ux * w + vx * t, p.y + uy * w + vy * t }, { p.x + ux * w - vx * t, p.y + uy * w - vy * t },
                                       { p.x - ux * w - vx * t, p.y - uy * w - vy * t }, { p.x - ux * w + vx * t, p.y - uy * w + vy * t } };
    };
    auto prev = nib(pts[0]);
    if (pts.size() == 1) { AddPolygon(k, chisel::Hull(prev)); return; }
    for (size_t i = 1; i < pts.size(); i++) {
        auto cur = nib(pts[i]);
        std::vector<chisel::Q> both = prev;
        both.insert(both.end(), cur.begin(), cur.end());
        AddPolygon(k, chisel::Hull(both));
        prev = cur;
    }
}

// o contorno vira uma forma com curvas suaves pelos pontos médios (como o Quill faz no canvas)
inline ID2D1PathGeometry* BuildGeometry(ID2D1Factory* f, const Stroke& s, bool live) {
    if (!f || s.pts.empty()) return nullptr;
    ID2D1PathGeometry* g = nullptr;
    if (FAILED(f->CreatePathGeometry(&g))) return nullptr;
    ID2D1GeometrySink* k = nullptr;
    if (FAILED(g->Open(&k))) { g->Release(); return nullptr; }
    k->SetFillMode(D2D1_FILL_MODE_WINDING);
    if (s.tool == MARKER || s.tool == FOUNTAIN || s.tool == REEDPEN) {
        ChiselFigures(k, s);
    } else {
        std::vector<pf::V> o;
        if (s.pts.size() == 1) {  // um ponto: bolinha
            double r = std::max(0.6, s.size * (0.5 + 0.5 * (s.sim ? 0.5 : s.pts[0].p)) / 2);
            for (int i = 0; i < 24; i++) { double a = i * 2 * 3.14159265358979323846 / 24; o.push_back({ s.pts[0].x + r * std::cos(a), s.pts[0].y + r * std::sin(a) }); }
        } else {
            auto opt = OptionsFor(s, !live);
            o = pf::OutlinePoints(pf::StrokePoints(Input(s), opt), opt);
        }
        if (o.size() >= 2) {
            auto P = [](pf::V v) { return D2D1::Point2F((float)v.x, (float)v.y); };
            auto M = [](pf::V a, pf::V b) { return D2D1::Point2F((float)((a.x + b.x) / 2), (float)((a.y + b.y) / 2)); };
            k->BeginFigure(P(o[0]), D2D1_FIGURE_BEGIN_FILLED);
            for (size_t i = 1; i + 1 < o.size(); i++) k->AddQuadraticBezier(D2D1::QuadraticBezierSegment(P(o[i]), M(o[i], o[i + 1])));
            k->AddQuadraticBezier(D2D1::QuadraticBezierSegment(P(o.back()), M(o.back(), o[0])));
            k->EndFigure(D2D1_FIGURE_END_CLOSED);
        }
    }
    k->Close(); k->Release();
    return g;
}

// ---------------------------------------------------------------------------
// texturas: grafite do lápis e cera do giz (o papel tem "dentes": a cera pega só nos altos)

inline unsigned Lehmer(unsigned& seed) { seed = (unsigned)(((unsigned long long)seed * 16807ULL) % 2147483647ULL); return seed; }

const int TOOTH = 192;
inline const std::vector<float>& ToothMap() {
    static std::vector<float> m;
    if (!m.empty()) return m;
    unsigned seed = 90210;
    auto rnd = [&] { return Lehmer(seed) / 2147483647.f; };
    struct Oct { int cells; std::vector<float> g; };
    auto make = [&](int cells) { Oct o{ cells, std::vector<float>(cells * cells) }; for (auto& v : o.g) v = rnd(); return o; };
    auto sample = [](const Oct& o, float x, float y) {
        auto at = [&](int ix, int iy) { int c = o.cells; return o.g[((iy % c + c) % c) * c + ((ix % c + c) % c)]; };
        float fx = x / TOOTH * o.cells, fy = y / TOOTH * o.cells;
        int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
        float tx = fx - x0, ty = fy - y0;
        tx = tx * tx * (3 - 2 * tx); ty = ty * ty * (3 - 2 * ty);
        return (at(x0, y0) * (1 - tx) + at(x0 + 1, y0) * tx) * (1 - ty) + (at(x0, y0 + 1) * (1 - tx) + at(x0 + 1, y0 + 1) * tx) * ty;
    };
    Oct o1 = make(12), o2 = make(32), o3 = make(96);
    m.resize(TOOTH * TOOTH);
    for (int y = 0; y < TOOTH; y++)
        for (int x = 0; x < TOOTH; x++) {
            float sx = x + y * 0.45f;  // fibras do papel puxadas na diagonal
            m[y * TOOTH + x] = sample(o1, sx, (float)y) * 0.18f + sample(o2, sx, (float)y) * 0.34f + sample(o3, x + y * 0.2f, (float)y) * 0.48f;
        }
    return m;
}

// quanto dos dentes do papel a cera preenche (0..4): mais pressão, mais cobertura
inline int CrayonCoverage(const Stroke& s) {
    if (s.sim || s.pts.empty()) return 2;
    double sum = 0;
    for (auto& p : s.pts) sum += p.p;
    return std::clamp((int)std::lround(sum / s.pts.size() * 5 - 0.5), 0, 4);
}

// pincéis de textura, um por alvo de desenho + cor + tipo (o pincel do D2D é preso ao alvo)
struct BrushKey { ID2D1RenderTarget* rt; UINT32 rgba; int kind; ID2D1BitmapBrush* brush; };
inline std::vector<BrushKey>& BrushCache() { static std::vector<BrushKey> c; return c; }
inline void ClearCache() { for (auto& b : BrushCache()) b.brush->Release(); BrushCache().clear(); }

// kind 0: grafite; 1..5: cera com cobertura kind-1
inline ID2D1BitmapBrush* TextureBrush(ID2D1RenderTarget* rt, D2D1_COLOR_F c, int kind) {
    UINT32 rgba = ((UINT32)(c.r * 255) << 24) | ((UINT32)(c.g * 255) << 16) | ((UINT32)(c.b * 255) << 8) | (UINT32)(c.a * 255);
    for (auto& b : BrushCache()) if (b.rt == rt && b.rgba == rgba && b.kind == kind) return b.brush;
    if (BrushCache().size() > 96) ClearCache();
    int N = kind == 0 ? 96 : TOOTH;
    std::vector<UINT32> px(N * N);
    auto put = [&](int i, int alpha) {
        float a = alpha / 255.f;
        px[i] = ((UINT32)alpha << 24) | ((UINT32)(c.r * a * 255 + 0.5f) << 16) | ((UINT32)(c.g * a * 255 + 0.5f) << 8) | (UINT32)(c.b * a * 255 + 0.5f);
    };
    if (kind == 0) {
        unsigned seed = 1337;
        for (int i = 0; i < N * N; i++) { float r = Lehmer(seed) / 2147483647.f; put(i, r < 0.22f ? 0 : r < 0.5f ? 150 : r < 0.8f ? 215 : 255); }
    } else {
        auto& m = ToothMap();
        float thr = 0.47f - (kind - 1) * 0.045f;
        for (int i = 0; i < N * N; i++) { float v = (m[i] - thr) / 0.2f; put(i, v <= 0 ? 0 : v >= 1 ? 255 : (int)std::lround(v * 255)); }
    }
    ID2D1Bitmap* bmp = nullptr;
    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    if (FAILED(rt->CreateBitmap(D2D1::SizeU(N, N), px.data(), N * 4, bp, &bmp))) return nullptr;
    ID2D1BitmapBrush* br = nullptr;
    rt->CreateBitmapBrush(bmp, D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR), &br);
    bmp->Release();
    if (!br) return nullptr;
    br->SetTransform(D2D1::Matrix3x2F::Scale(kind == 0 ? 0.5f : 0.55f, kind == 0 ? 0.5f : 0.55f));
    BrushCache().push_back({ rt, rgba, kind, br });
    return br;
}

// ---------------------------------------------------------------------------
// desenha o traço com a origem em o e a escala k (a tela usa 1; o PNG gravado usa 2)
// papel escuro: a tinta quase preta aparece clara (como o preto adaptável do Mac); as outras cores ficam iguais
inline bool& DarkPaper() { static bool d = false; return d; }
inline D2D1_COLOR_F Shown(D2D1_COLOR_F c) { return c; }  // a tinta é a cor escolhida em qualquer papel (preto continua preto)

inline void Paint(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* br, const Stroke& s0, ID2D1PathGeometry* g) {
    Stroke const& s = s0;
    D2D1_COLOR_F c = Shown(s.color);
    float op = std::clamp(s.opacity, 0.05f, 1.f);
    switch (s.tool) {
    case MARKER: {  // tinta translúcida: dois traços cruzados escurecem como marca-texto
        c.a = (DarkPaper() ? 0.62f : 0.55f) * op; br->SetColor(c); rt->FillGeometry(g, br);
        // onde a ponta de feltro encosta e levanta a tinta junta um pouco mais (as pontas ficam mais escuras)
        ID2D1Factory* f = nullptr; rt->GetFactory(&f);
        if (f && s.pts.size() >= 2) {
            ID2D1PathGeometry* pool = nullptr;
            if (SUCCEEDED(f->CreatePathGeometry(&pool))) {
                ID2D1GeometrySink* k = nullptr;
                if (SUCCEEDED(pool->Open(&k))) {
                    k->SetFillMode(D2D1_FILL_MODE_WINDING);
                    const float A = MARKER_DEG * 3.14159265f / 180, ux = std::cos(A), uy = std::sin(A), vx = -uy, vy = ux;
                    for (int e = 0; e < 2; e++) {
                        const Pt& p = e ? s.pts.back() : s.pts.front();
                        float w = s.size * (s.sim ? 1 : 0.85f + 0.3f * p.p) / 2 * 0.92f, t = std::max(1.2f, s.size * 0.3f) / 2;
                        std::vector<chisel::Q> q{ { p.x + ux * w + vx * t, p.y + uy * w + vy * t }, { p.x + ux * w - vx * t, p.y + uy * w - vy * t },
                                                  { p.x - ux * w - vx * t, p.y - uy * w - vy * t }, { p.x - ux * w + vx * t, p.y - uy * w + vy * t } };
                        AddPolygon(k, chisel::Hull(q));
                    }
                    k->Close(); k->Release();
                    c.a = 0.16f * op; br->SetColor(c); rt->FillGeometry(pool, br);
                }
                pool->Release();
            }
        }
        if (f) f->Release();
        break;
    }
    case PENCIL:
        if (auto t = TextureBrush(rt, c, 0)) { t->SetOpacity(0.92f * op); rt->FillGeometry(g, t); }
        else { c.a = 0.8f * op; br->SetColor(c); rt->FillGeometry(g, br); }
        break;
    case CRAYON: {
        int cov = CrayonCoverage(s);
        // franja irregular: a cera só pega nos dentes mais altos logo fora do corpo do traço
        if (auto t = TextureBrush(rt, c, 1 + std::max(0, cov - 2))) { t->SetOpacity(0.5f * op); rt->DrawGeometry(g, t, s.size * 0.22f); }
        // corpo: a cera cobre os dentes do papel, com o papel aparecendo nos vales
        if (auto t = TextureBrush(rt, c, 1 + std::max(0, cov - 1))) { t->SetOpacity(0.85f * op); rt->FillGeometry(g, t); }
        // miolo: onde a ponta apoia mais, a cera fecha mais
        ID2D1Factory* f = nullptr; rt->GetFactory(&f);
        if (f && s.pts.size() >= 2) {
            Stroke core = s; core.size = s.size * 0.55f;
            if (ID2D1PathGeometry* cg = BuildGeometry(f, core, true)) {
                if (auto t = TextureBrush(rt, c, 1 + std::min(4, cov + 1))) { t->SetOpacity(0.8f * op); rt->FillGeometry(cg, t); }
                cg->Release();
            }
        }
        if (f) f->Release();
        break;
    }
    case WATERCOLOR: {  // tinta aguada: corpo translúcido, o pigmento juntando na borda, tudo desfocado (borda molhada)
        float w = s.size;
        ID2D1DeviceContext* dc = nullptr;
        ID2D1CommandList* cl = nullptr;
        ID2D1Effect* blur = nullptr;
        ID2D1Image* target = nullptr;
        if (SUCCEEDED(rt->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&dc)) && dc &&
            SUCCEEDED(dc->CreateCommandList(&cl)) && SUCCEEDED(dc->CreateEffect(CLSID_D2D1GaussianBlur, &blur))) {
            dc->GetTarget(&target);
            dc->SetTarget(cl);
            c.a = 0.40f * op; br->SetColor(c); dc->FillGeometry(g, br);
            // a borda do pigmento segue só o contorno de fora (sem as voltas internas das pontas)
            ID2D1Factory* f = nullptr; dc->GetFactory(&f);
            ID2D1PathGeometry* outer = nullptr;
            if (f && SUCCEEDED(f->CreatePathGeometry(&outer))) {
                ID2D1GeometrySink* sk = nullptr;
                if (SUCCEEDED(outer->Open(&sk))) { g->Outline(nullptr, sk); sk->Close(); sk->Release(); }
                c.a = 0.22f * op; br->SetColor(c); dc->DrawGeometry(outer, br, std::max(1.f, w * 0.07f));
                outer->Release();
            }
            if (f) f->Release();
            dc->SetTarget(target);
            cl->Close();
            D2D1_MATRIX_3X2_F m; dc->GetTransform(&m);
            float k = std::hypot(m._11, m._12);  // escala da tela (zoom / 2x do PNG)
            blur->SetInput(0, cl);
            blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, std::max(0.6f, w * 0.075f * k));
            blur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_SOFT);
            blur->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_GAUSSIANBLUR_OPTIMIZATION_QUALITY);
            dc->SetTransform(D2D1::Matrix3x2F::Identity());
            dc->DrawImage(blur);
            dc->SetTransform(m);
        } else {  // sem efeitos: camadas largas e fracas imitam a borda macia
            c.a = 0.04f * op; br->SetColor(c); rt->DrawGeometry(g, br, w * 0.28f);
            c.a = 0.05f * op; br->SetColor(c); rt->DrawGeometry(g, br, w * 0.16f);
            c.a = 0.06f * op; br->SetColor(c); rt->DrawGeometry(g, br, w * 0.07f);
            c.a = 0.28f * op; br->SetColor(c); rt->FillGeometry(g, br);
        }
        if (target) target->Release();
        if (blur) blur->Release();
        if (cl) cl->Release();
        if (dc) dc->Release();
        break;
    }
    default:
        c.a = op; br->SetColor(c); rt->FillGeometry(g, br);
    }
}

inline void Draw(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* br, Stroke& s, D2D1_POINT_2F o, float k, bool live) {
    ID2D1Factory* f = nullptr; rt->GetFactory(&f);
    if (!f) return;
    ID2D1PathGeometry* g = nullptr;
    bool temp = true;
    if (!live && s.geo && s.geoFactory == f) { g = s.geo; temp = false; }
    else {
        g = BuildGeometry(f, s, live);
        if (g && !live && !s.geo) { s.geo = g; s.geoFactory = f; g->AddRef(); }
    }
    if (g) {
        D2D1_MATRIX_3X2_F old; rt->GetTransform(&old);
        rt->SetTransform(D2D1::Matrix3x2F::Scale(k, k) * D2D1::Matrix3x2F::Translation(o.x, o.y) * *D2D1::Matrix3x2F::ReinterpretBaseType(&old));
        Paint(rt, br, s, g);
        rt->SetTransform(old);
        if (temp) g->Release();
    }
    f->Release();
}


// ---------------------------------------------------------------------------
// geometria: borracha, laço, régua, linha reta e formas

struct Box { float x1, y1, x2, y2; };
inline Box BBox(const Stroke& s) {
    Box b{ 1e9f, 1e9f, -1e9f, -1e9f };
    for (auto& p : s.pts) { b.x1 = std::min(b.x1, p.x); b.y1 = std::min(b.y1, p.y); b.x2 = std::max(b.x2, p.x); b.y2 = std::max(b.y2, p.y); }
    float pad = s.size;
    return { b.x1 - pad, b.y1 - pad, b.x2 + pad, b.y2 + pad };
}
inline float DistToSeg(float px, float py, float ax, float ay, float bx, float by) {
    float dx = bx - ax, dy = by - ay, l = dx * dx + dy * dy;
    float t = l ? ((px - ax) * dx + (py - ay) * dy) / l : 0;
    t = std::clamp(t, 0.f, 1.f);
    return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}
inline bool Hit(const Stroke& s, float x, float y, float r) {
    Box b = BBox(s);
    if (x < b.x1 - r || x > b.x2 + r || y < b.y1 - r || y > b.y2 + r) return false;
    float rr = r + s.size / 2;
    if (s.pts.size() == 1) return std::hypot(s.pts[0].x - x, s.pts[0].y - y) <= rr;
    for (size_t i = 0; i + 1 < s.pts.size(); i++)
        if (DistToSeg(x, y, s.pts[i].x, s.pts[i].y, s.pts[i + 1].x, s.pts[i + 1].y) <= rr) return true;
    return false;
}
// borracha de pixels: tira o pedaço do traço que está dentro do raio; devolve false se não tocou
inline bool EraseFrom(const Stroke& s, float x, float y, float r, std::vector<Stroke>& pieces) {
    if (!Hit(s, x, y, r)) return false;
    float rr = r + s.size * 0.35f;
    // pontos mais juntos onde o traço passa perto da borracha (retas e formas têm pontos espaçados)
    std::vector<Pt> dense;
    float step = std::max(0.5f, r * 0.25f);
    for (size_t i = 0; i < s.pts.size(); i++) {
        if (i) {
            const Pt& a = s.pts[i - 1]; const Pt& b = s.pts[i];
            float L = std::hypot(b.x - a.x, b.y - a.y);
            if (L > step && DistToSeg(x, y, a.x, a.y, b.x, b.y) <= rr + L) {
                int n = (int)std::ceil(L / step);
                for (int k = 1; k < n; k++) { float t = (float)k / n; dense.push_back({ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.p + (b.p - a.p) * t }); }
            }
        }
        dense.push_back(s.pts[i]);
    }
    std::vector<Pt> cur;
    auto flush = [&] { if (cur.size() >= 2) { Stroke n = s; n.pts = cur; pieces.push_back(std::move(n)); } cur.clear(); };
    for (auto& p : dense) {
        bool inside = std::hypot(p.x - x, p.y - y) <= rr;
        if (!inside && !cur.empty() && DistToSeg(x, y, cur.back().x, cur.back().y, p.x, p.y) <= rr) flush();
        if (inside) flush(); else cur.push_back(p);
    }
    flush();
    return true;
}
inline bool InPolygon(float x, float y, const std::vector<D2D1_POINT_2F>& poly) {
    bool in = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        float xi = poly[i].x, yi = poly[i].y, xj = poly[j].x, yj = poly[j].y;
        if ((yi > y) != (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi) in = !in;
    }
    return in;
}
// quão perto de uma reta está o traço (1 = reto)
inline float Straightness(const std::vector<Pt>& p) {
    if (p.size() < 3) return 1;
    float ax = p[0].x, ay = p[0].y, bx = p.back().x, by = p.back().y, len = std::hypot(bx - ax, by - ay);
    if (len < 12) return 0;
    float dev = 0;
    for (auto& q : p) dev = std::max(dev, DistToSeg(q.x, q.y, ax, ay, bx, by));
    return 1 - dev / len;
}
inline std::vector<Pt> Line(float ax, float ay, float bx, float by, float p) {
    int steps = std::max(2, (int)std::ceil(std::hypot(bx - ax, by - ay) / 6));
    std::vector<Pt> o;
    for (int i = 0; i <= steps; i++) { float t = (float)i / steps; o.push_back({ ax + (bx - ax) * t, ay + (by - ay) * t, p }); }
    return o;
}

namespace forma {
using V2 = D2D1_POINT_2F;
inline std::vector<V2> Rdp(const std::vector<V2>& pts, float eps) {
    if (pts.size() < 3) return pts;
    V2 a = pts[0], b = pts.back();
    float L = std::hypot(b.x - a.x, b.y - a.y), mx = 0;
    size_t idx = 0;
    for (size_t i = 1; i + 1 < pts.size(); i++) {
        float d = L ? std::fabs((b.y - a.y) * pts[i].x - (b.x - a.x) * pts[i].y + b.x * a.y - b.y * a.x) / L : std::hypot(pts[i].x - a.x, pts[i].y - a.y);
        if (d > mx) { mx = d; idx = i; }
    }
    if (mx <= eps) return { a, b };
    auto l = Rdp(std::vector<V2>(pts.begin(), pts.begin() + idx + 1), eps), r = Rdp(std::vector<V2>(pts.begin() + idx, pts.end()), eps);
    l.pop_back();
    l.insert(l.end(), r.begin(), r.end());
    return l;
}
inline std::vector<Pt> Poly(const std::vector<V2>& vs, float p) {
    std::vector<Pt> o;
    for (size_t i = 0; i + 1 < vs.size(); i++) {
        auto seg = Line(vs[i].x, vs[i].y, vs[i + 1].x, vs[i + 1].y, p);
        o.insert(o.end(), seg.begin() + (i ? 1 : 0), seg.end());
    }
    return o;
}
}  // namespace forma

// um laço fechado feito à mão vira elipse, retângulo ou triângulo limpos (vazio: não reconheceu)
inline std::vector<Pt> RecognizeShape(const std::vector<Pt>& pts) {
    using forma::V2;
    size_t n = pts.size();
    if (n < 12) return {};
    std::vector<V2> xy;
    float x1 = 1e9f, y1 = 1e9f, x2 = -1e9f, y2 = -1e9f;
    for (auto& p : pts) { xy.push_back({ p.x, p.y }); x1 = std::min(x1, p.x); y1 = std::min(y1, p.y); x2 = std::max(x2, p.x); y2 = std::max(y2, p.y); }
    float w = x2 - x1, h = y2 - y1, diag = std::hypot(w, h);
    if (diag < 30) return {};
    if (std::hypot(xy[0].x - xy[n - 1].x, xy[0].y - xy[n - 1].y) > diag * 0.22f) return {};
    float p = pts[n / 2].p > 0 ? pts[n / 2].p : 0.5f;
    float cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
    size_t farI = 0; float fd = -1;
    for (size_t i = 0; i < n; i++) { float d = std::hypot(xy[i].x - cx, xy[i].y - cy); if (d > fd) { fd = d; farI = i; } }
    std::vector<V2> loop(xy.begin() + farI, xy.end());
    loop.insert(loop.end(), xy.begin(), xy.begin() + farI);
    loop.push_back(loop[0]);
    auto simple = forma::Rdp(loop, diag * 0.08f);
    int corners = (int)simple.size() - 1;
    float rx = w / 2 > 0 ? w / 2 : 1, ry = h / 2 > 0 ? h / 2 : 1, err = 0;
    for (auto& q : xy) err += std::fabs(std::hypot((q.x - cx) / rx, (q.y - cy) / ry) - 1);
    err /= n;
    if (corners == 3) return forma::Poly({ simple[0], simple[1], simple[2], simple[0] }, p);
    if (corners == 4 && err > 0.06f) {
        std::vector<V2> v(simple.begin(), simple.begin() + 4);
        bool tilted = false;
        for (auto& q : v) if (std::min(std::fabs(q.x - x1), std::fabs(q.x - x2)) > w * 0.18f || std::min(std::fabs(q.y - y1), std::fabs(q.y - y2)) > h * 0.18f) tilted = true;
        std::vector<V2> quad = tilted ? v : std::vector<V2>{ { x1, y1 }, { x2, y1 }, { x2, y2 }, { x1, y2 } };
        quad.push_back(quad[0]);
        return forma::Poly(quad, p);
    }
    if (err < 0.14f) {
        std::vector<Pt> o;
        int steps = std::max(48, (int)std::ceil((rx + ry) * 3.14159265f / 5));
        float start = std::atan2((xy[0].y - cy) / ry, (xy[0].x - cx) / rx);
        for (int i = 0; i <= steps; i++) { float t = start + (float)i / steps * 2 * 3.14159265f; o.push_back({ cx + std::cos(t) * rx, cy + std::sin(t) * ry, p }); }
        return o;
    }
    return {};
}

// papel: 0 liso, 1 pautado, 2 quadriculado, 3 pontilhado (espaço de 32 pt, como no Quill)
inline void DrawPattern(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* br, D2D1_RECT_F r, int pattern, bool dark = false, float lineW = 1) {
    if (pattern <= 0) return;
    const float gap = 32;
    if (pattern == 3) {
        br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.22f) : D2D1::ColorF(60 / 255.f, 60 / 255.f, 80 / 255.f, 0.26f));
        for (float y = std::floor(r.top / gap) * gap + gap; y < r.bottom; y += gap)
            for (float x = std::floor(r.left / gap) * gap + gap; x < r.right; x += gap) rt->FillEllipse(D2D1::Ellipse({ x, y }, 1.4f * lineW, 1.4f * lineW), br);
        return;
    }
    if (pattern == 1) br->SetColor(dark ? D2D1::ColorF(120 / 255.f, 160 / 255.f, 1, 0.22f) : D2D1::ColorF(70 / 255.f, 120 / 255.f, 220 / 255.f, 0.22f));
    else br->SetColor(dark ? D2D1::ColorF(1, 1, 1, 0.10f) : D2D1::ColorF(40 / 255.f, 40 / 255.f, 60 / 255.f, 0.10f));
    for (float y = std::floor(r.top / gap) * gap + gap; y < r.bottom; y += gap) rt->DrawLine({ r.left, y }, { r.right, y }, br, lineW);
    if (pattern == 2) for (float x = std::floor(r.left / gap) * gap + gap; x < r.right; x += gap) rt->DrawLine({ x, r.top }, { x, r.bottom }, br, lineW);
}

}  // namespace tinta
