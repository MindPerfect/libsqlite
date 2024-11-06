#ifndef MP_SQLITEUTILS_HH
#define MP_SQLITEUTILS_HH
#pragma once

/** \file SqliteUtils.hh
 * Declarations SQLite Utilities
 *
 * (c) Copyright  Semih Cemiloglu
 * All rights reserved, see COPYRIGHT file for details.
 *
 *
 */


// Std
#include <set>
#include <string>
#include <string_view>
// Prj
#include "Sqlite.hh"


namespace MP {


  // Open an SQLite DB from a given file name
  int OpenSQLiteDB(const std::string& dbf, SqliteDb& sqlDb, int dbOpenFlags = SQLITE_OPEN_READONLY);

  // Is the given table name exist in the database?
  bool TableExists(SqliteDb& db, std::string_view table);

  // Get all the tables defined in a given database
  int GetAllTableNames(SqliteDb& db, std::set<std::string>& tableSet);

} // namespace


     
#endif /* Include guard */
