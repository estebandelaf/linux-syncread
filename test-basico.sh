#/bin/bash
#
# Test básico de tarea 3 Sistemas Operativos (2013-1)
# by Esteban De La Fuente Rubio, DeLaF (esteban[at]delaf.cl)
#
# Ejecutar pruebas con: $ bash test.sh
# Dejar todo limpio con: $ bash test.sh clean
#

# número mayor que se utilizará
MAJOR=61

# dispositivo del módulo
DEV="/dev/syncread"

# módulo
MODULE="syncread"

# limpiar pantalla
clear

# borrar dispositivos si existen
echo "Borrando $DEV (si existe)..."
sudo rm -f $DEV

# limpiar directorio
echo "Limpiando código compilado del módulo $MODULE (si existe)..."
make clean > /dev/null

# descargar módulo si ya está cargado
echo "Descargando módulo $MODULE (si está cargado)..."
if [ `lsmod | grep $MODULE | wc -l` -eq 1 ]; then
	sudo rmmod $MODULE
fi

# si solo se pidió limpiar se detiene ejecución aquí
if [ "$1" = "clean" ]; then
	exit
fi

# crear dispositivos
echo "Creando $DEV..."
sudo mknod $DEV c 61 0

# ajustar propietario de los dispositivos recién creados (para evitar sudo)
echo "Cambiando propietario de $DEV `whoami`..."
sudo chown `whoami`: $DEV

# compilar módulo
echo "Compilando módulo $MODULE..."
make > /dev/null

# cargar módulo
echo "Cargando módulo $MODULE..."
sudo insmod $MODULE.ko

# ejecutar pruebas
echo ""
echo "Ejecutando pruebas:"
echo ""

echo "Hola Mundo!!" > $DEV
cat $DEV
