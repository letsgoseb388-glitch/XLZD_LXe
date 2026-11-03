#include "run.hh"
#include "construction.hh"
#include "options.hh"

#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

MyRunAction::MyRunAction()
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();

    // Create ntuple with all required columns (added new outputs 7/24)
    man->CreateNtuple("LXeDeposits", "LXe Energy Deposits");
man->CreateNtupleDColumn("sx");        // 0
man->CreateNtupleDColumn("sy");        // 1
man->CreateNtupleDColumn("sz");        // 2
man->CreateNtupleDColumn("E0");        // 3
man->CreateNtupleDColumn("x");         // 4
man->CreateNtupleDColumn("y");         // 5
man->CreateNtupleDColumn("z");         // 6
man->CreateNtupleDColumn("ETPC");      // 7
man->CreateNtupleDColumn("px");        // 8
man->CreateNtupleDColumn("py");        // 9
man->CreateNtupleDColumn("pz");        // 10
man->CreateNtupleIColumn("stepNumber");// 11
man->CreateNtupleDColumn("stepLength");// 12
man->CreateNtupleDColumn("globalTime");// 13
man->CreateNtupleSColumn("processName");// 14
man->CreateNtupleIColumn("trackID");   // 15
man->CreateNtupleIColumn("parentID");  // 16
man->CreateNtupleSColumn("particleType");// 17
man->CreateNtupleIColumn("eventID");   // 18  Geant4 event number (new); lets root_to_csv.py key events by ID instead of by source position
man->FinishNtuple(0);

    // Event-level table: one row per simulated event, including events with no LXe deposit (new)
    man->CreateNtuple("LXeEvents", "Primary and event-level information");
    man->CreateNtupleIColumn("eventID");   // 0
    man->CreateNtupleDColumn("sx");        // 1
    man->CreateNtupleDColumn("sy");        // 2
    man->CreateNtupleDColumn("sz");        // 3
    man->CreateNtupleDColumn("dirX");      // 4  primary direction (0,0,0 in ion mode)
    man->CreateNtupleDColumn("dirY");      // 5
    man->CreateNtupleDColumn("dirZ");      // 6
    man->CreateNtupleDColumn("E0");        // 7  primary gamma energy in MeV (0 in ion mode)
    man->CreateNtupleDColumn("weight");    // 8  importance weight (1 unless the spectrum file has a sampling-weight column)
    man->CreateNtupleIColumn("nGamma");    // 9  decay gammas emitted (ion mode)
    man->CreateNtupleDColumn("eGammaSum"); // 10 summed decay-gamma energy in MeV (ion mode)
    man->CreateNtupleDColumn("eLXe");      // 11 total energy deposited in the LXe in MeV (before the energy cut)
    man->FinishNtuple(1);

    // Decay gammas, one row per gamma (ion mode only; source for make_line_table.py)
    man->CreateNtuple("LXeGammas", "Gammas emitted by radioactive decay");
    man->CreateNtupleIColumn("eventID");   // 0
    man->CreateNtupleDColumn("E");         // 1  MeV
    man->CreateNtupleDColumn("time");      // 2  ns
    man->FinishNtuple(2);

    // Run metadata, a single row written at the end of the run
    man->CreateNtuple("LXeRun", "Run configuration");
    man->CreateNtupleIColumn("nEvents");                 // 0
    man->CreateNtupleDColumn("radius_mm");               // 1
    man->CreateNtupleDColumn("halfHeight_mm");           // 2
    man->CreateNtupleDColumn("energyCut_keV");           // 3
    man->CreateNtupleSColumn("mode");                    // 4
    man->CreateNtupleSColumn("angular");                 // 5
    man->CreateNtupleSColumn("isotope");                 // 6
    man->CreateNtupleSColumn("spectrumFile");            // 7
    man->CreateNtupleIColumn("depositPositionCorrected");// 8
    man->FinishNtuple(3);
}

MyRunAction::~MyRunAction()
{}

void MyRunAction::BeginOfRunAction(const G4Run*)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->OpenFile("LXe_output.root");
}

void MyRunAction::EndOfRunAction(const G4Run* run)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();

    // Run metadata row (ntuple 3). Geometry values are the detector's; /generator/radius and
    // /generator/halfHeight must be set to the same numbers (the sweep scripts already do this).
    const MyDetectorConstruction* det = static_cast<const MyDetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    const MyRunOptions& opt = RunOptions();
    man->FillNtupleIColumn(3, 0, run->GetNumberOfEvent());
    man->FillNtupleDColumn(3, 1, det->fLXeRadius/mm);
    man->FillNtupleDColumn(3, 2, det->fLXeHalfHeight/mm);
    man->FillNtupleDColumn(3, 3, det->fEnergyCut/keV);
    man->FillNtupleSColumn(3, 4, opt.mode);
    man->FillNtupleSColumn(3, 5, opt.angular);
    man->FillNtupleSColumn(3, 6, opt.isotope);
    man->FillNtupleSColumn(3, 7, opt.spectrumFile.empty() ? G4String("none") : opt.spectrumFile);
    man->FillNtupleIColumn(3, 8, opt.correctDepositPosition ? 1 : 0);
    man->AddNtupleRow(3);

    man->Write();
    man->CloseFile();
}
