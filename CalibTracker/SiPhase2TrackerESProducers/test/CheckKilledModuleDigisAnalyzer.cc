// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      CheckKilledModuleDigisAnalyzer
//
/**\class CheckKilledModuleDigisAnalyzer

 Description: Throwaway test analyzer: cross-checks that modules marked dead in
 Phase2OTQualityRcd genuinely have zero digis in the digitizer output, and that
 non-killed modules aren't all empty too (a sanity check that digitization is actually
 happening, not that everything is silently empty). Not part of the physics chain.
*/

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/Phase2TrackerDigi/interface/Phase2TrackerDigi.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CondFormats/DataRecord/interface/SiPhase2OuterTrackerCondDataRecords.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"

class CheckKilledModuleDigisAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit CheckKilledModuleDigisAnalyzer(const edm::ParameterSet&);

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  edm::ESGetToken<SiPixelQuality, Phase2OTQualityRcd> qualityToken_;
  edm::EDGetTokenT<edm::DetSetVector<Phase2TrackerDigi>> digiToken_;
};

CheckKilledModuleDigisAnalyzer::CheckKilledModuleDigisAnalyzer(const edm::ParameterSet& iConfig)
    : qualityToken_(esConsumes()),
      digiToken_(consumes<edm::DetSetVector<Phase2TrackerDigi>>(iConfig.getParameter<edm::InputTag>("digis"))) {}

void CheckKilledModuleDigisAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  const SiPixelQuality& quality = iSetup.getData(qualityToken_);
  const auto& digiCollection = iEvent.get(digiToken_);

  int killedModulesWithDigis = 0;
  int killedModulesChecked = 0;
  int liveModulesWithDigis = 0;

  for (auto const& detSet : digiCollection) {
    const uint32_t detId = detSet.detId();
    const bool isKilled = quality.IsModuleBad(detId);
    if (isKilled) {
      ++killedModulesChecked;
      if (!detSet.empty()) {
        ++killedModulesWithDigis;
        edm::LogPrint("CheckKilledModuleDigisAnalyzer")
            << "FAIL: killed module " << detId << " has " << detSet.size() << " digi(s) — should be zero";
      }
    } else if (!detSet.empty()) {
      ++liveModulesWithDigis;
    }
  }

  edm::LogPrint("CheckKilledModuleDigisAnalyzer")
      << "Event " << iEvent.id().event() << ": digi collection has " << digiCollection.size()
      << " module entries; of those, " << killedModulesChecked << " belong to killed modules ("
      << killedModulesWithDigis << " of which wrongly have digis); " << liveModulesWithDigis
      << " non-killed modules have digis.";
}

DEFINE_FWK_MODULE(CheckKilledModuleDigisAnalyzer);
