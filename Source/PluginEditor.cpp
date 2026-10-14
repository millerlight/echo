#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace
{
    void setupLabel (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    }

    // Linksbuendige Beschriftung (gleiche Kante wie "1"/"2").
    void setupLabelLeft (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    }

    // Slider-Fahrbahn: Wertseite gruen, Restseite blau (gleiche Helligkeit).
    const juce::Colour kTrackBlue  { 0xff42a2c8 };
    const juce::Colour kTrackGreen { 0xff4caf50 };

    void styleSliderTrack (juce::Slider& s)
    {
        s.setColour (juce::Slider::trackColourId,      kTrackGreen);  // Wertseite
        s.setColour (juce::Slider::backgroundColourId, kTrackBlue);   // Restseite
        s.setColour (juce::Slider::thumbColourId,      kTrackBlue);
    }

    void setupMiniLabel (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colours::grey);
    }

    void setupCombo (juce::ComboBox& c, const juce::StringArray& items)
    {
        c.addItemList (items, 1);
    }

    void setupSlider (juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
        s.setTextValueSuffix (suffix);
        s.setNumDecimalPlacesToDisplay (0); // keine Nachkommastellen
    }

    // Gleiche Datei? (Pfadvergleich, Windows-tolerant)
    bool sameFile (const juce::File& a, const juce::File& b)
    {
        return a.getFullPathName().equalsIgnoreCase (b.getFullPathName());
    }
}

//==============================================================================
EchoAudioProcessorEditor::EchoAudioProcessorEditor (EchoAudioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p)
{
    //--- Labels ---------------------------------------------------------------
    setupLabelLeft (lblMode,   "Modus");
    setupLabelLeft (lblDryWet, "Dry/Wet");
    setupLabelLeft (lblRow1,   "1");
    setupLabelLeft (lblRow2,   "2");
    setupLabel   (lblPan1,   "Pan");
    setupLabel   (lblPan2,   "Pan");
    setupLabel   (lblFb1,    "Feedback");
    setupLabel   (lblFb2,    "Feedback");
    setupMiniLabel (lblL1,   "L");
    setupMiniLabel (lblR1,   "R");
    setupMiniLabel (lblL2,   "L");
    setupMiniLabel (lblR2,   "R");

    lblBpm.setJustificationType (juce::Justification::centredLeft);
    lblBpm.setColour (juce::Label::textColourId, juce::Colours::white);

    setupLabel (lblSync, "Sync");

    //--- ComboBoxen -----------------------------------------------------------
    const juce::StringArray modeChoices   { "Ping-Pong", "Dual" };
    const juce::StringArray noteChoices   { "1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    const juce::StringArray subdivChoices { "Straight", "Dot", "Triplet", "None" };

    setupCombo (cbMode,     modeChoices);
    setupCombo (cbNote1,    noteChoices);
    setupCombo (cbSubdiv1,  subdivChoices);
    setupCombo (cbNote2,    noteChoices);
    setupCombo (cbSubdiv2,  subdivChoices);

    //--- Sync-Switcher (ON/OFF) --------------------------------------------------
    // SyncSwitch setzt selbst clickingTogglesState; Label "Sync" davor.

    //--- Slider ---------------------------------------------------------------
    setupSlider (slTime1,  " ms");
    setupSlider (slPan1,   " %");
    setupSlider (slFb1,    " %");
    setupSlider (slTime2,  " ms");
    setupSlider (slPan2,   " %");
    setupSlider (slFb2,    " %");
    setupSlider (slDryWet, " %");

    //--- Parameterbindung (erst NACH dem Befuellen der ComboBoxen!) -----------
    modeAtt    = std::make_unique<ComboAtt>  (proc.params, "mode",    cbMode);
    note1Att   = std::make_unique<ComboAtt>  (proc.params, "note1",   cbNote1);
    subdiv1Att = std::make_unique<ComboAtt>  (proc.params, "subdiv1", cbSubdiv1);
    note2Att   = std::make_unique<ComboAtt>  (proc.params, "note2",   cbNote2);
    subdiv2Att = std::make_unique<ComboAtt>  (proc.params, "subdiv2", cbSubdiv2);
    syncAtt    = std::make_unique<ButtonAtt> (proc.params, "sync",   syncSwitch);
    time1Att   = std::make_unique<SliderAtt> (proc.params, "time1",   slTime1);
    pan1Att    = std::make_unique<SliderAtt> (proc.params, "pan1",    slPan1);
    fb1Att     = std::make_unique<SliderAtt> (proc.params, "fb1",     slFb1);
    time2Att   = std::make_unique<SliderAtt> (proc.params, "time2",   slTime2);
    pan2Att    = std::make_unique<SliderAtt> (proc.params, "pan2",    slPan2);
    fb2Att     = std::make_unique<SliderAtt> (proc.params, "fb2",     slFb2);
    dryAtt     = std::make_unique<SliderAtt> (proc.params, "drywet",  slDryWet);

    //--- Kinder ---------------------------------------------------------------
    for (auto* c : { &cbMode, &cbNote1, &cbSubdiv1, &cbNote2, &cbSubdiv2 })
        addAndMakeVisible (*c);

    for (auto* s : { &slTime1, &slPan1, &slFb1, &slTime2, &slPan2, &slFb2, &slDryWet })
        addAndMakeVisible (*s);

    for (auto* l : { &lblBpm, &lblMode, &lblDryWet, &lblSync,
                     &lblRow1, &lblRow2, &lblPan1, &lblPan2,
                     &lblL1, &lblR1, &lblL2, &lblR2, &lblFb1, &lblFb2 })
        addAndMakeVisible (*l);

    addAndMakeVisible (syncSwitch);

    //--- Preset-Icons oben rechts (Preset als .echo-XML speichern/laden/loeschen)
    addAndMakeVisible (btnSavePreset);
    addAndMakeVisible (btnLoadPreset);
    addAndMakeVisible (btnDeletePreset);
    btnSavePreset.onClick   = [this] { savePreset(); };
    btnLoadPreset.onClick   = [this] { loadPreset(); };
    btnDeletePreset.onClick = [this] { deleteSelectedPreset(); };
    tooltip = std::make_unique<juce::TooltipWindow> (this, 700);

    //--- Preset-Menue (Liste wird beim Oeffnen neu aus dem Ordner gebaut) ------
    cbPreset.setTextWhenNothingSelected ("Preset");
    cbPreset.onAboutToShow = [this] { refreshPresetList(); };
    cbPreset.onChange      = [this] { applySelectedPreset(); };
    addAndMakeVisible (cbPreset);
    refreshPresetList();

    //--- Fahrbahn-Farben + Doppelklick-Standardwert -----------------------------
    // (Standardwert = Default des jeweiligen Parameters aus der APVTS.)
    auto defOf = [this] (const char* id) -> double
    {
        if (auto* p = proc.params.getParameter (id))
            return (double) p->convertFrom0to1 (p->getDefaultValue());
        return 0.0;
    };

    const struct { juce::Slider* s; const char* id; } sliderDefs[] =
    {
        { &slTime1,  "time1"  }, { &slPan1, "pan1" }, { &slFb1, "fb1" },
        { &slTime2,  "time2"  }, { &slPan2, "pan2" }, { &slFb2, "fb2" },
        { &slDryWet, "drywet" }
    };

    for (auto& e : sliderDefs)
    {
        styleSliderTrack (*e.s);
        e.s->setDoubleClickReturnValue (true, defOf (e.id));
    }

    setResizable (false, false);
    setSize (660, 213);

    updateEnablement();
    startTimerHz (4);
}

EchoAudioProcessorEditor::~EchoAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void EchoAudioProcessorEditor::timerCallback()
{
    lblBpm.setText (juce::String (proc.bpm.load()) + " BPM", juce::dontSendNotification);
    updateEnablement();
}

void EchoAudioProcessorEditor::updateEnablement()
{
    const bool sync = proc.params.getRawParameterValue ("sync")->load() > 0.5f;

    // Sync an -> Notenart steuert die Zeit (ms-Time-Felder versteckt).
    // Sync aus -> ms-Time-Feld pro Zeile statt Notenart.
    // Betonung wirkt in beiden Modi (auch ohne Sync) und ist nie grau.
    cbNote1.setVisible (sync);
    cbNote2.setVisible (sync);
    slTime1.setVisible (! sync);
    slTime2.setVisible (! sync);

    // DAW-BPM stark ausgrauen, wenn kein Sync verwendet wird.
    lblBpm.setColour (juce::Label::textColourId,
                      sync ? juce::Colours::white : juce::Colour (0xff525252));

    // Pan und Feedback sind in beiden Modi (Ping-Pong, Dual) immer aktiv.
    slPan1.setEnabled (true);
    slPan2.setEnabled (true);
    slFb1.setEnabled  (true);
    slFb2.setEnabled  (true);
}

//==============================================================================
void EchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1f1f1f));

    auto header = getLocalBounds().removeFromTop (32);
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillRect (header);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (18.0f).withStyle ("Bold")));
    g.drawText ("Echo", 12, 0, 100, 30, juce::Justification::centredLeft);
}

//==============================================================================
void EchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    // Preset-Icons oben rechts in der Kopfzeile (von rechts: loeschen, laden,
    // speichern). Das Loesch-Symbol sitzt direkt neben dem Ordner (laden).
    auto top = getLocalBounds().removeFromTop (32);
    btnDeletePreset.setBounds (top.removeFromRight (32).reduced (4, 4));
    btnLoadPreset.setBounds   (top.removeFromRight (32).reduced (4, 4));
    btnSavePreset.setBounds   (top.removeFromRight (32).reduced (4, 4));

    // Titelzeile ("Echo" wird gemalt) + kleine Leerzeile (1/3 Zeilhoehe);
    // darunter sitzt die BPM-/Sync-Zeile (alles darunter folgt mit).
    area.removeFromTop (30);
    area.removeFromTop (9);

    // Zeile: BPM + Sync-Schalter (linksbuendig)
    auto rowBpm = area.removeFromTop (26);
    lblBpm.setBounds     (rowBpm.removeFromLeft (80));
    lblSync.setBounds    (rowBpm.removeFromLeft (40));
    syncSwitch.setBounds (rowBpm.removeFromLeft (104).reduced (0, 2));
    // Preset-Menue am rechten Rand der Zeile ausrichten.
    cbPreset.setBounds   (rowBpm.removeFromRight (200).reduced (0, 1));
    area.removeFromTop (6);

    // Zeile: Modus (linksbuendig) -- Combo auf gleicher Hoehe/Abstand wie der
    // Sync-Schalter in der Zeile darueber (dessen linke Kante x = 130).
    auto rowMode = area.removeFromTop (26);
    lblMode.setBounds (rowMode.removeFromLeft (70));
    rowMode.removeFromLeft (50);                 // -> Combo beginnt bei x = 130
    cbMode.setBounds  (rowMode.removeFromLeft (140));
    area.removeFromTop (6);

    // Zeilen 1/2: Time, Pan und Feedback erhalten exakt dieselbe Laenge.
    const int w = 138;
    auto layoutRow = [&] (juce::Label& rowLbl,
                          juce::ComboBox& note, juce::Slider& time, juce::ComboBox& subdiv,
                          juce::Label& pan, juce::Label& lMark, juce::Slider& panSl, juce::Label& rMark,
                          juce::Label& fb, juce::Slider& fbSl)
    {
        auto row = area.removeFromTop (26);
        rowLbl.setBounds (row.removeFromLeft (20));

        const auto field = row.removeFromLeft (w);
        note.setBounds (field);
        time.setBounds (field);

        // Betonung + "Pan" weiter nach rechts, naeher am Pan-Regler.
        row.removeFromLeft (22);
        subdiv.setBounds (row.removeFromLeft (86));
        pan.setBounds    (row.removeFromLeft (28));
        lMark.setBounds  (row.removeFromLeft (4));
        panSl.setBounds  (row.removeFromLeft (w));
        rMark.setBounds  (row.removeFromLeft (8));
        fb.setBounds     (row.removeFromLeft (58));   // Abstand FB-Label -> Regler minimiert
        fbSl.setBounds   (row.removeFromLeft (w));
    };

    layoutRow (lblRow1, cbNote1, slTime1, cbSubdiv1, lblPan1, lblL1, slPan1, lblR1, lblFb1, slFb1);
    area.removeFromTop (6);

    layoutRow (lblRow2, cbNote2, slTime2, cbSubdiv2, lblPan2, lblL2, slPan2, lblR2, lblFb2, slFb2);
    area.removeFromTop (6);

    // Reihe ganz unten: Dry/Wet (linksbuendig)
    auto rowDry = area.removeFromTop (26);
    lblDryWet.setBounds (rowDry.removeFromLeft (70));
    slDryWet.setBounds  (rowDry);
}

//==============================================================================
// Preset speichern / laden / Menue (eigene .echo-Dateien im Preset-Ordner).
// Gespeichert wird genau der Plugin-State (wie ihn der Host bekommt).
juce::File EchoAudioProcessorEditor::presetDirectory()
{
    // Presets liegen im Dokumente-Ordner des Benutzers ("Dokumente\echo presets").
    // Der Ordner wird bei Bedarf automatisch angelegt.
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("echo presets");

    dir.createDirectory();
    return dir;
}

void EchoAudioProcessorEditor::refreshPresetList()
{
    const auto dir = presetDirectory();

    presetFiles.clear();
    dir.findChildFiles (presetFiles, juce::File::findFiles, false, "*.echo");

    // Reihenfolge: Aenderungsdatum (zuletzt geaendert zuerst).
    std::sort (presetFiles.begin(), presetFiles.end(),
               [] (const juce::File& a, const juce::File& b)
               {
                   return a.getLastModificationTime() > b.getLastModificationTime();
               });

    const int keep = cbPreset.getSelectedId();
    cbPreset.clear (juce::dontSendNotification);

    for (int i = 0; i < presetFiles.size(); ++i)
        cbPreset.addItem (presetFiles.getReference (i).getFileNameWithoutExtension(), i + 1);

    // Auswahl aus dem vom Processor gemerkten Preset-Pfad wiederherstellen.
    // Das laesst die Markierung das Schliessen/Oeffnen des Editors und das
    // Neuladen des DAW-Projekts ueberleben.
    if (proc.currentPresetFile.isNotEmpty())
    {
        markPresetSelected (juce::File (proc.currentPresetFile));
    }
    else if (keep > 0 && keep <= presetFiles.size())
    {
        cbPreset.setSelectedId (keep, juce::dontSendNotification);
    }
}

void EchoAudioProcessorEditor::applySelectedPreset()
{
    const int id = cbPreset.getSelectedId();

    if (id <= 0 || id > presetFiles.size())
        return;

    const auto file = presetFiles.getReference (id - 1);

    juce::MemoryBlock state;
    if (file.existsAsFile() && file.loadFileAsData (state))
    {
        proc.setStateInformation (state.getData(), (int) state.getSize());
        proc.updateHostDisplay();
        updateEnablement();

        // Geladenes Preset als aktiv merken (ueberlebt Editor-Neustart /
        // Projekt-Neuladen).
        markPresetSelected (file);
    }
}

void EchoAudioProcessorEditor::markPresetSelected (const juce::File& file)
{
    // Aktives Preset im Processor merken (Teil des Host-States).
    proc.currentPresetFile = file.getFullPathName();

    for (int i = 0; i < presetFiles.size(); ++i)
    {
        if (sameFile (presetFiles.getReference (i), file))
        {
            cbPreset.setSelectedId (i + 1, juce::dontSendNotification);
            return;
        }
    }

    // Datei liegt ausserhalb des Preset-Ordners -> keine Markierung.
    cbPreset.setSelectedId (0, juce::dontSendNotification);
}

void EchoAudioProcessorEditor::savePreset()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Preset speichern",
                                                       presetDirectory().getChildFile ("Preset.echo"),
                                                       "*.echo");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles
                                | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return;

            if (! file.hasFileExtension ("echo"))
                file = file.withFileExtension ("echo");

            juce::MemoryBlock state;
            proc.getStateInformation (state);
            file.replaceWithData (state.getData(), state.getSize());

            // Neu speichern -> sofort im Menue anzeigen und markieren
            // (die frische Datei hat das neueste Aenderungsdatum und steht oben).
            refreshPresetList();
            markPresetSelected (file);
        });
}

void EchoAudioProcessorEditor::loadPreset()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Preset laden",
                                                       presetDirectory(),
                                                       "*.echo");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();

            if (! file.existsAsFile())
                return;

            juce::MemoryBlock state;
            if (file.loadFileAsData (state))
            {
                proc.setStateInformation (state.getData(), (int) state.getSize());
                proc.updateHostDisplay();
                updateEnablement();

                // Geladenes Preset direkt im Menue markieren.
                refreshPresetList();
                markPresetSelected (file);
            }
        });
}

void EchoAudioProcessorEditor::deleteSelectedPreset()
{
    const int id = cbPreset.getSelectedId();

    // Nichts ausgewaehlt -> kurzer Hinweis.
    if (id <= 0 || id > presetFiles.size())
    {
        juce::AlertWindow::showAsync (
            juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::InfoIcon)
                .withTitle ("Preset loeschen")
                .withMessage ("Bitte zuerst im Preset-Menue ein Preset auswaehlen.")
                .withButton ("OK")
                .withAssociatedComponent (this),
            [] (int) {});

        return;
    }

    const auto file   = presetFiles.getReference (id - 1);
    const int  prevId = id;

    juce::Component::SafePointer<EchoAudioProcessorEditor> safe (this);

    // Sicherheitsabfrage vor dem Loeschen (Ergebnis 1 = erster Knopf = ja).
    juce::AlertWindow::showAsync (
        juce::MessageBoxOptions()
            .withIconType (juce::MessageBoxIconType::WarningIcon)
            .withTitle ("Preset loeschen")
            .withMessage ("Preset \"" + file.getFileNameWithoutExtension()
                              + "\" wirklich loeschen?")
            .withButton ("Loeschen")
            .withButton ("Abbrechen")
            .withAssociatedComponent (this),
        [safe, file, prevId] (int result)
        {
            if (result != 1)
                return;

            const bool deleted = file.deleteFile();

            if (safe == nullptr)
                return;

            if (deleted)
            {
                // War das geloeschte Preset das aktive, die Merkung aufheben.
                if (sameFile (juce::File (safe->proc.currentPresetFile), file))
                    safe->proc.currentPresetFile.clear();

                if (safe->cbPreset.getSelectedId() == prevId)
                    safe->cbPreset.setSelectedId (0, juce::dontSendNotification);

                safe->refreshPresetList();
            }
            else
            {
                juce::AlertWindow::showAsync (
                    juce::MessageBoxOptions()
                        .withIconType (juce::MessageBoxIconType::WarningIcon)
                        .withTitle ("Preset loeschen")
                        .withMessage ("Konnte \"" + file.getFileNameWithoutExtension()
                                          + "\" nicht loeschen.")
                        .withButton ("OK")
                        .withAssociatedComponent (safe.getComponent()),
                    [] (int) {});
            }
        });
}