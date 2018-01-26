/*
 * Copyright (C) 2011 TrintiyCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DB2STORE_H
#define DB2STORE_H

#include "DB2FileLoader.h"
#include "DB2fmt.h"
#include "Log.h"
#include "Field.h"
#include "DatabaseWorkerPool.h"
#include "Implementation/WorldDatabase.h"
#include "DatabaseEnv.h"

#include <vector>

struct SqlDb2
{
    const std::string * formatString;
    const std::string * indexName;
    std::string sqlTableName;
    int32 indexPos;
    int32 sqlIndexPos;
    SqlDb2(const std::string * _filename, const std::string * _format, const std::string * _idname, const char * fmt)
        :formatString(_format), indexName (_idname), sqlIndexPos(0)
    {
        // Convert dbc file name to sql table name
        sqlTableName = *_filename;
        for (uint32 i = 0; i< sqlTableName.size(); ++i)
        {
            if (isalpha(sqlTableName[i]))
                sqlTableName[i] = char(tolower(sqlTableName[i]));
            else if (sqlTableName[i] == '.')
                sqlTableName[i] = '_';
        }

        // Get sql index position
        DB2FileLoader::GetFormatRecordSize(fmt, &indexPos);
        if (indexPos >= 0)
        {
            uint32 uindexPos = uint32(indexPos);
            for (uint32 x = 0; x < formatString->size(); ++x)
            {
                // Count only fields present in sql
                if ((*formatString)[x] == FT_SQL_PRESENT)
                {
                    if (x == uindexPos)
                        break;
                    ++sqlIndexPos;
                }
            }
        }
    }
};

template<class T>
class DB2Storage
{
    typedef std::list<char*> StringPoolList;
    typedef std::vector<T*> DataTableEx;
public:
    explicit DB2Storage(const char *f) : nCount(0), fieldCount(0), fmt(f), indexTable(NULL), m_dataTable(NULL) { }
    ~DB2Storage() { Clear(); }

    T const* LookupEntry(uint32 id) const { return (id>=nCount)?NULL:indexTable[id]; }
    uint32  GetNumRows() const { return nCount; }
    char const* GetFormat() const { return fmt; }
    uint32 GetFieldCount() const { return fieldCount; }

        /// Copies the provided entry and stores it.
        void AddEntry(uint32 id, const T* entry)
        {
            if (LookupEntry(id))
                return;

            if (id >= nCount)
            {
                // reallocate index table
                char** tmpIdxTable = new char*[id+1];
                memset(tmpIdxTable, 0, (id+1) * sizeof(char*));
                memcpy(tmpIdxTable, (char*)indexTable, nCount * sizeof(char*));
                delete[] ((char*)indexTable);
                nCount = id + 1;
                indexTable = (T**)tmpIdxTable;
            }

            T* entryDst = new T;
            memcpy((char*)entryDst, (char*)entry, sizeof(T));
            m_dataTableEx.push_back(entryDst);
            indexTable[id] = entryDst;
        }

    bool Load(char const* fn, SqlDb2 * sql)
    {
        DB2FileLoader db2;
        // Check if load was sucessful, only then continue
        if (!db2.Load(fn, fmt))
            return false;

            uint32 sqlRecordCount = 0;
            uint32 sqlHighestIndex = 0;
            Field* fields = NULL;
            QueryResult result = QueryResult(NULL);
            // Load data from sql
            if (sql)
            {
                std::string query = "SELECT * FROM " + sql->sqlTableName;
                if (sql->indexPos >= 0)
                    query +=" ORDER BY " + *sql->indexName + " DESC";
                query += ';';


                result = WorldDatabase.Query(query.c_str());
                if (result)
                {
                    sqlRecordCount = uint32(result->GetRowCount());
                    if (sql->indexPos >= 0)
                    {
                        fields = result->Fetch();
                        sqlHighestIndex = fields[sql->sqlIndexPos].GetUInt32();
                    }
                    // Check if sql index pos is valid
                    if (int32(result->GetFieldCount()-1) < sql->sqlIndexPos)
                    {
                        TC_LOG_ERROR("server", "Invalid index pos for dbc:'%s'", sql->sqlTableName.c_str());
                        return false;
                    }
                }
            }

            char * sqlDataTable;
        fieldCount = db2.GetCols();

        // load raw non-string data
        m_dataTable = (T*)db2.AutoProduceData(fmt, nCount, (char**&)indexTable,
                sqlRecordCount, sqlHighestIndex, sqlDataTable);

        // create string holders for loaded string fields
        m_stringPoolList.push_back(db2.AutoProduceStringsArrayHolders(fmt, (char*)m_dataTable));

        // load strings from db2 data
        m_stringPoolList.push_back(db2.AutoProduceStrings(fmt, (char*)m_dataTable));

            // Insert sql data into arrays
            if (result)
            {
                if (indexTable)
                {
                    uint32 offset = 0;
                    uint32 rowIndex = db2.GetNumRows();
                    do
                    {
                        if (!fields)
                            fields = result->Fetch();

                        if (sql->indexPos >= 0)
                        {
                            uint32 id = fields[sql->sqlIndexPos].GetUInt32();
                            if (indexTable[id])
                            {
                                TC_LOG_ERROR("server", "Index %d already exists in db2:'%s'", id, sql->sqlTableName.c_str());
                                return false;
                            }
                            indexTable[id]=(T*)&sqlDataTable[offset];
                        }
                        else
                            indexTable[rowIndex]=(T*)&sqlDataTable[offset];
                        uint32 columnNumber = 0;
                        uint32 sqlColumnNumber = 0;

                        for (; columnNumber < sql->formatString->size(); ++columnNumber)
                        {
                            if ((*sql->formatString)[columnNumber] == FT_SQL_ABSENT)
                            {
                                switch (fmt[columnNumber])
                                {
                                    case FT_FLOAT:
                                        *((float*)(&sqlDataTable[offset]))= 0.0f;
                                        offset+=4;
                                        break;
                                    case FT_IND:
                                    case FT_INT:
                                        *((uint32*)(&sqlDataTable[offset]))=uint32(0);
                                        offset+=4;
                                        break;
                                    case FT_BYTE:
                                        *((uint8*)(&sqlDataTable[offset]))=uint8(0);
                                        offset+=1;
                                        break;
                                    case FT_STRING:
                                        // Beginning of the pool - empty string
                                        *((char**)(&sqlDataTable[offset]))=m_stringPoolList.back();
                                        offset+=sizeof(char*);
                                        break;
                                }
                            }
                            else if ((*sql->formatString)[columnNumber] == FT_SQL_PRESENT)
                            {
                                if (sqlColumnNumber > result->GetFieldCount() - 1)
                                {
                                    TC_LOG_ERROR("server", "SQL and DB2 format strings are not matching for table: '%s'", sql->sqlTableName.c_str());
                                    return false;
                                }

                                switch (fmt[columnNumber])
                                {
                                    case FT_FLOAT:
                                        *((float*)(&sqlDataTable[offset]))=fields[sqlColumnNumber].GetFloat();
                                        offset+=4;
                                        break;
                                    case FT_IND:
                                    case FT_INT:
                                        *((uint32*)(&sqlDataTable[offset]))=fields[sqlColumnNumber].GetUInt32();
                                        offset+=4;
                                        break;
                                    case FT_BYTE:
                                        *((uint8*)(&sqlDataTable[offset]))=fields[sqlColumnNumber].GetUInt8();
                                        offset+=1;
                                        break;
                                    case FT_STRING:
                                        TC_LOG_ERROR("server", "Unsupported data type in table '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                        return false;
                                    case FT_SORT:
                                        break;
                                    default:
                                        TC_LOG_ERROR("server", "Unsupported data type in table '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                        return false;
                                }

                                ++sqlColumnNumber;
                            }
                            else
                            {
                                TC_LOG_ERROR("server", "Incorrect sql format string '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                return false;
                            }
                        }

                        fields = NULL;
                        ++rowIndex;
                    }while (result->NextRow());
                }
            }

        // error in db2 file at loading if NULL
        return indexTable!=NULL;
    }

    bool LoadStringsFrom(char const* fn)
    {
        // DBC must be already loaded using Load
        if (!indexTable)
            return false;

        DB2FileLoader db2;
        // Check if load was successful, only then continue
        if (!db2.Load(fn, fmt))
            return false;

        // load strings from another locale dbc data
        m_stringPoolList.push_back(db2.AutoProduceStrings(fmt, (char*)m_dataTable));

        return true;
    }

    void Clear()
    {
        if (!indexTable)
            return;

        delete[] ((char*)indexTable);
        indexTable = NULL;
        delete[] ((char*)m_dataTable);
        m_dataTable = NULL;
            for (typename DataTableEx::const_iterator itr = m_dataTableEx.begin(); itr != m_dataTableEx.end(); ++itr)
                delete *itr;
            m_dataTableEx.clear();

        while (!m_stringPoolList.empty())
        {
            delete[] m_stringPoolList.front();
            m_stringPoolList.pop_front();
        }
        nCount = 0;
    }

    void EraseEntry(uint32 id) { indexTable[id] = NULL; }

private:
    uint32 nCount;
    uint32 fieldCount;
    uint32 recordSize;
    char const* fmt;
    T** indexTable;
    T* m_dataTable;
    DataTableEx m_dataTableEx;
    StringPoolList m_stringPoolList;
};

#endif