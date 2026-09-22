import FWCore.ParameterSet.Config as cms

# Customize functions for the OT bad-module failure-scenario sweep, used via
# cmsDriver.py's --customise flag on the DIGI step. Each turns on KillModules/DeadModules_DB
# on the three digitizer algorithms and wires SiPhase2OTFakeQualityESSource with the
# killSpecs for one scenario. noKill() is the baseline (default behavior, nothing to do).


def _wireKillModules(process, killSpecs, seed=12345, debug=True):
    process.SiPhase2OTFakeQualityESSource = cms.ESSource(
        "SiPhase2OTFakeQualityESSource",
        killSpecs=cms.VPSet(killSpecs),
        seed=cms.uint32(seed),
        debug=cms.untracked.bool(debug),
    )
    for algo in ("SSDigitizerAlgorithm", "PSSDigitizerAlgorithm", "PSPDigitizerAlgorithm"):
        pset = getattr(process.mix.digitizers.pixel, algo)
        pset.KillModules = cms.bool(True)
        pset.DeadModules_DB = cms.bool(True)
    return process


def noKill(process):
    # Baseline: KillModules stays off (default False in the digitizer cfi), no ES source needed.
    return process


def killAll2S(process):
    return _wireKillModules(process, [cms.PSet(moduleType=cms.string("Ph2SS"), fraction=cms.double(1.0))])


def killAllPSp(process):
    return _wireKillModules(process, [cms.PSet(moduleType=cms.string("Ph2PSP"), fraction=cms.double(1.0))])


def killAllPSs(process):
    return _wireKillModules(process, [cms.PSet(moduleType=cms.string("Ph2PSS"), fraction=cms.double(1.0))])


def killAllPS(process):
    return _wireKillModules(process, [cms.PSet(moduleType=cms.string("Ph2PS"), fraction=cms.double(1.0))])


def addTrackerDQM(process):
    # otDigiMon fills the OT digi-level DQM histograms (digis/module, occupancy, etc.)
    # from the mix:Tracker digi collection directly -- no RECO needed. It is not scheduled
    # by pdigi_valid on its own, and the full trackerphase2DQMSource sequence pulls in
    # cluster/rechit monitors that DO need local reco, so only otDigiMon is added here.
    process.load('DQM.SiTrackerPhase2.Phase2TrackerMonitorDigi_cff')
    process.otDigiMon_step = cms.EndPath(process.otDigiMon)
    process.schedule.append(process.otDigiMon_step)
    return process
