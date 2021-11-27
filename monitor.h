/**
 * @file monitor.h
 * Implementación de monitores para el núcleo de Linux (cabecera)
 */

#include <asm/semaphore.h>

/* estructuras de datos */
struct wait_link {
	struct semaphore wait_process;
	struct wait_link *next;
};
struct monitor {
	struct semaphore mutex;
	struct wait_link *wait_list;
};

/* prototipos */
struct monitor *monitor_make (void);
void monitor_destroy (struct monitor *m);
void monitor_enter (struct monitor *m);
void monitor_exit (struct monitor *m);
int monitor_wait (struct monitor *m);
void monitor_notify_all (struct monitor *m);
