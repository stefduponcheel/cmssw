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

    struct MuFromKaon {
    const reco::Candidate* genMu;
    const reco::Candidate* kaon;
    };

    bool matchMuon(const pat::Muon& reco,const reco::Candidate& gen,bool debug,const std::string& tag) const;

    const reco::Candidate* getAncestor(const reco::Candidate* gen, int lepId=13) const; // Get the first non-muon ancestor of a given gen. muon
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
const reco::Candidate* NewGenMatcher::getAncestor(const reco::Candidate* gen, int lepId) const
{
    if (!gen || gen->numberOfMothers() == 0)return nullptr;

    const reco::Candidate* mother = gen->mother();
    int motherId = std::abs(mother->pdgId());

    if (motherId == lepId) return getAncestor(mother,lepId);
    return mother;
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
    for (size_t i = 0; i < muons->size(); ++i) {

        const pat::Muon& mu = (*muons)[i];

        if (!mu.genParticle()) continue;

        const reco::Candidate* gen = mu.genParticle();

        if (debug_) {
            std::cout << "[MUON GEN MATCH] "
                    << "i=" << i
                    << " reco(pt=" << mu.pt()
                    << ", eta=" << mu.eta()
                    << ", phi=" << mu.phi()
                    << ")"
                    << " <- gen(pdgId=" << gen->pdgId()
                    << ", status=" << gen->status()
                    << ", pt=" << gen->pt()
                    << ", eta=" << gen->eta()
                    << ")\n";
        }
    }
    // ==========================================================
    // Phase 1: find all mu mu decays at GEN level
    // ==========================================================
    std::vector<DimuDecay> decays;
    std::vector<MuFromKaon> muFromKaons;
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
            if (dau->status() != 1) continue;
            if (std::abs(id) == 13 ) {
                muDaughters.push_back(dau);
            } else {
                nExtraStable++;
                if ( (id == 22) && (isEta || isEtaPrime) && !photon ) //Store the photon as extra particle for eta or eta prime
                photon = dau;
            }
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
    // Collect muons originating from a Kaon decay
    std::vector<const reco::Candidate*> genMuFromKaons;
    // Collect Kaons from phi to KK
    std::vector<const reco::Candidate*> genKaonsFromPhi;

    for (const auto& p : *genParticles) {

        int pid = std::abs(p.pdgId());
        if (pid != 321 || p.status() != 2) continue;
        if (debug_) {
        std::cout << "\nFound charged kaon pdgId=" << p.pdgId()
                  << " status=" << p.status()
                  << " pt=" << p.pt()
                  << " with " << p.numberOfDaughters()
                  << " daughters\n";
        }
        // Loop over kaon daughters
        for (size_t d = 0; d < p.numberOfDaughters(); ++d) {
            const reco::Candidate* dau = p.daughter(d);
            if (!dau) continue;
            // Select final-state muons
            if (std::abs(dau->pdgId()) == 13 && dau->status() == 1)
            {
                genMuFromKaons.push_back(dau);
                if (debug_)
                {
                    std::cout << "    >>> STORED GEN muon from kaon "<< "(charge=" << dau->charge() << ")\n";
                }
            }

        }
    }
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
    if (debug_) {
        std::cout << "\n=== Total GEN muons from charged kaons: "<< genMuFromKaons.size() << " ===\n\n";
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

        int matchedToGenDecay = 0;
        int unmatchedButGenExists = 0;
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

        int from_KK = 0;
        if (!matched)
        {
            if (mu1.genParticle() && mu2.genParticle()) {
            const reco::Candidate* g1 = mu1.genParticle();
            const reco::Candidate* g2 = mu2.genParticle();
            if (debug_) {
                std::cout << "  [KK MIS-ID] dimuon tagged as fromKK\n";

                std::cout << "    reco mu1: pt=" << mu1.pt()
                          << " eta=" << mu1.eta()
                          << " phi=" << mu1.phi()
                          << " charge=" << mu1.charge() << "\n";

                std::cout << "      gen match: pdgId=" << g1->pdgId()
                          << " status=" << g1->status()
                          << " pt=" << g1->pt()
                          << " eta=" << g1->eta()
                          << " phi=" << g1->phi() << "\n";

                std::cout << "    reco mu2: pt=" << mu2.pt()
                          << " eta=" << mu2.eta()
                          << " phi=" << mu2.phi()
                          << " charge=" << mu2.charge() << "\n";

                std::cout << "      gen match: pdgId=" << g2->pdgId()
                          << " status=" << g2->status()
                          << " pt=" << g2->pt()
                          << " eta=" << g2->eta()
                          << " phi=" << g2->phi() << "\n";
                }
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

        out->push_back(newCand);
    }
    // store final collection
    evt.put(std::move(out), "SelectedMuMuExtended");
}
DEFINE_FWK_MODULE(NewGenMatcher);
