/**
 * Tarea 3 - Sistemas Operativos - Semestre Otoño 2013 - Universidad de Chile
 * by Esteban De La Fuente Rubio, DeLaF (esteban[at]delaf.cl)
 *
 * El módulo se programó en Debian GNU/Linux con el kernel:
 *   Linux trunks 3.2.0-4-amd64 #1 SMP Debian 3.2.46-1 x86_64 GNU/Linux
 *
 * Pasos previos para poder probar el ejemplo (en el directorio donde está el
 * archivo Makefile y el archivo syncread.c):
 *   $ make
 *   $ sudo insmod syncread.ko
 *   $ sudo mknod /dev/syncread c 61
 *   $ sudo chown `whoami`: /dev/syncread
 * Ahora se puede probar el ejemplo con el usuario normal.
 */

/**
 * @file syncread.c
 * Implementación del módulo syncread
 * @author Esteban De La Fuente Rubio, DeLaF (esteban[at]delaf.cl)
 */

#include "syncread.h"

/**
 * Función llamada al cargar el módulo:
 *  - Registra el dispositivo de caracter con register_chrdev
 *  - Verifica que no existió error al registrar
 *  - Genera mensaje de tipo KERN_INFO
 * @return 0 en caso de éxito, -1 en caso de error
 */
static int module_load (void)
{
	/* variable para el estado devuelto por register_chrdev */
	short status = -1;
	/* mensaje del kernel */
	printk (KERN_INFO "Cargando modulo syncread\n");
	/* registrar el dispositivo de caracter
	http://www.fsl.cs.sunysb.edu/kernel-api/re941.html */
	status = register_chrdev (
		DEV_MAJOR,	/* número mayor */
		"syncread",	/* nombre para los dispositivos */
		&fops		/* operaciones asociadas al dispositivo */
	);
	/* verificar que no ha habido error (retorno negativo) al registrar el
	dispositivo */
	if(status < 0) {
		printk (KERN_INFO "A ocurrido un error al ejecutar "
			"register_chrdev\n");
		return status;
	}
	/* solicitar memoria para el buffer en el espacio de memoria del kernel
	http://www.fiveanddime.net/man-pages/kmalloc.9.html */
	printk (KERN_INFO "Solicitando memoria para el buffer");
	buffer = kmalloc (
		BUFFER_SIZE,
		GFP_KERNEL
	);
	/* en caso de que no se haya asignado memoria error */
	if (buffer==NULL) {
		printk (KERN_INFO "Ha ocurrido un error al asignar"
			"memoria con kmalloc\n");
		return -ENOMEM;
	}
	/* inicializar en ceros la memoria: $ man memset */
	memset (buffer, 0, BUFFER_SIZE);
	/* tamaño actual del buffer es 0 */
	curr_size = 0;
	/* crear monitor */
	m = monitor_make();
	/* mensaje indicando que el modulo se cargó */
	printk (KERN_INFO "Modulo syncread cargado correctamente\n");
	printk (KERN_INFO "Numero mayor en uso para el dispositivo: %d\n",
	       DEV_MAJOR);
	/* retornar todo ok */
	return 0;
}

/**
 * Función llamada al descargar el módulo
 *  - Desregistra el dispositivo de caracter con unregister_chrdev
 *  - Libera la memoria solicitada para el buffer con kfree
 *  - Genera mensaje de tipo KERN_INFO
 */
static void module_unload (void)
{
	/* desregistrar */
	unregister_chrdev (DEV_MAJOR, "syncread");
	/* liberar memoria */
	if (buffer) {
		kfree(buffer);
	}
	/* destruir monitor */
	monitor_destroy (m);
	/* mensaje indicando que se descargó el módulo */
	printk (KERN_INFO "Modulo syncread descargado\n");
}

/**
 * Función que abre el dispositivo
 * @param inode Información sobre el archivo
 * @param filp Representación del archivo que se abrirá
 * @return 0 en caso de éxito, -1 en caso de error
 */
static int device_open (struct inode *inode, struct file *filp)
{
	/* determinar modo en que se abrió el archivo */
	char *mode = filp->f_mode & FMODE_WRITE ? "write" :
				filp->f_mode & FMODE_READ ? "read" : "unknown";
	/* retorno posible del wait */
	int rc = 0;
	/* pedir monitor */
	monitor_enter (m);
	/* acciones en caso que sea modo escritura */
	if (filp->f_mode & FMODE_WRITE) {
		while (writing || readers)
			rc = monitor_wait (m);
		if (rc) {
			monitor_exit (m);
			return -1;
		}
		writing = 1;
		curr_size = 0;

	}
	/* acciones en caso que sea en modo lectura */
	else if (filp->f_mode & FMODE_READ) {
		readers++;
	}
	/* liberar monitor */
	monitor_exit (m);
	/* mensaje para el usuario */
	printk (KERN_INFO "Buffer abierto para %s\n", mode);
	/* todo ha ido ok */
	return 0;
}

/**
 * Función que libera el dispositivo
 * No se hace nada, ya que el dispositivo esta en RAM
 * @param inode Información sobre el archivo
 * @param filp Representación del archivo abierto
 * @return Siempre 0, ya que no se hace nada especial
 */
static int device_release (struct inode *inode, struct file *filp)
{
	/* determinar modo en que se abrió el archivo */
	char *mode = filp->f_mode & FMODE_WRITE ? "write" :
				filp->f_mode & FMODE_READ ? "read" : "unknown";
	/* pedir monitor */
	monitor_enter (m);
	/* acciones en caso que sea modo escritura */
	if (filp->f_mode & FMODE_WRITE) {
		writing = 0;
	}
	/* acciones en caso que sea en modo lectura */
	else if (filp->f_mode & FMODE_READ) {
		readers--;
	}
	/* liberar monitor */
	monitor_notify_all (m);
	monitor_exit (m);
	/* mensaje para el usuario */
	printk (KERN_INFO "Buffer cerrado para %s\n", mode);
	/* todo ha ido ok */
	return 0;
}

/**
 * Función que lee desde el dispositivo:
 *  - Verifica que el puntero este en una posición válida
 *  - Copia desde el dispositivo virtual al buffer entregado por el usuario
 * Sobre las estructuras de datos: http://www.tldp.org/LDP/tlk/ds/ds.html
 * @param filp Representación del archivo abierto
 * @param buff Apunta al espacio de direcciones del proceso (no es de fiar)
 * @param length Cantidad de bytes que se leeran
 * @param offset Desde que byte se leera
 * @return Cantidad de bytes leídos desde el dispositivo
 */
static ssize_t device_read (struct file *filp, char *buff, size_t length,
			loff_t *offset)
{
	ssize_t status;
	monitor_enter (m);
	while ((int)curr_size-1<(int)*offset && writing)
		monitor_wait(m);
	if (length > curr_size-*offset)
		length = curr_size-*offset;
	if (length < 0)
		length = 0;
	printk(KERN_INFO "read %d bytes at %d\n", (int)length, (int)*offset);
	status = copy_to_user (buff, buffer+*offset, length);
	*offset += length;
	monitor_exit (m);
	return length;
}

/**
 * Función que escribe en el dispositivo
 * Copia desde el buffer entregado por el usuario al dispositivo (sobreescribe)
 * Sobre las estructuras de datos: http://www.tldp.org/LDP/tlk/ds/ds.html
 * @param filp Representación del archivo abierto
 * @param buff Apunta al espacio de direcciones del proceso
 * @param length Cantidad de bytes que se escribiran
 * @param offset Desde que byte se leera
 * @return Cantidad de bytes escritos en el dispositivo
 */
static ssize_t device_write (struct file *filp, const char *buff, size_t length,
			loff_t *offset)
{
	ssize_t status;
	loff_t last;
	monitor_enter (m);
	last = *offset + length;
	if (last>BUFFER_SIZE) {
		length -= last-BUFFER_SIZE;
	}
	printk("<1>write %d bytes at %d\n", (int)length, (int)*offset);
	status = copy_from_user (buffer+*offset, buff, length);
	*offset += length;
	curr_size = *offset;
	monitor_notify_all (m);
	monitor_exit (m);
	return length;
}
