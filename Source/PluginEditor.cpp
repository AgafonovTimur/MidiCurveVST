#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    // разбивка панели справа (в логических пикселях)
    constexpr int panelPadding = 12;

    void setupLabel (juce::Label& label, float size, bool bold,
                     juce::Justification justification, juce::Colour colour = Theme::text)
    {
        label.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        label.setJustificationType (justification);
        label.setColour (juce::Label::textColourId, colour);
        label.setInterceptsMouseClicks (false, false);
        label.setMinimumHorizontalScale (1.0f);
        label.setBorderSize ({ 0, 0, 0, 0 });
    }

    void setupNumberField (juce::TextEditor& editor)
    {
        editor.setInputRestrictions (3, "0123456789");
        editor.setJustification (juce::Justification::centred);
        editor.setFont (juce::FontOptions (14.0f));
        editor.setSelectAllWhenFocused (true);
        editor.setIndents (4, 4);
    }
}

//==============================================================================
//  Справка
//==============================================================================
HelpOverlay::HelpOverlay()
{
    viewport.setViewedComponent (&body, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);

    closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closeButton);
}

void HelpOverlay::setTexts (const juce::String& helpText, const juce::String& closeText)
{
    body.setText (helpText);
    closeButton.setButtonText (closeText);
    resized();
}

void HelpOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));

    const auto box = boxBounds().toFloat();
    g.setColour (juce::Colour (0xff212121));
    g.fillRoundedRectangle (box, 8.0f);
    g.setColour (Theme::panelBorder);
    g.drawRoundedRectangle (box, 8.0f, 1.0f);
}

void HelpOverlay::resized()
{
    auto box = boxBounds().reduced (6);
    closeButton.setBounds (box.removeFromBottom (32).withSizeKeepingCentre (140, 30));
    box.removeFromBottom (6);
    viewport.setBounds (box);
    body.layoutForWidth (box.getWidth() - viewport.getScrollBarThickness());
}

void HelpOverlay::visibilityChanged()
{
    // справка всегда открывается с начала
    if (isVisible())
        viewport.setViewPosition (0, 0);
}

void HelpOverlay::mouseDown (const juce::MouseEvent& e)
{
    // щелчок мимо окна справки закрывает её
    if (! boxBounds().contains (e.getPosition()))
        setVisible (false);
}

void HelpOverlay::Body::setText (const juce::String& text)
{
    attributed.clear();
    attributed.setWordWrap (juce::AttributedString::byWord);

    juce::StringArray lines;
    lines.addLines (text);

    for (int i = 0; i < lines.size(); ++i)
    {
        auto line = lines[i];
        const bool isHeading = line.startsWith ("# ");

        if (isHeading)
            line = line.substring (2);

        attributed.append (line + (i + 1 < lines.size() ? "\n" : ""),
                           juce::FontOptions (isHeading ? 15.0f : 14.5f, isHeading ? juce::Font::bold : juce::Font::plain),
                           isHeading ? juce::Colours::white : Theme::text);
    }
}

void HelpOverlay::Body::layoutForWidth (int width)
{
    width = juce::jmax (100, width);
    layout.createLayout (attributed, (float) (width - padding * 2));
    setSize (width, (int) std::ceil (layout.getHeight()) + padding * 2);
    repaint();
}

void HelpOverlay::Body::paint (juce::Graphics& g)
{
    layout.draw (g, getLocalBounds().reduced (padding).toFloat());
}

//==============================================================================
//  Содержимое окна
//==============================================================================
EditorContent::EditorContent (MidiCurveProcessor& p)
    : processor (p)
{
    addAndMakeVisible (curveView);

    curveView.onCurveChanged = [this] (bool isFinal)
    {
        processor.setCurve (curveView.getCurve(), isFinal);
        updatePointFields();
    };

    curveView.onSelectionChanged = [this] { updatePointFields(); };

    // --- Last Message ---
    setupLabel (lastTitle, 15.0f, true, juce::Justification::centred);
    setupLabel (inLabel,  14.0f, false, juce::Justification::centredLeft);
    setupLabel (outLabel, 14.0f, false, juce::Justification::centredLeft);

    // --- Точка ---
    setupLabel (pointTitle, 14.0f, true, juce::Justification::centredLeft);
    setupLabel (pointHint, 12.0f, false, juce::Justification::centredLeft, Theme::textMuted);
    setupLabel (xCaption, 14.0f, false, juce::Justification::centred);
    setupLabel (yCaption, 14.0f, false, juce::Justification::centred);
    xCaption.setText ("X", juce::dontSendNotification);
    yCaption.setText ("Y", juce::dontSendNotification);

    for (auto* field : { &xEdit, &yEdit })
    {
        setupNumberField (*field);
        field->onReturnKey = [this] { applyPointField(); };
        field->onFocusLost = [this] { applyPointField(); };
        field->onEscapeKey = [this] { updatePointFields(); };
    }

    // --- Профиль ---
    setupLabel (profileTitle, 14.0f, true, juce::Justification::centredLeft);

    profileButton.onClick     = [this] { showProfileMenu(); };
    saveCurrentButton.onClick = [this] { saveCurrentCurve(); };
    saveAsButton.onClick      = [this] { askNameAndSave(); };
    importButton.onClick      = [this] { importProfile(); };
    exportButton.onClick      = [this] { exportCurve(); };
    folderButton.onClick      = [] { profilesFolder().startAsProcess(); };

    // --- Reset / Help ---
    resetButton.onClick = [this]
    {
        curveView.setCurve (Curve());
        curveView.setSelected (-1);
        processor.setCurve (curveView.getCurve(), true);
        updatePointFields();
    };

    helpButton.onClick = [this]
    {
        help.setVisible (true);
        help.toFront (true);
    };

    // --- Язык ---
    russianButton.setButtonText (juce::String::fromUTF8 ("Русский"));
    englishButton.setButtonText ("English");

    for (auto* b : { &russianButton, &englishButton })
    {
        b->setClickingTogglesState (false);
        b->setRadioGroupId (1001);
    }

    russianButton.onClick = [this] { processor.setLanguage (MidiCurveProcessor::russian); applyLanguage(); };
    englishButton.onClick = [this] { processor.setLanguage (MidiCurveProcessor::english); applyLanguage(); };

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &lastTitle, &inLabel, &outLabel,
             &pointTitle, &pointHint, &xCaption, &yCaption, &xEdit, &yEdit,
             &profileTitle, &profileButton, &saveCurrentButton, &saveAsButton,
             &importButton, &exportButton, &folderButton,
             &resetButton, &helpButton, &russianButton, &englishButton })
        addAndMakeVisible (c);

    addChildComponent (help);

    // путь к папке профилей; после каждой «\» можно переносить строку
    const auto breakHere = juce::String::charToString ((juce::juce_wchar) 0x200B);
    pathText = profilesFolder().getFullPathName()
                   .replace ("\\", "\\" + breakHere)
                   .replace ("/", "/" + breakHere);

    reloadFromProcessor();
    refreshLastMessage();
}

EditorContent::~EditorContent()
{
    // открытые окна (сохранение, вопросы) закрываем вместе с плагином
    if (auto* modal = juce::ModalComponentManager::getInstance())
        modal->cancelAllModalComponents();

    chooser.reset();
}

//==============================================================================
void EditorContent::reloadFromProcessor()
{
    curveView.setCurve (processor.getCurve());
    curveView.setSelected (-1);
    applyLanguage();
}

void EditorContent::applyLanguage()
{
    const auto language = processor.getLanguage();
    texts = Texts::get (language);

    russianButton.setToggleState (language == MidiCurveProcessor::russian, juce::dontSendNotification);
    englishButton.setToggleState (language == MidiCurveProcessor::english, juce::dontSendNotification);

    lastTitle.setText (texts.lastMessage, juce::dontSendNotification);
    pointTitle.setText (texts.point, juce::dontSendNotification);
    pointHint.setText (texts.pointHint, juce::dontSendNotification);
    profileTitle.setText (texts.profile, juce::dontSendNotification);

    saveCurrentButton.setButtonText (texts.saveCurrent);
    saveAsButton.setButtonText (texts.saveAs);
    importButton.setButtonText (texts.import);
    exportButton.setButtonText (texts.exportCurve);
    folderButton.setButtonText (texts.folder);
    resetButton.setButtonText (texts.reset);
    helpButton.setButtonText (texts.help);

    help.setTexts (texts.helpText, texts.close);

    lastCounter = -1;
    refreshLastMessage();
    updateProfileButton();
    updatePointFields();
    repaint();
}

void EditorContent::refreshLastMessage()
{
    const auto counter = processor.getMessageCounter();

    if (counter == lastCounter)
        return;

    lastCounter = counter;

    const auto in  = processor.getLastIn();
    const auto out = processor.getLastOut();

    inLabel.setText  (texts.in  + ": " + (in  >= 0 ? juce::String (in)  : juce::String ("--")), juce::dontSendNotification);
    outLabel.setText (texts.out + ": " + (out >= 0 ? juce::String (out) : juce::String ("--")), juce::dontSendNotification);
}

//==============================================================================
void EditorContent::updatePointFields()
{
    const auto index = curveView.getSelected();
    const auto& points = curveView.getCurve().getPoints();

    if (index < 0 || index >= (int) points.size())
    {
        for (auto* field : { &xEdit, &yEdit })
        {
            field->setText ({}, false);
            field->setEnabled (false);
        }

        pointHint.setVisible (true);
        return;
    }

    const auto& p = points[(size_t) index];

    if (! xEdit.hasKeyboardFocus (true)) xEdit.setText (juce::String (p.x), false);
    if (! yEdit.hasKeyboardFocus (true)) yEdit.setText (juce::String (p.y), false);

    // у крайних точек X закреплён
    xEdit.setEnabled (! curveView.getCurve().isEndpoint (index));
    yEdit.setEnabled (true);
    pointHint.setVisible (false);
}

void EditorContent::applyPointField()
{
    const auto index = curveView.getSelected();
    const auto& points = curveView.getCurve().getPoints();

    if (index < 0 || index >= (int) points.size())
        return;

    const auto& p = points[(size_t) index];
    const auto x = xEdit.getText().trim().isEmpty() ? p.x : xEdit.getText().getIntValue();
    const auto y = yEdit.getText().trim().isEmpty() ? p.y : yEdit.getText().getIntValue();

    if (x != p.x || y != p.y)
        curveView.setPointPosition (index, x, y);

    // показываем то, что получилось (например, если X упёрся в соседнюю точку)
    const auto& moved = curveView.getCurve().getPoints()[(size_t) index];
    xEdit.setText (juce::String (moved.x), false);
    yEdit.setText (juce::String (moved.y), false);
}

void EditorContent::updateProfileButton()
{
    const auto name = processor.getProfileName();
    profileButton.setButtonText ((name.isEmpty() ? texts.noProfile : name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"));
}

//==============================================================================
//  Профили
//==============================================================================
juce::File EditorContent::profilesFolder()
{
    auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                      .getChildFile ("MidiCurveVST")
                      .getChildFile ("Profiles");

    folder.createDirectory();
    return folder;
}

juce::Array<juce::File> EditorContent::listProfiles() const
{
    auto files = profilesFolder().findChildFiles (juce::File::findFiles, false, "*.json");

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0;
    });

    return files;
}

void EditorContent::showProfileMenu()
{
    // список перечитывается при каждом открытии — новые файлы видны сразу
    const auto files = listProfiles();
    const auto current = processor.getProfileName();

    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());

    if (files.isEmpty())
        menu.addItem (-1, texts.emptyFolder, false);

    for (int i = 0; i < files.size(); ++i)
    {
        const auto name = files[i].getFileNameWithoutExtension();
        menu.addItem (i + 1, name, true, name == current);
    }

    juce::Component::SafePointer<EditorContent> safe (this);

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&profileButton)
                            .withMinimumWidth (profileButton.getWidth()),
                        [safe, files] (int result)
                        {
                            // выбор профиля, даже того же самого, заново читает файл
                            if (safe != nullptr && result > 0 && result <= files.size())
                                safe->loadProfileFile (files[result - 1], false);
                        });
}

juce::String EditorContent::describeParseError (Curve::ParseError error, const juce::String& detail,
                                                const juce::File& file) const
{
    juce::String message;

    switch (error)
    {
        case Curve::ParseError::badJson:  message = texts.badJson.replace ("%1", detail); break;
        case Curve::ParseError::noPoints: message = texts.noPoints; break;
        case Curve::ParseError::badPoint: message = texts.badPoint.replace ("%1", detail); break;
        case Curve::ParseError::none:     break;
    }

    return texts.readError.replace ("%1", file.getFileName()) + "\n\n" + message;
}

bool EditorContent::loadProfileFile (const juce::File& file, bool)
{
    if (! file.existsAsFile())
    {
        showError (texts.readError.replace ("%1", file.getFileName()));
        return false;
    }

    Curve loaded;
    juce::String detail;
    const auto error = Curve::fromJson (file.loadFileAsString(), loaded, detail);

    if (error != Curve::ParseError::none)
    {
        showError (describeParseError (error, detail, file));
        return false;
    }

    curveView.setCurve (loaded);
    curveView.setSelected (-1);
    processor.setCurve (loaded, true);
    processor.setProfileName (file.getFileNameWithoutExtension());

    updateProfileButton();
    updatePointFields();
    return true;
}

bool EditorContent::writeCurveTo (const juce::File& file)
{
    if (file.replaceWithText (curveView.getCurve().toJson (texts.jsonHelp)))
        return true;

    showError (texts.writeError.replace ("%1", file.getFullPathName()));
    return false;
}

void EditorContent::saveCurrentCurve()
{
    const auto name = processor.getProfileName();

    if (name.isEmpty())
    {
        askNameAndSave();
        return;
    }

    writeCurveTo (profilesFolder().getChildFile (name + ".json"));
}

void EditorContent::askNameAndSave()
{
    auto* window = new juce::AlertWindow (texts.saveAsTitle, texts.profileName,
                                          juce::MessageBoxIconType::NoIcon, this);
    window->setLookAndFeel (&getLookAndFeel());
    window->addTextEditor ("name", processor.getProfileName());
    window->addButton (texts.ok,     1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton (texts.cancel, 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<EditorContent> safe (this);
    juce::Component::SafePointer<juce::AlertWindow> safeWindow (window);

    window->enterModalState (true, juce::ModalCallbackFunction::create ([safe, safeWindow] (int result)
    {
        if (safe == nullptr || safeWindow == nullptr || result != 1)
            return;

        const auto name = juce::File::createLegalFileName (safeWindow->getTextEditorContents ("name").trim());

        if (name.isNotEmpty())
            safe->saveToProfileName (name);
    }), true);
}

void EditorContent::saveToProfileName (const juce::String& name)
{
    const auto file = profilesFolder().getChildFile (name + ".json");

    auto save = [this, file, name]
    {
        if (writeCurveTo (file))
        {
            processor.setProfileName (name);
            updateProfileButton();
        }
    };

    if (file.existsAsFile() && name != processor.getProfileName())
        confirmReplace (name, save);
    else
        save();
}

void EditorContent::importProfile()
{
    chooser = std::make_unique<juce::FileChooser> (texts.importTitle,
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.json");

    juce::Component::SafePointer<EditorContent> safe (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;

        const auto source = fc.getResult();

        if (source == juce::File())
            return;

        // сначала проверяем, что файл читается, и только потом копируем
        Curve test;
        juce::String detail;
        const auto error = Curve::fromJson (source.loadFileAsString(), test, detail);

        if (error != Curve::ParseError::none)
        {
            safe->showError (safe->describeParseError (error, detail, source));
            return;
        }

        const auto target = profilesFolder().getChildFile (source.getFileNameWithoutExtension() + ".json");

        auto copyAndLoad = [safe, source, target]
        {
            if (safe == nullptr)
                return;

            if (source != target && ! source.copyFileTo (target))
            {
                safe->showError (safe->texts.writeError.replace ("%1", target.getFullPathName()));
                return;
            }

            safe->loadProfileFile (target, false);
        };

        if (source != target && target.existsAsFile())
            safe->confirmReplace (target.getFileNameWithoutExtension(), copyAndLoad);
        else
            copyAndLoad();
    });
}

void EditorContent::exportCurve()
{
    const auto name = processor.getProfileName();
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ((name.isEmpty() ? juce::String ("curve") : name) + ".json");

    chooser = std::make_unique<juce::FileChooser> (texts.exportTitle, start, "*.json");

    juce::Component::SafePointer<EditorContent> safe (this);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;

        auto file = fc.getResult();

        if (file == juce::File())
            return;

        if (! file.hasFileExtension ("json"))
            file = file.withFileExtension ("json");

        safe->writeCurveTo (file);
    });
}

void EditorContent::confirmReplace (const juce::String& name, std::function<void()> onYes)
{
    juce::Component::SafePointer<EditorContent> safe (this);

    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle (texts.replaceTitle)
                                      .withMessage (texts.replaceMessage.replace ("%1", name))
                                      .withButton (texts.yes)
                                      .withButton (texts.no)
                                      .withAssociatedComponent (this),
                                  [safe, onYes] (int result)
                                  {
                                      // при двух кнопках первая возвращает 1
                                      if (safe != nullptr && result == 1 && onYes != nullptr)
                                          onYes();
                                  });
}

void EditorContent::showError (const juce::String& message)
{
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::WarningIcon)
                                      .withTitle (texts.errorTitle)
                                      .withMessage (message)
                                      .withButton (texts.ok)
                                      .withAssociatedComponent (this),
                                  nullptr);
}

//==============================================================================
void EditorContent::paint (juce::Graphics& g)
{
    const auto panel = getLocalBounds().withLeft (logicalHeight).toFloat();

    g.setColour (Theme::panel);
    g.fillRect (panel);

    g.setColour (Theme::panelBorder);
    g.drawLine (panel.getX() + 0.5f, 0.0f, panel.getX() + 0.5f, (float) getHeight(), 1.0f);

    // разделители между блоками
    const auto left  = panel.getX() + (float) panelPadding;
    const auto right = panel.getRight() - (float) panelPadding;

    for (const auto y : { 84.0f, 172.0f, 438.0f })
        g.drawLine (left, y, right, y, 1.0f);

    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byChar);
    text.setJustification (juce::Justification::topLeft);
    text.append (texts.profilesPath + "\n", juce::FontOptions (12.0f), Theme::textMuted);
    text.append (pathText, juce::FontOptions (11.5f), Theme::textMuted);
    text.draw (g, pathArea);
}

void EditorContent::resized()
{
    curveView.setBounds (0, 0, logicalHeight, logicalHeight);
    help.setBounds (getLocalBounds());

    const int x = logicalHeight + panelPadding;
    const int w = panelWidth - panelPadding * 2;
    const int half = (w - 6) / 2;

    // Last Message
    lastTitle.setBounds (x, 10, w, 22);
    inLabel.setBounds   (x, 36, w, 20);
    outLabel.setBounds  (x, 56, w, 20);

    // Точка
    pointTitle.setBounds (x, 92, w, 20);
    xCaption.setBounds (x, 118, 20, 26);
    xEdit.setBounds    (x + 22, 118, half - 22, 26);
    yCaption.setBounds (x + half + 6, 118, 20, 26);
    yEdit.setBounds    (x + half + 28, 118, half - 22, 26);
    pointHint.setBounds (x, 146, w, 18);

    // Профиль
    profileTitle.setBounds      (x, 180, w, 20);
    profileButton.setBounds     (x, 204, w, 28);
    saveCurrentButton.setBounds (x, 238, w, 28);
    saveAsButton.setBounds      (x, 270, w, 28);
    importButton.setBounds      (x, 302, half, 28);
    exportButton.setBounds      (x + half + 6, 302, half, 28);
    folderButton.setBounds      (x, 334, w, 28);
    pathArea = juce::Rectangle<float> ((float) x, 368.0f, (float) w, 64.0f);

    // Reset / Help
    resetButton.setBounds (x, 448, half, 28);
    helpButton.setBounds  (x + half + 6, 448, half, 28);

    // язык — внизу справа, над уголком растягивания
    const int langY = logicalHeight - 20 - 30;
    russianButton.setBounds (x, langY, half, 30);
    englishButton.setBounds (x + half + 6, langY, half, 30);
}

//==============================================================================
//  Окно плагина
//==============================================================================
MidiCurveEditor::MidiCurveEditor (MidiCurveProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      content (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    const auto scale = juce::jlimit (0.5, 2.5, processor.editorScale);

    // окно тянется за правый нижний угол, пропорции всегда сохраняются
    setResizable (true, true);
    setResizeLimits (EditorContent::logicalWidth / 2, EditorContent::logicalHeight / 2,
                     EditorContent::logicalWidth * 5 / 2, EditorContent::logicalHeight * 5 / 2);

    if (auto* sizeLimits = getConstrainer())
        sizeLimits->setFixedAspectRatio ((double) EditorContent::logicalWidth / EditorContent::logicalHeight);

    setSize (juce::roundToInt (EditorContent::logicalWidth * scale),
             juce::roundToInt (EditorContent::logicalHeight * scale));

    lastStateRevision = processor.getStateRevision();
    ready = true;

    startTimerHz (30);
}

MidiCurveEditor::~MidiCurveEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MidiCurveEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void MidiCurveEditor::resized()
{
    const auto scale = (float) getWidth() / (float) EditorContent::logicalWidth;

    content.setBounds (0, 0, EditorContent::logicalWidth, EditorContent::logicalHeight);
    content.setTransform (juce::AffineTransform::scale (scale));

    if (ready)
        processor.editorScale = scale;
}

void MidiCurveEditor::timerCallback()
{
    const auto revision = processor.getStateRevision();

    // DAW загрузила проект уже после открытия окна
    if (revision != lastStateRevision)
    {
        lastStateRevision = revision;
        content.reloadFromProcessor();
    }

    content.refreshLastMessage();
}
