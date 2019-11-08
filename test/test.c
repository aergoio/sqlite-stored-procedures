#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "assert.h"

sqlite3 *db;

#define QUIT_TEST()  exit(1)

/****************************************************************************/

int bind_sql_parameters(sqlite3_stmt *stmt, const char *types, va_list ap){
  int rc, parameter_count, iDest, i;
  char c;

  parameter_count = sqlite3_bind_parameter_count(stmt);

  iDest = 1;
  for(i=0; (c = types[i])!=0; i++){
    if( i+1>parameter_count ) goto loc_invalid;
    if( c=='s' ){
      char *z = va_arg(ap, char*);
      sqlite3_bind_text(stmt, iDest+i, z, -1, SQLITE_TRANSIENT);
    }else if( c=='i' ){
      sqlite3_bind_int64(stmt, iDest+i, va_arg(ap, int));
    }else if( c=='l' ){
      sqlite3_bind_int64(stmt, iDest+i, va_arg(ap, sqlite_int64));
    //}else if( c=='f' ){  -- floats are promoted to double
    //  sqlite3_bind_double(stmt, iDest+i, va_arg(ap, float));
    }else if( c=='d' ){
      sqlite3_bind_double(stmt, iDest+i, va_arg(ap, double));
    }else if( c=='b' ){
      char *ptr = va_arg(ap, char*);
      int size = va_arg(ap, int);
      sqlite3_bind_blob(stmt, iDest+i, ptr, size, SQLITE_TRANSIENT);
      iDest++;
    }else if( c=='@' ){
      /* do not bind value at this position */
    }else{
      goto loc_invalid;
    }
  }

  return SQLITE_OK;

loc_invalid:
  return SQLITE_MISUSE;
}

/****************************************************************************/

void print_error(int rc, char *desc, char *sql, const char *function, int line){

  printf("\n\tFAIL %d: %s\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", rc, desc, sql, function, line);

}

/****************************************************************************/

void db_execute_fn(sqlite3 *db, char *sql, const char *function, int line){
  char *zErrMsg=0;
  int rc;

  rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    print_error(rc, zErrMsg, sql, function, line);
    sqlite3_free(zErrMsg);
    QUIT_TEST();
  }

}

#define db_execute(sql) db_execute_fn(db, sql, __FUNCTION__, __LINE__)

/****************************************************************************/

void db_catch_fn(sqlite3 *db, char *sql, const char *function, int line){
  char *zErrMsg=0;
  int rc;

  rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
  if( rc==SQLITE_OK ){
    print_error(rc, "expected error was not generated", sql, function, line);
    sqlite3_free(zErrMsg);
    QUIT_TEST();
  }

}

#define db_catch(sql) db_catch_fn(db, sql, __FUNCTION__, __LINE__)

/****************************************************************************/

void db_execute_many_fn(sqlite3 *db, char *sql, char *types, int count,
  const char *function, int line, ...
){
  va_list args;
  sqlite3_stmt *stmt=0;
  char *zErrMsg=0;
  int rc, i;

  db_execute_fn(db, "BEGIN", function, line);

  rc = sqlite3_prepare(db, sql, -1, &stmt, NULL);
  if( rc!=SQLITE_OK ){
    print_error(rc, zErrMsg, sql, function, line);
    sqlite3_free(zErrMsg);
    QUIT_TEST();
  }

  va_start(args, line);

  for( i=0; i<count; i++ ){
    rc = bind_sql_parameters(stmt, types, args);
    if( rc!=SQLITE_OK ){
      print_error(rc, "bind parameters", sql, function, line);
      QUIT_TEST();
    }

    rc = sqlite3_step(stmt);
    if( rc!=SQLITE_DONE ){
      print_error(rc, "sqlite3_step", sql, function, line);
      QUIT_TEST();
    }

    rc = sqlite3_reset(stmt);
    if( rc!=SQLITE_OK ){
      print_error(rc, "sqlite3_reset", sql, function, line);
      QUIT_TEST();
    }
  }

  va_end(args);

  sqlite3_finalize(stmt);

  db_execute_fn(db, "COMMIT", function, line);

}

#define db_execute_many(sql,types,count,...) db_execute_many_fn(db, sql, types, count, __FUNCTION__, __LINE__, __VA_ARGS__)

/****************************************************************************/

void db_check_int_fn(sqlite3 *db, char *sql, int expected, const char *function, int line){
  sqlite3_stmt *stmt=0;
  const char *zTail=0;
  int rc, returned;

  do{

    rc = sqlite3_prepare(db, sql, -1, &stmt, &zTail);
    if( rc!=SQLITE_OK ){
      print_error(rc, "sqlite3_prepare", sql, function, line);
      QUIT_TEST();
    }

    rc = sqlite3_step(stmt);

    if( zTail && zTail[0] ){

      if( rc!=SQLITE_DONE ){
        print_error(rc, "multi command returned a row", sql, function, line);
        QUIT_TEST();
      }

      sql = (char*) zTail;

    } else {

      if( rc!=SQLITE_ROW ){
        print_error(rc, "no row returned", sql, function, line);
        QUIT_TEST();
      }

      if( sqlite3_column_count(stmt)!=1 ){
        printf("\n\tFAIL: returned %d columns\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", sqlite3_column_count(stmt), sql, function, line);
        QUIT_TEST();
      }

      returned = sqlite3_column_int(stmt, 0);
      if( returned!=expected ){
        printf("\n\tFAIL: expected=%d returned=%d\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", expected, returned, sql, function, line);
        QUIT_TEST();
      }

      rc = sqlite3_step(stmt);
      if( rc!=SQLITE_DONE ){
        printf("\n\tFAIL: additional row returned\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", sql, function, line);
        QUIT_TEST();
      }

      sql = NULL;

    }

    sqlite3_finalize(stmt);

  } while( sql );

}

#define db_check_int(sql,expected) db_check_int_fn(db, sql, expected, __FUNCTION__, __LINE__)

/****************************************************************************/

void db_check_str_fn(sqlite3 *db, char *sql, char *expected, const char *function, int line){
  sqlite3_stmt *stmt=0;
  const char *zTail=0;
  char *returned;
  int rc;

  do{

    rc = sqlite3_prepare(db, sql, -1, &stmt, &zTail);
    if( rc!=SQLITE_OK ){
      print_error(rc, "sqlite3_prepare", sql, function, line);
      QUIT_TEST();
    }

    rc = sqlite3_step(stmt);

    if( zTail && zTail[0] ){

      if( rc!=SQLITE_DONE ){
        print_error(rc, "multi command returned a row", sql, function, line);
        QUIT_TEST();
      }

      sql = (char*) zTail;

    } else {

      if( rc!=SQLITE_ROW ){
        print_error(rc, "no row returned", sql, function, line);
        QUIT_TEST();
      }

      if( sqlite3_column_count(stmt)!=1 ){
        printf("\n\tFAIL: returned %d columns\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", sqlite3_column_count(stmt), sql, function, line);
        QUIT_TEST();
      }

      returned = (char*) sqlite3_column_text(stmt, 0);
      if( strcmp(returned,expected)!=0 ){
        printf("\n\tFAIL: expected='%s' returned='%s'\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", expected, returned, sql, function, line);
        QUIT_TEST();
      }

      rc = sqlite3_step(stmt);
      if( rc!=SQLITE_DONE ){
        printf("\n\tFAIL: additional row returned\n\tsql: %s\n\tfunction: %s\n\tline: %d\n", sql, function, line);
        QUIT_TEST();
      }

      sql = NULL;

    }

    sqlite3_finalize(stmt);

  } while( sql );

}

#define db_check_str(sql,expected) db_check_str_fn(db, sql, expected, __FUNCTION__, __LINE__)

/****************************************************************************/

void db_check_empty_fn(sqlite3 *db, char *sql, const char *function, int line){
  sqlite3_stmt *stmt=0;
  const char *zTail=0;
  int rc;

  do{

    rc = sqlite3_prepare(db, sql, -1, &stmt, &zTail);
    if( rc!=SQLITE_OK ){
      print_error(rc, "sqlite3_prepare", sql, function, line);
      QUIT_TEST();
    }

    rc = sqlite3_step(stmt);

    if( zTail && zTail[0] ){

      if( rc!=SQLITE_DONE ){
        print_error(rc, "multi command returned a row", sql, function, line);
        QUIT_TEST();
      }

      sql = (char*) zTail;

    } else {

      if( rc==SQLITE_ROW ){
        print_error(rc, "unexpected row returned", sql, function, line);
        QUIT_TEST();
      }

      sql = NULL;

    }

    sqlite3_finalize(stmt);

  } while( sql );

}

#define db_check_empty(sql) db_check_empty_fn(db, sql, __FUNCTION__, __LINE__)

/****************************************************************************/
/****************************************************************************/

int main(){
  int rc;

  rc = sqlite3_open(":memory:", &db);
  assert(rc==SQLITE_OK);


  // SET @variable = {expression}

  db_execute("set @aa = 11 + 22");

  db_check_int("select @aa", 33);
  db_check_int("select @aa + 11", 44);
  db_check_int("select @aa * 2", 66);

  db_execute("set @aa = @aa + 100");

  db_check_int("select @aa", 133);
  db_check_int("select (@aa-1)/2", 66);



  db_execute("set @varB = 22");
  db_execute("set @var = 11");
  db_execute("set @varC = 33");
  db_check_int("select @var", 11);
  db_check_int("select @varB", 22);
  db_check_int("select @varC", 33);

  db_execute("set @xx2 = 22");
  db_execute("set @xx = 11");
  db_check_int("select @xx", 11);
  db_check_int("select @xx2", 22);

  db_execute("set @yy = 11");
  db_execute("set @yy2 = 22");
  db_check_int("select @yy", 11);
  db_check_int("select @yy2", 22);



  db_execute("create table t1 (name)");

  db_execute("insert into t1 values (@aa)");
  db_execute("insert into t1 values (@aa * 2)");

  db_check_int("select count(*) from t1", 2);

  db_check_int("select * from t1 where rowid=1", 133);
  db_check_int("select * from t1 where rowid=2", 266);



  db_execute("create table sales (id INTEGER PRIMARY KEY, datetime DATE)");
  db_execute("create table sale_items (id INTEGER PRIMARY KEY, sale_id INTEGER, name, qty, price)");

  // insert a sale

  db_execute("insert into sales (datetime) values (datetime('now','localtime'))");

  db_execute("set @sale_id = last_insert_rowid()");
  db_execute("set @sale_id2 = select last_insert_rowid()");

  db_check_int("select last_insert_rowid()", 1);
  db_check_int("select @sale_id", 1);
  db_check_int("select @sale_id2", 1);

  db_execute("insert into sale_items (sale_id,name,qty,price) values (@sale_id, 'first', 3, 4.5)");
  db_execute("insert into sale_items (sale_id,name,qty,price) values (@sale_id, 'second', 1, 10)");

  db_check_int("select count(*) from sale_items", 2);

  db_check_int("select sale_id from sale_items where id=1", 1);
  db_check_int("select sale_id from sale_items where id=2", 1);

  // insert another sale

  db_execute("insert into sales (datetime) values (datetime('now','localtime'))");

  db_execute("set @sale_id = last_insert_rowid()");
  db_execute("set @sale_id2 = select last_insert_rowid()");

  db_check_int("select last_insert_rowid()", 2);
  db_check_int("select @sale_id", 2);
  db_check_int("select @sale_id2", 2);

  db_execute("insert into sale_items (sale_id,name,qty,price) values (@sale_id, 'third', 1, 9.5)");
  db_execute("insert into sale_items (sale_id,name,qty,price) values (@sale_id, 'fourth', 2, 7.3)");

  db_check_int("select count(*) from sale_items", 4);

  db_check_int("select sale_id from sale_items where id=3", 2);
  db_check_int("select sale_id from sale_items where id=4", 2);

  // another sale

  db_execute("insert into sales (datetime) values (datetime('now','localtime'))");
  db_execute("set @sale_id = last_insert_rowid()");

  db_check_int("select @sale_id", 3);

  db_execute_many("insert into sale_items (sale_id,name,qty,price) values (@sale_id, ?, ?, ?)",
                  "@sid", 3,
                  "aaa", 5, 3.5,
                  "bbb", 1, 2.99,
                  "ccc", 2, 2.49
  );

  db_check_int("select count(*) from sale_items", 7);

  db_check_int("select sale_id from sale_items where id=5", 3);
  db_check_int("select sale_id from sale_items where id=6", 3);
  db_check_int("select sale_id from sale_items where id=7", 3);

  db_check_int("select count(*) from sale_items where name='aaa'", 1);
  db_check_int("select count(*) from sale_items where name='bbb'", 1);
  db_check_int("select count(*) from sale_items where name='ccc'", 1);

  db_check_int("select id from sale_items where name='aaa'", 5);
  db_check_int("select id from sale_items where name='bbb'", 6);
  db_check_int("select id from sale_items where name='ccc'", 7);

  db_check_int("select id from sale_items where price=3.5", 5);
  db_check_int("select id from sale_items where price=2.99", 6);
  db_check_int("select id from sale_items where price=2.49", 7);

  // SET many variables

  db_execute("set @v1, @v2 = 11, 22");
  db_check_int("select @v1", 11);
  db_check_int("select @v2", 22);

  db_execute("set @m,@mm,@mmm = 10, 20, 30");
  db_check_int("select @m", 10);
  db_check_int("select @mm", 20);
  db_check_int("select @mmm", 30);

  db_execute("set @nnn,@nn,@n = 50, 60, 70");
  db_check_int("select @nnn", 50);
  db_check_int("select @nn", 60);
  db_check_int("select @n", 70);

  // setting the same variable twice - with pre declared variable
  db_catch("set @v1,@v2,@v1 = 50, 60, 70");
  db_catch("set @v1,@v2,@v2 = 50, 60, 70");
  db_catch("set @v1,@v1,@v2 = 50, 60, 70");

  // setting the same variable twice - with non-declared variable
  db_catch("set @pa1,@pa2,@pa1 = 50, 60, 70");
  db_catch("set @pb1,@pb2,@pb2 = 50, 60, 70");
  db_catch("set @pc1,@pc1,@pc2 = 50, 60, 70");

  db_execute("set @name, @qty, @price = SELECT name, qty, price from sale_items where id=5");
  db_check_int("select @name='aaa'", 1);
  db_check_int("select @name='bbb'", 0);
  db_check_int("select @qty", 5);
  db_check_int("select @price=3.5", 1);
  db_check_int("select @price>3.5", 0);
  db_check_int("select @price<3.5", 0);



  // DECLARE @variable {affinity}

  db_execute("declare @pre1 INTEGER");
  db_execute("declare @pre2 INTEGER,@pre3 INTEGER , @pre4  INTEGER ");
  db_execute("set @pre1,@pre2,@pre3,@pre4 = '5','5','5','5' ");
  db_check_int("select typeof(@pre1)='integer'", 1);
  db_check_int("select typeof(@pre2)='integer'", 1);
  db_check_int("select typeof(@pre3)='integer'", 1);
  db_check_int("select typeof(@pre4)='integer'", 1);

  db_execute("set @new1,@new2,@new3,@new4 = '5','5','5','5' ");
  db_check_int("select typeof(@new1)='text'", 1);
  db_check_int("select typeof(@new2)='text'", 1);
  db_check_int("select typeof(@new3)='text'", 1);
  db_check_int("select typeof(@new4)='text'", 1);

  db_execute("declare @pre11 TEXT, @pre12 INTEGER, @pre13 FLOAT");
  db_execute("set @pre11,@pre12,@pre13 = '5.0','5.0','5.0' ");
  db_check_int("select typeof(@pre11)='text'", 1);
  db_check_int("select typeof(@pre12)='integer'", 1);
  db_check_int("select typeof(@pre13)='real'", 1);
  db_check_int("select @pre11='5.0'", 1);
  db_check_int("select @pre12=5", 1);
  db_check_int("select @pre13=5.0", 1);

  db_execute("declare @new5");
  db_check_int("select typeof(@new5)='null'", 1);
  db_check_int("select @new5 is null", 1);

  db_execute("declare @new6 float");
  db_check_int("select typeof(@new6)='null'", 1);
  db_check_int("select @new6 is null", 1);

  db_execute("declare @new11, @new12 TEXT");

  // already declared variable
  db_catch("declare @pre2 TEXT");

  // variables with the same name
  db_catch("declare @new21 TEXT, @new21 TEXT");
  db_catch("declare @new22 TEXT, @new22 INTEGER");
  db_catch("declare @new23 TEXT, @new23");
  db_catch("declare @new24, @new24 TEXT");
  db_catch("declare @new25, @new25");



  // IF blocks

  db_execute("set @aa = 15");
  db_execute("set @bb = 25");

  // first expression is true

  db_execute("if @aa > 10 then");
  db_check_int("select @aa", 15);
  db_execute("else");
  db_check_empty("select @aa");
  db_execute("endif");
  db_check_int("select @aa", 15);

  // first expression is false

  db_execute("if @aa < 10 then");
  db_check_empty("select @aa");
  db_execute("else");
  db_check_int("select @aa", 15);
  db_execute("endif");
  db_check_int("select @aa", 15);

  // first expression is true

  db_check_int("if @aa > 10 then select @aa", 15);
  db_check_int("select @bb", 25);
  db_check_empty("else select @aa");
  db_check_empty("select @bb");
  db_execute("endif");
  db_check_int("select @aa", 15);
  db_check_int("select @bb", 25);

  // first expression is false

  db_check_empty("if @aa < 10 then select @aa");
  db_check_empty("select @bb");
  db_check_int("else select @aa", 15);
  db_check_int("select @bb", 25);
  db_execute("endif");
  db_check_int("select @aa", 15);

  // nested IF

  db_execute("if @aa > 10 then");
  db_check_int("select @aa", 15);
    db_execute("if 5 < 3 then");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_int("select @aa", 15);
    db_execute("if 5 > 3 then");
    db_check_int("select @bb", 25);
    db_execute("endif");
  db_check_int("select @aa", 15);
  db_execute("else");
  db_check_empty("select @aa");
    db_execute("if 7 < 3 then");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_empty("select @aa");
    db_execute("if 7 > 3 then");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_empty("select @aa");
  db_execute("endif");

  db_check_int("select @aa", 15);

  // nested IF with ELSE

  db_execute("if @aa > 10 then");
  db_check_int("select @aa", 15);
    db_execute("if 5 < 3 then");
    db_check_empty("select @bb");
    db_execute("else");
    db_check_int("select @bb", 25);
    db_execute("endif");
  db_check_int("select @aa", 15);
    db_execute("if 5 > 3 then");
    db_check_int("select @bb", 25);
    db_execute("else");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_int("select @aa", 15);
  db_execute("else");
  db_check_empty("select @aa");
    db_execute("if 7 < 3 then");
    db_check_empty("select @bb");
    db_execute("else");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_empty("select @aa");
    db_execute("if 7 > 3 then");
    db_check_empty("select @bb");
    db_execute("else");
    db_check_empty("select @bb");
    db_execute("endif");
  db_check_empty("select @aa");
  db_execute("endif");

  db_check_int("select @aa", 15);


  // ELSEIF - first expression is true

  db_execute("if @aa > 10 then");
  db_check_int("select @aa", 15);
  db_execute("elseif @aa > 5 then");
  db_check_empty("select @aa");
  db_execute("elseif @aa > 7 then");
  db_check_empty("select @aa");
  db_execute("else");
  db_check_empty("select @aa");
  db_execute("endif");

  db_check_int("select @aa", 15);

  db_check_int("if @aa > 10 then select @aa", 15);
  db_check_int("select @bb", 25);
  db_check_empty("elseif @aa > 5 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa > 7 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("else select @aa");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);


  // ELSEIF - first expression is false

  db_execute("if @aa < 10 then");
  db_check_empty("select @aa");
  db_execute("elseif @aa > 10 then");
  db_check_int("select @aa", 15);
  db_execute("elseif @aa > 5 then");
  db_check_empty("select @bb");
  db_execute("else");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);

  db_execute("if @aa < 10 then");
  db_check_empty("select @aa");
  db_execute("elseif @aa < 12 then");
  db_check_empty("select @bb");
  db_execute("elseif @aa > 5 then");
  db_check_int("select @aa", 15);
  db_execute("else");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);

  db_check_empty("if @aa < 10 then select @aa");
  db_check_empty("select @bb");
  db_check_int("elseif @aa > 10 then select @aa", 15);
  db_check_int("select @bb", 25);
  db_check_empty("else select @aa");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);

  db_check_empty("if @aa < 10 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa < 9 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa < 8 then select @aa");
  db_check_empty("select @bb");
  db_check_int("elseif @aa > 10 then select @aa", 15);
  db_check_int("select @bb", 25);
  db_check_empty("elseif @aa > 11 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa < 7 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("else select @aa");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);

  db_check_empty("if @aa < 10 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa < 9 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa < 8 then select @aa");
  db_check_empty("select @bb");
  db_check_int("elseif @aa > 10 then select @aa", 15);
  db_check_int("select @bb", 25);
  db_check_empty("elseif @aa < 7 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("elseif @aa > 11 then select @aa");
  db_check_empty("select @bb");
  db_check_empty("else select @aa");
  db_check_empty("select @bb");
  db_execute("endif");

  db_check_int("select @aa", 15);


  sqlite3_close(db);
  puts("OK. All tests pass!");
  return 0;

}
