#!/bin/bash

# default loading time parameter for each service
declare -a LOADING_TIME
LOADING_TIME[1]=1   # DatafeedEngine
LOADING_TIME[2]=5   # MatchingEngine
LOADING_TIME[3]=5   # BrokerageCenter
LOADING_TIME[4]=1   # WebClient backend  (WebClient)
LOADING_TIME[5]=1   # WebClient frontend (pushServer)

# default time-out parameter for services
TIME_OUT=400

# whether to keep execution log or not
LOG_FLAG=0

# different output colors
COLOR='\033[1;36m'          # bold;cyan
COLOR_WARNING='\033[1;33m'  # bold;yellow
COLOR_PROMPT='\033[1;32m'   # bold;green
COLOR_ERROR='\033[1;31m'    # bold;red
NO_COLOR='\033[0m'

# regex to capture any integer number
IS_NUMBER='^[0-9]+$'

# directory from which the script is being run from
CURRENT_DIR="$( pwd )"

# root folder of the cloned SHIFT repository
SHIFT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage
{
    echo -e ${COLOR}
    echo "OVERVIEW: Stevens High Frequency Trading (SHIFT) Simulation System Startup Script"
    echo
    echo "USAGE: ./startup.sh [options] <args>"
    echo
    echo "OPTIONS:";
    echo "  -h [ --help ]                      Display available options"
    echo "  -m [ --modules ] DE|ME|BC|WC|PS    List of modules to start up or terminate"
    echo "  -d [ --date ]                      Simulation date (format: YYYY-MM-DD)"
    echo "  -r [ --reset ]                     Reset stored portfolio and trading records information"
    echo "  -e [ --readonlyportfolio ]         Read-only portfolio information"
    echo "  -t [ --timeout ]                   Time-out duration counted in minutes"
    echo "                                     (default: 400)"
    echo "  -l [ --log ]                       Create a log of the execution"
    echo "  -s [ --status ]                    Status of the running services"
    echo "  -k [ --kill ]                      Kill all running services"
    echo -e ${NO_COLOR}
}

function startService
{
# default installation path prefix
    INSTALL_PREFIX="/usr/local"

    # if installation did not use default installation path
    if [ -f ~/.shift/${1}/install.log ]
    then
        source ~/.shift/${1}/install.log
    fi

    # if service is not installed, do not try to run it
    ( command -v ${INSTALL_PREFIX}/bin/${1} >/dev/null ) || return

    # start a new instance of the service
    echo -e ${COLOR}
    echo -ne "Starting ${1}...\r"
    if [ ${LOG_FLAG} -ne 0 ]
    then
        nohup ${INSTALL_PREFIX}/bin/${1} -v ${3} </dev/null >~/.shift/${1}/$( date +%s )-execution.log 2>&1 &
    else
        nohup ${INSTALL_PREFIX}/bin/${1} ${3} </dev/null >/dev/null 2>&1 &
    fi

    # loading time
    for (( i=${2} ; i>=1 ; i-- ))
    do
        echo -ne "Starting ${1}... ${i}\033[0K\r"
        sleep 1
    done
    echo -n "Starting ${1}... done."
    echo -e ${NO_COLOR}
}

function statusService
{
    # if service is not installed, do not check if it is running
    ( command -v /usr/local/bin/${1} >/dev/null ) || return 0

    if ( pgrep -x ${1} > /dev/null )
    then
        echo
        echo -ne "shift: ${1} status: ${COLOR_PROMPT}running${NO_COLOR}"
        echo
        return 0
    else
        echo
        echo -ne "shift: ${1} status: ${COLOR_ERROR}not running${NO_COLOR}"
        echo
        return 1
    fi
}

function killService
{
    # if the service is currently running, terminate it
    if ( pgrep -x ${1} > /dev/null )
    then
        echo -e ${COLOR}
        echo -n "Terminating running instance of ${1}..."
        echo -e ${NO_COLOR}
        killall -v -9 ${1}
    fi

    # delete all log & store folders for the service
    COUNT_LOG="$( ls -1 log/${2}* 2>/dev/null | wc -l )"
    [ ${COUNT_LOG} != 0 ] && rm -r log/${2}*
    COUNT_STORE="$( ls -1 store/${2}* 2>/dev/null | wc -l )"
    [ ${COUNT_STORE} != 0 ] && rm -r store/${2}*
}

function startPushServer
{
    # if the WebClient backend is not running, do not try to run pushServer
    ( pgrep -x WebClient > /dev/null ) || return

    # if log is to be kept, erase previous version
    [ ${LOG_FLAG} -ne 0 ] && [ -f ~/.shift/WebClient/push_server.log ] && rm ~/.shift/WebClient/push_server.log

    # accessing the frontend directory
    cd ${SHIFT_ROOT}/WebClient/frontend/service/transfer

    # start a new instance of the service
    echo -e ${COLOR}
    echo -ne "Starting pushServer...\r"
    if [ ${LOG_FLAG} -ne 0 ]
    then
        nohup /usr/bin/php pushServer.php </dev/null >~/.shift/WebClient/$( date +%s )-push_server.log 2>&1 &
    else
        nohup /usr/bin/php pushServer.php </dev/null >/dev/null 2>&1 &
    fi

    # loading time
    for (( i=${1} ; i>=1 ; i-- ))
    do
        echo -ne "Starting pushServer... ${i}\033[0K\r"
        sleep 1
    done
    echo -n "Starting pushServer... done."
    echo -e ${NO_COLOR}

    # returning to the directory from which the script is being run from
    cd ${CURRENT_DIR}
}

function statusPushServer
{
    # if the WebClient is not installed, do not check if the pushServer is running
    ( command -v /usr/local/bin/WebClient >/dev/null ) || return 0

    # if the push server is currently running, find out its PID
    PUSH_SERVER_PID=$( ps aux | grep [p]ushServer  | grep -v auto | head -1 | awk 'NR==1 {print $2; exit}' )

    if [ "${PUSH_SERVER_PID}" != "" ]
    then
        echo
        echo -ne "shift: pushServer status: ${COLOR_PROMPT}running${NO_COLOR}"
        echo
        return 0
    else
        echo
        echo -ne "shift: pushServer status: ${COLOR_ERROR}not running${NO_COLOR}"
        echo
        return 1
    fi
}

function killPushServer
{
    # if the push server is currently running, find out its PID
    PUSH_SERVER_PID=$( ps aux | grep [p]ushServer  | grep -v auto | head -1 | awk 'NR==1 {print $2; exit}' )

    # if the push server is currently running, terminate it
    if [ "${PUSH_SERVER_PID}" != "" ]
    then
        echo -e ${COLOR}
        echo -ne "Terminating running instance of pushServer...\r"
        kill -9 ${PUSH_SERVER_PID}
        sleep 1
        echo -n "Terminating running instance of pushServer... done."
        echo -e ${NO_COLOR}
    fi
}

declare -a MODULES

SIMULATION_DATE=""
RESET_FLAG=0
PFDBREADONLY_FLAG=0

STATUS_FLAG=0
KILL_FLAG=0

while [ "${1}" != "" ]; do
    case ${1} in
        -h | --help )
            usage
            exit 0
            ;;
        -m | --modules )
            while true
            do
                shift
                case ${1} in
                    DE | de | DatafeedEngine | datafeedengine )
                        MODULES+=("1_DE")
                        LOADING_TIME_INDEX=1
                        ;;
                    ME | me | MatchingEngine | matchingengine )
                        MODULES+=("2_ME")
                        LOADING_TIME_INDEX=2
                        ;;
                    BC | bc | BrokerageCenter | brokeragecenter )
                        MODULES+=("3_BC")
                        LOADING_TIME_INDEX=3
                        ;;
                    WC | wc | WebClient | webclient )
                        MODULES+=("4_WC")
                        LOADING_TIME_INDEX=4
                        ;;
                    PS | ps | PushServer | pushserver )
                        MODULES+=("5_PS")
                        LOADING_TIME_INDEX=5
                        ;;
                    * )
                        if [[ ${1} =~ ${IS_NUMBER} ]]
                        then
                            LOADING_TIME[${LOADING_TIME_INDEX}]=${1}
                        else
                            break
                        fi
                        ;;
                esac
            done
            ;;
        -d | --date )
            shift
            SIMULATION_DATE=${1}
            shift
            ;;
        -r | --reset )
            RESET_FLAG=1
            shift
            ;;
        -e | --readonlyportfolio )
            PFDBREADONLY_FLAG=1
            shift
            ;;
        -t | --timeout )
            shift
            if [[ ${1} =~ ${IS_NUMBER} ]]
            then
                TIME_OUT=${1}
            fi
            shift
            ;;
        -l | --log )
            LOG_FLAG=1
            shift
            ;;
        -s | --status )
            STATUS_FLAG=1
            shift
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

MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | awk '!a[$0]++' | tr '\n' ' '))

if [ ${#MODULES[@]} -eq 0 ]
then
    MODULES+=("1_DE")
    MODULES+=("2_ME")
    MODULES+=("3_BC")
    MODULES+=("4_WC")
    MODULES+=("5_PS")
fi

if [ ${STATUS_FLAG} -ne 0 ]
then
    MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | sort | tr '\n' ' '))
    STATUS_RETURN=0

    for i in ${MODULES[@]}
    do
        case ${i} in
            1_DE )
                statusService "DatafeedEngine"
                STATUS_RETURN=$(( ${STATUS_RETURN} | (${?} * 2#00001) ))
                ;;
            2_ME )
                statusService "MatchingEngine"
                STATUS_RETURN=$(( ${STATUS_RETURN} | (${?} * 2#00010) ))
                ;;
            3_BC )
                statusService "BrokerageCenter"
                STATUS_RETURN=$(( ${STATUS_RETURN} | (${?} * 2#00100) ))
                ;;
            4_WC )
                statusService "WebClient"
                STATUS_RETURN=$(( ${STATUS_RETURN} | (${?} * 2#01000) ))
                ;;
            5_PS )
                statusPushServer
                STATUS_RETURN=$(( ${STATUS_RETURN} | (${?} * 2#10000) ))
                ;;
            * )
                break
                ;;
        esac
    done

    echo
    exit ${STATUS_RETURN}
fi

# terminate previous instances of services that may be still running
MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | sort -r | tr '\n' ' '))

for i in ${MODULES[@]}
do
    case ${i} in
        1_DE )
            killService "DatafeedEngine" "DE"
            ;;
        2_ME )
            killService "MatchingEngine" "ME"
            ;;
        3_BC )
            killService "BrokerageCenter" "BC"
            ;;
        4_WC )
            killService "WebClient" "webclient"
            ;;
        5_PS )
            killPushServer
            ;;
        * )
            break
            ;;
    esac
done

# delete log & store folders if empty
COUNT_LOG="$( ls -1 log/* 2>/dev/null | wc -l )"
COUNT_STORE="$( ls -1 store/* 2>/dev/null | wc -l )"
echo -ne ${COLOR_ERROR}
[ ${COUNT_LOG} -eq 0 ] && [ -d log ] && rm -r log && echo && echo "Deleting log folder..."
[ ${COUNT_STORE} -eq 0 ] && [ -d store ] && rm -r store && echo && echo "Deleting store folder..."
echo -ne ${NO_COLOR}

if [ ${KILL_FLAG} -eq 0 ]
then
    MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | sort | tr '\n' ' '))

    for i in ${MODULES[@]}
    do
        case ${i} in
            1_DE )
                startService "DatafeedEngine" ${LOADING_TIME[1]} "-t ${TIME_OUT}"
                ;;
            2_ME )
                [ ${SIMULATION_DATE} ] || startService "MatchingEngine" ${LOADING_TIME[2]} "-t ${TIME_OUT}"
                [ ${SIMULATION_DATE} ] && startService "MatchingEngine" ${LOADING_TIME[2]} "-t ${TIME_OUT} -d ${SIMULATION_DATE}"
                ;;
            3_BC )
                if [ ${RESET_FLAG} -eq 0 ] && [ ${PFDBREADONLY_FLAG} -eq 0 ]
                then
                    startService "BrokerageCenter" ${LOADING_TIME[3]} "-t ${TIME_OUT}"
                elif [ ${RESET_FLAG} -ne 0 ] && [ ${PFDBREADONLY_FLAG} -eq 0 ]
                then
                    startService "BrokerageCenter" ${LOADING_TIME[3]} "-t ${TIME_OUT} -r"
                elif [ ${RESET_FLAG} -eq 0 ] && [ ${PFDBREADONLY_FLAG} -ne 0 ]
                then
                    startService "BrokerageCenter" ${LOADING_TIME[3]} "-t ${TIME_OUT} -e"
                else
                    startService "BrokerageCenter" ${LOADING_TIME[3]} "-t ${TIME_OUT} -r -e"
                fi
                ;;
            4_WC )
                startService "WebClient" ${LOADING_TIME[4]}
                ;;
            5_PS )
                startPushServer ${LOADING_TIME[5]}
                ;;
            * )
                break
                ;;
        esac
    done
fi

echo
