#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "assert.h"

sqlite3 *db;

struct product {
  char   name[32];
  int    qty;
  double price;
};

typedef struct product product;

void db_execute(char *cmd){
  char *zErrMsg=0;
  int rc;

  rc = sqlite3_exec(db, cmd, 0, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    printf("FAIL %d: %s\n\tsql: %s\n", rc, zErrMsg, cmd);
    sqlite3_free(zErrMsg);
    exit(1);
  }

}

void db_catch(char *cmd){
  char *zErrMsg=0;
  int rc;

  rc = sqlite3_exec(db, cmd, 0, 0, &zErrMsg);
  if( rc==SQLITE_OK ){
    printf("FAIL: expected error was not generated\n\tsql: %s\n", cmd);
    sqlite3_free(zErrMsg);
    exit(1);
  }

}

void db_execute_many(char *cmd, product *list, int count){
  sqlite3_stmt *stmt=0;
  char *zErrMsg=0; // *zTail=0;
  int rc, i;

  rc = sqlite3_prepare(db, cmd, -1, &stmt, NULL);  //&zTail);
  if( rc!=SQLITE_OK ){
    printf("FAIL %d: %s\n\tsql: %s\n", rc, zErrMsg, cmd);
    sqlite3_free(zErrMsg);
    exit(1);
  }

  //if( sqlite3_bind_parameter_count(stmt)!=3 ){
  //  printf("FAIL: statement uses %d parameters\n\tsql: %s\n", sqlite3_bind_parameter_count(stmt), cmd);
  //  exit(1);
  //}

  for( i=0; i<count; i++ ){
    sqlite3_bind_text(stmt, 2, list[i].name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, list[i].qty);
    sqlite3_bind_double(stmt, 4, list[i].price);

    rc = sqlite3_step(stmt);
    if( rc!=SQLITE_DONE ){
      printf("FAIL %d: on step\nsql: %s\n", rc, cmd);
      exit(1);
    }

    rc = sqlite3_reset(stmt);
    if( rc!=SQLITE_OK ){
      printf("FAIL %d: on reset\nsql: %s\n", rc, cmd);
      exit(1);
    }
  }

  sqlite3_finalize(stmt);

}

void db_check_int(char *cmd, int expected){
  sqlite3_stmt *stmt=0;
  char *zErrMsg=0;  // *zTail=0;
  int rc, returned;

  rc = sqlite3_prepare(db, cmd, -1, &stmt, NULL);  //&zTail);
  if( rc!=SQLITE_OK ){
    printf("FAIL %d: %s\n\tsql: %s\n", rc, zErrMsg, cmd);
    sqlite3_free(zErrMsg);
    exit(1);
  }

  rc = sqlite3_step(stmt);
  if( rc!=SQLITE_ROW ){
    printf("FAIL %d: no row returned\nsql: %s\n", rc, cmd);
    exit(1);
  }

  if( sqlite3_column_count(stmt)!=1 ){
    printf("FAIL: returned %d columns\n\tsql: %s\n", sqlite3_column_count(stmt), cmd);
    exit(1);
  }

  returned = sqlite3_column_int(stmt, 0);
  if( returned!=expected ){
    printf("FAIL: returned=%d expected=%d\n\tsql: %s\n", returned, expected, cmd);
    exit(1);
  }

  rc = sqlite3_step(stmt);
  if( rc!=SQLITE_DONE ){
    printf("FAIL: additional row returned\n\tsql: %s\n", cmd);
    exit(1);
  }

  sqlite3_finalize(stmt);

}

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

  product prod_list[3] = {
      {"aaa", 5, 3.5},
      {"bbb", 1, 2.99},
      {"ccc", 2, 2.49}
  };

  db_execute("insert into sales (datetime) values (datetime('now','localtime'))");
  db_execute("set @sale_id = last_insert_rowid()");

  db_check_int("select @sale_id", 3);

  db_execute_many("insert into sale_items (sale_id,name,qty,price) values (@sale_id, ?, ?, ?)", prod_list, 3);

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


  sqlite3_close(db);
  puts("OK. All tests pass!");
  return 0;

}
