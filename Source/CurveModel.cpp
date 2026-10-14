#include "CurveModel.h"

#include <algorithm>
#include <cmath>

namespace
{
    juce::String formatNumber (double value)
    {
        // 2 знака после точки, лишние нули убираем: 2.50 -> 2.5, 3.00 -> 3
        auto text = juce::String (value, 2);

        if (text.containsChar ('.'))
        {
            text = text.trimCharactersAtEnd ("0");
            if (text.endsWithChar ('.'))
                text = text.dropLastCharacters (1);
        }

        if (text == "-0")
            text = "0";

        return text;
    }

    juce::String escapeJson (const juce::String& s)
    {
        return s.replace ("\\", "\\\\").replace ("\"", "\\\"").replace ("\n", "\\n");
    }
}

//==============================================================================
void Curve::reset()
{
    points.clear();
    points.push_back ({ 0, 0, 0.0 });
    points.push_back ({ maxValue, maxValue, 0.0 });
}

//==============================================================================
// Форма отрезка: при c = 0 прямая, иначе плавная экспонента.
// В середине отрезка (t = 0.5) значение равно 1 / (e^(c/2) + 1).
double Curve::shape (double t, double c)
{
    if (std::abs (c) < 1.0e-6)
        return t;

    return (std::exp (c * t) - 1.0) / (std::exp (c) - 1.0);
}

// bend > 0 всегда означает «выгнут вверх», независимо от того,
// идёт отрезок вверх или вниз
double Curve::shapeFactor (double bend, double dy)
{
    return dy >= 0.0 ? -bend : bend;
}

double Curve::segmentValue (int seg, double t) const
{
    const auto& a = points[(size_t) seg];
    const auto& b = points[(size_t) seg + 1];
    const double dy = (double) (b.y - a.y);

    return (double) a.y + dy * shape (t, shapeFactor (a.bend, dy));
}

double Curve::evaluate (double x) const
{
    if (points.size() < 2)
        return x;

    x = juce::jlimit (0.0, (double) maxValue, x);

    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        const auto& a = points[i];
        const auto& b = points[i + 1];

        if (x <= (double) b.x || i + 2 == points.size())
        {
            if (b.x == a.x)
                return (double) b.y;

            const double t = juce::jlimit (0.0, 1.0, (x - (double) a.x) / (double) (b.x - a.x));
            return segmentValue ((int) i, t);
        }
    }

    return (double) points.back().y;
}

void Curve::fillLookup (std::array<juce::uint8, 128>& table) const
{
    for (int v = 0; v < 128; ++v)
    {
        const auto y = juce::roundToInt (evaluate ((double) v));
        table[(size_t) v] = (juce::uint8) juce::jlimit (0, maxValue, y);
    }
}

//==============================================================================
juce::Range<int> Curve::allowedX (int index) const
{
    if (index <= 0)
        return { 0, 0 };

    if (index >= size() - 1)
        return { maxValue, maxValue };

    return { points[(size_t) index - 1].x + 1, points[(size_t) index + 1].x - 1 };
}

int Curve::addPoint (int x, int y)
{
    x = juce::jlimit (1, maxValue - 1, x);
    y = juce::jlimit (0, maxValue, y);

    for (const auto& p : points)
        if (p.x == x)
            return -1;

    size_t index = 1;
    while (index < points.size() && points[index].x < x)
        ++index;

    // обе половины разрезанного отрезка получают его прежний изгиб
    const auto bend = points[index - 1].bend;
    points.insert (points.begin() + (std::ptrdiff_t) index, { x, y, bend });

    return (int) index;
}

bool Curve::removePoint (int index)
{
    if (index <= 0 || index >= size() - 1)
        return false;

    points.erase (points.begin() + index);
    return true;
}

void Curve::movePoint (int index, int x, int y)
{
    if (index < 0 || index >= size())
        return;

    const auto range = allowedX (index);
    auto& p = points[(size_t) index];

    p.x = juce::jlimit (range.getStart(), range.getEnd(), x);
    p.y = juce::jlimit (0, maxValue, y);
}

bool Curve::setBendFromHandle (int seg, double targetY)
{
    if (seg < 0 || seg >= size() - 1)
        return false;

    const auto& a = points[(size_t) seg];
    const auto& b = points[(size_t) seg + 1];
    const double dy = (double) (b.y - a.y);

    // у горизонтального отрезка изгибать нечего
    if (std::abs (dy) < 1.0)
        return false;

    const double m = juce::jlimit (0.005, 0.995, (targetY - (double) a.y) / dy);
    const double c = 2.0 * std::log (1.0 / m - 1.0);

    setBend (seg, dy > 0.0 ? -c : c);
    return true;
}

void Curve::setBend (int seg, double bend)
{
    if (seg < 0 || seg >= size() - 1)
        return;

    bend = juce::jlimit (-maxBend, maxBend, bend);
    bend = std::round (bend * 100.0) / 100.0;

    // почти прямой отрезок делаем ровно прямым
    if (std::abs (bend) < 0.08)
        bend = 0.0;

    points[(size_t) seg].bend = bend;
}

//==============================================================================
void Curve::normalise()
{
    for (auto& p : points)
    {
        p.x = juce::jlimit (0, maxValue, p.x);
        p.y = juce::jlimit (0, maxValue, p.y);
        p.bend = std::isfinite (p.bend) ? juce::jlimit (-maxBend, maxBend, p.bend) : 0.0;
    }

    std::stable_sort (points.begin(), points.end(),
                      [] (const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });

    // на одном x может стоять только одна точка
    points.erase (std::unique (points.begin(), points.end(),
                               [] (const CurvePoint& a, const CurvePoint& b) { return a.x == b.x; }),
                  points.end());

    if (points.empty() || points.front().x != 0)
        points.insert (points.begin(), { 0, 0, 0.0 });

    if (points.back().x != maxValue)
        points.push_back ({ maxValue, maxValue, 0.0 });

    points.back().bend = 0.0;
}

//==============================================================================
juce::String Curve::toJson (const juce::String& helpText) const
{
    juce::String json;

    json << "{\n"
         << "  \"app\": \"MidiCurveVST\",\n"
         << "  \"version\": 1,\n"
         << "  \"help\": \"" << escapeJson (helpText) << "\",\n"
         << "  \"points\": [\n";

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& p = points[i];
        const bool last = (i + 1 == points.size());

        json << "    { \"x\": " << p.x << ", \"y\": " << p.y;

        if (! last)
            json << ", \"curve\": " << formatNumber (p.bend);

        json << " }" << (last ? "" : ",") << "\n";
    }

    json << "  ]\n"
         << "}\n";

    return json;
}

Curve::ParseError Curve::fromJson (const juce::String& text, Curve& result, juce::String& detail)
{
    juce::var root;
    const auto parsed = juce::JSON::parse (text, root);

    if (parsed.failed())
    {
        detail = parsed.getErrorMessage();
        return ParseError::badJson;
    }

    const auto* list = root.getProperty ("points", juce::var()).getArray();

    if (list == nullptr || list->isEmpty())
        return ParseError::noPoints;

    std::vector<CurvePoint> loaded;

    for (int i = 0; i < list->size(); ++i)
    {
        const auto& item = list->getReference (i);

        if (! item.hasProperty ("x") || ! item.hasProperty ("y"))
        {
            detail = juce::String (i + 1);
            return ParseError::badPoint;
        }

        CurvePoint p;
        p.x = juce::roundToInt ((double) item.getProperty ("x", 0));
        p.y = juce::roundToInt ((double) item.getProperty ("y", 0));
        p.bend = (double) item.getProperty ("curve", 0.0);
        loaded.push_back (p);
    }

    result.points = std::move (loaded);
    result.normalise();
    return ParseError::none;
}

bool Curve::operator== (const Curve& other) const
{
    if (points.size() != other.points.size())
        return false;

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& a = points[i];
        const auto& b = other.points[i];

        if (a.x != b.x || a.y != b.y || std::abs (a.bend - b.bend) > 1.0e-9)
            return false;
    }

    return true;
}
