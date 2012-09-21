#!/usr/bin/python
#
# case-tables.py - generates case-tables.h file which contains
# tables for fast and correct character case convertion.
#
# Copyright (c) 2012 Dmitri Arkhangelski (dmitriar@gmail.com).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import sys

msg = "At least version 3.2 of Python is required to run the script!"
try:
    mj = sys.version_info.major
    mn = sys.version_info.minor
    if mj < 3 or (mj == 3 and mn < 2):
        sys.exit(msg)
except:
    sys.exit(msg)

def write_table(_file, _type, _name, _range, _case):
    _file.write("\n%s %s[0x%x] = {\n" % (_type, _name, _range))
    for c in range(0,_range):
        if _case == "lower":
            cc = ord(chr(c).lower())
        else:
            cc = ord(chr(c).upper())
        if _range == 0x100:
            # ASCII convertion rules
            if c > 0x7F: _file.write("0x%02x" % c)
            else: _file.write("0x%02x" % cc)
            if c < 0xFF: _file.write(", ")
            else: _file.write("  ")
        else:
            # UTF-16 conversion rules
            if cc == c: cc = 0x0
            _file.write("0x%04x" % cc)
            if c < 0xFFFF: _file.write(", ")
            else: _file.write("  ")
        if (c + 1) % 8 == 0:
            if _range == 0x100:
                _file.write("/* 0x%02x - 0x%02x */\n" % (c - 7, c))
            else:
                _file.write("/* 0x%04x - 0x%04x */\n" % (c - 7, c))
    _file.write("};\n");

with open("case-tables.h","wt") as f:
    f.write("/*\n")
    f.write("* case-tables.h - tables for fast and correct character case convertion.\n")
    f.write("* Don't modify this file - it's generated by case-tables.py script.\n")
    f.write("*\n")
    f.write("* Generated by Python %s\n" % sys.version)
    f.write("*/\n")
    write_table(f, "char",    "ascii_lowercase", 0x100,   "lower")
    write_table(f, "char",    "ascii_uppercase", 0x100,   "upper")
    write_table(f, "wchar_t", "u16_lowercase",   0x10000, "lower")
    write_table(f, "wchar_t", "u16_uppercase",   0x10000, "upper")
