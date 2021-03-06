/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


//////////////////////////////////////////////////////////////////////////////////////////////
//
// Main entry points for the plugin hooks etc.
//
#include <ts/ts.h>
#include <ts/remap.h>
#include <stdio.h>
#include <string.h>

#include "lulu.h"
#include "acl.h"


///////////////////////////////////////////////////////////////////////////////
// Initialize the plugin as a remap plugin.
//
TSReturnCode
TSRemapInit(TSRemapInterface *api_info, char *errbuf, int errbuf_size)
{
  if (api_info->size < sizeof(TSRemapInterface)) {
    strncpy(errbuf, "[tsremap_init] - Incorrect size of TSRemapInterface structure", errbuf_size - 1);
    return TS_ERROR;
  }

  if (api_info->tsremap_version < TSREMAP_VERSION) {
    snprintf(errbuf, errbuf_size - 1, "[tsremap_init] - Incorrect API version %ld.%ld", api_info->tsremap_version >> 16,
             (api_info->tsremap_version & 0xffff));
    return TS_ERROR;
  }

  // gGI = GeoIP_new(GEOIP_STANDARD); // This seems to break on threaded apps
  gGI = GeoIP_new(GEOIP_MMAP_CACHE);

  TSDebug(PLUGIN_NAME, "remap plugin is successfully initialized");
  return TS_SUCCESS; /* success */
}


TSReturnCode
TSRemapNewInstance(int argc, char *argv[], void **ih, char * /* errbuf */, int /* errbuf_size */)
{
  if (argc < 3) {
    TSError("[geoip_acl] Unable to create remap instance, need more parameters");
    return TS_ERROR;
  } else {
    Acl *a = NULL;

    if (!strncmp(argv[2], "country", 11)) {
      TSDebug(PLUGIN_NAME, "creating an ACL rule with ISO country codes");
      a = new CountryAcl();
    }

    if (a) {
      a->process_args(argc, argv);
      *ih = static_cast<void *>(a);
    } else {
      TSError("[geoip_acl] Unable to create remap instance, no supported ACL specified as first parameter");
      return TS_ERROR;
    }
  }

  return TS_SUCCESS;
}

void
TSRemapDeleteInstance(void *ih)
{
  Acl *a = static_cast<Acl *>(ih);

  delete a;
}


///////////////////////////////////////////////////////////////////////////////
// Main entry point when used as a remap plugin.
//
TSRemapStatus
TSRemapDoRemap(void *ih, TSHttpTxn rh, TSRemapRequestInfo *rri)
{
  if (NULL == ih) {
    TSDebug(PLUGIN_NAME, "No ACLs configured, this is probably a plugin bug");
  } else {
    Acl *a = static_cast<Acl *>(ih);

    if (!a->eval(rri, rh)) {
      TSHttpTxnSetHttpRetStatus((TSHttpTxn)rh, (TSHttpStatus)403);
      a->send_html((TSHttpTxn)rh);
    }
  }

  return TSREMAP_NO_REMAP;
}


/*
  local variables:
  mode: C++
  indent-tabs-mode: nil
  c-basic-offset: 2
  c-comment-only-line-offset: 0
  c-file-offsets: ((statement-block-intro . +)
  (label . 0)
  (statement-cont . +)
  (innamespace . 0))
  end:

  Indent with: /usr/bin/indent -ncs -nut -npcs -l 120 logstats.cc
*/
