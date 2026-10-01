import os
import FWCore.ParameterSet.Config as cms

process = cms.Process("WRITE")

process.load('Configuration.Geometry.GeometryExtendedRun4D121Reco_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T35', '')

process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.enable = False
process.MessageLogger.SiPhase2BadModuleByHandBuilder = dict()
process.MessageLogger.cout = cms.untracked.PSet(
    enable = cms.untracked.bool(True),
    enableStatistics = cms.untracked.bool(True),
    threshold = cms.untracked.string("INFO"),
    default = cms.untracked.PSet(limit = cms.untracked.int32(0)),
    FwkReport = cms.untracked.PSet(limit = cms.untracked.int32(-1), reportEvery = cms.untracked.int32(1000)),
    SiPhase2BadModuleByHandBuilder = cms.untracked.PSet(limit = cms.untracked.int32(-1)),
)

process.source = cms.Source("EmptyIOVSource",
    timetype = cms.string('runnumber'),
    firstValue = cms.uint64(1),
    lastValue = cms.uint64(1),
    interval = cms.uint64(1)
)

process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    DBParameters = cms.PSet(authenticationPath = cms.untracked.string('')),
    connect = cms.string('sqlite_file:BadModulesByHand_v0.db'),
    toPut = cms.VPSet(
        cms.PSet(record = cms.string('Phase2OTQualityRcd'), tag = cms.string('Phase2OTBadModulesByHand_v0')),
        cms.PSet(record = cms.string('SiPhase2ITQualityRcd'), tag = cms.string('Phase2ITBadModulesByHand_v0')),
    )
)

# Example input file lives in CondTools/SiPhase2Tracker/data/, same place
# DTCCablingMapProducer's own example CSV lives -- resolved via CMSSW_BASE so
# this cfg works regardless of the working directory it's run from (standard
# CMSSW unit-test convention: cmsRun is invoked with a full path to this cfg,
# but writes its own output into the current directory).
_exampleListFile = os.path.join(os.environ['CMSSW_BASE'], 'src/CondTools/SiPhase2Tracker/data/dead_modules_example.txt')

process.otBadModuleBuilder = cms.EDAnalyzer("SiPhase2BadModuleByHandBuilder",
    Record = cms.string('Phase2OTQualityRcd'),
    SinceAppendMode = cms.bool(True),
    IOVMode = cms.string('Run'),
    doStoreOnDB = cms.bool(True),
    badModuleListFile = cms.untracked.string(_exampleListFile),
    targetRecord = cms.untracked.string("Phase2OTQualityRcd"),
    printDebug = cms.untracked.bool(True),
)

process.itBadModuleBuilder = cms.EDAnalyzer("SiPhase2BadModuleByHandBuilder",
    Record = cms.string('SiPhase2ITQualityRcd'),
    SinceAppendMode = cms.bool(True),
    IOVMode = cms.string('Run'),
    doStoreOnDB = cms.bool(True),
    badModuleListFile = cms.untracked.string(_exampleListFile),
    targetRecord = cms.untracked.string("SiPhase2ITQualityRcd"),
    printDebug = cms.untracked.bool(True),
)

process.p = cms.Path(process.otBadModuleBuilder + process.itBadModuleBuilder)
