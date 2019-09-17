[header]: # "To generate a html version of this document:"
[pandoc]: # "pandoc README.md -c ../Templates/github.css -o README.html -s --self-contained"

# SHIFT QtClient Guide

## Ubuntu Deployment

- Before installation, make sure the package manager is updated, and basic build tools are installed:

``` bash
sudo apt-get update
sudo apt-get install autoconf build-essential
```

### STEP 1 - Build Tools

#### Install the following manually:

##### LibQt + Qt Creator:

- Download the [Qt Open Source](https://www.qt.io/download) online installer.
- Run the downloaded installer, and skip login when prompted:
    - Installing using the command line is not recommended since it's hard to control the version of Qt to be installed.
- In `Select Components`, check `Qt5.9.6` > `Desktop gcc 64-bit` under the `Qt` section.
- Follow the remaining instructions until Qt is fully installed.

#### Install the following using apt-get:

| Tool         | Package  |
| ------------ | -------- |
| CMake        | cmake    |
| Git          | git      |

- Lazy installation command using `apt-get` in one line:

``` bash
sudo apt-get install cmake git
```

### STEP 2 - Required Libraries

#### Install the following using apt-get:

| Library               | Package                | Required By   |
| --------------------- | ---------------------- | ------------- |
| Boost (C++ libraries) | `libboost-all-dev`     | LibMiscUtils  |
| C++ REST SDK          | `libcpprest-dev`       | LibMiscUtils  |
| cURL                  | `libcurl4-openssl-dev` | LibCoreClient |
| Mesa                  | `libgl1-mesa-dev`      | QtClient      |
| OpenSSL               | `libssl-dev`           | LibMiscUtils  |
| UUID                  | `uuid-dev`             | LibMiscUtils  |

- Lazy installation command using `apt-get` in one line:

``` bash
sudo apt-get install libboost-all-dev libcpprest-dev libcurl4-openssl-dev libgl1-mesa-dev libssl-dev uuid-dev
```

#### Install the following manually:

##### QuickFIX

- Choose a location to keep the QuickFIX source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
curl -LO http://prdownloads.sourceforge.net/quickfix/quickfix-1.15.1.tar.gz
tar -zxf quickfix-1.15.1.tar.gz
cd quickfix
./bootstrap
./configure --prefix=/usr/local/
make -j$(nproc)
sudo make install
cd ..
rm quickfix-1.15.1.tar.gz
```

##### QWT

- Choose a location to keep the QWT source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
curl -LO http://prdownloads.sourceforge.net/qwt/qwt-6.1.3.zip
unzip -a qwt-6.1.3.zip
cd qwt-6.1.3
~/Qt/5.9.6/gcc_64/bin/qmake qwt.pro
make -j$(nproc)
sudo make install
cd ..
rm qwt-6.1.3.zip
```

##### SHIFT Miscellaneous Utils & Core Client

Use the installer in the root folder of the SHIFT project:

- In the Terminal: `sudo ./install -m LM LC`

### STEP 3 - Compilation

- Open the `QtClient.pro` file in Qt Creator, and follow the instructions to configure the environment.
- **IMPORTANT**: Add `make install` to the build steps in Qt Creator:
     - In the side navigation bar click `Projects` > `Build & Run` > `Build Settings` > `Build Steps` > `Make`:
          - In `Make arguments`, enter `install`.
- Click the green play button to run.

---

## macOS Deployment

### STEP 1 - Build Tools

#### Install the following manually:

##### Homebrew:

- In the Terminal: `xcode-select --install`

- Install [Homebrew](http://brew.sh/) (APT-like environment for macOS):
    - In the Terminal: `/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

##### LibQt + Qt Creator:

- Download the [Qt Open Source](https://www.qt.io/download) online installer.
- Run the downloaded installer, and skip login when prompted. 
- The installer will complain that Xcode is not installed:
    - Keep clicking "OK" until the message goes away permanently.
- In `Select Components`, check `Qt5.9.6` > `macOS` under the `Qt` section.
- Follow the remaining instructions until Qt is fully installed, but **do not** launch Qt Creator when prompted.
- Launch Qt Creator for the first time from the Terminal with the following command:

``` bash
PATH="$(xcode-select -p)/usr/bin:${PATH}" ~/Qt/Qt\ Creator.app/Contents/MacOS/Qt\ Creator
```

- Close Qt Creator:
    - You can launch Qt Creator the normal way in future (e.g. via Spotlight).
- Optionally, create a symbolic link to Qt Creator so that it can be launched from the Applications folder:

``` bash
ln -s ~/Qt/Qt\ Creator.app /Applications/Qt\ Creator.app
```

#### Install the following using brew:

| Tool         | Formula  |
| ------------ | -------- |
| CMake        | cmake    |
| Git          | git      |

- Lazy installation command using `brew` in one line:

``` bash
brew install cmake git
```

### STEP 2 - Required Libraries

#### Install the following using brew:

| Library               | Formula      | Required By  |
| --------------------- | ------------ | ------------ |
| Boost (C++ libraries) | `boost`      | LibMiscUtils |
| C++ REST SDK          | `cpprestsdk` | LibMiscUtils |
| OpenSSL               | `openssl`    | LibMiscUtils |

- Lazy installation command using `brew` in one line:

``` bash
brew install boost cpprestsdk openssl
```

#### Install the following manually:

##### QuickFIX

- Choose a location to keep the QuickFIX source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
curl -LO http://prdownloads.sourceforge.net/quickfix/quickfix-1.15.1.tar.gz
tar -zxf quickfix-1.15.1.tar.gz
cd quickfix
./bootstrap
./configure --prefix=/usr/local/
make -j$(sysctl -n hw.physicalcpu)
sudo make install
cd ..
rm quickfix-1.15.1.tar.gz
```

##### QWT

- Choose a location to keep the QWT source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
curl -LO http://prdownloads.sourceforge.net/qwt/qwt-6.1.3.zip
unzip -a qwt-6.1.3.zip
cd qwt-6.1.3
~/Qt/5.9.6/clang_64/bin/qmake qwt.pro
make -j$(sysctl -n hw.physicalcpu)
sudo make install
sudo ln -s /usr/local/qwt-6.1.3/lib/qwt.framework /Library/Frameworks/qwt.framework
cd ..
rm qwt-6.1.3.zip
```

##### SHIFT Miscellaneous Utils & Core Client

Use the installer in the root folder of the SHIFT project:

- In the Terminal: `sudo ./install -m LM LC`

### STEP 3 - Compilation

- Open the `QtClient.pro` file in Qt Creator, and follow the instructions to configure the environment.
- **IMPORTANT**: Add `make install` to the build steps in Qt Creator:
     - In the side navigation bar click `Projects` > `Build & Run` > `Build Settings` > `Build Steps` > `Make`:
          - In `Make arguments`, enter `install`.
- Click the green play button to run.

---

## Windows Deployment

### STEP 1 - Build Tools

These are required in order to install all libraries contained in this guide.

#### CMake:

- Download the last available version of [CMake](https://cmake.org/) (3.11.2 at the time of this writting), and install it.
- Remember to check "Add cmake to system path" during the installation.

#### Git:

- Download the last available version of Open the QtClient.pro file in Qt Creator and start building.
- [Git](https://git-scm.com/) (2.17.1 at the time of this writting), and install it. 

#### Microsoft Visual Studio 2017:

- Download the lastest available version of [Visual Studio 2017 Community Edition](<https://www.visualstudio.com/), and install it.
- During the installation process, do not forget to select the `Desktop development with C++` workload, with the `Windows Driver Kit 10.0` component. **\***

**\*** The `Windows Driver Kit 10.0` may also be downloaded separately from <https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/>.

#### LibQt + Qt Creator:

- Go to <https://www.qt.io/download/>, select `Go Open Source` then click `Accept`. In the following page, select `Download`.
- Run the downloaded installer, and skip login when prompted. 
- In `Select Components`, check `MSVC 2017 64-bit` and `Qt Creator 4.5.0 CDB Debugger Support` under the `Tools` section.
- Follow the remaining instructions until Qt is fully installed.

### STEP 2 - Required Libraries

#### cURL:

- Download version 7.58.0 of the cURL library from <https://curl.haxx.se/download/> and unzip it in `C:\`.
- Open a new Visual Studio 2017 shell (`x64 Native Tools Command Prompt for VS 2017`).
- If it does not display the message "[vcvarsall.bat] Environment initialized for: 'x64'", run the following before continuing:

``` bash
cd "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build"
vcvars64.bat
```

- The following commands will build the cURL project and create both debug and release versions of the cURL library.

``` bash
cd "C:\curl-7.58.0\winbuild"
nmake /f Makefile.vc mode=static VC=14
nmake /f Makefile.vc mode=static VC=14 debug=yes
```

#### QWT:

- Download the last available version of [QWT](https://sourceforge.net/projects/qwt/files/) (6.1.3 at the time of this writting), and unzip it.
- Open a new Qt shell (`Qt 5.10.0 64-bit for Desktop (MSVC 2017)`), and do the following:

``` bash
cd "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build"
vcvars64.bat
cd <folder containing QWT unzipped files>
qmake qwt.pro
nmake
nmake install
```

#### QuickFIX:

- Download the last available version of [QuickFIX](http://www.quickfixengine.org/) (1.15.1 at the time of this writting), and unzip it in `C:\`.
- Open the Visual C++ solution `C:\quickfix\quickfix_vs15.sln` in Visual Studio 2017.
- In `Configuration Manager`, select `<New...>`, and add a `x64` configuration which copies the settings from `Win32`.
- Right click on the `quickfix` project from the list on the Solution Explorer.
- Build both Debug and Release as many times as needed, until you see the following output in both Debug and Release configurations:

```
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
```

#### LibMiscUtil:

- Open `Git Bash`, and do the following:

``` bash
cd <path_to_cloned_repository>
cd LibMiscUtils
./win-build.sh
```

#### LibCoreClient:

- Open `Git Bash`, and do the following: **\**
- Depending on which version of and the location where the cURL library was installed, the LibCoreClient CMakeLists.txt file may need to be updated before a successful installation.

``` bash
cd <path_to_cloned_repository>
cd LibCoreClient
./win-build.sh
```

### STEP 3 - System Environment Variables

- In Windows 7: Control Panel > System and Security > System > Advanced system settings > Environment Variables
- Create the following user environment variables in your system: 

| Variable        | Value                                |
| --------------- | ------------------------------------ |
| CURL_LIB        | `C:\<folder containing curl>\builds` |
| QWT_ROOT        | `C:\<folder containing qwt>`         |
| QuickFIX_VS2017 | `C:\<folder of quickfix>`            |

### STEP 4 - Compilation

- Open the `QtClient.pro` file in Qt Creator and start building.
- When debugging, if Qt Creator complains about the selected debugger, try other installed debuggers or use a long-term-support version of Qt.

### STEP 5 - Deployment

- Build the project in QtCreator in Release mode.
- Open a new Qt shell (`Qt 5.10.0 64-bit for Desktop (MSVC 2017)`), and do the following:

``` bash
cd "C:\Qt\Qt5.10.0\5.10\msvc2017\bin"
windeployqt.exe <folder containing the release version of the executable file>
```

- Create a new folder.
- Go to the release folder, and copy the `platforms` folder and the executable file to the new folder.
- Copy the following .dll files into the new folder as well:

```
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\D3Dcompiler_47.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\libEGL.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\libGLESV2.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\opengl32sw.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5Core.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5Gui.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5OpenGL.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5PrintSupport.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5Svg.dll
C:\Qt\Qt5.10.0\5.10\msvc2017\bin\Qt5Widgets.dll
C:\qwt-6.1.3\lib\qwt.dll
```

---
