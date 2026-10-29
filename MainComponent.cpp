#include "MainComponent.h"



//==============================================================================
// Helpers
//==============================================================================
namespace
{
    te::ExternalPlugin* findInstrument (te::AudioTrack& t)
    {
        for (auto p : t.pluginList.getPlugins())
            if (auto* ep = dynamic_cast<te::ExternalPlugin*> (&*p))
                return ep;
        return nullptr;
    }

    void styleToggle (juce::TextButton& b, juce::Colour onColour)
    {
        b.setClickingTogglesState (true);
        b.setColour (juce::TextButton::buttonOnColourId, onColour);
        b.setColour (juce::TextButton::buttonColourId, colours::panelLight);
    }

    juce::File projectFolder()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("L-DAW");
        dir.createDirectory();
        return dir;
    }
}

//==============================================================================
// TrackRow
//==============================================================================
TrackRow::TrackRow (te::AudioTrack& t, int i) : track (t), index (i)
{
    nameLabel.setEditable (false, true, false);
    nameLabel.setColour (juce::Label::textColourId, colours::text);
    nameLabel.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    nameLabel.onTextChange = [this] { track.setName (nameLabel.getText()); };
    nameLabel.addMouseListener (this, false);
    addAndMakeVisible (nameLabel);

    instrumentButton.setColour (juce::TextButton::buttonColourId, colours::panelLight);
    instrumentButton.setTooltip ("Choose instrument plug-in");
    instrumentButton.onClick = [this] { if (onChooseInstrument) onChooseInstrument (*this); };
    addAndMakeVisible (instrumentButton);

    editButton.setColour (juce::TextButton::buttonColourId, colours::panelLight);
    editButton.setTooltip ("Open instrument panel");
    editButton.onClick = [this] { if (onOpenInstrument) onOpenInstrument (*this); };
    addAndMakeVisible (editButton);

    styleToggle (armButton,  colours::record);
    styleToggle (muteButton, juce::Colour (0xffd9a400));
    styleToggle (soloButton, juce::Colour (0xff2f9be0));
    armButton.setTooltip ("Record arm (also routes your MIDI keyboard to this track)");
    armButton.onClick  = [this] { if (onArm) onArm (*this, armButton.getToggleState()); };
    muteButton.onClick = [this] { track.setMute (muteButton.getToggleState()); };
    soloButton.onClick = [this] { track.setSolo (soloButton.getToggleState()); };
    addAndMakeVisible (armButton);
    addAndMakeVisible (muteButton);
    addAndMakeVisible (soloButton);

    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange (-60.0, 6.0, 0.1);
    volumeSlider.setSkewFactorFromMidPoint (-12.0);
    volumeSlider.setDoubleClickReturnValue (true, 0.0);
    volumeSlider.setColour (juce::Slider::thumbColourId, colours::accent);
    volumeSlider.setPopupDisplayEnabled (true, true, nullptr);
    volumeSlider.setTextValueSuffix (" dB");
    volumeSlider.onValueChange = [this]
    {
        if (auto vp = track.getVolumePlugin())
            vp->setVolumeDb ((float) volumeSlider.getValue());
    };
    addAndMakeVisible (volumeSlider);

    panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.setDoubleClickReturnValue (true, 0.0);
    panSlider.setColour (juce::Slider::rotarySliderFillColourId, colours::accent);
    panSlider.setPopupDisplayEnabled (true, true, nullptr);
    panSlider.onValueChange = [this]
    {
        if (auto vp = track.getVolumePlugin())
            vp->setPan ((float) panSlider.getValue());
    };
    addAndMakeVisible (panSlider);

    refresh();
}

void TrackRow::refresh()
{
    nameLabel.setText (track.getName(), juce::dontSendNotification);

    if (auto* ep = findInstrument (track))
        instrumentButton.setButtonText (ep->getName());
    else
        instrumentButton.setButtonText ("- no instrument -");

    muteButton.setToggleState (track.isMuted (false), juce::dontSendNotification);
    soloButton.setToggleState (track.isSolo (false),  juce::dontSendNotification);

    if (auto vp = track.getVolumePlugin())
    {
        volumeSlider.setValue (vp->getVolumeDb(), juce::dontSendNotification);
        panSlider.setValue (vp->getPan(), juce::dontSendNotification);
    }
}

void TrackRow::setSelected (bool s) { selected = s; repaint(); }
void TrackRow::setArmed (bool a)    { armButton.setToggleState (a, juce::dontSendNotification); }

void TrackRow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.setColour (selected ? colours::panelLight : colours::panel);
    g.fillRect (r);

    g.setColour (selected ? colours::accent : colours::panelLight);
    g.fillRect (r.removeFromLeft (6));

    g.setColour (colours::background);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

    g.setColour (colours::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText (juce::String (index + 1), 10, 4, 24, 14, juce::Justification::left);
}

void TrackRow::resized()
{
    auto r = getLocalBounds().reduced (6, 6).withTrimmedLeft (8);

    auto left = r.removeFromLeft (150);
    left.removeFromTop (12);
    nameLabel.setBounds (left.removeFromTop (22));

    r.removeFromLeft (4);
    auto mid = r.removeFromLeft (190).withSizeKeepingCentre (190, 28);
    editButton.setBounds (mid.removeFromRight (44));
    mid.removeFromRight (4);
    instrumentButton.setBounds (mid);

    r.removeFromLeft (8);
    auto btns = r.removeFromLeft (3 * 30).withSizeKeepingCentre (3 * 30, 26);
    armButton.setBounds  (btns.removeFromLeft (28)); btns.removeFromLeft (3);
    muteButton.setBounds (btns.removeFromLeft (28)); btns.removeFromLeft (3);
    soloButton.setBounds (btns.removeFromLeft (28));

    panSlider.setBounds (r.removeFromRight (40));
    r.removeFromRight (4);
    volumeSlider.setBounds (r.withSizeKeepingCentre (r.getWidth(), 24));
}

void TrackRow::mouseDown (const juce::MouseEvent&)
{
    if (onSelect) onSelect (*this);
}

//==============================================================================
// TracksArea
//==============================================================================
TracksArea::TracksArea()
{
    addAndMakeVisible (arrange);
}

void TracksArea::paint (juce::Graphics& g)
{
    g.setColour (colours::panel);
    g.fillRect (0, 0, headerWidth, rulerHeight);
    g.setColour (colours::dimText);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("TRACKS", 14, 0, 200, rulerHeight, juce::Justification::centredLeft);
}

void TracksArea::resized()
{
    for (int i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, rulerHeight + i * rowHeight, headerWidth, rowHeight);

    arrange.setBounds (headerWidth + 1, 0, juce::jmax (100, getWidth() - headerWidth - 1), getHeight());
}

//==============================================================================
// MainComponent
//==============================================================================
MainComponent::MainComponent()
{
    setWantsKeyboardFocus (true);

    titleLabel.setText ("L-DAW", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, colours::accent);
    addAndMakeVisible (titleLabel);

    for (auto* b : { &playButton, &stopButton, &recordButton, &addTrackButton, &demoButton, &audioButton,
                     &pluginsButton, &newButton, &openButton, &saveButton, &undoButton, &redoButton })
    {
        b->setColour (juce::TextButton::buttonColourId, colours::panelLight);
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    playButton.setColour (juce::TextButton::buttonOnColourId, colours::play);
    recordButton.setColour (juce::TextButton::buttonOnColourId, colours::record);
    styleToggle (loopButton,  colours::accent);
    styleToggle (clickButton, colours::accent);
    loopButton.setWantsKeyboardFocus (false);
    clickButton.setWantsKeyboardFocus (false);
    addAndMakeVisible (loopButton);
    addAndMakeVisible (clickButton);

    playButton.onClick     = [this] { togglePlay(); };
    stopButton.onClick     = [this] { stopOrReturnToZero(); };
    recordButton.onClick   = [this] { toggleRecord(); };
    loopButton.onClick     = [this] { if (edit) { edit->getTransport().looping = loopButton.getToggleState(); updateLoopRange(); } };
    clickButton.onClick    = [this] { if (edit) edit->clickTrackEnabled = clickButton.getToggleState(); };
    addTrackButton.onClick = [this] { addTrack(); };
    demoButton.onClick     = [this] { showQuickStartMenu(); };
    audioButton.onClick    = [this] { showAudioSettings(); };
    pluginsButton.onClick  = [this] { showPluginScanner(); };
    newButton.onClick      = [this] { createNewEdit(); };
    openButton.onClick     = [this] { openProject(); };
    saveButton.onClick     = [this] { saveProject(); };
    undoButton.onClick     = [this] { if (edit) { edit->getUndoManager().undo(); rebuildTracks(); } };
    redoButton.onClick     = [this] { if (edit) { edit->getUndoManager().redo(); rebuildTracks(); } };

    tempoSlider.setSliderStyle (juce::Slider::IncDecButtons);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 56, 26);
    tempoSlider.setRange (40.0, 240.0, 1.0);
    tempoSlider.setValue (120.0, juce::dontSendNotification);
    tempoSlider.onValueChange = [this]
    {
        if (edit == nullptr) return;
        if (auto tempo = edit->tempoSequence.getTempo (0))
            tempo->setBpm (tempoSlider.getValue());
        updateLoopRange();
    };
    addAndMakeVisible (tempoSlider);

    for (int bars : { 1, 2, 4, 8, 16, 32 })
        loopLengthBox.addItem (juce::String (bars) + (bars == 1 ? " bar" : " bars"), bars);
    loopLengthBox.setSelectedId (4, juce::dontSendNotification);
    loopLengthBox.onChange = [this] { updateLoopRange(); };
    addAndMakeVisible (loopLengthBox);

    for (int bars : { 8, 16, 32, 64 })
        zoomBox.addItem (juce::String (bars) + " bars", bars);
    zoomBox.setSelectedId (16, juce::dontSendNotification);
    zoomBox.onChange = [this] { tracksArea.arrange.barsVisible = zoomBox.getSelectedId(); tracksArea.arrange.repaint(); };
    addAndMakeVisible (zoomBox);

    for (auto* l : { &tempoLabel, &loopLabel, &zoomLabel })
    {
        l->setColour (juce::Label::textColourId, colours::dimText);
        l->setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    }

    positionLabel.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold)));
    positionLabel.setColour (juce::Label::textColourId, colours::accent);
    positionLabel.setColour (juce::Label::backgroundColourId, juce::Colours::black);
    positionLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (positionLabel);

    statusLabel.setColour (juce::Label::textColourId, colours::dimText);
    addAndMakeVisible (statusLabel);

    tracksArea.arrange.onLaneClicked  = [this] (int i) { selectTrack (i); };
    tracksArea.arrange.onClipsChanged = [this] { tracksArea.arrange.repaint(); };
    tracksArea.arrange.onOpenEditor   = [this] (te::EditItemID id, int mode) { openEditor (id, mode); };
    tracksArea.arrange.onClipSelected = [this] (te::EditItemID id)
    {
        if (editorOpen && id.isValid() && findMidiClip (edit.get(), id) != nullptr)
            openEditor (id, -1);   // follow selection, keep current mode
    };
    editorPanel.onClose = [this] { closeEditor(); };
    addChildComponent (editorPanel);
    viewport.setViewedComponent (&tracksArea, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    createNewEdit();

    setSize (1360, 760);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    tracksArea.rows.clear();
    tracksArea.arrange.edit = nullptr;
    editorPanel.setClip (nullptr, {}, false);

    if (edit != nullptr)
        edit->getTransport().stop (false, true);

    edit = nullptr;
}

//==============================================================================
void MainComponent::createNewEdit()
{
    auto file = projectFolder().getNonexistentChildFile ("Untitled", ".tracktionedit");
    auto newEdit = te::createEmptyEdit (engine, file);
    newEdit->playInStopEnabled = true;
    newEdit->ensureNumberOfAudioTracks (4);

    int n = 1;
    for (auto* t : te::getAudioTracks (*newEdit))
        t->setName ("Track " + juce::String (n++));

    if (auto tempo = newEdit->tempoSequence.getTempo (0))
        tempo->setBpm (120.0);

    newEdit->getTransport().looping = true;

    currentFile = file;
    setEdit (std::move (newEdit));
    autoLoadMyInstruments();
    setStatus ("New project. Pick an instrument per track, arm a track (R) and play your MIDI keyboard. Space = play/stop.");
}

void MainComponent::setEdit (std::unique_ptr<te::Edit> newEdit)
{
    tracksArea.rows.clear();
    tracksArea.arrange.edit = nullptr;
    tracksArea.arrange.selectedClip = {};
    editorPanel.setClip (nullptr, {}, false);
    closeEditor();

    if (edit != nullptr)
        edit->getTransport().stop (false, true);

    edit = std::move (newEdit);
    selectedTrack = 0;
    armed = false;

    if (edit == nullptr)
        return;

    edit->playInStopEnabled = true;
    edit->getTransport().ensureContextAllocated();

    if (auto tempo = edit->tempoSequence.getTempo (0))
        tempoSlider.setValue (tempo->getBpm(), juce::dontSendNotification);

    loopButton.setToggleState (edit->getTransport().looping.get(), juce::dontSendNotification);
    clickButton.setToggleState (edit->clickTrackEnabled.get(), juce::dontSendNotification);

    tracksArea.arrange.edit = edit.get();
    updateLoopRange();
    rebuildTracks();
    routeMidiToSelectedTrack();
}

te::AudioTrack* MainComponent::getTrack (int index) const
{
    if (edit == nullptr) return nullptr;
    auto tracks = te::getAudioTracks (*edit);
    return juce::isPositiveAndBelow (index, tracks.size()) ? tracks[index] : nullptr;
}

void MainComponent::rebuildTracks()
{
    tracksArea.rows.clear();

    if (edit == nullptr) return;

    auto tracks = te::getAudioTracks (*edit);
    selectedTrack = juce::jlimit (0, juce::jmax (0, tracks.size() - 1), selectedTrack);

    for (int i = 0; i < tracks.size(); ++i)
    {
        auto* row = tracksArea.rows.add (new TrackRow (*tracks[i], i));
        row->onSelect           = [this] (TrackRow& r) { selectTrack (r.index); };
        row->onChooseInstrument = [this] (TrackRow& r) { selectTrack (r.index); chooseInstrument (r); };
        row->onOpenInstrument   = [this] (TrackRow& r) { selectTrack (r.index); openInstrumentWindow (r.track); };
        row->onArm = [this] (TrackRow& r, bool on)
        {
            armed = on;
            selectTrack (r.index);   // also re-routes MIDI and applies arm state
        };
        row->setSelected (i == selectedTrack);
        row->setArmed (armed && i == selectedTrack);
        tracksArea.addAndMakeVisible (row);
    }

    tracksArea.arrange.selectedTrack = selectedTrack;
    resized();
}

void MainComponent::selectTrack (int index)
{
    if (getTrack (index) == nullptr) return;

    selectedTrack = index;

    for (auto* r : tracksArea.rows)
    {
        r->setSelected (r->index == selectedTrack);
        r->setArmed (armed && r->index == selectedTrack);
    }

    tracksArea.arrange.selectedTrack = selectedTrack;
    tracksArea.arrange.repaint();
    routeMidiToSelectedTrack();
}

void MainComponent::routeMidiToSelectedTrack()
{
    auto* track = getTrack (selectedTrack);
    if (track == nullptr) return;

    auto& dm = engine.getDeviceManager();

    for (int i = 0; i < dm.getNumMidiInDevices(); ++i)
        if (auto mip = dm.getMidiInDevice (i))
        {
            mip->setMonitorMode (te::InputDevice::MonitorMode::on);
            mip->setEnabled (true);
        }

    edit->getTransport().ensureContextAllocated();

    int numMidiInputs = 0;

    for (auto* inst : edit->getAllInputDevices())
    {
        if (inst->getInputDevice().getDeviceType() != te::InputDevice::physicalMidiDevice)
            continue;

        inst->setTarget (track->itemID, true, &edit->getUndoManager(), 0);
        inst->setRecordingEnabled (track->itemID, armed);
        ++numMidiInputs;
    }

    edit->restartPlayback();

    if (numMidiInputs == 0)
        setStatus ("No MIDI keyboard found. Connect one and click the track again.");
}

void MainComponent::updateLoopRange()
{
    if (edit == nullptr) return;

    const int bars = juce::jmax (1, loopLengthBox.getSelectedId());
    auto& ts = edit->tempoSequence;
    edit->getTransport().setLoopRange (te::TimeRange (ts.toTime (te::BeatPosition::fromBeats (0.0)),
                                                      ts.toTime (te::BeatPosition::fromBeats (bars * 4.0))));
    tracksArea.arrange.repaint();
}

//==============================================================================
void MainComponent::togglePlay()
{
    if (edit == nullptr) return;
    auto& tc = edit->getTransport();

    if (tc.isPlaying()) tc.stop (false, false);
    else                tc.play (false);
}

void MainComponent::stopOrReturnToZero()
{
    if (edit == nullptr) return;
    auto& tc = edit->getTransport();

    if (tc.isPlaying() || tc.isRecording())
        tc.stop (false, false);
    else
        tc.setPosition (te::TimePosition());

    tracksArea.arrange.repaint();
}

void MainComponent::toggleRecord()
{
    if (edit == nullptr) return;
    auto& tc = edit->getTransport();

    if (tc.isRecording())
    {
        tc.stop (false, false);
        juce::Timer::callAfterDelay (300, [safe = juce::Component::SafePointer<MainComponent> (this)]
        {
            if (safe != nullptr) safe->tracksArea.arrange.repaint();
        });
        return;
    }

    if (! armed)
    {
        armed = true;
        selectTrack (selectedTrack);
        setStatus ("Armed " + getTrack (selectedTrack)->getName() + " for recording.");
    }

    tc.record (false);
}

void MainComponent::addTrack()
{
    if (edit == nullptr) return;

    const int count = te::getAudioTracks (*edit).size();
    edit->ensureNumberOfAudioTracks (count + 1);

    if (auto* t = getTrack (count))
        t->setName ("Track " + juce::String (count + 1));

    selectedTrack = count;
    rebuildTracks();
    selectTrack (count);
}

//==============================================================================
juce::Array<juce::PluginDescription> MainComponent::getInstrumentPlugins()
{
    juce::Array<juce::PluginDescription> list;
    for (auto& d : engine.getPluginManager().knownPluginList.getTypes())
        if (d.isInstrument)
            list.add (d);
    return list;
}

void MainComponent::chooseInstrument (TrackRow& row)
{
    auto list = getInstrumentPlugins();

    juce::PopupMenu m;
    for (int i = 0; i < list.size(); ++i)
        m.addItem (i + 1, list[i].name + "   (" + list[i].manufacturerName + ", " + list[i].pluginFormatName + ")");

    if (list.isEmpty())
        m.addItem (-1, "No instruments found - scan for plug-ins first", false);

    m.addSeparator();
    m.addItem (1000, "Remove instrument");
    m.addItem (1001, "Scan for plug-ins...");

    const auto trackID = row.track.itemID;

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&row),
                     [this, list, trackID, safe = juce::Component::SafePointer<MainComponent> (this)] (int r)
    {
        if (safe == nullptr || edit == nullptr || r == 0) return;

        te::AudioTrack* track = nullptr;
        for (auto* t : te::getAudioTracks (*edit))
            if (t->itemID == trackID)
                track = t;

        if (track == nullptr) return;

        if (r == 1001)                          showPluginScanner();
        else if (r == 1000)                     removeInstruments (*track);
        else if (r >= 1 && r <= list.size())    loadInstrument (*track, list[r - 1]);

        for (auto* rw : tracksArea.rows)
            rw->refresh();
    });
}

void MainComponent::removeInstruments (te::AudioTrack& t)
{
    juce::Array<te::Plugin*> toRemove;
    for (auto p : t.pluginList.getPlugins())
        if (dynamic_cast<te::ExternalPlugin*> (&*p) != nullptr)
            toRemove.add (&*p);

    for (auto* p : toRemove)
        p->deleteFromParent();
}

void MainComponent::loadInstrument (te::AudioTrack& t, const juce::PluginDescription& desc)
{
    removeInstruments (t);

    if (auto plugin = edit->getPluginCache().createNewPlugin (te::ExternalPlugin::xmlTypeName, desc))
    {
        t.pluginList.insertPlugin (plugin, 0, nullptr);
        setStatus ("Loaded " + desc.name + " on " + t.getName() + ".");
        edit->restartPlayback();
    }
    else
    {
        setStatus ("Could not load " + desc.name + ".");
    }
}

void MainComponent::openInstrumentWindow (te::AudioTrack& t)
{
    if (auto* ep = findInstrument (t))
    {
        if (ep->windowState != nullptr)
            ep->windowState->showWindowExplicitly();
    }
    else
    {
        setStatus ("No instrument on " + t.getName() + " yet - click the instrument button first.");
    }
}

void MainComponent::autoLoadMyInstruments()
{
    // Puts JNO-600 on track 1 and DR-1010 on track 2 if they've been scanned
    auto list = getInstrumentPlugins();
    auto find = [&list] (juce::StringArray keys) -> const juce::PluginDescription*
    {
        for (auto& d : list)
            for (auto& k : keys)
                if (d.name.containsIgnoreCase (k))
                    return &d;
        return nullptr;
    };

    int loaded = 0;
    if (auto* jno = find ({ "JNO" }))
        if (auto* t = getTrack (0)) { loadInstrument (*t, *jno); t->setName ("JNO-600"); ++loaded; }

    if (auto* dr = find ({ "DR-1010", "DR1010", "DR 1010" }))
        if (auto* t = getTrack (1)) { loadInstrument (*t, *dr); t->setName ("DR-1010"); ++loaded; }

    for (auto* r : tracksArea.rows)
        r->refresh();

    if (loaded == 0 && list.isEmpty())
        setStatus ("No plug-ins known yet: click Plug-ins > Options > Scan for new or updated VST3 plug-ins.");
}

//==============================================================================
void MainComponent::showQuickStartMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Load JNO-600 + DR-1010 on tracks 1-2");
    m.addSeparator();
    m.addSectionHeader ("Insert 4-bar demo clip on selected track");
    m.addItem (11, "Chords (Am - F - C - G)");
    m.addItem (12, "Bass line");
    m.addItem (13, "Drum beat (GM notes 36/38/42)");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&demoButton),
                     [this, safe = juce::Component::SafePointer<MainComponent> (this)] (int r)
    {
        if (safe == nullptr) return;
        if (r == 1)       { autoLoadMyInstruments(); if (getInstrumentPlugins().isEmpty()) showPluginScanner(); }
        else if (r >= 11) insertDemoPattern (r - 10);
    });
}

void MainComponent::insertDemoPattern (int type)
{
    auto* track = getTrack (selectedTrack);
    if (track == nullptr) return;

    auto& ts = edit->tempoSequence;

    // place after the last clip on the track
    double startBeat = 0.0;
    for (auto* c : track->getClips())
        startBeat = juce::jmax (startBeat, ts.toBeats (c->getPosition().getEnd()).inBeats());
    startBeat = std::ceil (startBeat / 4.0) * 4.0;

    const te::TimeRange range (ts.toTime (te::BeatPosition::fromBeats (startBeat)),
                               ts.toTime (te::BeatPosition::fromBeats (startBeat + 16.0)));

    auto clip = track->insertMIDIClip (range, nullptr);
    if (clip == nullptr) return;

    auto& seq = clip->getSequence();
    auto add = [&seq] (int note, double beat, double len, int vel)
    {
        seq.addNote (note, te::BeatPosition::fromBeats (beat), te::BeatDuration::fromBeats (len), vel, 0, nullptr);
    };

    if (type == 1)
    {
        clip->setName ("Chords");
        const int chords[4][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 55, 60, 64 }, { 55, 59, 62 } };
        for (int bar = 0; bar < 4; ++bar)
            for (int n : chords[bar])
                add (n, bar * 4.0, 3.9, 90);
    }
    else if (type == 2)
    {
        clip->setName ("Bass");
        const int roots[4] = { 33, 29, 36, 31 };
        const double pattern[] = { 0.0, 0.75, 1.5, 2.0, 2.75, 3.5 };
        for (int bar = 0; bar < 4; ++bar)
            for (double p : pattern)
                add (roots[bar] + (p == 2.75 ? 12 : 0), bar * 4.0 + p, 0.4, 100);
    }
    else
    {
        clip->setName ("Drums");
        for (int bar = 0; bar < 4; ++bar)
        {
            const double b = bar * 4.0;
            for (double k : { 0.0, 1.5, 2.0 })  add (36, b + k, 0.25, 115);   // kick
            for (double s : { 1.0, 3.0 })       add (38, b + s, 0.25, 105);   // snare
            for (int h = 0; h < 8; ++h)         add (42, b + h * 0.5, 0.2, h % 2 == 0 ? 95 : 70); // hats
        }
    }

    // make sure the loop covers the new clip
    const int neededBars = (int) ((startBeat + 16.0) / 4.0);
    if (neededBars > loopLengthBox.getSelectedId())
    {
        for (int bars : { 1, 2, 4, 8, 16, 32 })
            if (bars >= neededBars) { loopLengthBox.setSelectedId (bars); break; }
    }

    tracksArea.arrange.selectedClip = clip->itemID;
    openEditor (clip->itemID, type == 3 ? 2 : 1);
    setStatus ("Inserted demo clip on " + track->getName() + ". Press Play (or Space).");
}

//==============================================================================
void MainComponent::showAudioSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent (engine.getDeviceManager().deviceManager,
                                                             0, 2, 0, 8, false, false, true, false);
    selector->setSize (560, 420);

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle = "Audio settings (choose ASIO for lowest latency)";
    o.dialogBackgroundColour = colours::panel;
    o.useNativeTitleBar = true;
    o.resizable = true;
    o.launchAsync();
}

void MainComponent::showPluginScanner()
{
    auto& pm = engine.getPluginManager();
    auto deadMans = projectFolder().getChildFile ("RecentlyCrashedPluginsList");

    auto* list = new juce::PluginListComponent (pm.pluginFormatManager, pm.knownPluginList, deadMans, nullptr);
    list->setSize (760, 520);

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (list);
    o.dialogTitle = "Plug-ins  -  Options > Scan for new or updated VST3 plug-ins";
    o.dialogBackgroundColour = colours::panel;
    o.useNativeTitleBar = true;
    o.resizable = true;
    o.launchAsync();
}

//==============================================================================
void MainComponent::saveProject()
{
    if (edit == nullptr) return;

    chooser = std::make_unique<juce::FileChooser> ("Save project", currentFile, "*.tracktionedit");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File() || edit == nullptr) return;

        f = f.withFileExtension ("tracktionedit");
        edit->editFileRetriever = [f] { return f; };

        if (te::EditFileOperations (*edit).save (true, true, false))
        {
            currentFile = f;
            setStatus ("Saved " + f.getFullPathName());
        }
        else
        {
            setStatus ("Could not save " + f.getFullPathName());
        }
    });
}

void MainComponent::openProject()
{
    chooser = std::make_unique<juce::FileChooser> ("Open project", projectFolder(), "*.tracktionedit");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (! f.existsAsFile()) return;

        setEdit (nullptr);   // release the old edit before loading
        auto loaded = te::loadEditFromFile (engine, f);

        if (loaded == nullptr)
        {
            setStatus ("Could not open " + f.getFileName());
            createNewEdit();
            return;
        }

        currentFile = f;
        setEdit (std::move (loaded));
        setStatus ("Opened " + f.getFileName());
    });
}

//==============================================================================
void MainComponent::openEditor (te::EditItemID id, int mode)
{
    auto* mc = findMidiClip (edit.get(), id);
    if (mc == nullptr)
    {
        setStatus ("Select a MIDI clip first (double-click an empty spot in a track to create one).");
        return;
    }

    bool step = false;
    if (mode == 2)       step = true;
    else if (mode == 1)  step = false;
    else if (mode == -1) step = editorPanel.isStepMode();
    else
    {
        // auto: drum machines open in the step editor
        juce::String names = mc->getName() + " " + mc->getTrack()->getName();
        if (auto* at = dynamic_cast<te::AudioTrack*> (mc->getTrack()))
            for (auto p : at->pluginList.getPlugins())
                names << " " << p->getName();
        step = names.containsIgnoreCase ("DR-") || names.containsIgnoreCase ("DR1") || names.containsIgnoreCase ("drum");
    }

    tracksArea.arrange.selectedClip = id;
    editorPanel.setClip (edit.get(), id, step);
    editorOpen = true;
    editorPanel.setVisible (true);
    resized();
}

void MainComponent::closeEditor()
{
    editorOpen = false;
    editorPanel.setVisible (false);
    resized();
}

void MainComponent::setStatus (const juce::String& s)
{
    statusLabel.setText (s, juce::dontSendNotification);
}

bool MainComponent::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey)               { togglePlay(); return true; }
    if (k == juce::KeyPress::returnKey)              { stopOrReturnToZero(); return true; }
    if (k.getTextCharacter() == 'r' || k.getTextCharacter() == 'R') { toggleRecord(); return true; }
    if (k == juce::KeyPress::deleteKey || k == juce::KeyPress::backspaceKey) { tracksArea.arrange.deleteSelectedClip(); return true; }
    if (k == juce::KeyPress ('d', juce::ModifierKeys::commandModifier, 0)) { tracksArea.arrange.duplicateSelectedClip(); return true; }
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0)) { undoButton.triggerClick(); return true; }
    if (k == juce::KeyPress ('y', juce::ModifierKeys::commandModifier, 0)) { redoButton.triggerClick(); return true; }
    if (k == juce::KeyPress ('e', juce::ModifierKeys::commandModifier, 0)) { editorOpen ? closeEditor() : openEditor (tracksArea.arrange.selectedClip, 0); return true; }
    return false;
}

void MainComponent::timerCallback()
{
    if (edit == nullptr) return;

    auto& tc = edit->getTransport();
    auto& ts = edit->tempoSequence;

    const double beats = ts.toBeats (tc.getPosition()).inBeats();
    const int bar  = (int) std::floor (beats / 4.0) + 1;
    const int beat = (int) std::floor (std::fmod (beats, 4.0)) + 1;
    const int tick = (int) std::floor (std::fmod (beats, 1.0) * 100.0);
    positionLabel.setText (juce::String (bar).paddedLeft ('0', 3) + " . " + juce::String (beat)
                             + " . " + juce::String (tick).paddedLeft ('0', 2),
                           juce::dontSendNotification);

    playButton.setToggleState (tc.isPlaying(), juce::dontSendNotification);
    recordButton.setToggleState (tc.isRecording(), juce::dontSendNotification);

    tracksArea.arrange.repaint();
    editorPanel.tick();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
    g.setColour (colours::panel);
    g.fillRect (0, 0, getWidth(), 96);
    g.setColour (colours::accent);
    g.fillRect (0, 94, getWidth(), 2);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (96).reduced (10, 8);

    // row 1: title, transport, position, tempo, loop
    auto row1 = top.removeFromTop (38);
    titleLabel.setBounds (row1.removeFromLeft (150));
    for (auto* b : { &playButton, &stopButton, &recordButton, &loopButton, &clickButton })
    {
        b->setBounds (row1.removeFromLeft (62).reduced (2));
    }
    row1.removeFromLeft (10);
    positionLabel.setBounds (row1.removeFromLeft (170).reduced (2));
    row1.removeFromLeft (10);
    tempoLabel.setBounds (row1.removeFromLeft (40));
    tempoSlider.setBounds (row1.removeFromLeft (130).reduced (2, 4));
    loopLabel.setBounds (row1.removeFromLeft (44));
    loopLengthBox.setBounds (row1.removeFromLeft (90).reduced (2, 6));
    zoomLabel.setBounds (row1.removeFromLeft (44));
    zoomBox.setBounds (row1.removeFromLeft (90).reduced (2, 6));

    top.removeFromTop (6);

    // row 2: project / setup buttons + status
    auto row2 = top.removeFromTop (32);
    for (auto* b : { &newButton, &openButton, &saveButton, &undoButton, &redoButton })
        b->setBounds (row2.removeFromLeft (60).reduced (2));
    row2.removeFromLeft (10);
    for (auto* b : { &addTrackButton, &demoButton, &pluginsButton, &audioButton })
        b->setBounds (row2.removeFromLeft (100).reduced (2));
    row2.removeFromLeft (10);
    statusLabel.setBounds (row2);

    if (editorOpen)
    {
        const int eh = juce::jlimit (220, 520, r.getHeight() / 2);
        editorPanel.setBounds (r.removeFromBottom (eh));
    }
    viewport.setBounds (r);
    const int contentHeight = rulerHeight + tracksArea.rows.size() * rowHeight + 40;
    tracksArea.setBounds (0, 0, viewport.getMaximumVisibleWidth(), juce::jmax (contentHeight, r.getHeight()));
}
