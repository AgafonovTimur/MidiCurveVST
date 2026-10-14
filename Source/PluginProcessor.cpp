#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiCurveProcessor::MidiCurveProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // язык по умолчанию — как в системе: русский или английский
    const auto systemLanguage = juce::SystemStats::getUserLanguage().toLowerCase();
    language = systemLanguage.startsWith ("ru") ? russian : english;

    rebuildLookup();
}

void MidiCurveProcessor::prepareToPlay (double, int)
{
    processed.ensureSize (4096);
}

bool MidiCurveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    return out == juce::AudioChannelSet::stereo()
        || out == juce::AudioChannelSet::mono();
}

//==============================================================================
void MidiCurveProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    // плагин сам ничего не озвучивает
    buffer.clear();

    processed.clear();

    for (const auto meta : midiMessages)
    {
        const auto* data = meta.data;
        const auto size = meta.numBytes;

        // нажатие ноты (Note On с velocity больше 0) — меняем velocity по кривой.
        // Меньше 1 не отправляем: Note On с velocity 0 DAW приняла бы за отпускание клавиши.
        if (size == 3 && (data[0] & 0xf0) == 0x90 && data[2] > 0)
        {
            const auto inVelocity  = (int) (data[2] & 0x7f);
            const auto outVelocity = (juce::uint8) juce::jmax (1, (int) lookup[(size_t) inVelocity].load (std::memory_order_relaxed));

            const juce::uint8 bytes[3] = { data[0], data[1], outVelocity };
            processed.addEvent (bytes, 3, meta.samplePosition);

            lastIn.store (inVelocity);
            lastOut.store ((int) outVelocity);
            messageCounter.fetch_add (1);
            playedNotes.push ((juce::uint8) inVelocity, outVelocity, 0);
        }
        else
        {
            // всё остальное — без изменений
            processed.addEvent (data, size, meta.samplePosition);
        }
    }

    midiMessages.swapWith (processed);
}

//==============================================================================
Curve MidiCurveProcessor::getCurve() const
{
    const juce::ScopedLock sl (lock);
    return curve;
}

void MidiCurveProcessor::setCurve (const Curve& newCurve, bool notifyHost)
{
    {
        const juce::ScopedLock sl (lock);
        curve = newCurve;
    }

    rebuildLookup();

    if (notifyHost)
        notifyHostAboutChange();
}

void MidiCurveProcessor::rebuildLookup()
{
    std::array<juce::uint8, 128> table {};

    {
        const juce::ScopedLock sl (lock);
        curve.fillLookup (table);
    }

    for (size_t i = 0; i < table.size(); ++i)
        lookup[i].store (table[i], std::memory_order_relaxed);
}

void MidiCurveProcessor::setLanguage (int newLanguage)
{
    language = (newLanguage == english) ? english : russian;
    notifyHostAboutChange();
}

juce::String MidiCurveProcessor::getProfileName() const
{
    const juce::ScopedLock sl (lock);
    return profileName;
}

void MidiCurveProcessor::setProfileName (const juce::String& name)
{
    {
        const juce::ScopedLock sl (lock);
        profileName = name;
    }

    notifyHostAboutChange();
}

void MidiCurveProcessor::setShowPlayedNotes (bool shouldShow)
{
    showPlayedNotes = shouldShow;
    notifyHostAboutChange();
}

void MidiCurveProcessor::notifyHostAboutChange()
{
    // DAW должна понять, что проект изменился и его надо сохранить
    updateHostDisplay (juce::AudioProcessorListener::ChangeDetails {}
                           .withNonParameterStateChanged (true));
}

//==============================================================================
juce::AudioProcessorEditor* MidiCurveProcessor::createEditor()
{
    return new MidiCurveEditor (*this);
}

void MidiCurveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("MidiCurveVSTState");

    state.setProperty ("curve", getCurve().toJson ({}), nullptr);
    state.setProperty ("profile", getProfileName(), nullptr);
    state.setProperty ("language", getLanguage(), nullptr);
    state.setProperty ("scale", editorScale, nullptr);
    state.setProperty ("showPlayedNotes", showPlayedNotes.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiCurveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const auto state = juce::ValueTree::fromXml (*xml);

        if (! state.isValid())
            return;

        Curve loaded;
        juce::String detail;

        if (Curve::fromJson (state.getProperty ("curve", "").toString(), loaded, detail) == Curve::ParseError::none)
        {
            const juce::ScopedLock sl (lock);
            curve = loaded;
        }

        {
            const juce::ScopedLock sl (lock);
            profileName = state.getProperty ("profile", "").toString();
        }

        language = ((int) state.getProperty ("language", (int) language.load()) == english) ? english : russian;
        showPlayedNotes = (bool) state.getProperty ("showPlayedNotes", false);
        editorScale = juce::jlimit (0.5, 3.0, (double) state.getProperty ("scale", editorScale));

        rebuildLookup();

        // окно плагина могло быть открыто раньше, чем DAW прислала настройки
        stateRevision.fetch_add (1);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiCurveProcessor();
}
