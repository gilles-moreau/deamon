#include "src/common/skrum_protocol_defs.h"

extern int skrum_init_discovery(void);
extern int skrum_send_discovery_msg(skrum_msg_t *msg);
extern int skrum_receive_discovery_msg(int fd, skrum_msg_t *msg, struct sockaddr_in *cont_addr);

