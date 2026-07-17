#include "run.hh"

MyRunAction::MyRunAction()
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();

    // Create ntuple with all required columns
    man->CreateNtuple("LXeDeposits", "LXe Energy Deposits");
    man->CreateNtupleDColumn("sx");    // source x (mm)
    man->CreateNtupleDColumn("sy");    // source y (mm)
    man->CreateNtupleDColumn("sz");    // source z (mm)
    man->CreateNtupleDColumn("E0");    // source energy (MeV)
    man->CreateNtupleDColumn("x");     // deposit x (mm)
    man->CreateNtupleDColumn("y");     // deposit y (mm)
    man->CreateNtupleDColumn("z");     // deposit z (mm)
    man->CreateNtupleDColumn("ETPC");  // deposit energy (MeV)
    man->FinishNtuple(0);
}

MyRunAction::~MyRunAction()
{}

void MyRunAction::BeginOfRunAction(const G4Run*)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->OpenFile("LXe_output.root");
}

void MyRunAction::EndOfRunAction(const G4Run*)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->Write();
    man->CloseFile();
}
