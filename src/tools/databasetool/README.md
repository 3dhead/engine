# Table descriptions for the databasetool.

The databasetool binary will generate model files for the
table definitions given in `*.tbl` files. You can specify the fields,
the constraints and set an operator for dealing with conflict states.

The operator is taken into account when you execute an insert or
update statement and hit a unique key violation. This cna e.g. be used
to increase points for particular keys. The first time you normally
perform an insert - and the following times (you are still running the
same code on it) you will hit a key violation and thus perform the
insert or update with the operator specified. The default operator is
`set`. See a full list of valid operators below.

To add a new `*.tbl` file to a module and automatically generate code
for that table definition, you have to add the following after your
cmake `add_library(${LIB} ${SRCS})` call:

```
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl ExampleModels.h)
```

`ExampleModels.h` specifies a single header where all generated table models
are put into.

**Example:**

If no classname is specified, the table name will be used with `Model` as postfix.

Table `user` will be generated as `UserModel` class, if no other `classname` was
specified

All models will be put into a `persistence` namespace.

```
table <TABLENAME> {
  classname <STRING> (overrides the automatically determined name)
  namespace <STRING> (c++ namespace where the class is put into)
  field <FIELDNAME> {
    type <FIELDTYPE>
    notnull (optional)
    length <LENGTH> (optional)
	operator <OPERATOR>
    default <DEFAULTVALUE> (optional)
  }
  constraints {
    <FIELDNAME> unique
    <FIELDNAME> key
    <FIELDNAME> primarykey
    <FIELDNAME2> primarykey
    <FIELDNAME> autoincrement
    (<FIELD1>, <FIELD2>) unique
    <FIELDNAME> foreignkey <FOREIGNTABLE> <FOREIGNFIELD>
  }
}
```

## Valid field types
* password
* string
* text
* int
* long
* timestamp
* boolean
* short
* byte

## Valid operators
* set
* add
* subtract

## Other notable features
* Timestamps are handled in UTC.
* When using `ìnt` or `short` as a field type, there is also a setter configured that accepts enums.
