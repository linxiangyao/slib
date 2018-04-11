#ifndef __TEST_SQLITE_H
#define __TEST_SQLITE_H
#include "../src/db/sqlite/sqliteLib.h"
#include "../src/util/utilLib.h"
#include "testS.h"
#include "testLog.h"
using namespace std;
USING_NAMESPACE_S


void __testSqlite()
{
	printf("\n__testSqlite ---------------------------------------------------------\n");

	__initLog(ELogLevel_info);

	//primary key not null
	FileUtil::deleteFile("t.db");
	SqliteDb db;
	db.open("t.db");
	const char *sql = "create table if not exists student("
		//"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"uin integer PRIMARY KEY,"
		"user_name text);";

	if (!db.exec(sql))
	{
		slog_e("fail to db.exec");
		return;
	}

	{
		SqliteStmt stmt;
		if (!db.prepare("INSERT INTO student (uin, user_name) VALUES (?, ?)", &stmt))
		{
			slog_e("fail to prepare");
			return;
		}
		stmt.bindColumInt32(0, 1);
		stmt.bindColumText(1, "ryan");
		if (!stmt.step())
		{
			slog_e("fail to step");
			return;
		}

		if (!stmt.getIsStepDone())
		{
			slog_e("!is done");
			return;
		}

		if (stmt.step())
		{
			slog_e("after done can't step!");
			return;
		}

		stmt.reset();
		stmt.bindColumInt32(0, 2);
		stmt.bindColumText(1, "lin");
		if (!stmt.step())
		{
			slog_e("fail to step after reset");
			return;
		}
	}

	{

		SqliteStmt stmt;
		if (!db.prepare("select * from student", &stmt))
		{
			slog_e("fail to prepare");
			return;
		}

		while (true)
		{
			if (!stmt.step())
			{
				slog_e("fail to step");
				return;
			}

			if (stmt.getHasStepResultRow())
			{
				int col0 = 0;
				if (!stmt.getColumInt32(0, &col0))
				{
					slog_e("fail to getColumInt32 0");
					return;
				}

				std::string col1;
				if (!stmt.getColumText(1, &col1))
				{
					slog_e("fail to getColumText 1");
					return;
				}
				slog_i("col0=%0, col1=%1", col0, col1);
			}

			if (stmt.getIsStepDone())
				break;
		}
	}

	printf("__testSqlite ok\n");
}




#endif
