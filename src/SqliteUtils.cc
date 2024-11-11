
#include "SqliteUtils.hh"
// Std
#include <string>
#include <exception>
#include <filesystem>
#include <system_error>
#if defined(__GNUC__) && (__GNUC__ < 13)
# define FMT_HEADER_ONLY
# include <fmt/core.h>
using namespace fmt;
#else
# include <format>
#endif
// Prj
#include <absl/log/log.h>

using namespace std;
namespace fs = std::filesystem;

namespace MP {



// Open an SQLite DB from a given file name
int OpenSQLiteDB(const std::string& dbf, SqliteDb& sqlDb, int dbOpenFlags)
{
  int rv = 0;
  try {
    auto dbPath = fs::path(dbf);
    std::error_code fsec; // std::error_code
    bool dbFileExists = false;
    
    if(!(dbOpenFlags & SQLITE_OPEN_CREATE)) {
      dbFileExists = fs::exists(dbPath, fsec);
      if(!dbFileExists) {
        LOG(ERROR) << format("DB file {} doesn't exist...", dbf);
        return 1;
      };
    }

    sqlDb = SqliteDb(dbf, dbOpenFlags);

  } // try
  catch(std::exception& e) { 
    LOG(ERROR) << e.what();
    rv = 1;
  }

  return rv;
}



bool TableExists(SqliteDb& db, std::string_view table) {
  int rc{0};
  bool rv{false};
  try {
    string sqlStr = format("SELECT name FROM sqlite_master WHERE type='table' AND name='{}';", table);
    SqliteStmt stmt;
    rc = db.prepare(sqlStr, stmt);
    rc = stmt.step();
    if(rc == SQLITE_ROW) rv = true; // Table exists
    rc = stmt.finalize(); // Not necessary but called anyway
  }
  catch(const std::exception& e) {
    LOG(ERROR) << e.what();
  }

  return rv;
}



int GetAllTableNames(SqliteDb& db, std::set<std::string>& tableSet)
{
  int rc{1}; // Failure

  try {
    tableSet.clear();
    string sqlStr{"SELECT name FROM sqlite_master WHERE type='table'"};
    SqliteStmt stmt;
    rc = db.prepare(sqlStr, stmt);
    while(stmt++) {
      string tbl;
      stmt.column(0, tbl);
      tableSet.emplace(tbl);
    }

    rc = 0; // Success;
  }
  catch(const std::exception& e) {
    LOG(ERROR) << e.what();
  }

  return rc;
}


} // end namespace