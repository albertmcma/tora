
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

#include "connection/toqodbcprovider.h"
#include "connection/toqsqlconnection.h"

#define QT_DRIVER_NAME "QODBC"

QString toQODBCProvider::m_name = QT_DRIVER_NAME;
QString toQODBCProvider::m_displayName = ODBC_PROVIDER;

toQODBCProvider::toQODBCProvider(toConnectionProviderFinder::ConnectionProvirerParams const& p)
    : toQSqlProvider(p)
{}

toConnection::connectionImpl* toQODBCProvider::createConnectionImpl(toConnection &conn)
{
    return new toQSqlConnectionImpl(conn);
}

/*
bool toQODBCProvider::initialize()
{
	return true;
}

QList<QString> toQODBCProvider::hosts()
{
    QList<QString> ret = QList<QString>() << QString::null << "(TBD) hosts";
    return ret;
}

QList<QString> toQODBCProvider::databases(const QString &host, const QString &user, const QString &pwd)
{
	QList<QString> ret;
	ret << "TBD databases";
	return ret;
}

QList<QString> toQODBCProvider::options()
{
	QList<QString> ret;
	ret << "TBD options";
	return ret;
}

QWidget* toQODBCProvider::configurationTab(QWidget *parent)
{
	return NULL;
}

toConnectionTraits* toQODBCProvider::createConnectionTrait(void)
{
	return NULL;
}
*/
