#pragma once
#include <JuceHeader.h>
#include "EngineUI.h"
#include "Theme.h"
#include "Effects.h"
#include "ArrangeView.h"
#include "MidiEditors.h"
#include "MixerView.h"

using namespace ldaw;

//==============================================================================
// One track header: name, instrument, arm, mute, solo, volume, pan
//==============================================================================
class TrackRow : public juce::Component
{
public:
    TrackRow (te::AudioTrack& t, int index);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void refresh();
    void setSelected (bool);
    void setArmed (bool);

    te::AudioTrack& track;
    const int index;

    std::function<void (TrackRow&)>       onSelect;
    std::function<void (TrackRow&)>       onChooseInstrument;
    std::function<void (TrackRow&)>       onOpenInstrument;
    std::function<void (TrackRow&, bool)> onArm;

private:
    bool selected = false;
    juce::Label      nameLabel;
    juce::TextButton instrumentButton, editButton { "Edit" },
                     armButton { "R" }, muteButton { "M" }, soloButton { "S" };
    juce::Slider     volumeSlider, panSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRow)
};

//==============================================================================
class TracksArea : public juce::Component
{
public:
    TracksArea();
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::OwnedArray<TrackRow> rows;
    ArrangeView arrange;
};

//==============================================================================
class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    te::Engine engine { "L-DAW", std::make_unique<AppUIBehaviour>(), nullptr };
    std::unique_ptr<te::Edit> edit;
    juce::File currentFile;

    int  selectedTrack = 0;
    bool armed = false;
    bool mixerShown = false;
    int  tickCount = 0;

    juce::TooltipWindow tooltips { this, 700 };

    juce::TextButton playButton { "Play" }, stopButton { "Stop" }, recordButton { "Rec" },
                     loopButton { "Loop" }, clickButton { "Click" },
                     addTrackButton { "+ Track" }, demoButton { "Quick start" },
                     audioButton { "Audio / MIDI" }, pluginsButton { "Plug-ins" },
                     newButton { "New" }, openButton { "Open" }, saveButton { "Save" },
                     undoButton { "Undo" }, redoButton { "Redo" },
                     arrangeTab { "Arrange" }, mixerTab { "Mixer" };
    juce::Slider     tempoSlider;
    juce::ComboBox   loopLengthBox, zoomBox;
    juce::Label      tempoLabel { {}, "BPM" }, loopLabel { {}, "Loop" }, zoomLabel { {}, "View" },
                     positionLabel, statusLabel, titleLabel;
    juce::Viewport   viewport;
    TracksArea       tracksArea;
    MixerView        mixerView;
    MidiEditorPanel  editorPanel;
    bool             editorOpen = false;
    std::unique_ptr<juce::FileChooser> chooser;

    void createNewEdit();
    void setEdit (std::unique_ptr<te::Edit>);
    void rebuildTracks();
    void selectTrack (int index);
    void routeMidiToSelectedTrack();
    void updateLoopRange();
    void togglePlay();
    void stopOrReturnToZero();
    void toggleRecord();
    void addTrack();
    void chooseInstrument (TrackRow&);
    void openInstrumentWindow (te::AudioTrack&);
    void loadInstrument (te::AudioTrack&, const juce::PluginDescription&);
    void removeInstruments (te::AudioTrack&);
    void showQuickStartMenu();
    void insertDemoPattern (int type);
    void autoLoadMyInstruments();
    void showAudioSettings();
    void showPluginScanner();
    void saveProject();
    void openProject();
    void setStatus (const juce::String&);
    void openEditor (te::EditItemID, int mode);
    void closeEditor();
    void showMixer (bool);

    te::AudioTrack* getTrack (int index) const;
    juce::Array<juce::PluginDescription> getInstrumentPlugins();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
