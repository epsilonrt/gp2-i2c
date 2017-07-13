/**
 * Copyright © 2016 epsilonRT, All rights reserved.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * <http://www.cecill.info>. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 *
 * @file
 * @brief
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif

/* default values =========================================================== */
// Adresse de notre circuit sur le bus I2C (alignée à droite pour linux)
#define OWN_ADDRESS (0x46)
// Nombre d'échantillons pour le moyennage
#define AVERAGE_TERMS 128

/* build options ============================================================ */
// Validation watch-dog pour cadencer les mesures toutes les 8 s
#define WD_ENABLE 1

/* ========================================================================== */
#ifdef __cplusplus
}
#endif
#endif /* _CONFIG_H_ defined */
