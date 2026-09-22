#!/bin/bash
# Failure-scenario DQM sweep: generates one shared TTbar GEN-SIM sample, then runs the
# DIGI step (with pdigi_valid, which includes its own tracker digi-validation DQM) once
# per kill scenario, followed by HARVESTING to produce a comparable DQM.root per scenario.
#
# Usage: ./run_scenarios_sweep.sh [N_EVENTS] [SCENARIO_NAME]
#   N_EVENTS defaults to 1000.
#   SCENARIO_NAME optionally restricts the run to just one scenario (one of: Baseline,
#   Ph2SS_100, Ph2PSP_100, Ph2PSS_100, Ph2PS_100). Omit it to run all five.
#   The shared GEN-SIM sample is generated (or reused, if already present) regardless.
#
# Examples:
#   ./run_scenarios_sweep.sh 1000 Baseline   # just the baseline, 1000 events
#   ./run_scenarios_sweep.sh 1000            # full sweep, 1000 events
#
# Run this from anywhere after `cmsenv` in the CMSSW_20_0_0 area (it cd's into the CMSSW
# src area itself to make sure the Scenarios/Sweep customize module is on the path).

set -e

N_EVENTS=${1:-1000}
SCENARIO_FILTER=${2:-}
CMSSW_SRC="/afs/cern.ch/user/s/sduponch/private/PhD/Ph2TrackerDev/CMSSW_20_0_0/src"
EOS_BASE="/eos/user/s/sduponch/PhD/Phase2Tracker/FailureScenarios_Sweep${N_EVENTS}"
CONDITIONS="auto:phase2_realistic_T35"
ERA="Phase2C22I13M9"
GEOMETRY="ExtendedRun4D121"

cd "${CMSSW_SRC}"
eval `scram runtime -sh`

mkdir -p "${EOS_BASE}"
WORKDIR=$(mktemp -d)
cd "${WORKDIR}"

echo "=== Working dir: ${WORKDIR} ==="
echo "=== Output base (EOS): ${EOS_BASE} ==="
echo "=== N_EVENTS: ${N_EVENTS} ==="

# --- Step 1: shared GEN-SIM sample (only generated once, reused by all scenarios) ---
GENSIM_FILE="${EOS_BASE}/step1_GENSIM.root"
if [ -f "${GENSIM_FILE}" ]; then
  echo "=== GEN-SIM sample already exists, skipping: ${GENSIM_FILE} ==="
else
  echo "=== Generating GEN-SIM sample (${N_EVENTS} TTbar events) ==="
  cmsDriver.py TTbar_14TeV_TuneCP5_cfi \
    --conditions ${CONDITIONS} -n ${N_EVENTS} \
    --era ${ERA} \
    -s GEN,SIM \
    --eventcontent FEVTDEBUG --datatier GEN-SIM \
    --geometry ${GEOMETRY} \
    --beamspot HLLHC14TeV \
    --pileup NoPileUp \
    --fileout file:step1_GENSIM.root \
    --python_filename step1_GENSIM.py \
    --no_exec
  cmsRun step1_GENSIM.py
  cp step1_GENSIM.root "${GENSIM_FILE}"
  echo "=== GEN-SIM sample stored: ${GENSIM_FILE} ==="
fi

# --- Step 2/3 per scenario: DIGI (with our kill customize) + HARVESTING ---
# Scenario name -> customize function in Scenarios/Sweep/killScenarios_customize.py
declare -A SCENARIOS=(
  ["Baseline"]="noKill"
  ["Ph2SS_100"]="killAll2S"
  ["Ph2PSP_100"]="killAllPSp"
  ["Ph2PSS_100"]="killAllPSs"
  ["Ph2PS_100"]="killAllPS"
)

if [ -n "${SCENARIO_FILTER}" ] && [ -z "${SCENARIOS[${SCENARIO_FILTER}]+set}" ]; then
  echo "ERROR: unknown scenario '${SCENARIO_FILTER}'. Valid names: ${!SCENARIOS[@]}"
  exit 1
fi

for NAME in "${!SCENARIOS[@]}"; do
  if [ -n "${SCENARIO_FILTER}" ] && [ "${NAME}" != "${SCENARIO_FILTER}" ]; then
    continue
  fi
  FUNC="${SCENARIOS[$NAME]}"
  SCEN_DIR="${EOS_BASE}/${NAME}"
  mkdir -p "${SCEN_DIR}"

  echo ""
  echo "=== Scenario ${NAME} (customize: ${FUNC}) ==="

  echo "--- DIGI step ---"
  cmsDriver.py step2 \
    --conditions ${CONDITIONS} -n -1 \
    --era ${ERA} \
    -s DIGI:pdigi_valid \
    --eventcontent FEVTDEBUGHLT,DQM --datatier GEN-SIM-DIGI,DQMIO \
    --geometry ${GEOMETRY} \
    --pileup NoPileUp \
    --customise Scenarios/Sweep/killScenarios_customize.${FUNC}+addTrackerDQM \
    --filein file:"${GENSIM_FILE}" \
    --fileout file:step2_DIGI_${NAME}.root \
    --python_filename step2_DIGI_${NAME}.py \
    --no_exec
  cmsRun step2_DIGI_${NAME}.py
  cp step2_DIGI_${NAME}.root "${SCEN_DIR}/step2_DIGI.root"
  cp step2_DIGI_${NAME}_inDQM.root "${SCEN_DIR}/step2_inDQM.root"

  echo "--- HARVESTING step ---"
  # NOT cmsDriver's -s HARVESTING:@standardDQM -- verified that expects RECO-level
  # histograms (tracking, muon, HLT, ...) we never produce and crashes. The actual
  # Phase-2-tracker harvesting sequence is empty in this release (nothing to
  # post-process for these histograms beyond saving them), so this just runs DQMSaver
  # directly, matching the official reference config for this exact case
  # (DQM/SiTrackerPhase2/test/harvestingstep_phase2tk_cfg.py).
  (cd "${SCEN_DIR}" && cmsRun "${CMSSW_SRC}/Scenarios/Sweep/test/harvesting_minimal_cfg.py" \
    inputFiles=file:"${WORKDIR}/step2_DIGI_${NAME}_inDQM.root")

  echo "=== Scenario ${NAME} done, output in ${SCEN_DIR} ==="
done

echo ""
echo "=== Sweep complete. Results under ${EOS_BASE} ==="
echo "=== Working dir ${WORKDIR} left in place in case you want to inspect intermediate files; remove it manually when done. ==="
