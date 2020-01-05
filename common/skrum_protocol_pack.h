/* Skrum pack message header */

#ifndef _SKRUM_PROTOCOL_PACK_H
#define _SKRUM_PROTOCOL_PACK_H

#include "skrum_protocol_defs.h"
#include "pack.h"

void pack_ints_msg (ints_msg_t *msg, Buf buffer);
int unpack_ints_msq (ints_msg_t **msg, Buf buffer);

#endif
