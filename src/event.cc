#include "event.hh"
#include "eventinfo.hh"

#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

MyEventAction::MyEventAction()
{}

MyEventAction::~MyEventAction()
{}

void MyEventAction::EndOfEventAction(const G4Event* evt)
{
    const MyEventInfo& info = EventInfo();
    G4AnalysisManager* man = G4AnalysisManager::Instance();

    G4int eventID = evt->GetEventID();
    G4ThreeVector src(0., 0., 0.);
    if(evt->GetPrimaryVertex()) src = evt->GetPrimaryVertex()->GetPosition();

    // ntuple 1: LXeEvents, one row per event (all events, with or without an LXe deposit)
    man->FillNtupleIColumn(1, 0, eventID);
    man->FillNtupleDColumn(1, 1, src.x()/mm);
    man->FillNtupleDColumn(1, 2, src.y()/mm);
    man->FillNtupleDColumn(1, 3, src.z()/mm);
    man->FillNtupleDColumn(1, 4, info.dirX);
    man->FillNtupleDColumn(1, 5, info.dirY);
    man->FillNtupleDColumn(1, 6, info.dirZ);
    man->FillNtupleDColumn(1, 7, info.E0);
    man->FillNtupleDColumn(1, 8, info.weight);
    man->FillNtupleIColumn(1, 9, info.nGamma);
    man->FillNtupleDColumn(1, 10, info.eGammaSum);
    man->FillNtupleDColumn(1, 11, info.eLXe);
    man->AddNtupleRow(1);

    // ntuple 2: LXeGammas, one row per gamma emitted by radioactive decay (ion mode only)
    for(const auto& g : info.gammas)
    {
        man->FillNtupleIColumn(2, 0, eventID);
        man->FillNtupleDColumn(2, 1, g.first);
        man->FillNtupleDColumn(2, 2, g.second);
        man->AddNtupleRow(2);
    }
}
