#include <iostream>
#include <cmath>

#include "SimTracker/SiPhase2Digitizer/plugins/PSSDigitizerAlgorithm.h"

#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// Geometry
#include "Geometry/CommonTopologies/interface/PixelGeomDetUnit.h"

#include "CondFormats/DataRecord/interface/SiPhase2OuterTrackerCondDataRecords.h"

using namespace edm;

void PSSDigitizerAlgorithm::init(const edm::EventSetup& es) {
  if (use_LorentzAngle_DB_)  // Get Lorentz angle from DB record
    siPhase2OTLorentzAngle_ = &es.getData(siPhase2OTLorentzAngleToken_);

  if (use_deadmodule_DB_)  // Get Bad Channel (SiStripBadStrip) from DB
    badChannelPayload_ = &es.getData(badChannelToken_);

  geom_ = &es.getData(geomToken_);
}
PSSDigitizerAlgorithm::PSSDigitizerAlgorithm(const edm::ParameterSet& conf, edm::ConsumesCollector iC)
    : Phase2TrackerDigitizerAlgorithm(conf.getParameter<ParameterSet>("AlgorithmCommon"),
                                      conf.getParameter<ParameterSet>("PSSDigitizerAlgorithm"),
                                      iC),
      geomToken_(iC.esConsumes()) {
  if (use_LorentzAngle_DB_)
    siPhase2OTLorentzAngleToken_ = iC.esConsumes();

  if (use_deadmodule_DB_) {
    badChannelToken_ = iC.esConsumes();
  }

  pixelFlag_ = false;
  LogDebug("PSSDigitizerAlgorithm") << "Algorithm constructed "
                                    << "Configuration parameters: "
                                    << "Threshold/Gain = "
                                    << "threshold in electron Endcap = " << theThresholdInE_Endcap_
                                    << "threshold in electron Barrel = " << theThresholdInE_Barrel_ << " "
                                    << theElectronPerADC_ << " " << theAdcFullScale_ << " The delta cut-off is set to "
                                    << tMax_ << " pix-inefficiency " << addPixelInefficiency_;
}
PSSDigitizerAlgorithm::~PSSDigitizerAlgorithm() { LogDebug("PSSDigitizerAlgorithm") << "Algorithm deleted"; }
//
// -- Select the Hit for Digitization (sigScale will be implemented in future)
//
bool PSSDigitizerAlgorithm::select_hit(const PSimHit& hit, double tCorr, double& sigScale) const {
  double toa = hit.tof() - tCorr;
  return (toa > theTofLowerCut_ && toa < theTofUpperCut_);
}
//
// -- Compare Signal with Threshold
//
bool PSSDigitizerAlgorithm::isAboveThreshold(const digitizerUtility::SimHitInfo* const hisInfo,
                                             float charge,
                                             float thr) const {
  return (charge >= thr);
}
//
// -- Read module status from the Condition DB and kill the whole module's signal if dead
//
void PSSDigitizerAlgorithm::module_killing_DB(const Phase2TrackerGeomDetUnit* pixdet) {
  uint32_t detId = pixdet->geographicalId().rawId();

  if (!badChannelPayload_->IsModuleUsable(detId)) {
    signal_map_type& theSignal = _signal[detId];
    for (auto& s : theSignal) {
      s.second.set(0.);
    }
  }
}
