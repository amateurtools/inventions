// BeatDivisionTable.h
#pragma once

#include <array>
#include <cstddef>

struct BeatDivision {
    float gridDivision;          // in beats (e.g., 0.25 = sixteenth note @ 4/4)
    const char* label;           // Display ("1/16", "1/16T", "1/16D")
};

// Comprehensive, ordered array
constexpr std::array<BeatDivision, 21> kBeatDivisions = {{
    { 4.0f,    "1/1"   }, // whole note
    { 3.0f,    "1/1T"  }, // whole triplet
    { 2.0f,    "1/2"   }, // half note
    { 1.5f,    "1/2D"  }, // half dotted
    { 1.3333f, "1/2T"  }, // half triplet
    { 1.0f,    "1/4"   }, // quarter note
    { 0.75f,   "1/4D"  }, // quarter dotted
    { 0.6667f, "1/4T"  }, // quarter triplet
    { 0.5f,    "1/8"   }, // eighth
    { 0.375f,  "1/8D"  }, // eighth dotted
    { 0.3333f, "1/8T"  }, // eighth triplet
    { 0.25f,   "1/16"  }, // sixteenth
    { 0.1875f, "1/16D" }, // sixteenth dotted
    { 0.1667f, "1/16T" }, // sixteenth triplet
    { 0.125f,  "1/32"  }, // thirty-second
    { 0.09375f,"1/32D" }, // thirty-second dotted
    { 0.0833f, "1/32T" }, // thirty-second triplet
    { 0.0625f, "1/64"  }, // sixty-fourth
    { 0.04688f,"1/64D" }, // sixty-fourth dotted
    { 0.0417f, "1/64T" }, // sixty-fourth triplet
    // Extend further as desired
}};
