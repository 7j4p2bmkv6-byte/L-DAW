#pragma once
#include "Theme.h"

//==============================================================================
// Timeline: ruler, loop range, clips (move / copy / resize), playhead
//==============================================================================
class ArrangeView : public juce::Component
{
public:
    ArrangeView() { setWantsKeyboardFocus (false); }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void deleteSelectedClip();
    void duplicateSelectedClip();

    te::Edit* edit = nullptr;
    int selectedTrack = 0;
    int barsVisible = 16;
    te::EditItemID selectedClip;

    std::function<void (int)>                 onLaneClicked;
    std::function<void (te::EditItemID)>      onClipSelected;
    std::function<void (te::EditItemID, int)> onOpenEditor;    // mode 0 = auto, 1 = piano roll, 2 = step
    std::function<void()>                     onClipsChanged;

private:
    struct DragState
    {
        bool active = false, moved = false, resize = false, copy = false;
        te::EditItemID clipID;
        int track0 = 0, newTrack = 0;
        double start0 = 0, end0 = 0, downBeat = 0, newStart = 0, newEnd = 0;
    } drag;

    double pixelsPerBeat() const;
    double xToBeats (float x) const;
    float  beatsToX (double beats) const;
    int    yToTrack (int y) const;
    int    numTracks() const;
    te::Clip* clipAt (juce::Point<int>, int& trackIndex) const;
    void   applyDrag();
    void   showClipMenu (te::EditItemID);
    void   select (te::EditItemID);
};
