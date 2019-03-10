#ifndef NULLSOFT_XSPF_API_H
#define NULLSOFT_XSPF_API_H

// Service Manager
#include <api/service/api_service.h>
extern api_service *serviceManager;
#define WASABI_API_SVC serviceManager

// Media Library API
#include "../ml_local/api_mldb.h"
extern api_mldb *mldbApi;
#define AGAVE_API_MLDB mldbApi

#endif