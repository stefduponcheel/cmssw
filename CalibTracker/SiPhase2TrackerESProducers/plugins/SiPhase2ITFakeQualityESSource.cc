// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      SiPhase2ITFakeQualityESSource
//
/**\class SiPhase2ITFakeQualityESSource

 Description: "fake" SiPixelQuality ESProducer for Phase-2 Inner Tracker module killing —
 configurable random selection of dead modules, selected at the physical module level.

 Note: 
*/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"
#include "FWCore/Framework/interface/ModuleFactory.h"
#include "FWCore/Framework/interface/SourceFactory.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "CondFormats/DataRecord/interface/SiPhase2ITQualityRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"

#include "Geometry/CommonTopologies/interface/StackGeomDet.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandFlat.h"

class SiPhase2ITFakeQualityESSource : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder {
public:
  SiPhase2ITFakeQualityESSource(const edm::ParameterSet&);
  ~SiPhase2ITFakeQualityESSource() override = default;

  using ReturnType = std::unique_ptr<SiPixelQuality>;
  ReturnType produce(const SiPhase2ITQualityRcd&);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

protected:
  void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                       const edm::IOVSyncValue&,
                       edm::ValidityInterval&) override;

private:
  // IT modules are not stacks like OT modules, so this changes.
  std::vector<const GeomDet*> collectEligibleModules(const TrackerGeometry& tGeom,
                                                          const std::string& moduleType) const;

  // Fisher-Yates shuffle + truncate to the requested fraction. Uses engine_ directly
  // (std::shuffle doesn't work with the CLHEP engine, same reasoning as the old code).
  std::vector<const GeomDet*> selectRandomModules(const std::vector<const GeomDet*>& modules,
                                                          double fraction) const;

  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;

  std::vector<edm::ParameterSet> killSpecs_;
  bool debug_;
  std::unique_ptr<CLHEP::HepJamesRandom> engine_;
};

SiPhase2ITFakeQualityESSource::SiPhase2ITFakeQualityESSource(const edm::ParameterSet& iConfig)
    : killSpecs_(iConfig.getParameter<std::vector<edm::ParameterSet>>("killSpecs")),
      debug_(iConfig.getUntrackedParameter<bool>("debug", false)),
      engine_(std::make_unique<CLHEP::HepJamesRandom>(iConfig.getParameter<unsigned int>("seed"))) {
  auto cc = setWhatProduced(this);
  geomToken_ = cc.consumes();

  for (auto const& spec : killSpecs_) {
    const std::string& moduleType = spec.getParameter<std::string>("moduleType");
    const double fraction = spec.getParameter<double>("fraction");
    if (fraction < 0. || fraction > 1.) {
      throw cms::Exception("Inconsistent configuration")
          << "killSpecs fraction must be between 0 and 1, but is " << fraction;
    }
    if (moduleType != "Ph2PXB" && moduleType != "Ph2PXF" && moduleType != "Ph2PXB3D") {
      throw cms::Exception("Inconsistent configuration")
          << "killSpecs moduleType must be one of Ph2PXB, Ph2PXF, Ph2PXB3D, but is " << moduleType;
    }
  }
  findingRecord<SiPhase2ITQualityRcd>();
  }

  void SiPhase2ITFakeQualityESSource::setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                                    const edm::IOVSyncValue& iov,
                                                    edm::ValidityInterval& oValidity) {
  oValidity = edm::ValidityInterval{iov.beginOfTime(), iov.endOfTime()};
}

std::vector<const GeomDet*> SiPhase2ITFakeQualityESSource::collectEligibleModules(const TrackerGeometry& tGeom,
                                                                                    const std::string& moduleType) const {
  std::vector<const GeomDet*> eligible;
  for (auto const& det : tGeom.dets()) {
    const StackGeomDet* stack = dynamic_cast<const StackGeomDet*>(det);
    if (stack) {
      continue;  // this is an OT 2S/PS module, not ours — skip it
    }
    const auto type = tGeom.getDetectorType(det->geographicalId().rawId());
    if ((moduleType == "Ph2PXB" && type == TrackerGeometry::ModuleType::Ph2PXB) ||
        (moduleType == "Ph2PXF" && type == TrackerGeometry::ModuleType::Ph2PXF)) {
      eligible.push_back(det);
    // Handle 3D modules seperatly, since they have two sensors and both get counted as a module. Only store the uneven detID  
    // and for the disabling of the module, disable the other sensor as well.
    } else if (moduleType == "Ph2PXB3D" && type == TrackerGeometry::ModuleType::Ph2PXB3D && det->geographicalId().rawId() % 2 == 1) {
      eligible.push_back(det);
    }
  }
  return eligible;
}

std::vector<const GeomDet*> SiPhase2ITFakeQualityESSource::selectRandomModules(const std::vector<const GeomDet*>& modules, double fraction) const {
  if (modules.empty()) {
    return {};
  }
  std::vector<const GeomDet*> shuffled = modules;
  for (size_t i = shuffled.size() - 1; i > 0; --i) {
    size_t j = static_cast<size_t>(CLHEP::RandFlat::shoot(engine_.get(), 0, i + 1));
    std::swap(shuffled[i], shuffled[j]);
  }
  const size_t numToSelect = static_cast<size_t>(std::floor(fraction * modules.size()));
  shuffled.resize(numToSelect);
  return shuffled;
}
SiPhase2ITFakeQualityESSource::ReturnType SiPhase2ITFakeQualityESSource::produce(const SiPhase2ITQualityRcd& iRecord) {
  const auto& geomRcd = iRecord.getRecord<TrackerDigiGeometryRecord>();
  const TrackerGeometry& tGeom = geomRcd.get(geomToken_);
  
  std::set<uint32_t> deadDetIds;  // dedupe in case killSpecs overlap on the same module

  for (auto const& spec : killSpecs_) {
    const std::string& moduleType = spec.getParameter<std::string>("moduleType");
    const double fraction = spec.getParameter<double>("fraction");

    const auto eligible = collectEligibleModules(tGeom, moduleType);
    const auto selected = selectRandomModules(eligible, fraction);

    if (debug_) {
      edm::LogPrint("SiPhase2ITFakeQualityESSource")
          << "moduleType " << moduleType << ": killing " << selected.size() << " / " << eligible.size()
          << " modules (fraction = " << fraction << ")";
    }
  
  for (const auto* module : selected) {
    const uint32_t detId = module->geographicalId().rawId();
    if (moduleType == "Ph2PXB" || moduleType == "Ph2PXF") {
      deadDetIds.insert(detId);
    } else if (moduleType == "Ph2PXB3D") {
      deadDetIds.insert(detId);
      deadDetIds.insert(detId + 1);  // also disable the other sensor in the 3D module
      }
    }
  }
  auto product = std::make_unique<SiPixelQuality>();
  for (uint32_t detId : deadDetIds) {
    SiPixelQuality::disabledModuleType badModule;
    badModule.DetID = detId;
    badModule.errorType = 0;  // "whole" module disabled
    badModule.BadRocs = 0;    // not implemented yet, only supports whole-module disabling
    product->addDisabledModule(badModule);
  }

  if (debug_) {
    edm::LogPrint("SiPhase2ITFakeQualityESSource") << "Total unique dead sensor DetIds: " << deadDetIds.size();
  }

  return product;
}


void SiPhase2ITFakeQualityESSource::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setComment(
      "Fake Phase-2 Inner Tracker module quality ESSource — kills a random fraction of modules per "
      "kill-spec, combined into one payload");

  edm::ParameterSetDescription killSpecDesc;
  killSpecDesc.add<std::string>("moduleType")->setComment("Ph2PXB, Ph2PXF, Ph2PXB3D");
  killSpecDesc.add<double>("fraction")->setComment("fraction of eligible modules for this type to kill, 0-1");
  desc.addVPSet("killSpecs", killSpecDesc);

  desc.add<unsigned int>("seed")->setComment("random seed, shared across all killSpecs entries");
  desc.addUntracked<bool>("debug", false)->setComment("enable debug printout");
  descriptions.add("SiPhase2ITFakeQualityESSource", desc);
}

DEFINE_FWK_EVENTSETUP_SOURCE(SiPhase2ITFakeQualityESSource);
