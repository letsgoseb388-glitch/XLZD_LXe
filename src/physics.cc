#include "physics.hh"

MyPhysicsList::MyPhysicsList()
{
    // EM physics for gamma interactions in LXe
    // option4 is best for low energy and precision
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());

    // Radioactive decay of ions (used by /generator/mode ion). G4DecayPhysics does not decay ions.
    // Needs the G4RADIOACTIVEDATA, G4LEVELGAMMADATA and G4ENSDFSTATEDATA datasets at run time.
    RegisterPhysics(new G4RadioactiveDecayPhysics());
}

MyPhysicsList::~MyPhysicsList()
{}
