//***************************************************************************
/*
 * TOra - An Oracle Toolkit for DBA's and developers
 * Copyright (C) 2000 GlobeCom AB
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *      As a special exception, you have permission to link this program
 *      with the Oracle Client libraries and distribute executables, as long
 *      as you follow the requirements of the GNU GPL in regard to all of the
 *      software in the executable aside from Oracle client libraries. You
 *      are also allowed to link this program with the Qt Non Commercial for
 *      Windows.
 *
 *      Specifically you are not permitted to link this program with the
 *      Qt/UNIX or Qt/Windows products of TrollTech. And you are not
 *      permitted to distribute binaries compiled against these libraries
 *      without written consent from GlobeCom AB. Observe that this does not
 *      disallow linking to the Qt Free Edition.
 *
 * All trademarks belong to their respective owners.
 *
 ****************************************************************************/

#ifndef __TORESULTLINE_H
#define __TORESULTLINE_H

#include <time.h>

#include "toresult.h"
#include "tolinechart.h"

class toSQL;

/** Display the result of a query in a piechart. The first column of the query should
 * contain the x value and the rest of the columns should be values of the diagram. The
 * legend is the column name. Connects to the tool timer for updates automatically.
 */

class toResultLine : public toLineChart, public toResult {
  Q_OBJECT
  /** Display flow in change per second instead of actual values.
   */
  bool Flow;
  /** Timestamp of last fetch.
   */
  time_t LastStamp;
  /** Last read values.
   */
  std::list<double> LastValues;
  /** Query to run.
   */
  QString SQL;
  /** Parameters to query
   */
  toQList Param;
  void query(const QString &sql,const toQList &param,bool first);
public:
  /** Create widget.
   * @param parent Parent of list.
   * @param name Name of widget.
   */
  toResultLine(QWidget *parent,const char *name=NULL);

  /** Display actual values or flow/s.
   * @param on Display flow or absolute values.
   */
  void setFlow(bool on)
  { Flow=on; }
  /** Return if flow is displayed.
   * @return If flow is used.
   */
  bool flow(void)
  { return Flow; }

  /** Set SQL to query.
   * @param sql Query to run.
   */
  void setSQL(const QString &sql)
  { SQL=sql; }
  /** Set the SQL statement of this list.
   * @param sql SQL containing statement.
   */
  void setSQL(toSQL &sql);

  /** Reimplemented for internal reasons.
   */
  virtual void query(const QString &sql,const toQList &param)
  { query(sql,param,true); }
  /** Perform the specified query.
   * @param sql SQL containing statement.
   */
  void query(toSQL &sql);
  /** Reimplemented for internal reasons.
   */
  void query(const QString &sql)
  { toQList p; query(sql,p); }
  /** Reimplemented for internal reasons.
   */
  virtual void clear(void)
  { LastStamp=0; LastValues.clear(); toLineChart::clear(); }
  /** Transform valueset. Make it possible to perform more complex transformation.
   * called directly before adding the valueset to the chart. After flow transformation.
   * Default is passthrough.
   * @param input The untransformed valueset.
   * @return The valueset actually added to the chart.
   */
  virtual std::list<double> transform(std::list<double> &input);
public slots:
  /** Read another value to the chart.
   */
  virtual void refresh(void)
  { query(SQL,Param,false); }
  /** Reimplemented for internal reasons.
   */
  virtual void changeParams(const QString &Param1)
  { toQList p; p.insert(p.end(),Param1); query(SQL,p); }
  /** Reimplemented for internal reasons.
   */
  virtual void changeParams(const QString &Param1,const QString &Param2)
  { toQList p; p.insert(p.end(),Param1); p.insert(p.end(),Param2); query(SQL,p); }
  /** Reimplemented for internal reasons.
   */
  virtual void changeParams(const QString &Param1,const QString &Param2,const QString &Param3)
  { toQList p; p.insert(p.end(),Param1); p.insert(p.end(),Param2); p.insert(p.end(),Param3); query(SQL,p); }
};

#endif
