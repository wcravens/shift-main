[header]: # "To generate a html version of this document:"
[pandoc]: # "pandoc README.md -c Templates/github.css -o README.html -s --self-contained"

# SHIFT Developer Guide

## Build Tools

These are required in order to install all tools and libraries contained in this guide. 

### Ubuntu:

- In the Terminal: `sudo apt-get install autoconf build-essential`

### macOS:

- In the Terminal: `xcode-select --install`

- Install [Homebrew](http://brew.sh/) (APT-like environment for macOS):
    - In the Terminal: `/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

---

## Installation Instructions

When installing in each of the platforms, use the following commands:

### Ubuntu:

- In the Terminal: `sudo apt-get install [pkg]`

### macOS:

- In the Terminal: `brew install [formula]`

Additional instructions for each platform may be included at the end of each section.

---

## Required Libraries

| Library               | Ubuntu [pkg]                | macOS [formula]     |
| --------------------- | --------------------------- | ------------------- |
| Boost (C++ libraries) | `libboost-all-dev`          | `boost`             |
| C++ REST SDK          | `libcpprest-dev`            | `cpprestsdk`        |
| cURL                  | `libcurl4-openssl-dev`      | *pre-installed*     |
| OpenSSL               | `libssl-dev`                | `openssl`           |
| libxml2               | `libxml2-dev`               | *pre-installed*     |
| PostgreSQL            | `postgresql-server-dev-all` | `postgresql` **\*** |
| UUID                  | `uuid-dev`                  | *pre-installed*     |

**\*** This macOS formula installs both PostgreSQL and its libraries.

### macOS:

- In the Terminal:

``` bash
ln -s /usr/local/opt/postgresql/include /usr/local/include/postgresql
```

---

## Required Tools

| Tool              | Ubuntu [pkg]                    | macOS [formula]       |
| ----------------- | ------------------------------- | --------------------- |
| CMake             | `cmake`                         | `cmake`               |
| Doxygen           | `doxygen`                       | `doxygen --with-llvm` |
| Git               | `git`                           | `git`                 |
| Pandoc            | `pandoc`                        | `pandoc`              |
| pkg-config        | `pkg-config`                    | `pkg-config`          |
| PostgreSQL **\*** | `postgresql postgresql-contrib` | `postgresql`          |

**\*** Required only in the machines where a PostgreSQL instance is needed.

---

## Recommended Tools

| Tool    | Ubuntu [pkg]    | macOS [formula] |
| ------- | --------------- | --------------- |
| Nmap    | `nmap`          | `nmap`          |
| cURL    | `curl`          | *pre-installed* |
| pgAdmin | `pgadmin3`      | *use a .dmg*    |
| unzip   | `unzip`         | *pre-installed* |

---

## Visual Studio Code

[VSCode](https://code.visualstudio.com/) is the recommended development environment, since it works the same in every operating system, and integrates well with CMake. 

Once downloaded, open Visual Studio Code, and install the following extensions:

| Extension                      |
| ------------------------------ |
| `C/C++`                        |
| `CMake`                        |
| `CMake Tools`                  |
| `Git History` **\***           |
| `vscode-todo-highlight` **\*** |
| `vscode-icons` **\***          |

**\*** Recommended, not required.

When done, run the `vscode_init.sh` shell script located in the `Scripts` folder. This will initialize VSCode configuration files in all projects (so that one does not have to create them by themselves).

---

## PostgreSQL Configuration

In order to access the database from an external computer (e.g. connecting to the server via pgAdmin), refer to this [question](https://dba.stackexchange.com/questions/83984/connect-to-postgresql-server-fatal-no-pg-hba-conf-entry-for-host/) in Stack Overflow.

SHIFT connects to PostgreSQL with a user called "hanlonpgsql4". Follow the steps below, and create this user in the last step.

### Ubuntu:

- In the Terminal:

``` bash
sudo service postgresql start
sudo -u postgres psql -d postgres
CREATE EXTENSION adminpack; /* complete postgresql-contrib installation */
CREATE ROLE "<user>" WITH NOSUPERUSER CREATEDB INHERIT LOGIN ENCRYPTED PASSWORD '<password>';
\q
```

### macOS:

- In the Terminal:

``` bash
brew services start postgresql
psql -d postgres
CREATE ROLE "<user>" WITH NOSUPERUSER CREATEDB INHERIT LOGIN ENCRYPTED PASSWORD '<password>';
\q
```

---

## PostgreSQL Database Instances Creation

These are the database instances that are required per project:

| Project         | Database Name         | Database Owner  | Creation Script    | Extra Script       |
| --------------- | --------------------- | --------------- | ------------------ | ------------------ |
| DatafeedEngine  | HFSL_dbhft            | hanlonpgsql4    | `de_instances.sql` | *none*             |
| BrokerageCenter | shift_brokeragecenter | hanlonpgsql4    | `bc_instances.sql` | `user_entries.sql` |

If you correctly created the "hanlonpgsql4" user in the previous step, use the scripts contained in the table above (located in the `Scripts` folder) with the following commands:

### Ubuntu:

- In the Terminal: `sudo -u postgres psql -d postgres -f <script_name>.sql`

### macOS:

- In the Terminal: `psql -d postgres -f <script_name>.sql`

---

## QuickFIX Installation

### Ubuntu & macOS:

- Choose a location to keep the QuickFIX source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
git clone https://github.com/quickfix/quickfix.git
cd quickfix
mkdir build
cd build

# Ubuntu:
cmake .. -DHAVE_SSL=ON
# macOS:
cmake .. -DHAVE_SSL=ON -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl

make
sudo make install
```

- Further Documentation: <http://www.quickfixengine.org/quickfix/doc/html/> 

---

## SHIFT Installation

**Before installing any of the SHIFT servers, you should create proper configuration files.**

The default configuration files (pointing to localhost) can be copied by using the `copy_config.sh` script contained in the `Scripts` folder or by using the `-f` parameter in the installer. 

To install all servers and libraries:

- In the Terminal: `sudo ./install.sh`

To uninstall all servers and libraries:

- In the Terminal: `sudo ./install.sh -u`

For additional options:

- In the Terminal: `./install.sh -h`

When installing separate modules, required SHIFT libraries will be automatically installed.

A list of the aliases used by the installer is provided below.

| Alias | Project Name    | Project Type    | Binary/Library Name          | Prereqs |
| ----- | --------------- | --------------- | ---------------------------- | ------- |
| LM    | LibMiscUtils    | Shared Library  | `libshift_miscutils` **\***  | *none*  |
| DE    | DatafeedEngine  | Server (Binary) | `DatafeedEngine`             | LM      |
| ME    | MatchingEngine  | Server (Binary) | `MatchingEngine`             | *none*  |
| BC    | BrokerageCenter | Server (Binary) | `BrokerageCenter`            | LM      |
| LC    | LibCoreClient   | Shared Library  | `libshift_coreclient` **\*** | LM      |

By default, everything is installed under /usr/local/ (in the /usr/local/SHIFT/ folder), but the `-p` parameter can be used to change the default installation path prefix.

Regardless of the location chosen by the user to install SHIFT, symlinks will always be created in the following paths (and properly removed by the uninstaller if necessary).

| Type                 | Path                                   |
| -------------------- | -------------------------------------- |
| Binaries             | /usr/local/bin/                        |
| Configuration Files  | /usr/local/share/SHIFT/                |
| Libraries            | /usr/local/lib/                        |
| Library Header Files | /usr/local/include/shift/*libraryname* |

**\*** Library files with enabled debug symbols are also provided with the `-d` postfix.

---

## Starting and Stopping all Services

To start up all services:

- In the Terminal: `./startup.sh`

To terminate all services:

- In the Terminal: `./startup.sh -k`

For additional options:

- In the Terminal: `./startup.sh -h`

---

## Fixing the Shared Library Cache (Ubuntu 18.04 LTS Bionic Beaver)

When running our servers for the first time in Ubuntu, it is possible the system will not find our shared libraries. To fix such problem:

- In the Terminal: `sudo ldconfig`

---
