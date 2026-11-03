#ifndef STACKING_HH
#define STACKING_HH

#include "G4UserStackingAction.hh"
#include "G4ClassificationOfNewTrack.hh"

// Ion mode only (does nothing for tracks that are not created by radioactive decay):
// tallies the gammas emitted by the decay and removes the decay products that must not reach
// the LXe from a source sitting on the (vacuum) skin: betas, neutrinos, alphas, conversion
// electrons and ground-state daughter ions. Excited daughter ions are kept so that they de-excite.
class MyStackingAction : public G4UserStackingAction
{
public:
    MyStackingAction();
    ~MyStackingAction();
    virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track*);
};

#endif
