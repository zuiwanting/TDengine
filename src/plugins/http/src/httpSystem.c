/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "os.h"
#include "taos.h"
#include "tglobal.h"
#include "tsocket.h"
#include "ttimer.h"
#include "tadmin.h"
#include "httpInt.h"
#include "httpContext.h"
#include "httpSession.h"
#include "httpServer.h"
#include "httpResp.h"
#include "httpHandle.h"
#include "gcHandle.h"
#include "restHandle.h"
#include "tgHandle.h"

#ifndef _ADMIN
void adminInitHandle(HttpServer* pServer) {}
void opInitHandle(HttpServer* pServer) {}
#endif

HttpServer tsHttpServer;
void taosInitNote(int numOfNoteLines, int maxNotes, char* lable);

int httpInitSystem() {
  strcpy(tsHttpServer.label, "rest");
  tsHttpServer.serverIp = 0;
  tsHttpServer.serverPort = tsHttpPort;
  tsHttpServer.numOfThreads = tsHttpMaxThreads;
  tsHttpServer.processData = httpProcessData;

  pthread_mutex_init(&tsHttpServer.serverMutex, NULL);

  if (tsHttpEnableRecordSql != 0) {
    taosInitNote(tsNumOfLogLines / 10, 1, (char*)"http_note");
  }
  restInitHandle(&tsHttpServer);
  adminInitHandle(&tsHttpServer);
  gcInitHandle(&tsHttpServer);
  tgInitHandle(&tsHttpServer);
  opInitHandle(&tsHttpServer);

  return 0;
}

int httpStartSystem() {
  httpPrint("start http server ...");

  if (tsHttpServer.status != HTTP_SERVER_INIT) {
    httpError("http server is already started");
    return -1;
  }

  if (!httpInitContexts()) {
    httpError("http init contexts failed");
    return -1;
  }

  if (!httpInitSessions()) {
    httpError("http init session failed");
    return -1;
  }

  if (!httpInitConnect()) {
    httpError("http init server failed");
    return -1;
  }

  return 0;
}

void httpStopSystem() {
  tsHttpServer.status = HTTP_SERVER_CLOSING;
  shutdown(tsHttpServer.fd, SHUT_RD);
  tgCleanupHandle();
}

void httpCleanUpSystem() {
  httpPrint("http server cleanup");
  httpStopSystem();

  httpCleanupContexts();
  httpCleanUpSessions();
  httpCleanUpConnect();
  pthread_mutex_destroy(&tsHttpServer.serverMutex);

  tsHttpServer.status = HTTP_SERVER_CLOSED;
}

int32_t httpGetReqCount() {
  return atomic_exchange_32(&tsHttpServer.requestNum, 0);
}
