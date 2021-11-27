/**
 * @file syncread.h
 * Implementación del módulo syncread (cabecera)
 * @author Esteban De La Fuente Rubio, DeLaF (esteban[at]delaf.cl)
 */

/* Bibliotecas */
#include <linux/module.h>	/* requerido por todos los módulos */
#include <linux/init.h>		/* module_init y module_exit */
#include <linux/kernel.h>	/* printk y KERN_INFO */
#include <linux/fs.h>		/* struct file_operations */
#include <linux/slab.h>		/* kmalloc */
#include <asm/uaccess.h>	/* copy_to_user y copy_from_user */

/* Definiciones */
#define DEV_MAJOR 61		/* número mayor que identifica el módulo */
#define BUFFER_SIZE 8192	/* tamaño máximo del buffer (dispositivo) */

/* Información del módulo */
MODULE_AUTHOR ("Esteban De La Fuente Rubio");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("Tarea 3 - Sistemas Operativos - Otoño 2013 - UChile");
MODULE_SUPPORTED_DEVICE ("/dev/syncread");

/* Prototipos de funciones */
static int module_load (void);
static void module_unload (void);
static int device_open (struct inode *inode, struct file *filp);
static int device_release (struct inode *inode, struct file *filp);
static ssize_t device_read (struct file *filp, char *buf, size_t length,
			loff_t *offset);
static ssize_t device_write (struct file *filp, const char *buf, size_t length,
			loff_t *offset);

/* Estructura con funciones para manejar el dispositivo. Se hace un mapeo entre
las llamadas a sistema y las funciones definidas */
static struct file_operations fops = {
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write
};

/* Mapeo de funciones para carga y descarga */
module_init (module_load);	/* Carga con insmod */
module_exit (module_unload);	/* Descarga con rmmod */

/* Variables globales */
static char *buffer = NULL;	/* este ES el dispositivo virtual */
static ssize_t curr_size;	/* tamaño actualmente utilizado en el buffer */
static struct monitor *m;	/* monitor para la sincronización de procesos */
static short int writing = 0;	/* indica que no (0) o hay (1) un escritor */
static short int readers = 0;	/* indica los lectores que están leyendo */
