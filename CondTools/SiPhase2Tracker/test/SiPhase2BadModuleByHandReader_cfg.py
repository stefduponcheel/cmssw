import FWCore.ParameterSet.Config as cms

process = cms.Process("READ")

process.load('Configuration.Geometry.GeometryExtendedRun4D121Reco_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T35', '')

process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.enable = False
process.MessageLogger.SiPhase2BadModuleByHandReader = dict()
process.MessageLogger.cout = cms.untracked.PSet(
    enable = cms.untracked.bool(True),
    enableStatistics = cms.untracked.bool(True),
    threshold = cms.untracked.string("INFO"),
    default = cms.untracked.PSet(limit = cms.untracked.int32(0)),
    FwkReport = cms.untracked.PSet(limit = cms.untracked.int32(-1), reportEvery = cms.untracked.int32(1000)),
    SiPhase2BadModuleByHandReader = cms.untracked.PSet(limit = cms.untracked.int32(-1)),
)

process.source = cms.Source("EmptyIOVSource",
    timetype = cms.string('runnumber'),
    firstValue = cms.uint64(1),
    lastValue = cms.uint64(1),
    interval = cms.uint64(1)
)

# Reads back the sqlite file written by SiPhase2BadModuleByHandBuilder_cfg.py --
# run that one first (see test_CondToolsSiPhase2Tracker.sh for the ordering).
process.load("CondCore.CondDB.CondDB_cfi")
process.CondDB.connect = 'sqlite_file:BadModulesByHand_v0.db'

process.PoolDBESSource = cms.ESSource("PoolDBESSource",
    process.CondDB,
    toGet = cms.VPSet(
        cms.PSet(record = cms.string('Phase2OTQualityRcd'), tag = cms.string('Phase2OTBadModulesByHand_v0')),
        cms.PSet(record = cms.string('SiPhase2ITQualityRcd'), tag = cms.string('Phase2ITBadModulesByHand_v0')),
    )
)

process.get = cms.EDAnalyzer("EventSetupRecordDataGetter",
    toGet = cms.VPSet(
        cms.PSet(record = cms.string('Phase2OTQualityRcd'), data = cms.vstring('SiPixelQuality')),
        cms.PSet(record = cms.string('SiPhase2ITQualityRcd'), data = cms.vstring('SiPixelQuality')),
    ),
    verbose = cms.untracked.bool(True)
)

process.otReader = cms.EDAnalyzer("SiPhase2OTBadModuleReader", printDebug = cms.untracked.bool(True))
process.itReader = cms.EDAnalyzer("SiPhase2ITBadModuleReader", printDebug = cms.untracked.bool(True))

process.p = cms.Path(process.get + process.otReader + process.itReader)
