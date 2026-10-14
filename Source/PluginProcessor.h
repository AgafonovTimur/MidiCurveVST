#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

#include "CurveModel.h"

//==============================================================================
//  Очередь коротких сообщений из потока звука в окно плагина.
//  Пишет всегда один поток, читает другой, поэтому блокировки не нужны.
//==============================================================================
class SmallQueue
{
public:
    SmallQueue() { storage.resize ((size_t) capacity); }

    void push (juce::uint8 b0, juce::uint8 b1, juce::uint8 b2)
    {
        const auto scope = fifo.write (1);

        if (scope.blockSize1 > 0)
            storage[(size_t) scope.startIndex1] = { b0, b1, b2 };
        else if (scope.blockSize2 > 0)
            storage[(size_t) scope.startIndex2] = { b0, b1, b2 };
        // очередь переполнена — сообщение теряется, это лучше, чем ждать в потоке звука
    }

    bool pop (std::array<juce::uint8, 3>& result)
    {
        const auto scope = fifo.read (1);

        if (scope.blockSize1 > 0) { result = storage[(size_t) scope.startIndex1]; return true; }
        if (scope.blockSize2 > 0) { result = storage[(size_t) scope.startIndex2]; return true; }

        return false;
    }

private:
    static constexpr int capacity = 512;

    juce::AbstractFifo fifo { capacity };
    std::vector<std::array<juce::uint8, 3>> storage;
};

//==============================================================================
//  MidiCurveVST: меняет velocity нажатых нот по кривой.
//  Остальные MIDI-сообщения (отпускания клавиш, CC, pitch bend и т.д.)
//  проходят без изменений.
//==============================================================================
class MidiCurveProcessor : public juce::AudioProcessor
{
public:
    enum Language { russian = 0, english = 1 };

    MidiCurveProcessor();
    ~MidiCurveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    Curve getCurve() const;
    // notifyHost = true — сообщить DAW, что проект изменился
    void setCurve (const Curve& newCurve, bool notifyHost);

    int  getLanguage() const noexcept { return language.load(); }
    void setLanguage (int newLanguage);

    juce::String getProfileName() const;
    void setProfileName (const juce::String& name);

    // последняя обработанная нота: velocity до и после кривой (-1 — ещё не было)
    int getLastIn() const noexcept       { return lastIn.load(); }
    int getLastOut() const noexcept      { return lastOut.load(); }
    int getMessageCounter() const noexcept { return messageCounter.load(); }

    // сыгранные ноты для отметки на поле: [0] — пришедшая velocity, [1] — ушедшая
    bool popPlayedNote (std::array<juce::uint8, 3>& note) { return playedNotes.pop (note); }

    // галка «Показывать нажатые ноты»
    bool getShowPlayedNotes() const noexcept { return showPlayedNotes.load(); }
    void setShowPlayedNotes (bool shouldShow);

    // растёт, когда DAW загрузила настройки из проекта
    int getStateRevision() const noexcept { return stateRevision.load(); }

    // масштаб окна плагина — сохраняется вместе с проектом
    double editorScale = 1.0;

private:
    void rebuildLookup();
    void notifyHostAboutChange();

    mutable juce::CriticalSection lock;
    Curve curve;
    juce::String profileName;

    std::array<std::atomic<juce::uint8>, 128> lookup;
    juce::MidiBuffer processed;

    std::atomic<int> language { russian };
    std::atomic<int> lastIn { -1 }, lastOut { -1 }, messageCounter { 0 };
    std::atomic<int> stateRevision { 0 };
    std::atomic<bool> showPlayedNotes { false };

    SmallQueue playedNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiCurveProcessor)
};
