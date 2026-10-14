#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

//==============================================================================
//  Кривая: точки (x, y от 0 до 127) и изгиб каждого отрезка.
//  bend — изгиб отрезка от этой точки до следующей:
//  0 — прямая, больше 0 — выгнут вверх, меньше 0 — вниз (от -10 до 10).
//==============================================================================
struct CurvePoint
{
    int x = 0;
    int y = 0;
    double bend = 0.0;
};

class Curve
{
public:
    static constexpr int    maxValue = 127;
    static constexpr double maxBend  = 10.0;

    Curve() { reset(); }

    // прямая от (0, 0) до (127, 127)
    void reset();

    const std::vector<CurvePoint>& getPoints() const noexcept { return points; }
    int  size() const noexcept                     { return (int) points.size(); }
    bool isEndpoint (int index) const noexcept     { return index == 0 || index == size() - 1; }

    // значение кривой в точке x (0..127)
    double evaluate (double x) const;

    // значение на отрезке seg при t от 0 до 1
    double segmentValue (int seg, double t) const;

    // таблица «пришло -> ушло» для всех 128 значений velocity
    void fillLookup (std::array<juce::uint8, 128>& table) const;

    // добавить точку; возвращает её номер или -1, если на этом x уже есть точка
    int  addPoint (int x, int y);
    bool removePoint (int index);

    // переместить точку с учётом соседей (крайние двигаются только по вертикали)
    void movePoint (int index, int x, int y);

    // по какому x может стоять точка
    juce::Range<int> allowedX (int index) const;

    // изгиб отрезка по положению ручки (y, куда потянули середину отрезка)
    bool setBendFromHandle (int seg, double targetY);
    void setBend (int seg, double bend);

    // привести в порядок после ручного ввода или загрузки
    void normalise();

    //==========================================================================
    enum class ParseError { none, badJson, noPoints, badPoint };

    juce::String toJson (const juce::String& helpText) const;
    static ParseError fromJson (const juce::String& text, Curve& result, juce::String& detail);

    bool operator== (const Curve& other) const;
    bool operator!= (const Curve& other) const { return ! operator== (other); }

private:
    static double shape (double t, double c);
    static double shapeFactor (double bend, double dy);

    std::vector<CurvePoint> points;
};
