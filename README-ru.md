<a href="https://aergo.io"><img height="20%" width="20%" src="https://user-images.githubusercontent.com/7624275/225525458-5aaf74d3-286c-483d-ad2a-9eadc9dbe9ac.png"></a>
представляет:

# Хранимые процедуры для SQLite

<p align="right"><a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README.md">English</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-zh.md">中文</a></p>

Пример процедуры:

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

И как вызвать её:

```sql
CALL add_new_sale(ARRAY( ARRAY('DJI Avata',1,1168.00), ARRAY('iPhone 14',1,799.90), ARRAY('iWatch',2,249.99) ));
```


## Подождите, но зачем?

Действительно, в большинстве случаев SQLite не нуждается в этом, потому что приложения могут выполнять
SQL-команды непосредственно на локальной базе данных!

Но хранимые процедуры полезны при использовании **реплицированного SQLite**, особенно когда
реплики используются разными языками программирования.

Эта функция будет добавлена в [AergoLite](https://aergolite.aergo.io/), самую
безопасную репликацию баз данных SQLite. В этом случае процедуры будут работать почти как умные
контракты.

Другое возможное использование - в Node.js с `node-sqlite3`, где некоторые виды
транзакции сложно управлять.


## Поддерживаемые команды

Доступны следующие команды:

- DECLARE
- SET
- IF .. ELSEIF .. ELSE .. END IF
- LOOP .. BREAK .. CONTINUE .. END LOOP
- FOREACH .. BREAK .. CONTINUE .. END LOOP
- CALL
- RETURN
- RAISE EXCEPTION



## DECLARE

Оператор `DECLARE` используется для объявления переменной. Переменная является локальной для процедуры и не видна за пределами процедуры.

```sql
DECLARE @variable {affinity}
```

Аффинность является необязательной. Если указано, оно может быть одним из следующих:

- `TEXT`
- `INTEGER`
- `REAL`
- `BLOB`

Аффинность используется для преобразования значения переменной в указанную аффинность при использовании переменной в выражении.

В текущей версии нам не нужно объявлять переменные, если вам не нужна аффинность.
Просто используйте команду `SET` для создания новых переменных.


## SET

Оператор `SET` используется для присваивания значения одной или нескольким переменным.

Он имеет следующий формат:

```
SET @variable = {expression};
SET @variable1 [, @variable2...] = {SQL command};
SET @variable = ({SQL command});
```

Первая форма используется для присваивания значения переменной. Значение является результатом выражения.

Вторая форма используется для присваивания результата SQL-команды одной или нескольким переменным. SQL-команда должна возвращать одну строку. Для каждого возвращаемого столбца должна быть одна переменная.

Третья форма (с использованием скобок вокруг SQL-команды) используется для присваивания нескольких строк результата одной переменной.

Поддерживаемые SQL-команды:

- `SELECT`
- `INSERT` с `RETURNING` clause
- `UPDATE` с `RETURNING` clause
- `DELETE` с `RETURNING` clause
- `CALL`

Вот несколько примеров:

```sql
SET @value = 12.5;
SET @arr = ARRAY(11, 2.5, 'hello!', x'6120622063');
SET @result = @qty * @price;
SET @name, @price = SELECT name, price FROM products WHERE id = 123;
SET @ids = (SELECT id FROM products WHERE price > 250);
SET @users = (UPDATE users SET active=1 WHERE active=0 RETURNING id, name);
```


## IF блоки

Оператор `IF` используется для выполнения блока операторов, если условие истинно.

Он имеет следующий формат:

```sql
IF {expression} THEN
...
ELSEIF {expression} THEN
...
ELSE
...
END IF;
```

Клаузулы `ELSEIF` и `ELSE` являются необязательными.

Поддерживаются вложенные блоки `IF`.


## LOOP

Циклы используются для многократного выполнения блока операторов.

Оператор `LOOP` имеет следующий формат:

```sql
LOOP
...
END LOOP;
```

Поддерживаются вложенные циклы.

Оператор `BREAK` используется для выхода из цикла.

Обычно его используют с оператором `IF`:

```sql
LOOP
  ...
  IF {expression} THEN
    BREAK;
  END IF;
  ...
END LOOP;
```

Можно выйти из вложенного цикла, указав количество циклов для выхода:

```sql
LOOP
  ...
  LOOP
    ...
    BREAK 2;   -- выход из внешнего цикла
    ...
  END LOOP;
  ...
END LOOP;
```

Оператор `CONTINUE` используется для пропуска оставшейся части цикла и начала следующей итерации.

Можно продолжить выполнение в родительском цикле, указав уровень для спуска:

```sql
LOOP
  ...
  LOOP
    ...
    CONTINUE 2;  -- продолжить выполнение во внешнем цикле
    ...
  END LOOP;
  ...
END LOOP;
```


## FOREACH

Оператор `FOREACH` используется для итерации по списку значений.

Он имеет следующий формат:

```
FOREACH @variable [, @variable2...] IN {input} DO
...
END LOOP;
```

Входные данные могут быть одним из следующих:

- ARRAY литерал
- ARRAY переменная
- SELECT
- INSERT с RETURNING-клаузой
- UPDATE с RETURNING-клаузой
- DELETE с RETURNING-клаузой
- CALL

Вот несколько примеров:

* С ARRAY литералом

```
CREATE OR REPLACE PROCEDURE sum_array() BEGIN
 SET @sum = 0;
 FOREACH @item IN ARRAY(11,22,33) DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

* С ARRAY переменной

```
CREATE OR REPLACE PROCEDURE sum_array(@arr) BEGIN
 SET @sum = 0;
 FOREACH @item IN @arr DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

Более полезный пример:

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

* С SELECT

```
FOREACH @id, @sale_id, @prod, @qty, @price IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

Вместо указания каждой переменной можно использовать `FOREACH VALUE`:

```
FOREACH VALUE IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

Поддерживаются операторы `BREAK` и `CONTINUE`, а также вложенные циклы `FOREACH`.


## ВЫЗОВ

Используется для вызова хранимых процедур.

Имеет следующий формат:

```sql
CALL {procedure_name} ([{expression} [, {expression}...]]);
```

Аргументы могут быть одним из следующих:

- литеральное значение
- переменная
- литерал ARRAY

Вот несколько примеров:

```sql
CALL compute(11, 22);
CALL compute(@val1, @val2);
CALL add_new_sale(ARRAY( ARRAY('iphone 14',1,1234.00), ARRAY('ipad',1,2345.90), ARRAY('iwatch',2,499.99) ));
```

Оператор `CALL` возвращает результат, возвращаемый процедурой, таким же образом, как оператор `SELECT`.

При выполнении внутри процедуры можно присвоить возвращаемое значение переменной:

```sql
SET @sale_id = CALL add_new_sale(ARRAY( ARRAY('iphone 14',1,1234.00), ARRAY('ipad 12',1,2345.90) ));
```


## ВОЗВРАТ

Используется для остановки выполнения хранимой процедуры.

Также может возвращать значения вызывающему.

Возвращаемые значения могут быть одним из следующих:

- литеральные значения
- переменные
- выражения

Вот несколько примеров:

```sql
RETURN 11;
RETURN @var1, @var2;
RETURN @var1 + @var2;
RETURN 11, @var1 + @var2;
```


## ВОЗБУЖДЕНИЕ ИСКЛЮЧЕНИЯ

Используется для остановки выполнения хранимой процедуры, отката всех записей, сделанных в базе данных, и возврата сообщения об ошибке.

Вот несколько примеров:

```sql
RAISE EXCEPTION 'какое-то сообщение об ошибке';
RAISE EXCEPTION 'какое-то сообщение об ошибке %s %s', @var1, @var2;
```



## Статус

Это бета-версия программного обеспечения. Все тесты проходят. Если вы обнаружите какую-либо ошибку, пожалуйста, сообщите о ней.

Вы можете ознакомиться с некоторыми примерами в файле [test.c](test/test.c)


## Сборка и установка

```
make
sudo make install
```


## Запуск тестов

```
make test
```
