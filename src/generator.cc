#include "generator.hh"

MyPrimaryGenerator::MyPrimaryGenerator()
: fLXeRadius(650.*mm), fLXeHalfHeight(650.*mm), fSkinThickness(10.*mm)
{
    fParticleGun = new G4ParticleGun(1);

    // Set particle type to gamma
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleEnergy(2447.*MeV);

    // Messenger to change geometry parameters
    fMessenger = new G4GenericMessenger(this, "/generator/", "Generator settings");
    fMessenger->DeclarePropertyWithUnit("radius", "mm", fLXeRadius, "LXe radius");
    fMessenger->DeclarePropertyWithUnit("halfHeight", "mm", fLXeHalfHeight, "LXe half height");
}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fParticleGun;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
    // Sample random position on the vacuum skin surface
    // Randomly choose between cylindrical side, top cap, or bottom cap
    G4double skinRadius = fLXeRadius + fSkinThickness;
    G4double skinHalfHeight = fLXeHalfHeight + fSkinThickness;

    // Total surface areas for weighted random selection
    G4double sideArea = 2. * M_PI * skinRadius * 2. * skinHalfHeight;
    G4double capArea = M_PI * skinRadius * skinRadius;
    G4double totalArea = sideArea + 2. * capArea;

    G4double rand = G4UniformRand() * totalArea;

    G4double x, y, z;

    if(rand < sideArea) {
        // Cylindrical side
        G4double phi = G4UniformRand() * 2. * M_PI;
        z = (2. * G4UniformRand() - 1.) * skinHalfHeight;
        x = skinRadius * std::cos(phi);
        y = skinRadius * std::sin(phi);
    } else if(rand < sideArea + capArea) {
        // Top cap
        G4double phi = G4UniformRand() * 2. * M_PI;
        G4double r = skinRadius * std::sqrt(G4UniformRand());
        x = r * std::cos(phi);
        y = r * std::sin(phi);
        z = skinHalfHeight;
    } else {
        // Bottom cap
        G4double phi = G4UniformRand() * 2. * M_PI;
        G4double r = skinRadius * std::sqrt(G4UniformRand());
        x = r * std::cos(phi);
        y = r * std::sin(phi);
        z = -skinHalfHeight;
    }

    fParticleGun->SetParticlePosition(G4ThreeVector(x, y, z));

    // Isotropic random direction
    G4double cosTheta = 2. * G4UniformRand() - 1.;
    G4double sinTheta = std::sqrt(1. - cosTheta * cosTheta);
    G4double phi2 = 2. * M_PI * G4UniformRand();
    G4double dx = sinTheta * std::cos(phi2);
    G4double dy = sinTheta * std::sin(phi2);
    G4double dz = cosTheta;

    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(dx, dy, dz));
    fParticleGun->GeneratePrimaryVertex(anEvent);
}
