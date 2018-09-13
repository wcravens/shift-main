#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Brokerage Center
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../BrokerageCenter/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../BrokerageCenter/.vscode/settings.json

# Datafeed Engine
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../DatafeedEngine/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../DatafeedEngine/.vscode/settings.json

# Lib Core Client
if [ ! -d ${SCRIPTS_DIR}/../LibCoreClient/.vscode ]
then
    mkdir ${SCRIPTS_DIR}/../LibCoreClient/.vscode
fi
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../LibCoreClient/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../LibCoreClient/.vscode/settings.json

# Lib Misc Utils
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../LibMiscUtils/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../LibMiscUtils/.vscode/settings.json

# Matching Engine
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../MatchingEngine/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../MatchingEngine/.vscode/settings.json

# New Matching Engine
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../NewMatchingEngine/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../NewMatchingEngine/.vscode/settings.json

# Web Client (backend)
cp ${SCRIPTS_DIR}/vscode/c_cpp_properties.json ${SCRIPTS_DIR}/../WebClient/backend/.vscode/c_cpp_properties.json
cp ${SCRIPTS_DIR}/vscode/settings.json ${SCRIPTS_DIR}/../WebClient/backend/.vscode/settings.json
