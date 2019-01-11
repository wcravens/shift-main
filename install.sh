#!/bin/bash

# different output colors
COLOR='\033[1;36m'          # bold;cyan
COLOR_WARNING='\033[1;33m'  # bold;yellow
COLOR_PROMPT='\033[1;32m'   # bold;green
COLOR_ERROR='\033[1;31m'    # bold;red
NO_COLOR='\033[0m'

# current user running the installation script
CURRENT_USER="$( who | awk 'NR==1 {print $1; exit}' )"

# default installation path prefix
INSTALL_PREFIX="/usr/local"

# number of cores of CPU
CORE_NUM=1
if [ "$(uname)" == "Darwin" ]; then
	if [ ! $(sysctl -n hw.physicalcpu) -eq 1 ]
	then
		CORE_NUM=$(($(sysctl -n hw.physicalcpu) - 1))
	fi
fi
if [ "$(uname)" == "Linux" ]; then
	if [ ! $(nproc) -eq 1 ]
	then
        CORE_NUM=$(($(nproc) - 1))
    fi
fi

# avoid rebuilding LM before installing
# (this should only be used during development,
#  since LM is required by all other projects)
AVOID_INSTALL=0

# delete previous build folders before installing
CLEAN_INSTALL=0

# force the copy of default configuration files
FORCE_INSTALL=0

function usage
{
    echo -e ${COLOR}
    echo "OVERVIEW: Stevens High Frequency Trading (SHIFT) Simulation System Installation Script"
    echo
    echo "USAGE: sudo ./install.sh [options] <args>"
    echo
    echo "OPTIONS:";
    echo "  -a [ --avoid ]                     Avoid rebuilding LM"
    echo "  -c [ --clean ]                     Clean install"
    echo "                                     (delete previous build folders)"
    echo "  -f [ --force ]                     Use default configuration files"
    echo "                                     (not recommended)"
    echo "  -h [ --help ]                      Display available options"
    echo "  -m [ --modules ] LM|DE|ME|BC|LC    List of modules to (un)install"
    echo "  -p [ --path ] arg                  Set (un)installation path prefix"
    echo "                                     (default: /usr/local)"
    echo "  -u [ --uninstall ]                 Uninstall"
    echo -e ${NO_COLOR}
}

function chownFix
{
    if [ "$(uname)" == "Darwin" ]; then
        chown -R ${CURRENT_USER} ${1}
    fi
    if [ "$(uname)" == "Linux" ]; then
        chown -R ${CURRENT_USER}:${CURRENT_USER} ${1}
    fi
}

function installServer
{
    echo -e ${COLOR}
    echo "Installing ${1}..."
    echo -e ${NO_COLOR}

    # create ~/.shift directory if it does not exist yet 
    [ -d ~/.shift/${1} ] || mkdir -p ~/.shift/${1}

    # save installation path to install.log (erasing previous version)
    [ -f ~/.shift/${1}/install.log ] && rm ~/.shift/${1}/install.log
    echo "INSTALL_PREFIX=${INSTALL_PREFIX}" > ~/.shift/${1}/install.log
    
    # give ownership of the ~/.shift directory to the current user
    chownFix ~/.shift

    if [ ${CLEAN_INSTALL} -ne 0 ]
    then
        [ -d ${1}/build ] && rm -r ${1}/build
    fi

    if [ ${FORCE_INSTALL} -ne 0 ]
    then
        echo -ne ${COLOR_WARNING}
        echo "Copying ${1} default configuration files!"
        echo -e ${NO_COLOR}
        [ -d ${1}/config ] && rm -r ${1}/config
        cp -r ${1}/config.default ${1}/config
        chownFix ${1}/config
    fi

    # build & install
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX} -DCMAKE_PREFIX_PATH:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}: configuration files are probably missing (please see usage with -h or --help)"
        echo
        exit 1
    fi

    # give ownership of the build directory to the current user
    chownFix ${1}/build
}

function uninstallServer
{
    echo -e ${COLOR}
    echo -n "Uninstalling ${1}..."
    echo -e ${NO_COLOR}

    # if installation did not use default installation path
    if [ -f ~/.shift/${1}/install.log ]
    then
        source ~/.shift/${1}/install.log
        rm ~/.shift/${1}/install.log
    fi

    # if installation is not found
    if [ ! -f ${INSTALL_PREFIX}/bin/${1} ]
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for ${1}: binary not found"
        echo
        return
    fi

    # remove installation
    [ -f ${INSTALL_PREFIX}/bin/${1} ] && rm ${INSTALL_PREFIX}/bin/${1}
    [ -d ${INSTALL_PREFIX}/share/shift/${1} ] && rm -r ${INSTALL_PREFIX}/share/shift/${1}

    # if the last SHIFT module is being uninstalled, delete SHIFT folders
    [ -d ${INSTALL_PREFIX}/bin ] && [ -z "$(ls -A ${INSTALL_PREFIX}/bin)" ] && rm -r ${INSTALL_PREFIX}/bin
    [ -d ${INSTALL_PREFIX}/share/shift ] && [ -z "$(ls -A ${INSTALL_PREFIX}/share/shift)" ] && rm -r ${INSTALL_PREFIX}/share/shift
    [ -d ${INSTALL_PREFIX}/share ] && [ -z "$(ls -A ${INSTALL_PREFIX}/share)" ] && rm -r ${INSTALL_PREFIX}/share
    [ -d ${INSTALL_PREFIX} ] && [ -z "$(ls -A ${INSTALL_PREFIX})" ] && rm -r ${INSTALL_PREFIX}

    echo -e ${COLOR}
    echo "${1} was uninstalled from ${INSTALL_PREFIX}"
    echo -e ${NO_COLOR}
}

function installLibrary
{
    echo -e ${COLOR}
    echo "Installing ${1}..."
    echo -e ${NO_COLOR}

    # e.g. "LibCoreClient" -> "coreclient"
    LIB_NAME=$(echo ${1} | awk 'NR==1 {print substr(tolower($1), 4); exit}') 

    # create ~/.shift directory if it does not exist yet 
    [ -d ~/.shift/${1} ] || mkdir -p ~/.shift/${1}

    # save installation path to install.log (erasing previous version)
    [ -f ~/.shift/${1}/install.log ] && rm ~/.shift/${1}/install.log
    echo "INSTALL_PREFIX=${INSTALL_PREFIX}" > ~/.shift/${1}/install.log

    # give ownership of the ~/.shift directory to the current user
    chownFix ~/.shift

    if [ ${CLEAN_INSTALL} -ne 0 ]
    then
        [ -d ${1}/build ] && rm -r ${1}/build
    fi

    # build & install debug version of the library
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX} -DCMAKE_PREFIX_PATH:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}"
        echo
        exit 1
    fi

    # build & install release version of the library
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX} -DCMAKE_PREFIX_PATH:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}"
        echo
        exit 1
    fi

    # give ownership of the build directory to the current user
    chownFix ${1}/build
}

function uninstallLibrary
{
    echo -e ${COLOR}
    echo -n "Uninstalling ${1}..."
    echo -e ${NO_COLOR}

    # e.g. "LibCoreClient" -> "coreclient"
    LIB_NAME=$(echo ${1} | awk 'NR==1 {print substr(tolower($1), 4); exit}') 

    # if installation did not use default installation path
    if [ -f ~/.shift/${1}/install.log ]
    then
        source ~/.shift/${1}/install.log
        rm ~/.shift/${1}/install.log
    fi

    # remove installation
    if [ "$(uname)" == "Darwin" ]; then
        # if installation folder is not found
        if [ ! -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.dylib ]
        then
            echo
            echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for ${1}: library not found"
            echo
            return
        fi
        # debug library
        [ -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}-d.dylib ] && rm ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}-d.dylib
        # release library
        [ -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.dylib ] && rm ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.dylib
    fi
    if [ "$(uname)" == "Linux" ]; then
        # if installation folder is not found
        if [ ! -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.so ]
        then
            echo
            echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for ${1}: library not found"
            echo
            return
        fi
        # debug library
        [ -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}-d.so ] && rm ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}-d.so
        # release library
        [ -f ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.so ] && rm ${INSTALL_PREFIX}/lib/libshift_${LIB_NAME}.so
    fi

    # remove include directory
    [ -d ${INSTALL_PREFIX}/include/shift/${LIB_NAME} ] && rm -r ${INSTALL_PREFIX}/include/shift/${LIB_NAME}

    # remove pkg-config script
    # debug package
    [ -f ${INSTALL_PREFIX}/lib/pkgconfig/libshift_${LIB_NAME}.pc ] && rm ${INSTALL_PREFIX}/lib/pkgconfig/libshift_${LIB_NAME}.pc
    # release package
    [ -f ${INSTALL_PREFIX}/lib/pkgconfig/libshift_${LIB_NAME}-d.pc ] && rm ${INSTALL_PREFIX}/lib/pkgconfig/libshift_${LIB_NAME}-d.pc

    # if the last SHIFT library is being uninstalled, delete SHIFT folders
    [ -d ${INSTALL_PREFIX}/lib/pkgconfig ] && [ -z "$(ls -A ${INSTALL_PREFIX}/lib/pkgconfig)" ] && rm -r ${INSTALL_PREFIX}/lib/pkgconfig
    [ -d ${INSTALL_PREFIX}/lib ] && [ -z "$(ls -A ${INSTALL_PREFIX}/lib)" ] && rm -r ${INSTALL_PREFIX}/lib
    [ -d ${INSTALL_PREFIX}/include/shift ] && [ -z "$(ls -A ${INSTALL_PREFIX}/include/shift)" ] && rm -r ${INSTALL_PREFIX}/include/shift
    [ -d ${INSTALL_PREFIX}/include ] && [ -z "$(ls -A ${INSTALL_PREFIX}/include)" ] && rm -r ${INSTALL_PREFIX}/include
    [ -d ${INSTALL_PREFIX} ] && [ -z "$(ls -A ${INSTALL_PREFIX})" ] && rm -r ${INSTALL_PREFIX}

    echo -e ${COLOR}
    echo "${1} was uninstalled from ${INSTALL_PREFIX}"
    echo -e ${NO_COLOR}
}

declare -a MODULES
UNINSTALL_FLAG=0

while [ "${1}" != "" ]; do
    case ${1} in
        -a | --avoid )
            AVOID_INSTALL=1
            shift
            ;;
        -c | --clean )
            CLEAN_INSTALL=1
            shift
            ;;
        -f | --force )
            FORCE_INSTALL=1
            shift
            ;;
        -h | --help )
            usage
            exit 0
            ;;
        -m | --modules )
            while true
            do
                shift
                case ${1} in
                    LM | lm | LibMiscUtils | libmiscutils )
                        MODULES+=('0_LM')
                        ;;
                    DE | de | DatafeedEngine | datafeedengine )
                        MODULES+=('1_DE')
                        ;;
                    ME | me | MatchingEngine | matchingengine )
                        MODULES+=('2_ME')
                        ;;
                    BC | bc | BrokerageCenter | brokeragecenter )
                        MODULES+=('3_BC')
                        ;;
                    LC | lc | LibCoreClient | libcoreclient )
                        MODULES+=('4_LC')
                        ;;
                    * )
                        break
                        ;;
                esac
            done
            ;;
        -p | --path )
            shift
            if [ "${1}" != "" ]
            then
                INSTALL_PREFIX=${1}
            fi
            shift
            ;;
        -u | --uinstall )
            UNINSTALL_FLAG=1
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
    darwin* ) # macOS
        ;;
    linux* )  # Linux
        ;;
    *)
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} ${OSTYPE} is not supported"
        echo
        exit 1
        ;;
esac

if [ ${EUID} -ne 0 ]
then
    echo
    echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} please run the installation script as root"
    echo
    exit 1
fi

# LibMiscUtils is a requirement for all other projects
if [ ${UNINSTALL_FLAG} -eq 0 ] && [ ${AVOID_INSTALL} -eq 0 ]
then
    [[ "${MODULES[@]}" =~ "1_DE" ]] && MODULES+=('0_LM')
    [[ "${MODULES[@]}" =~ "2_ME" ]] && MODULES+=('0_LM')
    [[ "${MODULES[@]}" =~ "3_BC" ]] && MODULES+=('0_LM')
    [[ "${MODULES[@]}" =~ "4_LC" ]] && MODULES+=('0_LM')
fi

# remove duplicates
MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | awk '!a[$0]++' | tr '\n' ' '))

if [ ${#MODULES[@]} -eq 0 ]
then
    MODULES+=('0_LM')
    MODULES+=('1_DE')
    MODULES+=('2_ME')
    MODULES+=('3_BC')
    MODULES+=('4_LC')
fi

# sort modules to be (un)installed
MODULES=($(echo ${MODULES[@]} | tr ' ' '\n' | sort | tr '\n' ' '))

if [ ${UNINSTALL_FLAG} -eq 0 ]
then
    for i in ${MODULES[@]}
    do
        case ${i} in
            0_LM )
                installLibrary "LibMiscUtils"
                ;;
            1_DE )
                installServer "DatafeedEngine"
                ;;
            2_ME )
                installServer "MatchingEngine"
                ;;
            3_BC )
                installServer "BrokerageCenter"
                ;;
            4_LC )
                installLibrary "LibCoreClient"
                ;;
            * )
                break
                ;;
        esac
    done
else
    for i in ${MODULES[@]}
    do
        case ${i} in
            0_LM )
                uninstallLibrary "LibMiscUtils"
                ;;
            1_DE )
                uninstallServer "DatafeedEngine"
                ;;
            2_ME )
                uninstallServer "MatchingEngine"
                ;;
            3_BC )
                uninstallServer "BrokerageCenter"
                ;;
            4_LC )
                uninstallLibrary "LibCoreClient"
                ;;
            * )
                break
                ;;
        esac
    done
fi

