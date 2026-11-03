#ifndef GENERATOR_HH
#define GENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleTable.hh"
#include "G4GenericMessenger.hh"
#include "Randomize.hh"
#include <cmath>
#include <vector>

#include "options.hh"

class MyPrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    MyPrimaryGenerator();
    ~MyPrimaryGenerator();
    virtual void GeneratePrimaries(G4Event*);

private:
    // (Re)builds the line table / ion definition from RunOptions(); called when the /generator/ options change
    void Configure();

    G4ParticleGun* fParticleGun;
    G4double fLXeRadius;
    G4double fLXeHalfHeight;
    G4double fSkinThickness;
    G4GenericMessenger* fMessenger;
    MyOptionsMessenger* fOptionsMessenger;

    MyRunOptions fCfg;               // the options the members below were built from
    G4bool fConfigured;
    G4bool fIonSet;
    G4int fIonZ, fIonA;              // resolved ion (ion mode)
    std::vector<G4double> fLineE;    // line energies (internal units)
    std::vector<G4double> fLineCum;  // cumulative sampling probability
    std::vector<G4double> fLineW;    // importance weight of each line (physical / sampling probability)
};

#endif
