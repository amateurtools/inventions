class BandSelectorOverlay : public juce::Component
{
public:
    struct Crosshair { float freq, thresh; }; // normalized 0..1
    Crosshair bands[2]; // or std::vector for N bands

    int draggingIndex = -1;

    void paint(juce::Graphics& g) override {
        for (const auto& band : bands)
        {
            auto pt = spectrumToLocal(band.freq, band.thresh);
            g.setColour(juce::Colours::orange);
            g.drawEllipse(pt.x - r, pt.y - r, 2*r, 2*r, 2.0f);
            g.drawLine(...); // crossbars
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (int i=0; i<2; ++i)
            if (clickedNear(e.position, bands[i])) draggingIndex = i;
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (draggingIndex >= 0)
        {
            bands[draggingIndex] = localToSpectrum(e.position, ...);
            // --- Collision avoidance: check against the other band, push away if too close
            if (areBandsTooClose(bands[0], bands[1]))
                bands[draggingIndex] = repelFrom(bands[draggingIndex], bands[1]);
            repaint();
        }
    }
};

// Your concept for the GUI—two circular, draggable crosshairs (XY controls) overlaid on the spectrum analyzer, each representing threshold and center frequency for a detector, with collision avoidance so they never overlap—is both innovative and user-friendly. Here’s how this idea stands out and how you can approach it in JUCE:
// Why this design is strong

//     Intuitive Control: Users can adjust both center frequency (X axis) and threshold (Y axis) directly where they're visually analyzing the spectrum, creating a tight correlation between what they see and what they hear.

//     Clear Visual Feedback: The crosshairs over the spectrum make it obvious what each filter is focusing on, improving usability over abstract parameter knobs or sliders.

//     Collision Awareness: Preventing overlap ensures each control remains distinct, avoiding confusion and accidental parameter changes—essential for precise band-based triggers.

//     Extensible: If you decide to support more than two bands, your paradigm scales naturally: more crosshairs, same rules, easy to manage visually.

// How to implement in JUCE (summary of approach)

//     Custom Component:
//     Implement a custom Component (e.g., BandSelectorOverlay), which receives mouse events and manages N crosshair positions over your analyzer view.

//     Draw crosshairs and labels:
//     In paint(), render each crosshair at the current (freq, threshold) mapped to spectrum coordinates, and use g.drawEllipse(...) with optional cross lines. JUCE’s graphics context is well-suited for this

//     .

//     Mouse handling (dragging):
//     On mouseDown/mouseDrag, check which crosshair is selected (distance to mouse), then move it along XY axes, clamping frequency/threshold to valid spectrum bounds.

//     Collision avoidance:
//     In your drag logic, if a crosshair’s new position would collide (i.e. get within a certain minimum distance of the other), snap or repel it away—so they never overlap.

//     Parameter communication:
//     Update plugin parameters (center frequency, threshold) as crosshair positions change, and pull their positions from parameter values for repaint.

//     (Optional) Tooltips/labels:
//     Show parameter labels or tooltip popups on hover or selection for extra clarity.

// Relevant JUCE tools and references

//     JUCE mouse event handling: Makes it straightforward to assign pick/grab logic to specific points or shapes in your overlay

// .

// Custom drawing: Use JUCE’s Graphics drawing for flexible, high-fidelity visuals over your analyzer

// .

// Component structure: Place the overlay as a child component over your spectrum display for event routing and Z-ordering.

