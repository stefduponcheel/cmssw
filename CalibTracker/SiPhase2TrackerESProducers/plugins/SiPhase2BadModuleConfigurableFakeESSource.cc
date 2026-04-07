// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      SiPhase2BadModuleConfigurableFakeESSource
//
/**\class SiPhase2BadModuleConfigurableFakeESSource

 Description: "fake" SiStripBadStrip ESProducer - configurable list of bad Ph2 2S modules 

 Implementation:
      Adapted to Phase-2 from CalibTracker/SiStripESProducers/plugins/fake/SiStripBadModuleConfigurableFakeESSource.cc, inspired by CalibTracker/SiPhase2TrackerESProducers/plugins/SiPhase2BadStripConfigurableFakeESSource.cc
*/
//
// Original Author:  Stef Duponcheel
//         Created:  Mon, 23 Mar 2026 10:15:33 GMT
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/ModuleFactory.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "CondFormats/DataRecord/interface/SiPhase2OuterTrackerCondDataRecords.h"
#include "CondFormats/SiStripObjects/interface/SiStripBadStrip.h"
#include "DataFormats/Phase2TrackerDigi/interface/Phase2TrackerDigi.h"
#include "DataFormats/SiStripDetId/interface/SiStripDetId.h"
#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "FWCore/AbstractServices/interface/RandomNumberGenerator.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include <set>



class SiPhase2BadModuleConfigurableFakeESSource : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder {
public:
  SiPhase2BadModuleConfigurableFakeESSource(const edm::ParameterSet&);
  ~SiPhase2BadModuleConfigurableFakeESSource() override;

  void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                      const edm::IOVSyncValue& iov,
                      edm::ValidityInterval& iValidity) override;

  using ReturnType = std::unique_ptr<SiStripBadStrip>;
  ReturnType produce(const SiPhase2OuterTrackerBadStripRcd&);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  std::vector<uint32_t> collectEligibleDetIds(const TrackerGeometry&) const;
  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;
  bool is2SModule(const TrackerGeometry&, DetId detid) const;
  bool debug_;
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
SiPhase2BadModuleConfigurableFakeESSource::SiPhase2BadModuleConfigurableFakeESSource(const edm::ParameterSet& iConfig) : debug_(iConfig.getUntrackedParameter<bool>("debug", false)) {
  //the following line is needed to tell the framework what
  // data is being produced

  auto cc = setWhatProduced(this);
  geomToken_ = cc.consumes();
  
  findingRecord<SiPhase2OuterTrackerBadStripRcd>();

}

SiPhase2BadModuleConfigurableFakeESSource::~SiPhase2BadModuleConfigurableFakeESSource() {
  // do anything here that needs to be done at destruction time
  // (e.g. close files, deallocate resources etc.)
}

void SiPhase2BadModuleConfigurableFakeESSource::setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                                              const edm::IOVSyncValue& iov,
                                                              edm::ValidityInterval& iValidity) {
  iValidity = edm::ValidityInterval{iov.beginOfTime(), iov.endOfTime()};
}

bool SiPhase2BadModuleConfigurableFakeESSource::is2SModule(const TrackerGeometry& tGeom, DetId detid) const {
    uint32_t rawId = detid.rawId();
    if (detid.det() == DetId::Detector::Tracker) {
        if (tGeom.getDetectorType(rawId) == TrackerGeometry::ModuleType::Ph2SS) {
          return true;
      }
    }
    return false;
}

std::vector<uint32_t> SiPhase2BadModuleConfigurableFakeESSource::collectEligibleDetIds(const TrackerGeometry& tGeom) const {
    std::set<uint32_t> uniqueDetIds;

    for (auto const& det_u : tGeom.detUnits()) {
      const DetId detid = det_u->geographicalId();
      if (is2SModule(tGeom, detid)) {
        uniqueDetIds.insert(detid.rawId());
      }
    }
    return std::vector<uint32_t>(uniqueDetIds.begin(), uniqueDetIds.end());
}

// ------------ method called to produce the data  ------------
SiPhase2BadModuleConfigurableFakeESSource::ReturnType SiPhase2BadModuleConfigurableFakeESSource::produce(const SiPhase2OuterTrackerBadStripRcd& iRecord) {
  using Phase2TrackerGeomDetUnit = PixelGeomDetUnit; 
  using namespace edm::es;
  TrackerGeometry const& tGeom = iRecord.get(geomToken_);
  auto product = std::make_unique<SiStripBadStrip>();
  const std::vector<uint32_t> eligibleDetIds = collectEligibleDetIds(tGeom);

  for (const auto& rawId : eligibleDetIds){

    const auto* detUnit = tGeom.idToDetUnit(DetId(rawId));
    const auto* pixdet = dynamic_cast<const Phase2TrackerGeomDetUnit*>(detUnit);
    if (!pixdet) {
      if (debug_) {
        edm::LogError("SiPhase2BadModuleConfigurableFakeESSource") << "Could not cast detUnit to Phase2TrackerGeomDetUnit for DetId: " << rawId;
      }
      continue; 
    }
    unsigned int maxChannel = 2039; // Initialize to the maximum possible channel number (for 2S modules)- Check Phase2TrackerDigi::pixelToChannel(row, col) implementation.
    const unsigned int nChannelsEncoded = maxChannel + 1;

    std::vector<unsigned int> theSiStripVector; // vector to collect all bad ranges for this one module
    unsigned int theBadChannelsRange;
    theBadChannelsRange = product->encodePhase2(0, nChannelsEncoded); // mark all channels as bad
    theSiStripVector.push_back(theBadChannelsRange); 
    SiStripBadStrip::Range range(theSiStripVector.begin(), theSiStripVector.end());
    if (!product->put(rawId, range)) {
      edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource") << " detid already exists for rawId: " << rawId;
      } 
    }
  return product;
}
void SiPhase2BadModuleConfigurableFakeESSource::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.addUntracked<bool>("debug", false);
  descriptions.add("SiPhase2BadModuleConfigurableFakeESSource", desc);
}
//define this as a plug-in
#include "FWCore/Framework/interface/SourceFactory.h"
DEFINE_FWK_EVENTSETUP_SOURCE(SiPhase2BadModuleConfigurableFakeESSource);
