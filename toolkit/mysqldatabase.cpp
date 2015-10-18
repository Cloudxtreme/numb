/***************************************************************************
                          mysqldatabase.cpp  -  description
                             -------------------
    begin                : Ven nov 22 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#include "mysqldatabase.h"

MysqlDatabase::MysqlDatabase(String *_databaseHost, String *_databaseUser, String *_databasePassword, String *_database) {
  parametersInitialized = 0;
  mysqlUseResultInitialized = false;
  row = NULL;
  result = NULL;
  if (_databaseHost)
    databaseHost = _databaseHost;
  else
    systemLog->sysLog(ERROR, "databaseHost is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databaseUser)
    databaseUser = _databaseUser;
  else
    systemLog->sysLog(ERROR, "databaseUser is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databasePassword)
    databasePassword = _databasePassword;
  else
    systemLog->sysLog(ERROR, "databasePassword is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_database)
    database = _database;
  else
    systemLog->sysLog(ERROR, "database is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databaseHost && _databaseUser && _databasePassword && _database)
    parametersInitialized = 1;
  mysqlConnect();

  return;
}

MysqlDatabase::MysqlDatabase(char *_databaseHost, char *_databaseUser, char *_databasePassword, char *_database) {
  parametersInitialized = 0;
  mysqlUseResultInitialized = false;
  row = NULL;
  result = NULL;
  if (_databaseHost) {
    databaseHost = new String(strlen(_databaseHost)+1);
    strcpy(databaseHost->bloc, _databaseHost);
  }
  else
    systemLog->sysLog(ERROR, "databaseHost is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databaseUser) {
    databaseUser = new String(strlen(_databaseUser)+1);
    strcpy(databaseUser->bloc, _databaseUser);
  }
  else
    systemLog->sysLog(ERROR, "databaseUser is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databasePassword) {
    databasePassword = new String(strlen(_databasePassword)+1);
    strcpy(databasePassword->bloc, _databasePassword);
  }
  else
    systemLog->sysLog(ERROR, "databasePassword is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_database) {
    database = new String(strlen(_database)+1);
    strcpy(database->bloc, _database);
  }
  else
    systemLog->sysLog(ERROR, "database is NULL. You must specify this parameter correctly for using this class correctly\n");
  if (_databaseHost && _databaseUser && _databasePassword && _database)
    parametersInitialized = 1;
  mysqlConnect();

  return;
}


MysqlDatabase::~MysqlDatabase() {
  if (parametersInitialized) {
    mysqlClose();
    if (result)
      mysqlFreeResult();
  }
  result = NULL;
  if (databaseHost)
    delete databaseHost;
  if (databaseUser)
    delete databaseUser;
  if (databasePassword)
    delete databasePassword;
  if (database)
    delete database;
  row = NULL;
  mysqlUseResultInitialized = false;

  return;
}

int MysqlDatabase::mysqlConnect(void)
{
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot opening mysql connection\n");
    return EINVAL;
  }
	(void)mysql_init(&connection);
	if (! mysql_real_connect(&connection, databaseHost->bloc, databaseUser->bloc, databasePassword->bloc, database->bloc, 0, NULL, 0)) {
    systemLog->sysLog(ERROR, "connection to database failed: %s\n", mysql_error(&connection));
		return EINVAL;
	}

  return 0;
}

int MysqlDatabase::mysqlClose(void) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot opening mysql connection\n");
    return EINVAL;
  }
	mysql_close(&connection);

  // @@@ Dans le cas d'un serveur MySql 4.x (enfin surtout librairie cliente 4.x) il *FAUT* appeler cette fonction.
  // Sinon des fuites mémoires sont à prévoir.
  // mysql_thread_end();

  return 0;
}

int MysqlDatabase::mysqlQuery(String *queryString) {
  int returnCode;

  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot do the query\n");
    return EINVAL;
  }
  if (! queryString) {
    systemLog->sysLog(ERROR, "queryString argument musn't be NULL. cannot do the query\n");
  }
  returnCode = mysql_query(&connection, queryString->bloc);
  if (returnCode) {
    systemLog->sysLog(ERROR, "mysql query error: %s\n", mysql_error(&connection));
    systemLog->sysLog(ERROR, "error query is: %s\n", queryString->bloc);
  }

  return returnCode;
}

int MysqlDatabase::mysqlQuery(char *queryString) {
  int returnCode;

  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot do the query\n");
    return EINVAL;
  }
  if (! queryString) {
    systemLog->sysLog(ERROR, "queryString argument musn't be NULL. cannot do the query\n");
  }
  returnCode = mysql_query(&connection, queryString);
  if (returnCode) {
    systemLog->sysLog(ERROR, "mysql query error: %s\n", mysql_error(&connection));
    systemLog->sysLog(ERROR, "error query is: %s\n", queryString);
  }

  return returnCode;
}

int MysqlDatabase::mysqlFetchRow(void) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot fetch row\n");
    return EINVAL;
  }
  // Return 1 if there is no rows available
  if (! mysql_field_count(&connection))
    return 1;
  if (! mysqlUseResultInitialized) {
    mysqlUseResultInitialized = true;
    result = mysql_use_result(&connection);
    if (! result) {
      systemLog->sysLog(ERROR, "cannot use result on the mysql connection: (%d) %s\n", mysql_errno(&connection), mysql_error(&connection));
      return EINVAL;
    }
  }
  row = mysql_fetch_row(result);
  if (! row) {
    mysqlFreeResult();
    return -1;
  }

  return 0;
}

int MysqlDatabase::mysqlAffectedRows(void) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot fetch row\n");
    return EINVAL;
  }
  return mysql_affected_rows(&connection);
}

int MysqlDatabase::mysqlFreeResult(void) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot free result\n");
    return EINVAL;
  }
  if (! result) {
    systemLog->sysLog(ERROR, "result argument mustn't be NULL. cannot free result\n");
    return EINVAL;
  }
  mysql_free_result(result);
  result = NULL;
  row = NULL;
  mysqlUseResultInitialized = false;

  return 0;
}

unsigned long MysqlDatabase::mysqlRealEscapeString(String *mysqlQueryFrom, String *mysqlQueryTo) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot free result\n");
    return EINVAL;
  }
  return mysql_real_escape_string(&connection, mysqlQueryTo->getBloc(), mysqlQueryFrom->getBloc(), mysqlQueryFrom->getBlocSize());
}

unsigned long MysqlDatabase::mysqlRealEscapeString(char *mysqlQueryFrom, char *mysqlQueryTo) {
  if (! parametersInitialized) {
    systemLog->sysLog(ERROR, "database parameters are not initialized correctly. cannot free result\n");
    return EINVAL;
  }
  return mysql_real_escape_string(&connection, mysqlQueryTo, mysqlQueryFrom, strlen(mysqlQueryFrom));
}

