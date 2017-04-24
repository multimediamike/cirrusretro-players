#!/usr/bin/python

# Part of the the Cirrus Retro project
# This Python script generates the build specification for the ninja
# build system.

print "Creating the ninja build file for the Cirrus Retro players..."

from ninja_syntax import Writer

GME_TYPES = [
  "ay",
  "gbs",
  "gym",
  "hes",
  "kss",
  "nsf",
  "sap",
  "spc",
  "vgm"
]

XZ_EMBEDDED_C_SOURCES = [
  "xz_crc32.c",
  "xz_dec_lzma2.c",
  "xz_dec_stream.c"
]

GME_CPP_SOURCES = [
  "Ay_Apu.cpp",
  "Ay_Cpu.cpp",
  "Ay_Emu.cpp",
  "Blip_Buffer.cpp",
  "Classic_Emu.cpp",
  "Data_Reader.cpp",
  "Dual_Resampler.cpp",
  "Effects_Buffer.cpp",
  "Fir_Resampler.cpp",
  "Gb_Apu.cpp",
  "Gb_Cpu.cpp",
  "Gb_Oscs.cpp",
  "Gbs_Emu.cpp",
  "gme.cpp",
  "Gme_File.cpp",
  "Gym_Emu.cpp",
  "Hes_Apu.cpp",
  "Hes_Cpu.cpp",
  "Hes_Emu.cpp",
  "Kss_Cpu.cpp",
  "Kss_Emu.cpp",
  "Kss_Scc_Apu.cpp",
  "M3u_Playlist.cpp",
  "Multi_Buffer.cpp",
  "Music_Emu.cpp",
  "Nes_Apu.cpp",
  "Nes_Cpu.cpp",
  "Nes_Fme7_Apu.cpp",
  "Nes_Namco_Apu.cpp",
  "Nes_Oscs.cpp",
  "Nes_Vrc6_Apu.cpp",
  "Nsfe_Emu.cpp",
  "Nsf_Emu.cpp",
  "Sap_Apu.cpp",
  "Sap_Cpu.cpp",
  "Sap_Emu.cpp",
  "Sms_Apu.cpp",
  "Snes_Spc.cpp",
  "Spc_Cpu.cpp",
  "Spc_Dsp.cpp",
  "Spc_Emu.cpp",
  "Spc_Filter.cpp",
  "Vgm_Emu.cpp",
  "Vgm_Emu_Impl.cpp",
  "Ym2413_Emu.cpp",
  "Ym2612_Emu.cpp"
]

buildfile = open("build.ninja", "w")

n = Writer(buildfile)

# variable declarations
n.comment("variable declarations")
n.variable("CC", "emcc")
n.variable("CXX", "em++")
n.newline()
n.variable("ROOT", ".")
n.variable("XZ_ROOT", "$ROOT/xz-embedded")
n.variable("GME_ROOT", "$ROOT/gme-source-0.6.0")
n.variable("OBJECTS", "$ROOT/objects")
n.variable("FINAL_DIR", "$ROOT/final")
n.newline()
n.variable("EXPORT_LIST", "\"['_crPlayerContextSize', '_crPlayerInitialize', '_crPlayerLoadFile', '_crPlayerSetTrack', '_crPlayerGenerateStereoFrames', '_crPlayerVoicesCanBeToggled', '_crPlayerGetVoiceCount', '_crPlayerGetVoiceName', '_crPlayerSetVoiceState', '_crPlayerCleanup', '_main']\"")
n.newline()

# build rules
n.comment("build rules")
n.rule("XZ_CC",
  command="$CC -Wall -Os -I $XZ_ROOT -MMD -MT $out -MF $out.d -o $out -c $in",
  description="XZ_CC $out",
  depfile="$out.d")
n.newline()

for gme_type in GME_TYPES:
  n.rule("GME_" + gme_type + "_CXX",
    command="$CXX -Wall -Os -I $GME_ROOT -DHAVE_CONFIG_H -I $GME_ROOT/config-headers/" + gme_type + " -MMD -MT $out -MF $out.d -o $out -c $in",
    description="GME_CXX $out",
    depfile="$out.d")
  n.newline()

n.rule("LINK_GME",
  command="$CXX -o $out $in --memory-init-file 0 -Wall -Os -s EXPORTED_FUNCTIONS=$EXPORT_LIST -s TOTAL_MEMORY=50000000",
  description="LINK $out")
n.newline()

n.rule("COMPRESS",
  command="zopfli -c --i25 $in > $out",
  description="COMPRESS $out")
n.newline()

# build the Embedded XZ library
n.comment("build the Embedded XZ library")
xz_object_list = []
for src in XZ_EMBEDDED_C_SOURCES:
  object_file = "$OBJECTS/" + src[:-1] + "o"
  n.build(object_file, "XZ_CC", inputs="$XZ_ROOT/" + src)
  xz_object_list.append(object_file)
object_file = "$OBJECTS/xzdec.o"
n.build(object_file, "XZ_CC", inputs="xzdec.c")
xz_object_list.append(object_file)
n.newline()

targets_list = []
compressed_targets_list = []

# build the many GME players
for gme_type in GME_TYPES:
  n.comment("the build files for " + gme_type + " GME player")
  object_list = xz_object_list[:]
  cxx_compiler = "GME_" + gme_type + "_CXX"
  for src in GME_CPP_SOURCES:
    object_file = "$OBJECTS/" + gme_type + "_" + src[:-3] + "o"
    n.build(object_file, cxx_compiler, inputs="$GME_ROOT/" + src)
    object_list.append(object_file)
  object_file = "$OBJECTS/gme_" + gme_type + "_interface.o"
  n.build(object_file, cxx_compiler, inputs="gme_interface.cpp")
  object_list.append(object_file)
  n.newline()

  player_target = "$FINAL_DIR/cr-gme-" + gme_type + ".js"
  n.comment("link the " + gme_type + " GME player")
  n.build(player_target, "LINK_GME", inputs=object_list)
  targets_list.append(player_target)
  compressed_targets_list.append(player_target + "gz")
  n.newline()

# the compressed targets
for i in xrange(len(targets_list)):
    n.build(compressed_targets_list[i], "COMPRESS", inputs=targets_list[i])
n.newline()

# the uncompressed targets
n.build("players", "phony", inputs=targets_list)
n.newline()

# the compressed targets
n.build("compressed-players", "phony", inputs=compressed_targets_list)
n.newline()

# default rule
n.comment("build the uncompressed players by default")
n.default("players")

