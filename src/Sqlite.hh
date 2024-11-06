#ifndef MP_SQLITE_HH
#define MP_SQLITE_HH
#pragma once

/** \file Sqlite.hh
 * Declarations SQLite
 *
 * (c) Copyright  Semih Cemiloglu
 * All rights reserved, see COPYRIGHT file for details.
 *
 * $Id$
 *
 *
 */


// Std
#include <any>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <cstring>
#include <tuple>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <source_location>
// Prj
#include <sqlite3.h>

namespace MP {


// Adaptation of DBC facilities: Expects and Ensures
inline void Expects(bool v, const std::source_location loc = std::source_location::current())
{
  if(!v) {
    std::ostringstream os;
    os << "Expect failure at " << loc.file_name() << '(' << loc.line() << ':' << loc.column() << ')' << loc.function_name();
    throw std::runtime_error(os.str());
  }
}

inline void Ensures(bool v, const std::source_location loc = std::source_location::current())
{
  if(!v) {
    std::ostringstream os;
    os << "Ensure failure at " << loc.file_name() << '(' << loc.line() << ':' << loc.column() << ')' << loc.function_name();
    throw std::runtime_error(os.str());
  }
}


  // Sqlite C interface entry point:
  // https://www.sqlite.org/cintro.html

  // TYPES
  // To hold Sqlite blobs in memory
  typedef std::vector<uint8_t> Blob_t;


  constexpr bool SqliteExceptionsEnabled = true;
  static inline bool SqliteEx = SqliteExceptionsEnabled;

  // ================================= SqliteDb class ============================================


  // Custom deleter for sqlite3 shared_ptr objects
  void Sqlite3StmtDeleter(sqlite3_stmt* stmt);

  // Sqlite3 Statement handle/holder
  // https://www.sqlite.org/c3ref/stmt.html
  class SqliteStmt
  {
    protected:
      std::shared_ptr<sqlite3_stmt> m_stmt; // Shared ptr to the statement handle      
      int m_bindPos;     // Bind ordinal to use during bind() operations  1-based
      int m_colPos;      // Column ordinal to use during column() operations 0-based
      mutable int m_rc;  // Return code from the last operation
      mutable bool m_ex; // Exceptions enabled?

    public:
      // CREATORS
      SqliteStmt(sqlite3_stmt* stmt = nullptr); // Takes ownership of the given pointer
      ~SqliteStmt();

      // ACCESSORS
      // Access the sqlite3 statement handle
      sqlite3_stmt* get() { return m_stmt.get(); }
      sqlite3_stmt* operator->() { return m_stmt.get(); }
      operator sqlite3_stmt*() { return m_stmt.get(); }

      // Get last return code
      inline int rc() const { return m_rc; }

      // Get exceptions capability for this instance
      inline bool ex() const { return m_ex; }
      // Set exceptions capability for this instance
      inline void ex(bool val) const { m_ex = val; }

      // Check error and throw if exceptions enabled
      inline int ce() const { return checkError(); }

      // MODIFIERS
      // Specializations are below
      template <typename T>
      int bind(int pos, const T t);
      template <typename T>
      int bindref(int pos, const T& t);
      
      // Bind to a tuple by value
      template <typename... Args>
      inline int bind(int pos, const std::tuple<Args...> args)
      { 
        m_rc = sqlite3_bind_blob(m_stmt.get(), pos, &args, sizeof(args), nullptr);
        checkError();
        return m_rc;
      }  
      // Bind to a tuple by reference
      template <typename... Args>
      inline int bindref(int pos, const std::tuple<Args...>& args)
      {
        m_rc = sqlite3_bind_blob(m_stmt.get(), pos, &args, sizeof(args), nullptr); 
        checkError();
        return m_rc;
      }

      int step();
      // Postfix ++, shorthand for step() but to be used in loops
      bool operator++(int); 


      // Get number of columns available
      int columnCount()
      { return sqlite3_column_count(m_stmt.get());  }

      // Get the name of the pointed column
      const char* columnName(int col)
      { return sqlite3_column_name( m_stmt.get(), col ); }

      // Get actual type of the given column
      // int sqlite3_column_type(sqlite3_stmt*, int iCol);
      int columnType(int col)
      { return sqlite3_column_type( m_stmt.get(), col ); }

      // Get actual name of the given column
      const char* columnTypeStr(int col);

      // Get the declared type of the given column
      const char* columnDeclType(int col)
      { return sqlite3_column_decltype( m_stmt.get(), col); }


      // Specializations are below
      // Column access member template
      template <typename T>
      inline void column(int col, T& t);

      template <typename... Args>
      inline void column(int col, std::tuple<Args...>& args)
      { 
        void* bptr = const_cast<void*>( sqlite3_column_blob(m_stmt.get(), col) ); // Obtain blob pointer
        Ensures(bptr); // Must be non-null
        int size = sqlite3_column_bytes(m_stmt.get(), col);
        Ensures( size == sizeof(args)); // Tuple size must match the blob size
        args = * static_cast<std::tuple<Args...>*>(bptr); // Copy blob data it to the tuple
      }

      // Return column of any type
      std::any column(int col);      


      // at() is synonym for column()
      template <typename T>
      inline void at(int col, T& t) { column(col, t); }

      // Reset the statement back to initial state, ready to be executed again.
      int reset();

      // Finalizes the statement, deleting the managed statement handle.
      int finalize();

      // Check if an error occurred in the last operation
      int checkError() const;

      // << operator is a shorthand for bind()
      template <typename T> friend inline SqliteStmt& operator<<(SqliteStmt& stmt, const T& t)
      { stmt.m_rc = stmt.bind(stmt.m_bindPos++, t); Ensures(stmt.m_rc == SQLITE_OK); return stmt; }

      // >> operator is a shorthand for column()
      template <typename T> friend inline SqliteStmt& operator>>(SqliteStmt& stmt, T& t)
      { stmt.column(stmt.m_colPos++, t); return stmt; }

  }; // class


  //
  // bind() group
  // Binds input parameters to a prepared statement
  // 1-based ordinals
  template <> inline int SqliteStmt::bind(int i, const int32_t val)
  { return m_rc = sqlite3_bind_int(m_stmt.get(), i, val); }

  template <> inline int SqliteStmt::bind(int i, const int64_t val)
  { return m_rc = sqlite3_bind_int64(m_stmt.get(), i, val); }

  template <> inline int SqliteStmt::bind(int i, const double val)
  { return m_rc = sqlite3_bind_double(m_stmt.get(), i, val); }

  template <> inline int SqliteStmt::bind(int i, const std::string_view val)
  { return m_rc = sqlite3_bind_text(m_stmt.get(), i, val.data(), val.length(), nullptr); }
  
  template <> inline int SqliteStmt::bind(int i, const char* val)
  { return m_rc = sqlite3_bind_text(m_stmt.get(), i, val, std::strlen(val), nullptr); }

  template <> inline int SqliteStmt::bind(int i, const std::nullptr_t)
  { return m_rc = sqlite3_bind_null(m_stmt.get(), i); }

  template <> inline int SqliteStmt::bindref(int i, const std::string& val)
  { return m_rc = sqlite3_bind_text(m_stmt.get(), i, val.data(), val.length(), nullptr); }

  template <> inline int SqliteStmt::bindref(int i, const Blob_t& v)
  { return m_rc = sqlite3_bind_blob(m_stmt.get(), i, v.data(), v.size(), nullptr); }


  //
  // column() group
  // Extracts output from the executed (stepped) statements
  // 0-based ordinals
  template <> inline void SqliteStmt::column(int i, int32_t& v)
  { v = sqlite3_column_int(m_stmt.get(), i); }

  template <> inline void SqliteStmt::column(int i, int64_t& v)
  { v = sqlite3_column_int64(m_stmt.get(), i); }

  template <> inline void SqliteStmt::column(int i, double& v)
  { v = sqlite3_column_double(m_stmt.get(), i); }

  template <> inline void SqliteStmt::column(int i, std::string& v)
  { v = (const char*)sqlite3_column_text(m_stmt.get(), i); }

  template <> inline void SqliteStmt::column(int i, Blob_t& v) { 
    uint8_t* ptr = (uint8_t*) sqlite3_column_blob(m_stmt.get(), i); 
    Ensures(ptr); // Must be a non-null ptr
    int size = sqlite3_column_bytes(m_stmt.get(), i);
    v.resize(size);
    for(int j=0; j<size; ++j) v.at(j) = *(ptr+j);
    // Consider memcpy() here
  }


  // ================================= SqliteDb class ============================================

  // Custom deleter for sqlite3 shared_ptr objects
  void Sqlite3Deleter(sqlite3* dbh);

  // Sqlite3 Db handle/session holder
  class SqliteDb
  {
    protected:
      std::shared_ptr<sqlite3> m_dbh; // Shared ptr to the db handle
      std::string m_filename; // Filename of the database
      int m_flags;            // Opening flags
      mutable int m_rc;       // Return code from the last operation
      mutable bool m_ex;      // Exceptions enabled?

    public:
      // CREATORS
      SqliteDb();
      SqliteDb(std::string_view filename, int flags = SQLITE_OPEN_READWRITE);
      ~SqliteDb();

      // ACCESSORS
      // Access the sqlite3/database handle
      sqlite3* get() const { return m_dbh.get(); }
      sqlite3* operator->() const { return m_dbh.get(); }
      operator sqlite3*() const { return m_dbh.get(); }


      std::string_view getFileName() const { return m_filename; }
      int getFlags() const { return m_flags; }

      // Get last return code
      inline int rc() const { return m_rc; }

      // Get exceptions capability for this instance
      inline bool ex() const { return m_ex; }
      // Set exceptions capability for this instance
      inline void ex(bool val) const { m_ex = val; }

      // Check error and throw if exceptions enabled
      inline int ce() const { return this->checkError(); } // Fetches and uses eec
      inline int ce2() const { return SqliteDb::CheckError(m_rc, m_ex); } // Uses m_rc

      // Execute SQL statements
      int exec(std::string_view stmt) const;

      // Check if an error occurred in the last operation
      int checkError() const;


      // MODIFIERS
      // Construct-prepare the SqliteStmt for a given SQL stamement string
      int prepare(std::string_view sqlStr, SqliteStmt& stmt);

      // An alternate form for the prepare()
      SqliteStmt stmt(std::string_view sqlStr);


    
      // STATIC MEMBERS
      // Check if an error occurred in the last operation and report
      static int CheckError(int rc, bool throwOnError = SqliteExceptionsEnabled);


    private:
      // Not allowed

  }; // class



  


} // namespace


     
#endif /* Include guard */
