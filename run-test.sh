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

# Get number of threads for compile
if [[ "Darwin" == $(uname) ]]; then
  # macOS commands
  __N_CORES=$(sysctl -n hw.logicalcpu)
else
  # linux commands
  __N_CORES=$(nproc)
fi

# ====================================
# Make build directory and build MQSim
# ====================================
export CC=clang
export CXX=clang++

if [[ -d ${PROJECT_BUILD_HOME} ]]; then
  rm -Rf ${PROJECT_BUILD_HOME};
fi

mkdir ${PROJECT_BUILD_HOME};

cd ${PROJECT_BUILD_HOME};

# Build
cmake -DCMAKE_BUILD_TYPE=Release ${PROJECT_HOME};
make -j ${__N_CORES};

# Return to project home
cd ${PROJECT_HOME}

# =============================
# Run simulator for all samples
# =============================
function __run_sample() {
  __SAMPLE_HOME=$2/$1
  __RESULT_HOME=$3/$1

  __SAMPLE_SSD_CONF=${__SAMPLE_HOME}/ssd.xml

  # 1. Remove old result file in $__SAMPLE_HOME

  rm ${__SAMPLE_HOME}/*_scenario_*.xml 2> /dev/null;

  for __FLOW in ${__SAMPLE_HOME}/flow-[0-9]*.xml; do
    # Hide output
    echo - Run: build/bin/MQSim -i ${__SAMPLE_SSD_CONF} -w ${__FLOW};
    build/bin/MQSim -i ${__SAMPLE_SSD_CONF} -w ${__FLOW} > /dev/null;
  done;

  mkdir -p ${__RESULT_HOME};
  mv ${__SAMPLE_HOME}/*_scenario_*.xml ${__RESULT_HOME};
}

# Before run kill all MQSim
killall MQSim;

for __DIR in ${SAMPLE_CONFIG_HOME}/*; do
  __run_sample "$(basename ${__DIR})"  \
               "${SAMPLE_CONFIG_HOME}" \
               "${SAMPLE_RESULT_HOME}";
done

diff -urN ${SAMPLE_BASELINE_HOME} ${SAMPLE_RESULT_HOME} > ${SAMPLE_DIFFERENTIAL};
