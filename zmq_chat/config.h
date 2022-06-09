#ifndef CONFIG_H
#define CONFIG_H


/* --------------------------- connection --------------------------- */
char *servport = ":5555";
char *publport = ":5556";
char *transport = "tcp://"; 

char *msg = "msg";    /* for zmq_setsockopt option */

/* --------------------------- thread --------------------------- */
enum threadstate {term, work};

/* --------------------------- message --------------------------- */
#define msg_buffer_size 256

#endif /* CONFIG_H */