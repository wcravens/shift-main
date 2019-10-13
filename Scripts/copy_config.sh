#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Brokerage Center
[ -d ${SCRIPTS_DIR}/../BrokerageCenter/config ] && rm -r ${SCRIPTS_DIR}/../BrokerageCenter/config
cp -r ${SCRIPTS_DIR}/../BrokerageCenter/config.default ${SCRIPTS_DIR}/../BrokerageCenter/config

# Datafeed Engine
[ -d ${SCRIPTS_DIR}/../DatafeedEngine/config ] && rm -r ${SCRIPTS_DIR}/../DatafeedEngine/config
cp -r ${SCRIPTS_DIR}/../DatafeedEngine/config.default ${SCRIPTS_DIR}/../DatafeedEngine/config

# Matching Engine
[ -d ${SCRIPTS_DIR}/../MatchingEngine/config ] && rm -r ${SCRIPTS_DIR}/../MatchingEngine/config
cp -r ${SCRIPTS_DIR}/../MatchingEngine/config.default ${SCRIPTS_DIR}/../MatchingEngine/config

# Web Client
[ -d ${SCRIPTS_DIR}/../WebClient/config ] && rm -r ${SCRIPTS_DIR}/../WebClient/config
cp -r ${SCRIPTS_DIR}/../WebClient/config.default ${SCRIPTS_DIR}/../WebClient/config
