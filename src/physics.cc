#include "physics.hh"

MyPhysicsList::MyPhysicsList()
{
    // EM physics for gamma interactions in LXe
    // option4 is best for low energy and precision
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
}

MyPhysicsList::~MyPhysicsList()
{}
