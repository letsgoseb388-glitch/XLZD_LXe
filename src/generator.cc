#include "generator.hh"
#include "eventinfo.hh"

#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4Gamma.hh"
#include "G4IonTable.hh"
#include "G4ThreeVector.hh"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
    // One gamma line: energy (MeV), physical intensity (any normalisation) and relative sampling weight.
    struct Line { G4double E; G4double intensity; G4double sampling; };

    void Fatal(const char* where, const std::string& message)
    {
        G4Exception(where, "XLZD001", FatalException, message.c_str());
    }

    // PROVISIONAL built-in tables: major gamma lines, intensity in % per decay, from ENSDF values as
    // remembered when this was written (not generated from Geant4's own decay data).
    // Regenerate with:  /generator/mode ion  ->  make_line_table.py  ->  /generator/spectrumFile
    std::vector<Line> BuiltInLines(const G4String& isotope)
    {
        std::vector<Line> table;
        if(isotope == "Bi214")
        {
            static const G4double d[][2] = {
                {0.60931, 45.49}, {0.66545, 1.53},  {0.76836, 4.89},  {0.80617, 1.26},
                {0.93406, 3.09},  {1.12029, 14.91}, {1.15519, 1.63},  {1.23811, 5.83},
                {1.28096, 1.43},  {1.37767, 3.97},  {1.38531, 0.79},  {1.40150, 1.33},
                {1.40798, 2.39},  {1.50923, 2.13},  {1.66128, 1.05},  {1.72960, 2.84},
                {1.76449, 15.28}, {1.84742, 2.03},  {2.11855, 1.16},  {2.20421, 4.91},
                {2.44786, 1.55} };
            for(const auto& r : d) table.push_back({r[0], r[1], r[1]});
        }
        else if(isotope == "Tl208")
        {
            static const G4double d[][2] = {
                {0.25261, 0.69},  {0.27737, 6.6},   {0.51077, 22.6},  {0.58319, 85.0},
                {0.76313, 1.79},  {0.86056, 12.5},  {2.61451, 99.75} };
            for(const auto& r : d) table.push_back({r[0], r[1], r[1]});
        }
        else
        {
            Fatal("MyPrimaryGenerator::BuiltInLines",
                  "No built-in line table for isotope '" + isotope + "' (use Bi214 or Tl208, or /generator/spectrumFile)");
        }
        return table;
    }

    // Text file: E_MeV  intensity  [sampling_weight]   ('#' starts a comment; the sampling weight defaults to the intensity)
    std::vector<Line> ReadLines(const G4String& fileName)
    {
        std::ifstream in(fileName.c_str());
        if(!in) Fatal("MyPrimaryGenerator::ReadLines", "Cannot open spectrum file '" + fileName + "'");
        std::vector<Line> table;
        std::string text;
        while(std::getline(in, text))
        {
            const std::size_t hash = text.find('#');
            if(hash != std::string::npos) text.erase(hash);
            std::istringstream row(text);
            G4double e = 0., intensity = 0., sampling = 0.;
            if(!(row >> e >> intensity)) continue;
            if(!(row >> sampling)) sampling = intensity;
            table.push_back({e, intensity, sampling});
        }
        return table;
    }

    G4bool IonFor(const G4String& isotope, G4int& Z, G4int& A)
    {
        if(isotope == "Bi214") { Z = 83; A = 214; return true; }
        if(isotope == "Tl208") { Z = 81; A = 208; return true; }
        if(isotope == "Pb214") { Z = 82; A = 214; return true; }
        if(isotope == "Co60")  { Z = 27; A = 60;  return true; }
        if(isotope == "K40")   { Z = 19; A = 40;  return true; }
        return false;
    }
}

MyPrimaryGenerator::MyPrimaryGenerator()
: fLXeRadius(650.*mm), fLXeHalfHeight(650.*mm), fSkinThickness(10.*mm),
  fConfigured(false), fIonSet(false), fIonZ(0), fIonA(0)
{
    fParticleGun = new G4ParticleGun(1);

    // particle type set to gamma
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleEnergy(2447.*keV); // changed from meV*

    // geometry is parameterized
    fMessenger = new G4GenericMessenger(this, "/generator/", "Generator settings");
    fMessenger->DeclarePropertyWithUnit("radius", "mm", fLXeRadius, "LXe radius");
    fMessenger->DeclarePropertyWithUnit("halfHeight", "mm", fLXeHalfHeight, "LXe half height");

    // source mode, angular law, isotope, ... (/generator/mode etc. and /detector/correctDepositPosition)
    fOptionsMessenger = new MyOptionsMessenger(&RunOptions());
}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fParticleGun;
    delete fOptionsMessenger;
}

void MyPrimaryGenerator::Configure()
{
    fCfg = RunOptions();
    fConfigured = true;
    fIonSet = false;
    fLineE.clear();
    fLineCum.clear();
    fLineW.clear();

    if(fCfg.mode != "mono" && fCfg.mode != "lines" && fCfg.mode != "ion")
        Fatal("MyPrimaryGenerator::Configure", "Unknown /generator/mode '" + fCfg.mode + "' (use mono, lines or ion)");
    if(fCfg.angular != "legacy" && fCfg.angular != "cosine")
        Fatal("MyPrimaryGenerator::Configure", "Unknown /generator/angular '" + fCfg.angular + "' (use legacy or cosine)");

    // plain gamma gun again (an earlier ion-mode run may have replaced the definition)
    fParticleGun->SetParticleDefinition(G4Gamma::Gamma());

    if(fCfg.mode == "lines")
    {
        std::vector<Line> table = fCfg.spectrumFile.empty() ? BuiltInLines(fCfg.isotope) : ReadLines(fCfg.spectrumFile);

        // lines with a zero sampling weight (or no intensity) are dropped from the physical normalisation as well
        G4double sumIntensity = 0., sumSampling = 0.;
        for(const auto& l : table)
            if(l.E > 0. && l.intensity > 0. && l.sampling > 0.) { sumIntensity += l.intensity; sumSampling += l.sampling; }
        if(sumSampling <= 0.) Fatal("MyPrimaryGenerator::Configure", "The line table has no usable line");

        G4double cumulative = 0., wMax = 0.;
        for(const auto& l : table)
        {
            if(!(l.E > 0. && l.intensity > 0. && l.sampling > 0.)) continue;
            const G4double pSample = l.sampling/sumSampling;
            const G4double pPhys   = l.intensity/sumIntensity;
            cumulative += pSample;
            fLineE.push_back(l.E*MeV);
            fLineCum.push_back(cumulative);
            fLineW.push_back(pPhys/pSample);
            wMax = std::max(wMax, pPhys/pSample);
        }
        fLineCum.back() = 1.;   // guard against rounding
        std::string origin = "built-in table " + std::string(fCfg.isotope);
        if(!fCfg.spectrumFile.empty()) origin = std::string(fCfg.spectrumFile);
        G4cout << "MyPrimaryGenerator: lines mode, " << fLineE.size() << " lines from "
               << origin << ", max event weight " << wMax << G4endl;
    }
    else if(fCfg.mode == "ion")
    {
        if(fCfg.ionZ > 0 && fCfg.ionA > 0) { fIonZ = fCfg.ionZ; fIonA = fCfg.ionA; }
        else if(!IonFor(fCfg.isotope, fIonZ, fIonA))
            Fatal("MyPrimaryGenerator::Configure", "Unknown isotope '" + fCfg.isotope + "' for ion mode (set /generator/ionZ and /generator/ionA)");
        G4cout << "MyPrimaryGenerator: ion mode, Z=" << fIonZ << " A=" << fIonA << " at rest" << G4endl;
    }
    else
    {
        G4cout << "MyPrimaryGenerator: mono mode, 2.447 MeV gamma" << G4endl;
    }
    G4cout << "MyPrimaryGenerator: angular law " << fCfg.angular << G4endl;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
    MyEventInfo& info = EventInfo();
    info.Reset();

    if(!fConfigured || !fCfg.SameSource(RunOptions())) Configure();

    // Sample random position on the vacuum skin surface & randomly choose between cylindrical side, top cap, or bottom cap
    G4double skinRadius = fLXeRadius + fSkinThickness;
    G4double skinHalfHeight = fLXeHalfHeight + fSkinThickness;

    // Total surface areas for weighted random selection
    G4double sideArea = 2. * M_PI * skinRadius * 2. * skinHalfHeight;
    G4double capArea = M_PI * skinRadius * skinRadius;
    G4double totalArea = sideArea + 2. * capArea;

    G4double rand = G4UniformRand() * totalArea;

    G4double x, y, z;
    G4ThreeVector inward;   // inward surface normal at the sampled point (used by the cosine-law option)

    if(rand < sideArea) {
        // Cylindrical side
        G4double phi = G4UniformRand() * 2. * M_PI;
        z = (2. * G4UniformRand() - 1.) * skinHalfHeight;
        x = skinRadius * std::cos(phi);
        y = skinRadius * std::sin(phi);
        inward = G4ThreeVector(-std::cos(phi), -std::sin(phi), 0.);
    } else if(rand < sideArea + capArea) {
        // top cap
        G4double phi = G4UniformRand() * 2. * M_PI;
        G4double r = skinRadius * std::sqrt(G4UniformRand());
        x = r * std::cos(phi);
        y = r * std::sin(phi);
        z = skinHalfHeight;
        inward = G4ThreeVector(0., 0., -1.);
    } else {
        // bottom cap
        G4double phi = G4UniformRand() * 2. * M_PI;
        G4double r = skinRadius * std::sqrt(G4UniformRand());
        x = r * std::cos(phi);
        y = r * std::sin(phi);
        z = -skinHalfHeight;
        inward = G4ThreeVector(0., 0., 1.);
    }

    fParticleGun->SetParticlePosition(G4ThreeVector(x, y, z));

    if(fCfg.mode == "ion")
    {
        // Radioactive ion at rest on the skin; Geant4's RadioactiveDecay process decays it.
        if(!fIonSet)
        {
            // the ion table is only usable once the run has been initialised, so define the ion lazily
            G4ParticleDefinition* ion = G4IonTable::GetIonTable()->GetIon(fIonZ, fIonA, 0.);
            if(!ion) Fatal("MyPrimaryGenerator::GeneratePrimaries", "Could not create the requested ion");
            fParticleGun->SetParticleDefinition(ion);
            fParticleGun->SetParticleCharge(0.);
            fIonSet = true;
        }
        fParticleGun->SetParticleEnergy(0.);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
        fParticleGun->GeneratePrimaryVertex(anEvent);
        return;   // E0 = 0, direction = (0,0,0), weight = 1 in the event record; the decay gammas are tallied by the stacking action
    }

    G4ThreeVector direction;
    if(fCfg.angular == "cosine")
    {
        // Cosine law about the inward normal: what an isotropic external gamma flux produces on a surface
        G4double cosT = std::sqrt(G4UniformRand());
        G4double sinT = std::sqrt(1. - cosT * cosT);
        G4double phiD = 2. * M_PI * G4UniformRand();
        G4ThreeVector e1 = inward.orthogonal().unit();
        G4ThreeVector e2 = inward.cross(e1);
        direction = cosT * inward + sinT * std::cos(phiD) * e1 + sinT * std::sin(phiD) * e2;
    }
    else
    {
        // Isotropic random direction (original behaviour)
        G4double cosTheta = 2. * G4UniformRand() - 1.;
        G4double sinTheta = std::sqrt(1. - cosTheta * cosTheta);
        G4double phi2 = 2. * M_PI * G4UniformRand();
        G4double dx = sinTheta * std::cos(phi2);
        G4double dy = sinTheta * std::sin(phi2);
        G4double dz = cosTheta;
        direction = G4ThreeVector(dx, dy, dz);
    }
    fParticleGun->SetParticleMomentumDirection(direction);

    // Energy: fixed 2.447 MeV (mono) or one line drawn from the table (lines). Drawn after position
    // and direction so that mono mode consumes the random numbers exactly as the original generator did.
    G4double energy = 2447.*keV;
    G4double weight = 1.;
    if(fCfg.mode == "lines")
    {
        const G4double u = G4UniformRand();
        std::size_t k = static_cast<std::size_t>(std::lower_bound(fLineCum.begin(), fLineCum.end(), u) - fLineCum.begin());
        if(k >= fLineE.size()) k = fLineE.size() - 1;
        energy = fLineE[k];
        weight = fLineW[k];
    }
    fParticleGun->SetParticleEnergy(energy);

    info.E0 = energy/MeV;
    info.dirX = direction.x();
    info.dirY = direction.y();
    info.dirZ = direction.z();
    info.weight = weight;

    fParticleGun->GeneratePrimaryVertex(anEvent);
}
