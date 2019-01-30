#!/bin/bash

####################################################
#
# Scheduling tasks with Crontab:
#
# - In the Terminal:
#       usr@Ubuntu:~$ crontab -e
# - Chose your favorite editor, then add the following line to the end of the file:
#       */5 * * * * ~/SHIFT/Scripts/supervisor.sh
# - This will run the supervisor script every 5 minutes
#   (restarting all services if any of them crashed during the last 5 minutes).
#
# The syntax of crontab entries is the following:
#
# * * * * * command to be executed
# - - - - -
# | | | | |
# | | | | ----- Day of week (0 - 7) (Sunday=0 or 7)
# | | | ------- Month (1 - 12)
# | | --------- Day of month (1 - 31)
# | ----------- Hour (0 - 23)
# ------------- Minute (0 - 59)
#
####################################################

# different output colors
COLOR='\033[1;36m'          # bold;cyan
COLOR_WARNING='\033[1;33m'  # bold;yellow
COLOR_PROMPT='\033[1;32m'   # bold;green
COLOR_ERROR='\033[1;31m'    # bold;red
NO_COLOR='\033[0m'

# directory from which the script is being run from
CURRENT_DIR="$( pwd )"

# root folder of the supervisor script
SUPERVISOR_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage
{
    echo -e ${COLOR}
    echo "OVERVIEW: Stevens High Frequency Trading (SHIFT) Simulation System Supervisor Script"
    echo
    echo "USAGE: ./supervisor.sh [options]"
    echo
    echo "OPTIONS:";
    echo "  -h [ --help ]                      Display available options"
    echo "  -k [ --kill ]                      Kill all running services"
    echo -e ${NO_COLOR}
}

KILL_FLAG=0

while [ "${1}" != "" ]; do
    case ${1} in
        -h | --help )
            usage
            exit 0
            ;;
        -k | --kill )
            KILL_FLAG=1
            shift
            ;;
        *)
            echo
            echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} ${1} option is not available (please see usage with -h or --help)"
            echo
            exit 1
            ;;
    esac
done

case ${OSTYPE} in
    darwin* )
        ;;
    linux* )
        ;;
    *)
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} ${OSTYPE} is not supported"
        echo
        exit 1
        ;;
esac

if [ ! -f ~/.shift/execution.log ]
then
    echo "Date                         , Runtime" > ~/.shift/execution.log
fi

cd ${SUPERVISOR_ROOT}/..

./startup.sh -s >/dev/null 2>&1 # check if all services are running properly
STATUS_RETURN=${?}

if [ ${STATUS_RETURN} -gt 0 ] || [ ${KILL_FLAG} -ne 0 ]
then
    ./startup.sh -k >/dev/null 2>&1 # terminate all services that are still running

    if [ -f ~/.shift/supervisor.log ]
    then
        source ~/.shift/supervisor.log # START_DATE is saved at supervisor.log
        rm ~/.shift/supervisor.log
        END_DATE="$( date +%s )"
        RUNTIME=$(( ${END_DATE} - ${START_DATE} ))
        
        echo -n "$( date ) , " >> ~/.shift/execution.log
        if [ ${KILL_FLAG} -ne 0 ]
        then
            printf "%02d:%02d:%02d (killed)\n" $((${RUNTIME}/3600)) $((${RUNTIME}%3600/60)) $((${RUNTIME}%60)) >> ~/.shift/execution.log
        else
            printf "%02d:%02d:%02d (" $((${RUNTIME}/3600)) $((${RUNTIME}%3600/60)) $((${RUNTIME}%60)) >> ~/.shift/execution.log
            [ $(( ${STATUS_RETURN} & 2#00001 )) -gt 0 ] && echo -n " DatafeedEngine " >> ~/.shift/execution.log
            [ $(( ${STATUS_RETURN} & 2#00010 )) -gt 0 ] && echo -n " MatchingEngine " >> ~/.shift/execution.log
            [ $(( ${STATUS_RETURN} & 2#00100 )) -gt 0 ] && echo -n " BrokerageCenter " >> ~/.shift/execution.log
            [ $(( ${STATUS_RETURN} & 2#01000 )) -gt 0 ] && echo -n " WebClient " >> ~/.shift/execution.log
            [ $(( ${STATUS_RETURN} & 2#10000 )) -gt 0 ] && echo -n " pushServer " >> ~/.shift/execution.log
            echo ")" >> ~/.shift/execution.log
        fi
    fi

    if [ ${KILL_FLAG} -eq 0 ]
    then
        echo "START_DATE=$( date +%s )" > ~/.shift/supervisor.log
        ./startup.sh >/dev/null 2>&1 # start all services
    fi
fi

cd ${CURRENT_DIR}
