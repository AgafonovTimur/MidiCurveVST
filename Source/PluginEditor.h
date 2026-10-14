#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "CurveView.h"
#include "Theme.h"
#include "Texts.h"

//==============================================================================
//  Окно справки поверх плагина
//==============================================================================
class HelpOverlay : public juce::Component
{
public:
    HelpOverlay();

    void setTexts (const juce::String& helpText, const juce::String& closeText);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void visibilityChanged() override;

private:
    // текст справки: заголовки жирным, прокрутка колесом
    class Body : public juce::Component
    {
    public:
        void setText (const juce::String& text);
        void layoutForWidth (int width);
        void paint (juce::Graphics&) override;

    private:
        juce::AttributedString attributed;
        juce::TextLayout layout;
        static constexpr int padding = 14;
    };

    juce::Rectangle<int> boxBounds() const { return getLocalBounds().reduced (40, 28); }

    juce::Viewport viewport;
    Body body;
    juce::TextButton closeButton;
};

//==============================================================================
//  Всё содержимое окна в «логическом» размере 780 x 560.
//  При растягивании окна оно целиком масштабируется.
//==============================================================================
class EditorContent : public juce::Component
{
public:
    static constexpr int logicalWidth  = 780;
    static constexpr int logicalHeight = 560;
    static constexpr int panelWidth    = logicalWidth - logicalHeight;   // поле с точками — квадрат

    explicit EditorContent (MidiCurveProcessor&);
    ~EditorContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // вызывается таймером окна
    void refreshLastMessage();
    void pollPlayedNotes();
    void reloadFromProcessor();

private:
    void applyLanguage();
    void updatePointFields();
    void applyPointField();
    void updateProfileButton();

    // профили
    static juce::File profilesFolder();
    juce::Array<juce::File> listProfiles() const;
    void showProfileMenu();
    bool loadProfileFile (const juce::File& file, bool copyIntoFolder);
    bool writeCurveTo (const juce::File& file);
    void saveCurrentCurve();
    void askNameAndSave();
    void saveToProfileName (const juce::String& name);
    void importProfile();
    void exportCurve();
    void confirmReplace (const juce::String& name, std::function<void()> onYes);
    void showError (const juce::String& message);
    juce::String describeParseError (Curve::ParseError error, const juce::String& detail,
                                     const juce::File& file) const;

    MidiCurveProcessor& processor;
    Texts texts;

    CurveView curveView;

    juce::Label lastTitle, inLabel, outLabel;
    juce::Label pointTitle, pointHint, xCaption, yCaption;
    juce::TextEditor xEdit, yEdit;
    juce::ToggleButton showNotesToggle;

    juce::Label profileTitle;
    juce::TextButton profileButton, saveCurrentButton, saveAsButton,
                     importButton, exportButton, folderButton;
    juce::Rectangle<float> pathArea;   // подсказка с путём к папке профилей
    juce::String pathText;

    juce::TextButton resetButton, helpButton;
    juce::TextButton russianButton, englishButton;

    HelpOverlay help;

    std::unique_ptr<juce::FileChooser> chooser;
    int lastCounter = -1;

    JUCE_DECLARE_WEAK_REFERENCEABLE (EditorContent)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorContent)
};

//==============================================================================
class MidiCurveEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit MidiCurveEditor (MidiCurveProcessor&);
    ~MidiCurveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MidiCurveProcessor& processor;
    MidiCurveLookAndFeel lookAndFeel;
    EditorContent content;

    int lastStateRevision = 0;
    bool ready = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiCurveEditor)
};
