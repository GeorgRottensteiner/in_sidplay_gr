#include "../nde/NDE.h"
#include <iostream>
#include <time.h>
using namespace std;
int main()
{

	Database db;

	// open the media library's database by passing the filenames of the data file and index file
	// we have to tell the database not to create the table and index
	// paths are hardcoded for this example.
	Table *table = db.OpenTable("C:/Documents and Settings/benski/Application Data/Winamp/Plugins/ml/main.dat", 
		"C:/Documents and Settings/benski/Application Data/Winamp/Plugins/ml/main.idx", false, false);

	cout << "number of records: " << table->GetRecordsCount() << endl;

	// a Scanner lets us iterate through the records
	Scanner *scanner = table->NewScanner(0);

	// scan through each record
	for (scanner->First();!scanner->Eof();scanner->Next())
	{
		time_t time; // time_t -> char * conversion routines require a pointer, so we'll allocate on the stack

		/*
		Scanner::GetFieldByName(char *) returns a "Field *"
		we have to cast it to the appropriate subclass of Field * and hope for the best
		using dynamic_cast<> and making sure it doesn't cast to 0 would be better 
		but this is just an example ... 
		*/

		FilenameField *fileName = (FilenameField *)scanner->GetFieldByName("filename");
		StringField *title = (StringField *)scanner->GetFieldByName("title");
		StringField *artist= (StringField *)scanner->GetFieldByName("artist");
		StringField *album= (StringField *)scanner->GetFieldByName("album");
		IntegerField *year=(IntegerField *)scanner->GetFieldByName("year");
		StringField *genre=(StringField *)scanner->GetFieldByName("genre");
		StringField *comment=(StringField *)scanner->GetFieldByName("comment");
		IntegerField *trackno=(IntegerField *)scanner->GetFieldByName("trackno");
		IntegerField *length=(IntegerField *)scanner->GetFieldByName("length"); // in seconds
		IntegerField *type=(IntegerField *)scanner->GetFieldByName("type");
		IntegerField *lastUpdate=(IntegerField *)scanner->GetFieldByName("lastupd"); // it's an integer field but the data is actually a time_t
		IntegerField *lastPlay=(IntegerField *)scanner->GetFieldByName("lastplay"); // time_t
		IntegerField *rating=(IntegerField *)scanner->GetFieldByName("rating");
		StringField *tuid2=(StringField *)scanner->GetFieldByName("tuid2"); // not sure what this is.  maybe some kind of unique id
		IntegerField *playCount=(IntegerField *)scanner->GetFieldByName("playCount");
		IntegerField *fileTime=(IntegerField *)scanner->GetFieldByName("fileTime"); // time_t
		IntegerField *fileSize=(IntegerField *)scanner->GetFieldByName("fileSize"); // in kilobytes
		IntegerField *bitRate=(IntegerField *)scanner->GetFieldByName("bitRate"); // in kbps

		/*
		sometimes Scanner::GetFieldByName() returns 0 (usually if the field isn't present for this record)
		so we need to check.
		*/
		if (fileName && fileName->GetString())
			cout << "filename: " << fileName->GetString() << endl;
		if (title && title->GetString())
			cout << "title = "<< title->GetString() << endl;
		if (artist && artist->GetString())
			cout << "artist= "<< artist->GetString() << endl;
		if (album && album->GetString())
			cout << "album= "<< album->GetString() << endl;
		if (year)
			cout << "year="<< year->GetValue() << endl;
		if (genre && genre->GetString())
			cout << "genre="<< genre->GetString() << endl;
		if (comment && comment->GetString())
			cout << "comment="<< comment->GetString() << endl;
		if (trackno)
			cout << "trackno="<< trackno->GetValue() << endl;
		if (length)
			cout << "length="<< length->GetValue() << " seconds" << endl;
		if (type)
			cout << "type="<< type->GetValue() << endl;
		if (lastUpdate)
		{
			time = lastUpdate->GetValue();
			cout << "lastUpdate="<< asctime(localtime(&time));
		}


		if (lastPlay)
		{
			time = lastPlay->GetValue();
			cout << "lastPlay="<<  asctime(localtime(&time));
		}
		if (rating)			
			cout << "rating="<< rating->GetValue() << endl;
		if (tuid2 && tuid2->GetString())
			cout << "tuid2="<< tuid2->GetString() << endl;
		if (playCount)
			cout << "playCount="<< playCount->GetValue() << endl;
		if (fileTime)
		{
			time = fileTime->GetValue();
			cout << "fileTime="<< asctime(localtime(&time));
		}
		if (fileSize)
		{
			cout << "fileSize="<< fileSize->GetValue() << " kilobytes" << endl;
		}

		if (bitRate)
		{
			cout << "bitRate="<< bitRate->GetValue() << " kbps" << endl;
		}
		cout << "----------" << endl;
	}

	// cleanup
	table->DeleteScanner(scanner);
	db.CloseTable(table);

	return 0;
}