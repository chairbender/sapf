# sapf, cross-platform edition

this is a highly work-in-progress fork of James McCartney's [sapf](https://github.com/lfnoise/sapf) (Sound As Pure Form) which aims to implement cross-platform alternatives for the various macOS libraries used in the original codebase. for the time being, the top priority platform is Linux.

[original README](README.txt)

## building

a Nix flake is included. simply run:

```shell
nix develop
meson setup --buildtype release build
meson compile -C build
```

and you should get a binary at `./build/sapf`. This will be optimized for your
native architecture and has the maximum level of optimization. This is recommended for
normal usage. 

To build an
unoptimized binary, simply omit the `--buildtype release` param (which will default to
`--buildtype debug`). If you've already ran `setup` and want to change the buildtype to `debug`, you can run
```shell
meson configure --buildtype debug build
meson compile -C build
```
Note you can view the current buildtype setting via `meson configure build`.

You can specify different targets defined in the meson.build file such as `meson compile sapf_x86_64_v3 -C build`.

if not using Nix, you will need to install dependencies manually instead of the `nix develop`. the mandatory dependencies for a portable build are currently:

- libedit
- libsndfile
- fftw
- rtaudio

for installing dependencies, you can refer to the CI scripts in this repo:

- [install-debian-deps.sh](.github/scripts/install-debian-deps.sh) (Debian, Ubuntu, Mint, etc.)
- [install-macos-deps.sh](.github/scripts/install-macos-deps.sh) (macOS with Homebrew)

## running tests
Tests are written using doctest (which is obtained via a wrap) and located in the `tests` folder.
See [the doctest documentation](https://github.com/doctest/doctest/tree/master?tab=readme-ov-file#documentation) for more details.

You can use meson test to run the tests.

This will respect the `--buildtype` setting. For optimal debugging experience,
you should use the `debug` buildtype. You can check the current buildtype with
`meson configure build`. To change it, you can run `meson configure --buildtype debug build`.

The below will run the tests
```shell
meson test --verbose -C build
```
Without `--verbose` you won't get the doctest test report (not to be confused with meson's
own, less useful test report) printed to stdout and instead would have to view the test
log file.

Note there is currently a feature request for doctest for better integration 
with meson but it is not yet implemented ATTOW: https://github.com/doctest/doctest/issues/531
This seems to be why the default meson test report isn't that useful.

## Windows Usage Caveats

Windows support is currently WIP. The following current "quirks" apply:

- It will default to WASAPI and use your primary output device. Ability to select different devices or audio drivers (ASIO, etc..) is not yet supported.
- When pasting in multiline strings, they should work. Just note that many built-in Windows terminals by default will strip out certain characters like tabs,
    ruining your beautiful formatting. This can be changed in the terminal's settings.

## Windows Build + Development

Windows support is achieved by building via [msys2](https://www.msys2.org/) (using mingw-w64-ucrt6) as opposed to building natively on Windows (probably possible but probably much more annoying).

If you're for some reason not on x86_64, you'll have to replace any of the below references to that architecture
with your own! You can find and view info on packages on [msys2 packages](https://packages.msys2.org/queue) to see
if the package exists for your architecture. 

1. Install [msys2](https://www.msys2.org/). Make sure to keep track of where you installed it as this will be where your
"root directory" is for your msys2 / mingw64 shells. For this guide we will assume the default of `C:\msys64`
2. If you haven't yet, clone or copy this repo somewhere inside the msys2 install. For example within the msys2 shell you could install git via
`pacman -S git` and then git clone this repo into your "home" folder.
3. Open a msys2 (ucrt) shell and install some needed development dependencies.
    (Note the ca-certificates are required in order to download the gtest wrap, and in general you'll
    have a bad time doing anything on msys2 without these certs.)
   ```shell
   # press ENTER when prompted to choose "all"
   pacman -S --needed base-devel \
    mingw-w64-ucrt-x86_64-toolchain
   pacman -S \
    mingw-w64-ucrt-x86_64-meson \
    mingw-w64-ucrt-x86_64-fftw \
    mingw-w64-ucrt-x86_64-libsndfile \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-rtaudio \
    mingw-w64-ucrt-x86_64-readline \
    mingw-w64-ucrt-x86_64-ca-certificates
   ```
4. Close and reopen the shell to ensure it loads everything you just installed. 
5. Now we can try to build in the msys2 (ucrt) shell.
Navigate to the root directory of this repo.
6.  ```shell
    # remove --buildtype release to build an unoptimized exe with debug symbols
    meson setup --buildtype release build 
    meson compile -C build
    ```
7. You should see `sapf.exe` is created under the `${workspaceFolder}/build` directory.
8. You need all the required DLLs in order to run it via Windows. Go to your msys2 folder `C:\msys64\ucrt64\bin`
and copy all of the dlls that look like `lib*.dll` (i.e. libreadline8.dll, libogg-0.dll, etc...). This is
more than needed but I'm not sure the exact subset of dlls needed yet.
9. Now you can run the exe directly by clicking or via your preferred command prompt.
10. Test if its all working with a simple command (you should hear audio out of your primary output device)
`15 .0 sinosc 200 * 300 + .0 sinosc .1 * play`

When setting up your IDE, make sure it's using the ucrt64 libraries (C:\msys64\ucrt64\include)
and binaries (C:\msys64\ucrt64\bin) for compilation / linking and NOT your native windows libraries / binaries.

See README_VSCODE.md for vscode-specific setup.