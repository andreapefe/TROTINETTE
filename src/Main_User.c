
/*
	!!!! NB : ALIMENTER LA CARTE AVANT DE CONNECTER L'USB !!!

VERSION 16/12/2021 :
- ToolboxNRJ V4
- Driver version 2021b (synchronisation de la mise à jour Rcy -CCR- avec la rampe)
- Validé Décembre 2021

*/


/*
STRUCTURE DES FICHIERS

COUCHE APPLI = Main_User.c : 
programme principal à modifier. Par défaut hacheur sur entrée +/-10V, sortie 1 PWM
Attention, sur la trottinette réelle, l'entrée se fait sur 3V3.
Attention, l'entrée se fait avec la poignée d'accélération qui va de 0.6V à 2.7V !

COUCHE SERVICE = Toolbox_NRJ_V4.c
Middleware qui configure tous les périphériques nécessaires, avec API "friendly"

COUCHE DRIVER =
clock.c : contient la fonction Clock_Configure() qui prépare le STM32. Lancée automatiquement à l'init IO
lib : bibliothèque qui gère les périphériques du STM : Drivers_STM32F103_107_Jan_2015_b
*/



#include "ToolBox_NRJ_v4.h"
#include "math.h"
#include "ADC_DMA.h"



//=================================================================================================================
// 					USER DEFINE
//=================================================================================================================

// Choix de la fréquence PWM (en kHz)
#define FPWM_Khz (20.0)

// Période d'échantillonage
#define PI (3.14159265359)
#define Ft (400.0)
#define phi_rad (0.3491) //20 degrés
#define T ((1.0/((PI/phi_rad)*Ft)))

// Correcteur PI
#define tau_f (7.43e-5)
#define Kgf (48.0*2.2*(1e3/(5.1e3+10e3)))
#define ti (Kgf/(2.0*PI*Ft*sqrt(4.0*(PI*PI)*(Ft*Ft)*(tau_f*tau_f)+1.0)))
#define K (2e-3/ti)

// Coefficients correcteur
#define a0 (-1.0)
#define b0 (-K + (T/(2.0*ti)))
#define b1 (K + (T/(2.0*ti)))
						


//==========END USER DEFINE========================================================================================

// ========= Variable globales indispensables et déclarations fct d'IT ============================================

void IT_Principale(void);
//=================================================================================================================


/*=================================================================================================================
 					FONCTION MAIN : 
					NB : On veillera à allumer les diodes au niveau des E/S utilisée par le progamme. 
					
					EXEMPLE: Ce progamme permet de générer une PWM (Voie 1) à 20kHz dont le rapport cyclique se règle
					par le potentiomètre de "l'entrée Analogique +/-10V"
					Placer le cavalier sur la position "Pot."
					La mise à jour du rapport cyclique se fait à la fréquence 1kHz.

//=================================================================================================================*/


int Courant_1,Cons_In;
float erreur_prec, sortie_prec, sortie, erreur;
float ratio;
float tt;


float Te,Te_us;

int main (void)
{
	
	
// !OBLIGATOIRE! //	
Conf_Generale_IO_Carte();	
	

	
// ------------- Discret, choix de Te -------------------	
Te=	T; // en seconde
Te_us=Te*1000000.0; // conversion en µs pour utilisation dans la fonction d'init d'interruption
	

//______________ Ecrire ici toutes les CONFIGURATIONS des périphériques ________________________________	
// Paramétrage ADC pour entrée analogique
Conf_ADC();
// Configuration de la PWM avec une porteuse Triangle, voie 1 & 2 activée, inversion voie 2
Triangle (FPWM_Khz);
Active_Voie_PWM(1);	
Active_Voie_PWM(2);	
Inv_Voie(2);

Start_PWM;
R_Cyc_1(2048);  // positionnement à 50% par défaut de la PWM
R_Cyc_2(2048);

// Activation LED
LED_Courant_On;
LED_PWM_On;
LED_PWM_Aux_Off;
LED_Entree_10V_Off;
LED_Entree_3V3_On;
LED_Codeur_Off;

	sortie_prec = 0.0;
	erreur_prec = 0.0;
	tt = b0;

// Conf IT
Conf_IT_Principale_Systick(IT_Principale, Te_us);

	while(1)
	{
	}

}





//=================================================================================================================
// 					FONCTION D'INTERRUPTION PRINCIPALE SYSTICK
//=================================================================================================================


void IT_Principale(void)
{
	//Recupération des valeurs
	Cons_In = Entree_3V3();
	Courant_1 = I1();
	
	erreur = (Cons_In - Courant_1)*3.3/4096.0;
	//erreur = 0.1;
	sortie = b1*erreur + b0*erreur_prec - a0*sortie_prec;
	
	//Saturation
	if (sortie > 1.0)
	{
		sortie = 1.0;
	} else if (sortie < 0.0)
	{
		sortie = 0.0;
	}
	
	sortie_prec = sortie;
	erreur_prec = erreur;

	
	//Calcul du ratio en int
	ratio = sortie*4096;
	R_Cyc_1((int) ratio);
	R_Cyc_2((int) ratio);
	
}

