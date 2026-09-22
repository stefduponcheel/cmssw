// -*- C++ -*-
//
// Package:    CalibTracker/SiPhase2TrackerESProducers
// Class:      SiPhase2OTFakeQualityESSource
//
/**\class SiPhase2OTFakeQualityESSource

 Description: "fake" SiPixelQuality ESProducer for Phase-2 Outer Tracker module killing —
 configurable random selection of dead 2S / PS-p / PS-s / whole-PS modules, selected at
 the physical module (stack) level. Supports killing several module types at once
 (e.g. 30% of 2S and 30% of PS-p together) via a list of kill-specs, merged into one
 combined output payload.

 Note: SiPixelQuality is reused here as a generic "list of dead module DetIds," the same
 way its own Inner Tracker pixel digitizer already uses it — not because 2S/PS-s are
 pixel-type sensors. They aren't. This keeps one payload format shared by all Outer
 Tracker sensor types instead of a strip-flavored one and a pixel-flavored one that could
 drift out of sync with each other. The output is published under a distinct
 "phase2OT"-style label specifically so it's never confused with real Inner Tracker pixel
 quality info living under the same record type.
*/

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

#include "CondFormats/DataRecord/interface/SiPhase2OuterTrackerCondDataRecords.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"

#include "Geometry/CommonTopologies/interface/StackGeomDet.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandFlat.h"

class SiPhase2OTFakeQualityESSource : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder {
public:
  SiPhase2OTFakeQualityESSource(const edm::ParameterSet&);
  ~SiPhase2OTFakeQualityESSource() override = default;

  using ReturnType = std::unique_ptr<SiPixelQuality>;
  ReturnType produce(const Phase2OTQualityRcd&);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

protected:
  void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                       const edm::IOVSyncValue&,
                       edm::ValidityInterval&) override;

private:
  // Collects the physical module "stacks" (2S or PS modules) eligible for a given
  // moduleType ("Ph2SS" -> 2S modules, "Ph2PSP"/"Ph2PSS"/"Ph2PS" -> PS modules).
  std::vector<const StackGeomDet*> collectEligibleStacks(const TrackerGeometry& tGeom,
                                                          const std::string& moduleType) const;

  // Fisher-Yates shuffle + truncate to the requested fraction. Uses engine_ directly
  // (std::shuffle doesn't work with the CLHEP engine, same reasoning as the old code).
  std::vector<const StackGeomDet*> selectRandomStacks(const std::vector<const StackGeomDet*>& stacks,
                                                       double fraction) const;

  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;

  std::vector<edm::ParameterSet> killSpecs_;
  bool debug_;
  std::unique_ptr<CLHEP::HepJamesRandom> engine_;
};

SiPhase2OTFakeQualityESSource::SiPhase2OTFakeQualityESSource(const edm::ParameterSet& iConfig)
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
    if (moduleType != "Ph2SS" && moduleType != "Ph2PSP" && moduleType != "Ph2PSS" && moduleType != "Ph2PS") {
      throw cms::Exception("Inconsistent configuration")
          << "killSpecs moduleType must be one of Ph2SS, Ph2PSP, Ph2PSS, Ph2PS, but is " << moduleType;
    }
  }

  findingRecord<Phase2OTQualityRcd>();
}

void SiPhase2OTFakeQualityESSource::setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                                    const edm::IOVSyncValue& iov,
                                                    edm::ValidityInterval& oValidity) {
  oValidity = edm::ValidityInterval{iov.beginOfTime(), iov.endOfTime()};
}

std::vector<const StackGeomDet*> SiPhase2OTFakeQualityESSource::collectEligibleStacks(
    const TrackerGeometry& tGeom, const std::string& moduleType) const {
  std::vector<const StackGeomDet*> eligible;
  for (auto const& det : tGeom.dets()) {
    const auto* stack = dynamic_cast<const StackGeomDet*>(det);
    if (!stack) {
      continue;  // not a 2S/PS module stack (could be a leaf sensor, or another structure entirely)
    }

    const auto lowerType = tGeom.getDetectorType(stack->lowerDet()->geographicalId().rawId());
    const auto upperType = tGeom.getDetectorType(stack->upperDet()->geographicalId().rawId());

    const bool isTwoS =
        (lowerType == TrackerGeometry::ModuleType::Ph2SS && upperType == TrackerGeometry::ModuleType::Ph2SS);
    // Confirmed empirically (not assumed): for PS modules, lowerDet() is the Ph2PSP macro-pixel
    // sensor and upperDet() is the Ph2PSS strip sensor — the reverse of what the naming might
    // suggest.
    const bool isPS =
        (lowerType == TrackerGeometry::ModuleType::Ph2PSP && upperType == TrackerGeometry::ModuleType::Ph2PSS);

    if (moduleType == "Ph2SS" && isTwoS) {
      eligible.push_back(stack);
    } else if ((moduleType == "Ph2PSP" || moduleType == "Ph2PSS" || moduleType == "Ph2PS") && isPS) {
      eligible.push_back(stack);
    }
  }
  return eligible;
}

std::vector<const StackGeomDet*> SiPhase2OTFakeQualityESSource::selectRandomStacks(
    const std::vector<const StackGeomDet*>& stacks, double fraction) const {
  if (stacks.empty()) {
    return {};
  }
  std::vector<const StackGeomDet*> shuffled = stacks;
  for (size_t i = shuffled.size() - 1; i > 0; --i) {
    size_t j = static_cast<size_t>(CLHEP::RandFlat::shoot(engine_.get(), 0, i + 1));
    std::swap(shuffled[i], shuffled[j]);
  }
  const size_t numToSelect = static_cast<size_t>(std::floor(fraction * stacks.size()));
  shuffled.resize(numToSelect);
  return shuffled;
}

SiPhase2OTFakeQualityESSource::ReturnType SiPhase2OTFakeQualityESSource::produce(const Phase2OTQualityRcd& iRecord) {
  const auto& geomRcd = iRecord.getRecord<TrackerDigiGeometryRecord>();
  const TrackerGeometry& tGeom = geomRcd.get(geomToken_);

  std::set<uint32_t> deadDetIds;  // dedupe in case killSpecs overlap on the same module

  for (auto const& spec : killSpecs_) {
    const std::string& moduleType = spec.getParameter<std::string>("moduleType");
    const double fraction = spec.getParameter<double>("fraction");

    const auto eligible = collectEligibleStacks(tGeom, moduleType);
    const auto selected = selectRandomStacks(eligible, fraction);

    if (debug_) {
      edm::LogPrint("SiPhase2OTFakeQualityESSource")
          << "moduleType " << moduleType << ": killing " << selected.size() << " / " << eligible.size()
          << " modules (fraction = " << fraction << ")";
    }

    for (const auto* stack : selected) {
      const uint32_t lowerId = stack->lowerDet()->geographicalId().rawId();
      const uint32_t upperId = stack->upperDet()->geographicalId().rawId();
      if (moduleType == "Ph2SS" || moduleType == "Ph2PS") {
        // whole module: both sensors, whatever their type
        deadDetIds.insert(lowerId);
        deadDetIds.insert(upperId);
      } else if (moduleType == "Ph2PSP") {
        deadDetIds.insert(lowerId);  // lowerDet() is the PSP macro-pixel sensor
      } else if (moduleType == "Ph2PSS") {
        deadDetIds.insert(upperId);  // upperDet() is the PSS strip sensor
      }
    }
  }

  auto product = std::make_unique<SiPixelQuality>();
  for (uint32_t detId : deadDetIds) {
    SiPixelQuality::disabledModuleType badModule;
    badModule.DetID = detId;
    badModule.errorType = 0;  // "whole" module disabled
    badModule.BadRocs = 0;    // not applicable — Outer Tracker modules have no ROCs
    product->addDisabledModule(badModule);
  }

  if (debug_) {
    edm::LogPrint("SiPhase2OTFakeQualityESSource") << "Total unique dead sensor DetIds: " << deadDetIds.size();
  }

  return product;
}

void SiPhase2OTFakeQualityESSource::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setComment(
      "Fake Phase-2 Outer Tracker module quality ESSource — kills a random fraction of modules per "
      "kill-spec, combined into one payload");

  edm::ParameterSetDescription killSpecDesc;
  killSpecDesc.add<std::string>("moduleType")->setComment("Ph2SS, Ph2PSP, Ph2PSS, or Ph2PS");
  killSpecDesc.add<double>("fraction")->setComment("fraction of eligible modules for this type to kill, 0-1");
  desc.addVPSet("killSpecs", killSpecDesc);

  desc.add<unsigned int>("seed")->setComment("random seed, shared across all killSpecs entries");
  desc.addUntracked<bool>("debug", false)->setComment("enable debug printout");
  descriptions.add("SiPhase2OTFakeQualityESSource", desc);
}

DEFINE_FWK_EVENTSETUP_SOURCE(SiPhase2OTFakeQualityESSource);
