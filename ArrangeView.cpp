#include "ArrangeView.h"

using namespace ldaw;

double ArrangeView::pixelsPerBeat() const     { return juce::jmax (1, getWidth()) / (double) juce::jmax (1, barsVisible * 4); }
double ArrangeView::xToBeats (float x) const  { return juce::jmax (0.0, x / pixelsPerBeat()); }
float  ArrangeView::beatsToX (double b) const { return (float) (b * pixelsPerBeat()); }
int    ArrangeView::numTracks() const         { return edit != nullptr ? te::getAudioTracks (*edit).size() : 0; }

int ArrangeView::yToTrack (int y) const
{
    return juce::jlimit (0, juce::jmax (0, numTracks() - 1), (y - rulerHeight) / rowHeight);
}

void ArrangeView::select (te::EditItemID id)
{
    selectedClip = id;
    if (onClipSelected) onClipSelected (id);
    repaint();
}

//==============================================================================
void ArrangeView::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
    if (edit == nullptr) return;

    auto& tc = edit->getTransport();
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();
    const auto tracks = te::getAudioTracks (*edit);

    // lanes
    for (int i = 0; i < tracks.size(); ++i)
    {
        const float y = (float) (rulerHeight + i * rowHeight);
        g.setColour (i == selectedTrack ? colours::panel.brighter (0.06f)
                                        : (i % 2 == 0 ? colours::background : colours::background.brighter (0.03f)));
        g.fillRect (0.0f, y, w, (float) rowHeight);
    }

    // loop range
    if (tc.looping.get())
    {
        auto lr = tc.getLoopRange();
        const float x1 = beatsToX (beatOf (*edit, lr.getStart()));
        const float x2 = beatsToX (beatOf (*edit, lr.getEnd()));
        g.setColour (colours::accent.withAlpha (0.07f));
        g.fillRect (x1, (float) rulerHeight, x2 - x1, h - rulerHeight);
        g.setColour (colours::accent.withAlpha (0.8f));
        g.fillRect (x1, (float) rulerHeight - 5.0f, x2 - x1, 5.0f);
    }

    // grid
    for (int beat = 0; beat <= barsVisible * 4; ++beat)
    {
        g.setColour (juce::Colours::white.withAlpha ((beat % 4) == 0 ? 0.14f : 0.05f));
        g.drawVerticalLine ((int) beatsToX (beat), (float) rulerHeight, h);
    }

    // ruler
    g.setColour (colours::panel);
    g.fillRect (0.0f, 0.0f, w, (float) rulerHeight - 5.0f);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    for (int bar = 0; bar < barsVisible; ++bar)
    {
        const float x = beatsToX (bar * 4);
        g.setColour (colours::dimText);
        g.drawVerticalLine ((int) x, 0.0f, (float) rulerHeight - 5.0f);
        g.drawText (juce::String (bar + 1), (int) x + 4, 2, 40, 16, juce::Justification::left);
    }

    // clips
    for (int i = 0; i < tracks.size(); ++i)
    {
        const float laneY = (float) (rulerHeight + i * rowHeight);

        for (auto* clip : tracks[i]->getClips())
        {
            auto pos = clip->getPosition();
            const double sb = beatOf (*edit, pos.getStart());
            const double eb = beatOf (*edit, pos.getEnd());
            juce::Rectangle<float> r (beatsToX (sb), laneY + 3.0f,
                                      juce::jmax (3.0f, beatsToX (eb) - beatsToX (sb)), (float) rowHeight - 6.0f);

            const bool isSel = clip->itemID == selectedClip;
            const bool isDragged = drag.active && drag.moved && ! drag.copy && clip->itemID == drag.clipID;

            g.setColour (colours::clip.withAlpha (isDragged ? 0.35f : 0.85f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (isSel ? colours::accent : colours::clip.brighter (0.5f));
            g.drawRoundedRectangle (r, 4.0f, isSel ? 2.0f : 1.0f);

            g.setColour (juce::Colours::white);
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText (clip->getName(), r.reduced (4.0f, 1.0f).removeFromTop (13.0f), juce::Justification::left, true);

            if (auto* mc = dynamic_cast<te::MidiClip*> (clip))
            {
                auto& notes = mc->getSequence().getNotes();
                if (notes.size() > 0)
                {
                    int lo = 127, hi = 0;
                    for (auto* n : notes) { lo = juce::jmin (lo, n->getNoteNumber()); hi = juce::jmax (hi, n->getNoteNumber()); }
                    const int span = juce::jmax (12, hi - lo + 1);
                    auto noteArea = r.reduced (2.0f).withTrimmedTop (13.0f);
                    const float nh = juce::jmax (1.5f, noteArea.getHeight() / (float) span);

                    g.setColour (juce::Colours::white.withAlpha (0.85f));
                    for (auto* n : notes)
                    {
                        const double ns = sb + n->getStartBeat().inBeats();
                        if (ns >= eb) continue;
                        const double nl = juce::jmin (n->getLengthBeats().inBeats(), eb - ns);
                        const float ny = noteArea.getBottom() - (float) (n->getNoteNumber() - lo + 1) * nh;
                        g.fillRect (beatsToX (ns), ny, juce::jmax (1.5f, (float) (nl * pixelsPerBeat()) - 1.0f), nh);
                    }
                }
            }

            // resize handle
            if (isSel)
            {
                g.setColour (colours::accent);
                g.fillRect (r.getRight() - 4.0f, r.getY() + 4.0f, 3.0f, r.getHeight() - 8.0f);
            }
        }
    }

    // drag ghost
    if (drag.active && drag.moved)
    {
        const double s = drag.resize ? drag.start0 : drag.newStart;
        const double e = drag.resize ? drag.newEnd : drag.newStart + (drag.end0 - drag.start0);
        const int    t = drag.resize ? drag.track0 : drag.newTrack;
        juce::Rectangle<float> ghost (beatsToX (s), (float) (rulerHeight + t * rowHeight) + 3.0f,
                                      beatsToX (e) - beatsToX (s), (float) rowHeight - 6.0f);
        g.setColour (colours::accent.withAlpha (0.25f));
        g.fillRoundedRectangle (ghost, 4.0f);
        g.setColour (colours::accent);
        g.drawRoundedRectangle (ghost, 4.0f, 2.0f);
        if (drag.copy)
            g.drawText ("+ copy", ghost.reduced (4.0f), juce::Justification::bottomRight, false);
    }

    // playhead
    const float px = beatsToX (beatOf (*edit, tc.getPosition()));
    g.setColour (tc.isRecording() ? colours::record : juce::Colours::white);
    g.drawVerticalLine ((int) px, 0.0f, h);
}

//==============================================================================
te::Clip* ArrangeView::clipAt (juce::Point<int> p, int& trackIndex) const
{
    trackIndex = (p.y - rulerHeight) / rowHeight;
    if (edit == nullptr || p.y < rulerHeight) return nullptr;

    auto tracks = te::getAudioTracks (*edit);
    if (! juce::isPositiveAndBelow (trackIndex, tracks.size())) return nullptr;

    const double beat = xToBeats ((float) p.x);

    for (auto* clip : tracks[trackIndex]->getClips())
    {
        auto pos = clip->getPosition();
        if (beat >= beatOf (*edit, pos.getStart()) && beat < beatOf (*edit, pos.getEnd()))
            return clip;
    }
    return nullptr;
}

void ArrangeView::mouseMove (const juce::MouseEvent& e)
{
    int ti = 0;
    if (auto* c = clipAt (e.getPosition(), ti))
    {
        const float right = beatsToX (beatOf (*edit, c->getPosition().getEnd()));
        setMouseCursor (e.x > right - 7 ? juce::MouseCursor::LeftRightResizeCursor
                                        : juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }
}

void ArrangeView::mouseDown (const juce::MouseEvent& e)
{
    drag = {};
    if (edit == nullptr) return;

    // Ruler -> move playhead (snaps to beat, Shift = free)
    if (e.y < rulerHeight)
    {
        double beat = xToBeats ((float) e.x);
        if (! e.mods.isShiftDown()) beat = std::floor (beat + 0.5);
        edit->getTransport().setPosition (timeOf (*edit, beat));
        repaint();
        return;
    }

    int trackIndex = 0;
    auto* clip = clipAt (e.getPosition(), trackIndex);

    if (onLaneClicked && juce::isPositiveAndBelow (trackIndex, numTracks()))
        onLaneClicked (trackIndex);

    if (clip == nullptr)
    {
        select ({});
        return;
    }

    select (clip->itemID);

    if (e.mods.isPopupMenu())
    {
        showClipMenu (clip->itemID);
        return;
    }

    drag.active   = true;
    drag.clipID   = clip->itemID;
    drag.track0   = drag.newTrack = trackIndex;
    drag.start0   = drag.newStart = beatOf (*edit, clip->getPosition().getStart());
    drag.end0     = drag.newEnd   = beatOf (*edit, clip->getPosition().getEnd());
    drag.downBeat = xToBeats ((float) e.x);
    drag.resize   = e.x > beatsToX (drag.end0) - 7;
}

void ArrangeView::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag.active) return;

    drag.moved = drag.moved || e.getDistanceFromDragStart() > 3;
    if (! drag.moved) return;

    const double snap  = e.mods.isShiftDown() ? 0.25 : 1.0;      // beat, Shift = 1/16
    const double delta = std::round ((xToBeats ((float) e.x) - drag.downBeat) / snap) * snap;

    if (drag.resize)
    {
        drag.newEnd = juce::jmax (drag.start0 + 0.25, drag.end0 + delta);
    }
    else
    {
        drag.newStart = juce::jmax (0.0, drag.start0 + delta);
        drag.newTrack = yToTrack (e.y);
        drag.copy     = e.mods.isCtrlDown() || e.mods.isAltDown();
    }
    repaint();
}

void ArrangeView::mouseUp (const juce::MouseEvent&)
{
    if (drag.active && drag.moved)
        applyDrag();

    drag = {};
    repaint();
}

void ArrangeView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (edit == nullptr || e.y < rulerHeight) return;

    int trackIndex = 0;
    if (auto* clip = clipAt (e.getPosition(), trackIndex))
    {
        if (onOpenEditor) onOpenEditor (clip->itemID, 0);
        return;
    }

    // empty lane -> create a new 1-bar MIDI clip
    auto tracks = te::getAudioTracks (*edit);
    if (! juce::isPositiveAndBelow (trackIndex, tracks.size())) return;

    const double startBeat = std::floor (xToBeats ((float) e.x) / 4.0) * 4.0;
    edit->getUndoManager().beginNewTransaction();

    if (auto nc = tracks[trackIndex]->insertMIDIClip (te::TimeRange (timeOf (*edit, startBeat), timeOf (*edit, startBeat + 4.0)), nullptr))
    {
        nc->setName ("Clip");
        select (nc->itemID);
        if (onClipsChanged) onClipsChanged();
        if (onOpenEditor) onOpenEditor (nc->itemID, 0);
    }
}

//==============================================================================
void ArrangeView::applyDrag()
{
    auto* clip = te::findClipForID (*edit, drag.clipID);
    if (clip == nullptr) return;

    edit->getUndoManager().beginNewTransaction();

    if (drag.resize)
    {
        clip->setEnd (timeOf (*edit, drag.newEnd), true);
    }
    else
    {
        auto tracks = te::getAudioTracks (*edit);
        auto newStart = timeOf (*edit, drag.newStart);

        if (drag.copy || drag.newTrack != drag.track0)
        {
            auto* mc = dynamic_cast<te::MidiClip*> (clip);
            if (mc != nullptr && juce::isPositiveAndBelow (drag.newTrack, tracks.size()))
            {
                if (auto* nc = copyMidiClip (*mc, *tracks[drag.newTrack], newStart))
                {
                    if (! drag.copy)
                        clip->removeFromParent();
                    select (nc->itemID);
                }
            }
        }
        else
        {
            clip->setStart (newStart, false, true);
        }
    }

    if (onClipsChanged) onClipsChanged();
}

void ArrangeView::deleteSelectedClip()
{
    if (edit == nullptr) return;
    if (auto* clip = te::findClipForID (*edit, selectedClip))
    {
        edit->getUndoManager().beginNewTransaction();
        clip->removeFromParent();
        select ({});
        if (onClipsChanged) onClipsChanged();
    }
}

void ArrangeView::duplicateSelectedClip()
{
    auto* mc = findMidiClip (edit, selectedClip);
    if (mc == nullptr) return;

    if (auto* track = dynamic_cast<te::AudioTrack*> (mc->getTrack()))
    {
        edit->getUndoManager().beginNewTransaction();
        if (auto* nc = copyMidiClip (*mc, *track, mc->getPosition().getEnd()))
            select (nc->itemID);
        if (onClipsChanged) onClipsChanged();
    }
}

void ArrangeView::showClipMenu (te::EditItemID id)
{
    juce::PopupMenu m;
    m.addItem (1, "Open in piano roll");
    m.addItem (2, "Open in step editor (drums)");
    m.addSeparator();
    m.addItem (3, "Duplicate   (Ctrl+D)");
    m.addItem (4, "Delete      (Del)");

    m.showMenuAsync (juce::PopupMenu::Options(),
                     [safe = juce::Component::SafePointer<ArrangeView> (this), id] (int r)
    {
        if (safe == nullptr) return;
        safe->selectedClip = id;
        if (r == 1 && safe->onOpenEditor) safe->onOpenEditor (id, 1);
        if (r == 2 && safe->onOpenEditor) safe->onOpenEditor (id, 2);
        if (r == 3) safe->duplicateSelectedClip();
        if (r == 4) safe->deleteSelectedClip();
    });
}
