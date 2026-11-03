#ifndef EVENTINFO_HH
#define EVENTINFO_HH

#include "globals.hh"
#include <utility>
#include <vector>

// Per-event bookkeeping shared by the generator, stacking, stepping and event actions.
// The generator resets and fills the source part at the start of every event; the
// event action writes it out (and the gamma list) in EndOfEventAction.
// thread_local so the same code stays correct if the run manager is ever made multithreaded.
struct MyEventInfo
{
    // source (filled by the generator)
    G4double E0 = 0.;                         // primary gamma energy in MeV (0 in ion mode)
    G4double dirX = 0., dirY = 0., dirZ = 0.; // primary direction (0,0,0 in ion mode)
    G4double weight = 1.;                     // importance weight (mean 1 under the sampling distribution)

    // radioactive-decay gammas (filled by the stacking action, ion mode only)
    G4int    nGamma = 0;
    G4double eGammaSum = 0.;                  // MeV
    std::vector<std::pair<G4double, G4double> > gammas;   // (energy MeV, global time ns)

    // filled by the stepping action
    G4double eLXe = 0.;                       // total energy deposited in LXe this event (MeV), independent of /detector/energyCut

    void Reset() { *this = MyEventInfo(); }
};

inline MyEventInfo& EventInfo()
{
    static thread_local MyEventInfo info;
    return info;
}

#endif
