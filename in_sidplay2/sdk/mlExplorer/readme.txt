The included example simply reads the data from winamp's local media database.
It iterates through each file and outputs information about it.

Even though NDE means 'Nullsoft Database Engine', it's not a database in any normal use of the word.
It's simply a way to store read/write a data file in a structured way.
Querying is very limited and there's no joins or aggregrates.

----
Opening a table

call the OpenTable() method on a Database object.  You can pass "true" for one of the paramters if you want to create the table instead of open it
The "table" and "index" parameters are file names.

----
Accessing records

The normal way to access data is through a "Scanner", which is comprobable to a cursor in SQL.
You ask the table to return a scanner, optionally set filters or a query, and then iterate through.
You can get the value for a field by calling a Scanner::GetFieldByName().
The returned type must be cast to a subclass of Field *.  The appropriate class depends on the datatype

----
Filters

undocumented at this time.

----
Queries

undocumented at this time.  There is a query language of some sort.  There is no documentation, however there may be code in winamp
which uses the query language.

----

The "schema" of the media library's local media database is:
field name datatype comments


filename   String 
title      String
artist     String
album      String
year       Integer
genre      String
comment    String 
trackno    Integer  track number
length     Integer  length in seconds
type       Integer  not sure what these represent
lastupd    Integer  really a time_t value.
lastplay   Integer  really a time_t value
rating     Integer 
tuid2      String   unique id?
playCount  Integer
fileTime   Integer  really a time_t value
fileSize   Integer  in kilobytes
bitRate    Integer  in kbps