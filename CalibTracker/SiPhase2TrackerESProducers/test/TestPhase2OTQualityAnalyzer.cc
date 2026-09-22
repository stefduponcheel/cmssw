// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      TestPhase2OTQualityAnalyzer
//
/**\class TestPhase2OTQualityAnalyzer

 Description: Throwaway test analyzer: requests the SiPixelQuality product from
 Phase2OTQualityRcd once per event, purely to trigger
 SiPhase2OTFakeQualityESSource::produce() so its debug output can be inspected.
 Not part of the physics chain.
*/

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CondFormats/DataRecord/interface/SiPhase2OuterTrackerCondDataRecords.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

class TestPhase2OTQualityAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit TestPhase2OTQualityAnalyzer(const edm::ParameterSet&);

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  edm::ESGetToken<SiPixelQuality, Phase2OTQualityRcd> qualityToken_;
  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;
};

TestPhase2OTQualityAnalyzer::TestPhase2OTQualityAnalyzer(const edm::ParameterSet&)
    : qualityToken_(esConsumes()), geomToken_(esConsumes()) {}

void TestPhase2OTQualityAnalyzer::analyze(const edm::Event&, const edm::EventSetup& iSetup) {
  const SiPixelQuality& quality = iSetup.getData(qualityToken_);
  edm::LogPrint("TestPhase2OTQualityAnalyzer")
      << "Retrieved SiPixelQuality with " << quality.getBadComponentList().size() << " disabled entries.";

  // Sanity-check the actual consumption API (IsModuleBad/IsModuleUsable) that the digitizer
  // and tracking code will rely on later — not just the raw list size.
  if (!quality.getBadComponentList().empty()) {
    const uint32_t knownBadId = quality.getBadComponentList().front().DetID;
    edm::LogPrint("TestPhase2OTQualityAnalyzer")
        << "IsModuleBad(" << knownBadId << ") [should be true]  = " << quality.IsModuleBad(knownBadId);
    edm::LogPrint("TestPhase2OTQualityAnalyzer")
        << "IsModuleUsable(" << knownBadId << ") [should be false] = " << quality.IsModuleUsable(knownBadId);
  }

  const TrackerGeometry& tGeom = iSetup.getData(geomToken_);
  for (auto const& det : tGeom.dets()) {
    const uint32_t detId = det->geographicalId().rawId();
    if (!quality.IsModuleBad(detId)) {
      edm::LogPrint("TestPhase2OTQualityAnalyzer")
          << "IsModuleUsable(" << detId << ") [should be true, a module we never touched] = "
          << quality.IsModuleUsable(detId);
      break;
    }
  }
}

DEFINE_FWK_MODULE(TestPhase2OTQualityAnalyzer);
