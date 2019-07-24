import os
import ycm_core

flags = [
'-Wall',
'-Wextra',
'-Werror',
'-std=c11',
'-I.',
'-Iinclude'
]

def FlagsForFile(filename):
  return { 'flags': flags, 'do_cache': True }
