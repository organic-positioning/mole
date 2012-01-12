/****************************************************************************
**
** Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
****************************************************************************/

#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QVariant>

class LocalServer : public QTcpServer
{
  Q_OBJECT

 public:
  LocalServer(QObject *parent = 0, int port = 0);
  ~LocalServer ();

 private:
  QString scans;

 private slots:
  void handleRequest();

};

#endif // LOCAL_SERVER_H
