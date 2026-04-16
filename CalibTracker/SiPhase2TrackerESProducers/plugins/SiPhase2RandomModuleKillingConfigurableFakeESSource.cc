// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      SiPhase2BadModuleConfigurableFakeESSource
//
/**\class SiPhase2BadModuleConfigurableFakeESSource

 Description: "fake" SiStripBadStrip ESProducer - configurable random list of bad Ph2 2S modules

 Implementation:
      Adapted to Phase-2 from CalibTracker/SiStripESProducers/plugins/fake/SiStripBadModuleConfigurableFakeESSource.cc,
      with random selection of a fraction of 2S modules.
*/

#include <memory>
#include <set>
#include <vector>
#include <algorithm>
#include <cmath>

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

// random
#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandFlat.h"

class SiPhase2RandomModuleKillingConfigurableFakeESSource : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder {
public:
  SiPhase2RandomModuleKillingConfigurableFakeESSource(const edm::ParameterSet&);
  ~SiPhase2RandomModuleKillingConfigurableFakeESSource() override = default;

  void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                      const edm::IOVSyncValue& iov,
                      edm::ValidityInterval& iValidity) override;

  using ReturnType = std::unique_ptr<SiStripBadStrip>;
  ReturnType produce(const SiPhase2OuterTrackerBadStripRcd&);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  bool is2SModule(const TrackerGeometry&, DetId detid) const;
  std::vector<uint32_t> collectEligibleDetIds(const TrackerGeometry&) const;
  std::vector<uint32_t> selectRandomDetIds(const std::vector<uint32_t>& detIds) const;

  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;

  bool debug_;
  double badModulesFraction_;
  std::unique_ptr<CLHEP::HepJamesRandom> engine_;
};

SiPhase2RandomModuleKillingConfigurableFakeESSource::SiPhase2RandomModuleKillingConfigurableFakeESSource(const edm::ParameterSet& iConfig)
    : debug_(iConfig.getUntrackedParameter<bool>("debug", false)),
      badModulesFraction_(iConfig.getParameter<double>("badModulesFraction")),
      engine_(std::make_unique<CLHEP::HepJamesRandom>(iConfig.getParameter<unsigned int>("seed"))) {
  auto cc = setWhatProduced(this);
  geomToken_ = cc.consumes();

  if (badModulesFraction_ < 0. || badModulesFraction_ > 1.) {
    throw cms::Exception("Inconsistent configuration")
        << "badModulesFraction must be between 0 and 1, but is " << badModulesFraction_;
  }

  findingRecord<SiPhase2OuterTrackerBadStripRcd>();
}

void SiPhase2RandomModuleKillingConfigurableFakeESSource::setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                                                        const edm::IOVSyncValue& iov,
                                                                        edm::ValidityInterval& iValidity) {
  iValidity = edm::ValidityInterval{iov.beginOfTime(), iov.endOfTime()};
}

bool SiPhase2RandomModuleKillingConfigurableFakeESSource::is2SModule(const TrackerGeometry& tGeom, DetId detid) const {
    uint32_t rawId = detid.rawId();
    if (detid.det() == DetId::Detector::Tracker) {
        if (tGeom.getDetectorType(rawId) == TrackerGeometry::ModuleType::Ph2SS) {
          return true;
      }
    }
    return false;
}

std::vector<uint32_t> SiPhase2RandomModuleKillingConfigurableFakeESSource::collectEligibleDetIds(const TrackerGeometry& tGeom) const {
    std::set<uint32_t> uniqueDetIds;

    for (auto const& det_u : tGeom.detUnits()) {
      const DetId detid = det_u->geographicalId();
      if (is2SModule(tGeom, detid)) {
        uniqueDetIds.insert(detid.rawId());
      }
    }
    return std::vector<uint32_t>(uniqueDetIds.begin(), uniqueDetIds.end());
}

std::vector<uint32_t> SiPhase2RandomModuleKillingConfigurableFakeESSource::selectRandomDetIds(const std::vector<uint32_t>& detIds) const {
    std::vector<uint32_t> shuffledDetIds = detIds;
    // std::shuffle(shuffledDetIds.begin(), shuffledDetIds.end(), *engine_); // engine_ does not work in shuffle need to think of something else
    // manually shuffle using CLHEP?
    for (size_t i = shuffledDetIds.size() - 1; i > 0; --i) {
        size_t j = static_cast<size_t>(CLHEP::RandFlat::shoot(engine_.get(), 0, i + 1));
        std::swap(shuffledDetIds[i], shuffledDetIds[j]);
    }
    if (debug_) {
        edm::LogPrint("SiPhase2RandomModuleKillingConfigurableFakeESSource") << "Printing first 10 shuffled detIds:";
        for (size_t i = 0; i < std::min(size_t(10), shuffledDetIds.size()); ++i) {
            edm::LogPrint("SiPhase2RandomModuleKillingConfigurableFakeESSource") << "Shuffled detId " << i << ": " << shuffledDetIds[i];
        }
    }
    const size_t numToSelect = static_cast<size_t>(std::floor(badModulesFraction_ * detIds.size()));
    shuffledDetIds.resize(numToSelect);
    return shuffledDetIds;
}

std::unique_ptr<SiStripBadStrip> SiPhase2RandomModuleKillingConfigurableFakeESSource::produce(const SiPhase2OuterTrackerBadStripRcd& iRecord) {
  using Phase2TrackerGeomDetUnit = PixelGeomDetUnit; 
  using namespace edm::es;
  TrackerGeometry const& tGeom = iRecord.get(geomToken_);
  auto product = std::make_unique<SiStripBadStrip>();
  const std::vector<uint32_t> eligibleDetIds = collectEligibleDetIds(tGeom);
  if (debug_) {
    edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource")
        << "Found " << eligibleDetIds.size() << " eligible 2S modules.";
  }
  const std::vector<uint32_t> selectedDetIds = selectRandomDetIds(eligibleDetIds);
  if (debug_) {
    edm::LogPrint("SiPhase2BadModuleConfigurableFakeESSource")
        << "Randomly selected " << selectedDetIds.size() << " / " << eligibleDetIds.size()
        << " 2S modules to be killed (fraction = " << badModulesFraction_ << ").";
  }

  for (const auto& rawId : selectedDetIds) {
    
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


void  SiPhase2RandomModuleKillingConfigurableFakeESSource::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setComment("Configurable Fake Phase-2 Outer Tracker Bad 2S Module ESSource");
  desc.add<unsigned int>("seed", 67)->setComment("random seed");
  desc.add<double>("badModulesFraction", 0.1)->setComment("fraction of 2S modules to kill");
  desc.addUntracked<bool>("debug", false)->setComment("enable debug printout");
  descriptions.add("SiPhase2RandomModuleKillingConfigurableFakeESSource", desc);
}

#include "FWCore/Framework/interface/SourceFactory.h"
DEFINE_FWK_EVENTSETUP_SOURCE(SiPhase2RandomModuleKillingConfigurableFakeESSource);
