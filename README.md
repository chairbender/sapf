# sapf, cross-platform edition

this is a highly work-in-progress fork of James McCartney's [sapf](https://github.com/lfnoise/sapf) (Sound As Pure Form) which aims to implement cross-platform alternatives for the various macOS libraries used in the original codebase. for the time being, the top priority platform is Linux.

[original README](README.txt)

## building

a Nix flake is included. simply run:

```shell
nix develop
meson setup build
meson compile sapf -C build
```

and you should get a binary at `./build/sapf`.

It's important to specify `compile sapf`. Previously you could run `meson compile -C build`, but now we have multiple targets and not all of them are buildable on
a given system - the `sapf` target will build for your native system. You can substitute `sapf` for other targets such as `sapf_x86_64_v3` (check meson.build).

if not using Nix, you will need to install dependencies manually instead of the `nix develop`. the mandatory dependencies for a portable build are currently:

- libedit
- libsndfile
- fftw
- rtaudio

for installing dependencies, you can refer to the CI scripts in this repo:

- [install-debian-deps.sh](.github/scripts/install-debian-deps.sh) (Debian, Ubuntu, Mint, etc.)
- [install-macos-deps.sh](.github/scripts/install-macos-deps.sh) (macOS with Homebrew)

## running tests
Tests are written using doctest (which is obtained via a wrap).
See [the doctest documentation](https://github.com/doctest/doctest/tree/master?tab=readme-ov-file#documentation) for more details.

The test are inline with the production code. There is not a separate "test exe" - testing is done simply by building
the main exe with testing enabled, and running it (you can find the logic for this at the beginning of main()). Passing
`--exit` ensures it exists after testing. Otherwise, it will run the tests, and then run the main program like normal.
You can pass other doctest-specific command line arguments and they will be forwarded to doctest.

You can still use meson test to run the tests:
```shell
meson test unoptimized --print-errorlogs -C build
```
Without `--print-errorlogs` you'll need to view the testlog.txt to see the actual failing test cases.
The `unoptimized` tests the unoptimized version of the exe (which means it should build more quickly).
Susbtitute with `optimized` to test the optimized (-O3, -march=native) version of the exe.

Note there is currently a feature request for doctest for better integration 
with meson but it is not yet implemented ATTOW: https://github.com/doctest/doctest/issues/531

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
    # remove --buildtype debug to build without debug symbols
    meson setup --buildtype debug build 
    meson compile sapf -C build
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
Since it's not exactly straightforward to get everything working nicely in VSCode under Windows, here's 
some guidance.

