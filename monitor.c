#include "monitor.h"

/**
 * @file monitor.c
 * Implementación de monitores para el núcleo de Linux
 * @author Luis Mateu
 */

/**
 * Crear y entregar un nuevo monitor
 * @return Monitor recién creado
 */
struct monitor *monitor_make (void) {
	struct monitor *m = (struct monitor *) kmalloc(
		sizeof(struct monitor),
		GFP_KERNEL
	);
	if (m==NULL) {
		printk (KERN_INFO "[monitor_make] kmalloc error\n");
		return NULL;
	}
	sema_init(&m->mutex, 1);
	m->wait_list = NULL;
	return m;
}

/**
 * Destruir el monitor m
 * @param m Monitor que se desea destruir
 */
void monitor_destroy (struct monitor *m) {
	if(m)
		kfree(m);
}

/**
 * Solicitar la propiedad del monitor
 * @param m Monitor que se desea solicitar
 */
void monitor_enter (struct monitor *m) {
	down(&m->mutex);
}

/**
 * Devolver la propiedad del monitor
 * @param m Monitor que se desea liberar
 */
void monitor_exit (struct monitor *m) {
	up(&m->mutex);
}

/**
 * Devolver el monitor y bloquear a la espera de monitor_notify_all
 * @param m Monitor por el cual se desea esperar
 * @return 0 en caso de éxito, distinto de 0 en caso de algún problema
 */
int monitor_wait (struct monitor *m) {
	int rc = 0;
	struct wait_link link;
	sema_init(&link.wait_process, 0);
	link.next = m->wait_list;
	m->wait_list = &link;
	up(&m->mutex);
	rc = down_interruptible (&link.wait_process);
	if (rc) {
		/* Si down_interruptible retorno por un control-C, y no por
		monitor_notify_all, hay que borrar este link de la lista
		m->wait_list */
		struct wait_link **ppnode = &m->wait_list;
		while (*ppnode!=NULL && *ppnode!=&link)
			ppnode = &((*ppnode)->next);
		if (*ppnode==&link)
			*ppnode = link.next;
	}
	down(&m->mutex);
	return rc;
}

/**
 * Despertar a todos los procesos que esperan en monitor_wait
 * @param m Monitor al cual se despertarán todos los procesos que esperan por el
 */
void monitor_notify_all (struct monitor *m) {
	while (m->wait_list!=NULL) {
		up(&m->wait_list->wait_process);
		m->wait_list = m->wait_list->next;
	}
}
