version: @VERSION@.{build}

branches:
  except:
    - gh-pages

clone_folder: c:\projects\cdogs-sdl
image:
- Visual Studio 2019
configuration:
- Release
matrix:
  fast_finish: true
environment:
  CTEST_OUTPUT_ON_FAILURE: 1
  SDL2_VERSION: 2.0.10
  SDL2_IMAGE_VERSION: 2.0.5
  SDL2_MIXER_VERSION: 2.0.4
  SDLDIR: C:\projects\cdogs-sdl
  VERSION: @VERSION@

install:
  - IF NOT EXIST %APPVEYOR_BUILD_FOLDER%\SDL2-devel-%SDL2_VERSION%-VC.tar.gz appveyor DownloadFile http://libsdl.org/release/SDL2-devel-%SDL2_VERSION%-VC.zip
  - 7z x SDL2-devel-%SDL2_VERSION%-VC.zip -oC:\
  - xcopy C:\SDL2-%SDL2_VERSION%\* %SDLDIR%\ /S /Y
  - IF NOT EXIST %APPVEYOR_BUILD_FOLDER%\SDL2_mixer-devel-%SDL2_MIXER_VERSION%-VC.zip appveyor DownloadFile https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-%SDL2_MIXER_VERSION%-VC.zip
  - 7z x SDL2_mixer-devel-%SDL2_MIXER_VERSION%-VC.zip -oC:\
  - xcopy C:\SDL2_mixer-%SDL2_MIXER_VERSION%\* %SDLDIR%\ /S /Y
  - IF NOT EXIST %APPVEYOR_BUILD_FOLDER%\SDL2_image-devel-%SDL2_IMAGE_VERSION%-VC.zip appveyor DownloadFile https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-%SDL2_IMAGE_VERSION%-VC.zip
  - 7z x SDL2_image-devel-%SDL2_IMAGE_VERSION%-VC.zip -oC:\
  - xcopy C:\SDL2_image-%SDL2_IMAGE_VERSION%\* %SDLDIR%\ /S /Y
  - xcopy %SDLDIR%\lib\x86\* %SDLDIR%\lib\ /S

before_build:
  - .\build\windows\get-sdl2-dlls.bat dll "appveyor DownloadFile"
  - cmake -DCMAKE_PREFIX_PATH="%SDLDIR%" -G "Visual Studio 16 2019" -A Win32 .

build:
  project: c:\projects\cdogs-sdl\cdogs-sdl.sln
  verbosity: minimal
  parallel: true

after_build:
  - msbuild c:\projects\cdogs-sdl\src\PACKAGE.vcxproj
  - dir

cache:
- SDL2-devel-%SDL2_VERSION%-VC.zip
- SDL2_image-devel-%SDL2_IMAGE_VERSION%-VC.zip
- SDL2_mixer-devel-%SDL2_MIXER_VERSION%-VC.zip
- dir\SDL2-%SDL2_VERSION%-win32-x86.zip
- dir\SDL2_image-%SDL2_IMAGE_VERSION%-win32-x86.zip
- dir\SDL2_mixer-%SDL2_MIXER_VERSION%-win32-x86.zip

artifacts:
  - path: /.\C-Dogs*.exe/
  - path: /.\C-Dogs*.zip/

deploy:
  provider: GitHub
  description: ''
  auth_token:
    secure: Ap9XXGG/1cyV9hpat7EaYxZHBt/VWdH82Bx+nIugdJ1Dh3I9e8OP4L/IXkadUjdR
  prerelease: false
  force_update: true 	#to be in piece with Travis CI
  on:
    appveyor_repo_tag: true

after_deploy:
  - .\build\appveyor\butler.bat
