
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
