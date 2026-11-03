#include "stacking.hh"
#include "eventinfo.hh"

#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4Gamma.hh"
#include "G4Ions.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <vector>

MyStackingAction::MyStackingAction()
{}

MyStackingAction::~MyStackingAction()
{}

G4ClassificationOfNewTrack MyStackingAction::ClassifyNewTrack(const G4Track* track)
{
    // Primaries and everything not created by radioactive decay are left alone
    if(track->GetParentID() == 0) return fUrgent;
    const G4VProcess* creator = track->GetCreatorProcess();
    if(!creator) return fUrgent;

    // Diagnostic: print each creator process once (shows the exact process name used by this Geant4 version;
    // in ion mode "RadioactiveDecay" must appear, otherwise this action does nothing)
    static thread_local std::vector<const G4VProcess*> seen;
    if(std::find(seen.begin(), seen.end(), creator) == seen.end())
    {
        seen.push_back(creator);
        G4cout << "MyStackingAction: creator process '" << creator->GetProcessName()
               << "' (first seen for " << track->GetParticleDefinition()->GetParticleName() << ")" << G4endl;
    }

    if(creator->GetProcessName() != "RadioactiveDecay") return fUrgent;

    const G4ParticleDefinition* def = track->GetParticleDefinition();

    // Gammas (including X-rays) emitted by the decay: tally and keep
    if(def == G4Gamma::Gamma())
    {
        MyEventInfo& info = EventInfo();
        const G4double e = track->GetKineticEnergy()/MeV;
        info.nGamma++;
        info.eGammaSum += e;
        info.gammas.emplace_back(e, track->GetGlobalTime()/ns);
        return fUrgent;
    }

    // Daughter nuclei: keep excited states (they de-excite by gamma emission), drop ground states
    // (e.g. Po-214, which would alpha-decay, and the long-lived Pb-210 further down the chain)
    if(def->GetParticleType() == "nucleus")
    {
        const G4Ions* ion = dynamic_cast<const G4Ions*>(def);
        return (ion && ion->GetExcitationEnergy() > 1.*keV) ? fUrgent : fKill;
    }

    // Betas, anti-neutrinos, alphas, conversion electrons: not part of the external gamma source
    return fKill;
}
