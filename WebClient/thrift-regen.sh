#!/bin/bash

WEBCLIENT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

### REGENERATE THRIFT FILES

echo "REGENERATING THRIFT FILES"
cd "${WEBCLIENT_ROOT}/backend/service/thrift"
rm -rf gen-cpp
thrift -r --gen cpp shift_service.thrift
rm -rf gen-php
thrift -r --gen php shift_service.thrift

### MOVE PHP TO INTENDED LOCATION

echo "MOVING PHP TO ITS PROPER DESTINATION"
cd "${WEBCLIENT_ROOT}/frontend/service/thrift"
rm -rf gen-php
mv "${WEBCLIENT_ROOT}/backend/service/thrift/gen-php" .

### DONE

echo "DONE!"
