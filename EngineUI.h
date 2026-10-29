#pragma once
#include <JuceHeader.h>

namespace te = tracktion;

//==============================================================================
// Floating window that shows a plug-in's own editor (e.g. the JNO-600 panel)
//==============================================================================
class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow (te::PluginWindowState& s, te::Plugin& p)
        : juce::DocumentWindow (p.getName(), juce::Colour (0xff2a2b2f),
                                juce::DocumentWindow::closeButton, true),
          state (s)
    {
        setUsingNativeTitleBar (true);

        juce::Component* content = nullptr;

        if (auto* ep = dynamic_cast<te::ExternalPlugin*> (&p))
            if (auto* inst = ep->getAudioPluginInstance())
                content = inst->hasEditor() ? static_cast<juce::Component*> (inst->createEditorIfNeeded())
                                            : static_cast<juce::Component*> (new juce::GenericAudioProcessorEditor (*inst));

        if (content == nullptr)
        {
            auto* l = new juce::Label ({}, "No editor available for " + p.getName());
            l->setJustificationType (juce::Justification::centred);
            l->setSize (360, 80);
            content = l;
        }

        setContentOwned (content, true);

        static int cascade = 0;
        cascade = (cascade + 1) % 8;
        setTopLeftPosition (120 + cascade * 30, 120 + cascade * 30);
    }

    ~PluginWindow() override
    {
        clearContentComponent();   // delete the editor before the window goes
    }

    void closeButtonPressed() override
    {
        state.closeWindowExplicitly();   // the engine deletes this window
    }

private:
    te::PluginWindowState& state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};

//==============================================================================
// Tells Tracktion Engine how to build plug-in windows in this app
//==============================================================================
struct AppUIBehaviour : public te::UIBehaviour
{
    std::unique_ptr<juce::Component> createPluginWindow (te::PluginWindowState& s) override
    {
        if (auto* ws = dynamic_cast<te::Plugin::WindowState*> (&s))
            return std::make_unique<PluginWindow> (s, ws->plugin);

        return {};
    }
};
