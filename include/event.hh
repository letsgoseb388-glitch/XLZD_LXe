#ifndef EVENT_HH
#define EVENT_HH

#include "G4UserEventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Event.hh"

// Writes the event-level ntuples (one row per event, including events with no LXe deposit)
// and the list of decay gammas (ion mode).
class MyEventAction : public G4UserEventAction
{
public:
    MyEventAction();
    ~MyEventAction();
    virtual void EndOfEventAction(const G4Event*);
};

#endif
