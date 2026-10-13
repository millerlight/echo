#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
EchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray modeChoices   { "Ping-Pong", "Dual" };
    const juce::StringArray noteChoices   { "1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    // Betonung: Straight (gerade), Dot, Triplet, None (= kein Echo fuer die Zeile).
    const juce::StringArray subdivChoices { "Straight", "Dot", "Triplet", "None" };

    layout.add (std::make_unique<juce::AudioParameterChoice> ("mode",   "Modus",     modeChoices,   1)); // Dual
    layout.add (std::make_unique<juce::AudioParameterBool>   ("sync",   "Tempo-Sync", true));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("time1",  "Time 1 [ms]",
                        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f), 500.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("time2",  "Time 2 [ms]",
                        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f), 1000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("drywet", "Dry/Wet [%]",
                        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> ("note1",   "Notenart 1", noteChoices,   3)); // 1/8
    layout.add (std::make_unique<juce::AudioParameterChoice> ("subdiv1", "Betonung 1", subdivChoices, 2)); // Triplet
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("pan1", "Pan 1 [%]",
                        juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f), -100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("fb1", "Feedback 1 [%]",
                        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> ("note2",   "Notenart 2", noteChoices,   2)); // 1/4
    layout.add (std::make_unique<juce::AudioParameterChoice> ("subdiv2", "Betonung 2", subdivChoices, 0)); // Straight
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("pan2", "Pan 2 [%]",
                        juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("fb2", "Feedback 2 [%]",
                        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f));

    return layout;
}

//==============================================================================
EchoAudioProcessor::EchoAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

//==============================================================================
const juce::String EchoAudioProcessor::getName() const  { return JucePlugin_Name; }

bool EchoAudioProcessor::acceptsMidi() const            { return false; }
bool EchoAudioProcessor::producesMidi() const           { return false; }
bool EchoAudioProcessor::isMidiEffect() const           { return false; }

double EchoAudioProcessor::getTailLengthSeconds() const
{
    const float f1 = params.getRawParameterValue ("fb1")->load() * 0.01f;
    const float f2 = params.getRawParameterValue ("fb2")->load() * 0.01f;
    const float fb = juce::jmax (f1, f2);

    if (fb < 0.01f)
        return 1.5;

    const double tail = 4.0 / juce::jmax (0.001, 1.0 - (double) fb);
    return juce::jlimit (1.5, maxDelaySeconds, tail);
}

//==============================================================================
int EchoAudioProcessor::getNumPrograms()                                { return 1; }
int EchoAudioProcessor::getCurrentProgram()                             { return 0; }
void EchoAudioProcessor::setCurrentProgram (int)                        {}
const juce::String EchoAudioProcessor::getProgramName (int)             { return {}; }
void EchoAudioProcessor::changeProgramName (int, const juce::String&)   {}

//==============================================================================
void EchoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    delayL1.prepare (sampleRate, maxDelaySeconds);
    delayR1.prepare (sampleRate, maxDelaySeconds);
    delayL2.prepare (sampleRate, maxDelaySeconds);
    delayR2.prepare (sampleRate, maxDelaySeconds);
}

void EchoAudioProcessor::releaseResources()
{
    delayL1.reset();
    delayR1.reset();
    delayL2.reset();
    delayR2.reset();
}

bool EchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
void EchoAudioProcessor::updateTempo (const juce::MidiBuffer& midiMessages)
{
    // 1) Bevorzugt: Tempo direkt vom Host-Playhead.
    if (auto* hostHead = getPlayHead())
    {
        if (auto position = hostHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                bpm.store (juce::jlimit (30, 300, juce::roundToInt (*hostBpm)));
        }
    }

    // 2) Fallback: MIDI-Tempo-Meta-Events (falls der Host sie liefert).
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isTempoMetaEvent())
        {
            const double secondsPerQuarter = message.getTempoSecondsPerQuarterNote();

            if (secondsPerQuarter > 0.0)
                bpm.store (juce::jlimit (30, 300, juce::roundToInt (60.0 / secondsPerQuarter)));
        }
    }
}

//==============================================================================
void EchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    updateTempo (midiMessages);

    const int numSamples = buffer.getNumSamples();

    if (numSamples == 0)
        return;

    auto raw = [this] (const char* id) { return params.getRawParameterValue (id)->load(); };

    const int   mode = juce::roundToInt (raw ("mode"));   // 0 = Ping-Pong, 1 = Dual
    const bool  sync = raw ("sync") > 0.5f;
    const float wet  = juce::jlimit (0.0f, 1.0f, raw ("drywet") * 0.01f);

    const float fb1 = juce::jlimit (0.0f, 1.0f, raw ("fb1") * 0.01f);
    const float fb2 = juce::jlimit (0.0f, 1.0f, raw ("fb2") * 0.01f);
    const float pan1 = juce::jlimit (-1.0f, 1.0f, raw ("pan1") * 0.01f);
    const float pan2 = juce::jlimit (-1.0f, 1.0f, raw ("pan2") * 0.01f);

    const int noteIdx1 = juce::roundToInt (raw ("note1"));
    const int noteIdx2 = juce::roundToInt (raw ("note2"));
    const int subIdx1  = juce::roundToInt (raw ("subdiv1"));
    const int subIdx2  = juce::roundToInt (raw ("subdiv2"));

    static const double noteMult[6]   = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };
    static const double subdivMult[3] = { 1.0, 1.5, 2.0 / 3.0 };

    // Betonung "None" (Index 3): diese Zeile schickt kein Echo (nur Dry).
    const bool active1 = (subIdx1 != 3);
    const bool active2 = (subIdx2 != 3);

    const double quad = 60.0 / (double) juce::jlimit (30, 300, bpm.load());

    // Per-Zeile ms-Time fuer Sync-aus-Betrieb.
    const double manual1 = (double) raw ("time1") * 0.001;
    const double manual2 = (double) raw ("time2") * 0.001;

    auto delayFor = [sync, quad] (int noteIdx, int subdivIdx, double manualMs) -> double
    {
        // Betonung wirkt in beiden Modi (auch ohne Sync, auf die ms-Time).
        const double subdivFactor = subdivMult[juce::jlimit (0, 2, subdivIdx)];

        if (! sync)
            return manualMs * subdivFactor;

        const double noteFactor   = noteMult  [juce::jlimit (0, 5, noteIdx)];
        return quad * noteFactor * subdivFactor;
    };

    const double delaySamples1 = juce::jlimit (0.001, maxDelaySeconds, delayFor (noteIdx1, subIdx1, manual1)) * currentSampleRate;
    const double delaySamples2 = juce::jlimit (0.001, maxDelaySeconds, delayFor (noteIdx2, subIdx2, manual2)) * currentSampleRate;

    // Mono-Bus: reine Durchleitung.
    if (buffer.getNumChannels() < 2)
        return;

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    const float pi4 = juce::MathConstants<float>::pi * 0.25f;

    // Equal-Power-Gains der beiden Pan-Positionen (pan1 = Zeile 1, pan2 = Zeile 2).
    // Der Ping-Pong-Rahmen wird direkt von diesen beiden Pan-Reglern aufgespannt:
    // Echo n liegt bei pan1, Echo n+1 bei pan2 (und umgekehrt).
    const float gL1 = std::cos ((pan1 + 1.0f) * pi4);   // Position pan1 -> L
    const float gR1 = std::sin ((pan1 + 1.0f) * pi4);   // Position pan1 -> R
    const float gL2 = std::cos ((pan2 + 1.0f) * pi4);   // Position pan2 -> L
    const float gR2 = std::sin ((pan2 + 1.0f) * pi4);   // Position pan2 -> R

    if (mode == 1) // DUAL: unabhaengige Delays pro Seite, eigene Pan + Feedback.
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = right[i];

            // "None" schaltet die Zeile ab: kein Echo, kein Schreiben.
            const float e1 = active1 ? delayL1.readAt (delaySamples1) : 0.0f;
            const float e2 = active2 ? delayR2.readAt (delaySamples2) : 0.0f;

            // Echoanteil (ohne Dry), fuer das Dry/Wet-Crossfade.
            const float echoL = e1 * gL1 + e2 * gL2;
            const float echoR = e1 * gR1 + e2 * gR2;

            // 0 % = nur trocken, 100 % = nur Echo.
            left[i]  = inL * (1.0f - wet) + wet * echoL;
            right[i] = inR * (1.0f - wet) + wet * echoR;

            if (active1) delayL1.write (inL + e1 * fb1);
            if (active2) delayR2.write (inR + e2 * fb2);
        }
    }
    else // PING-PONG: zwei unabhaengige, kreuzgekoppelte Voices.
    {
        // Der Ping-Pong-Rahmen wird von den beiden Pan-Reglern aufgespannt:
        // jedes Echo liegt abwechselnd auf Position pan1 und Position pan2.
        //
        //   Voice 1 (Zeile 1, linker Eingang): Echo 1 auf pan1, dann pan2, pan1, ...
        //   Voice 2 (Zeile 2, rechter Eingang): Echo 1 auf pan2, dann pan1, pan2, ...
        //
        // physikalische Lines: Voice 1 -> (delayL1, delayR1), Voice 2 -> (delayL2, delayR2);
        // A = Echo-Position pan1, B = Echo-Position pan2.

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = right[i];

            float echoL = 0.0f;
            float echoR = 0.0f;

            // Voice 1 (startet bei pan1): bei "None" komplett aus.
            if (active1)
            {
                const float eA = delayL1.readAt (delaySamples1);   // Echo bei pan1
                const float eB = delayR1.readAt (delaySamples1);   // Echo bei pan2

                echoL += eA * gL1 + eB * gL2;
                echoR += eA * gR1 + eB * gR2;

                delayL1.write (inL + eB * fb1);
                delayR1.write (eA * fb1);
            }

            // Voice 2 (startet bei pan2): bei "None" komplett aus.
            if (active2)
            {
                const float eA = delayL2.readAt (delaySamples2);   // Echo bei pan1
                const float eB = delayR2.readAt (delaySamples2);   // Echo bei pan2

                echoL += eA * gL1 + eB * gL2;
                echoR += eA * gR1 + eB * gR2;

                delayR2.write (inR + eA * fb2);
                delayL2.write (eB * fb2);
            }

            // Dry/Wet-Crossfade: 0 % = nur trocken, 100 % = nur Echo.
            left[i]  = inL * (1.0f - wet) + wet * echoL;
            right[i] = inR * (1.0f - wet) + wet * echoR;
        }
    }
}

//==============================================================================
bool EchoAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* EchoAudioProcessor::createEditor()
{
    return new EchoAudioProcessorEditor (*this);
}

//==============================================================================
void EchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoAudioProcessor();
}