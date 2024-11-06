
#include "Sqlite.hh"
// Std
#include <string>
#include <format>
// Prj
#include <absl/log/log.h>


using namespace std;

namespace MP {

// Custom deleter for sqlite3 shared_ptr objects
void Sqlite3Deleter(sqlite3* dbh)
{
  // Called when dbh is going out of scope/deallocated.
  if(dbh) {
    VLOG(2) << std::format("Closing Sqlite3 Dbh={}", (void*)dbh);
    int rv = sqlite3_close_v2(dbh);
    if(rv != SQLITE_OK) {
      LOG(WARNING) << std::format("{} {}", rv, sqlite3_errstr(rv));
    }
  }
}

// Default constructor
SqliteDb::SqliteDb() : m_dbh{0}, m_filename{}, m_flags{0}, m_rc{0}, m_ex{SqliteEx} {}

// Common constructor
SqliteDb::SqliteDb(std::string_view filename, int flags) :
 m_dbh{}, m_filename{filename}, m_flags{flags}, m_rc{0}, m_ex{SqliteEx}
{
  sqlite3* dbh = nullptr;
  const char *zVfs = nullptr;
  int rv = m_rc = sqlite3_open_v2(filename.data(), &dbh, flags, zVfs);
  if(rv == SQLITE_OK) {
    m_dbh.reset(dbh, Sqlite3Deleter);
    VLOG(2) << std::format("Constructed Sqlite3 Dbh={}", (void*)m_dbh.get());
    rv = sqlite3_extended_result_codes(dbh, 1); // Enable extended result codes by default
  }
  else {
    LOG(ERROR) << "Sqlite3 err=" << sqlite3_errmsg(dbh);
  }  
}

// Destructor. Actual work is done via Sqlite3Deleter()
SqliteDb::~SqliteDb()
{ 
}

// https://www.sqlite.org/c3ref/errcode.html
int SqliteDb::checkError() const
{
  int eec = sqlite3_extended_errcode(m_dbh.get());
  if(eec != SQLITE_OK) {
    const char* emsg = sqlite3_errmsg(m_dbh.get());
    constexpr const char* fmt = "Sqlite eec={} {}";
    if(eec==SQLITE_ROW or eec==SQLITE_DONE) {
      LOG(INFO) << std::format(fmt, eec, emsg);
    }
    else {
      LOG(ERROR) << std::format(fmt, eec, emsg);
      if(SqliteExceptionsEnabled && m_ex) throw std::runtime_error(emsg);      
    }
  }
  return eec;
}

// https://www.sqlite.org/c3ref/errcode.html
// https://www.sqlite.org/rescode.html
int SqliteDb::CheckError(int rc, bool throwOnError)
{
  if(rc != SQLITE_OK) {
    const char* emsg = sqlite3_errstr(rc);
    constexpr const char* fmt = "Sqlite rc={} {}";
    if(rc==SQLITE_ROW or rc==SQLITE_DONE) {
      LOG(INFO) << std::format(fmt, rc, emsg);
    }
    else {
      LOG(ERROR) << std::format(fmt, rc, emsg);
      if(SqliteExceptionsEnabled && throwOnError) throw std::runtime_error(emsg);
    }
  }

  return rc;
}


// https://www.sqlite.org/c3ref/exec.html
// int sqlite3_exec(
//  sqlite3*,                                  /* An open database */
//  const char *sql,                           /* SQL to be evaluated */
//  int (*callback)(void*,int,char**,char**),  /* Callback function */
//  void *,                                    /* 1st argument to callback */
//  char **errmsg                              /* Error msg written here */
// );
int SqliteDb::exec(std::string_view stmt) const
{
  char* errmsg = nullptr;

  int rc = m_rc = sqlite3_exec(m_dbh.get(), stmt.data(), nullptr, nullptr, &errmsg);
  if(errmsg)  { 
    sqlite3_free(errmsg);
    LOG(ERROR) << errmsg;
  }
  else {
    CheckError(rc, m_ex);
  }
  return rc;
}


// https://www.sqlite.org/c3ref/prepare.html
// int sqlite3_prepare_v3(
//  sqlite3 *db,            /* Database handle */
//  const char *zSql,       /* SQL statement, UTF-8 encoded */
//  int nByte,              /* Maximum length of zSql in bytes. */
//  unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
//  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
//  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
// );
//
int SqliteDb::prepare(std::string_view sqlStr, SqliteStmt& stmt)
{
  int sqlStrLen = sqlStr.length();
  unsigned int prepFlags = 0;
  sqlite3_stmt *ppStmt = nullptr;
  const char *pzTail = nullptr;

  int rc = m_rc = sqlite3_prepare_v3(m_dbh.get(), sqlStr.data(), sqlStrLen, prepFlags, &ppStmt, &pzTail);
  if(rc == SQLITE_OK) {
    // Take ownership of the returned sqlite3_stmt*
    stmt = SqliteStmt(ppStmt);
  }
  else {
    CheckError(rc, m_ex);
  }

  return rc;
}


SqliteStmt SqliteDb::stmt(std::string_view sqlStr)
{
  int sqlStrLen = sqlStr.length();
  unsigned int prepFlags = 0;
  sqlite3_stmt *ppStmt = nullptr;
  const char *pzTail = nullptr;

  int rc = m_rc = sqlite3_prepare_v3(m_dbh.get(), sqlStr.data(), sqlStrLen, prepFlags, &ppStmt, &pzTail);
  if(rc == SQLITE_OK) {
    // Take ownership of the returned sqlite3_stmt*
    return SqliteStmt(ppStmt);
  }
  else {
    CheckError(rc, m_ex);
    return SqliteStmt();
  }
}


//===================================================================================


// Custom deleter for sqlite3 shared_ptr objects
void Sqlite3StmtDeleter(sqlite3_stmt* stmt)
{
  // Called when stmt is going out of scope/deallocated.
  if(stmt) {
    VLOG(2) << std::format("Finalizing Sqlite3 Stmt={}", (void*)stmt);
    int rv = sqlite3_finalize(stmt);
    if(rv != SQLITE_OK) {
      // Do not throw as this is called within destructor
      LOG(ERROR) << std::format("{} {}", rv, sqlite3_errstr(rv));
    }
  }
}


SqliteStmt::SqliteStmt(sqlite3_stmt* stmt) : m_bindPos{1}, m_colPos{0}, m_rc{0}, m_ex{SqliteEx}
{
  m_stmt.reset(stmt, Sqlite3StmtDeleter);
  if(stmt) {
    VLOG(2) <<  std::format("Constructed Sqlite3 Stmt={}", (void*)m_stmt.get());
  }
}

SqliteStmt::~SqliteStmt()
{
}

// https://www.sqlite.org/c3ref/step.html
int SqliteStmt::step()
{  
  // Reset the column position to the beginning
  m_colPos = 0; 
  m_rc = sqlite3_step(m_stmt.get());
  return checkError();
}


bool SqliteStmt::operator++(int)
{
  m_colPos = 0; // Reset the column position to beginning with every step
  int rc = m_rc = sqlite3_step(m_stmt.get());
  if(rc==SQLITE_ROW) return true;
  if(rc==SQLITE_DONE) {
    VLOG(1) << std::format("Stepping done for Stmt={}", (void*)m_stmt.get());
    return false;
  }
  if(rc!=SQLITE_OK) {
    LOG(WARNING) << "Error stepping Stmt=" << (void*)m_stmt.get();
    checkError(); //?
  }
  return false;
}


// https://www.sqlite.org/c3ref/reset.html
int SqliteStmt::reset()
{
  // Reset binding and column positions for the next set operations
  m_bindPos = 1;
  m_colPos = 0;
  m_rc = sqlite3_reset(m_stmt.get());
  return checkError();
}


// https://www.sqlite.org/c3ref/finalize.html
int SqliteStmt::finalize()
{
  // Reset binding and column positions for the next set operations
  m_bindPos = 1;
  m_colPos = 0;
  m_stmt.reset(); // This eventually calls sqlite3_finalize() via Deleter
  return 0;
}


// Check if an error occurred in the last operation
int SqliteStmt::checkError() const
{
  if(m_rc != SQLITE_OK) {
    const char* emsg = sqlite3_errstr(m_rc);
    constexpr const char* fmt = "Sqlite rc={} {}";
    if(m_rc==SQLITE_ROW or m_rc==SQLITE_DONE) {
      VLOG(2) << std::format(fmt, m_rc, emsg);
    }
    else {
      LOG(ERROR) << std::format(fmt, m_rc, emsg);
      if(SqliteExceptionsEnabled && m_ex) throw std::runtime_error(emsg);
    }
  }

  return m_rc;
}



any SqliteStmt::column(int col)
{
  int ct = sqlite3_column_type(m_stmt.get(), col);

  any val; 
  switch(ct) { // Column type
    case SQLITE_INTEGER: 
      val = sqlite3_column_int(m_stmt.get(), col);
      return val;
    case SQLITE_FLOAT: 
      val = sqlite3_column_double(m_stmt.get(), col);
      return val;
    case SQLITE_TEXT:
      val = string{ (const char*) sqlite3_column_text(m_stmt.get(), col) };
      return val;
    case SQLITE_BLOB: 
     {
      uint8_t* ptr = (uint8_t*) sqlite3_column_blob(m_stmt.get(), col); 
      int size = sqlite3_column_bytes(m_stmt.get(), col);
      Blob_t blob(size);
      for(int j=0; j<size; ++j) blob.at(j) = *(ptr+j);      
      val = blob;
      return val;
     }
    case SQLITE_NULL:
    default:
      return val; // Null value
  }
}


const char* SqliteStmt::columnTypeStr(int col)
{
  int ct = sqlite3_column_type(m_stmt.get(), col);

  switch(ct) { // Column type
    case SQLITE_INTEGER: return "SQLITE_INTEGER";
    case SQLITE_FLOAT: return "SQLITE_FLOAT";
    case SQLITE_TEXT: return "SQLITE_TEXT";
    case SQLITE_BLOB: return "SQLITE_BLOB";
    case SQLITE_NULL: return "SQLITE_NULL";
    default:
      return "UnknownType";
  }
}



} // end namespace