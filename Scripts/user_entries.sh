#!/bin/bash

# different output colors
COLOR='\033[1;36m'          # bold;cyan
COLOR_WARNING='\033[1;33m'  # bold;yellow
COLOR_PROMPT='\033[1;32m'   # bold;green
COLOR_ERROR='\033[1;31m'    # bold;red
NO_COLOR='\033[0m'

# sum of error codes returned by the BrokerageCenter: 0 means all users were successfully created
ERROR_CODES=0

###############################################################################
BrokerageCenter -u democlient -p password -i Demo Client democlient@shift -s
ERROR_CODES=$((${ERROR_CODES} + ${?}))
BrokerageCenter -u webclient -p password -i Web Client webclient@shift -s
ERROR_CODES=$((${ERROR_CODES} + ${?}))
BrokerageCenter -u qtclient -p password -i Qt Client qtclient@shift -s
ERROR_CODES=$((${ERROR_CODES} + ${?}))
###############################################################################

###############################################################################
for i in $(seq -f "%03g" 1 10)
do
    BrokerageCenter -u test$i -p password -i Test $i test@shift -s
    ERROR_CODES=$((${ERROR_CODES} + ${?}))
done
###############################################################################

if [ ${ERROR_CODES} -eq 0 ]
then
    echo
    echo -ne "status: ${COLOR_PROMPT}all users were successfully created${NO_COLOR}"
    echo
    exit 0
else
    echo
    echo -ne "status: ${COLOR_ERROR}some of the users failed to be created${NO_COLOR}"
    echo
    exit 1
fi
