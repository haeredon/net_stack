#ifndef HANDLER_HANDLERS_HTTP_H
#define HANDLER_HANDLERS_HTTP_H

#include "handlers/handler.h"


struct http_priv_t {
    int dummy;
};

struct http_write_args_t {
  
};

struct handler_t* http_create_handler(struct handler_config_t *handler_config);


#endif // HANDLER_HANDLERS_HTTP_H