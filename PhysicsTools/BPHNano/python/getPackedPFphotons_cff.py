import FWCore.ParameterSet.Config as cms
from PhysicsTools.NanoAOD.common_cff import *
getPackedPFphotons = cms.EDProducer(
    "getPackedPFphotons",
    src_pfCands = cms.InputTag("packedPFCandidates"),
    src_mumu = cms.InputTag("MuMu:SelectedDiLeptons"),

)
packedPFphotonTables = cms.EDProducer(
    "SimplePATCandidateFlatTableProducer",
    src  = cms.InputTag("getPackedPFphotons", "PackedPFphotons"),
    cut  = cms.string(""),  # keep all selected ones
    name = cms.string("PFPhoton"),   # branch prefix: PFPhoton_*
    doc  = cms.string("PF photons near dimuon (from packedPFCandidates)"),
    singleton = cms.bool(False),
    extension = cms.bool(False),
    variables = cms.PSet(pt  = cms.PSet(expr = cms.string("pt()"),  type = cms.string("float"),  doc = cms.string("pT")),

    )
    
)
getPackedPFphotonsSequence = cms.Sequence(getPackedPFphotons)
packedPFphotonTable = cms.Sequence(packedPFphotonTables)
