import FWCore.ParameterSet.Config as cms

# The Outer Tracker Lorentz Angle are now taken from Global Tag

##
## Fake Sim Outer Tracker Lorentz Angle
##

# SiPhase2OTFakeLorentzAngleESSource = cms.ESSource('SiPhase2OuterTrackerFakeLorentzAngleESSource',
#                                                   LAValue = cms.double(0.07),
#                                                   recordName = cms.string("LorentzAngle"))

# es_prefer_fake_LA = cms.ESPrefer("SiPhase2OuterTrackerFakeLorentzAngleESSource","SiPhase2OTFakeLorentzAngleESSource")

##
## Fake Sim Outer Tracker Lorentz Angle
##

# SiPhase2OTFakeSimLorentzAngleESSource = cms.ESSource('SiPhase2OuterTrackerFakeLorentzAngleESSource',
#                                                      LAValue = cms.double(0.07),
#                                                      recordName = cms.string("SimLorentzAngle"))

# es_prefer_fake_simLA = cms.ESPrefer("SiPhase2OuterTrackerFakeLorentzAngleESSource","SiPhase2OTFakeSimLorentzAngleESSource")

##
## Fake Sim Outer Tracker Lorentz Angle
##

# from CalibTracker.SiPhase2TrackerESProducers.siPhase2BadStripConfigurableFakeESSource_cfi import siPhase2BadStripConfigurableFakeESSource
# SiPhase2OTFakeBadStripsESSource = siPhase2BadStripConfigurableFakeESSource.clone(seed = 1,
#                                                                                  printDebug = False,
#                                                                                  badComponentsFraction = 0.7, # Change the fraction to 70%
#                                                                                  appendToDataLabel = '')

# es_prefer_fake_BadStrips = cms.ESPrefer("SiPhase2BadStripConfigurableFakeESSource","SiPhase2OTFakeBadStripsESSource")

# SiPhase2OTFakeBadModulesESSource = cms.ESSource("SiPhase2BadModuleConfigurableFakeESSource")

SiPhase2RandomModuleKillingConfigurableFakeESSource = cms.ESSource(
    "SiPhase2RandomModuleKillingConfigurableFakeESSource",
    seed = cms.uint32(12345),
    badModulesFraction = cms.double(0.05),
    debug = cms.untracked.bool(True)
)
