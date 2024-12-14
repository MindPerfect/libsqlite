/** \file Sqlite_t.cc 
 * Test definitions for the Random value generators.
 *
 * (c) Copyright by Semih Cemiloglu
 * All rights reserved, see COPYRIGHT file for details.
 *
 * $Id$
 *
 *
 */

#include "Sqlite.hh"
// Std includes
// Google Test
#include <gtest/gtest.h>
// Prj includes
#if defined(__GNUC__) && (__GNUC__ < 13)
# define FMT_HEADER_ONLY
# include <fmt/core.h>
using namespace fmt;
#else
# include <format>
#endif
#include <absl/log/log.h>


using namespace std;
using namespace MP;


TEST(Sqlite_test, Base)
{

  LOG(INFO) << format( "sizeof(int)={} sizeof(double)={}", sizeof(int), sizeof(double));

  int rv;
  SqliteDb sd1("test1.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  EXPECT_TRUE(sd1.get());

  // Non-existent database
  SqliteDb sd2("test2.db", SQLITE_OPEN_READONLY) ;  
  EXPECT_FALSE(sd2.get());

  rv = sd1.exec("DROP TABLE IF EXISTS T1");
  EXPECT_TRUE(rv==0);

  rv = sd1.exec("CREATE TABLE T1 (i int, r real, t text, b blob)");
  EXPECT_TRUE(rv==0);

  rv = sd1.exec("INSERT INTO T1 VALUES (1, 1.0, 'First', null)");
  EXPECT_TRUE(rv==0);

  rv = sd1.exec("INSERT INTO T1 VALUES (2, 2.0, 'Second', null)");
  EXPECT_TRUE(rv==0);

  rv = sd1.exec("INSERT INTO T1 VALUES (3, 3.0, 'Third', 3.0)");
  EXPECT_TRUE(rv==0);

  rv = sd1.exec("INSERT INTO T1 VALUES (4, 4.0, 'Fourth', '4th')");
  EXPECT_TRUE(rv==0);

  SqliteStmt sstmt;
  rv = sd1.prepare("SELECT * FROM T1", sstmt);
  ASSERT_TRUE(rv==0);


  // Make a loop and extract columns
  int row = 0;
  do {
    rv = sstmt.step();
    if(rv==SQLITE_DONE) {
      LOG(INFO) << "Sqlite Loop has ended";
      break;
    }
    if(rv==SQLITE_ROW) {
      int col = 0;
      int ival{};
      double dval{};
      string sval;
      any a;
      sstmt.column(col, ival);
      a = sstmt.column(col);
      LOG(INFO) << format( "Row {} Col {} = {} ({} ~ {})", row, col, ival, sizeof(ival), sizeof(a));
      col++;
      sstmt.column(col, dval);
      LOG(INFO) << format( "Row {} Col {} = {}", row, col, dval);
      col++;
      sstmt.column(col, sval);
      LOG(INFO) << format( "Row {} Col {} = {}", row, col, sval);
      col++;
      a = sstmt.column(col);
      LOG(INFO) << format( "Row {} Col {} Type = {} ({} ~ {})", row, col, sstmt.columnTypeStr(col), sizeof(int), sizeof(a));
      row++;
      continue;
    }
    if(rv!=SQLITE_OK) break;

  } while(true);


  auto tuple_in = std::make_tuple(1.0, 2.0, 3.0, 4.0, 5.0);

  {
    LOG(INFO) << "Using new interface";
    SqliteStmt sqlstmt = sd1.stmt("INSERT INTO T1 VALUES (?, ?, ?, ?)");
    LOG(INFO) << "Obtaining new SqliteStmt";
    ASSERT_TRUE(sd1.checkError()==0);
    LOG(INFO) << "Binding the Stmt parameters";
    sqlstmt << 5 << 5.0 << "Fifth" << nullptr;
    rv = sqlstmt.step();
    ASSERT_TRUE(rv==SQLITE_DONE);
    
    sqlstmt.reset();
    sqlstmt << 6 << 6.0 << "Sixth";
    sqlstmt.bindref(4, tuple_in);
    rv = sqlstmt.step();
    ASSERT_TRUE(rv==SQLITE_DONE);
    
    LOG(INFO) << "Finalizing the Stmt";
    rv = sqlstmt.finalize();
    LOG(INFO) << "Before the scope ends";
  }
  LOG(INFO) << "After the scope ends";
  

  rv = sstmt.reset(); // Reset the prepared stement
  EXPECT_TRUE(rv==0);
  row = 0;
  auto tuple_out = std::make_tuple(0.0, 0.0, 0.0, 0.0, 0.0);

  while(sstmt++) {
    int col = 0;
    int ival{-1};
    double dval{-1};
    string s;
    //sstmt.column(col, ival);
    //ival = sstmt.at(col);
    //sstmt >> ival;
    sstmt >> ival >> dval >> s;
    LOG(INFO) << format("{} {} {}", ival, dval, s);

    if(ival==6) {
      sstmt.column(3, tuple_out);
      sd1.checkError();
      LOG(INFO) << format("First elem of tuple={}", (double)get<0>(tuple_out));
      ASSERT_TRUE(std::get<0>(tuple_out)==1.0);
    }

    //LOG(INFO) << format( "Row {} Col {} = {}", row, col, ival);
    //col++;
    //sstmt(col, dval);
    //dval = sstmt.at(col);
    //sstmt >> dval;
    //LOG(INFO) << format( "Row {} Col {} = {}", row, col, dval);
    row++;
  };
  ASSERT_TRUE(sstmt.rc()==SQLITE_DONE);

}


TEST(Sqlite_test, Transactions) {
    SqliteDb db("test_transactions.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    // create a database case that can be written and read
    ASSERT_TRUE(db.get());
    // check that this is, yet again, a valid doable execution

    db.exec("CREATE TABLE IF NOT EXISTS T2 (id INTEGER PRIMARY KEY, name TEXT)");
    // make a table if it doesnt already exist

    db.exec("BEGIN TRANSACTION");
    // add a dummy value
    db.exec("INSERT INTO T2 (id, name) VALUES (1, 'Alice')");
    db.exec("ROLLBACK");
    // check the statement can be rolled back

    SqliteStmt stmt;
    db.prepare("SELECT COUNT(*) FROM T2", stmt);
    // Check what's in table T2. Since it was rolled back, the table should be empty.
    stmt.step(); // Executes the SELECT statement and fetches the result row.

    int count = 0;
    stmt.column(0, count); 

    if (count == 0) {
        LOG(INFO) << "Transaction rolled back successfully. T2 is empty.";
    } else {
        LOG(ERROR) << "Rollback failed. Rows found in T2.";
    }
}
