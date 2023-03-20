<a href="https://aergo.io"><img height="20%" width="20%" src="https://user-images.githubusercontent.com/7624275/225525458-5aaf74d3-286c-483d-ad2a-9eadc9dbe9ac.png"></a>
呈现：

# SQLite 存储过程

<p align="right"><a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README.md">English</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-ja.md">日本語</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-ru.md">Русский</a></p>

示例过程：

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

如何调用它：

```sql
CALL add_new_sale(ARRAY( ARRAY('DJI Avata',1,1168.00), ARRAY('Huawei P50',1,799.90), ARRAY('Xiaomi',2,249.99) ));
```

## 等等，但是为什么？

确实，在大多数情况下，SQLite不需要这个功能，因为应用程序可以直接在本地数据库上运行
SQL命令！

但是当使用复制的 SQLite 时，存储过程很有用，主要是当
副本被不同的编程语言使用时。

这个功能将被添加到 AergoLite，最安全的
SQLite 数据库的复制。在这种情况下，过程将几乎像智能合约一样工作。

另一个潜在的用途是在使用 node-sqlite3 的 Node.js 中，其中某些类型的
事务很难管理。

## 支持的命令

以下命令可用：

* DECLARE
* SET
* IF .. ELSEIF .. ELSE .. END IF
* LOOP .. BREAK .. CONTINUE .. END LOOP
* FOREACH .. BREAK .. CONTINUE .. END LOOP
* CALL
* RETURN
* RAISE EXCEPTION


## DECLARE

`DECLARE` 语句用于声明一个变量。该变量仅限于过程内部，过程外部不可见。

affinity（亲和性）是可选的。如果指定，可以是以下之一：

* `TEXT`
* `INTEGER`
* `REAL`
* `BLOB`

当变量在表达式中使用时，affinity 用于将变量的值转换为指定的亲和性。

在当前版本中，如果不需要 affinity，我们不需要声明变量。
只需使用 `SET` 命令创建新变量。


## SET

`SET` 语句用于为一个或多个变量赋值。

它有以下格式：

```sql
SET @variable = {expression};
SET @variable1 [, @variable2...] = {SQL command};
SET @variable = ({SQL command});
```

第一种形式用于将值赋给一个变量。值是表达式的结果。

第二种形式用于将一个 SQL 命令的结果赋给一个或多个变量。SQL 命令必须返回一行。每个返回的列都必须有一个变量。

第三种形式（在 SQL 命令周围使用括号）用于将多行结果分配给一个变量。

支持的SQL命令有：

- `SELECT`
- 带有 `RETURNING` 子句的 `INSERT`
- 带有 `RETURNING` 子句的 `UPDATE`
- 带有 `RETURNING` 子句的 `DELETE`
- `CALL`

这里有一些示例：

```sql
SET @value = 12.5;
SET @arr = ARRAY(11, 2.5, 'hello!', x'6120622063');
SET @result = @qty * @price;
SET @name, @price = SELECT name, price FROM products WHERE id = 123;
SET @ids = (SELECT id FROM products WHERE price > 250);
SET @users = (UPDATE users SET active=1 WHERE active=0 RETURNING id, name);
```


## IF 语句块

`IF` 语句用于在条件为真时执行一系列语句。

它有这样的格式：

```sql
IF {expression} THEN
...
ELSEIF {expression} THEN
...
ELSE
...
END IF;
```

`ELSEIF` 和 `ELSE` 子句是可选的。

支持嵌套的 `IF` 语句块。


## LOOP

循环用于多次执行一系列语句。

`LOOP` 语句有这样的格式：

```sql
LOOP
...
END LOOP;
```

支持嵌套循环。

`BREAK` 语句用于退出循环。

通常与 `IF` 语句一起使用：

```sql
LOOP
  ...
  IF {expression} THEN
    BREAK;
  END IF;
  ...
END LOOP;
```

可以通过指定要中断的循环数量来从嵌套循环中跳出：

```sql
LOOP
  ...
  LOOP
    ...
    BREAK 2;   -- 从外层循环中跳出
    ...
  END LOOP;
  ...
END LOOP;
```

`CONTINUE` 语句用于跳过循环的其余部分并开始下一次迭代。

可以通过指定要下降的层次来在父循环上继续：

```sql
LOOP
  ...
  LOOP
    ...
    CONTINUE 2;  -- 在外循环继续执行
    ...
  END LOOP;
  ...
END LOOP;
```


## FOREACH

`FOREACH` 语句用于遍历一系列值。

它有这样的格式：

```
FOREACH @variable [, @variable2...] IN {input} DO
...
END LOOP;
```

输入可以是以下之一：

- ARRAY 字面量
- ARRAY 变量
- SELECT
- 带有 RETURNING 子句的 INSERT
- 带有 RETURNING 子句的 UPDATE
- 带有 RETURNING 子句的 DELETE
- CALL

这里有一些示例：

* 使用 ARRAY 字面量

```
CREATE OR REPLACE PROCEDURE sum_array() BEGIN
 SET @sum = 0;
 FOREACH @item IN ARRAY(11,22,33) DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

* 使用 ARRAY 变量

```
CREATE OR REPLACE PROCEDURE sum_array(@arr) BEGIN
 SET @sum = 0;
 FOREACH @item IN @arr DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

一个更有用的例子：

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

* 使用 SELECT

```
FOREACH @id, @sale_id, @prod, @qty, @price IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

可以使用`FOREACH VALUE`代替指定每个变量：

```
FOREACH VALUE IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

支持 `BREAK` 和 `CONTINUE` 语句，以及嵌套的 `FOREACH` 循环。


## CALL

用于调用存储过程。

格式如下：

```sql
CALL {procedure_name} ([{expression} [, {expression}...]]);
```

参数可以是以下之一：

- 字面值
- 变量
- ARRAY 字面值

以下是一些示例：

```sql
CALL compute(11, 22);
CALL compute(@val1, @val2);
CALL add_new_sale(ARRAY( ARRAY('iphone 14',1,1234.00), ARRAY('ipad',1,2345.90), ARRAY('iwatch',2,499.99) ));
```

`CALL` 语句以与 `SELECT` 语句相同的方式返回过程返回的结果。

在过程内运行时，可以将返回的值分配给变量：

```sql
SET @sale_id = CALL add_new_sale(ARRAY( ARRAY('iphone 14',1,1234.00), ARRAY('ipad 12',1,2345.90) ));
```


## RETURN

用于停止执行存储过程。

还可以向调用者返回值。

返回的值可以是以下之一：

- 字面值
- 变量
- 表达式

以下是一些示例：

```sql
RETURN 11;
RETURN @var1, @var2;
RETURN @var1 + @var2;
RETURN 11, @var1 + @var2;
```


## RAISE EXCEPTION

用于停止执行存储过程，回滚对数据库所做的所有写操作，并返回错误消息。

以下是一些示例：

```sql
RAISE EXCEPTION 'some error message';
RAISE EXCEPTION 'some error message %s %s', @var1, @var2;
```




## 状态

这是测试版软件。所有测试都通过了。如果您发现任何错误，请报告。

您可以在文件 [test.c](test/test.c) 中查看一些示例。



## 构建和安装

```
make
sudo make install
```


## 运行测试

```
make test
```
