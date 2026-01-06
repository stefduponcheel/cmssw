#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "CommonTools/Utils/interface/StringCutObjectSelector.h"
#include "helper.h"
#include <iostream>

#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PFParticle.h"

#include "DataFormats/Common/interface/ValueMap.h"
// https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuidePATUserData
// No addUserData for pat::PackedCandidate, so we create a ValueMap instead. This produces external variables that need to be read into the flat table.
class getPackedPFphotons : public edm::stream::EDProducer<> 
{
    public: 
        explicit getPackedPFphotons(const edm::ParameterSet& cfg);
        ~getPackedPFphotons() override {}

        void produce(edm::Event&, const edm::EventSetup&) override;
        double photonPfIso03(pat::PackedCandidate pho, edm::Handle<pat::PackedCandidateCollection> pfcands);
    private:
        const edm::EDGetTokenT<pat::PackedCandidateCollection> pfCandsToken;
        const edm::EDGetTokenT<std::vector<pat::CompositeCandidate>> mumuToken_;


};
getPackedPFphotons::getPackedPFphotons(const edm::ParameterSet& cfg)
    : pfCandsToken(consumes<pat::PackedCandidateCollection>(cfg.getParameter<edm::InputTag>("src_pfCands"))),
      mumuToken_(consumes<std::vector<pat::CompositeCandidate>>(cfg.getParameter<edm::InputTag>("src_mumu")))
{
    produces<pat::PackedCandidateCollection>("PackedPFphotons");
    produces<edm::ValueMap<double>>("PhotonPfIso03");
    produces<edm::ValueMap<double>>("PhotonDr");
}
double getPackedPFphotons::photonPfIso03(pat::PackedCandidate pho, edm::Handle<pat::PackedCandidateCollection> pfcands)
{
    double ptsum = 0.0;
    for (const pat::PackedCandidate &pfc : *pfcands) {

        double dR = deltaR(pho.p4(), pfc.p4());

        if (dR>=0.3) continue;

        if (pfc.charge()!=0 && abs(pfc.pdgId())==211 && pfc.pt()>0.2) {
            if (dR>0.0001) ptsum+=pfc.pt();
        } 
        else if (pfc.charge()==0 && (abs(pfc.pdgId())==22||abs(pfc.pdgId())==130) && pfc.pt()>0.5) {
            if (dR>0.01) ptsum+=pfc.pt();
        }
    }
    return ptsum/pho.pt();
}
void getPackedPFphotons::produce(edm::Event& iEvent, const edm::EventSetup&)
{
    edm::Handle<pat::PackedCandidateCollection> pfCands;
    edm::Handle<std::vector<pat::CompositeCandidate>> dimuons;
    iEvent.getByToken(mumuToken_, dimuons);
    iEvent.getByToken(pfCandsToken, pfCands);
    auto out = std::make_unique<pat::PackedCandidateCollection>();
    std::vector<double> isoVals;
    std::vector<double> drVals;
    // Idea is to store all PF photons that have a DR <0.5 w.r.t. the dimuon pair in the event. There is only one dimuon per event in our case which simplifies things. I then want to store the properties of the all the PF photons that pass this criteria.
    for (const auto& dimuon : *dimuons) //Why do we have to do *dimuons here? and not dimuons? Because dimuons is a handle, we need to dereference it to get the actual collection. Handles are like smart pointers that manage access to the data in the event.
    {
        for (const auto& pf : *pfCands)
        {
            if (pf.pdgId() !=22) continue; //Only photons
            // Askin meeting about whether or not to do a pt cut here. For now, I won't.
            double dR = reco::deltaR(dimuon.eta(), dimuon.phi(), pf.eta(), pf.phi());
            if (dR <0.5)
            {
                pat::PackedCandidate newPFPhoton {pf};
                double iso03 = photonPfIso03(pf, pfCands);
                drVals.push_back(dR);
                isoVals.push_back(iso03);
                out->push_back(newPFPhoton);
            }
        }
    }
    auto outH = iEvent.put(std::move(out), "PackedPFphotons");
    auto isoMap = std::make_unique<edm::ValueMap<double>>();
    auto drMap = std::make_unique<edm::ValueMap<double>>(); 
    edm::ValueMap<double>::Filler isoFiller(*isoMap);
    edm::ValueMap<double>::Filler drFiller(*drMap);
    isoFiller.insert(outH, isoVals.begin(), isoVals.end());
    drFiller.insert(outH,  drVals.begin(),  drVals.end());
    isoFiller.fill();
    drFiller.fill();
    iEvent.put(std::move(isoMap), "PhotonPfIso03");
    iEvent.put(std::move(drMap), "PhotonDr");
}

DEFINE_FWK_MODULE(getPackedPFphotons);
