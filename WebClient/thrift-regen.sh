#!/bin/bash

ROOT_DIR=`pwd`
GENCPP_DIR="${ROOT_DIR}/backend/service/thrift/gen-cpp";
GENPHP_DIR="${ROOT_DIR}/backend/service/thrift/gen-php";

### RECREATE THRIFT FILES
echo $ROOT_DIR;

echo "REGENERATING THRIFT FILES";
cd backend/service/thrift;
rm -rf gen-cpp;
thrift -r --gen cpp shift_service.thrift

rm -rf gen-php;
thrift -r --gen php shift_service.thrift;

### MOVE PHP TO INTENDED LOCATION

echo "MOVING PHP TO ITS PROPER DESTINATION"
cd "${ROOT_DIR}/frontend/service/thrift";
rm -rf gen-php;
cp -r "${GENPHP_DIR}" .;
touch "php.move.done"

echo "gen-php MV DONE!"


