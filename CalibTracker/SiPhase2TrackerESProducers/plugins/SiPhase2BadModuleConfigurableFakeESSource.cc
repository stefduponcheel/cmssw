// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      SiPhase2BadModuleConfigurableFakeESSource
//
/**\class SiPhase2BadModuleConfigurableFakeESSource

 Description: "fake" SiStripBadStrip ESProducer - configurable list of bad Ph2 OT modules 

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


// needed for the random number generation
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/JamesRandom.h"



class SiPhase2BadModuleConfigurableFakeESSource : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder {
public:
  SiPhase2BadModuleConfigurableFakeESSource(const edm::ParameterSet&);
  ~SiPhase2BadModuleConfigurableFakeESSource() override;

  void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                      const edm::IOVSyncValue& iov,
                      edm::ValidityInterval& iValidity) override;

  using ReturnType = std::unique_ptr<SiStripBadStrip>;
  ReturnType produce(const SiPhase2OuterTrackerBadStripRcd&);
private:
  std::vector<uint32_t> collectEligibleDetIds(const TrackerGeometry&) const;
  bool isOuterTrackerStripModule(const TrackerGeometry&, DetId detid) const;
  // es tokens
  edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> trackTopoToken_;
  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;
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
SiPhase2BadModuleConfigurableFakeESSource::SiPhase2BadModuleConfigurableFakeESSource(const edm::ParameterSet& iConfig) {
  //the following line is needed to tell the framework what
  // data is being produced
  auto cc = setWhatProduced(this);
  trackTopoToken_ = cc.consumes();
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
bool SiPhase2BadModuleConfigurableFakeESSource::isOuterTrackerStripModule(const TrackerGeometry& tGeom, DetId detid) const {
    uint32_t rawId = detid.rawId();
    int subid = detid.subdetId();
    if (detid.det() == DetId::Detector::Tracker) {
      if (subid == StripSubdetector::TOB || subid == StripSubdetector::TID) {
        if (tGeom.getDetectorType(rawId) == TrackerGeometry::ModuleType::Ph2PSS ||
            tGeom.getDetectorType(rawId) == TrackerGeometry::ModuleType::Ph2SS) {
          return true;
        }
      }
    }
    return false;
}
std::vector<uint32_t> SiPhase2BadModuleConfigurableFakeESSource::collectEligibleDetIds(const TrackerGeometry& tGeom) const {
  std::vector<uint32_t> eligibleDetIds;
  edm::LogPrint("collectEligibleDetIds") << "Collecting eligible DetIds for bad module generation...";
  for (auto const& det_u : tGeom.detUnits())
  { 
    const DetId detid = det_u->geographicalId();
    bool isEligible = isOuterTrackerStripModule(tGeom, detid);
    if (isEligible) {
      eligibleDetIds.push_back(detid.rawId());
    }
  }
  return eligibleDetIds;
}

// ------------ method called to produce the data  ------------
SiPhase2BadModuleConfigurableFakeESSource::ReturnType SiPhase2BadModuleConfigurableFakeESSource::produce(const SiPhase2OuterTrackerBadStripRcd& iRecord) {
  // You can add arguments to the make_unique function call
  // and they will be forwarded to the constructor of the
  // data object. Also you can call functions that modify
  // the data object after creating it. Often, before this
  // you will retrieve data from the EventSetup through the
  // record.
  //
  bool debug = false;
  bool killAllModules_ = true;
  int counter = 0;
  using Phase2TrackerGeomDetUnit = PixelGeomDetUnit; 
  using namespace edm::es;
  TrackerGeometry const& tGeom = iRecord.get(geomToken_);
  if (debug) {
    edm::LogPrint("collectEligibleDetIds") << "There are " << tGeom.detUnits().size() << " modules in this geometry.";
  }
  auto product = std::make_unique<SiStripBadStrip>();
  const std::vector<uint32_t> eligibleDetIds = collectEligibleDetIds(tGeom);
  if (debug) {
    edm::LogPrint("produce") << "Eligible DetIds for bad module generation: " << eligibleDetIds.size();
  }
  for (const auto& rawId : eligibleDetIds){
    if (debug) {
      edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource") << "Processing DetId: " << rawId;
    }
    const auto* detUnit = tGeom.idToDetUnit(DetId(rawId)); // Ask, is this the right way to get the detUnit for a given rawId?
    if (!detUnit) {
      if (debug) {
        edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource") << "Could not retrieve detUnit for DetId: " << rawId;
      }
      continue;
    }
    const auto* pixdet = dynamic_cast<const Phase2TrackerGeomDetUnit*>(detUnit);
    if (!pixdet) {
      if (debug) {
        edm::LogError("SiPhase2BadModuleConfigurableFakeESSource") << "Could not cast detUnit to Phase2TrackerGeomDetUnit for DetId: " << rawId;
      }
      continue; 
    }
    const PixelTopology& topol = pixdet->specificTopology();
    const unsigned int nrows = topol.nrows();
    const unsigned int ncols = topol.ncolumns();
    const unsigned int nChannels = nrows * ncols;
    const int subid = DetId(rawId).subdetId();
    const auto moduleType = tGeom.getDetectorType(rawId);

    if (debug) {
      edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource")
        << "Module " << counter
        << " rawId=" << rawId
        << " subdet=" << subid
        << " moduleType=" << static_cast<int>(moduleType)
        << " nrows=" << nrows
      << " ncols=" << ncols
      << " nchannels=" << nChannels;  
    }
    // Kill the first module:
    if (killAllModules_ || counter == 0) {
      std::vector<unsigned int> theSiStripVector; // vector to collect all bad ranges for this one module
      unsigned int theBadChannelsRange;
      theBadChannelsRange = product->encodePhase2(0, nChannels); // mark all channels as bad
      theSiStripVector.push_back(theBadChannelsRange); 
      SiStripBadStrip::Range range(theSiStripVector.begin(), theSiStripVector.end());
      if (!product->put(rawId, range)) {
        edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource") << "[SiPhase2BadModuleConfigurableFakeESSource::produce] detid already exists for rawId: " << rawId;
      } else {
        if (debug) {
          edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource") << "Marked entire module as bad for DetId: " << rawId;
          }     
        }
    }
    counter++;
  }
  return product;
}

//define this as a plug-in
#include "FWCore/Framework/interface/SourceFactory.h"
DEFINE_FWK_EVENTSETUP_SOURCE(SiPhase2BadModuleConfigurableFakeESSource);
