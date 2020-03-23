#!/bin/bash

rm -rf shiftpy;
mkdir shiftpy;
thrift -r -out shiftpy --gen py shift_service.thrift;

