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

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qworkspace.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qtimer.h>
#include <qstring.h>

#include <map>

#include "totool.h"
#include "tomarkedtext.h"
#include "tomain.h"
#include "tosql.h"
#include "toconnection.h"

#include "tooutput.moc"

#include "icons/refresh.xpm"
#include "icons/tooutput.xpm"
#include "icons/eraselog.xpm"
#include "icons/online.xpm"
#include "icons/offline.xpm"

#define CONF_POLLING    "Refresh"
#define DEFAULT_POLLING	"10 seconds"

class toOutputPrefs : public QFrame, public toSettingTab
{ 
  QComboBox* AutoPolling;
  toTool *Tool;

public:
  toOutputPrefs(toTool *tool,QWidget* parent = 0,const char* name = 0)
    : QFrame(parent,name),toSettingTab("output.html"),Tool(tool)
  {
    QGroupBox *GroupBox1 = new QGroupBox( this, "GroupBox1" );
    GroupBox1->setGeometry( QRect( 10, 10, 380, 380 ) ); 
    GroupBox1->setTitle( tr( "SQL Output"  ) );
    
    QLabel *label=new QLabel("&Polling timeout",GroupBox1);
    label->setGeometry(QRect(20,30,100,20));
    QToolTip::add( label, tr( "Time between trying to poll for output." ) );

    AutoPolling=toRefreshCreate(GroupBox1,NULL,Tool->config(CONF_POLLING,DEFAULT_POLLING));
    AutoPolling->setGeometry( QRect( 130,30,220,20));
    label->setBuddy(AutoPolling);
  }
  virtual void saveSetting(void)
  {
    Tool->setConfig(CONF_POLLING,AutoPolling->currentText());
  }
};

class toOutputTool : public toTool {
protected:
  std::map<toConnection *,QWidget *> Windows;

  virtual char **pictureXPM(void)
  { return tooutput_xpm; }
public:
  toOutputTool()
    : toTool(103,"SQL Output")
  { }
  virtual const char *menuItem()
  { return "SQL Output"; }
  virtual QWidget *toolWindow(QWidget *parent,toConnection &connection)
  {
    std::map<toConnection *,QWidget *>::iterator i=Windows.find(&connection);
    if (i!=Windows.end()) {
      (*i).second->setFocus();
      return NULL;
    } else {
      QWidget *window=new toOutput(parent,connection);
      Windows[&connection]=window;
      window->setIcon(*toolbarImage());
      return window;
    }
  }
  void closeWindow(toConnection &connection)
  {
    std::map<toConnection *,QWidget *>::iterator i=Windows.find(&connection);
    if (i!=Windows.end())
      Windows.erase(i);
  }
  virtual QWidget *configurationTab(QWidget *parent)
  { return new toOutputPrefs(this,parent); }
};

static toOutputTool OutputTool;

static QPixmap *toRefreshPixmap;
static QPixmap *toEraseLogPixmap;
static QPixmap *toOnlinePixmap;
static QPixmap *toOfflinePixmap;

toOutput::toOutput(QWidget *main,toConnection &connection,bool enabled)
  : toToolWidget(OutputTool,"output.html",main,connection)
{
  if (!toRefreshPixmap)
    toRefreshPixmap=new QPixmap((const char **)refresh_xpm);
  if (!toEraseLogPixmap)
    toEraseLogPixmap=new QPixmap((const char **)eraselog_xpm);
  if (!toOnlinePixmap)
    toOnlinePixmap=new QPixmap((const char **)online_xpm);
  if (!toOfflinePixmap)
    toOfflinePixmap=new QPixmap((const char **)offline_xpm);

  QToolBar *toolbar=toAllocBar(this,"SQL Output",connection.description());

  new QToolButton(*toRefreshPixmap,
		  "Poll for output now",
		  "Poll for output now",
		  this,SLOT(refresh(void)),
		  toolbar);
  toolbar->addSeparator();
  DisableButton=new QToolButton(toolbar);
  DisableButton->setToggleButton(true);
  DisableButton->setIconSet(QIconSet(*toOnlinePixmap),false);
  DisableButton->setIconSet(QIconSet(*toOfflinePixmap),true);
  DisableButton->setOn(!enabled);
  connect(DisableButton,SIGNAL(toggled(bool)),this,SLOT(disable(bool)));
  QToolTip::add(DisableButton,"Enable or disable getting SQL output.");

  new QToolButton(*toEraseLogPixmap,
		  "Clear output",	
		  "Clear output",
		  this,SLOT(clear()),
		  toolbar);
  toolbar->addSeparator();
  new QLabel("Refresh",toolbar);
  connect(toRefreshCreate(toolbar,NULL,OutputTool.config(CONF_POLLING,DEFAULT_POLLING)),
	  SIGNAL(activated(const QString &)),this,SLOT(changeRefresh(const QString &)));
  toolbar->setStretchableWidget(new QLabel("",toolbar));

  Output=new toMarkedText(this);

  connect(timer(),SIGNAL(timeout(void)),this,SLOT(refresh(void)));
  toRefreshParse(timer(),OutputTool.config(CONF_POLLING,DEFAULT_POLLING));
  if (enabled)
    disable(false);
}

static toSQL SQLEnable("toOutput:Enable",
		       "BEGIN\n"
		       "    DBMS_OUTPUT.ENABLE;\n"
		       "END;",
		       "Enable output collection");
static toSQL SQLDisable("toOutput:Disable",
			"BEGIN\n"
			"    DBMS_OUTPUT.DISABLE;\n"
			"END;",
			"Disable output collection");

void toOutput::disable(bool dis)
{
  try {
    if (dis)
      connection().allExecute(SQLDisable);
    else
      connection().allExecute(SQLEnable);
    QString str=toSQL::string(SQLEnable,connection());
    if (dis)
      connection().delInit(str);
    else
      connection().addInit(str);
  } catch (...) {
    toStatusMessage("Couldn't enable/disable output for session");
  }
}

toOutput::~toOutput()
{
  disable(true);
  OutputTool.closeWindow(connection());
}

static toSQL SQLLines("toOutput:Poll",
		      "BEGIN\n"
		      "    DBMS_OUTPUT.GET_LINE(:lines<char[1000],out>,\n"
		      "                         :numlines<char[100],out>);\n"
		      "END;",
		      "Get lines from SQL Output, must use same bindings");

void toOutput::poll()
{
  try {
    bool any;
    do {
      toQList params;
      toQuery query(connection(),toQuery::All,SQLLines,params);

      any=false;
      while(!query.eof()) {
	QString line=query.readValueNull();
	int status=QString(query.readValue()).toInt();
	if (status==0) {
	  any=true;
	  insertLine(line);
	}
      }
    } while(!any);
  } TOCATCH
}

void toOutput::refresh(void)
{
  poll();
}

void toOutput::clear(void)
{
  Output->clear();
}

void toOutput::changeRefresh(const QString &str)
{ 
  toRefreshParse(timer(),str);
}

bool toOutput::enabled(void)
{
  return !DisableButton->isOn();
}
