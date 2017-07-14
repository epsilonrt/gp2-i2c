# gp2-i2c

*Module I²C pour capteur de particules fines GP2Y1010AU0F*

---
Copyright © 2017 epsilonRT, All rights reserved.

<a href="http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html">
  <img src="https://raw.githubusercontent.com/epsilonrt/gxPL/master/doc/images/osi.png" alt="osi.png" align="right" valign="top">
</a>

Un ATtiny85 est utilisé pour générer le signal de commande de la led du capteur, 
et mesurer le signal en sortie du capteur (280µs après l'allumage de la led).
La valeur fournie sur le bus I²C correspond au moyennage sur 64 mesures 
(paramétrable dans config.h). Un cycle de mesure est effectué toutes les 8 
secondes.

Le schéma du module est le suivant:

![schéma](https://github.com/epsilonrt/gp2-i2c/raw/master/hardware/gp2-i2c-sch.jpg)

## Lecture par l'interface I²C
Le __gp2-i2c__ se comporte comme un esclave I²C, son adresse<sup>1</sup> est :
<table>
<tr><th>1</th><th>0</th><th>0</th><th>0</th><th>1</th><th>1</th><th>0</th><th>RW</th></tr>
</table>

1. L'adresse de base peut être modifiée en modifiant la constante
 OWN_ADDRESS dans le fichier config.h et en recompilant le firmware.

La valeur lue sur le bus I²C est en millivolts, sur 2 octets, octet de poids 
faible en premier (LSB First).

La lecture peut se faire par un accès en lecture ou en écriture-lecture:  
* Dans le cas d'un accès lecture, le maître I²C transmet l'adresse de l'esclave
avec le bit R/W à 1, puis lit les octets qui sont acquités par l'esclave.
Le pointeur d'octet interne est incrémenté automatiquement. La première lecture 
renvoie le LSB, puis le MSB, puis le LSB, ainsi de suite...  
* Dans le cas d'un accès écriture-lecture, le maître I²C transmet l'adresse de 
l'esclave avec le bit R/W à 0, puis l'adresse de l'octet à lire (0 pour LSB, 1 
pour MSB), puis restart suivi de l'adresse de l'esclave avec le bit R/W à 1, 
puis lit les octets qui sont acquités par l'esclave.

## Programmation de l'attiny

L'utilisation d'un attiny comme gp2-i2c nécessite une modification des 
fusibles et une programmation de la mémoire FLASH avec le firmware gp2-i2c. 

La compilation du firmware nécessite l'installation de 
[AvrIO](http://www.epsilonrt.fr/avrio/doc01.html).

### Configuration des fusibles du attiny

Le microcontrôleur [attiny](http://www.atmel.com/devices/attiny.aspx) 
est paramétré de la façon suivante :

**Low Fuses**  
* CKSEL 3:0 = 0001 -> PLL 16MHz  
* SUT   1:0 = 00   -> 14CK + 1K (1024) CK + 4 ms  
* CKOUT     = 1    -> CKOUT non validée  
* CKDIV8    = 1    -> Pas de division  
Soit une valeur de 0xC1

**High Fuses**  
* BODLVL 2:0 = 101 -> 2.7V  
* EESAVE     = 1  
* WDTON      = 1  
* SPIEN      = 0  
* DWEN       = 1  
* RSTDISBL   = 1  
Soit une valeur de 0xDD.

Si AvrIO est installé correctement, la programmation des fusibles peut se faire 
à l'aide de la commande `make fuse` lancé depuis le shell ou Codelite.

### Compilation et Programmation du firmware

Il est possible de modifier la configuration du firmware grâce au fichier 
[config.h](https://github.com/epsilonrt/gp2-i2c/blob/master/config.h).

Grâce à AvrIO, il est possible de compiler et programmer à partir du shell:

    make
    make program

ou à partir de Codelite ou ATMEL Studio.
