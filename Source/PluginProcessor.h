#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Tempo-syncbares Stereo-Echo mit zwei Betriebsmodi (Ping-Pong, Dual).

    Ping-Pong: Zwei unabhaengige Ping-Pong-Kanaele (Zeile 1 = linker Eingang,
    Zeile 2 = rechter Eingang). Der Ping-Pong-Rahmen wird von den beiden
    Pan-Reglern aufgespannt: Jedes Echo liegt abwechselnd auf Position pan1
    und Position pan2 (Voice 1 startet bei pan1, Voice 2 bei pan2);
    Feedback steuert das Abklingen.

    Dual: Zwei unabhaengige Delays mit pro Zeile eigener Notenart/Betonung
    bzw. ms-Time, Pan-Position und Feedback.

    Sync an  -> Notenart/Betonung bestimmen die Delayzeit pro Zeile.
    Sync aus -> pro Zeile eine eigene ms-Time (Betonung wirkt weiterhin).
    Globale Regler: Sync (ON/OFF-Switcher) und Dry/Wet
    (0 % = trocken, 100 % = nur Echo). Kein Bypass.
*/
class EchoAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    EchoAudioProcessor();
    ~EchoAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Parameter-Zustand. Public, damit der Editor ihn an die Regler binden kann. */
    juce::AudioProcessorValueTreeState params;

    /** Vom Host geliefertes Tempo in BPM (atomar, Editor liest es per Timer). */
    std::atomic<int> bpm { 120 };

    /** Vollpfad des aktuell geladenen Presets (leer = keins). Wird mit dem
        Host-State gespeichert, damit die Preset-Auswahl im Menue das Schliessen
        und erneute Oeffnen des Editorfensters sowie das Neuladen des DAW-Projekts
        ueberlebt. */
    juce::String currentPresetFile;

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateTempo (const juce::MidiBuffer& midiMessages);

    static constexpr double maxDelaySeconds = 12.0; // 30 BPM, ganze Note dot

    /** Ein Delay-Ringpuffer pro Kanal, inkl. fraktionaler Auslesung. */
    struct DelayLine
    {
        juce::AudioBuffer<float> buf;
        double sr = 44100.0;
        int size = 0;
        int writePos = 0;

        void prepare (double sampleRate, double maxSeconds)
        {
            sr = sampleRate;
            size = juce::jmax (8, (int) (sr * maxSeconds) + 8);
            buf.setSize (1, size);
            buf.clear();
            writePos = 0;
        }

        void reset()
        {
            buf.clear();
            writePos = 0;
        }

        /** Liest einen um delaySamples verzoegerten Wert (lineare Interpolation). */
        float readAt (double delaySamples) const
        {
            if (size <= 0)
                return 0.0f;

            delaySamples = juce::jlimit (1.0, (double) size - 2.0, delaySamples);

            double pos = (double) writePos - delaySamples;
            while (pos < 0.0)
                pos += (double) size;
            // Rundung kann pos exakt auf size bringen -> in [0, size) zurueckholen.
            while (pos >= (double) size)
                pos -= (double) size;

            int    i1   = (int) pos;
            const double frac = pos - (double) i1;
            if (i1 >= size)                 // Sicherheitsnetz (sollte nicht noetig sein)
                i1 = size - 1;
            const int    i2   = (i1 + 1) % size;

            const float* d = buf.getReadPointer (0);
            return d[i1] * (float) (1.0 - frac) + d[i2] * (float) frac;
        }

        void write (float v)
        {
            if (size <= 0)
                return;

            buf.getWritePointer (0)[writePos] = v;

            if (++writePos >= size)
                writePos = 0;
        }
    };

    // Vier Delay-Lines:
    //   Ping-Pong: (delayL1, delayR1) = Voice 1,  (delayL2, delayR2) = Voice 2
    //   Dual:      delayL1 = Kanal 1,  delayR2 = Kanal 2
    DelayLine delayL1, delayR1, delayL2, delayR2;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoAudioProcessor)
};