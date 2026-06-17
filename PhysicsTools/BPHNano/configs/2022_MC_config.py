# Configuration file for 2022 data:
from FWCore.ParameterSet.VarParsing import VarParsing
import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Run3_cff import Run3
from Configuration.AlCa.GlobalTag import GlobalTag
from PhysicsTools.PatAlgos.tools.helpers import associatePatAlgosToolsTask
from PhysicsTools.NanoAOD.nano_cff import nanoAOD_customizeCommon 
from PhysicsTools.NanoAOD.custom_bph_cff import nanoAOD_customizeBPH 
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete

options = VarParsing('python')

options.register('globalTag', '126X_mcRun3_2022_realistic_v2', 
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Global tag"
)

options.register('isMC', True,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Adds gen info/matching"
)


options.register('wantSummary', True,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Processing summary"
)

options.register('wantFullRECO', False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Produces additional EDM file"
    )

options.register('reportEvery', 1,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.int,
    "Report every N events"
)

options.register('skip', 0,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.int,
    "Skip first N events"
)

options.register('decay', 'all',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Options: all KLL KshortLL TrkTrkLL"
)
options.parseArguments()

if options.isMC:
   options.tag+="_mc"
else:
   options.tag+="_data"

options.tag+='_'
options.tag+=options.decay

outputFileNANO = cms.untracked.string('MC.root') # 10/10/2025: Simplified output name
annotation = '%s nevts:%d' % (outputFileNANO, options.maxEvents)
#Import the process
process = cms.Process('NANO',Run3)
# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('PhysicsTools.NanoAOD.nano_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load("TrackingTools/TransientTrack/TransientTrackBuilder_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = options.reportEvery
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
)
# Input source
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(options.inputFiles),
    secondaryFileNames = cms.untracked.vstring(),
    skipEvents=cms.untracked.uint32(options.skip),
)

process.options = cms.untracked.PSet(
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
    wantSummary = cms.untracked.bool(options.wantSummary),
)

process.nanoMetadata.strings.tag = annotation
# Production Info
process.configurationMetadata = cms.untracked.PSet(
    annotation = cms.untracked.string(annotation),
    name = cms.untracked.string('Applications'),
    version = cms.untracked.string('$Revision: 1.19 $')
)
# Output definition
process.NANOAODSIMoutput = cms.OutputModule("NanoAODOutputModule",
    compressionAlgorithm = cms.untracked.string('LZMA'),
    compressionLevel = cms.untracked.int32(9),
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('NANOAOD'),
        filterName = cms.untracked.string('')
    ),
    fileName = outputFileNANO,
    SelectEvents = cms.untracked.PSet(SelectEvents = cms.vstring('nanoAOD_step'))
)
process.NANOAODSIMoutput.outputCommands = cms.untracked.vstring(
'drop *',
'keep *_TrgMatchMuonTable_*_*',
'keep *_MuMuTable_*_*',
'keep *_MuMuAncestorsTable_*_*',
'keep *_packedPFphotonTables_*_*',
'keep *_genWeightsTable_*_*',
'keep *_genTable_*_*',
)
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '126X_mcRun3_2022_realistic_v2', '')

# Path and EndPath definitions
process.nanoAOD_step = cms.Path(process.nanoSequenceMC)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.NANOAODSIMoutput_step = cms.EndPath(process.NANOAODSIMoutput)

# Schedule definition
process.schedule = cms.Schedule(process.nanoAOD_step,process.endjob_step,process.NANOAODSIMoutput_step)
associatePatAlgosToolsTask(process)
from PhysicsTools.NanoAOD.nano_cff import nanoAOD_customizeCommon 

#Customisation of the process

# Automatic addition of the customisation function from PhysicsTools.NanoAOD.nano_cff
process = nanoAOD_customizeCommon(process)
# Automatic addition of the customisation function from PhysicsTools.NanoAOD.custom_bph_cff
process = nanoAOD_customizeBPH(process)

process.add_(cms.Service('InitRootHandlers', EnableIMT = cms.untracked.bool(False)))
process.NANOAODSIMoutput.fakeNameForCrab=cms.untracked.bool(True)
# Add early deletion of temporary data products to reduce peak memory need
process = customiseEarlyDelete(process)
# End adding early deletion
