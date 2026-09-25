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
    # Phase2TrackerMonitorDigi_cff defines TWO SEPARATE module instances of the same
    # class, not one shared one: otDigiMon (PixelPlotFillingFlag=False,
    # TopFolderName="TrackerPhase2OTDigi") and pixDigiMon (PixelPlotFillingFlag=True,
    # TopFolderName="TrackerPhase2ITDigi"). otDigiMon alone NEVER fills IT histograms --
    # verified by inspecting a real harvested file: with only otDigiMon scheduled, no
    # TrackerPhase2ITDigi folder exists at all. Both need scheduling for OT+IT coverage;
    # they write to separate top folders so there's no collision between them.
    # Their default InnerPixelDigiSource/digi InputTags are correct as-is -- verified via
    # edmDumpEventContent that simSiPixelDigis (not mix) is the actual persisted label
    # for PixelDigi, even in a pure Phase-2 DIGI-only job.
    process.load('DQM.SiTrackerPhase2.Phase2TrackerMonitorDigi_cff')
    process.DigiMon_step = cms.EndPath(process.otDigiMon + process.pixDigiMon)
    process.schedule.append(process.DigiMon_step)
    return process


def _wireKillModulesIT(process, killSpecs, seed=12345, debug=True):
    process.SiPhase2ITFakeQualityESSource = cms.ESSource(
        "SiPhase2ITFakeQualityESSource",
        killSpecs=cms.VPSet(killSpecs),
        seed=cms.uint32(seed),
        debug=cms.untracked.bool(debug),
    )
    # Pixel3DDigitizerAlgorithm inherits PixelDigitizerAlgorithm's module_killing_DB
    # code but is a SEPARATE configured instance (its own PSet) -- its own
    # DeadModules_DB flag must be turned on too, or Ph2PXB3D-classified modules would
    # never even reach the kill check at all.
    for algo in ("PixelDigitizerAlgorithm", "Pixel3DDigitizerAlgorithm"):
        pset = getattr(process.mix.digitizers.pixel, algo)
        pset.KillModules = cms.bool(True)
        pset.DeadModules_DB = cms.bool(True)
    return process


def killAllPh2PXB(process):
    return _wireKillModulesIT(process, [cms.PSet(moduleType=cms.string("Ph2PXB"), fraction=cms.double(1.0))])


def killAllPh2PXF(process):
    return _wireKillModulesIT(process, [cms.PSet(moduleType=cms.string("Ph2PXF"), fraction=cms.double(1.0))])


def killAllPh2PXB3D(process):
    return _wireKillModulesIT(process, [cms.PSet(moduleType=cms.string("Ph2PXB3D"), fraction=cms.double(1.0))])
