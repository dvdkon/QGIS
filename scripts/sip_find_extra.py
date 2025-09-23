#!/usr/bin/env python3
"""
This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

Finds .sip.in files without a corresponding .h file present by parsing the
sipify-generated header comments.
"""
import sys
import itertools as it
from pathlib import Path

QGIS_DIR = Path(__file__).parent.parent

error_found = True
for sipfile in (QGIS_DIR / "python").glob("**/auto_generated/**/*.sip.in"):
    sipfile = sipfile.relative_to(QGIS_DIR)
    with open(sipfile) as f:
        # Read first 7 lines where generated header should be
        headerlines = list(it.islice(f, 7))
        if headerlines[5] != " * Do not edit manually ! Edit header and run scripts/sipify.py again   *\n":
            print(f"{sipfile}: No standardised header! Skipping...")
            print(headerlines)
            continue

        headerfile = headerlines[3][3:-3].strip()
        if not (QGIS_DIR / headerfile).exists():
            print(f"{sipfile}: Source header {headerfile} missing!")
            error_found = True

if error_found:
    sys.exit(1)
