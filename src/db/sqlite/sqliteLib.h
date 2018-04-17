#include "../../../3rd/db/sqlite/sqlite3.h"
#include "../../comm/comm.h"
S_NAMESPACE_BEGIN


/*

col_num is zero base.

*/
class SqliteStmt
{
public:
	SqliteStmt()
	{
		m_stmt = nullptr;
		m_has_row = false;
		m_is_done = false;
	}

	~SqliteStmt()
	{
		close();
	}

	void close()
	{
		if (m_stmt != nullptr)
		{
			sqlite3_finalize(m_stmt);
			m_stmt = nullptr;
			m_has_row = false;
			m_is_done = false;
		}
	}

	bool step()
	{
		if (m_stmt == nullptr)
			return false;
		if (m_is_done)
			return false;

		int ret = sqlite3_step(m_stmt);
		if (ret == SQLITE_DONE)
		{
			m_has_row = false;
			m_is_done = true;
		}
		else if (ret == SQLITE_ROW)
		{
			m_has_row = true;
		}
		else
		{
			m_has_row = false;
			return false;
		}
		return true;
	}

	bool getIsStepDone()
	{
		return m_is_done;
	}

	bool getHasStepResultRow()
	{
		return m_has_row;
	}

	bool bindColumInt32(size_t colum_num, int32_t v)
	{
		int ret = sqlite3_bind_int(m_stmt, (int)colum_num + 1, v);
		return ret == SQLITE_OK;
	}

	bool bindColumUint32(size_t colum_num, uint32_t v)
	{
		return bindColumInt32(colum_num, (int32_t)v);
	}

	bool bindColumInt64(size_t colum_num, int64_t v)
	{
		int ret = sqlite3_bind_int64(m_stmt, (int)colum_num + 1, v);
		return ret == SQLITE_OK;
	}

	bool bindColumUint64(size_t colum_num, uint64_t v)
	{
		return bindColumInt64(colum_num, (int64_t)v);
	}

	bool bindColumText(size_t colum_num, const std::string& v)
	{
		int ret = sqlite3_bind_text(m_stmt, (int)colum_num + 1, v.c_str(), -1, SQLITE_TRANSIENT);
		return ret == SQLITE_OK;
	}

	bool getColumInt32(size_t colum_num, int32_t* v)
	{
		int type = sqlite3_column_type(m_stmt, (int)colum_num);
		if (type != SQLITE_INTEGER)
			return false;

		*v = sqlite3_column_int(m_stmt, (int)colum_num);
		return true;
	}

	bool getColumUint32(size_t colum_num, uint32_t* v)
	{
		return getColumInt32(colum_num, (int32_t*)v);
	}

	bool getColumInt64(size_t colum_num, int64_t* v)
	{
		int type = sqlite3_column_type(m_stmt, (int)colum_num);
		if (type != SQLITE_INTEGER)
			return false;

		*v = sqlite3_column_int64(m_stmt, (int)colum_num);
		return true;
	}

	bool getColumUint64(size_t colum_num, uint64_t* v)
	{	
		return getColumInt64(colum_num, (int64_t*)v);
	}

	bool getColumText(size_t colum_num, std::string* v)
	{
		int type = sqlite3_column_type(m_stmt, (int)colum_num);
		if (type != SQLITE_TEXT)
			return false;

		const unsigned char* text = sqlite3_column_text(m_stmt, (int)colum_num);
		if (text != nullptr)
		{
			*v = (const char*)text;
		}

		return true;
	}

	bool getColumBlob(size_t colum_num, Binary* v)
	{
		int type = sqlite3_column_type(m_stmt, (int)colum_num);
		if (type != SQLITE_BLOB)
			return false;

		int byte_count = sqlite3_column_bytes(m_stmt, (int)colum_num);
		if (byte_count > 0)
		{
			const void *blob = sqlite3_column_blob(m_stmt, (int)colum_num);
			v->append((byte_t*)blob, byte_count);
		}

		return true;
	}

	void reset()
	{
		if (m_stmt == nullptr)
			return;

		sqlite3_reset(m_stmt);
		m_has_row = false;
		m_is_done = false;
	}

private:
	friend class SqliteDb;
	sqlite3_stmt* m_stmt;
	bool m_has_row;
	bool m_is_done;
};





class SqliteDb
{
public:
	SqliteDb()
	{
		m_db = nullptr;
	}

	~SqliteDb()
	{
		close();
	}

	bool open(const std::string& file_name)
	{
		if (m_db != nullptr)
			return false;

		int ret = sqlite3_open(file_name.c_str(), &m_db);
		return ret == SQLITE_OK;
	}

	bool exec(const std::string& sql)
	{
		int ret = sqlite3_exec(m_db, sql.c_str(), nullptr, 0, nullptr);
		return ret == SQLITE_OK;
	}

	bool prepare(const std::string& sql, SqliteStmt* stmt)
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare(m_db, sql.c_str(), -1, &s, NULL) != SQLITE_OK)
			return false;

		stmt->close();
		stmt->m_stmt = s;
		return true;
	}

	void close()
	{
		if (m_db != nullptr)
		{
			sqlite3_close(m_db);
			m_db = nullptr;
		}
	}

private:
	sqlite3 * m_db;
};




//SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL
S_NAMESPACE_END
