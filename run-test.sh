#!/bin/bash

# ============
# Environments
# ============
PROJECT_HOME=$(pwd)
PROJECT_BUILD_HOME=${PROJECT_HOME}/build
SAMPLE_HOME=${PROJECT_HOME}/samples
SAMPLE_RESULT_HOME=${SAMPLE_HOME}/result
SAMPLE_CONFIG_HOME=${SAMPLE_HOME}/config
SAMPLE_BASELINE_HOME=${SAMPLE_HOME}/baseline
SAMPLE_DIFFERENTIAL=${SAMPLE_HOME}/differential.diff

# ==================
# Internal functions
# ==================
function __build_mqsim() {
  # Get number of threads for compile
  if [[ "Darwin" == $(uname) ]]; then
    # macOS commands
    __N_CORES=$(sysctl -n hw.logicalcpu)
  else
    # linux commands
    __N_CORES=$(nproc)
  fi

  if [[ -d ${PROJECT_BUILD_HOME} ]]; then
    rm -Rf ${PROJECT_BUILD_HOME};
  fi

  mkdir ${PROJECT_BUILD_HOME};

  cd ${PROJECT_BUILD_HOME};

  # Build
  cmake -DCMAKE_BUILD_TYPE=Release \
        -DFORCE_INLINE=ON \
        -DBLOCK_ERASE_HISTO=Off \
        -DSKIP_EXCEPTION_CHECK=On \
        -DSTATIC_BOOST=Off \
        ${PROJECT_HOME};

  make -j ${__N_CORES};

  # Return to project home
  cd ${PROJECT_HOME}
}

function __kill_all_other_mqsim() {
  # Before run, kill all MQSim.
  if [[ 0 -lt $(ps -e | grep MQSim | wc -l) ]]; then
    killall MQSim;
  fi
}

function __run_sample() {
  __SAMPLE_HOME=$2/$1
  __RESULT_HOME=$3/$1

  __SAMPLE_SSD_CONF=${__SAMPLE_HOME}/ssd.xml

  # 1. Remove old result file in $__SAMPLE_HOME

  rm ${__SAMPLE_HOME}/*_scenario_*.xml 2> /dev/null;

  for __FLOW in ${__SAMPLE_HOME}/flow-[0-9]*.xml; do
    echo "[run]     ${PROJECT_BUILD_HOME}/bin/MQSim -i ${__SAMPLE_SSD_CONF} -w ${__FLOW}";
    ${PROJECT_BUILD_HOME}/bin/MQSim -i ${__SAMPLE_SSD_CONF} -w ${__FLOW} | grep "Total simulation time";
  done;

  mkdir -p ${__RESULT_HOME};
  mv ${__SAMPLE_HOME}/*_scenario_*.xml ${__RESULT_HOME};
}

# =============================
# Run simulator for all samples
# =============================
__kill_all_other_mqsim;

__build_mqsim;

for __DIR in ${SAMPLE_CONFIG_HOME}/*; do
  __run_sample "$(basename ${__DIR})"  \
               "${SAMPLE_CONFIG_HOME}" \
               "${SAMPLE_RESULT_HOME}";
done

# Check result differentials.
diff -urN ${SAMPLE_BASELINE_HOME} ${SAMPLE_RESULT_HOME} > ${SAMPLE_DIFFERENTIAL};

if [[ 0 -lt $(cat ${SAMPLE_DIFFERENTIAL} | wc -l) ]]; then
  echo "[failed]  Different simulation result!" 1>&2;
  exit -1;
else
  echo "[success] Same simulation result with before";
  exit 0;
fi
