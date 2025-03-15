# sapf, cross-platform edition

this is a highly work-in-progress fork of James McCartney's [sapf](https://github.com/lfnoise/sapf) (Sound As Pure Form) which aims to implement cross-platform alternatives for the various macOS libraries used in the original codebase. for the time being, the top priority platform is Linux.

[original README](README.txt)

## building

a Nix flake is included. simply run:

```shell
nix develop
meson setup build
meson compile -C build
```

and you should get a binary at `./build/sapf`.

if not using Nix, you will need to install dependencies manually instead of the `nix develop`. the mandatory dependencies for a portable build are currently:

- libedit
- libsndfile
- fftw
- rtaudio

for installing dependencies, you can refer to the CI scripts in this repo:

- [install-debian-deps.sh](.github/scripts/install-debian-deps.sh) (Debian, Ubuntu, Mint, etc.)
- [install-macos-deps.sh](.github/scripts/install-macos-deps.sh) (macOS with Homebrew)

## Windows

Keep in mind this was written by someone fairly new to this toolchain and hence there
are things here that may be wrong or unnecessary and this is written with more for a beginner-oriented
perspective, partly for my own documentation and partly for others in a similar situation. Feel free to submit a PR and improve this if you know better!

Windows support is achieved by building via msys2 + MinGW64 as opposed to building natively on Windows (probably possible but probably much more annoying).

For the unfamiliar, msys2 is an app you can run that gives you a shell (with the root directory based in wherever you installed it).
Within this shell, Linux-based stuff actually works (with caveats). But we want to run the project on Windows. That's why we
use msys2 to then install MinGW64, which is itself ANOTHER shell you can open (it installs like a normal app just like msys2, just click it to open
a shell). When we compile the project via MinGW64, it produces an executable that can theoretically run on Windows (provided we've either
statically comipled in the needed libraries or provided them as DLLs alongside the .exe).

This project uses meson as the build tool. But meson doesn't actually automatically fetch dependencies, nor does it
actually perform the compilation. We need to install the dependencies via pacman (each dependency is basically
a collection of .lib and .h files), ensure meson "sees" them, and also tell meson to compile using clang (since by default it
uses gcc).

I believe the project requires clang so the instructions use that compiler. Note that clang installs gcc as a dependency,
so we need to always force meson to build with clang (you'll see how to do that below).

If you're for some reason not on x86_64, you'll have to replace any of the below references to that architecture
with your own! You can find and view info on packages on [msys2 packages](https://packages.msys2.org/queue) to see
if the package exists for your architecture. Note these packages often also have "-clang" versions, but AFAICT we
DON'T want to install those as they end up in a location that isn't found by default for this toolchain.

1. Install [msys2](https://www.msys2.org/). Make sure to keep track of where you installed it as this will be where your
"root directory" is for your msys2 / mingw64 shells. Files in here can be accessed in Windows just like normal, or accessed in the 
shell (so for example you can edit in VSCode and then compile in the mingw64 shell), just
be careful about not messing up the line endings when you edit on different OSes (different OSes use different line ending chars and it creates problems if you
mix them).
2. If you haven't yet, clone or copy this repo somewhere inside the msys2 install. For example within the msys2 shell you could install git via
`pacman -S git` and then git clone this repo into your "home" folder.
3. Open msys2 shell and install mingw64. PROTIP: Shift+Ins to paste, Ctrl+Ins to copy!
4.  ```shell
    pacman -S mingw-w64-x86_64-clang
    ```
5. Open mingw64 shell and do the rest of the commands there (not sure if you actually need to run them in mingw64 but that's what I did)
6. `pacman -S mingw-w64-x86_64-meson mingw-w64-x86_64-libsndfile mingw-w64-x86_64-fftw mingw-w64-x86_64-rtaudio mingw-w64-x86_64-readline`
7. Now we can try to build. We need to always pass --native-file which forces meson to use clang.
Still in the MinGW64 shell, navigate to the root directory of this repo.
TODO: Not sure you need to pass it to compile or just setup.
8. If you get "meson not found" or something like that, just close and reopen the mingw64 shell.
8.  ```shell
    meson setup --native-file clang.ini build 
    meson compile -C build
    ```
9. If you get "package blah not found", it means your PKG_CONFIG_PATH doesn't contain a "blah.pc" file. You can
confirm if pkg-config sees the lib via `pkg-config --modversion blah` (replacing blah with the name of the package). You
should see a version number printed. If not, you have to ensure you found the package. Check the msys2 page for the packages
you installed and check the "files" for a ".pc" file and note the path. Ensure that path is part of your PKG_CONFIG_PATH (`echo $PKG_CONFIG_PATH`).
10. When you actually compile, if you see errors, you'll have to search and resolve them. Some tips:
  - search on whatever the compiler claims is "missing". Usually you'll find what dependency is supposed to provide that.
  - Find the msys2 page for the dependency (assuming you installed it already) and check where it installs the .h files. 
  - Look at the path of the .h files and update the code to point to it instead of the old one (using a windows ifdef)
11. copy the needed dlls

VSCODE Setup
Copy and modify .vscode/c_cpp_properties.example.json to c_cpp_properties.json

TODO: not sure how this works, atm the only way I can get it to run is by copying the exe into C:\msys64\mingw64\bin and running it there.
If I copy the dlls manually then I get a bunch of missing symbol errors probably because that's not how its supposed to work.

TODO: relocate this stuff to PKM.


TODO: consider static compilation 

TODO: 
libedit dep isn't available for mingw64 directly via pacman. We can try
- readline mingw-w64-clang-x86_64-readline (also no editline struct - so doesn't have what we need)
- wineditline (doesn't seem to have all the needed stuff - no EditLine type)
- compile from source under mingw641

I think we need to include a different header, but not sure if we include wineditline's header or readline's headers

Compile from source
git clone git@github.com:cdesjardins/libedit.git
cd libedit
./configure
make
make install
(note where it installed)

Can't - termios.h not supported in minGW.

So instead, we need to actually port this stuff to NOT use editlib, and we need to use something else instead.

Also, glob.h is not supported - so we need to work around that: 
https://github.com/msys2/MSYS2-packages/issues/269
https://stackoverflow.com/questions/15052998/what-to-substitute-for-glob-t-and-glob-on-port-to-windows
It's a posix thing so it's just not there, we need to use a cross-platform alternative or specific mingw feature. Or disable 
that functionality for now

ctime_r
https://stackoverflow.com/questions/17085603/time-functions-in-mingw
https://stackoverflow.com/questions/19048012/implement-thread-safe-ctime-function
https://stackoverflow.com/questions/41621922/support-gmtime-r-timegm-functions-on-windows-mingw32
https://stackoverflow.com/questions/16647819/timegm-cross-platform

TODO: apparently for the time stuff we can actually get it back using a define?
see https://stackoverflow.com/questions/18551409/localtime-r-support-on-mingw


isascii/toascii
basically it wasn't meant to be an ifdef in THIS lib, but it IS an ifdef due to something in mingw. So we need to substitute it

When we disable libedit via USE_LIBEDIT, it gets less errors but we still need to solve those

TODO: remove wineditline / readline unless I will use them as the alternatives.
Port to linenoise-ng instead.

TODO: methodically fix / port all the ifdefs _WIN32 stuff to make it work on windows as opposed to disabling entire functionality

VSCode setup
https://stackoverflow.com/questions/49209996/vs-code-mingw-intellisence-not-working-for-c
https://code.visualstudio.com/docs/cpp/customize-default-settings-cpp

Linker issues
rtaudio
https://stackoverflow.com/questions/35130096/c-undefined-reference-to-defined-constant


TODO: not sure if host_machine section needed in clang.ini
