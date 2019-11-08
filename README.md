# SQLite Stored Procedures

This is a proof-of-concept.

It is the implementation of stored procedures on SQLite as well as some additional commands that could be used outside of stored procedures (on transactions).


## Implemented so far:

1. The `SET` command/statement:

```
SET @variable = {expression}
SET @variable = SELECT ...
```

2. Automatic local variable value binding to statements

```
SELECT name FROM products WHERE id=@variable
```

3. `IF` blocks

```
IF {expression} THEN
...
ELSEIF {expression} THEN
...
ELSE
...
ENDIF
```

4. `DECLARE` statement

```
DECLARE @variable {affinity}
```


## Status

The code is working. You can check the examples in the file `test.c`.

There are other features to be added. More details on the [wiki](https://github.com/aergoio/sqlite-stored-procedures/wiki/SQLite-Stored-Procedures)


## Installing

```
make
sudo make install
```


## Testing

```
make test
```
