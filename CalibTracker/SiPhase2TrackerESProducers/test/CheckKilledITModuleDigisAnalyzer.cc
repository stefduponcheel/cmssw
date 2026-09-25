// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      CheckKilledITModuleDigisAnalyzer
//
/**\class CheckKilledITModuleDigisAnalyzer

 Description: Throwaway test analyzer: cross-checks that IT modules marked dead in
 SiPhase2ITQualityRcd genuinely have zero digis in the digitizer output (PixelDigi,
 label "Pixel"), and that non-killed modules aren't all empty too. Not part of the
 physics chain.
*/

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CondFormats/DataRecord/interface/SiPhase2ITQualityRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"

class CheckKilledITModuleDigisAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit CheckKilledITModuleDigisAnalyzer(const edm::ParameterSet&);

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  edm::ESGetToken<SiPixelQuality, SiPhase2ITQualityRcd> qualityToken_;
  edm::EDGetTokenT<edm::DetSetVector<PixelDigi>> digiToken_;
};

CheckKilledITModuleDigisAnalyzer::CheckKilledITModuleDigisAnalyzer(const edm::ParameterSet& iConfig)
    : qualityToken_(esConsumes()),
      digiToken_(consumes<edm::DetSetVector<PixelDigi>>(iConfig.getParameter<edm::InputTag>("digis"))) {}

void CheckKilledITModuleDigisAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
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
        edm::LogPrint("CheckKilledITModuleDigisAnalyzer")
            << "FAIL: killed module " << detId << " has " << detSet.size() << " digi(s) — should be zero";
      }
    } else if (!detSet.empty()) {
      ++liveModulesWithDigis;
    }
  }

  edm::LogPrint("CheckKilledITModuleDigisAnalyzer")
      << "Event " << iEvent.id().event() << ": digi collection has " << digiCollection.size()
      << " module entries; of those, " << killedModulesChecked << " belong to killed modules ("
      << killedModulesWithDigis << " of which wrongly have digis); " << liveModulesWithDigis
      << " non-killed modules have digis.";
}

DEFINE_FWK_MODULE(CheckKilledITModuleDigisAnalyzer);
