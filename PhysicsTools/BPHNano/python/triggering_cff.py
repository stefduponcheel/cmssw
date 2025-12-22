import FWCore.ParameterSet.Config as cms
from HLTrigger.HLTfilters.hltHighLevel_cfi import hltHighLevel

# Trigger filter: require DoubleMu4_3_LowMass
triggerSelection = hltHighLevel.clone(
    HLTPaths = ["HLT_DoubleMu4_3_LowMass*"],
    andOr = True,          # OR between paths
    throw = False          # do not crash if trigger is missing
)
TriggerSequence = cms.Sequence(triggerSelection)
