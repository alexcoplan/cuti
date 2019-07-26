#!/usr/bin/env python3

import argparse

cflags  = "-g -Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments"
cflags += "-std=c11 -fcolor-diagnostics"

ninja_vars = {
  "builddir" : "build",
  "cc" : "clang",
  "cflags" : "-g -Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments -std=c11 -fcolor-diagnostics",
  "ldflags" : "-L$builddir",
}

ninjafile_base = """
rule cc
  command = $cc -MMD -MT $out -MF $out.d $cflags -c $in -o $out
  description = CC $out
  depfile = $out.d
  deps = gcc

rule link
  command = $cc $ldflags -o $out $in
  description = LINK $out

"""

def get_san_flags(desc):
  if desc is None:
    return ""

  flags = ""
  sans = desc.split(',')
  for san in sans:
    flags += " -fsanitize=%s" % san
  return flags

def strip_c_ext(c_file):
  parts = c_file.split(".")
  if len(parts) != 2 or parts[1] != "c":
    return None
  return parts[0]

class BuildEnv:
  def __init__(self, vars):
    self.vars = vars
    self.progs = []
    self.objs = []

  def Program(self, name, src):
    objects = []
    for f in src:
      obj_name = strip_c_ext(f)
      objects.append(obj_name)
      if obj_name not in self.objs:
        self.objs.append(obj_name)
    self.progs.append((name, objects))
  
  def Test(self, name, src):
    return self.Program("test/%s" % name, src + ["test.c"])

  def write_ninja(self, fp):
    for k,v in self.vars.items():
      fp.write("%s = %s\n" % (k,v))

    fp.write(ninjafile_base)

    fp.write("# objects\n")
    for obj in self.objs:
      fp.write("build $builddir/%s.o: cc %s.c\n" % (obj, obj))

    fp.write("\n# executables\n")
    for (name, objs) in self.progs:
      obj_line = " ".join(map(lambda x: "$builddir/%s.o" % x, objs))
      fp.write("build $builddir/%s: link %s\n" % (name, obj_line))


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('--sanitizers', '--san', dest='sanitizers', default=None)
  args = parser.parse_args()
  san_flags = get_san_flags(args.sanitizers)
  ninja_vars["cflags"] += san_flags
  ninja_vars["ldflags"] += san_flags

  env = BuildEnv(ninja_vars)
  env.Test('test_test', ['test_test.c'])
  env.Test('test_utf_buffer', ['test_utf_buffer.c', 'utf_buffer.c'])

  with open("build.ninja", "w") as f:
    env.write_ninja(f)