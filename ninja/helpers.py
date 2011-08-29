#!/usr/bin/python

import os
import sys

objects = []

def found_src(file_path, no_extension, build_rule):
  #out = sys.argv[1]
  object_path = no_extension + '.o'
  print "build %s: %s %s" % (object_path, build_rule, file_path)
  objects.append(object_path)


def found_file(file_path):
  if file_path.endswith(".cpp"):
    found_src(file_path, file_path[:-4], 'cxx')
  if file_path.endswith(".cc"):
    found_src(file_path, file_path[:-3], 'cxx')
  if file_path.endswith(".c"):
    found_src(file_path, file_path[:-2], 'cc')
  if file_path.endswith(".o"):
    objects.append(file_path)

def visit_directory(dir_path):
  for child_name in os.listdir(dir_path):
    child_path = os.path.join(dir_path, child_name)
    if os.path.isfile(child_path):
      found_file(child_path)
    else:
      visit_directory(child_path)

def do_main():
  out = sys.argv[1]
  defs = []
  for arg in sys.argv[2:]:
    if '=' in arg:
      defs.append(arg)
    else:
      visit_directory(arg)

  for line in sys.stdin.readlines():
    line = line.strip()
    found_file(line)
  object_args = ' '.join(objects)

  if out.endswith('.so'):
    rule = 'linksharedlib'
  elif out.endswith('.a'):
    rule = 'linkstaticlib'
  else:
    rule = 'link'
  print 'build bin/%s: %s %s' % (out, rule, object_args)
  for definition in defs:
    print '  %s' % (definition)

if __name__ == '__main__':
  do_main()
