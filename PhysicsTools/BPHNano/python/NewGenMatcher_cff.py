import FWCore.ParameterSet.Config as cms
from PhysicsTools.NanoAOD.common_cff import Var

NewGenMatcher = cms.EDProducer(
    "NewGenMatcher",
    src_muon = cms.InputTag("muonBPH:SelectedMuons"),
    src_MuMu = cms.InputTag("MuMu:SelectedDiLeptons"),
    src_gen  = cms.InputTag("mergedGenParticles"),

    maxDeltaR = cms.double(0.05),
    maxRelPt  = cms.double(0.5),
    debug     = cms.bool(True),
)

MuMuAncestorsTable = cms.EDProducer("SimpleCompositeCandidateFlatTableProducer",
    src  = cms.InputTag("NewGenMatcher", "SelectedMuMuExtended"),
    name = cms.string("MuMuOrigin"),
    doc  = cms.string("GEN-level origin classification for dimuons"),

    variables = cms.PSet(

        matchedToGenDecay = Var("userInt('matchedToGenDecay')", bool),
        unmatchedButGenExists = Var("userInt('unmatchedButGenExists')", bool),
        genDecayExists = Var("userInt('genDecayExists')", bool),

        fromEta = Var("userInt('fromEta')", bool),
        fromEta_MuMu = Var("userInt('fromEta_MuMu')", bool),
        fromEta_MuMuGamma = Var("userInt('fromEta_MuMuGamma')", bool),

        fromOmega = Var("userInt('fromOmega')", bool),
        fromOmega_MuMu = Var("userInt('fromOmega_MuMu')", bool),
        fromOmega_MuMuPi0 = Var("userInt('fromOmega_MuMuPi0')", bool),

        fromPhi = Var("userInt('fromPhi')", bool),
        fromKK = Var("userInt('from_KK')", bool),

        fromEtaPrime = Var("userInt('fromEtaPrime')", bool),
        fromEtaPrime_MuMuGamma = Var("userInt('fromEtaPrime_MuMuGamma')", bool),

        fromRho = Var("userInt('fromRho')", bool),

        etaPhoton_pt  = Var("userFloat('etaPhoton_pt')",  float),
        etaPhoton_eta = Var("userFloat('etaPhoton_eta')", float),
        etaPhoton_phi = Var("userFloat('etaPhoton_phi')", float),
        etaPhoton_DeltaR = Var("userFloat('etaPhoton_DeltaR')", float),

        # Combinatorial studies:
        pair_bothGenMatched = Var("userInt('pair_bothGenMatched')", bool),
        pair_oneGenMatched = Var("userInt('pair_oneGenMatched')", bool),
        pair_noGenMatched = Var("userInt('pair_noGenMatched')", bool),
        matchedButOther = Var("userInt('matchedButOther')", bool),

        sameOrigin = Var("userInt('sameOrigin')", bool),
        trueCombinatorial = Var("userInt('trueCombinatorial')", bool),
        originValid = Var("userInt('originValid')", bool),

        origin_1_pgdId =  Var("userInt('origin1_pdgId')", int),
        origin_2_pgdId =  Var("userInt('origin2_pdgId')", int),

    )
)

NewGenMatcherSequence = cms.Sequence(NewGenMatcher)
NewGenMatcherTables = cms.Sequence(MuMuAncestorsTable)
