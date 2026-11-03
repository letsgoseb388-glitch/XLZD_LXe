#include "options.hh"

#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithABool.hh"
#include "G4ApplicationState.hh"

MyRunOptions& RunOptions()
{
    static MyRunOptions options;
    return options;
}

MyOptionsMessenger::MyOptionsMessenger(MyRunOptions* options)
: fOptions(options)
{
    // The /generator/ and /detector/ directories already exist (created by the G4GenericMessengers
    // in MyPrimaryGenerator and MyDetectorConstruction); these commands are added to them.
    fModeCmd = new G4UIcmdWithAString("/generator/mode", this);
    fModeCmd->SetGuidance("Primary source: mono = one fixed 2.447 MeV gamma per event (original behaviour);");
    fModeCmd->SetGuidance("lines = one gamma per event with the energy drawn from a line table (default);");
    fModeCmd->SetGuidance("ion = a radioactive ion at rest, decayed by Geant4 (cascade gammas, validation mode).");
    fModeCmd->SetParameterName("mode", false);
    fModeCmd->SetCandidates("mono lines ion");
    fModeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fAngularCmd = new G4UIcmdWithAString("/generator/angular", this);
    fAngularCmd->SetGuidance("Angular law of gammas leaving the skin (mono and lines modes):");
    fAngularCmd->SetGuidance("legacy = isotropic over 4pi from every skin point (original behaviour);");
    fAngularCmd->SetGuidance("cosine = cosine law about the inward surface normal (isotropic external flux).");
    fAngularCmd->SetParameterName("angular", false);
    fAngularCmd->SetCandidates("legacy cosine");
    fAngularCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fIsotopeCmd = new G4UIcmdWithAString("/generator/isotope", this);
    fIsotopeCmd->SetGuidance("Built-in line table (lines mode) or ion (ion mode): Bi214 or Tl208 (ion mode also Pb214, Co60, K40).");
    fIsotopeCmd->SetParameterName("isotope", false);
    fIsotopeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fSpectrumFileCmd = new G4UIcmdWithAString("/generator/spectrumFile", this);
    fSpectrumFileCmd->SetGuidance("Lines mode: read the line table from this file instead of the built-in table.");
    fSpectrumFileCmd->SetGuidance("Text file, one line per row: E_MeV  intensity  [sampling_weight]; '#' starts a comment.");
    fSpectrumFileCmd->SetGuidance("Give 'none' to go back to the built-in table.");
    fSpectrumFileCmd->SetParameterName("file", false);
    fSpectrumFileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fIonZCmd = new G4UIcmdWithAnInteger("/generator/ionZ", this);
    fIonZCmd->SetGuidance("Ion mode: atomic number of the ion (with /generator/ionA; 0 = use /generator/isotope).");
    fIonZCmd->SetParameterName("Z", false);
    fIonZCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fIonACmd = new G4UIcmdWithAnInteger("/generator/ionA", this);
    fIonACmd->SetGuidance("Ion mode: mass number of the ion (with /generator/ionZ; 0 = use /generator/isotope).");
    fIonACmd->SetParameterName("A", false);
    fIonACmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fDepositPositionCmd = new G4UIcmdWithABool("/detector/correctDepositPosition", this);
    fDepositPositionCmd->SetGuidance("false (default): record the pre-step position of every deposit (original behaviour).");
    fDepositPositionCmd->SetGuidance("true: record the post-step position for neutral tracks (where the gamma interacts) and the step midpoint for charged tracks.");
    fDepositPositionCmd->SetParameterName("flag", false);
    fDepositPositionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

MyOptionsMessenger::~MyOptionsMessenger()
{
    delete fModeCmd;
    delete fAngularCmd;
    delete fIsotopeCmd;
    delete fSpectrumFileCmd;
    delete fIonZCmd;
    delete fIonACmd;
    delete fDepositPositionCmd;
}

void MyOptionsMessenger::SetNewValue(G4UIcommand* command, G4String value)
{
    if(command == fModeCmd)                  fOptions->mode = value;
    else if(command == fAngularCmd)          fOptions->angular = value;
    else if(command == fIsotopeCmd)          fOptions->isotope = value;
    else if(command == fSpectrumFileCmd)     fOptions->spectrumFile = (value == "none") ? G4String("") : value;
    else if(command == fIonZCmd)             fOptions->ionZ = fIonZCmd->GetNewIntValue(value.c_str());
    else if(command == fIonACmd)             fOptions->ionA = fIonACmd->GetNewIntValue(value.c_str());
    else if(command == fDepositPositionCmd)  fOptions->correctDepositPosition = fDepositPositionCmd->GetNewBoolValue(value.c_str());
}
