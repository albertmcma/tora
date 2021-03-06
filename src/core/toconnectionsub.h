#pragma once

#include "core/tocache.h"
#include "core/toqvalue.h"
#include "core/tora_export.h"

#include <QtCore/QDateTime>

class queryImpl;
class toQueryAbstr;

/** This class is an abstract definition of an actual connection to a database.
 * Each @ref toConnection object can have one or more actual connections to the
 * database depending on long running queries. Normally you will never need to
 * bother with this class if you aren't creating a new database provider
 * (@ref toConnectionProvider).
 */
class TORA_EXPORT toConnectionSub
{
	friend class toQueryAbstr;
    public:

        /** Create connection to database. */
        toConnectionSub() : Query(NULL), Broken(false), Initialized(false), mutex(QMutex::NonRecursive) {}

        /** Close connection. */
        virtual ~toConnectionSub() {}

        // GETTERS

        /** Query current running on connection or NULL. */
        inline toQueryAbstr *query()
        {
            return Query;
        }

        /** Get time when last query on this connection has finished */
        inline QDateTime lastUsed(void)
        {
            return LastUsed;
        }

        // SETTERS

        /** Set query currently running on connection. NULL means none. */
        inline void setQuery(toQueryAbstr *query)
        {
            Query = query;
        }

        // ACTIONS

        /** Cancel anything running on this sub. */
        virtual void cancel(void) { };

        /** Close connection. */
        virtual void close(void) = 0;

        virtual void commit(void) = 0;
        virtual void rollback(void) = 0;

        virtual QString version() = 0;

        virtual toQueryParams sessionId() = 0;

        virtual bool hasTransaction();

        virtual queryImpl* createQuery(toQueryAbstr *query) = 0;

        /** get additional details about db object */
        virtual toQAdditionalDescriptions* decribe(toCache::ObjectRef const&) = 0;

        /** resolve object name (synonym) */
        virtual toCache::ObjectRef resolve(toCache::ObjectRef const& objectName)
        {
            return objectName;
        };

        /** Set time when last query on this connection has finished to "now" */
        inline void setLastUsed(void)
        {
            LastUsed = QDateTime::currentDateTime();
        }

        inline bool isBroken()
        {
            return Broken;
        }

        inline bool isInitialized()
        {
            return Initialized;
        }

        inline void setInitialized(bool initialized)
        {
            Initialized = initialized;
        }

        inline QString const& schema() const
        {
            return Schema;
        }

        inline void setSchema(QString const& schema)
        {
            Schema = schema;
        }

        QString lastSql() const
        {
			QMutexLocker l(&mutex);
        	return LastSql;
        }

    protected:
        void setLastSql(QString const& last)
        {
        	QMutexLocker l(&mutex);
        	LastSql = last;
        }

        toQueryAbstr *Query;
        bool Broken, Initialized;
        QString Schema;
        QDateTime LastUsed; // last time this db connection was actually used

        mutable QMutex mutex;
        QString LastSql;
};
