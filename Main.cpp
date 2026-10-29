#include <JuceHeader.h>
#include "MainComponent.h"

class LDAWApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "L-DAW"; }
    const juce::String getApplicationVersion() override { return "0.2.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypefaceName ("Segoe UI");
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override             { mainWindow = nullptr; }
    void systemRequestedQuit() override  { quit(); }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : juce::DocumentWindow (name, juce::Colour (0xff1b1c1f), juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
            setResizeLimits (1100, 500, 4000, 3000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (LDAWApplication)
