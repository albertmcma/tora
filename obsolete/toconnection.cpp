
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

#include "utils.h"
#include "tologger.h"

#include "toconf.h"
#include "toconnection.h"
#include "tohighlightedtext.h"
#include "tomain.h"
#include "tosql.h"
#include "totool.h"
#include "toconnectionpool.h"

#include <string>
#include <time.h>

#include <qapplication.h>
#include <qdir.h>
#include <QtGui/QProgressDialog>
#include <QtGui/QTextStream>
#include <qwidget.h>

#include <QtGui/QString>
#include <QtGui/QDateTime>
#include <QtGui/QPointer>
#include <QtGui/QList>

// A little magic to get lrefresh to work and get a check on qApp

#undef QT_TRANSLATE_NOOP
#define QT_TRANSLATE_NOOP(x,y) QTRANS(x,y)

// Connection provider implementation

std::map<QString, toConnectionProvider *> *toConnectionProvider::Providers;
std::map<QString, toConnectionProvider *> *toConnectionProvider::Types;

void toConnectionProvider::checkAlloc(void)
{
    if (!Providers)
        Providers = new std::map<QString, toConnectionProvider *>;
}

void toConnectionProvider::addProvider(const QString &provider)
{
    checkAlloc();
    Provider = provider;
    (*Providers)[Provider] = this;
}

toConnectionProvider::toConnectionProvider(const QString &provider, bool add
                                          )
{
    Provider = provider;
    if (add
       )
        addProvider(provider);
    if (!Types)
        Types = new std::map<QString, toConnectionProvider *>;
    (*Types)[provider] = this;
}

std::list<QString> toConnectionProvider::providedOptions(const QString &)
{
    std::list<QString> ret;
    return ret;
}

void toConnectionProvider::removeProvider(const QString &provider)
{
    if (Providers)
    {
        std::map<QString, toConnectionProvider *>::iterator i = Providers->find(provider);
        if (i != Providers->end())
            Providers->erase(i);
    }
}

toConnectionProvider::~toConnectionProvider()
{
    try
    {
        if (!Provider.isEmpty())
            removeProvider(Provider);
        std::map<QString, toConnectionProvider *>::iterator i = Types->find(Provider);
        if (i != Types->end())
            Types->erase(i);
    }
    catch (...)
    {
        TLOG(1, toDecorator, __HERE__) << "   Ignored exception." << std::endl;
    }
}

std::list<QString> toConnectionProvider::providedHosts(const QString &)
{
    std::list<QString> ret;
    return ret;
}

std::list<QString> toConnectionProvider::providers()
{
    std::list<QString> ret;
    if (!Providers)
        return ret;
    for (std::map<QString, toConnectionProvider *>::iterator i = Providers->begin(); i != Providers->end(); i++)
        ret.insert(ret.end(), (*i).first);
    return ret;
}

void toConnectionProvider::initializeAll(void)
{
    if (Types)
        for (std::map<QString, toConnectionProvider *>::iterator i = Types->begin();
                i != Types->end(); i++)
            (*i).second->initialize();
}

toConnectionProvider &toConnectionProvider::fetchProvider(const QString &provider)
{
    checkAlloc();
    std::map<QString, toConnectionProvider *>::iterator i = Providers->find(provider);
    if (i == Providers->end())
        throw QT_TRANSLATE_NOOP("toConnectionProvider", "Tried to fetch unknown provider %1").arg(QString(provider));
    return *((*i).second);
}

std::list<QString> toConnectionProvider::options(const QString &provider)
{
    return fetchProvider(provider).providedOptions(provider);
}

QWidget *toConnectionProvider::configurationTab(const QString &provider, QWidget *parent)
{
    return fetchProvider(provider).providerConfigurationTab(provider, parent);
}

toConnection::connectionImpl *toConnectionProvider::connection(const QString &provider,
        toConnection *conn)
{
    return fetchProvider(provider).provideConnection(provider, conn);
}

std::list<QString> toConnectionProvider::hosts(const QString &provider)
{
    return fetchProvider(provider).providedHosts(provider);
}

std::list<QString> toConnectionProvider::databases(const QString &provider, const QString &host,
        const QString &user, const QString &pwd)
{
    return fetchProvider(provider).providedDatabases(provider, host, user, pwd);
}

// const QString &toConnectionProvider::config(const QString &tag, const QString &def) {
//     QString str = Provider;
//     str.append(":");
//     str.append(tag);
//     return toConfigurationSingle::Instance().globalConfig(str, def);
// }
//
// void toConnectionProvider::setConfig(const QString &tag, const QString &def) {
//     QString str = Provider;
//     str.append(":");
//     str.append(tag);
//     toConfigurationSingle::Instance().globalSetConfig(str, def);
// }

QWidget *toConnectionProvider::providerConfigurationTab(const QString &, QWidget *)
{
    return NULL;
}

// toConnection implementation

toConnectionSub* toConnection::addConnection()
{
    toBusy busy;
    toConnectionSub *sub = Connection->createConnection();
    toLocker lock(ConnectionLock)
    ;
    toQList params;
    for (std::list<QString>::iterator i = InitStrings.begin(); i != InitStrings.end(); i++)
    {
        try
        {
            Connection->execute(sub, *i, params);
        }
        TOCATCH
    }

    return sub;
}


void toConnection::closeConnection(toConnectionSub *sub)
{
    if (Connection)
        Connection->closeConnection(sub);
}


toConnection::toConnection(const QString &provider,
                           const QString &user, const QString &password,
                           const QString &host, const QString &database,
                           const QString &schema,
                           const QString &color,
                           const std::set<QString> &options)
    : Provider(provider),
      User(user),
      Password(password),
      Host(host),
      Database(database),
      Schema(schema),
      Color(color),
      Options(options),
      Connection(0)
{
    Connection = toConnectionProvider::connection(Provider, this);
    NeedCommit = Abort = false;
    ConnectionPool = new toConnectionPool(this);
    ///Cache = new toCache(description(false).trimmed());
    CacheNew = new toCacheNew(description(false).trimmed());

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    {
        toLocker clock(ConnectionLock);
        //if (cache && toConfigurationSingle::Instance().objectCache() == toConfiguration::ON_CONNECT)
        //  Cache->readObjects(new cacheObjects(this));

        if (toConfigurationSingle::Instance().objectCache() == toConfiguration::ON_CONNECT)
            CacheNew->readObjects(new cacheObjectsNew(this));
    }
}

toConnection::toConnection(const toConnection &other)
    : Provider(other.Provider),
      User(other.User),
      Password(other.Password),
      Host(other.Host),
      Database(other.Database),
      Schema(other.Schema),
      Color(other.Color),
      Options(other.Options),
      Connection(0),
      //Cache(other.Cache),
      CacheNew(other.CacheNew)
{
    Connection = toConnectionProvider::connection(Provider, this);
    NeedCommit = Abort = false;
    ConnectionPool = new toConnectionPool(this);

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    {
        toExclusiveLocker lock(CacheNew->cacheLock);
        CacheNew->refCount.fetchAndAddAcquire(1);
    }
}

std::list<QString> toConnection::running(void)
{
    toBusy busy;
    toLocker lock(ConnectionLock)
    ;
    std::list<QString> ret;
    // this is insane. disabled code that tried to get sql from
    // running queries
    return ret;
}

std::list<QString> toConnection::primaryKeys()
{
    return Connection->primaryKeys();
}

void toConnection::cancelAll(void)
{
    ConnectionPool->cancelAll();
}

toConnection::~toConnection()
{
    toBusy busy;
    Abort = true;

    //unsigned cacheRefCnt;
    //{
    // toLocker clock(Cache->cacheLock);
    // cacheRefCnt = --Cache->refCount;
    //}
    //if(cacheRefCnt == 0)
    //{
    // this->Cache->writeDiskCache();
    // // this will wait for cacheObjects thread to finish in toCache::~toCache
    // delete this->Cache;
    //}

    unsigned cacheNewRefCnt;
    {
        toExclusiveLocker clock(CacheNew->cacheLock);
        cacheNewRefCnt = CacheNew->refCount.deref();
    }
    if(cacheNewRefCnt == 0)
    {
        ////TODO this->CacheNew->writeDiskCache();
        // this will wait for cacheObjects thread to finish in toCache::~toCache
        delete this->CacheNew;
    }

    {
        closeWidgets();

        ConnectionPool->cancelAll(true);

        delete ConnectionPool;
        ConnectionPool = 0;

        toLocker lock(ConnectionLock);
    }
    delete Connection;
}

toConnectionSub* toConnection::pooledConnection(void)
{
    return ConnectionPool->borrow();
}

void toConnection::commit(toConnectionSub *sub)
{
    toBusy busy;
    Connection->commit(sub);
}

void toConnection::commit(void)
{
    ConnectionPool->commit();
    setNeedCommit(false);
}

void toConnection::rollback(toConnectionSub *sub)
{
    toBusy busy;
    Connection->rollback(sub);
}

void toConnection::rollback(void)
{
    ConnectionPool->rollback();
    setNeedCommit(false);
}

void toConnection::delWidget(QWidget *widget)
{
    for (std::list<QPointer<QWidget> >::iterator i = Widgets.begin();
            i != Widgets.end();
            i++)
    {

        if ((*i) && (*i) == widget)
        {
            Widgets.erase(i);
            break;
        }
    }
}

void toConnection::addWidget(QWidget *widget)
{
    Widgets.insert(Widgets.end(), QPointer<QWidget>(widget));
}

bool toConnection::closeWidgets(void)
{
    for (std::list<QPointer<QWidget> >::iterator i = Widgets.begin();
            i != Widgets.end();
            i++)
    {

        if (!(*i))
            continue;

        // make double sure destroy flag is set
        (*i)->setAttribute(Qt::WA_DeleteOnClose);

        // black magic to close widget's MDI subwindow too
        if ((*i)->parent()
                && (*i)->parent()->metaObject()->className() == QMdiSubWindow::staticMetaObject.className())
        {
            qobject_cast<QMdiSubWindow*>((*i)->parent())->close();
        }

        if (!(*i)->close())
        {
            // close will fail if parent already closed.
            // closing parent will hide children though
            if ((*i)->isVisible())
                return false;
        }
        else
            delete *i;
    }

    Widgets.clear();
    return true;
}

QString toConnection::description(bool version) const
{
    QString ret(User);
    ret += QString::fromLatin1("@");

    if (!toIsMySQL(*this))
        ret += Database;

    if (!Host.isEmpty() && Host != "SQL*Net")
    {
        if (!toIsMySQL(*this))
            ret += QString::fromLatin1(".");
        ret += Host;
    }

    if (version)
    {
        if (!Version.isEmpty())
        {
            ret += QString::fromLatin1(" [");
            ret += Version;
            ret += QString::fromLatin1("]");
        }
    }
    return ret;
}

void toConnection::addInit(const QString &sql)
{
    delInit(sql);
    InitStrings.insert(InitStrings.end(), sql);
}

void toConnection::delInit(const QString &sql)
{
    std::list<QString>::iterator i = InitStrings.begin();
    while (i != InitStrings.end())
    {
        if ((*i) == sql)
            i = InitStrings.erase(i);
        else
            i++;
    }
}

const std::list<QString>& toConnection::initStrings() const
{
    return InitStrings;
}

void toConnection::parse(const QString &sql)
{
    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->parse(*sub, sql);
}

void toConnection::parse(const toSQL &sql)
{
    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->parse(*sub, toSQL::sql(sql, *this));
}

void toConnection::execute(const toSQL &sql, toQList &params)
{
    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->execute(*sub, toSQL::sql(sql, *this), params);
}

void toConnection::execute(const QString &sql, toQList &params)
{
    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->execute(*sub, sql, params);
}

void toConnection::execute(const toSQL &sql,
                           const QString &arg1, const QString &arg2,
                           const QString &arg3, const QString &arg4,
                           const QString &arg5, const QString &arg6,
                           const QString &arg7, const QString &arg8,
                           const QString &arg9)
{
    int numArgs;
    if (!arg9.isNull())
        numArgs = 9;
    else if (!arg8.isNull())
        numArgs = 8;
    else if (!arg7.isNull())
        numArgs = 7;
    else if (!arg6.isNull())
        numArgs = 6;
    else if (!arg5.isNull())
        numArgs = 5;
    else if (!arg4.isNull())
        numArgs = 4;
    else if (!arg3.isNull())
        numArgs = 3;
    else if (!arg2.isNull())
        numArgs = 2;
    else if (!arg1.isNull())
        numArgs = 1;
    else
        numArgs = 0;

    toQList args;
    if (numArgs > 0)
        args.insert(args.end(), arg1);
    if (numArgs > 1)
        args.insert(args.end(), arg2);
    if (numArgs > 2)
        args.insert(args.end(), arg3);
    if (numArgs > 3)
        args.insert(args.end(), arg4);
    if (numArgs > 4)
        args.insert(args.end(), arg5);
    if (numArgs > 5)
        args.insert(args.end(), arg6);
    if (numArgs > 6)
        args.insert(args.end(), arg7);
    if (numArgs > 7)
        args.insert(args.end(), arg8);
    if (numArgs > 8)
        args.insert(args.end(), arg9);

    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->execute(*sub, toSQL::sql(sql, *this), args);
}

void toConnection::execute(const QString &sql,
                           const QString &arg1, const QString &arg2,
                           const QString &arg3, const QString &arg4,
                           const QString &arg5, const QString &arg6,
                           const QString &arg7, const QString &arg8,
                           const QString &arg9)
{
    int numArgs;
    if (!arg9.isNull())
        numArgs = 9;
    else if (!arg8.isNull())
        numArgs = 8;
    else if (!arg7.isNull())
        numArgs = 7;
    else if (!arg6.isNull())
        numArgs = 6;
    else if (!arg5.isNull())
        numArgs = 5;
    else if (!arg4.isNull())
        numArgs = 4;
    else if (!arg3.isNull())
        numArgs = 3;
    else if (!arg2.isNull())
        numArgs = 2;
    else if (!arg1.isNull())
        numArgs = 1;
    else
        numArgs = 0;

    toQList args;
    if (numArgs > 0)
        args.insert(args.end(), arg1);
    if (numArgs > 1)
        args.insert(args.end(), arg2);
    if (numArgs > 2)
        args.insert(args.end(), arg3);
    if (numArgs > 3)
        args.insert(args.end(), arg4);
    if (numArgs > 4)
        args.insert(args.end(), arg5);
    if (numArgs > 5)
        args.insert(args.end(), arg6);
    if (numArgs > 6)
        args.insert(args.end(), arg7);
    if (numArgs > 7)
        args.insert(args.end(), arg8);
    if (numArgs > 8)
        args.insert(args.end(), arg9);

    toBusy busy;

    PoolPtr sub(ConnectionPool);
    Version = Connection->version(*sub);

    Connection->execute(*sub, sql, args);
}

void toConnection::allExecute(const toSQL &sql, toQList &params)
{
    ConnectionPool->executeAll(toSQL::sql(sql, *this), params);
}

void toConnection::allExecute(const QString &sql, toQList &params)
{
    ConnectionPool->executeAll(sql, params);
}

void toConnection::allExecute(const toSQL &sql,
                              const QString &arg1, const QString &arg2,
                              const QString &arg3, const QString &arg4,
                              const QString &arg5, const QString &arg6,
                              const QString &arg7, const QString &arg8,
                              const QString &arg9)
{
    int numArgs;
    if (!arg9.isNull())
        numArgs = 9;
    else if (!arg8.isNull())
        numArgs = 8;
    else if (!arg7.isNull())
        numArgs = 7;
    else if (!arg6.isNull())
        numArgs = 6;
    else if (!arg5.isNull())
        numArgs = 5;
    else if (!arg4.isNull())
        numArgs = 4;
    else if (!arg3.isNull())
        numArgs = 3;
    else if (!arg2.isNull())
        numArgs = 2;
    else if (!arg1.isNull())
        numArgs = 1;
    else
        numArgs = 0;

    toQList args;
    if (numArgs > 0)
        args.insert(args.end(), arg1);
    if (numArgs > 1)
        args.insert(args.end(), arg2);
    if (numArgs > 2)
        args.insert(args.end(), arg3);
    if (numArgs > 3)
        args.insert(args.end(), arg4);
    if (numArgs > 4)
        args.insert(args.end(), arg5);
    if (numArgs > 5)
        args.insert(args.end(), arg6);
    if (numArgs > 6)
        args.insert(args.end(), arg7);
    if (numArgs > 7)
        args.insert(args.end(), arg8);
    if (numArgs > 8)
        args.insert(args.end(), arg9);

    ConnectionPool->executeAll(toSQL::sql(sql, *this), args);
}

void toConnection::allExecute(const QString &sql,
                              const QString &arg1, const QString &arg2,
                              const QString &arg3, const QString &arg4,
                              const QString &arg5, const QString &arg6,
                              const QString &arg7, const QString &arg8,
                              const QString &arg9)
{
    int numArgs;
    if (!arg9.isNull())
        numArgs = 9;
    else if (!arg8.isNull())
        numArgs = 8;
    else if (!arg7.isNull())
        numArgs = 7;
    else if (!arg6.isNull())
        numArgs = 6;
    else if (!arg5.isNull())
        numArgs = 5;
    else if (!arg4.isNull())
        numArgs = 4;
    else if (!arg3.isNull())
        numArgs = 3;
    else if (!arg2.isNull())
        numArgs = 2;
    else if (!arg1.isNull())
        numArgs = 1;
    else
        numArgs = 0;

    toQList args;
    if (numArgs > 0)
        args.insert(args.end(), arg1);
    if (numArgs > 1)
        args.insert(args.end(), arg2);
    if (numArgs > 2)
        args.insert(args.end(), arg3);
    if (numArgs > 3)
        args.insert(args.end(), arg4);
    if (numArgs > 4)
        args.insert(args.end(), arg5);
    if (numArgs > 5)
        args.insert(args.end(), arg6);
    if (numArgs > 6)
        args.insert(args.end(), arg7);
    if (numArgs > 7)
        args.insert(args.end(), arg8);
    if (numArgs > 8)
        args.insert(args.end(), arg9);

    ConnectionPool->executeAll(sql, args);
}

const QString &toConnection::provider(void) const
{
    return Provider;
}

/* This method runs as a separate thread executed from:
   toCache::readObjects(toTask * t)
*/
void toConnection::cacheObjectsNew::run()
{
    /* Increase the semaphore to "unlock" parent thread, it's waiting for our start */
    Connection->CacheNew->ReadingThreadSemaphore.up();
    Connection->CacheNew->setCacheState(toCacheNew::READING_FROM_DB);

    QList<toCacheNew::CacheEntry*> objs = Connection->Connection->objectNamesNew();
    if (!Connection->Abort)
    {
        Q_FOREACH(toCacheNew::CacheEntry * o, objs)
        {
            Connection->CacheNew->addEntry(o);
        }
    }

    Connection->CacheNew->ownersRead = true;
    Connection->CacheNew->setCacheState(toCacheNew::DONE);
};

//void toConnection::cacheObjects::run()
//{
//  bool diskloaded = false;
//
//  ///try
//  ///{
//  /// Connection->Cache->setCacheState(toCache::READING_OBJECTS);
//  /// /* Increase the semaphore to "unlock" parent thread */
//      ///Connection->Cache->ReadingThread.up();
//
//  /// diskloaded = Connection->Cache->loadDiskCache();
//  /// if (!diskloaded && !Connection->Abort)
//  /// {
//  ///     const QList<objectName> &n = Connection->Connection->objectNames();
//  ///     if (!Connection->Abort)
//  ///         Connection->Cache->setObjectList(n);
//  /// }
//
//  /// if (!diskloaded && !Connection->Abort)
//  /// {
//  ///     Connection->Cache->setCacheState(toCache::READING_SYNONYMS);
//  ///     /* NOTE: we cannot pass n as parameter of synonymMap because
//  ///        it's parameter needs to be sorted */
//  ///     std::map<QString, objectName> m =
//  ///         Connection->Connection->synonymMap(Connection->Cache->objects(true));
//  ///     if (!Connection->Abort)
//  ///     {
//  ///         Connection->Cache->setSynonymList(m);
//  ///         Connection->Cache->writeDiskCache();
//  ///     }
//  /// }
//  /// /* Increase the semaphore to "unlock" parent thread destructor */
//      ///Connection->Cache->ReadingThread.up();
//      ///Connection->Cache->setCacheState(toCache::DONE);
//  /// toMainWidget()->checkCaching();
//  ///}
//  ///catch (...)
//  ///{
//  /// Connection->Cache->ReadingThread.up();
//  /// Connection->Cache->setCacheState(toCache::FAILED);
//  /// toMainWidget()->checkCaching();
//  ///}
//
//}

QString toConnection::quote(const QString &name, const bool quoteLowercase)
{
    if (!name.isNull())
        return Connection->quote(name, quoteLowercase);
    return QString::null;
}

QString toConnection::unQuote(const QString &name)
{
    if (!name.isNull())
        return Connection->unQuote(name);
    return QString::null;
}

//tool toSyntaxAnalyzer &toConnection::connectionImpl::analyzer()
//tool {
//tool     return toSyntaxAnalyzer::defaultAnalyzer();
//tool }

toSyntaxAnalyzer &toConnection::analyzer()
{
    return Connection->analyzer();
}

//QList<toConnection::objectName> toConnection::connectionImpl::objectNames(const QString &owner,
//        const QString &type,
//        const QString &name)
//{
//    QList<objectName> ret;
//    return ret;
//}

///TODO not needed if ObjectNamesNew is pure virtual
////*virtual*/ QList<toCacheNew::CacheEntry*> toConnection::connectionImpl::objectNamesNew()
///{
///
///}

//std::map<QString, toConnection::objectName> toConnection::connectionImpl::synonymMap(QList<objectName> &)
//{
//    std::map<QString, objectName> ret;
//    return ret;
//}

//toQDescList toConnection::connectionImpl::columnDesc(const objectName &)
//{
//    toQDescList ret;
//    return ret;
//}

//const toQDescList &toConnection::columns(const objectName &object, bool nocache)
//{
//    toQDescList& cols = Cache->columns(object);
//    if (cols.empty() || nocache)
//    {
//        Cache->addColumns(object, Connection->columnDesc(object));
//        cols = Cache->columns(object);
//    }
//
//    return cols;
//}

void toConnection::rereadCache(void)
{
    ///Cache->rereadCache(new cacheObjects(this));
    CacheNew->rereadCache(new cacheObjectsNew(this));
}

//const QList<toConnection::objectName> &toConnection::objects(bool block) const
//{
//    return Cache->objects(block);
//}
QList<toCacheNew::CacheEntry const*> toConnection::objects(bool block) const
{
    return CacheNew->objects(block);
}

//bool toConnection::rereadObjects(const QString &owner,
//                                 const QString &type,
//                                 const QString &name)
//{
//    bool added = false; // did we actually add at least one new object?
//
//    ///QList<toConnection::objectName> objects = this->Connection->objectNames(owner, type, name);
//    ///for (QList<toConnection::objectName>::iterator i = objects.begin(); i != objects.end(); i++)
//    ///{
//    ///    added = added || Cache->addIfNotExists(*i);
//    ///}
//    // TODO: If all objects of a particular type/owner are fetched we should delete all non
//    // matching records from the cache!
//    return added;
//} // rereadObjects
bool toConnection::rereadObjectNew(const QString &owner, const QString &name)
{
    bool added = false;
    return added;
}

//bool toConnection::cacheAvailable(toCache::cacheEntryType type, bool block)
//{
//    return Cache->cacheAvailable(type, block, new cacheObjects(this));
//}

bool toConnection::cacheRefreshRunning() const
{
    return CacheNew->cacheRefreshRunning();
}

//const toConnection::objectName& toConnection::realName(const QString &object, QString &synonym, bool block) const
//{
//    return Cache->realName(object, synonym, block, user(), database());
//}

//const toConnection::objectName& toConnection::realName(const QString &object, bool block) const
//{
//    QString dummy;
//    return Cache->realName(object, dummy, block, user(), database());
//}

//const std::list<toConnection::objectName> toConnection::tables(const objectName &object, bool nocache) const
//{
//  return Cache->tables(object, nocache);
//}

void toConnection::connectionImpl::parse(toConnectionSub *,
        const QString &)
{
    throw qApp->translate(
        "toConnection",
        "Parse only not implemented for this type of connection");
}
