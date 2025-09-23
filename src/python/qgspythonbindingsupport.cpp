/***************************************************************************
    qgspythonbindingsupport.cpp - support code used by Python bindings
    ---------------------
    begin                : July 2025
    copyright            : (C) 2025 by David Koňařík
    email                : dvdkon@konarici.cz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Must be included *before* Qt, since it uses "slots" as a variable name, but
// Qt #defines it to an empty string.
#include <Python.h>

#include "qgspythonbindingsupport.hpp"

#include "qgis.h"
#include "qgslogger.h"

static QString getTraceback()
{
#define TRACEBACK_FETCH_ERROR( what ) \
  {                                   \
    errMsg = what;                    \
    goto done;                        \
  }

  // acquire global interpreter lock to ensure we are in a consistent state
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  QString errMsg;
  QString result;

  PyObject *modStringIO = nullptr;
  PyObject *modTB = nullptr;
  PyObject *obStringIO = nullptr;
  PyObject *obResult = nullptr;

  PyObject *type, *value, *traceback;

  PyErr_Fetch( &type, &value, &traceback );
  PyErr_NormalizeException( &type, &value, &traceback );

  const char *iomod = "io";

  modStringIO = PyImport_ImportModule( iomod );
  if ( !modStringIO )
    TRACEBACK_FETCH_ERROR( QString( "can't import %1" ).arg( iomod ) );

  obStringIO = PyObject_CallMethod( modStringIO, ( char * ) "StringIO", nullptr );

  /* Construct a cStringIO object */
  if ( !obStringIO )
    TRACEBACK_FETCH_ERROR( "cStringIO.StringIO() failed" );

  modTB = PyImport_ImportModule( "traceback" );
  if ( !modTB )
    TRACEBACK_FETCH_ERROR( "can't import traceback" );

  obResult = PyObject_CallMethod( modTB, ( char * ) "print_exception", ( char * ) "OOOOO", type, value ? value : Py_None, traceback ? traceback : Py_None, Py_None, obStringIO );

  if ( !obResult )
    TRACEBACK_FETCH_ERROR( "traceback.print_exception() failed" );

  Py_DECREF( obResult );

  obResult = PyObject_CallMethod( obStringIO, ( char * ) "getvalue", nullptr );
  if ( !obResult )
    TRACEBACK_FETCH_ERROR( "getvalue() failed." );

  /* And it should be a string all ready to go - duplicate it. */
  if ( !PyUnicode_Check( obResult ) )
    TRACEBACK_FETCH_ERROR( "getvalue() did not return a string" );

  result = QString::fromUtf8( PyUnicode_AsUTF8( obResult ) );

done:

  // All finished - first see if we encountered an error
  if ( result.isEmpty() && !errMsg.isEmpty() )
  {
    result = errMsg;
  }

  Py_XDECREF( modStringIO );
  Py_XDECREF( modTB );
  Py_XDECREF( obStringIO );
  Py_XDECREF( obResult );
  Py_XDECREF( value );
  Py_XDECREF( traceback );
  Py_XDECREF( type );

  // we are done calling python API, release global interpreter lock
  PyGILState_Release( gstate );

  return result;
}

QGISEXTERN QString *qgsPythonExceptionErrorMessage()
{
  // if an explicit QgsProcessingException was raised, we don't retrieve
  // and append the trace. It's too noisy for these "expected" type errors.
  // For all other exceptions, it's likely a coding error in the algorithm,
  // so we DO retrieve the full traceback for debugging.
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  PyTypeObject *err = reinterpret_cast< PyTypeObject * >( PyErr_Occurred() );
  const bool isProcessingException = err && QString( err->tp_name ) == QStringLiteral( "QgsProcessingException" );

  QString what;
  if ( isProcessingException )
  {
    PyObject *type, *value, *traceback;
    PyErr_Fetch( &type, &value, &traceback );
    // check whether the object is already a unicode string
    if ( PyUnicode_Check( value ) )
    {
      what = QString::fromUtf8( PyUnicode_AsUTF8( value ) );
    }
    else
    {
      PyObject *str = PyObject_Str( value );
      what = QString::fromUtf8( PyUnicode_AsUTF8( str ) );
      Py_XDECREF( str );
    }
    PyGILState_Release( gstate );
  }
  else
  {
    PyGILState_Release( gstate );
    QString trace = getTraceback();
    QgsLogger::critical( trace );
    what = trace;
  }
  return new QString(what);
}
