#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/HepMCCandidate/interface/GenParticleFwd.h"

#include "DataFormats/Math/interface/deltaR.h"

#include <vector>
#include <iostream>
#include <cmath>

class NewGenMatcher : public edm::stream::EDProducer<> 
{
public:
    explicit NewGenMatcher(const edm::ParameterSet& cfg);
    ~NewGenMatcher() override {}

    void produce(edm::Event&, const edm::EventSetup&) override;
private:
    enum class DecayKind{
        EtaMuMu, 
        EtaMuMuGamma, 
        OmegaMuMu, 
        OmegaMuMuPi0, 
        PhiMuMu, 
        EtaPrimeMuMuGamma, 
        RhoMuMu, 
    };
    struct DimuDecay{
    const reco::Candidate* mother;
    const reco::Candidate* mu1;
    const reco::Candidate* mu2;
    const reco::Candidate* extra; // Stores the extra particle in case of a 3 body decay
    DecayKind kind; // Which decay is it?
    bool hasExtra() const { return extra != nullptr; } // Mainly debugging purposes
    };

    bool matchMuon(const pat::Muon& reco,const reco::Candidate& gen,bool debug,const std::string& tag) const;
    bool findMatchMuon(const pat::Muon& reco,const reco::GenParticleCollection& gens) const; // Function to find a gen match with reco muon for combinatorial background studies
    const reco::Candidate* getAncestor(const reco::Candidate* gen) const; // Get the first non-muon ancestor of a given gen. muon
    void printAncestorChain(const reco::Candidate* gen) const;
    const reco::Candidate* findBestGenMuonMatch(const pat::Muon& reco,const reco::GenParticleCollection& gens) const;
    const reco::Candidate* findStatus1MuonDescendant(const reco::Candidate* c) const;

    //Token:
    edm::EDGetTokenT<std::vector<pat::Muon>> muonToken_;
    edm::EDGetTokenT<std::vector<pat::CompositeCandidate>> mumuToken_;
    edm::EDGetTokenT<reco::GenParticleCollection> genToken_;
    // Matching parameters
    double maxDeltaR_;
    double maxRelPt_;
    bool debug_;
};
NewGenMatcher::NewGenMatcher(const edm::ParameterSet& cfg)
{
    muonToken_ = consumes<std::vector<pat::Muon>>(cfg.getParameter<edm::InputTag>("src_muon") );
    mumuToken_ = consumes<std::vector<pat::CompositeCandidate>>(cfg.getParameter<edm::InputTag>("src_MuMu"));
    genToken_ = consumes<reco::GenParticleCollection>(cfg.getParameter<edm::InputTag>("src_gen"));

    maxDeltaR_ = cfg.getParameter<double>("maxDeltaR");  // e.g. 0.05
    maxRelPt_  = cfg.getParameter<double>("maxRelPt");   // e.g. 0.5
    debug_     = cfg.getParameter<bool>("debug");

    produces<std::vector<pat::CompositeCandidate>>("SelectedMuMuExtended");
};
bool NewGenMatcher::matchMuon(const pat::Muon& reco,
                                            const reco::Candidate& gen,
                                            bool debug,
                                            const std::string& tag) const
{
    double dR = reco::deltaR(reco.eta(), reco.phi(), gen.eta(), gen.phi());
    double relPt = std::abs(reco.pt() - gen.pt()) / gen.pt();
    if (debug) {
        std::cout << "    [" << tag << "] reco(pt=" << reco.pt() << ",eta=" << reco.eta()
                  << ") vs gen(pt=" << gen.pt() << ",eta=" << gen.eta()
                  << ") dR=" << dR << " relPt=" << relPt << std::endl;
    }
    if (dR > maxDeltaR_) {return false;}
    if (relPt > maxRelPt_) {return false;}
    if (reco.charge() != gen.charge()) {return false;}

    return true;
}
bool NewGenMatcher::findMatchMuon(const pat::Muon& reco,const reco::GenParticleCollection& gens) const
{
    for (const auto& gp: gens){
        if (std::abs(gp.pdgId()) != 13 ) continue;
        if (gp.status() != 1) continue;
        if (matchMuon(reco, gp, false, "findMatchMuon")) {
            return true;
        }
    }
    return false;
}
const reco::Candidate* NewGenMatcher::getAncestor(const reco::Candidate* gen) const
{
    if (!gen || gen->numberOfMothers() == 0)return nullptr;

    const reco::Candidate* mother = gen->mother(0);
    if (!mother) return nullptr;
    int motherId = std::abs(mother->pdgId());
    double motherpt = mother->pt();
    if (motherId == 13 || motherpt < 1e-10){
        return getAncestor(mother);
    }
    return mother;
}

void NewGenMatcher::printAncestorChain(const reco::Candidate* gen) const
{
    if (!gen) return;

    std::cout << "    pdgId=" << gen->pdgId()
              << " status=" << gen->status()
              << " pt=" << gen->pt()
              << " eta=" << gen->eta()
              << " phi=" << gen->phi()
              << " nMothers=" << gen->numberOfMothers()
              << "\n";

    if (gen->numberOfMothers() == 0) return;

    const reco::Candidate* mother = gen->mother(0);

    printAncestorChain(mother);
}
const reco::Candidate* NewGenMatcher::findBestGenMuonMatch(
    const pat::Muon& reco,
    const reco::GenParticleCollection& gens) const
{
    const reco::Candidate* best = nullptr;
    double bestDR = 1e9;

    for (const auto& gp : gens) {
        if (gp.status() != 1) continue;   
        if (std::abs(gp.pdgId()) != 13) continue;

        if (!matchMuon(reco, gp, false, "findBestGenMuonMatch")) continue;

        double dR = reco::deltaR(reco.eta(), reco.phi(), gp.eta(), gp.phi());
        if (dR < bestDR) {
            bestDR = dR;
            best = &gp;
        }
    }
    return best;
}
const reco::Candidate* NewGenMatcher::findStatus1MuonDescendant(const reco::Candidate* c) const
{
    if (!c) return nullptr;
    if (debug_) { std::cout
                  << "[recurse] pdgId=" << c->pdgId()
                  << " status=" << c->status()
                  << " pt=" << c->pt()
                  << " nDau=" << c->numberOfDaughters()
                  << "\n";
    }
    // Found final muon
    if (std::abs(c->pdgId()) == 13 && c->status() == 1) {
        if (debug_) { std::cout << "  --> FOUND status-1 muon\n";}
        return c;
    }
    // Stop when no daughters, i.e. stable:
    if (c->numberOfDaughters() == 0)
        return nullptr;
    // Loop over daughters
    for (size_t i = 0; i < c->numberOfDaughters(); ++i){
        const reco::Candidate* dau = c -> daughter(i);
        if (!dau) continue;
        const reco::Candidate* found = findStatus1MuonDescendant(dau); 
        if (found) return found;
    }
    return nullptr;

}

void NewGenMatcher::produce(edm::Event& evt, const edm::EventSetup&)
{
    // Get inputs
    edm::Handle<std::vector<pat::Muon>> muons;
    edm::Handle<std::vector<pat::CompositeCandidate>> dimuons;
    edm::Handle<reco::GenParticleCollection> genParticles;

    evt.getByToken(muonToken_,  muons);
    evt.getByToken(mumuToken_,  dimuons);
    evt.getByToken(genToken_,   genParticles);

    auto out = std::make_unique<std::vector<pat::CompositeCandidate>>();
    out->reserve(dimuons->size());
    // ==========================================================
    // Phase 1: find all mu mu decays at GEN level
    // ==========================================================
    std::vector<DimuDecay> decays;

    for (const auto& p : *genParticles)
    {
        int pid = std::abs(p.pdgId());
        bool isEta      = (pid == 221);
        bool isOmega    = (pid == 223);
        bool isPhi      = (pid == 333);
        bool isEtaPrime = (pid == 331);
        bool isRho      = (pid == 113);
        if (!isEta && !isOmega && !isPhi && !isEtaPrime && !isRho ) continue;
        if (debug_) { std::cout << " Found mother (pdgId=" << p.pdgId() << ") pt=" << p.pt() << " eta=" << p.eta() << " phi=" << p.phi() << " with " << p.numberOfDaughters() << " daughters\n"; }
        std::vector<const reco::Candidate*> muDaughters;
        const reco::Candidate* photon = nullptr; // for eta(') to mu mu gamma
        const reco::Candidate* pi0    = nullptr; // for omega to mu mu pi0. Note that the pion stored will be status =2 so that is why we use mergedGenparticles
                                                 // instead of only relying on packed. In a lot of cases this pi0 wont be stored and directly decay into two status =1 photons
                                                 // but these photons can convert to e+e- pairs (rarely but happens) so the idea is to classify any decay that has more than two status = 1 particles  
                                                 // or a pion as omegaMuMuPi0. If only two stable particles are found then we classify it as OmegaToMuMu
        int nExtraStable = 0; // Count the number of stable particles explicitly that are not muons

        for (size_t d = 0; d < p.numberOfDaughters(); ++d)
        {
            const reco::Candidate* dau = p.daughter(d);

            if (!dau) continue;
            const int id = std::abs(dau->pdgId());
            if (debug_) { std::cout << " daughter " << d << " pdgId=" << id << " status=" << dau->status() << " pt=" << dau->pt() << " eta=" << dau->eta() << " phi=" << dau->phi() << "\n"; }

            if (id == 111 && !pi0) pi0 = dau; // Leave open the possibility for status = 2 !
            // --- handle muons that are not status 1 but *have* status-1 muon daughters ---
            if (id == 13)
            {
            const reco::Candidate* finalMu = (dau->status() == 1) ? dau : findStatus1MuonDescendant(dau);
            if (finalMu) muDaughters.push_back(finalMu);
                continue;
            }
            if (dau->status() != 1) continue;
            nExtraStable++;
            if ( (id == 22) && (isEta || isEtaPrime) && !photon ) //Store the photon as extra particle for eta or eta prime
                photon = dau;
        }

        if (muDaughters.size() != 2){continue;}
        DimuDecay decay;
        decay.mother = &p;
        decay.mu1    = muDaughters[0];
        decay.mu2    = muDaughters[1];
        decay.extra  = nullptr; 
        bool keep = false;
        if (isEta) {
            decay.kind = photon ? DecayKind::EtaMuMuGamma : DecayKind::EtaMuMu;
            decay.extra = photon;
            keep = true;
        }
        else if (isOmega) {
            if (nExtraStable >= 1 || pi0) {
                decay.kind = DecayKind::OmegaMuMuPi0;
                decay.extra = pi0; // may be nullptr (gamma-only case)
            } else {
                decay.kind = DecayKind::OmegaMuMu;
            }
            keep = true;
        }
        else if (isPhi) {
            decay.kind = DecayKind::PhiMuMu;
            keep = true;
        }
        else if (isEtaPrime && photon) {
            decay.kind = DecayKind::EtaPrimeMuMuGamma;
            decay.extra = photon;
            keep = true;
        }
        else if (isRho) {
            decay.kind = DecayKind::RhoMuMu;
            keep = true;
        }
        if (keep) 
        {
            decays.push_back(decay);

            if (debug_) 
            {
                std::cout << "    ==> Kept decay of pdgId=" << p.pdgId() << " as ";
                switch (decay.kind) 
                {
                    case DecayKind::EtaMuMu:            std::cout << "EtaMuMu"; break;
                    case DecayKind::EtaMuMuGamma:       std::cout << "EtaMuMuGamma"; break;
                    case DecayKind::OmegaMuMu:          std::cout << "OmegaMuMu"; break;
                    case DecayKind::OmegaMuMuPi0:       std::cout << "OmegaMuMuPi0"; break;
                    case DecayKind::PhiMuMu:            std::cout << "PhiMuMu"; break;
                    case DecayKind::EtaPrimeMuMuGamma:  std::cout << "EtaPrimeMuMuGamma"; break;
                    case DecayKind::RhoMuMu:            std::cout << "RhoMuMu"; break;
                }
                std::cout << "\n";
            }
        }
    } 
    bool genDecayExists = (!decays.empty());
    std::vector<bool> decayUsed(decays.size(), false);
    // Collect Kaons from phi to KK
    std::vector<const reco::Candidate*> genKaonsFromPhi;
    for (const auto& p : *genParticles)
    {
        if (std::abs(p.pdgId()) != 333) continue; // φ
        const reco::Candidate* kplus  = nullptr;
        const reco::Candidate* kminus = nullptr;
        for (size_t d = 0; d < p.numberOfDaughters(); ++d) 
        {
        const reco::Candidate* dau = p.daughter(d);
        if (!dau) continue;
        if (dau->pdgId() ==  321) kplus  = dau;
        if (dau->pdgId() == -321) kminus = dau;
        }
        if (!kplus || !kminus) continue;
        genKaonsFromPhi.push_back(kplus);
        genKaonsFromPhi.push_back(kminus);
    }
    if (debug_) {
    std::cout << "Stored " << genKaonsFromPhi.size()
              << " charged kaons from φ\n\n";
    }   
    for (const auto& cand : *dimuons) 
    {
        pat::CompositeCandidate newCand{cand};
        // Initialize variables:
        int fromEta                = 0;
        int fromEta_MuMu           = 0;
        int fromEta_MuMuGamma      = 0;

        int fromOmega              = 0;
        int fromOmega_MuMu         = 0;
        int fromOmega_MuMuPi0      = 0;

        int fromPhi                = 0;

        int fromEtaPrime           = 0;
        int fromEtaPrime_MuMuGamma = 0;

        int fromRho                = 0;

        float etaPhoton_pt    = -67.f;
        float etaPhoton_eta   = -67.f;
        float etaPhoton_phi   = -67.f;
        float etaPhoton_DeltaR = -67.f;

        float genMuon1_pt = -67.f;
        float genMuon1_eta = -67.f;
        float genMuon1_phi = -67.f;
        float genMuon1_vtx_x = -67.f;
        float genMuon1_vtx_y = -67.f;
        float genMuon2_pt = -67.f;
        float genMuon2_eta = -67.f;
        float genMuon2_phi = -67.f;
        float genMuon2_vtx_x = -67.f;
        float genMuon2_vtx_y = -67.f;

        float genMother_vtx_x = -67.f;
        float genMother_vtx_y = -67.f;
        float genMother_pt = -67.f;

        int matchedToGenDecay = 0;
        int unmatchedButGenExists = 0;

        int from_KK = 0;
        int sameOrigin = 0;
        int trueCombinatorial = 0;
        int originValid = 0;

        bool isSwapped = false;

        // For origin studies
        int origin1_pdgId = 0;
        int origin2_pdgId = 0;
        // --- reco muons ---
        int idx1 = cand.userInt("l1_idx");
        int idx2 = cand.userInt("l2_idx");

        const pat::Muon& mu1 = (*muons)[idx1];
        const pat::Muon& mu2 = (*muons)[idx2];

        // --- try matching to GEN decays ---
        const DimuDecay* matched = nullptr;
        int matchedIndex = -1;
        for (size_t i = 0; i < decays.size(); ++i) {

            if (decayUsed[i]) continue;  // ensure unique use

            const auto& dec = decays[i];
            bool direct =
                matchMuon(mu1, *dec.mu1, debug_, "direct mu1-gen1") &&
                matchMuon(mu2, *dec.mu2, debug_, "direct mu2-gen2");

            bool swapped =
                matchMuon(mu1, *dec.mu2, debug_, "swap mu1-gen2") &&
                matchMuon(mu2, *dec.mu1, debug_, "swap mu2-gen1");

            if (direct || swapped) {
                matched       = &dec;
                matchedIndex  = i;
                matchedToGenDecay = 1;
                isSwapped = swapped; 

                if (debug_) {
                    std::cout << "  --> MATCHED dimuon to PDG " 
                            << dec.mother->pdgId()
                            << " kind=";

                    switch (dec.kind) {
                        case DecayKind::EtaMuMu:            std::cout << "EtaMuMu"; break;
                        case DecayKind::EtaMuMuGamma:       std::cout << "EtaMuMuGamma"; break;
                        case DecayKind::OmegaMuMu:          std::cout << "OmegaMuMu"; break;
                        case DecayKind::OmegaMuMuPi0:       std::cout << "OmegaMuMuPi0"; break;
                        case DecayKind::PhiMuMu:            std::cout << "PhiMuMu"; break;
                        case DecayKind::EtaPrimeMuMuGamma:  std::cout << "EtaPrimeMuMuGamma"; break;
                        case DecayKind::RhoMuMu:            std::cout << "RhoMuMu"; break;
                    }
                    std::cout << "\n";
                }
                break;
            }
        }

        if (!matchedToGenDecay && genDecayExists)
            unmatchedButGenExists = 1;
        if (matched) {

            decayUsed[matchedIndex] = true;  // enforce uniqueness

            const reco::Candidate* genMu_forRecoMu1 = nullptr;
            const reco::Candidate* genMu_forRecoMu2 = nullptr;
            // Check the swap status
            if (!isSwapped) {
                genMu_forRecoMu1 = matched->mu1;
                genMu_forRecoMu2 = matched->mu2;
            } else {
                genMu_forRecoMu1 = matched->mu2;
                genMu_forRecoMu2 = matched->mu1;
            }

            if (genMu_forRecoMu1)
            {
                genMuon1_pt = genMu_forRecoMu1->pt();
                genMuon1_eta = genMu_forRecoMu1->eta();
                genMuon1_phi = genMu_forRecoMu1->phi();
                genMuon1_vtx_x = genMu_forRecoMu1->vx();
                genMuon1_vtx_y = genMu_forRecoMu1->vy();
            }
            if (genMu_forRecoMu2)            {
                genMuon2_pt = genMu_forRecoMu2->pt();
                genMuon2_eta = genMu_forRecoMu2->eta();
                genMuon2_phi = genMu_forRecoMu2->phi();
                genMuon2_vtx_x = genMu_forRecoMu2->vx();
                genMuon2_vtx_y = genMu_forRecoMu2->vy();
            }
            genMother_vtx_x = matched->mother->vx();
            genMother_vtx_y = matched->mother->vy();

            switch (matched->kind) {

                case DecayKind::EtaMuMu:
                    fromEta = 1;
                    fromEta_MuMu = 1;
                    break;

                case DecayKind::EtaMuMuGamma:
                    fromEta = 1;
                    fromEta_MuMuGamma = 1;
                    
                    etaPhoton_pt  = matched->extra->pt();
                    etaPhoton_eta = matched->extra->eta();
                    etaPhoton_phi = matched->extra->phi();
                    etaPhoton_DeltaR = reco::deltaR(cand.userFloat("fitted_eta"), cand.userFloat("fitted_phi"), matched->extra->eta(), matched->extra->phi());
                    break;

                case DecayKind::OmegaMuMu:
                    fromOmega = 1;
                    fromOmega_MuMu = 1;
                    break;

                case DecayKind::OmegaMuMuPi0:
                    fromOmega = 1;
                    fromOmega_MuMuPi0 = 1;
                    break;

                case DecayKind::PhiMuMu:
                    fromPhi = 1;
                    break;

                case DecayKind::EtaPrimeMuMuGamma:
                    fromEtaPrime = 1;
                    fromEtaPrime_MuMuGamma = 1;
                    break;

                case DecayKind::RhoMuMu:
                    fromRho = 1;
                    break;
            }
        }
        // Here we do KK matching:
        if (!matched)
        {
            // Idea for KKtoMuMu background: Identify Kaons from phi to KK and try to match these gen. Kaons with the reco muons from the MuMu pair
            for (size_t i = 0; i + 1 < genKaonsFromPhi.size(); i += 2) {

                const reco::Candidate* k1 = genKaonsFromPhi[i];
                const reco::Candidate* k2 = genKaonsFromPhi[i + 1];

                bool direct =
                    matchMuon(mu1, *k1, debug_, "KK direct mu1-k1") &&
                    matchMuon(mu2, *k2, debug_, "KK direct mu2-k2");

                bool swapped =
                    matchMuon(mu1, *k2, debug_, "KK swap mu1-k2") &&
                    matchMuon(mu2, *k1, debug_, "KK swap mu2-k1");

                if (direct || swapped) {
                    from_KK = 1;

                    if (debug_) {
                        std::cout << "  --> MATCHED dimuon to KK at kaon-pair index "<< i;
                    }
                    break; 
                }
            }
        }
        // For combinatorial background. Add two categories. One that is events where both muons are truth matched but not to the same decay or one we are not interested in, 
        // and another is where only one of the two is matched, maybe also category where noone is matched?

        int mu1_genMatched = findMatchMuon(mu1, *genParticles) ? 1 : 0;
        int mu2_genMatched = findMatchMuon(mu2, *genParticles) ? 1 : 0;

        int pair_bothGenMatched = (mu1_genMatched && mu2_genMatched) ? 1 : 0;
        int pair_oneGenMatched  = ((mu1_genMatched + mu2_genMatched) == 1) ? 1 : 0;
        int pair_noGenMatched   = (!mu1_genMatched && !mu2_genMatched) ? 1 : 0;

        int isModeled = (fromEta || fromOmega_MuMuPi0 || fromOmega_MuMu || fromPhi || fromRho || fromEtaPrime || from_KK) ? 1 : 0;        
        int matchedButOther = (pair_bothGenMatched && !isModeled) ? 1 : 0;

        if (matchedButOther)
        {   
            const reco::Candidate* genMu1 = findBestGenMuonMatch(mu1, *genParticles);
            const reco::Candidate* genMu2 = findBestGenMuonMatch(mu2, *genParticles);
            std::cout << "\n[DEBUG] Ancestry for matched-but-other dimuon\n";
            std::cout << "  genMu1:\n";
            printAncestorChain(genMu1);
            std::cout << "  genMu2:\n";
            printAncestorChain(genMu2);
            const reco::Candidate* a1 = genMu1 ? getAncestor(genMu1) : nullptr;
            const reco::Candidate* a2 = genMu2 ? getAncestor(genMu2) : nullptr;

            originValid = (a1 && a2) ? 1 : 0;
            sameOrigin = (a1 && a2 && a1 == a2) ? 1 : 0;
            // Explicitly store the pdg ID of the ancestors for later study:
            origin1_pdgId = a1 ? a1->pdgId() : 0;
            origin2_pdgId = a2 ? a2->pdgId() : 0;
            trueCombinatorial = (pair_bothGenMatched && originValid && !sameOrigin) ? 1 : 0;
            if (debug_) {
                std::cout << "  [ORIGIN] a1=" << (a1 ? a1->pdgId() : 0)
                  << " a2=" << (a2 ? a2->pdgId() : 0)
                  << " sameOrigin=" << sameOrigin
                  << " trueCombinatorial=" << trueCombinatorial
                  << " originValid= " << originValid
                  << "\n";
                }
        }
        newCand.addUserInt("matchedToGenDecay", matchedToGenDecay);
        newCand.addUserInt("unmatchedButGenExists", unmatchedButGenExists);
        newCand.addUserInt("genDecayExists", genDecayExists ? 1 : 0);
        newCand.addUserInt("fromEta", fromEta);
        newCand.addUserInt("fromEta_MuMu", fromEta_MuMu);
        newCand.addUserInt("fromEta_MuMuGamma", fromEta_MuMuGamma);
        newCand.addUserInt("fromOmega", fromOmega);
        newCand.addUserInt("fromOmega_MuMu", fromOmega_MuMu);
        newCand.addUserInt("fromOmega_MuMuPi0", fromOmega_MuMuPi0);
        newCand.addUserInt("fromPhi", fromPhi);
        newCand.addUserInt("fromEtaPrime", fromEtaPrime);
        newCand.addUserInt("fromEtaPrime_MuMuGamma", fromEtaPrime_MuMuGamma);
        newCand.addUserInt("fromRho", fromRho);
        newCand.addUserFloat("etaPhoton_pt",   etaPhoton_pt);
        newCand.addUserFloat("etaPhoton_eta",  etaPhoton_eta);
        newCand.addUserFloat("etaPhoton_phi",  etaPhoton_phi);
        newCand.addUserFloat("etaPhoton_DeltaR", etaPhoton_DeltaR);

        newCand.addUserInt("from_KK", from_KK);

        newCand.addUserInt("isModeled", isModeled);

        newCand.addUserInt("pair_bothGenMatched", pair_bothGenMatched);
        newCand.addUserInt("pair_oneGenMatched", pair_oneGenMatched);
        newCand.addUserInt("pair_noGenMatched", pair_noGenMatched);
        newCand.addUserInt("matchedButOther", matchedButOther);


        newCand.addUserInt("sameOrigin", sameOrigin);
        newCand.addUserInt("trueCombinatorial", trueCombinatorial);
        newCand.addUserInt("originValid", originValid);

        newCand.addUserInt("origin1_pdgId", origin1_pdgId);
        newCand.addUserInt("origin2_pdgId", origin2_pdgId);

        newCand.addUserFloat("genMuon1_pt", genMuon1_pt);
        newCand.addUserFloat("genMuon1_eta", genMuon1_eta);
        newCand.addUserFloat("genMuon1_phi", genMuon1_phi);
        newCand.addUserFloat("genMuon1_vtx_x", genMuon1_vtx_x);
        newCand.addUserFloat("genMuon1_vtx_y", genMuon1_vtx_y);

        newCand.addUserFloat("genMuon2_pt", genMuon2_pt);
        newCand.addUserFloat("genMuon2_eta", genMuon2_eta);
        newCand.addUserFloat("genMuon2_phi", genMuon2_phi);
        newCand.addUserFloat("genMuon2_vtx_x", genMuon2_vtx_x);
        newCand.addUserFloat("genMuon2_vtx_y", genMuon2_vtx_y);

        newCand.addUserFloat("genMother_vtx_x", genMother_vtx_x);
        newCand.addUserFloat("genMother_vtx_y", genMother_vtx_y);
        newCand.addUserFloat("genMother_pt", genMother_pt);
        out->push_back(newCand);
    }  
    // store final collection
    evt.put(std::move(out), "SelectedMuMuExtended");
}
DEFINE_FWK_MODULE(NewGenMatcher);
