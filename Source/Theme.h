#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
//  Цвета — тёмная тема из Ear Trainer (панель настроек, тема «Тёмная»)
//==============================================================================
namespace Theme
{
    inline const juce::Colour background   { 0xff181818 };   // поле с точками
    inline const juce::Colour panel        { 0xff1e1e1e };   // правая панель
    inline const juce::Colour panelBorder  { 0xff333333 };
    inline const juce::Colour grid         { 0xff2c2c2c };
    inline const juce::Colour centreLine   { 0xff4a4a4a };
    inline const juce::Colour button       { 0xff333333 };
    inline const juce::Colour buttonHover  { 0xff3d3d3d };
    inline const juce::Colour buttonActive { 0xff5a5a5a };
    inline const juce::Colour text         { 0xffd0d0d0 };
    inline const juce::Colour textMuted    { 0xff7d7d7d };
    inline const juce::Colour textHint     { 0xff5c5c5c };
    inline const juce::Colour curve        { 0xffd0d0d0 };
    inline const juce::Colour menu         { 0xff2a2a2a };
    inline const juce::Colour menuHover    { 0xff424242 };
}

//==============================================================================
class MidiCurveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MidiCurveLookAndFeel()
        : juce::LookAndFeel_V4 (ColourScheme (juce::Colour (0xff212121),   // фон окон
                                              Theme::button,              // фон элементов
                                              Theme::menu,                // фон меню
                                              Theme::panelBorder,         // обводка
                                              Theme::text,                // текст
                                              Theme::buttonActive,        // заливка
                                              juce::Colours::white,       // выделенный текст
                                              Theme::menuHover,           // выделение
                                              Theme::text))               // текст меню
    {
       #if JUCE_WINDOWS
        setDefaultSansSerifTypefaceName ("Segoe UI");
       #endif

        setColour (juce::TextButton::buttonColourId,   Theme::button);
        setColour (juce::TextButton::buttonOnColourId, Theme::buttonActive);
        setColour (juce::TextButton::textColourOffId,  Theme::text);
        setColour (juce::TextButton::textColourOnId,   Theme::text);

        setColour (juce::Label::textColourId, Theme::text);

        setColour (juce::TextEditor::backgroundColourId,     Theme::button);
        setColour (juce::TextEditor::textColourId,           Theme::text);
        setColour (juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, Theme::buttonActive);
        setColour (juce::TextEditor::highlightColourId,      Theme::buttonActive);
        setColour (juce::CaretComponent::caretColourId,      Theme::text);

        setColour (juce::PopupMenu::backgroundColourId,            Theme::menu);
        setColour (juce::PopupMenu::textColourId,                  Theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::menuHover);
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff212121));
        setColour (juce::AlertWindow::textColourId,       Theme::text);
        setColour (juce::AlertWindow::outlineColourId,    Theme::panelBorder);

        setColour (juce::ScrollBar::thumbColourId, Theme::buttonActive);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

        auto colour = button.getToggleState() ? Theme::buttonActive : Theme::button;

        if (! button.isEnabled())
            colour = colour.withMultipliedAlpha (0.5f);
        else if (isDown)
            colour = Theme::buttonActive;
        else if (isHighlighted && ! button.getToggleState())
            colour = Theme::buttonHover;

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 6.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override
    {
        const auto height = juce::jmin (14.0f, (float) buttonHeight * 0.55f);
        return juce::FontOptions (height, button.getToggleState() ? juce::Font::bold : juce::Font::plain);
    }

    juce::AlertWindow* createAlertWindow (const juce::String& title, const juce::String& message,
                                          const juce::String& button1, const juce::String& button2,
                                          const juce::String& button3, juce::MessageBoxIconType iconType,
                                          int numButtons, juce::Component* associatedComponent) override
    {
        auto* window = juce::LookAndFeel_V4::createAlertWindow (title, message, button1, button2, button3,
                                                                iconType, numButtons, associatedComponent);
        window->setLookAndFeel (this);
        return window;
    }

    void drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        const auto size = (float) juce::jmin (w, h);
        g.setColour (isMouseOver || isMouseDragging ? Theme::text : Theme::textMuted);

        for (float i = 0.3f; i < 1.0f; i += 0.3f)
            g.drawLine ((float) w - size * i, (float) h, (float) w, (float) h - size * i, 1.2f);
    }
};
