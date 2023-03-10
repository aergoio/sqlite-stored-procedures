#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sqlite3.h"
#include "assert.h"

sqlite3 *db;

#include "db_functions.c"

/*

TO BE TESTED ON STORED PROCEDURES:

- RETURN
- DECLARE
- SET
- IF THEN .. ELSEIF .. ELSE .. END IF
- LOOP .. BREAK .. CONTINUE .. END LOOP
- FOREACH .. END LOOP
- CALL

some tests:

- RETURN without arguments
- RETURN with one argument (fixed)
- RETURN with one argument (variable)
- RETURN with many arguments (fixed and variable)
- RETURN with 1 array argument
- RETURN with 1 array of arrays
- RETURN with expression

- SET with ARRAY literal
- SET with ARRAY variable
- SET with SELECT / expression
- SET with INSERT and RETURNING
- SET with UPDATE and RETURNING
- SET with DELETE and RETURNING
- SET with CALL

- IF blocks that are active:
  - on the IF branch
  - on the first ELSEIF branch
  - on the second ELSEIF branch
  - on the ELSE branch
also:
  - IF with RETURN
  - IF with SET
  - IF with CALL
  - IF with LOOP
  - IF with FOREACH
  - IF with nested IF
  - IF with nested LOOP
  - IF with nested FOREACH
- nested IF blocks

- basic LOOP block
- LOOP block with BREAK
- LOOP block with CONTINUE
- LOOP block with RETURN
- nested LOOP blocks

- FOREACH with ARRAY literal
- FOREACH with ARRAY variable
- FOREACH with SELECT
- FOREACH with INSERT and RETURNING
- FOREACH with UPDATE and RETURNING
- FOREACH with DELETE and RETURNING
- FOREACH with CALL
- FOREACH block with BREAK
- FOREACH block with CONTINUE
- FOREACH block with RETURN
- nested FOREACH blocks

- CALL (another procedure) without arguments
- CALL (another procedure) with fixed/literal arguments
- CALL (another procedure) with variables as arguments
- recursive CALL (should fail)


invalid syntax:

- RETURN with many arrays


*/

int main(){
  int rc;

  rc = sqlite3_open(":memory:", &db);
  assert(rc==SQLITE_OK);


  // RETURN without arguments (insert on a table and then returns before another insertion)

  db_execute("CREATE TABLE t1 (id INTEGER PRIMARY KEY, a, b, c, d)");

  db_execute("CREATE PROCEDURE return_nothing() BEGIN "
    "INSERT INTO t1 VALUES (1, 123, 2.5, 'hello world!', x'4142434445');"
    "RETURN;"
    "INSERT INTO t1 VALUES (2, 456, 3.7, 'how are you?', x'782059207a');"
    "END;"
  );

  db_check_many("CALL return_nothing()",
    NULL
  );

  db_check_many("SELECT * FROM t1",
    "1|123|2.5|hello world!|ABCDE",
    NULL
  );


  // RETURN with one argument (fixed)

#if 0
  db_execute("CREATE PROCEDURE return_11() BEGIN RETURN 11; END;");

  // CALL without arguments
  db_check_int("call return_11()", 11);
#endif

  // RETURN with one argument (variable)

  db_execute("CREATE PROCEDURE echo(@value) BEGIN RETURN @value; END;");

  db_check_int("call echo(11)", 11);

  db_check_double("call echo(2.5)", 2.5, 0.0001);
  db_check_str("call echo('hello!')", "hello!");
  db_check_str("call echo('hello''world!')", "hello'world!");
  db_check_str("call echo(x'6120622063')", "a b c");

  db_check_many("CALL echo('hello''world!')",
    "hello'world!",
    NULL
  );

  db_check_many("CALL echo(ARRAY(11,2.5,'hello!',x'6120622063'))",
    "11",
    "2.5",
    "hello!",
    "a b c",
    NULL
  );

  db_check_many("CALL echo( ARRAY( 11 , 2.5 , 'hello!' , x'6120622063' ) )",
    "11",
    "2.5",
    "hello!",
    "a b c",
    NULL
  );

  db_check_many("CALL echo(ARRAY( ARRAY(1,'first',1.1), ARRAY(2,'second',2.2), ARRAY(3,'third',3.3) ))",
    "1|first|1.1",
    "2|second|2.2",
    "3|third|3.3",
    NULL
  );

  db_check_many("CALL echo( ARRAY( ARRAY(11,2.3,'hello!',x'6120622063') , ARRAY(12,2.5,'world!',x'4120422043') ) )",
    "11|2.3|hello!|a b c",
    "12|2.5|world!|A B C",
    NULL
  );

  db_check_many("CALL echo( ARRAY( ARRAY(11,2.3,'hello!',x'6120622063') , ARRAY(12,2.5,'world!',x'4120422043') , ARRAY(13,2.7,'bye!',x'782059207a') ) )",
    "11|2.3|hello!|a b c",
    "12|2.5|world!|A B C",
    "13|2.7|bye!|x Y z",
    NULL
  );


  // RETURN with many literal arguments on the same row
#if 0
  db_execute("CREATE PROCEDURE return_many_literal() BEGIN "
    "RETURN 123, 2.5, 'hello world', x'4142434445';"
    "END;"
  );

  db_check_many("CALL return_many_literal()",
    "123|2.5|hello world|ABCDE",
    NULL
  );


  // RETURN with many literal arguments on different rows (ARRAY literal)

  db_execute("CREATE PROCEDURE return_array_literal() BEGIN "
    "RETURN ARRAY(123, 2.5, 'hello world', x'4142434445');"
    "END;"
  );

  db_check_many("CALL return_array_literal()",
    "123",
    "2.5",
    "hello world",
    "ABCDE",
    NULL
  );
#endif

  // RETURN with many variable arguments on the same row

  db_execute("CREATE PROCEDURE return_many_variable(@a, @b, @c, @d) BEGIN "
    "RETURN @a, @b, @c, @d;"
    "END;"
  );

  db_check_many("CALL return_many_variable(123, 2.5, 'hello world', x'4142434445')",
    "123|2.5|hello world|ABCDE",
    NULL
  );


  // RETURN with array of variables
#if 0
  db_execute("CREATE PROCEDURE return_array_variables(@a, @b, @c, @d) BEGIN "
    "RETURN ARRAY(@a, @b, @c, @d);"
    "END;"
  );

  db_check_many("CALL return_array_variables(123, 2.5, 'hello world', x'4142434445')",
    "123",
    "2.5",
    "hello world",
    "ABCDE",
    NULL
  );
#endif


  // RETURN with expression

  db_execute("CREATE PROCEDURE sum(@a, @b) BEGIN RETURN @a + @b; END;");

  db_check_int("call sum(11, 22)", 33);
  db_check_double("call sum(11.1, 22.2)", 33.3, 0.0001);


  db_execute("CREATE PROCEDURE div(@a, @b) BEGIN RETURN @b / @a + 1; END;");

  db_check_int("call div(11, 33)", 4);
  db_check_double("call div(8, 100.0)", 13.5, 0.0001);


  db_execute("CREATE PROCEDURE concat(@a, @b) BEGIN RETURN @a || @b; END;");

  db_check_str("call concat('hello', 'world')", "helloworld");
  db_check_str("call concat(11, 22)", "1122");


////////////////////////////////////////////////////////////////////////////////
// SET
////////////////////////////////////////////////////////////////////////////////

  // SET with ARRAY literal

  db_execute(
    "CREATE PROCEDURE set_array_literal() BEGIN "
    "SET @arr = ARRAY(11,2.5,'hello!',x'6120622063');"
    "RETURN @arr;"
    "END;"
  );

  db_check_many("CALL set_array_literal()",
    "11",
    "2.5",
    "hello!",
    "a b c",
    NULL
  );


  // SET with ARRAY variable

#if 0
  db_execute(
    "CREATE PROCEDURE set_array_variable(@arr) BEGIN "
    "SET @copy = @arr;"
    "RETURN @copy;"
    "END;"
  );

  db_check_many("CALL set_array_variable( ARRAY(11,2.5,'hello!',x'6120622063') )",
    "11",
    "2.5",
    "hello!",
    "a b c",
    NULL
  );
#endif


  // SET with SELECT (and expression)  (pass one value as argument and multiply by 2)

  db_execute(
    "CREATE PROCEDURE set_select(@a) BEGIN "
    "SET @b = SELECT @a * 2;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL set_select(11)", 22);
  db_check_double("CALL set_select(2.2)", 4.4, 0.0001);


  // SET with SELECT (and expression)  (pass one value as argument and multiply by 2)

  db_execute(
    "CREATE PROCEDURE set_select2(@a, @b) BEGIN "
    "SET @c = 2;"
    "SET @res = SELECT @a * @b + @c;"
    "RETURN @res;"
    "END;"
  );

  db_check_int("CALL set_select2(11, 3)", 35);
  db_check_double("CALL set_select2(2.2, 3)", 8.6, 0.0001);


  // SET with expression  (pass one value as argument and multiply by 3)

  db_execute(
    "CREATE PROCEDURE set_expression(@a) BEGIN "
    "SET @b = 2;"
    "SET @res = @a * @b + 3;"
    "RETURN @res;"
    "END;"
  );

  db_check_int("CALL set_expression(11)", 25);
  db_check_double("CALL set_expression(2.2)", 7.4, 0.0001);


  // SET with INSERT and RETURNING
#if 0
  db_execute("create table test (id integer primary key, a integer, b real, c text, d blob)");

  db_execute(
    "CREATE PROCEDURE set_insert_returning() BEGIN "
    "SET @id = INSERT INTO test (a,b,c,d) VALUES (11,2.5,'hello!',x'6120622063') RETURNING id;"
    "RETURN @id;"
    "END;"
  );

  db_check_int("CALL set_insert_returning()", 1);
  db_check_int("CALL set_insert_returning()", 2);
  db_check_int("CALL set_insert_returning()", 3);


  // SET with UPDATE and RETURNING

  db_execute(
    "CREATE PROCEDURE set_update_returning() BEGIN "
    "SET @id = UPDATE test SET a = 22, b = 3.5, c = 'world!', d = x'4120422043' WHERE id = 1 RETURNING id;"
    "RETURN @id;"
    "END;"
  );

  db_check_int("CALL set_update_returning()", 1);

  db_execute(
    "CREATE PROCEDURE set_update_all_returning() BEGIN "
    "SET @ids = (UPDATE test SET a = 33, b = 4.5, c = 'bye!', d = x'582059207A' RETURNING id);"
    "RETURN @ids;"
    "END;"
  );

  db_check_many("CALL set_update_all_returning()",
    "1",
    "2",
    "3",
    NULL
  );


  // SET with SELECT and many rows

  db_execute(
    "CREATE PROCEDURE set_select_many() BEGIN "
    "SET @a = (SELECT id FROM test);"
    "RETURN @a;"
    "END;"
  );

  db_check_many("CALL set_select_many()",
    "1",
    "2",
    "3",
    NULL
  );


  // SET with DELETE and RETURNING

  db_execute(
    "CREATE PROCEDURE set_delete_returning() BEGIN "
    "SET @ids = (DELETE FROM test WHERE a = 33 RETURNING id);"
    "RETURN @ids;"
    "END;"
  );

  db_check_many("CALL set_delete_returning()",
    "1",
    "2",
    "3",
    NULL
  );
#endif


  // SET with CALL - procedure 1 (set_call) calls procedure 2 (sum) passing 2 values to it

  db_execute(
    "CREATE PROCEDURE set_call(@a, @b) BEGIN "
    "SET @c = CALL sum(@a, @b);"
    "RETURN @c;"
    "END;"
  );

  db_execute(
    "CREATE OR REPLACE PROCEDURE sum(@a, @b) BEGIN "
    "SET @c = @a + @b;"
    "RETURN @c;"
    "END;"
  );

  db_check_int("CALL set_call(11, 22)", 33);
  db_check_double("CALL set_call(2.2, 3.3)", 5.5, 0.0001);


  // SET with CALL - parameters in different order

  db_execute(
    "CREATE PROCEDURE calc(@nsub, @nnum, @ndiv) BEGIN "
    "SET @res = @nnum / @ndiv - @nsub;"
    "RETURN @res;"
    "END;"
  );

  db_execute(
    "CREATE PROCEDURE test_call2(@num, @div, @sub) BEGIN "
    "SET @result = CALL calc(@sub, @num, @div);"
    "RETURN @result;"
    "END;"
  );

  db_check_int("CALL test_call2(10, 2, 1)", 4);
  db_check_double("CALL test_call2(100.0, 8, 2)", 10.5, 0.0001);



////////////////////////////////////////////////////////////////////////////////
// IF .. THEN .. ELSEIF .. THEN .. ELSE .. END IF
////////////////////////////////////////////////////////////////////////////////

/*
- IF blocks that are active:
  - on the IF branch
  - on the first ELSEIF branch
  - on the second ELSEIF branch
  - on the ELSE branch
*/

  // IF with SELECT
#if 0
  db_execute(
    "CREATE PROCEDURE if_select(@a) BEGIN "
    "IF SELECT @a > 10 THEN "
    "  SET @b = 1;"
    "ELSEIF SELECT @a > 5 THEN "
    "  SET @b = 2;"
    "ELSEIF SELECT @a > 0 THEN "
    "  SET @b = 3;"
    "ELSE "
    "  SET @b = 4;"
    "END IF;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL if_select(11)", 1);
  db_check_int("CALL if_select(6)", 2);
  db_check_int("CALL if_select(1)", 3);
  db_check_int("CALL if_select(-1)", 4);
#endif


  // IF with expression

  db_execute(
    "CREATE PROCEDURE if_expression(@a) BEGIN "
    "IF @a > 10 THEN "
    "  SET @b = 1;"
    "ELSEIF @a > 5 THEN "
    "  SET @b = 2;"
    "ELSEIF @a > 0 THEN "
    "  SET @b = 3;"
    "ELSE "
    "  SET @b = 4;"
    "END IF;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL if_expression(11)", 1);
  db_check_int("CALL if_expression(6)", 2);
  db_check_int("CALL if_expression(1)", 3);
  db_check_int("CALL if_expression(-1)", 4);


  // nested IF blocks

  db_execute(
    "CREATE PROCEDURE if_nested(@a) BEGIN "
    "IF @a > 10 THEN "
    "  IF @a > 40 THEN "
    "    SET @b = 11;"
    "  ELSEIF @a > 30 THEN "
    "    SET @b = 12;"
    "  ELSEIF @a > 20 THEN "
    "    SET @b = 13;"
    "  ELSE "
    "    SET @b = 14;"
    "  END IF;"
    "ELSEIF @a > 0 THEN "
    "  IF @a >= 7 THEN "
    "    SET @b = 21;"
    "  ELSEIF @a >= 5 THEN "
    "    SET @b = 22;"
    "  ELSEIF @a >= 3 THEN "
    "    SET @b = 23;"
    "  ELSE "
    "    SET @b = 24;"
    "  END IF;"
    "ELSEIF @a < -10 THEN "
    "  IF @a < -40 THEN "
    "    SET @b = 31;"
    "  ELSEIF @a < -30 THEN "
    "    SET @b = 32;"
    "  ELSEIF @a < -20 THEN "
    "    SET @b = 33;"
    "  ELSE "
    "    SET @b = 34;"
    "  END IF;"
    "ELSE "
    "  IF @a < -7 THEN "
    "    SET @b = 41;"
    "  ELSEIF @a < -5 THEN "
    "    SET @b = 42;"
    "  ELSEIF @a < -3 THEN "
    "    SET @b = 43;"
    "  ELSE "
    "    SET @b = 44;"
    "  END IF;"
    "END IF;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL if_nested(41)", 11);
  db_check_int("CALL if_nested(31)", 12);
  db_check_int("CALL if_nested(21)", 13);
  db_check_int("CALL if_nested(11)", 14);
  db_check_int("CALL if_nested(7)", 21);
  db_check_int("CALL if_nested(5)", 22);
  db_check_int("CALL if_nested(3)", 23);
  db_check_int("CALL if_nested(1)", 24);
  db_check_int("CALL if_nested(-41)", 31);
  db_check_int("CALL if_nested(-31)", 32);
  db_check_int("CALL if_nested(-21)", 33);
  db_check_int("CALL if_nested(-11)", 34);
  db_check_int("CALL if_nested(-8)", 41);
  db_check_int("CALL if_nested(-6)", 42);
  db_check_int("CALL if_nested(-4)", 43);
  db_check_int("CALL if_nested(-1)", 44);


////////////////////////////////////////////////////////////////////////////////
// LOOP .. BREAK .. CONTINUE .. END LOOP
////////////////////////////////////////////////////////////////////////////////

  // LOOP block with BREAK

  db_execute(
    "CREATE PROCEDURE loop_basic(@a) BEGIN "
    "SET @b = 0;"
    "LOOP "
    "  SET @b = @b + 1;"
    "  IF @b > @a THEN "
    "    BREAK;"
    "  END IF;"
    "END LOOP;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL loop_basic(10)", 11);
  db_check_int("CALL loop_basic(5)", 6);

  // LOOP block with CONTINUE

  db_execute(
    "CREATE PROCEDURE loop_continue(@a) BEGIN "
    "SET @b = 0;"
    "LOOP "
    "  SET @b = @b + 10;"
    "  IF @a - @b > 10 THEN "
    "    CONTINUE;"
    "  END IF;"
    "  IF @b > @a THEN "
    "    BREAK;"
    "  END IF;"
    "END LOOP;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL loop_continue(15)", 20);
  db_check_int("CALL loop_continue(25)", 30);


  // LOOP block with RETURN

  db_execute(
    "CREATE PROCEDURE loop_return(@a) BEGIN "
    "SET @b = 0;"
    "LOOP "
    "  SET @b = @b + 1;"
    "  IF @b > @a THEN "
    "    RETURN @b;"
    "  END IF;"
    "END LOOP;"
    "RETURN @b;"
    "END;"
  );

  db_check_int("CALL loop_return(10)", 11);
  db_check_int("CALL loop_return(5)", 6);


  // nested LOOP blocks with BREAK to the first level
  // outer loop: from 1 to @nrows -> concatenate @nrows times the string '|'
  // inner loop: from 1 to @ncols -> concatenate @ncols times the string 'x'

  db_execute(
    "CREATE PROCEDURE loop_nested(@nrows, @ncols) BEGIN "
    "SET @s = '';"
    "LOOP "
    "  SET @i = 0;"
    "  LOOP "
    "    SET @s = @s || 'x';"
    "    SET @i = @i + 1;"
    "    IF @i >= @ncols THEN "
    "      BREAK;"
    "    END IF;"
    "  END LOOP;"
    "  SET @s = @s || '|';"
    "  SET @nrows = @nrows - 1;"
    "  IF @nrows <= 0 THEN "
    "    BREAK;"
    "  END IF;"
    "END LOOP;"
    "RETURN @s;"
    "END;"
  );

  db_check_str("CALL loop_nested(3, 5)", "xxxxx|xxxxx|xxxxx|");
  db_check_str("CALL loop_nested(5, 3)", "xxx|xxx|xxx|xxx|xxx|");


  // nested LOOP blocks with BREAK to the second level (BREAK 2)

  db_execute(
    "CREATE PROCEDURE loop_nested_break2(@nrows, @ncols) BEGIN "
    "SET @s = '';"
    "LOOP "
    "  SET @i = 0;"
    "  LOOP "
    "    SET @s = @s || 'x';"
    "    SET @i = @i + 1;"
    "    IF @i >= @ncols THEN "
    "      BREAK 2;"
    "    END IF;"
    "  END LOOP;"
    "  SET @s = @s || '|';"
    "  SET @nrows = @nrows - 1;"
    "  IF @nrows <= 0 THEN "
    "    BREAK;"
    "  END IF;"
    "END LOOP;"
    "RETURN @s;"
    "END;"
  );

  db_check_str("CALL loop_nested_break2(3, 5)", "xxxxx");
  db_check_str("CALL loop_nested_break2(5, 3)", "xxx");


  // nested LOOP blocks with CONTINUE to the second level

  db_execute(
    "CREATE PROCEDURE loop_nested_continue2(@nrows, @ncols) BEGIN "
    "SET @s = '';"
    "LOOP "
    "  SET @s = @s || '|';"
    "  SET @nrows = @nrows - 1;"
    "  IF @nrows < 0 THEN "
    "    BREAK;"
    "  END IF;"
    "  SET @i = 0;"
    "  LOOP "
    "    SET @s = @s || 'x';"
    "    SET @i = @i + 1;"
    "    IF @i >= @ncols THEN "
    "      CONTINUE 2;"
    "    END IF;"
    "  END LOOP;"
    "  SET @s = @s || '###';"
    "END LOOP;"
    "RETURN @s;"
    "END;"
  );  

  db_check_str("CALL loop_nested_continue2(3, 5)", "|xxxxx|xxxxx|xxxxx|");
  db_check_str("CALL loop_nested_continue2(5, 3)", "|xxx|xxx|xxx|xxx|xxx|");



////////////////////////////////////////////////////////////////////////////////
// FOREACH
////////////////////////////////////////////////////////////////////////////////


  // FOREACH with array variable and 1 simple value (integer) per row

  db_execute(
    "CREATE PROCEDURE sum_array(@arr) BEGIN"
    " SET @sum = 0;"
    " FOREACH @item IN @arr DO"
    "   SET @sum = @sum + @item;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL sum_array( ARRAY(11,22,33) )", 66);


  // SET with SELECT

  db_execute("CREATE TABLE sales (id INTEGER PRIMARY KEY, time)");
  db_execute("CREATE TABLE sale_items (id INTEGER PRIMARY KEY, sale_id INTEGER, prod TEXT, qty INTEGER, price REAL)");

  db_execute(
    "CREATE PROCEDURE add_sale(@prod, @qty, @price) BEGIN "
    "INSERT INTO sales (time) VALUES (datetime('now'));"
    "SET @sale_id = SELECT last_insert_rowid();"
    "INSERT INTO sale_items (sale_id, prod, qty, price) VALUES (@sale_id, @prod, @qty, @price);"
    "RETURN @sale_id;"
    "END;"
  );

  db_check_int("call add_sale('iphone 12',3,1234.90)", 1);

  db_check_many("select * from sale_items",
    "1|1|iphone 12|3|1234.9",
    NULL
  );

  db_check_int("call add_sale('iphone 13',1,1250.00)", 2);

  db_check_many("select * from sale_items",
    "1|1|iphone 12|3|1234.9",
    "2|2|iphone 13|1|1250.0",
    NULL
  );


  // FOREACH with array variable and many values per row

  db_execute(
    "CREATE OR REPLACE PROCEDURE add_sale(@prods) BEGIN "
    "INSERT INTO sales (time) VALUES (datetime('now'));"
    "SET @sale_id = last_insert_rowid();"
    "FOREACH @prod, @qty, @price IN @prods DO "
    "INSERT INTO sale_items (sale_id, prod, qty, price) VALUES (@sale_id, @prod, @qty, @price);"
    "END LOOP;"
    "RETURN @sale_id;"
    "END;"
  );

  db_check_int("CALL add_sale( ARRAY( ARRAY('iphone 14',1,12390.00), ARRAY('ipad',2,5950.00), ARRAY('iwatch',2,490.00) ) )", 3);

  db_check_many("select * from sale_items",
    "1|1|iphone 12|3|1234.9",
    "2|2|iphone 13|1|1250.0",
    "3|3|iphone 14|1|12390.0",
    "4|3|ipad|2|5950.0",
    "5|3|iwatch|2|490.0",
    NULL
  );


  // FOREACH with ARRAY variable - 1 array per row - nested FOREACH

#if 0
  db_execute(
    "CREATE OR REPLACE PROCEDURE add_sale(@prods) BEGIN "
    "INSERT INTO sales (time) VALUES (datetime('now'));"
    "SET @sale_id = last_insert_rowid();"
    "FOREACH @prod IN @prods DO "
    "FOREACH @name, @qty, @price IN @prod DO "
    "INSERT INTO sale_items (sale_id, prod, qty, price) VALUES (@sale_id, @name, @qty, @price);"
    "END LOOP;"
    "END LOOP;"
    "RETURN @sale_id;"
    "END;"
  );

  db_check_int("CALL add_sale( ARRAY( ARRAY('iphone 15',1,12390.00), ARRAY('ipad',2,5950.00), ARRAY('iwatch',2,490.00) ) )", 4);

  db_check_many("select * from sale_items",
    "1|1|iphone 12|3|1234.9",
    "2|2|iphone 13|1|1250.0",
    "3|3|iphone 14|1|12390.0",
    "4|3|ipad|2|5950.0",
    "5|3|iwatch|2|490.0",
    "6|4|iphone 15|1|12390.0",
    "7|4|ipad|2|5950.0",
    "8|4|iwatch|2|490.0",
    NULL
  );
#endif


  // FOREACH with ARRAY literal - 1 simple value per row

  db_execute(
    "CREATE OR REPLACE PROCEDURE sum_array(@arr) BEGIN"
    " SET @sum = 0;"
    " FOREACH @item IN ARRAY(11,22,33) DO"
    "   SET @sum = @sum + @item;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL sum_array( ARRAY(11,22,33) )", 66);


  // FOREACH with ARRAY literal - 1 array per row - nested FOREACH
#if 0
  db_execute(
    "CREATE OR REPLACE PROCEDURE sum_array() BEGIN"
    " SET @sum = 0;"
    " FOREACH @item IN ARRAY( ARRAY(11,22,33), ARRAY(44,55,66) ) DO"
    "   FOREACH @i IN @item DO"
    "     SET @sum = @sum + @i;"
    "   END LOOP;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL sum_array()", 231);
#endif


  // FOREACH with SELECT

  db_execute(
    "CREATE OR REPLACE PROCEDURE sum_sales() BEGIN"
    " SET @sum = 0;"
    " FOREACH @id, @sale_id, @prod, @qty, @price IN SELECT * FROM sale_items DO"
    "   SET @sum = @sum + @qty * @price;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  //db_check_double("CALL sum_sales()", 3*1234.9 + 1*1250.0 + 1*12390.0 + 2*5950.0 + 2*490.0 + 1*12390.0 + 2*5950.0 + 2*490.0, 0.0001);
  db_check_double("CALL sum_sales()", 3*1234.9 + 1*1250.0 + 1*12390.0 + 2*5950.0 + 2*490.0, 0.0001);


  // FOREACH VALUE with SELECT

  db_execute(
    "CREATE OR REPLACE PROCEDURE sum_sales2() BEGIN"
    " SET @sum = 0;"
    " FOREACH VALUE IN SELECT * FROM sale_items DO"
    "   SET @sum = @sum + @qty * @price;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  //db_check_double("CALL sum_sales2()", 3*1234.9 + 1*1250.0 + 1*12390.0 + 2*5950.0 + 2*490.0 + 1*12390.0 + 2*5950.0 + 2*490.0, 0.0001);
  db_check_double("CALL sum_sales2()", 3*1234.9 + 1*1250.0 + 1*12390.0 + 2*5950.0 + 2*490.0, 0.0001);



  // FOREACH with INSERT and RETURNING

#if 0
  db_execute("drop table test");
  db_execute("create table test (id integer primary key, number integer)");

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_insert() BEGIN"
    " SET @ids = '';"
    " FOREACH @id IN INSERT INTO test (number) VALUES (11),(22),(33),(44) RETURNING id DO"
    "   SET @ids = @ids || @id || ',';"
    " END LOOP;"
    " RETURN @ids;"
    "END"
  );

  db_check_str("CALL test_insert()", "1,2,3,4,");


  // FOREACH with UPDATE and RETURNING

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_update() BEGIN"
    " SET @ids = '';"
    " FOREACH @id IN UPDATE test SET number = number + 1 RETURNING id DO"
    "   SET @ids = @ids || @id || ',';"
    " END LOOP;"
    " RETURN @ids;"
    "END"
  );

  db_check_str("CALL test_update()", "1,2,3,4,");


  // FOREACH with DELETE and RETURNING

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_delete() BEGIN"
    " SET @ids = '';"
    " FOREACH @id IN DELETE FROM test RETURNING id DO"
    "   SET @ids = @ids || @id || ',';"
    " END LOOP;"
    " RETURN @ids;"
    "END"
  );

  db_check_str("CALL test_delete()", "1,2,3,4,");
#endif


  // FOREACH with CALL

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_call() BEGIN"
    " SET @sum = 0;"
    " FOREACH @value IN CALL echo( ARRAY(1,2,3,4) ) DO"
    "   SET @sum = @sum + @value;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL test_call()", 10);


  // FOREACH block with BREAK

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_break() BEGIN"
    " SET @sum = 0;"
    " FOREACH @value IN ARRAY(1,2,3,4) DO"
    "   SET @sum = @sum + @value;"
    "   IF @value = 2 THEN"
    "     BREAK;"
    "   END IF;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL test_break()", 3);


  // FOREACH block with CONTINUE

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_continue() BEGIN"
    " SET @sum = 0;"
    " FOREACH @value IN ARRAY(1,2,3,4) DO"
    "   IF @value = 3 THEN"
    "     CONTINUE;"
    "   END IF;"
    "   SET @sum = @sum + @value;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL test_continue()", 7);


  // FOREACH block with RETURN

  db_execute(
    "CREATE OR REPLACE PROCEDURE test_return() BEGIN"
    " SET @sum = 0;"
    " FOREACH @value IN ARRAY(1,2,3,4) DO"
    "   SET @sum = @sum + @value;"
    "   IF @value = 2 THEN"
    "     RETURN @sum;"
    "   END IF;"
    " END LOOP;"
    " RETURN @sum;"
    "END"
  );

  db_check_int("CALL test_return()", 3);




  // CALL (another procedure) without arguments
  // CALL (another procedure) with fixed/literal arguments
  // CALL (another procedure) with variables as arguments


  // recursive CALL - recursive multiplication

  db_execute(
    "CREATE OR REPLACE PROCEDURE mult(@a, @b) BEGIN"
    " IF @b = 0 THEN"
    "   RETURN @b;"
    " ELSE"
    "   SET @b = @b - 1;"
    "   SET @res = CALL mult(@a, @b);"
    "   RETURN @res + @a;"
    " END IF;"
    "END"
  );

  db_check_int("CALL mult(3, 4)", 12);
  db_check_int("CALL mult(9, 3)", 27);






  // functions!
  // recursive functions


/*

CREATE FUNCTION fib (@n) BEGIN
 IF @n < 1 THEN
   SET @n = 0;
 ELSEIF @n > 1 THEN
   SET @n = fib(@n - 1) + fib(@n - 2);
 END IF;
 RETURN @n;
END

*/




  sqlite3_close(db);
  puts("OK. All tests pass!");
  return 0;

}
