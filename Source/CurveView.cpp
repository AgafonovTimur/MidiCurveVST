#include "CurveView.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float margin       = 10.0f;   // отступ, чтобы крайние точки не обрезались
    constexpr float pointRadius  = 5.5f;
    constexpr float handleRadius = 4.5f;
    constexpr float pointHit     = 10.0f;   // радиус «попадания» мышью
    constexpr float handleHit    = 9.0f;
}

CurveView::CurveView()
{
    setRepaintsOnMouseActivity (false);
    // щелчок по полю забирает фокус у полей X/Y — введённое там значение применяется
    setWantsKeyboardFocus (true);
}

void CurveView::setCurve (const Curve& newCurve)
{
    curve = newCurve;

    if (selected >= curve.size())
        selected = -1;

    dragPoint = dragHandle = hoverPoint = hoverHandle = -1;
    repaint();
}

void CurveView::setSelected (int index)
{
    selected = (index >= 0 && index < curve.size()) ? index : -1;
    repaint();

    if (onSelectionChanged != nullptr)
        onSelectionChanged();
}

void CurveView::setPointPosition (int index, int x, int y)
{
    if (index < 0 || index >= curve.size())
        return;

    curve.movePoint (index, x, y);
    repaint();
    changed (true);
}

//==============================================================================
void CurveView::addPlayedNote (int inVelocity, int outVelocity)
{
    markers.push_back ({ inVelocity, outVelocity, juce::Time::getMillisecondCounterHiRes() });

    if (markers.size() > 32)
        markers.erase (markers.begin());

    repaint();
}

void CurveView::clearPlayedNotes()
{
    if (! markers.empty())
    {
        markers.clear();
        repaint();
    }
}

float CurveView::markerAlpha (const NoteMarker& marker, double now) const
{
    return (float) juce::jlimit (0.0, 1.0, 1.0 - (now - marker.startTime) / markerFadeMs);
}

void CurveView::updatePlayedNotes()
{
    if (markers.empty())
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();

    markers.erase (std::remove_if (markers.begin(), markers.end(),
                                   [this, now] (const NoteMarker& m) { return markerAlpha (m, now) <= 0.0f; }),
                   markers.end());

    // пока отметки гаснут — перерисовываем
    repaint();
}

//==============================================================================
juce::Rectangle<float> CurveView::plotArea() const
{
    return getLocalBounds().toFloat().reduced (margin);
}

juce::Point<float> CurveView::toPixel (double x, double y) const
{
    const auto area = plotArea();
    return { area.getX() + (float) (x / Curve::maxValue) * area.getWidth(),
             area.getBottom() - (float) (y / Curve::maxValue) * area.getHeight() };
}

juce::Point<double> CurveView::toValue (juce::Point<float> pixel) const
{
    const auto area = plotArea();
    return { (double) ((pixel.x - area.getX()) / area.getWidth()) * Curve::maxValue,
             (double) ((area.getBottom() - pixel.y) / area.getHeight()) * Curve::maxValue };
}

bool CurveView::hasHandle (int seg) const
{
    const auto& p = curve.getPoints();
    // на совсем коротком отрезке ручку не показываем — она бы закрыла точки
    return seg >= 0 && seg + 1 < curve.size() && p[(size_t) seg + 1].x - p[(size_t) seg].x >= 4;
}

juce::Point<float> CurveView::handlePosition (int seg) const
{
    const auto& p = curve.getPoints();
    const double x = ((double) p[(size_t) seg].x + (double) p[(size_t) seg + 1].x) * 0.5;
    return toPixel (x, curve.segmentValue (seg, 0.5));
}

int CurveView::findPoint (juce::Point<float> pixel) const
{
    int best = -1;
    float bestDistance = pointHit;

    for (int i = 0; i < curve.size(); ++i)
    {
        const auto& p = curve.getPoints()[(size_t) i];
        const auto distance = toPixel (p.x, p.y).getDistanceFrom (pixel);

        if (distance <= bestDistance)
        {
            best = i;
            bestDistance = distance;
        }
    }

    return best;
}

int CurveView::findHandle (juce::Point<float> pixel) const
{
    for (int seg = 0; seg + 1 < curve.size(); ++seg)
        if (hasHandle (seg) && handlePosition (seg).getDistanceFrom (pixel) <= handleHit)
            return seg;

    return -1;
}

//==============================================================================
void CurveView::changed (bool isFinal)
{
    if (onCurveChanged != nullptr)
        onCurveChanged (isFinal);
}

void CurveView::addPointAt (juce::Point<float> pixel)
{
    const auto v = toValue (pixel);
    const auto index = curve.addPoint (juce::roundToInt (v.x), juce::roundToInt (v.y));

    if (index >= 0)
    {
        repaint();
        changed (true);
        setSelected (index);
    }
}

void CurveView::deletePoint (int index)
{
    if (! curve.removePoint (index))
        return;

    if (selected == index)      selected = -1;
    else if (selected > index)  --selected;

    dragPoint = dragHandle = hoverPoint = hoverHandle = -1;

    repaint();
    changed (true);

    if (onSelectionChanged != nullptr)
        onSelectionChanged();
}

void CurveView::straightenSegment (int seg)
{
    curve.setBend (seg, 0.0);
    dragHandle = -1;
    repaint();
    changed (true);
}

void CurveView::updateHover (juce::Point<float> pixel)
{
    const auto newPoint  = findPoint (pixel);
    const auto newHandle = newPoint < 0 ? findHandle (pixel) : -1;

    if (newPoint != hoverPoint || newHandle != hoverHandle)
    {
        hoverPoint  = newPoint;
        hoverHandle = newHandle;

        if (hoverPoint >= 0)        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        else if (hoverHandle >= 0)  setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        else                        setMouseCursor (juce::MouseCursor::CrosshairCursor);
    }
}

//==============================================================================
void CurveView::mouseEnter (const juce::MouseEvent& e)
{
    mouseInside = true;
    mouseMove (e);
}

void CurveView::mouseExit (const juce::MouseEvent&)
{
    mouseInside = false;
    hoverPoint = hoverHandle = -1;
    repaint();
}

void CurveView::mouseMove (const juce::MouseEvent& e)
{
    mouseInside = true;
    mouseValue = toValue (e.position);
    updateHover (e.position);
    repaint();
}

void CurveView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();

    didDrag = false;
    dragPoint = dragHandle = -1;

    const auto point  = findPoint (e.position);
    const auto handle = point < 0 ? findHandle (e.position) : -1;

    // правая кнопка: добавить / удалить точку, на ручке — выпрямить отрезок
    if (e.mods.isPopupMenu())
    {
        if (point >= 0)        { if (! curve.isEndpoint (point)) deletePoint (point); }
        else if (handle >= 0)  straightenSegment (handle);
        else                   addPointAt (e.position);
        return;
    }

    if (! e.mods.isLeftButtonDown())
        return;

    // двойной щелчок левой кнопкой: добавить / удалить точку, на ручке — выпрямить отрезок
    if (e.getNumberOfClicks() == 2)
    {
        if (point >= 0)        { if (! curve.isEndpoint (point)) deletePoint (point); }
        else if (handle >= 0)  straightenSegment (handle);
        else                   addPointAt (e.position);
        return;
    }

    if (point >= 0)
    {
        dragPoint = point;
        setSelected (point);
    }
    else if (handle >= 0)
    {
        dragHandle = handle;
    }
    else if (selected >= 0)
    {
        setSelected (-1);
    }
}

void CurveView::mouseDrag (const juce::MouseEvent& e)
{
    mouseValue = toValue (e.position);
    const auto v = mouseValue;

    if (dragPoint >= 0)
    {
        int x = juce::roundToInt (v.x);
        int y = juce::roundToInt (v.y);

        if (e.mods.isShiftDown())
        {
            // Shift: точка ходит только по диагонали от 0,0 до 127,127
            const auto range = curve.allowedX (dragPoint);
            const auto d = juce::jlimit (range.getStart(), range.getEnd(),
                                         juce::roundToInt ((v.x + v.y) * 0.5));
            x = y = d;
        }

        curve.movePoint (dragPoint, x, y);
        didDrag = true;
        repaint();
        changed (false);

        if (onSelectionChanged != nullptr)
            onSelectionChanged();
    }
    else if (dragHandle >= 0)
    {
        if (curve.setBendFromHandle (dragHandle, v.y))
        {
            didDrag = true;
            repaint();
            changed (false);
        }
    }
    else
    {
        repaint();
    }
}

void CurveView::mouseUp (const juce::MouseEvent& e)
{
    if (didDrag)
        changed (true);

    didDrag = false;
    dragPoint = dragHandle = -1;
    updateHover (e.position);
    repaint();
}

//==============================================================================
void CurveView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    const auto area = plotArea();

    // сетка 8 x 8, все линии одного цвета
    for (int i = 1; i < 8; ++i)
    {
        const auto x = area.getX() + area.getWidth()  * (float) i / 8.0f;
        const auto y = area.getY() + area.getHeight() * (float) i / 8.0f;

        g.setColour (Theme::grid);
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    }

    g.setColour (Theme::grid);
    g.drawRect (area, 1.0f);

    // кривая
    {
        juce::Path path;
        const auto steps = juce::jmax (64, (int) area.getWidth());

        for (int i = 0; i <= steps; ++i)
        {
            const double x = (double) i / steps * Curve::maxValue;
            const auto pixel = toPixel (x, curve.evaluate (x));

            if (i == 0) path.startNewSubPath (pixel);
            else        path.lineTo (pixel);
        }

        g.setColour (Theme::curve);
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // отметки сыгранных нот: вертикальная черта на пришедшей velocity
    // (точка на кривой рисуется ниже, поверх точек); постепенно гаснут
    if (! markers.empty())
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();

        for (const auto& marker : markers)
        {
            const auto alpha = markerAlpha (marker, now);

            if (alpha <= 0.0f)
                continue;

            const auto top    = toPixel (marker.in, Curve::maxValue);
            const auto bottom = toPixel (marker.in, 0);

            g.setColour (Theme::noteMarker.withAlpha (alpha * 0.85f));
            g.drawLine (top.x, top.y, bottom.x, bottom.y, 2.0f);
        }
    }

    // ручки изгиба посередине отрезков
    for (int seg = 0; seg + 1 < curve.size(); ++seg)
    {
        if (! hasHandle (seg))
            continue;

        const auto pos = handlePosition (seg);
        const bool active = (seg == hoverHandle || seg == dragHandle);
        const bool bent = std::abs (curve.getPoints()[(size_t) seg].bend) > 0.0;
        const auto r = active ? handleRadius + 1.5f : handleRadius;

        // ручка жёлтая; у изогнутого отрезка — светлее
        g.setColour (bent ? Theme::handleBent : Theme::handle);
        g.fillEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);

        g.setColour (active ? juce::Colours::white : Theme::handleOutline);
        g.drawEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f, 1.5f);
    }

    // точки
    for (int i = 0; i < curve.size(); ++i)
    {
        const auto& p = curve.getPoints()[(size_t) i];
        const auto pos = toPixel (p.x, p.y);
        const bool isSelected = (i == selected);
        const bool isHover = (i == hoverPoint || i == dragPoint);
        const auto r = isSelected ? pointRadius + 1.5f : (isHover ? pointRadius + 1.0f : pointRadius);

        g.setColour (isSelected ? Theme::pointSelected : Theme::point);
        g.fillEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);

        g.setColour (isSelected || isHover ? juce::Colours::white : Theme::pointOutline);
        g.drawEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f, 1.6f);
    }

    // отметки сыгранных нот на кривой — поверх точек
    if (! markers.empty())
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();

        for (const auto& marker : markers)
        {
            const auto alpha = markerAlpha (marker, now);

            if (alpha <= 0.0f)
                continue;

            const auto dot = toPixel (marker.in, marker.out);
            g.setColour (Theme::noteMarker.withAlpha (alpha));
            g.fillEllipse (dot.x - 4.5f, dot.y - 4.5f, 9.0f, 9.0f);
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.drawEllipse (dot.x - 4.5f, dot.y - 4.5f, 9.0f, 9.0f, 1.2f);
        }
    }

    // координаты в левом верхнем углу
    juce::String info;

    if (dragPoint >= 0)
    {
        const auto& p = curve.getPoints()[(size_t) dragPoint];
        info << "x: " << p.x << "\ny: " << p.y;
    }
    else if (dragHandle >= 0)
    {
        info << "curve: " << juce::String (curve.getPoints()[(size_t) dragHandle].bend, 2);
    }
    else if (mouseInside)
    {
        info << "x: " << juce::jlimit (0, Curve::maxValue, juce::roundToInt (mouseValue.x))
             << "\ny: " << juce::jlimit (0, Curve::maxValue, juce::roundToInt (mouseValue.y));
    }

    if (info.isNotEmpty())
    {
        g.setColour (Theme::textMuted);
        g.setFont (juce::FontOptions (13.0f));
        g.drawMultiLineText (info, (int) area.getX() + 6, (int) area.getY() + 16, 120);
    }
}
