<a href="https://aergo.io"><img height="20%" width="20%" src="https://user-images.githubusercontent.com/7624275/225525458-5aaf74d3-286c-483d-ad2a-9eadc9dbe9ac.png"></a>
presents:

# Stored Procedures for SQLite

<p align="right"><a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-zh.md">中文</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-ja.md">日本語</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-ru.md">Русский</a></p>

Example procedure:

```
CREATE PROCEDURE add_new_sale(@products) BEGIN 
 INSERT INTO sales (time) VALUES (datetime('now'));
 SET @sale_id = last_insert_rowid();
 FOREACH @prod_id, @qty, @price IN @products DO 
   INSERT INTO sale_items (sale_id, prod_id, qty, price) VALUES (@sale_id, @prod_id, @qty, @price);
 END LOOP;
 RETURN @sale_id;
END;
```

And how to call it:

```sql
CALL add_new_sale([['DJI Avata',1,1168.00], ['iPhone 14',1,799.90], ['iWatch',2,249.99]]);
```


## Wait, but why?

Indeed, in most cases SQLite does not need this because applications can run the
SQL commands directly on the local database!

But stored procedures are useful when using **replicated SQLite**, mainly when
the replicas are used by different programming languages.

This feature will be added to [AergoLite](https://aergolite.aergo.io/), the most
secure replication of SQLite databases. The procedures will work almost as smart
contracts in this case.

Another potential use is in Node.js with `node-sqlite3`, where some kind of
transactions are hard to manage.


## Supported Commands

The following commands are available:

- DECLARE
- SET
- IF .. ELSEIF .. ELSE .. END IF
- LOOP .. BREAK .. CONTINUE .. END LOOP
- FOREACH .. BREAK .. CONTINUE .. END LOOP
- CALL
- RETURN
- RAISE EXCEPTION



## DECLARE

The `DECLARE` statement is used to declare a variable. The variable is local to the procedure and is not visible outside the procedure.

```sql
DECLARE @variable {affinity}
```

The affinity is optional. If specified, it can be one of the following:

- `TEXT`
- `INTEGER`
- `REAL`
- `BLOB`

The affinity is used to convert the value of the variable to the specified affinity when the variable is used in an expression.

In the current version, we do not need to declare the variables if you do not need affinity.
Just use the `SET` command to create new variables.


## SET

The `SET` statement is used to assign a value to one or more variables.

It has this format:

```
SET @variable = {expression};
SET @variable1 [, @variable2...] = {SQL command};
SET @variable = ({SQL command});
```

The first form is used to assign a value to a variable. The value is the result of the expression.

The second form is used to assign the result of a SQL command to one or more variables. The SQL command must return a single row. It must has one variable for each returned column.

The third form (using parentheses around the SQL command) is used to assign multiple rows of result into a single variable.

The supported SQL commands are:

- `SELECT`
- `INSERT` with `RETURNING` clause
- `UPDATE` with `RETURNING` clause
- `DELETE` with `RETURNING` clause
- `CALL`

Here are some examples:

```sql
SET @value = 12.5;
SET @list = [11, 2.5, 'hello!', x'6120622063'];
SET @result = @qty * @price;
SET @name, @price = SELECT name, price FROM products WHERE id = 123;
SET @ids = (SELECT id FROM products WHERE price > 250);
SET @users = (UPDATE users SET active=1 WHERE active=0 RETURNING id, name);
```


## IF blocks

The `IF` statement is used to execute a block of statements if a condition is true.

It has this format:

```sql
IF {expression} THEN
...
ELSEIF {expression} THEN
...
ELSE
...
END IF;
```

The `ELSEIF` and `ELSE` clauses are optional.

Nested `IF` blocks are supported.


## LOOP

Loops are used to execute a block of statements multiple times.

The `LOOP` statement has this format:

```sql
LOOP
...
END LOOP;
```

Nested loops are supported.

The `BREAK` statement is used to exit the loop.

It is common to use it with an `IF` statement:

```sql
LOOP
  ...
  IF {expression} THEN
    BREAK;
  END IF;
  ...
END LOOP;
```

It is possible to break from a nested loop by specifying the number of loops to break:

```sql
LOOP
  ...
  LOOP
    ...
    BREAK 2;   -- break from the outer loop
    ...
  END LOOP;
  ...
END LOOP;
```

The `CONTINUE` statement is used to skip the rest of the loop and start the next iteration.

It is possible to continue on a parent loop by specifying the level to descend:

```sql
LOOP
  ...
  LOOP
    ...
    CONTINUE 2;  -- continue execution on the outer loop
    ...
  END LOOP;
  ...
END LOOP;
```


## FOREACH

The `FOREACH` statement is used to iterate over a list of values.

It has this format:

```
FOREACH @variable [, @variable2...] IN {input} DO
...
END LOOP;
```

The input can be one of the following:

- LIST literal
- LIST variable
- SELECT
- INSERT with RETURNING clause
- UPDATE with RETURNING clause
- DELETE with RETURNING clause
- CALL

Here are some examples:

* With LIST literal

```
CREATE OR REPLACE PROCEDURE sum_array() BEGIN
 SET @sum = 0;
 FOREACH @item IN [11,22,33] DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

* With LIST variable

```
CREATE OR REPLACE PROCEDURE sum_array(@arr) BEGIN
 SET @sum = 0;
 FOREACH @item IN @arr DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

A more useful example:

```
CREATE PROCEDURE add_new_sale(@products) BEGIN 
 INSERT INTO sales (time) VALUES (datetime('now'));
 SET @sale_id = last_insert_rowid();
 FOREACH @prod_id, @qty, @price IN @products DO 
   INSERT INTO sale_items (sale_id, prod_id, qty, price) VALUES (@sale_id, @prod_id, @qty, @price);
 END LOOP;
 RETURN @sale_id;
END;
```

* With SELECT

```
FOREACH @id, @sale_id, @prod, @qty, @price IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

Instead of specifying each variable, it is possible to use `FOREACH VALUE`:

```
FOREACH VALUE IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

The `BREAK` and `CONTINUE` statements are supported, as well as nested `FOREACH` loops.


## CALL

It is used to call stored procedures.

It has this format:

```sql
CALL {procedure_name} ([{expression} [, {expression}...]]);
```

The arguments can be one of the following:

- literal value
- variable
- LIST literal

Here are some examples:

```sql
CALL compute(11, 22);
CALL compute(@val1, @val2);
CALL add_new_sale([['iphone 14',1,1234.00], ['ipad',1,2345.90], ['iwatch',2,499.99]]);
```

The `CALL` statement returns the result returned by the procedure in the same way as a `SELECT` statement.

When run inside of a procedure, it is possible to assign the returned value to a variable:

```sql
SET @sale_id = CALL add_new_sale([['iphone 14',1,1234.00], ['ipad 12',1,2345.90]]);
```


## RETURN

It is used to stop the execution of the stored procedure.

It can also return values to the caller.

The returned values can be one of the following:

- literal values
- variables
- expressions

Here are some examples:

```sql
RETURN 11;
RETURN @var1, @var2;
RETURN @var1 + @var2;
RETURN 11, @var1 + @var2;
```


## RAISE EXCEPTION

It is used to stop the execution of the stored procedure, rolling back all writes made to the database, and return an error message.

Here are some examples:

```sql
RAISE EXCEPTION 'some error message';
RAISE EXCEPTION 'some error message %s %s', @var1, @var2;
```




## Status

This is beta software. All tests are passing. If you find any bug, please report it.

You can check some examples in the file [test.c](test/test.c)


## Build and Install

```
make
sudo make install
```


## Running Tests

```
make test
```
