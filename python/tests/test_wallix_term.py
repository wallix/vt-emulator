#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import unittest
import os
import sys

from wallix_term.wallix_term import (OutputFormat,
                                     TranscriptPrefix,
                                     CreateFileMode,
                                     Allocator,
                                     TerminalEmulatorException,
                                     TerminalEmulator,
                                     TerminalEmulatorBuffer,
                                     transcript_from_ttyrec)


unittest.util._MAX_LENGTH = 9999

def read_file(filename:str) -> bytes:
    with open(filename, 'rb') as f:
        return f.read()

class TestTerminalEmulator(unittest.TestCase):
    def __init__(self, what):
        super().__init__(what)
        self.maxDiff = None

    def test_term(self):
        term = TerminalEmulator(3,10)
        buf = TerminalEmulatorBuffer()

        data = None

        def set_data(d):
            nonlocal data
            data = d
        term.set_log_function(set_data)

        term.feed(b'\033[324a')
        self.assertEqual(data, 'Undecodable sequence: \\x1b[324a')

        term.set_title('Lib test')
        term.feed(b'ABC')

        contents = br'{"x":3,"y":0,"lines":3,"columns":10,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"ABC"}]],[[{}]],[[{}]]]}'

        buf.prepare(term, OutputFormat.json)
        self.assertEqual(buf.as_bytes(), contents)

        filename = "/tmp/termemu-test.json"
        buf.write_integrity(filename)
        self.assertEqual(read_file(filename), contents)
        os.remove(filename)

        buf.write(filename, create_mode=CreateFileMode.force_create)
        self.assertEqual(read_file(filename), contents)
        os.remove(filename)

        self.assertEqual(buf.as_bytes(), contents)

        term.resize(2, 2)

        contents = br'{"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]}'

        buf.prepare(term, OutputFormat.json)
        self.assertEqual(buf.as_bytes(), contents)

        contents = br'{"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]],"extra":"plop"}'

        buf.prepare(term, OutputFormat.json, b'"plop"')
        self.assertEqual(buf.as_bytes(), contents)

        contents = br'{"y":-1,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]}'

        term.feed(b"\033[?25l")
        buf.prepare(term, OutputFormat.json)
        self.assertEqual(buf.as_bytes(), contents)

    def test_buffer_transcript(self):
        os.environ["TZ"] = "CET-1CEST,M3.5.0,M10.5.0/3" # for localtime_r

        buf = TerminalEmulatorBuffer()
        buf.prepare_transcript_from_ttyrec("../test/data/ttyrec1", TranscriptPrefix.datetime)

        self.assertEqual(buf.as_bytes(),
                         "2017-11-29 17:29:05 [2]~/projects/vt-emulator!4902$(nomove)✗ l               ~/projects/vt-emulator\n"
                         "2017-11-29 17:29:05 binding/  jam/     LICENSE   packaging/  redemption/  test/   typescript\n"
                         "2017-11-29 17:29:05 browser/  Jamroot  out_text  README.md   src/         tools/  vt-emulator.kdev4\n"
                         "2017-11-29 17:29:06 [2]~/projects/vt-emulator!4903$(nomove)✗                 ~/projects/vt-emulator\n".encode())

    def test_transcript(self):
        os.environ["TZ"] = "CET-1CEST,M3.5.0,M10.5.0/3" # for localtime_r
        outfile = "/tmp/emu_transcript_py.txt"
        self.assertRaises(TerminalEmulatorException,
                          lambda: transcript_from_ttyrec("aaa", outfile, 0o664, CreateFileMode.force_create, TranscriptPrefix.datetime))

        transcript_from_ttyrec("../test/data/ttyrec1", outfile, 0o664,
                               CreateFileMode.force_create,
                               TranscriptPrefix.datetime)

        self.assertEqual(read_file(outfile),
                         "2017-11-29 17:29:05 [2]~/projects/vt-emulator!4902$(nomove)✗ l               ~/projects/vt-emulator\n"
                         "2017-11-29 17:29:05 binding/  jam/     LICENSE   packaging/  redemption/  test/   typescript\n"
                         "2017-11-29 17:29:05 browser/  Jamroot  out_text  README.md   src/         tools/  vt-emulator.kdev4\n"
                         "2017-11-29 17:29:06 [2]~/projects/vt-emulator!4903$(nomove)✗                 ~/projects/vt-emulator\n"
                         "".encode())

    def test_buffer_transcript_big_file(self):
        os.environ["TZ"] = "CET-1CEST,M3.5.0,M10.5.0/3" # for localtime_r
        buf = TerminalEmulatorBuffer()
        buf.prepare_transcript_from_ttyrec("../test/data/debian.ttyrec", TranscriptPrefix.datetime)
        self.assertEqual(buf.as_bytes(), read_file("../test/data/debian.transcript.txt"))

    def test_transcript_big_file(self):
        os.environ["TZ"] = "CET-1CEST,M3.5.0,M10.5.0/3" # for localtime_r
        outfile = "/tmp/emu_transcript_py.txt"
        transcript_from_ttyrec("../test/data/debian.ttyrec", outfile, 0o664,
                               CreateFileMode.force_create, TranscriptPrefix.datetime)
        self.assertEqual(read_file(outfile), read_file("../test/data/debian.transcript.txt"))
