#include "stepping.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"



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

    // Energy cut, skip deposits below the threshold set by /detector/energyCut

    const MyDetectorConstruction* det = static_cast<const MyDetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    if(edep < det->fEnergyCut) return;

    G4AnalysisManager* man = G4AnalysisManager::Instance();

        // Source position (true primary vertex, not the current tracks vertex)
        const G4Event* evt = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4ThreeVector srcPos = evt->GetPrimaryVertex()->GetPosition();
    G4double sx = srcPos.x()/mm;
    G4double sy = srcPos.y()/mm;
    G4double sz = srcPos.z()/mm;

    
    // Primary source energy, always 2.447 MeV for this simulation
    G4double E0 = 2.447;

    // Deposit position
    G4ThreeVector depPos = step->GetPreStepPoint()->GetPosition();
    G4double x = depPos.x()/mm;
    G4double y = depPos.y()/mm;
    G4double z = depPos.z()/mm;

    // Deposit energy
    G4double ETPC = edep/MeV;

    // Fill ntuple (new outputs added 7/24)
// Momentum direction
G4ThreeVector momDir = step->GetPreStepPoint()->GetMomentumDirection();
G4double px = momDir.x();
G4double py = momDir.y();
G4double pz = momDir.z();

// step information
G4int stepNum = step->GetTrack()->GetCurrentStepNumber();
G4double stepLength = step->GetStepLength()/mm;
G4double globalTime = step->GetPreStepPoint()->GetGlobalTime()/ns;

// process name
G4String processName = "none";
if(step->GetPostStepPoint()->GetProcessDefinedStep())
    processName = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();

// ]track info
G4int trackID = step->GetTrack()->GetTrackID();
G4int parentID = step->GetTrack()->GetParentID();
G4String particleType = step->GetTrack()->GetParticleDefinition()->GetParticleName();

man->FillNtupleDColumn(0, 0, sx);
man->FillNtupleDColumn(0, 1, sy);
man->FillNtupleDColumn(0, 2, sz);
man->FillNtupleDColumn(0, 3, E0);
man->FillNtupleDColumn(0, 4, x);
man->FillNtupleDColumn(0, 5, y);
man->FillNtupleDColumn(0, 6, z);
man->FillNtupleDColumn(0, 7, ETPC);
man->FillNtupleDColumn(0, 8, px);
man->FillNtupleDColumn(0, 9, py);
man->FillNtupleDColumn(0, 10, pz);
man->FillNtupleIColumn(0, 11, stepNum);
man->FillNtupleDColumn(0, 12, stepLength);
man->FillNtupleDColumn(0, 13, globalTime);
man->FillNtupleSColumn(0, 14, processName);
man->FillNtupleIColumn(0, 15, trackID);
man->FillNtupleIColumn(0, 16, parentID);
man->FillNtupleSColumn(0, 17, particleType);
man->AddNtupleRow(0);
}
