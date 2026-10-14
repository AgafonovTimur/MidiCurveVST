#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

#include "CurveModel.h"

//==============================================================================
//  Квадратное поле с кривой: точки, отрезки, ручки изгиба.
//==============================================================================
class CurveView : public juce::Component
{
public:
    CurveView();

    void setCurve (const Curve& newCurve);
    const Curve& getCurve() const noexcept { return curve; }

    int  getSelected() const noexcept { return selected; }
    void setSelected (int index);

    // ввод координат вручную из правой панели
    void setPointPosition (int index, int x, int y);

    // isFinal = false — точку ещё тянут мышью
    std::function<void (bool isFinal)> onCurveChanged;
    std::function<void()> onSelectionChanged;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> plotArea() const;
    juce::Point<float>  toPixel (double x, double y) const;
    juce::Point<double> toValue (juce::Point<float> pixel) const;

    juce::Point<float> handlePosition (int seg) const;
    bool hasHandle (int seg) const;

    int findPoint (juce::Point<float> pixel) const;
    int findHandle (juce::Point<float> pixel) const;

    void addPointAt (juce::Point<float> pixel);
    void deletePoint (int index);
    void straightenSegment (int seg);
    void updateHover (juce::Point<float> pixel);
    void changed (bool isFinal);

    Curve curve;

    int selected    = -1;
    int dragPoint   = -1;
    int dragHandle  = -1;
    int hoverPoint  = -1;
    int hoverHandle = -1;
    bool didDrag    = false;

    bool mouseInside = false;
    juce::Point<double> mouseValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveView)
};
