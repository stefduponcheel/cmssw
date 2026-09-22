import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C22I13M9_cff import Phase2C22I13M9
import FWCore.ParameterSet.VarParsing as VarParsing

# Minimal harvesting step for digi-only DQM output: just saves the MonitorElements that
# were already filled at the DIGI step (via addTrackerDQM in killScenarios_customize.py)
# as a proper DQM_*.root file, with no additional "client" post-processing.
#
# The standard -s HARVESTING:@standardDQM sequence is NOT usable here: it expects
# RECO-level histograms (tracking, muon, HLT, ...) that a digi-only job never produces,
# and crashes trying to process them. The actual Phase-2-tracker-specific harvesting
# sequence (DQM.SiTrackerPhase2.Phase2TrackerDQMHarvesting_cff) is an empty
# cms.Sequence() in this release, confirming there's nothing to "harvest" for these
# histograms beyond saving them -- matching the official reference config,
# DQM/SiTrackerPhase2/test/harvestingstep_phase2tk_cfg.py, whose own schedule is also
# just DQMSaver alone.
#
# Usage: cmsRun harvesting_minimal_cfg.py inputFiles=file:<...>_inDQM.root

options = VarParsing.VarParsing('analysis')
options.parseArguments()

process = cms.Process('HARVESTING', Phase2C22I13M9)

process.load('Configuration.StandardSequences.Services_cff')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.Geometry.GeometryExtendedRun4D121Reco_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.DQMSaverAtRunEnd_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))

process.source = cms.Source("DQMRootSource",
    fileNames = cms.untracked.vstring(options.inputFiles),
)

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T35', '')

process.dqmsave_step = cms.Path(process.DQMSaver)
process.schedule = cms.Schedule(process.dqmsave_step)
