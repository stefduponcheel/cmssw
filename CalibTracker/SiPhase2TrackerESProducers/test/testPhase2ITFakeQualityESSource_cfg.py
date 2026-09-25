import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C22I13M9_cff import Phase2C22I13M9

process = cms.Process("TEST", Phase2C22I13M9)

process.load('Configuration.Geometry.GeometryExtendedRun4D121Reco_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T35', '')

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))


process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')
process.MessageLogger.cerr.default = cms.untracked.PSet(limit = cms.untracked.int32(0))
process.MessageLogger.cerr.FwkReport = cms.untracked.PSet(limit = cms.untracked.int32(0))
process.MessageLogger.cerr.SiPhase2ITFakeQualityESSource = cms.untracked.PSet(limit = cms.untracked.int32(-1))

process.SiPhase2ITFakeQualityESSource = cms.ESSource("SiPhase2ITFakeQualityESSource",
    killSpecs = cms.VPSet(
        cms.PSet(moduleType = cms.string("Ph2PXB"),  fraction = cms.double(0.3)),
        cms.PSet(moduleType = cms.string("Ph2PXF"), fraction = cms.double(0.3)),
        cms.PSet(moduleType = cms.string("Ph2PXB3D"), fraction = cms.double(0.3)),
    ),
    seed = cms.uint32(12345),
    debug = cms.untracked.bool(True),
)
process.get = cms.EDAnalyzer("EventSetupRecordDataGetter",
    toGet = cms.VPSet(
        cms.PSet(record = cms.string("SiPhase2ITQualityRcd"), data = cms.vstring("SiPixelQuality/"))
    ),
    verbose = cms.untracked.bool(True)
)
process.p = cms.Path(process.get)
