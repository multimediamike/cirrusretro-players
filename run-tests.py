#!/usr/bin/python

import commands
import glob
import multiprocessing
import tempfile

NODE = "node"

def runTest(spec):
    tf = tempfile.NamedTemporaryFile(suffix='.wav')
    command = "NODE_PATH=$PWD/final %s cr-test-harness.js %s %s" % (NODE, spec, tf.name)
    (status, output) = commands.getstatusoutput(command)
    return { 'spec': spec, 'status': status, 'output': output }

if __name__ == "__main__":
    testSpecs = glob.glob("test-specs/*.json")
    pool = multiprocessing.Pool()
    it = pool.imap_unordered(runTest, testSpecs);
    succeeded = 0
    try:
        while 1:
            result = it.next()
            if result['status'] == 0:
                succeeded += 1
                print "."
            else:
                print result['spec'] + " failed:" + result['output']
    except StopIteration:
        pass

    print "%d / %d tests succeeded" % (succeeded, len(testSpecs));
