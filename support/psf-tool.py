#!/usr/bin/python

import functools
import itertools
import os
import struct
import sys

PSF_SIGNATURE = "PSF Song Archive"
PSF_HEADER_SIZE = 20
FILE_RECORD_SIZE = 12

# Taken from:
#   http://stackoverflow.com/a/32775270
def readcstr(f):
    toeof = iter(functools.partial(f.read, 1), '')
    return ''.join(itertools.takewhile('\0'.__ne__, toeof))

# This function takes an open file handle to a psfarchive file and returns
# an array of dictionaries that represent the files in the archive. The
# dictionary keys are:
#  'offset': absolute offset of the file within the archive
#  'size': size of the file
#  'filename': name of the file
def load_file_list(f):
    f.seek(0, os.SEEK_SET)
    header = f.read(PSF_HEADER_SIZE)
    if header[0:16] != PSF_SIGNATURE:
        return None

    # load the file info records
    file_count = struct.unpack(">I", header[16:20])[0]
    file_list = file_count * [None]
    file_records = f.read(file_count * FILE_RECORD_SIZE)
    for i in xrange(file_count):
        # parse the record
        (offset, size, filename_offset) = struct.unpack(">III", file_records[i*FILE_RECORD_SIZE:(i+1)*FILE_RECORD_SIZE])
        # read the filename
        f.seek(filename_offset, os.SEEK_SET)
        filename = readcstr(f)
        file_list[i] = { 'offset': offset, 'size': size, 'filename': filename }

    return file_list

def list_archive(psfarchive):
    # check that the file exists
    if not os.path.exists(psfarchive):
        print psfarchive + " is not a valid file"
        return

    # load the list of files
    f = open(psfarchive, "rb")
    file_list = load_file_list(f)
    if not file_list:
        print psfarchive + " is not a valid psfarchive"
        return

    # format the list
    for i in xrange(len(file_list)):
        entry = file_list[i]
        print "%d: '%s' is %d (0x%X) bytes, located @ offset %d (0x%X)" % (i, entry['filename'], entry['size'], entry['size'], entry['offset'], entry['offset'])

def create(parameter):
    pass

# Split an existing psfarchive file into its constituent PSF files.
# Also, convert them back to zlib compression if they were converted to
# use xz compression instead.
def split(psfarchive):
    # check that the file exists
    if not os.path.exists(psfarchive):
        print psfarchive + " is not a valid file"
        return

    # load the list of files
    f = open(psfarchive, "rb")
    file_list = load_file_list(f)

    # derive the base name
    path = os.path.basename(psfarchive)
    path = path[:path.rfind('.')]

    print "splitting %d files from '%s' into directory '%s'..." % (len(file_list), psfarchive, path)

    # create the path if it's not already there
    if os.path.exists(path):
        print "(path already exists)"
    else:
        os.makedirs(path)

    # split the files
    for entry in file_list:
        out_filename = path + "/" + entry['filename']
        out_file = open(out_filename, "wb")
        f.seek(entry['offset'], os.SEEK_SET)
        file_data = f.read(entry['size'])
        out_file.write(file_data)
        out_file.close()

def refactor(psfarchive):
    # check that the file exists
    if not os.path.exists(psfarchive):
        print psfarchive + " is not a valid file"
        return

    # load the list of files
    f = open(psfarchive, "rb")
    file_list = load_file_list(f)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print "USAGE: psf-tool.py <command> <parameter>"
        print "Valid commands: list, create, split, refactor"
        sys.exit(1)

    command = sys.argv[1]
    parameter = sys.argv[2]

    if command not in ["list", "create", "split", "refactor"]:
        print command + " is not a valid command"
        print "Valid commands are create, split, and refactor"
        sys.exit(1)

    if command == "list":
        list_archive(parameter)
    elif command == "create":
        create(parameter)
    elif command == "split":
        split(parameter)
    elif command == "refactor":
        refactor(parameter)
