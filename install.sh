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
case ${OSTYPE} in
    darwin* ) # in macOS
		if [ ! $(sysctl -n hw.physicalcpu) -eq 1 ]
		then
			CORE_NUM=$(($(sysctl -n hw.physicalcpu) - 1))
		fi
        ;;
    linux* )  # in Linux
		if [ ! $(nproc) -eq 1 ]
		then
        	CORE_NUM=$(($(nproc) - 1))
        fi
        ;;
esac

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
    case ${OSTYPE} in
        darwin* ) # in macOS
            chown -R ${CURRENT_USER} ${1}
            ;;
        linux* )  # in Linux
            chown -R ${CURRENT_USER}:${CURRENT_USER} ${1}
            ;;
    esac
}

function installServer
{
    echo -e ${COLOR}
    echo "Installing ${1}..."
    echo -e ${NO_COLOR}

    INSTALL_PATH=${INSTALL_PREFIX}/SHIFT

    # create ~/.shift directory if it does not exist yet 
    [ -d ~/.shift/${1} ] || mkdir -p ~/.shift/${1}

    # save installation path to install.log (erasing previous version)
    [ -f ~/.shift/${1}/install.log ] && rm ~/.shift/${1}/install.log
    echo "INSTALL_PATH=${INSTALL_PATH}" > ~/.shift/${1}/install.log
    
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
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}: configuration files are probably missing (please see usage with -h or --help)"
        echo
        exit 1
    fi

    # give ownership of the build directory to the current user
    chownFix ${1}/build

    # create binary file symlink (erasing previous version)
    [ -h /usr/local/bin/${1} ] && rm /usr/local/bin/${1}
    ln -s ${INSTALL_PATH}/${1}/${1} /usr/local/bin/${1}

    # create configuration files symlinks (erasing previous versions)
    [ -d /usr/local/share/SHIFT ] || mkdir -p /usr/local/share/SHIFT
    [ -h /usr/local/share/SHIFT/${1} ] && rm /usr/local/share/SHIFT/${1}
    ln -s ${INSTALL_PATH}/${1}/config /usr/local/share/SHIFT/${1}

    echo -e ${COLOR}
    echo "${1} was installed in ${INSTALL_PATH}/${1}/:"
    echo "-- Binary file symlinked to /usr/local/bin/${1}"
    echo "-- Configuration files symlinked to /usr/local/share/SHIFT/${1}/"
    echo -e ${NO_COLOR}
}

function uninstallServer
{
    echo -e ${COLOR}
    echo -n "Uninstalling ${1}..."
    echo -e ${NO_COLOR}

    INSTALL_PATH=${INSTALL_PREFIX}/SHIFT

    # if installation did not use default installation path
    if [ -f ~/.shift/${1}/install.log ]
    then
        source ~/.shift/${1}/install.log
        rm ~/.shift/${1}/install.log
    fi

    # if installation folder is not found
    if [ ! -d ${INSTALL_PATH}/${1} ]
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for ${1}: installation path ${INSTALL_PATH}/${1} not found"
        echo
        return
    fi

    # remove symlinks
    [ -h /usr/local/bin/${1} ] && rm /usr/local/bin/${1}
    [ -h /usr/local/share/SHIFT/${1} ] && rm /usr/local/share/SHIFT/${1}

    # remove installation
    [ -d ${INSTALL_PATH}/${1} ] && rm -r ${INSTALL_PATH}/${1}

    # if the last SHIFT module is being uninstalled, delete SHIFT installation folder
    [ -d ${INSTALL_PATH} ] && [ -z "$(ls -A ${INSTALL_PATH})" ] && rm -r ${INSTALL_PATH}

    # if the last SHIFT module is being uninstalled, delete SHIFT configuration folder
    [ -d /usr/local/share/SHIFT ] && [ -z "$(ls -A /usr/local/share/SHIFT)" ] && rm -r /usr/local/share/SHIFT

    echo -e ${COLOR}
    echo "${1} was uninstalled from ${INSTALL_PATH}/${1}/"
    echo -e ${NO_COLOR}
}

function installLibrary
{
    echo -e ${COLOR}
    echo "Installing ${1}..."
    echo -e ${NO_COLOR}

    INSTALL_PATH=${INSTALL_PREFIX}/SHIFT

    # e.g. "LibCoreClient" -> "coreclient"
    LIB_NAME=$(echo ${1} | awk 'NR==1 {print substr(tolower($1), 4); exit}') 

    # create ~/.shift directory if it does not exist yet 
    [ -d ~/.shift/${1} ] || mkdir -p ~/.shift/${1}

    # save installation path to install.log (erasing previous version)
    [ -f ~/.shift/${1}/install.log ] && rm ~/.shift/${1}/install.log
    echo "INSTALL_PATH=${INSTALL_PATH}" > ~/.shift/${1}/install.log

    # give ownership of the ~/.shift directory to the current user
    chownFix ~/.shift

    if [ ${CLEAN_INSTALL} -ne 0 ]
    then
        [ -d ${1}/build ] && rm -r ${1}/build
    fi

    # build & install debug version of the library
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}"
        echo
        exit 1
    fi

    # build & install release version of the library
    cmake -H${1} -B${1}/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build ${1}/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install ${1}"
        echo
        exit 1
    fi

    # give ownership of the build directory to the current user
    chownFix ${1}/build

    # create library file symlink (erasing previous version)
    case ${OSTYPE} in
        darwin* ) # in macOS
            # debug library
            [ -h /usr/local/lib/libshift_${LIB_NAME}-d.dylib ] && rm /usr/local/lib/libshift_${LIB_NAME}-d.dylib
            ln -s ${INSTALL_PATH}/${1}/libshift_${LIB_NAME}-d.dylib /usr/local/lib/libshift_${LIB_NAME}-d.dylib
            # release library
            [ -h /usr/local/lib/libshift_${LIB_NAME}.dylib ] && rm /usr/local/lib/libshift_${LIB_NAME}.dylib
            ln -s ${INSTALL_PATH}/${1}/libshift_${LIB_NAME}.dylib /usr/local/lib/libshift_${LIB_NAME}.dylib
            ;;
        linux* )  # in Linux
            [ -h /usr/local/lib/libshift_${LIB_NAME}-d.so ] && rm /usr/local/lib/libshift_${LIB_NAME}-d.so
            ln -s ${INSTALL_PATH}/${1}/libshift_${LIB_NAME}-d.so /usr/local/lib/libshift_${LIB_NAME}-d.so
            [ -h /usr/local/lib/libshift_${LIB_NAME}.so ] && rm /usr/local/lib/libshift_${LIB_NAME}.so
            ln -s ${INSTALL_PATH}/${1}/libshift_${LIB_NAME}.so /usr/local/lib/libshift_${LIB_NAME}.so
            ;;
    esac

    # create header files symlinks (erasing previous versions)
    [ -d /usr/local/include/shift ] || mkdir -p /usr/local/include/shift
    [ -h /usr/local/include/shift/${LIB_NAME} ] && rm /usr/local/include/shift/${LIB_NAME}
    ln -s $INSTALL_PATH/$1/include /usr/local/include/shift/${LIB_NAME}

    echo -e ${COLOR}
    echo "${1} was installed in ${INSTALL_PATH}/${1}/:"
    case ${OSTYPE} in
        darwin*) # in macOS
            echo "-- Debug library file symlinked to /usr/local/lib/libshift_${LIB_NAME}-d.dylib"
            echo "-- Release library file symlinked to /usr/local/lib/libshift_${LIB_NAME}.dylib"
            ;;
        linux*)  # in Linux
            echo "-- Debug library file symlinked to /usr/local/lib/libshift_${LIB_NAME}-d.so"
            echo "-- Release library file symlinked to /usr/local/lib/libshift_${LIB_NAME}.so"
            ;;
    esac
    echo "-- Header files symlinked to /usr/local/include/shift/${LIB_NAME}/"
    echo -e ${NO_COLOR}
}

function uninstallLibrary
{
    echo -e ${COLOR}
    echo -n "Uninstalling ${1}..."
    echo -e ${NO_COLOR}

    INSTALL_PATH=${INSTALL_PREFIX}/SHIFT

    # e.g. "LibCoreClient" -> "coreclient"
    LIB_NAME=$(echo ${1} | awk 'NR==1 {print substr(tolower($1), 4); exit}') 

    # if installation did not use default installation path
    if [ -f ~/.shift/${1}/install.log ]
    then
        source ~/.shift/${1}/install.log
        rm ~/.shift/${1}/install.log
    fi

    # if installation folder is not found
    if [ ! -d ${INSTALL_PATH}/${1} ]
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for ${1}: installation path ${INSTALL_PATH}/${1} not found"
        echo
        return
    fi

    # remove symlinks
    case ${OSTYPE} in
        darwin* ) # in macOS
            # debug library
            [ -h /usr/local/lib/libshift_${LIB_NAME}-d.dylib ] && rm /usr/local/lib/libshift_${LIB_NAME}-d.dylib
            # release library
            [ -h /usr/local/lib/libshift_${LIB_NAME}.dylib ] && rm /usr/local/lib/libshift_${LIB_NAME}.dylib
            ;;
        linux* )  # in Linux
            # debug library
            [ -h /usr/local/lib/libshift_${LIB_NAME}-d.so ] && rm /usr/local/lib/libshift_${LIB_NAME}-d.so
            # release library
            [ -h /usr/local/lib/libshift_${LIB_NAME}.so ] && rm /usr/local/lib/libshift_${LIB_NAME}.so
            ;;
    esac
    [ -h /usr/local/include/shift/${LIB_NAME} ] && rm /usr/local/include/shift/${LIB_NAME}

    # remove installation
    [ -d ${INSTALL_PATH}/${1} ] && rm -r ${INSTALL_PATH}/${1}

    # remove pkg-congif script
    # debug package
    [ -h /usr/local/lib/pkgconfig/libshift_${LIB_NAME}.pc ] && rm /usr/local/lib/pkgconfig/libshift_${LIB_NAME}.pc
    # release package
    [ -h /usr/local/lib/pkgconfig/libshift_${LIB_NAME}-d.pc ] && rm /usr/local/lib/pkgconfig/libshift_${LIB_NAME}-d.pc

    # if the last SHIFT module is being uninstalled, delete SHIFT installation folder
    [ -d ${INSTALL_PATH} ] && [ -z "$(ls -A ${INSTALL_PATH})" ] && rm -r ${INSTALL_PATH}

    # if the last SHIFT library is being uninstalled, delete SHIFT include folder
    [ -d /usr/local/include/shift ] && [ -z "$(ls -A /usr/local/include/shift)" ] && rm -r /usr/local/include/shift

    echo -e ${COLOR}
    echo "${1} was uninstalled from ${INSTALL_PATH}/${1}/"
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
