#!/usr/bin/env python3
"""
This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

Used for building SIP bindings when SIP_GRANULAR_BUILD=ON. Takes each SIP file
under auto_generated/ to be its own module and buids the ones that have been
changed (C++ output is older by mtime than .sip file).
"""
import os
from pathlib import Path
import sys

from sipbuild.abstract_project import AbstractProject
from sipbuild.exceptions import UserException
from sipbuild.project import Project, PyProjectOptionException
from sipbuild.pyproject import PyProject


def bootstrap_project():
    # Code taken from AbstractProject.bootstrap, because we need to modify the
    # project before .setup() gets run (see original for comments).

    pyproject = PyProject()

    project_factory_name = None
    sip_section_name = "tool.sip"
    value_name = "project-factory"
    sip_section = pyproject.get_section(sip_section_name)
    if sip_section is not None:
        f_name = sip_section.get(value_name)
        if f_name is not None:
            if not isinstance(f_name, str):
                raise PyProjectOptionException(value_name,
                        "should be a 'str' and not '{0}'".format(
                                type(f_name).__name__),
                        section_name=sip_section_name)

            project_factory_name = f_name

    if project_factory_name is None:
        default_factory_py = "project.py"

        if os.path.isfile(default_factory_py):
            project_factory = AbstractProject.import_callable(
                default_factory_py, AbstractProject
            )
        else:
            project_factory = Project
    else:
        project_factory = AbstractProject.import_callable(
            project_factory_name, AbstractProject
        )

    project = project_factory()

    if not isinstance(project, Project):
        raise UserException("The project factory did not return a Project")

    project.arguments = None  # None -- use sys.argv

    # Project.setup() deletes the build directory, which is unsuitable for
    # incremental builds, unless we lie and pretend the directory is temporary.
    project._temp_build_dir = True

    project.setup(
        pyproject, "build", "Tool for QGIS granular build of SIP bindings"
    )

    return project


def setup_bindings(project: Project, whitelist: set[str]):
    # Take important configuration from existing bindings
    old_bindings = next(iter(project.bindings.values()))

    # Delete all existing bindings
    project.bindings.clear()

    # Look for .sip files in the auto_generated/ directory (those created by
    # sipify) and make each into a module
    for sipfile in Path("auto_generated").glob("**/*.sip"):
        # Using the QGIS convention that each NAME.sip file corresponds to a
        # _NAME module.
        name = sipfile.stem
        outfile = Path(project.build_dir) / f"_{name}" / f"sip_{name}part0.cpp"

        if whitelist:
            if name not in whitelist:
                continue
        else:
            if outfile.exists() and sipfile.stat().st_mtime <= outfile.stat().st_mtime:
                # Output is still current, don't add the binding to build
                continue

        bindings = project.bindings_factory(project, name)
        for opt in bindings.get_options():
            setattr(bindings, opt.name, getattr(old_bindings, opt.name))
        # Create exactly one file per .sip file
        bindings.concatenate = 1
        bindings.sip_file = str(sipfile)
        project.bindings[name] = bindings


    print(f"Will build {len(project.bindings)} binding files", file=sys.stderr)


if __name__ == "__main__":
    # For debugging, allow setting exactly which bindings to build
    bindings_to_build = set(os.environ.get("BINDINGS_LIST", "").split(","))
    if bindings_to_build == {""}:
        bindings_to_build = {}

    project = bootstrap_project()
    project.compile = False  # Leave compiling to CMake
    setup_bindings(project, bindings_to_build)
    project.builder.build()
