project(
  'sapf',
  ['cpp'],
  meson_version : '>=1.1'
)

cpp = meson.get_compiler('cpp')
sources = [
  'src/AudioToolboxBuffers.cpp',
  'src/AudioToolboxSoundFile.cpp',
  'src/CoreOps.cpp',
  'src/DelayUGens.cpp',
  'src/dsp.cpp',
  'src/elapsedTime.cpp',
  'src/ErrorCodes.cpp',
  'src/FilterUGens.cpp',
  'src/main.cpp',
  'src/MathFuns.cpp',
  'src/MathOps.cpp',
  'src/Midi.cpp',
  'src/MultichannelExpansion.cpp',
  'src/Object.cpp',
  'src/Opcode.cpp',
  'src/OscilUGens.cpp',
  'src/Parser.cpp',
  'src/Play.cpp',
  'src/PortableBuffers.cpp',
  'src/primes.cpp',
  'src/RCObj.cpp',
  'src/RandomOps.cpp',
  'src/SetOps.cpp',
  'src/SndfileSoundFile.cpp',
  'src/SoundFiles.cpp',
  'src/Spectrogram.cpp',
  'src/StreamOps.cpp',
  'src/symbol.cpp',
  'src/Types.cpp',
  'src/UGen.cpp',
  'src/VM.cpp',
]
deps = []
cpp_args = ['-std=c++17']
link_args = []

# needed even when not building a test exe because tests are inline with production code
doctest_dep = dependency('doctest', required: true)
deps += doctest_dep

# libedit
if host_machine.system() == 'windows'
    deps += dependency('readline', required: true)
    deps += dependency('history', required: true)
    cpp_args += ['-DUSE_LIBEDIT=0', '-D_POSIX_THREAD_SAFE_FUNCTIONS']
else
  deps += dependency('libedit', required: true)
    cpp_args += ['-DUSE_LIBEDIT=1']
endif

if get_option('asan')
  cpp_args += ['-fsanitize=address', '-O1', '-fno-omit-frame-pointer', '-g']
  link_args += ['-fsanitize=address']
endif

if get_option('accelerate')
  add_project_arguments('-DSAPF_ACCELERATE', language: 'cpp')
else
  deps += dependency('fftw3', required: true, version: '>=3')
  deps += dependency('eigen3', required: true)
endif

if get_option('apple_lock')
  add_project_arguments('-DSAPF_APPLE_LOCK', language: 'cpp')
endif

if get_option('audiotoolbox')
  add_project_arguments('-DSAPF_AUDIOTOOLBOX', language: 'cpp')
else
  deps += dependency('rtaudio', required: true, version: '>=5.2')
  if not cpp.has_header('RtAudio.h') and cpp.has_header('rtaudio/RtAudio.h')
    add_project_arguments('-DSAPF_RTAUDIO_H=<rtaudio/RtAudio.h>', language: 'cpp')
  endif

  deps += dependency('sndfile', required: true)
endif

if get_option('carbon')
  add_project_arguments('-DSAPF_CARBON', language: 'cpp')
endif

if get_option('cocoa')
  add_project_arguments('-DSAPF_COCOA', language: 'cpp')
  sources += 'src/makeImage.mm'
  add_languages('objcpp', required : true)
else
  sources += 'src/makeImage.cpp'
endif

if get_option('corefoundation')
  add_project_arguments('-DSAPF_COREFOUNDATION', language: 'cpp')
endif

if get_option('coremidi')
  add_project_arguments('-DSAPF_COREMIDI', language: 'cpp')
endif

if get_option('dispatch')
  cpp_args += ['-fblocks']
  link_args += ['-lBlocksRuntime', '-ldispatch']
  add_project_arguments('-DSAPF_DISPATCH', language: 'cpp')
endif

if get_option('mach_time')
  add_project_arguments('-DSAPF_MACH_TIME', language: 'cpp')
endif

if get_option('manta')
  add_project_arguments('-DSAPF_MANTA', language: 'cpp')
endif



# testing is disabled in the release exe
release_cpp_args = cpp_args + ['-DOCTEST_CONFIG_DISABLE', '-O3']

# build targeting the native system this was built on, with optimizations (this is what people 
# building on their own machine should use for optimal performance)
executable(
  'sapf',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=native'],
  link_args : link_args
)

# The below builds target specific architectures. Because this program uses
# SIMD, the best performance is achieved when a user uses the most recent architecture supported
# by their CPU. Users will get the best results if they build the library themselves however (exe_native).

# should have maximum compatibility with ancient x86_64 cpus
executable(
  'sapf_x86_64',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=x86-64'],
  link_args : link_args
)

# for somewhat older x86 CPUs (2008ish)
executable(
  'sapf_x86_64_v2',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=x86-64-v2'],
  link_args : link_args
)

# for more cutting edge x86 cpus
executable(
  'sapf_x86_64_v3',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=x86-64-v3'],
  link_args : link_args
)

# support M1 chip and newer
executable(
  'sapf_arm_m1',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=armv8.4-a', '-mcpu=apple-m1'],
  link_args : link_args
)

# support M3 chip
executable(
  'sapf_arm_m3',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : release_cpp_args + ['-march=armv8.5-a', '-mcpu=apple-m3'],
  link_args : link_args
)

# exe with testing enabled without optimizations
testable_exe_native_unoptimized = executable(
  'sapf_unoptimized_testable',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : cpp_args,
  link_args : link_args
)

# exe with testing enabled with all optimizations for the native system
testable_exe_native_optimized = executable(
  'sapf_optimized_testable',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : cpp_args + ['-O3', '-march=native'],
  link_args : link_args
)

# tests for different optimization levels
test('unoptimized', testable_exe_native_unoptimized, args: ['--exit'])
test('optimized', testable_exe_native_optimized, args: ['--exit'])