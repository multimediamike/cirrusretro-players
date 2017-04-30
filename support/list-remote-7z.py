#!/usr/bin/python

import commands
import httplib
import json
import os
import struct
import sys
import tempfile
import urlparse

CMD_7ZA = "/usr/bin/7za"

def fetch_http_bytes(netloc, path, offset, length):
    conn = httplib.HTTPConnection(netloc)
    range_header = "bytes=%d-%d" % (offset, offset+length-1)
    conn.request("GET", path, headers={'Range': range_header})
    resp = conn.getresponse()
    content = resp.read()
    conn.close()
    return content

def list_remote_7z(url):
    # make sure heavy-lifting tool works
    if not os.access(CMD_7ZA, os.X_OK):
        print "Expected to find the executable tool '%s'" % (CMD_7ZA)
        sys.exit(1)

    # break down the URL
    parts = urlparse.urlparse(url)

    # get a handle to a temporary file
    f = tempfile.NamedTemporaryFile()

    # get the first 32 bytes
    content = fetch_http_bytes(parts.netloc, parts.path, 0, 32)
    f.write(content)
    (sig1, sig2) = struct.unpack(">IH", content[0:6])
    table_offset = struct.unpack("<I", content[12:16])[0] + 32
    table_length = struct.unpack("<H", content[20:22])[0]

    # validate 7-zip signature
    if sig1 != 0x377abcaf or sig2 != 0x271c:
        print "URL does not reference a 7-zip file"
        return None

    # Empirical observation: if the table length field in the header is
    # <= 38 then the table offset references another table that will
    # contain the true directory entry. But if it's larger than 38, then
    # the header directly references the directory entry. There is probably
    # a flag in the header that signals this.
    if table_length <= 38:
        f.seek(table_offset, os.SEEK_SET)
        content = fetch_http_bytes(parts.netloc, parts.path, table_offset, table_length)
        f.write(content)
        dir_entry_length = struct.unpack(">H", content[7:9])[0] & 0x7FFF;
        dir_entry_offset = table_offset - dir_entry_length
    else:
        dir_entry_length = table_length
        dir_entry_offset = table_offset

    # read the directory entry
    f.seek(dir_entry_offset, os.SEEK_SET)
    content = fetch_http_bytes(parts.netloc, parts.path, dir_entry_offset, dir_entry_length)
    f.write(content)
    f.flush()

    # run the 7za command and get the file listing
    command = "%s l %s" % (CMD_7ZA, f.name)
    (status, output) = commands.getstatusoutput(command)
    if status != 0:
        print "%s returned %d" % (CMD_7ZA, status)

    # parse the output
    files = []
    lines = output.splitlines()
    parsing = False
    for line in lines:
        if line.startswith("-------"):
            parsing = not parsing
            continue
        if parsing:
            size = int(line[25:38])
            name = line[53:]
            file_record = { 'name': name, 'size': size }
            files.append(file_record)

    return { 'url': url, 'files': files }

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "USAGE: list-remote-7z.py <url of 7z file>"
        sys.exit(1)

    files = list_remote_7z(sys.argv[1])

    print json.dumps(files, indent=4)
