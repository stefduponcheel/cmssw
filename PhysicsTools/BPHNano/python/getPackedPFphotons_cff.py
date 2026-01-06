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
    variables = cms.PSet(
        pt = Var("pt", float),
        eta = Var("eta", float),
        phi = Var("phi", float),
    ),
    externalVariables = cms.PSet(PhotonPfIso03 = cms.PSet(
            src  = cms.InputTag("getPackedPFphotons", "PhotonPfIso03"),
            type = cms.string("double"), 
            doc  = cms.string("PF isolation (dR<0.3)/pt"),
        ),
        PhotonDr = cms.PSet(
            src  = cms.InputTag("getPackedPFphotons", "PhotonDr"),
            type = cms.string("double"),  
            doc  = cms.string("dR between photon and dimuon"),
        ),
  )
)

getPackedPFphotonsSequence = cms.Sequence(getPackedPFphotons)
packedPFphotonTable = cms.Sequence(packedPFphotonTables)
