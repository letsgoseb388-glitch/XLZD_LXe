#ifndef OPTIONS_HH
#define OPTIONS_HH

#include "globals.hh"
#include "G4UImessenger.hh"

class G4UIcmdWithAString;
class G4UIcmdWithAnInteger;
class G4UIcmdWithABool;

// Run-time options added for the decay-based source modes.
// Every option has a default; only the source mode itself changes behaviour relative to the
// original simulation ("lines" is the new default, "mono" reproduces the old primaries exactly).
struct MyRunOptions
{
    G4String mode = "lines";        // mono | lines | ion
    G4String angular = "legacy";    // legacy (isotropic 4pi from skin points) | cosine (inward cosine law)
    G4String isotope = "Bi214";     // built-in line table / ion: Bi214 | Tl208 (ion mode also Pb214, Co60, K40)
    G4String spectrumFile = "";     // lines mode: overrides the built-in table (E_MeV intensity [sampling_weight])
    G4int    ionZ = 0;              // ion mode: explicit Z and A override the isotope name
    G4int    ionA = 0;
    G4bool   correctDepositPosition = false;   // false = legacy pre-step position

    // true if every option that affects primary generation is identical
    G4bool SameSource(const MyRunOptions& o) const
    {
        return mode == o.mode && angular == o.angular && isotope == o.isotope &&
               spectrumFile == o.spectrumFile && ionZ == o.ionZ && ionA == o.ionA;
    }
};

MyRunOptions& RunOptions();

// /generator/{mode,angular,isotope,spectrumFile,ionZ,ionA} and /detector/correctDepositPosition
class MyOptionsMessenger : public G4UImessenger
{
public:
    explicit MyOptionsMessenger(MyRunOptions* options);
    ~MyOptionsMessenger();
    virtual void SetNewValue(G4UIcommand* command, G4String value);

private:
    MyRunOptions* fOptions;
    G4UIcmdWithAString*   fModeCmd;
    G4UIcmdWithAString*   fAngularCmd;
    G4UIcmdWithAString*   fIsotopeCmd;
    G4UIcmdWithAString*   fSpectrumFileCmd;
    G4UIcmdWithAnInteger* fIonZCmd;
    G4UIcmdWithAnInteger* fIonACmd;
    G4UIcmdWithABool*     fDepositPositionCmd;
};

#endif
