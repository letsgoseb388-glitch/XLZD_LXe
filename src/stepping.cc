#include "stepping.hh"
#include "G4AnalysisManager.hh"

MySteppingAction::MySteppingAction()
{}

MySteppingAction::~MySteppingAction()
{}

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    // Only record energy deposits inside LXe volume
    G4VPhysicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if(!volume) return;
    if(volume->GetLogicalVolume()->GetName() != "logicLXe") return;

    G4double edep = step->GetTotalEnergyDeposit();
    if(edep <= 0.) return;

    G4AnalysisManager* man = G4AnalysisManager::Instance();

    // Source position (primary vertex)
    G4ThreeVector srcPos = step->GetTrack()->GetVertexPosition();
    G4double sx = srcPos.x()/mm;
    G4double sy = srcPos.y()/mm;
    G4double sz = srcPos.z()/mm;

    // Source energy
    G4double E0 = step->GetTrack()->GetVertexKineticEnergy()/MeV;

    // Deposit position
    G4ThreeVector depPos = step->GetPreStepPoint()->GetPosition();
    G4double x = depPos.x()/mm;
    G4double y = depPos.y()/mm;
    G4double z = depPos.z()/mm;

    // Deposit energy
    G4double ETPC = edep/MeV;

    // Fill ntuple
    man->FillNtupleDColumn(0, 0, sx);
    man->FillNtupleDColumn(0, 1, sy);
    man->FillNtupleDColumn(0, 2, sz);
    man->FillNtupleDColumn(0, 3, E0);
    man->FillNtupleDColumn(0, 4, x);
    man->FillNtupleDColumn(0, 5, y);
    man->FillNtupleDColumn(0, 6, z);
    man->FillNtupleDColumn(0, 7, ETPC);
    man->AddNtupleRow(0);
}
