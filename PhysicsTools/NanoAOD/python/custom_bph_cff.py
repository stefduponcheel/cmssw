import FWCore.ParameterSet.Config as cms
from PhysicsTools.NanoAOD.nano_cff import *

##for gen and trigger muon
from PhysicsTools.BPHNano.pverticesBPH_cff import *
from PhysicsTools.BPHNano.genparticlesBPH_cff import *
#from PhysicsTools.BPHNano.particlelevelBPH_cff import *
from PhysicsTools.BPHNano.triggering_cff import *
from PhysicsTools.BPHNano.NewGenMatcher_cff import * #Have to import this
## BPH collections
from PhysicsTools.BPHNano.muons_cff import *
from PhysicsTools.BPHNano.MuMu_cff import *
from PhysicsTools.BPHNano.tracks_cff import *
from PhysicsTools.BPHNano.DiTrack_cff import *
from PhysicsTools.BPHNano.V0_cff import *
from PhysicsTools.BPHNano.BToKLL_cff import *
from PhysicsTools.BPHNano.BToTrkTrkLL_cff import *
from PhysicsTools.BPHNano.BToV0LL_cff import *
from PhysicsTools.BPHNano.BToV0TrkLL_cff import *
from PhysicsTools.BPHNano.pverticesBPH_cff import *
#for the merged gen particles:
from PhysicsTools.NanoAOD.particlelevel_cff import *
#PackedPF photons
from PhysicsTools.BPHNano.getPackedPFphotons_cff import *
def nanoAOD_customizeMC(process):
    process.load('PhysicsTools.BPHNano.particlelevelBPH_cff')
    process.load('PhysicsTools.BPHNano.genparticlesBPH_cff')
    process.nanoSequence = cms.Sequence(process.nanoSequence +process.particleLevelBPHSequence + process.genParticleBPHSequence+ process.genParticleBPHTables )
    return process



def nanoAOD_customizeMuonBPH(process):
    process.load('PhysicsTools.BPHNano.muons_cff')
    process.nanoSequence = cms.Sequence( process.nanoSequence + process.muonBPHSequence + process.muonBPHTables)
    return process


def nanoAOD_customizeDiMuonBPH(process):
    process.load('PhysicsTools.BPHNano.MuMu_cff')
    process.nanoSequence = cms.Sequence( process.nanoSequence + MuMuSequence + MuMuTables)
    return process



# def nanoAOD_customizeTrackBPH(process):
#     process.load('PhysicsTools.BPHNano.tracks_cff')    
#     process.nanoSequence = cms.Sequence( process.nanoSequence + tracksBPHSequence + tracksBPHTables)
#     return process


# def nanoAOD_customizeBToKLL(process):
#     process.load('PhysicsTools.BPHNano.BToKLL_cff')
#     process.nanoSequence = cms.Sequence( process.nanoSequence + BToKMuMuSequence + BToKMuMuTables)
#     return process



# def nanoAOD_customizeBToTrkTrkLL(process):
#     process.load('PhysicsTools.BPHNano.DiTrack_cff')    
#     process.load('PhysicsTools.BPHNano.BToTrkTrkLL_cff')    
#     process.nanoSequence = cms.Sequence( process.nanoSequence + DiTrackSequence + BToTrkTrkMuMuSequence + BToTrkTrkMuMuTables  )
#     return process


def nanoAOD_customizeBToKshortLL(process):
    process.load('PhysicsTools.BPHNano.V0_cff')
    process.load('PhysicsTools.BPHNano.BToV0LL_cff') 
    process.nanoSequenceMC = cms.Sequence( process.nanoSequence+ KshortToPiPiSequenceMC + KshortToPiPiTablesMC + BToKshortMuMuSequence + BToKshortMuMuTables  )
    process.nanoSequence = cms.Sequence( process.nanoSequence+ KshortToPiPiSequence + KshortToPiPiTables + BToKshortMuMuSequence + BToKshortMuMuTables  )
    return process

# def nanoAOD_customizeLambdabToLambdaLL(process):
#     process.load('PhysicsTools.BPHNano.V0_cff')
#     process.load('PhysicsTools.BPHNano.BToV0LL_cff')
#     process.nanoSequenceMC = cms.Sequence( process.nanoSequence+ LambdaToProtonPiSequenceMC + LambdaToProtonPiTablesMC + LambdabToLambdaMuMuSequence + LambdabToLambdaMuMuTables  )
#     process.nanoSequence = cms.Sequence( process.nanoSequence+ LambdaToProtonPiSequence + LambdaToProtonPiTables + LambdabToLambdaMuMuSequence + LambdabToLambdaMuMuTables  )
#     return process


# def nanoAOD_customizeBToChargedKstarLL(process):
#     process.load('PhysicsTools.BPHNano.V0_cff')
#     process.load('PhysicsTools.BPHNano.BToV0TrkLL_cff')
#     process.nanoSequenceMC = cms.Sequence( process.nanoSequence+ KshortToPiPiSequenceMC + BToChargedKstarMuMuSequence + KshortToPiPiTable + BToChargedKstarsMuMuTable)
#     process.nanoSequence = cms.Sequence( process.nanoSequence+ KshortToPiPiSequence + BToChargedKstarMuMuSequence + KshortToPiPiTable + BToChargedKstarsMuMuTable)
#     return process

# def nanoAOD_customizeXibToXiLL(process):
#     process.load('PhysicsTools.BPHNano.V0_cff')
#     process.load('PhysicsTools.BPHNano.BToV0TrkLL_cff')
#     process.nanoSequenceMC = cms.Sequence( process.nanoSequence+ LambdaToProtonPiSequenceMC + XibToXiMuMuSequence + LambdabToLambdaMuMuTables + XibToXiMuMuTable)
#     process.nanoSequence = cms.Sequence( process.nanoSequence+ LambdaToProtonPiSequence + XibToXiMuMuSequence + LambdabToLambdaMuMuTables + XibToXiMuMuTable)
#     return process




def nanoAOD_customizeBPH(process):
    process.load('PhysicsTools.BPHNano.genparticlesBPH_cff')
    process.load('PhysicsTools.BPHNano.muons_cff')
    process.load('PhysicsTools.BPHNano.MuMu_cff')
    process.load('PhysicsTools.BPHNano.tracks_cff')# For Kshort to PiPi to mumu, but don't store them
    # process.load('PhysicsTools.BPHNano.BToKLL_cff')
    process.load('PhysicsTools.BPHNano.DiTrack_cff')
    # process.load('PhysicsTools.BPHNano.BToTrkTrkLL_cff')
    process.load('PhysicsTools.BPHNano.V0_cff') # For Kshort to PiPi to mumu
    process.load('PhysicsTools.BPHNano.pverticesBPH_cff')
    process.load('PhysicsTools.BPHNano.triggering_cff')
    process.load('PhysicsTools.NanoAOD.particlelevel_cff')
    process.load('PhysicsTools.BPHNano.NewGenMatcher_cff')
    process.load('PhysicsTools.BPHNano.getPackedPFphotons_cff')
    # process.load('PhysicsTools.BPHNano.BToV0LL_cff')
    # process.load('PhysicsTools.BPHNano.V0_cff')
    # process.load('PhysicsTools.BPHNano.BToV0TrkLL_cff')
    # process.nanoSequenceMC = cms.Sequence(TriggerSequence + genParticleBPHSequence + genParticleBPHTables + muonBPHSequenceMC + muonBPHTablesMC+ MuMuSequence + MuMuTables + BPHPrimaryVerticesSequence + process.nanoSequenceMC )
    process.nanoSequenceMC = cms.Sequence(TriggerSequence + cms.Sequence(particleLevelTask) + muonBPHSequence + MuMuSequence + MuMuTables + getPackedPFphotonsSequence + packedPFphotonTable)# muonBPHSequence + MuMuSequence + MuMuTable + muonBPHTables + NewGenMatcherSequence + NewGenMatcherTables)
    
    # process.nanoSequenceMC = cms.Sequence(cms.Sequence(particleLevelTask) + genParticleBPHSequence +genParticleBPHTables + muonBPHSequenceMC +muonBPHTablesMC+  MuMuSequence + MuMuTables + MuMuAncestorSequence + MuMuAncestorTables)  #Need to do trigger sequence
                                         #tracksBPHSequenceMC + tracksBPHTablesMC + BToKMuMuSequence + BToKMuMuTables + DiTrackSequence + BToTrkTrkMuMuSequence + BToTrkTrkMuMuTables + KshortToPiPiSequenceMC + KshortToPiPiTablesMC + BToKshortMuMuSequence + BToKshortMuMuTables +  LambdaToProtonPiSequenceMC + LambdaToProtonPiTablesMC + LambdabToLambdaMuMuSequence + LambdabToLambdaMuMuTables + BToChargedKstarMuMuSequence + BToChargedKstarsMuMuTable + XibToXiMuMuSequence + XibToXiMuMuTable)

    #process.nanoSequence = cms.Sequence(process.nanoSequence + muonBPHSequence + muonBPHTables + MuMuSequence + MuMuTables + tracksBPHSequence +  KshortToPiPiSequence + KshortToPiPiTables + BPHPrimaryVerticesSequence)
                                         #tracksBPHSequence + tracksBPHTables + BToKMuMuSequence + BToKMuMuTables + DiTrackSequence + BToTrkTrkMuMuSequence + BToTrkTrkMuMuTables + KshortToPiPiSequence + KshortToPiPiTables + BToKshortMuMuSequence + BToKshortMuMuTables +  LambdaToProtonPiSequence + LambdaToProtonPiTables + LambdabToLambdaMuMuSequence + LambdabToLambdaMuMuTables+BToChargedKstarMuMuSequence+BToChargedKstarsMuMuTable + XibToXiMuMuSequence + XibToXiMuMuTable
                                        
    return process


