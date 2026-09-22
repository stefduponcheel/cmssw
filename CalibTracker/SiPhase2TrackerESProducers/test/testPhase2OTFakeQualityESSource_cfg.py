import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C22I13M9_cff import Phase2C22I13M9

process = cms.Process("TEST", Phase2C22I13M9)

process.load('Configuration.Geometry.GeometryExtendedRun4D121Reco_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T35', '')

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))

process.SiPhase2OTFakeQualityESSource = cms.ESSource("SiPhase2OTFakeQualityESSource",
    killSpecs = cms.VPSet(
        cms.PSet(moduleType = cms.string("Ph2SS"),  fraction = cms.double(0.3)),
        cms.PSet(moduleType = cms.string("Ph2PSP"), fraction = cms.double(0.3)),
        cms.PSet(moduleType = cms.string("Ph2PSS"), fraction = cms.double(0.3)),
        cms.PSet(moduleType = cms.string("Ph2PS"),  fraction = cms.double(0.3)),
    ),
    seed = cms.uint32(12345),
    debug = cms.untracked.bool(True),
)

process.testAnalyzer = cms.EDAnalyzer("TestPhase2OTQualityAnalyzer")
process.p = cms.Path(process.testAnalyzer)
