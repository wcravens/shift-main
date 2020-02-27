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

# delete previous build folders before installing
CLEAN_INSTALL=0

# force the copy of default configuration files
FORCE_INSTALL=0

function usage
{
    echo -e ${COLOR}
    echo "OVERVIEW: Stevens High Frequency Trading (SHIFT) Simulation System Web Client Installation Script"
    echo
    echo "USAGE: sudo ./install.sh [options] <args>"
    echo
    echo "OPTIONS:";
    echo "  -c [ --clean ]                     Clean install"
    echo "                                     (delete previous build folders)"
    echo "  -f [ --force ]                     Use default configuration files"
    echo "                                     (not recommended)"
    echo "  -h [ --help ]                      Display available options"
    echo "  -p [ --path ] arg                  Set (un)installation path prefix"
    echo "                                     (default: /usr/local)"
    echo "  -u [ --uninstall ]                 Uninstall"
    echo -e ${NO_COLOR}
}

function installWebClient
{
    echo -e ${COLOR}
    echo "Installing WebClient backend..."
    echo -e ${NO_COLOR}

    # create ~/.shift directory if it does not exist yet 
    [ -d ~/.shift/WebClient ] || mkdir -p ~/.shift/WebClient

    # save installation path to install.log (erasing previous version)
    [ -f ~/.shift/WebClient/install.log ] && rm ~/.shift/WebClient/install.log
    echo "INSTALL_PREFIX=${INSTALL_PREFIX}" > ~/.shift/WebClient/install.log
    
    # give ownership of the ~/.shift directory to the current user
    chown -R ${CURRENT_USER}:${CURRENT_USER} ~/.shift

    if [ ${CLEAN_INSTALL} -ne 0 ]
    then
        [ -d backend/build ] && rm -r backend/build
    fi

    if [ ${FORCE_INSTALL} -ne 0 ]
    then
        echo -ne ${COLOR_WARNING}
        echo "Copying WebClient default configuration files!"
        echo -e ${NO_COLOR}
        [ -d config ] && rm -r config
        cp -r config.default config
        chown -R ${CURRENT_USER}:${CURRENT_USER} config
    fi

    # build & install
    cmake -Hbackend -Bbackend/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
    if ( ! cmake --build backend/build --target install -- -j${CORE_NUM} )
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} failed to install WebClient: configuration files are probably missing (please see usage with -h or --help)"
        echo
        exit 1
    fi

    # give ownership of the build directory to the current user
    chown -R ${CURRENT_USER}:${CURRENT_USER} backend/build

    # frontend composer
    [ -d frontend/vendor ] && rm -r frontend/vendor
    composer install --quiet --working-dir=frontend
    composer require --quiet --working-dir=frontend vlucas/phpdotenv
    chown -R ${CURRENT_USER}:${CURRENT_USER} frontend/vendor
    chown -R ${CURRENT_USER}:${CURRENT_USER} ~/.composer

    # frontend data folder
    [ -d frontend/public/data ] && rm -r frontend/public/data
    mkdir -p frontend/public/data
    chown -R ${CURRENT_USER}:${CURRENT_USER} frontend/public/data
}

function uninstallWebClient
{
    echo -e ${COLOR}
    echo -n "Uninstalling WebClient..."
    echo -e ${NO_COLOR}

    # if installation did not use default installation path
    if [ -f ~/.shift/WebClient/install.log ]
    then
        source ~/.shift/WebClient/install.log
        rm ~/.shift/WebClient/install.log
    fi

     # if installation is not found
    if [ ! -f ${INSTALL_PREFIX}/bin/WebClient ]
    then
        echo
        echo -e "shift: ${COLOR_ERROR}error:${NO_COLOR} nothing to do for WebClient: binary not found"
        echo
        return
    fi

    # remove installation
    [ -f /usr/local/bin/WebClient ] && rm /usr/local/bin/WebClient
    [ -d /usr/local/share/shift/WebClient ] && rm -r /usr/local/share/shift/WebClient

    # if the last SHIFT module is being uninstalled, delete SHIFT configuration folder
    [ -d /usr/local/share/shift ] && [ -z "$(ls -A /usr/local/share/shift)" ] && rm -r /usr/local/share/shift

    # frontend composer
    [ -d frontend/vendor ] && rm -r frontend/vendor

    # frontend data folder
    [ -d frontend/public/data ] && rm -r frontend/public/data

    echo -e ${COLOR}
    echo "WebClient was uninstalled from ${INSTALL_PREFIX}"
    echo -e ${NO_COLOR}
}

UNINSTALL_FLAG=0

while [ "${1}" != "" ]; do
    case ${1} in
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

if [ ${UNINSTALL_FLAG} -eq 0 ]
then
    installWebClient
else
    uninstallWebClient
fi
