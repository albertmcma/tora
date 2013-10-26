
/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * TOra - An Oracle Toolkit for DBA's and developers
 * 
 * Shared/mixed copyright is held throughout files in this product
 * 
 * Portions Copyright (C) 2000-2001 Underscore AB
 * Portions Copyright (C) 2003-2005 Quest Software, Inc.
 * Portions Copyright (C) 2004-2013 Numerous Other Contributors
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;  only version 2 of
 * the License is valid for this program.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program as the file COPYING.txt; if not, please see
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 * 
 *      As a special exception, you have permission to link this program
 *      with the Oracle Client libraries and distribute executables, as long
 *      as you follow the requirements of the GNU GPL in regard to all of the
 *      software in the executable aside from Oracle client libraries.
 * 
 * All trademarks belong to their respective owners.
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "connection/toqpsqlquery.h"
#include "connection/toqpsqlprovider.h"
#include "connection/toqpsqlquery.h"
#include "connection/toqpsqlconnection.h"
#include "core/tosql.h"
#include "core/tocache.h"
#include "core/utils.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>


// Only attempt to cancel a query using a secondary connection if we
// didn't build with the postgres api.
// Opening a second connection has drawbacks and can fail to
// successfully cancel queries if there is any problem getting a
// second connection, causing crashes.
static toSQL SQLCancelPg("toQSqlConnection:Cancel",
                         "SELECT pg_cancel_backend(:pid)",
                         "",
                         "8.0",
                         "QPSQL");


#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
// seems to be the only way for < 8.0 to not get pg_cancel_backend
static toSQL SQLCancelPgOld("toQSqlConnection:Cancel",
                            "native",
                            "",
                            "",
                            "QPSQL");


void native_cancel(QSqlDriver *driver)
{
    QVariant v = driver->handle();
    if(v.isValid() && v.typeName() == QString("PGconn*"))
    {
#ifdef LIBPQ_DECL_CANCEL
        PGconn *handle = *static_cast<PGconn **>(v.data());
        if(!handle)
            return;

        PGcancel *cancel = PQgetCancel(handle);
        if(!cancel)
            return;

        char *errbuf = new char[1024];
        PQcancel(cancel, errbuf, 1024);
        PQfreeCancel(cancel);
        delete[] errbuf;
#endif
    }
}
#endif

/*
QList<QString> psqlQuery::extraData(const toQSqlProviderAggregate &aggr)
{
	QList<QString> ret;
	const QList<toCache::CacheEntry const*> &objects = query()->connection().getCache().entries(false);
	for (QList<toCache::CacheEntry const*>::const_iterator i = objects.begin(); i != objects.end(); i++)
	{
		if ((*i)->type == toCache::DATABASE && aggr.Type == toQSqlProviderAggregate::AllDatabases)
		{
			ret << (*i)->name.first;
		}
		else if ((*i)->type == toCache::TABLE)
		{
			if (aggr.Type == toQSqlProviderAggregate::AllTables ||
			    (aggr.Type == toQSqlProviderAggregate::CurrentDatabase && (*i)->name.first == query()->connection().user()) ||
			    (aggr.Type == toQSqlProviderAggregate::SpecifiedDatabase && (*i)->name.first == aggr.Data))
				ret << ((*i)->name.first + "." + (*i)->name.second);
		}
	}
	return ret;
}
*/

QSqlQuery* psqlQuery::createQuery(const QString &query)
{
	LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock);
	
	QSqlQuery *ret;
	ret = new QSqlQuery(*ptr);
	//	if (toQMySqlProvider::OnlyForward)
	//		ret->setForwardOnly(true);
	ret->exec(query);
	return ret;
}

psqlQuery::psqlQuery(toQuery *query, toQPSqlConnectionSub *conn)
	: queryImpl(query)
	, Connection(conn)
	, ColumnOrderSize(0)
{
	Column = 0;
	ColumnOrder = NULL;
	EOQ = true;
	Query = NULL;
}

psqlQuery::~psqlQuery()
{
	delete Query;
	delete[] ColumnOrder;
}

void psqlQuery::execute(void)
{
    Query = NULL;
    //    try
    {
    	// TODO psqlQuery::QueryParam may delete contens of the 3rd parameter
    	QList<QString> empty;
    	QString sql = psqlQuery::QueryParam(parseReorder(query()->sql()), query()->params(), empty);
    	Query = createQuery(sql);
    }
    //    catch (const toQSqlProviderAggregate &aggr)
    //    {
    //        ExtraData = extraData(aggr);
    //        if (ExtraData.begin() != ExtraData.end())
    //            CurrentExtra = *ExtraData.begin();
    //
    //        QString t = QueryParam(parseReorder(query()->sql()), query()->params(), ExtraData);
    //        if (t.isEmpty())
    //        {
    //            Utils::toStatusMessage("Nothing to send to aggregate query");
    //            Query = NULL;
    //            EOQ = true;
    //            return;
    //        }
    //        else
    //            Query = createQuery(t);
    //    }

    checkQuery();
}

void psqlQuery::execute(QString const& sql)
{
    QList<QString> empty;
    Query = createQuery(sql);
    checkQuery();
}
void psqlQuery::cancel(void)
{
	if (!Connection->ConnectionID.isEmpty())
	{
		try
		{
			toConnection const &conn = query()->connection();
			const QString &sql = toSQL::sql(SQLCancelPg, conn);
			if(!sql.isEmpty() && sql != "native")
			{
				toConnectionSubLoan c(const_cast<toConnection&>(conn));
				toQuery(c, sql, toQueryParams() << Connection->ConnectionID);
			}
			else
			{
				// don't lock here or deadlock while waiting
				// for query to finish
				QSqlDatabase *c = const_cast<QSqlDatabase *>(&(Connection->Connection));
				//TODO PgSQL only? native_cancel(c->driver());
			}
		}
		catch (...)
		{
			TLOG(1, toDecorator, __HERE__) << "	Ignored exception." << std::endl;
		}
	}
}

QString psqlQuery::parseReorder(const QString &str)
{
	//if (str.toUpper().startsWith("TOAD"))
	//{
	//	std::list<int> order;
	//	int num = -1;
	//	int i;
	//	for (i = 4; i < str.length(); i++)
	//	{
	//		char c = str.at(i).toAscii();
	//		if (isspace(c))
	//			;
	//		else if (isdigit(c))
	//		{
	//			if (num < 0)
	//				num = 0;
	//			num *= 10;
	//			num += c - '0';
	//		}
	//		else if (c == '*')
	//		{
	//			if (num >= 0)
	//				throw QString("Invalid column selection, number before *");
	//			if (c == '*')
	//			{
	//				order.insert(order.end(), -1);
	//				do
	//				{
	//					i++;
	//				}
	//				while (str.at(i).isSpace());
	//				break;
	//			}
	//		}
	//		else
	//		{
	//			if (num < 0)
	//				throw QString("Invalid column selection, number missing");
	//			order.insert(order.end(), num);
	//			num = -1;
	//			if (c != ',')
	//				break;
	//		}
	//	}
	//	ColumnOrderSize = order.size();
	//	if (ColumnOrderSize == 0)
	//		throw QString("Missing column selection");
	//	delete[] ColumnOrder;
	//	ColumnOrder = new int[ColumnOrderSize];
	//	int pos = 0;
	//	for (std::list<int>::iterator j = order.begin(); j != order.end(); j++, pos++)
	//		ColumnOrder[pos] = *j;

	//	return str.mid(i);
	//}
	//else
		return str;
}

toQValue psqlQuery::readValue(void)
{
	if (!Query)
		throw QString::fromLatin1("Fetching from unexecuted query");
	if (EOQ)
		throw QString::fromLatin1("Tried to read past end of query");

	LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock);
	QVariant val;
	bool fixEmpty = false;
	if (ColumnOrder)
	{
		int col = ColumnOrder[Column];
		if (col >= 1)
		{
			val = Query->value(col - 1);
			if (Query->isNull(col - 1))
				val.clear();
			else if ((val.type() == QVariant::Date || val.type() == QVariant::DateTime) && val.isNull())
				fixEmpty = true;
		}
		else if (col == 0)
		{
			val = CurrentExtra;
		}
	}
	else
	{
		val = Query->value(Column);
		if (Query->isNull(Column))
			val.clear();
		else if ((val.type() == QVariant::Date || val.type() == QVariant::DateTime) && val.isNull())
			fixEmpty = true;
	}
	if (fixEmpty)
	{
		switch (val.type())
		{
		case QVariant::Date:
			val = QVariant(QString("0000-00-00"));
			break;
		case QVariant::DateTime:
			val = QVariant(QString("0000-00-00T00:00:00"));
			break;
		default:
			break;
			// Do nothing
		}
	}

	// sapdb marks value as invalid on some views
	// for example tables,indexes etc, so ignore this check
	Column++;
	if ((ColumnOrder && Column == ColumnOrderSize) || (!ColumnOrder && Column == (unsigned int) Record.count()))
	{
		Column = 0;
		EOQ = !Query->next();
	}
	if (EOQ && ExtraData.begin() != ExtraData.end())
	{
		delete Query;
		Query = NULL;
		CurrentExtra = *ExtraData.begin();

		ptr.unlock();
		Query = createQuery(QueryParam(parseReorder(query()->sql()), query()->params(), ExtraData));
		checkQuery();
		ptr.lock();
	}

	return toQValue::fromVariant(val);
}

bool psqlQuery::eof(void)
{
	return EOQ;
}

unsigned long psqlQuery::rowsProcessed(void)
{
	try
	{
		LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock, true);

		if (!Query)
			return 0L;
		return Query->numRowsAffected();
	}
	catch(...)
	{
		return 0L;
	}
}

unsigned psqlQuery::columns(void)
{
	LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock);
	unsigned ret = Record.count();
	if (ColumnOrder)
		ret = ColumnOrderSize;

	return ret;
}

toQColumnDescriptionList psqlQuery::describe(void)
{
	LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock);
	toQColumnDescriptionList ret;
	if (Query && Query->isSelect())
	{
		QString provider = query()->connection().provider();
		QSqlRecord rec = Query->record();
		ret = Describe(provider, rec, ColumnOrder, ColumnOrderSize);
	}
	return ret;
}

void psqlQuery::checkQuery(void) // Must *not* call while locked
{
    LockingPtr<QSqlDatabase> ptr(Connection->Connection, Connection->Lock);

    do
    {
        if (!Query->isActive())
        {
            QString msg = QString::fromLatin1("Query not active ");
            msg += query()->sql();
            throw toQPSqlConnectionSub::ErrorString(Query->lastError(), msg);
        }

        if (Query->isSelect())
        {
            Record = Query->record();
            if (ColumnOrder && ColumnOrder[ColumnOrderSize - 1] == -1)
            {
                unsigned int newsize = ColumnOrderSize + Record.count() - 1;
                int *newalloc = new int[newsize];
                unsigned int i;
                for (i = 0; i < ColumnOrderSize - 1; i++)
                    newalloc[i] = ColumnOrder[i];
                for (int colnum = 1; i < newsize; i++, colnum++)
                    newalloc[i] = colnum;
                delete[] ColumnOrder;
                ColumnOrder = newalloc;
                ColumnOrderSize = newsize;
            }
            EOQ = !Query->next();
            Column = 0;
        }
        else
        {
            EOQ = true;
        }
        if (EOQ && ExtraData.begin() != ExtraData.end())
        {
            delete Query;
            Query = NULL;

            ptr.unlock();
            Query = createQuery(QueryParam(parseReorder(query()->sql()), query()->params(), ExtraData));
            ptr.lock();
        }
    }
    while (ExtraData.begin() != ExtraData.end() && EOQ);
}

toQColumnDescriptionList psqlQuery::Describe(const QString &type, QSqlRecord record, int *order, unsigned int orderSize)
{
	toQColumnDescriptionList ret;
	unsigned int count = record.count();
	if (order)
	{
		count = orderSize;
	}
	for (unsigned int i = 0; i < count; i++)
	{
		toCache::ColumnDescription desc;
		desc.AlignRight = false;
		int col = i;
		if (order)
			col = order[i] - 1;
		if (col == -1)
		{
			desc.Name = "Database";
			desc.Datatype = "STRING";
			desc.Null = false;
			desc.AlignRight = false;
			ret.insert(ret.end(), desc);
			continue;
		}
		desc.Name = record.fieldName(col);
		desc.AlignRight = false;

		int size = 1;

		QSqlField info = record.field(desc.Name);

		info.typeID();
		desc.Datatype = QString::fromLatin1("TBD(DATATYPE)");

		if (info.length() > size)
		{
			desc.Datatype += QString::fromLatin1(" (");
			if (info.length() % size == 0)
				desc.Datatype += QString::number(info.length() / size);
			else
				desc.Datatype += QString::number(info.length());
			if (info.precision() > 0)
			{
				desc.Datatype += QString::fromLatin1(",");
				desc.Datatype += QString::number(info.precision());
			}
			desc.Datatype += QString::fromLatin1(")");
		}
		desc.Null = !info.requiredStatus();

		ret.insert(ret.end(), desc);
	}
	return ret;
}

QString psqlQuery::QueryParam(const QString &in, toQueryParams const &params, QList<QString> &extradata)
{
    QString ret;
    bool inString = false;
    toQueryParams::const_iterator cpar = params.constBegin();
    QString query = QString(in);

    std::map<QString, QString> binds;

    for (int i = 0; i < query.length(); i++)
    {
        QChar rc = query.at(i);
        char  c  = rc.toLatin1();

        char nc = 0;
        if (i + 1 < query.length())
            nc = query.at(i + 1).toLatin1();

        switch (c)
        {
        case '\\':
            ret += rc;
            ret += query.at(++i);
            break;
        case '\'':
            inString = !inString;
            ret += rc;
            break;
        case ':':
            // mostly for postgres-style casts, ignore ::
            if (nc == ':')
            {
                ret += rc;
                ret += nc;
                i++;
                break;
            }

            if (!inString)
            {
                QString nam;
                for (i++; i < query.length(); i++)
                {
                    rc = query.at(i);
                    if (!rc.isLetterOrNumber())
                        break;
                    nam += rc;
                }
                c = rc.toLatin1();
                QString in;
                if (c == '<')
                {
                    for (i++; i < query.length(); i++)
                    {
                        rc = query.at(i);
                        c = rc.toLatin1();
                        if (c == '>')
                        {
                            i++;
                            break;
                        }
                        in += rc;
                    }
                }
                i--;

//                toQSqlProviderAggregate aggr;
//                if (in == "alldatabases")
//                    aggr = toQSqlProviderAggregate(toQSqlProviderAggregate::AllDatabases);
//                else if (in == "alltables")
//                    aggr = toQSqlProviderAggregate(toQSqlProviderAggregate::AllTables);
//                else if (in == "currenttables")
//                    aggr = toQSqlProviderAggregate(toQSqlProviderAggregate::CurrentDatabase);
//                else if (in == "database")
//                    aggr = toQSqlProviderAggregate(toQSqlProviderAggregate::SpecifiedDatabase);

                QString str;
                QString tmp;
//                if (aggr.Type == toQSqlProviderAggregate::None || aggr.Type == toQSqlProviderAggregate::SpecifiedDatabase)
                {
                    if (nam.isEmpty())
                        break;

                    if (binds.find(nam) != binds.end())
                    {
                        ret += binds[nam];
                        break;
                    }
                    if (cpar == params.end())
                        throw toConnection::exception(QString::fromLatin1("Not all bind variables supplied"), i);
                    if ((*cpar).isNull())
                    {
                        str = QString::fromLatin1("NULL");
                    }
                    else if ((*cpar).isInt() || (*cpar).isDouble())
                    {
                        str = QString(*cpar);
                    }
                    tmp = (*cpar);
                    cpar++;
                }
                if (str.isNull())
                {
//                    if (aggr.Type != toQSqlProviderAggregate::None)
//                    {
//                        if (!extradata.isEmpty())
//                        {
////                            WTF?
////                            if ( extradata->empty() )
////                                return QString::null;
//                        	tmp = extradata.first();
//                        	extradata.removeFirst();
//                        }
//                        else
//                        {
//                            aggr.Data = tmp;
//                            throw aggr;
//                        }
//                    }
//                    else
                    {
                        if (in != QString::fromLatin1("noquote"))
                            str += QString::fromLatin1("'");
                    }
                    for (int j = 0; j < tmp.length(); j++)
                    {
                        QChar d = tmp.at(j);
                        switch (d.toLatin1())
                        {
                        case 0:
                            str += QString::fromLatin1("\\0");
                            break;
                        case '\n':
                            str += QString::fromLatin1("\\n");
                            break;
                        case '\t':
                            str += QString::fromLatin1("\\t");
                            break;
                        case '\r':
                            str += QString::fromLatin1("\\r");
                            break;
                        case '\'':
                            str += QString::fromLatin1("\\\'");
                            break;
                        case '\"':
                            str += QString::fromLatin1("\\\"");
                            break;
                        case '\\':
                            str += QString::fromLatin1("\\\\");
                            break;
                        default:
                            str += d;
                        }
                    }
//                    if (in != QString::fromLatin1("noquote") && aggr.Type == toQSqlProviderAggregate::None)
                        str += QString::fromLatin1("'");
                }
                binds[nam] = str;
                ret += str;
                break;
            }
        default:
            ret += rc;
        }
    }
    return ret;
}