/***************************************************************************
                          mysqldatabase.h  -  description
                             -------------------
    begin                : Ven nov 22 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H

#include <mysql/mysql.h>
#include "log.h"
#include "mystring.h"

// Memory debugging
#include "debugtk.h"

/**
  *@author spe
  */

class MysqlDatabase {
private:
  MYSQL connection;
  MYSQL_RES *result;
  String *databaseHost;
  String *databaseUser;
  String *databasePassword;
  String *database;
  int parametersInitialized;
  bool mysqlUseResultInitialized;
public:
  MYSQL_ROW row;
  MysqlDatabase(String *, String *, String *, String *);
  MysqlDatabase(char *, char *, char *, char *);
  ~MysqlDatabase();
  int mysqlConnect(void);
  int mysqlClose(void);
  int mysqlQuery(String *);
  int mysqlQuery(char *);
  int mysqlAffectedRows(void);
  int mysqlFetchRow(void);
  int mysqlFreeResult(void);
  unsigned long mysqlRealEscapeString(String *mysqlQueryFrom, String *mysqlQueryTo);
  unsigned long mysqlRealEscapeString(char *mysqlQueryFrom, char *mysqlQueryTo);
};

#endif
