#include "construction.hh"

MyDetectorConstruction::MyDetectorConstruction()
: fLXeRadius(650.*mm), fLXeHalfHeight(650.*mm), fSkinThickness(10.*mm), fEnergyCut(0.)
{
    fMessenger = new G4GenericMessenger(this, "/detector/", "Detector Construction");
    fMessenger->DeclarePropertyWithUnit("radius", "mm", fLXeRadius, "LXe cylinder radius");
    fMessenger->DeclarePropertyWithUnit("halfHeight", "mm", fLXeHalfHeight, "LXe cylinder half height");
    fMessenger->DeclarePropertyWithUnit("energyCut", "keV", fEnergyCut, "Minimum energy deposit to record");
}

MyDetectorConstruction::~MyDetectorConstruction()
{}

G4VPhysicalVolume* MyDetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();

    // Materials
    G4Material* lXe = nist->FindOrBuildMaterial("G4_lXe");
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");

    // World
    G4double worldSize = fLXeRadius + fSkinThickness + 100.*mm;
    G4Box* solidWorld = new G4Box("solidWorld", worldSize, worldSize, worldSize + fLXeHalfHeight);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "logicWorld");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "physWorld", 0, false, 0, true);

    // Vacuum skin — slightly larger than LXe volume
    G4Tubs* solidSkin = new G4Tubs("solidSkin", 0., fLXeRadius + fSkinThickness, fLXeHalfHeight + fSkinThickness, 0., 360.*deg);
    G4LogicalVolume* logicSkin = new G4LogicalVolume(solidSkin, vacuum, "logicSkin");
    new G4PVPlacement(0, G4ThreeVector(), logicSkin, "physSkin", logicWorld, false, 0, true);

    // LXe cylinder — sits inside the skin
    G4Tubs* solidLXe = new G4Tubs("solidLXe", 0., fLXeRadius, fLXeHalfHeight, 0., 360.*deg);
    G4LogicalVolume* logicLXe = new G4LogicalVolume(solidLXe, lXe, "logicLXe");
    new G4PVPlacement(0, G4ThreeVector(), logicLXe, "physLXe", logicSkin, false, 0, true);

    fScoringVolume = logicLXe;

    return physWorld;
}
