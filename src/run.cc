#include "run.hh"

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
