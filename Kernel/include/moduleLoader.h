/*
 * moduleLoader.h — Copia los módulos empaquetados por mp.bin a sus direcciones
 * de destino fijas (sampleCodeModuleAddress, sampleDataModuleAddress).
 */
#ifndef MODULELOADER_H
#define MODULELOADER_H

void loadModules(void * payloadStart, void ** moduleTargetAddress);

#endif