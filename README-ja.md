<a href="https://aergo.io"><img height="20%" width="20%" src="https://user-images.githubusercontent.com/7624275/225525458-5aaf74d3-286c-483d-ad2a-9eadc9dbe9ac.png"></a>
が提供する：

# SQLiteのストアドプロシージャ

<p align="right"><a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README.md">English</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-zh.md">中文</a> | <a href="https://github.com/aergoio/sqlite-stored-procedures/blob/master/README-ru.md">Русский</a></p>

例の手順：

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

そして、それを呼び出す方法：

```sql
CALL add_new_sale([['DJI Avata',1,1168.00], ['iPhone 14',1,799.90], ['iWatch',2,249.99]]);
```


## でも、なぜ？

確かに、ほとんどの場合、SQLiteはこれを必要としません。なぜなら、アプリケーションはローカルデータベース上で
SQLコマンドを直接実行できるからです！

しかし、ストアドプロシージャは、**レプリケートされたSQLite**を使用する場合に便利であり、主に
レプリカが異なるプログラミング言語で使用される場合です。

この機能は、SQLiteデータベースの最も
安全なレプリケーションである[AergoLite](https://aergolite.aergo.io/)に追加されます。手順は、この場合ほとんどスマート
コントラクトとして機能します。

もう1つの潜在的な使用法は、`node-sqlite3`を使用したNode.jsで、一部の種類の
トランザクションが管理しにくい場合です。


## サポートされているコマンド

以下のコマンドが利用可能です：

- DECLARE
- SET
- IF .. ELSEIF .. ELSE .. END IF
- LOOP .. BREAK .. CONTINUE .. END LOOP
- FOREACH .. BREAK .. CONTINUE .. END LOOP
- CALL
- RETURN
- RAISE EXCEPTION


## DECLARE

`DECLARE` ステートメントは、変数を宣言するために使用されます。変数はプロシージャにローカルであり、プロシージャの外部からは見えません。

```sql
DECLARE @variable {affinity}
```

親和性はオプションです。指定された場合、次のいずれかになります。

- `TEXT`
- `INTEGER`
- `REAL`
- `BLOB`

親和性は、変数が式で使用されるときに、変数の値を指定された親和性に変換するために使用されます。

現在のバージョンでは、親和性が必要ない場合は変数を宣言する必要はありません。
新しい変数を作成するには、`SET` コマンドを使用してください。


## SET

`SET` ステートメントは、1つ以上の変数に値を割り当てるために使用されます。

この形式があります。

```
SET @variable = {expression};
SET @variable1 [, @variable2...] = {SQL command};
SET @variable = ({SQL command});
```

最初の形式は、変数に値を割り当てるために使用されます。値は式の結果です。

2番目の形式は、SQLコマンドの結果を1つ以上の変数に割り当てるために使用されます。SQLコマンドは単一の行を返さなければなりません。返された列ごとに1つの変数が必要です。

3番目の形式（SQLコマンドの周りに括弧を使用）は、複数の行の結果を1つの変数に割り当てるために使用されます。

サポートされているSQLコマンドは次のとおりです。

- `SELECT`
- `INSERT` with `RETURNING` clause
- `UPDATE` with `RETURNING` clause
- `DELETE` with `RETURNING` clause
- `CALL`

以下はいくつかの例です。

```sql
SET @value = 12.5;
SET @arr = [11, 2.5, 'hello!', x'6120622063'];
SET @result = @qty * @price;
SET @name, @price = SELECT name, price FROM products WHERE id = 123;
SET @ids = (SELECT id FROM products WHERE price > 250);
SET @users = (UPDATE users SET active=1 WHERE active=0 RETURNING id, name);
```


## IFブロック

`IF`文は、条件が真の場合に一連の文を実行するために使用されます。

次の形式を持っています。

```sql
IF {expression} THEN
...
ELSEIF {expression} THEN
...
ELSE
...
END IF;
```

`ELSEIF`と`ELSE`節はオプションです。

ネストされた`IF`ブロックがサポートされています。


## LOOP

ループは、一連の文を複数回実行するために使用されます。

`LOOP`文は次の形式を持っています。

```sql
LOOP
...
END LOOP;
```

ネストされたループがサポートされています。

`BREAK`文は、ループを終了するために使用されます。

`IF`文と一緒に使用することが一般的です。

```sql
LOOP
  ...
  IF {expression} THEN
    BREAK;
  END IF;
  ...
END LOOP;
```

ネストされたループから脱出するには、脱出するループの数を指定します。

```sql
LOOP
  ...
  LOOP
    ...
    BREAK 2;   -- 外側のループから脱出
    ...
  END LOOP;
  ...
END LOOP;
```

`CONTINUE`文は、ループの残りをスキップして次の反復を開始するために使用されます。

親ループで続行するには、降りるレベルを指定します。

```sql
LOOP
  ...
  LOOP
    ...
    CONTINUE 2;  -- 外側のループで実行を続行
    ...
  END LOOP;
  ...
END LOOP;
```


## FOREACH

`FOREACH`文は、値のリストを繰り返し処理するために使用されます。

この形式があります:

```
FOREACH @variable [, @variable2...] IN {input} DO
...
END LOOP;
```

入力は以下のいずれかになります:

- 配列リテラル
- 配列変数
- SELECT
- RETURNING句を含むINSERT
- RETURNING句を含むUPDATE
- RETURNING句を含むDELETE
- CALL

いくつかの例を示します:

* 配列リテラルを使用した場合

```
CREATE OR REPLACE PROCEDURE sum_array() BEGIN
 SET @sum = 0;
 FOREACH @item IN [11,22,33] DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

* 配列変数を使用した場合

```
CREATE OR REPLACE PROCEDURE sum_array(@arr) BEGIN
 SET @sum = 0;
 FOREACH @item IN @arr DO
   SET @sum = @sum + @item;
 END LOOP;
 RETURN @sum;
END
```

もっと役に立つ例:

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

* SELECTを使用した場合

```
FOREACH @id, @sale_id, @prod, @qty, @price IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

各変数を指定する代わりに、`FOREACH VALUE`を使用することができます:

```
FOREACH VALUE IN SELECT * FROM sale_items DO
  ...
END LOOP;
```

`BREAK`および`CONTINUE`文がサポートされており、ネストされた`FOREACH`ループも使用できます


## CALL

ストアドプロシージャを呼び出すために使用されます。

この形式があります：

```sql
CALL {procedure_name} ([{expression} [, {expression}...]]);
```

引数は次のいずれかになります：

- リテラル値
- 変数
- LISTリテラル

いくつかの例を示します：

```sql
CALL compute(11, 22);
CALL compute(@val1, @val2);
CALL add_new_sale([['iphone 14',1,1234.00], ['ipad',1,2345.90], ['iwatch',2,499.99]]);
```

`CALL`文は、`SELECT`文と同様に、プロシージャによって返される結果を返します。

プロシージャ内で実行すると、返された値を変数に割り当てることができます：

```sql
SET @sale_id = CALL add_new_sale([['iphone 14',1,1234.00], ['ipad 12',1,2345.90]]);
```


## RETURN

ストアドプロシージャの実行を停止するために使用されます。

また、呼び出し元に値を返すこともできます。

返される値は次のいずれかになります：

- リテラル値
- 変数
- 式

いくつかの例を示します：

```sql
RETURN 11;
RETURN @var1, @var2;
RETURN @var1 + @var2;
RETURN 11, @var1 + @var2;
```


## RAISE EXCEPTION

ストアドプロシージャの実行を停止し、データベースへのすべての書き込みをロールバックし、エラーメッセージを返すために使用されます。

いくつかの例を示します：

```sql
RAISE EXCEPTION 'some error message';
RAISE EXCEPTION 'some error message %s %s', @var1, @var2;
```




## ステータス

これはベータ版のソフトウェアです。すべてのテストが通っています。バグが見つかった場合は、報告してください。

ファイル[test.c](test/test.c)でいくつかの例を確認できます。


## ビルドとインストール

```
make
sudo make install
```


## テストの実行

```
make test
```
