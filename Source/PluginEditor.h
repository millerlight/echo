#pragma once

#include "PluginProcessor.h"

//==============================================================================
/** Schalter (Switcher) mit deutlich sichtbarem ON/OFF-Zustand. */
class SyncSwitch : public juce::ToggleButton
{
public:
    SyncSwitch()
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const auto b = getLocalBounds().reduced (0, 4);
        const bool on = getToggleState();

        // Schiene: gruen bei ON, dunkel bei OFF.
        g.setColour (on ? juce::Colour (0xff2e7d32) : juce::Colour (0xff424242));
        g.fillRoundedRectangle (b.toFloat(), b.getHeight() * 0.5f);
        g.setColour (on ? juce::Colour (0xff66bb6a) : juce::Colour (0xff555555));
        g.drawRoundedRectangle (b.toFloat(), (float) b.getHeight() * 0.5f, 1.0f);

        // Zustandstext: immer lesbar, mittig.
        g.setFont (juce::Font (juce::FontOptions (b.getHeight() * 0.55f).withStyle ("Bold")));
        g.setColour (juce::Colours::white);
        g.drawText (on ? "ON" : "OFF", b.translated (0, -1), juce::Justification::centred);

        // Schaltknopf wandert auf die aktive Seite.
        const float d = (float) b.getHeight() * 0.95f;
        juce::Rectangle<float> knob;
        if (on)  knob = juce::Rectangle<float> ((float) b.getRight() - d, (float) b.getY(), d, d);
        else     knob = juce::Rectangle<float> ((float) b.getX(), (float) b.getY(), d, d);

        g.setColour (juce::Colour (0xfff5f5f5));
        g.fillEllipse (knob);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SyncSwitch)
};

//==============================================================================
/** Icon-Knopf (Preset speichern/laden/loeschen) fuer die obere rechte Ecke. */
class PresetIconButton : public juce::Button
{
public:
    enum IconType { saveIcon, loadIcon, deleteIcon };

    explicit PresetIconButton (IconType t)
        : juce::Button (t == saveIcon ? "Preset speichern"
                        : (t == loadIcon ? "Preset laden" : "Preset loeschen")),
          type (t)
    {
        setTooltip (getName());
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        g.setColour (down ? juce::Colour (0xffbdbdbd)
                          : (over ? juce::Colours::white : juce::Colours::lightgrey));

        auto b = getLocalBounds().toFloat().reduced (4.0f);

        if (type == saveIcon)
        {
            // Diskette
            g.drawRoundedRectangle (b, 2.0f, 1.6f);

            juce::Rectangle<float> shutter { b.getX() + b.getWidth() * 0.28f, b.getY(),
                                             b.getWidth() * 0.44f, b.getHeight() * 0.34f };
            g.fillRect (shutter);

            juce::Rectangle<float> label { b.getX() + b.getWidth() * 0.22f,
                                           b.getBottom() - b.getHeight() * 0.34f,
                                           b.getWidth() * 0.56f,
                                           b.getHeight() * 0.32f };
            g.fillRect (label);
        }
        else if (type == loadIcon)
        {
            // Ordner
            juce::Rectangle<float> tab { b.getX(), b.getY(),
                                         b.getWidth() * 0.45f, b.getHeight() * 0.22f };
            g.fillRect (tab);

            juce::Rectangle<float> body { b.getX(), b.getY() + b.getHeight() * 0.16f,
                                          b.getWidth(), b.getHeight() * 0.82f };
            g.drawRoundedRectangle (body, 2.0f, 1.6f);
        }
        else
        {
            // Papierkorb
            const float w = b.getWidth();
            const float h = b.getHeight();

            // Deckel
            g.drawLine (b.getX(), b.getY() + h * 0.20f,
                        b.getRight(), b.getY() + h * 0.20f, 1.6f);

            // Griff
            g.drawLine (b.getX() + w * 0.34f, b.getY() + h * 0.05f,
                        b.getX() + w * 0.66f, b.getY() + h * 0.05f, 1.6f);
            g.drawLine (b.getX() + w * 0.34f, b.getY() + h * 0.05f,
                        b.getX() + w * 0.34f, b.getY() + h * 0.20f, 1.6f);
            g.drawLine (b.getX() + w * 0.66f, b.getY() + h * 0.05f,
                        b.getX() + w * 0.66f, b.getY() + h * 0.20f, 1.6f);

            // Koerper (leicht nach unten verjuengt)
            juce::Path body;
            body.startNewSubPath (b.getX() + w * 0.16f, b.getY() + h * 0.28f);
            body.lineTo (b.getX() + w * 0.24f, b.getBottom());
            body.lineTo (b.getX() + w * 0.76f, b.getBottom());
            body.lineTo (b.getX() + w * 0.84f, b.getY() + h * 0.28f);
            g.strokePath (body, juce::PathStrokeType (1.6f));
        }
    }

private:
    IconType type;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetIconButton)
};

//==============================================================================
/** Preset-Auswahl; die Liste wird beim Oeffnen des Menues neu aus dem Ordner
    aufgebaut (Reihenfolge: Aenderungsdatum). */
class PresetComboBox : public juce::ComboBox
{
public:
    PresetComboBox() = default;

    std::function<void()> onAboutToShow;

    void showPopup() override
    {
        if (onAboutToShow)
            onAboutToShow();

        juce::ComboBox::showPopup();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetComboBox)
};

//==============================================================================
/** Editor fuer das Echo-Plugin (dunkles Theme, ca. 660 x 213 px). */
class EchoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit EchoAudioProcessorEditor (EchoAudioProcessor&);
    ~EchoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    // Preset-Speichern/-Laden als eigenes .echo-XML (ueber die State-Methoden).
    void savePreset();
    void loadPreset();
    void deleteSelectedPreset();
    void refreshPresetList();
    void applySelectedPreset();
    void markPresetSelected (const juce::File&);
    static juce::File presetDirectory();

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    EchoAudioProcessor& proc;

    juce::Label lblBpm, lblMode, lblDryWet, lblSync;
    juce::Label lblRow1, lblRow2;
    juce::Label lblPan1, lblPan2, lblL1, lblR1, lblL2, lblR2, lblFb1, lblFb2;

    juce::ComboBox   cbMode, cbNote1, cbSubdiv1, cbNote2, cbSubdiv2;
    SyncSwitch       syncSwitch;
    juce::Slider     slTime1, slPan1, slFb1, slTime2, slPan2, slFb2, slDryWet;

    PresetIconButton btnSavePreset   { PresetIconButton::saveIcon };
    PresetIconButton btnLoadPreset   { PresetIconButton::loadIcon };
    PresetIconButton btnDeletePreset { PresetIconButton::deleteIcon };
    PresetComboBox   cbPreset;
    juce::Array<juce::File> presetFiles;
    std::unique_ptr<juce::FileChooser>   fileChooser;
    std::unique_ptr<juce::TooltipWindow> tooltip;

    std::unique_ptr<ComboAtt>  modeAtt, note1Att, subdiv1Att, note2Att, subdiv2Att;
    std::unique_ptr<ButtonAtt> syncAtt;
    std::unique_ptr<SliderAtt> time1Att, pan1Att, fb1Att, time2Att, pan2Att, fb2Att, dryAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoAudioProcessorEditor)
};