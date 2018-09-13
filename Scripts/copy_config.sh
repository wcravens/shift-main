#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Brokerage Center
if [ ! -d ${SCRIPTS_DIR}/../BrokerageCenter/config ]
then
    cp -r ${SCRIPTS_DIR}/../BrokerageCenter/config.default ${SCRIPTS_DIR}/../BrokerageCenter/config
fi

# Datafeed Engine
if [ ! -d ${SCRIPTS_DIR}/../DatafeedEngine/config ]
then
    cp -r ${SCRIPTS_DIR}/../DatafeedEngine/config.default ${SCRIPTS_DIR}/../DatafeedEngine/config
fi

# Matching Engine
if [ ! -d ${SCRIPTS_DIR}/../MatchingEngine/config ]
then
    cp -r ${SCRIPTS_DIR}/../MatchingEngine/config.default ${SCRIPTS_DIR}/../MatchingEngine/config
fi

# Web Client
if [ ! -d ${SCRIPTS_DIR}/../WebClient/config ]
then
    cp -r ${SCRIPTS_DIR}/../WebClient/config.default ${SCRIPTS_DIR}/../WebClient/config
fi
