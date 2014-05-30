/* ----------------------------------------------------------------------
 Program flyback;

 Quick + Dirty-Progrann zur Berechnung von Netzgetrennten Sperrwandler-
 netzgeraeten.
 Konzept-,
 Es wird der beschriebene Entwicklungsgang nach Wuestenhube auf den Rechner
 uebertragen. Dabei werden fuer alle Formel alle Variablen gespeichert
 Jede Variable ist ein strukt aus Variable.value extended real)
 und Variable.level (16bit-integer) 
 value haelt das Ergebnis, level haelt die Ebene der Reoptimierung.
 level=0: Variable nicht definiert (gesucht)
 Sobald durch eine Formel oder Eingabe der Variablen ein neuer Wert
 zugewiesen wurde, wird level incrementiert. So koennen Rechenfehler erkant
 (Die Ausgabe eines Rehengangs muss den selben Level haben!) werden, und
 eine zukuenftige Version kann je nach Level der Variablen aus den Eingaben
 selbstaendig alle uebrigen Groessen errechnen, soweit bekannt.
 Gleichzeitigkeit einer Tabellenkalkulation...
 Das programm basiert auf Dialog ueber StdIO.h also ist pipen mit
 Dateien moeglich.
 Das programm startet mit Level 0 und fragt nach Daten, deren Variablen
 nach gueltigen Eingaben den Level incremrntieren. Nach Einabe aller benoetigten
 Variablen (starrer Rechengang in der 1. Version) werden die Ausgabewerte 
 berechnet. Es wird gepromtet neue Rechnung (j/n) bei j wird der
 aktuelle Level des Programms incrementiert, und nach Allen Variablen gepromptet.
 Der alte Wert ist default, er kann durch Enter uebernommen werden. In jedem 
 fall wird der level der jeweiligen Variable inc.
 und der Rechengang starr fortgesetzt.
 wuenschenswert:
 1. Variablen als Array: strukt level:intege; value:array [1..32678] of extended real
    so koennen fuer jeden Optimierungsschritt die Ergebnisse ausgegeben und nach 
    Kriterien sortiert werden. Automatische E-Reihenanpassung und Reoptimierung
    ist so bei jeweiliger Neuberechnung und Suche des Minimums moeglich.
    Dem Anwender wird dann die optimale Konfiguration bei Reoptimierung nach
    Vorgaben autmatisch serviert
  2. Algorithmus zur ermittlung der zu verwendenden Formel, um die fehlenden
    werte zu berechnen. So kann z.B. zu einem gegebenen Trafo ein Schaltnetzgeraet
    gerechnet werden, oder fuer ein vorhandenes eine Spezifikation erstellt werden...
    je nach dem Was gegeben ist. Gleichzeitig Vorschlaege an den Anwender,
    welche Variablen noch fehlern....

-----------------------------------------------------------------------
 zunaechst wird linear programmiert mit array und level aber arry wird
 zwar gefaellt, aber nicht gelesen, level wird zu Fehlersuche (Doppelte
 (Zuweisung) verwendet. Sprache ANSI-C
------------------------------------------------------------------------- 

compiler:

gcc -lm -o flyback flyback.c

**********************************************************
License: GPL2
Author: Ludwig Jaffe
Date: june 1999
Rediscovery & minor edits: Oct 2005
*********************************************************
History:
While cleaning up my old discs and tapes I found
Stuff from my first diploma thesis. 
Here I wrote a small quick an dirty tool to calculate 
the transformer of a flyback converter.
(My power supply worked, but there IS NO GUARANTEE
OF ANY KIND FOR CORRECTNESS AND USABILITY)
Having found this little piece of code I think that it
might be useful for others. So I released it into the internet.

The program can be transfered to english quite easily.
An example is given in the arrays for dialogues.
The hint-texts should also be translated.
Beeing too lazy for that the programme is profided as is.

*/




#include <stdio.h>
/*#include <conio.h> /*only for getch, take getchar and omit conio.h*/
/*this was needed because this program was originally written and compiled*/
/*for dos and some sort of free (as in free beer) c compiler*/
/*ncurses would be a good choice. for replacing conio.*/

#include <math.h>
#include <ctype.h> /*tolower etc.*/


/*fix Numbers*/
#define BOOL	unsigned char
#define FALSE	(BOOL)0
#define TRUE	(BOOL)1
#define MAXOPT 20 /* Maximale Zahl der Optimierungslevel*/
#define MAXVAL 50 /* Maximale Zahl der Variablen*/

/*fixe konstanten*/
#define dreieck 0
#define trapez  1

/* Variablennamen im Feld*/
#define lastitem 28 /*Number of last data item*/
#define Pomax	0 /*maximale Ausgangsleistung*/
#define Pomin	1 /*minimale Ausgangsleistung*/
#define Pimax	2 /*maximale Ausgangsleistung*/
#define Pimin	3 /*minimale Ausgangsleistung*/
#define eff	4 /*Wirkungsgrad*/
#define p	5 /*Pomax/Pomin ->Entscheidung Dreieck/Trapez-Strom*/
#define verlauf 6 /*0=dreieck-strom, 1=trapezstrom*/
#define Uimax   7 /*max. Eingangsspannung*/
#define Uimin   8 /*min. Eingangsspannung*/
#define Uo      9 /*Ausgansspannung*/
#define Uf     10 /*Durchlasspannung Diode*/
#define Uls    11 /*Spannungsabfall Sekundaerwicklung unter Vollast*/
#define Usat   12 /*Spannungsabfall eingeschalteter Transistor*/
#define Ulp    13 /*Spannungsabfall Primaerwicklung unter Vollast*/
#define VTmax  14 /*maximales Tastverhaeltnis des Ansteuer IC*/
#define TR     15 /*Uebersetzungsverhaeltnis des Trafos [Transfer Ratio]*/
#define Uotr   16 /*Sekundaerspannung die uebersetzt an der Primaerseite erscheint*/
#define Udsmin 17 /*minimale Drain-Source-Spannung die der ausgeschaltete
		    Schalttransistor vertragen muss - ohne Sicherheit -*/
#define Uds    18 /*Drain-Source-Spannung, die ausgeschaltete
		    Schalttransistor vertraegt>=Udsmin*designfactor (Datenblatt) -*/
#define Idsmin 19 /*minimaler Drain-Source-Strom die der 
		    Schalttransistor vertragen muss - ohne Sicherheit -*/
#define Ids    20 /*Drain-Source-Strom, die ausgeschaltete
		    Schalttransistor vertraegt>=Idsmin*designfactor (Datenblatt) -*/

#define fw     21 /*Betriebsfrequenz: Geschmackssache, darf nicht in Bereich der
		    Schaltzeiten der Dioden kommen -*/
#define Lp     22 /*Primaerinduktivitaet des Trafos (wird errechnet)*/
#define Bmax   23 /*max. zulässige Induktion Bmax des Kernmaterials in Tesla (T) (wird eingegeben)*/
#define Amin   24 /*minimaler magnetischer Kernquerschnitt Amin in mm^2 (wird eingegeben)*/
#define N1     25 /*Minimale Primärwindungszahl (wird errechnet)*/
#define N2     26 /*zugehörige Sekundärwindungszahl (wird errechnet)*/
#define s      27 /*Luftspalt (wird errechnet)*/
#define AL     28 /*AL-Wert aus Datenbuch (wird eingegeben)*/




		    
/*magic Numbers*/
const float designfactor=1.25; /*Sicherheitsfaktor der ueberdimensionierung*/
const float initeff=80.0; /* anfaenglicher Wirkungsgrad 80% als Designhilfe */
const float mue0=12.57E-9;  /*magnetische Formkonstante 12,59E-9 H/cm */


//by commenting out you can use english language for user dialogs

//Texts fuer Nutzerdialog //
/*
const char *dialog [] =
{
   "maximale Ausgangsleistung",
   "minimale Ausgangsleistung",
   "maximale Eingangsleistung",
   "minimale Eingangsleistung", 
   "anfaenglicher Wirkungsgrad (Annahme) ca. 70..90%",
   "Kriterium p für Stromverlauf", 
   "Stromverlauf",// kein Dialog fuer verlauf,da besondere Eingabe//
   "maximale Eingangsspannung",
   "minimale Eingangsspannung",
   "Ausgangsspannung",
   "Durchlasspannung Gleichrichterdiode",                                                                 
   "Spannungsabfall an der Sekundaerwicklung unter Vollast",
   "Spannungsabfall am eingeschalteten Transistor",
   "Spannungsabfall an der Primaerwicklung unter Vollast",
   "maximales Tastverhaeltnis des Ansteuer-ICs ( < 50% )",
   "Uebersetzungsverhaeltnis des Wandler-Trafos",
   "", //Uotr berechnet, kein Dialog// 
   "Minimale Drain-Source- bzw. Collector-Emiter- Spannung ", 
   "Maximale Spannung Uds bzw. Uce und Ucb des verwendeten Transistors",
   "",
   "Maximaler Strom Ids bzw. Ice des verwendeten Transistors",
   "Betriebsfrequenz fw in Hz",
   "Primärinduktivität in H",
   "max. zulässige Induktion Bmax des Kernmaterials in Tesla (T)",
   "minimaler magnetischer Kernquerschnitt Amin in mm^2",
   "Primärwindungszahl N1", 
   "Sekundärwindungszahl N2",
   "Luftspaltlänge s in mm",
   "AL-Wert in H des ausgesuchten Kerns aus dem Datenbuch\n",
  };
*/

//Texts for user interaction //
const char *dialog [] =
{
   "maximum Output Power",
   "minimum Output Power",
   "maximum Input Power",
   "minimum Input Power", 
   "assumed Efficiency before calculus (60..90%)",
   "Criterion p für current waveform", 
   "current waveform", //kein Dialog fuer verlauf,da besondere Eingabe//
   "maximum Input Voltage",
   "minimum Input Voltage",
   "Output Voltage",
   "Forward drop voltage of diode",                                                                 
   "Voltage drop at secundary winding under full-load condition",
   "Voltage drop at switched-on Transistor",
   "Voltage drop at primary winding under full-load condition",
   "maximum duty cycle produced by controller-ICs ( < 50% )",
   "Transformation ratio of transformer",
   "", //Uotr berechnet, kein Dialog// 
   "Minimum Drain-Source- / Collector-Emiter- Voltage ", 
   "Maximum Voltage Uds / Uce & Ucb of chosen Transistor",
   "",
   "Maximum Current Ids / Ice of chosen Transistor",
   "Operating Frequency fw in Hz",
   "Primary Inductance in H",
   "max. allowed Induction Bmax of core material in Tesla (T)",
   "minimum magnetic cross section of core Amin in mm^2",
   "Primary turns N1", 
   "Secundary turns N2",
   "Length of Air-Gap s in mm",
   "AL-Value in H of chosen core from datasheet\n",
  };


  
enum units_of_measure { Volts, Amps, Watts, Ohms, Farad, Henry };

char *units[] = { "V", "A", "W", "Ohm", "F", "H" };


typedef struct wert
{
  double value; /* Variableninhalt je Optimierungs-Level*/
  unsigned short int level; /*Optimierungslevel 0..65535*/
  }WERT;

static WERT values[MAXVAL][MAXOPT];

unsigned short int aktual_level;

/* -----functions------------------------------------ */

void wait_for_key()
{
   char ch;	
   printf ("\nPress Enter Key to continue ...\n");
   fflush (stdout);
   do ch=getchar(); while (ch!='\n');
   /*(void) getchar ();*/
}

void clrscr()
/* clearscreen with stdout*/
{
char i;
for (i=0;i<25;i++) printf("\n");
}

int yesno()
/* returns 1 if "j","J" or "y","Y" was hit */
/* returns 0 if "n", "N" was hit*/
/*waits until one of the above is hit*/

{
char dummy,rpt;
int retval;
rpt=0;
retval=0;
do
 {
  dummy=getchar();
  printf("dummy %c\n",dummy);
  if ((dummy=='y')||(dummy=='j')||(dummy=='Y')||(dummy=='J')) {
	printf("yes");
        retval=1;
        rpt=0;
  }
  else
    if ((dummy=='n')||(dummy=='N')) {
    printf("no");
    retval=0;
    rpt=0;
    }
    else rpt=1;
 }
while (rpt);
}


void init_values() /*initialisiere globales Datenfeld*/
{
  unsigned short int i;	
  unsigned short int j;
  for (i=0;i<MAXOPT;i++)
    {
       for (j=0;j<MAXVAL;j++)
       {
       values[j][i].value=0;
       values[j][i].level=0;
       }	    
    }	    
  aktual_level=0; /*soll-level=0*/    
}	

double readvalue(double oldvalue, char *text_p, double minval, double maxval)
/*double oldvalue, minval, maxval;*/
/*char *text_p;*/
{
  char buffer[128];  /*Eingabepuffer*/
  char multiple;     /*unit-multiple*/
  double initval;    /*backup for oldvalue in errorcase*/
  unsigned short int numconv; /*zahl der Wandlungen entscheidet, ob
  				in einem anderen Format gewandelt werden muss*/
  BOOL ok;
  /*initialize*/
  initval=oldvalue; /*sicherung der Voreinstellung*/

  /*Eingabe der ersten Variablen*/		
    do
    {   
	//Deutscher Dialog
	//printf("\nEingabe %s",text_p);
	//English Dialogue        
        printf("\Input %s",text_p);
        printf(" (default=%6lg) : ",oldvalue);  
       /* printf("minimum%le\n",minval);
	printf("maximum%le\n",maxval);*/
        fgets(buffer, 127, stdin);
        /*initialisierung wiederholend*/
        ok=TRUE; /*falls TRUE wird nach Eingabe beendet. sonst wiederholt.*/
        multiple=0; /*initialisierung*/
        numconv = sscanf(buffer, "%lf %[pnumkMG?h]", &oldvalue, &multiple);

        /*Einheitsvorsaetze p,n,u,m,k,M,G auswerten */
        switch(multiple)
        {
        case 'p': oldvalue=oldvalue*1E-12;break;
        case 'n': oldvalue=oldvalue*1E-9;break;
        case 'u': oldvalue=oldvalue*1E-6;break;
        case 'æ': oldvalue=oldvalue*1E-6;break;
        case 'm': oldvalue=oldvalue*1E-3;break;
        case 'k': oldvalue=oldvalue*1E3;break;
        case 'M': oldvalue=oldvalue*1E6;break;
        case 'G': oldvalue=oldvalue*1E9;break;
	case '?': oldvalue=0; printf("Variable als 'gesucht' markiert. Nichtimplementiert!\n");break;
        case 'h': oldvalue=initval;
		ok=FALSE; /*Eingabe wiederholen*/
             	printf("\nHilfe: Fuer jede Variable wird gepromptet.");
		printf("\nWird nichts, oder nichts auswertbares eingegeben,");
		printf("\nwird der in klammern angegebene Standartwert eingesetzt.");
		printf("\nEs koennen Werte dezimal (0.344), wissenschaftlich (2.3E-4)");
		printf("\noder technisch (3.4n) eingegeben werden.");
		printf("\nFolgende Vorsilben sind zulaessig:");
		printf("\n'p'=1E-12, 'n'=1E-9, 'u' oder 'æ'=1E-6, 'm'=1E-3,");
                printf("\n'k'=1E3, 'M'=1E6, 'G'=1E9");
		printf("\nStatt einer Zahl kann auch ein '?' eingegeben werden,");
		printf("\num die jeweilige Variable als gesucht auszuweisen.");
		printf("\nDas Programm wird -falls moeglich- versuchen, alle gesuchten");
		printf("\nVariablen zu berechnen."); 
		printf("\nEnde der Hilfe, Bitte Eingabe wiederholen.\n\n");
                printf("\nKNOWN BUGS: falls eine Einheit eingegeben wird, wird sie zwar korrekt");
		printf("\nverarbeitet, jedoch stimmen defaults, und Ausgaben im weiteren Programm nicht!");
		printf("\nSSCANF-Problem etc...");
		break;

    //    default: ; /* do nothing*/
        }
        
       /* Weitere Verbesserungen:
	  bei Eingabe von h kommt ein Hilfetext (multiple=h)
	  bei Eingabe von ? wird diese Eingabe ignoriert, und der Level nicht
	  veraendert, somit wird diese Groesse als gesuchte Groesse ausgewiesen.
	  bei allen anderen eingaben wird der Level incrementiert.
	  in der neuen Form wird nur die Dialognr an die Funktion uebergeben, und die
	  zuordnung der werte ins wertefeld, und der levels erfolgt durch die
	  Eingabefktion.
	  Weiterhin laeuft die rechenfkt wie geplant: sie greift dierekt auf
	  das Value-feld zu, und aendert selbstverstaendlich nach erfolgreicher
	   Berechnung den level der berechneten groesse (inc)
	  Ist eine Berechnungsrunde abgeschlossen, wird der "soll-level inc)*/

        if (oldvalue<minval)
	    {printf("ERROR: value must be >= %6lg\n",minval);
	     oldvalue=initval; /*alten default einstellen*/
	     ok=FALSE; /*Eingabe wiederholen*/ 
            }
	if (oldvalue>maxval)
            {printf("ERROR: value must be <= %6lg\n",maxval);
	     oldvalue=initval; /*alten default einstellen*/
	     ok=FALSE; /*Eingabe wiederholen*/
	    }
    }
  while (ok=FALSE);
  //Prompt auf Deutsch
  //  printf("Eingabe: %lg\n",oldvalue);
  //English prompt
  printf("Input: %lg\n",oldvalue);
  return oldvalue;
}

void calculus()
{
double tmp; /*temporal storage to speed up processing*/
char ch;    /*temporal keyentry*/
BOOL flag;  /*emulates boolean*/
/* Gesamter Berechnungsablauf nach Wuestenhube S.92ff im Dialog*/

flag=FALSE;
clrscr();
printf("\n                C A L C U L U S");
printf("\n               -----------------\n\n");
/*Eingabe max. Ausgangsleistung*/
values[Pomax][aktual_level].value=readvalue(5,dialog[Pomax],1.0E-12,1.0E6); 
values[Pomax][aktual_level].level=aktual_level;
/*Eingabe min. Ausgangsleistung*/
values[Pomin][aktual_level].value=readvalue(0.01,dialog[Pomin],1.0E-12,
			      values[Pomax][aktual_level].value);
values[Pomin][aktual_level].level=aktual_level; 
printf("\nZunaechst wird ein Wirkungsgrad von %g fuer die Berechnungen vorgeschlagen.\n",initeff);
tmp=readvalue(75,dialog[eff],1.0E-12,100.0); 
values[eff][aktual_level].level=aktual_level;
values[eff][aktual_level].value=tmp/100; /*prozent in Faktor*/

/*Berechnung von Pimax aus Pomax und dem Wirkungsgrad eff*/
values[Pimax][aktual_level].value=values[Pomax][aktual_level].value
  /values[eff][aktual_level].value;
values[Pimax][aktual_level].level=aktual_level;

/*Berechnung von Pimin aus Pomin und dem Wirkungsgrad eff*/
values[Pimin][aktual_level].value=values[Pomin][aktual_level].value
 /values[eff][aktual_level].value;
values[Pimin][aktual_level].level=aktual_level;
printf("\nBei Annahme eines Wirkungsgrades von %lg %% betraegt die",tmp);
printf("\nmaximale Leistungsaufnahme %lg.",
        values[Pimax][aktual_level].value);
printf("\nBei Annahme eines Wirkungsgrades von %lg %% betraegt die",tmp);
 printf("\nminimale Leistungsaufnahme %lg.",
        values[Pimin][aktual_level].value);

printf("\n\nFestlegen der Betriebsart: Dreick- oder Trapez-Stromverlauf");
printf("\nDas Entscheidungskriterium ist das Verhaeltnis");
printf("\np=maximale Ausgangsleistung / minimale Ausgangsleistung");
/*Berechnung p=Pomax/Pomin*/
tmp=values[p][aktual_level].value=values[Pomax][aktual_level].value/
				values[Pomin][aktual_level].value;
values[p][aktual_level].level=aktual_level;
/*Auswertung p*/
printf("\np=%lg",tmp);
if (tmp<2) 
{  
  printf("\nTrapezfoermiger Stromverlauf wird empfohlen. (p<2)");
}
if ((tmp>2)&&(tmp<3)) 
{  
  printf("\nTrapezfoermiger oder Dreieckfoermiger Stromverlauf werden empfohlen. (2<p<3)\n");
}
if (tmp>3) 
{  
  printf("\nDreieckfoermiger Stromverlauf wird empfohlen. (p>3)");
}
printf("\nDer Empfehlung entsprechend wurde die Voreinstellung getroffen.");
printf("\nBitte Stromverlauf auswaehlen:\n\n<D>reieckfoermig\n<T>rapezfoermig\n");
printf("\njede andere Taste waehlt die Voreinstellung aus.\n");
while (flag==FALSE) 
 {
   ch=getchar();
   ch=tolower(ch);
   switch (ch)
   {
    case 't': flag=TRUE;
      values[verlauf][aktual_level].value=trapez; 
      values[verlauf][aktual_level].level=aktual_level;
    break; 

    case 'd': flag=TRUE;
      values[verlauf][aktual_level].value=dreieck; 
      values[verlauf][aktual_level].level=aktual_level;
    break;  
    default : 
      flag=TRUE;
      if (tmp<2) 
        {
         values[verlauf][aktual_level].value=trapez; 
         values[verlauf][aktual_level].level=aktual_level;
        }
      else
        {
         values[verlauf][aktual_level].value=dreieck; 
         values[verlauf][aktual_level].level=aktual_level;
        }
    break;
   } /*switch(ch)*/
 } /*while (flag=FALSE)*/ 

if (values[verlauf][aktual_level].value==dreieck)
  printf("\nDreieck");
if (values[verlauf][aktual_level].value==trapez) 
  printf("\nTrapez");
printf("foermiger Stromverlauf wurde ausgewaehlt.\n");
wait_for_key();

clrscr();
printf("\nEckdaten des Sperrwandlers");
printf("\n-----------------------------");
printf("\n\nIm folgenden werden die Anforderungen an den Sperrwandler vorgegeben");
printf("\nErmittle Uebersetzungsverhaeltnis des Trafos (Kapitel 7.9)\n");

/*Eingabe max. Eingangsspannung DC hinter evtl. Gleichrichter u. Siebung
unter Beruecksichtigung von Ueberspannung*/
values[Uimax][aktual_level].value=readvalue(410,dialog[Uimax],1.0E-6,1.0E6);
values[Uimax][aktual_level].level=aktual_level; 

/*Eingabe min. Eingangsspannung DC hinter evtl. Gleichrichter u. Siebung
unter Beruecksichtigung von Unterspannung*/
values[Uimin][aktual_level].value=readvalue(14,dialog[Uimin],1.0E-6,
                              values[Uimax][aktual_level].value);  
values[Uimin][aktual_level].level=aktual_level; 

/*Eingabe Ausgangsspannung DC hinter  Gleichrichterdiode u. Siebung*/
values[Uo][aktual_level].value=readvalue(24,dialog[Uo],1.0E-12,1.0E12);  
values[Uo][aktual_level].level=aktual_level; 

/*Eingabe Durchlasspannung der Gleichrichterdiode*/
values[Uf][aktual_level].value=readvalue(0.7,dialog[Uf],1.0E-12,100.0);
values[Uf][aktual_level].level=aktual_level; 

/*Eingabe Spannungsabfall Sekundaerwicklung bei Vollast*/
values[Uls][aktual_level].value=readvalue(1,dialog[Uls],1.0E-12,1.0E3);  
values[Uls][aktual_level].level=aktual_level; 

/*Eingabe Restspannung Transistor eingeschaltet*/
values[Usat][aktual_level].value=readvalue(1,dialog[Usat],1.0E-12,1.0E12);  
values[Usat][aktual_level].level=aktual_level; 

/*Eingabe Spannungsabfall Primaerwicklung bei Vollast*/
values[Ulp][aktual_level].value=readvalue(1,dialog[Ulp],1.0E-12,1.0E3);  
values[Ulp][aktual_level].level=aktual_level; 

/*Eingabe max. Tastverhaeltnis des AnsteuerIC*/
values[VTmax][aktual_level].value=readvalue(0.45,dialog[VTmax],1.0E-12,1.0); 
values[VTmax][aktual_level].level=aktual_level;

/*berechne K7.9*/
/* ue=(1-VTmax)*(Uo+Uf+Uls)/(VTmax*(Uimin-Usat-Ulp)) */
tmp= (1-values[VTmax][aktual_level].value)*(values[Uo][aktual_level].value
       +values[Uf][aktual_level].value+values[Uls][aktual_level].value)
      /(values[VTmax][aktual_level].value
      *(values[Uimin][aktual_level].value
      -values[Usat][aktual_level].value
      -values[Ulp][aktual_level].value));


printf("\nDas Uebersetzungsverhaeltnis des Transformators wurde berechnet: %lg",tmp);
printf("\nDa in der Praxis selten ein Transformator mit dem berechneten");
printf("\nUebersetzungsverhaeltnis zur Verfuegung steht, kann der Wert jetzt");
printf("\neingegeben werden. Der berechnete Wert wird vorgegeben und steht.");
printf("\nin Klammern.\n");
values[TR][aktual_level].value=readvalue(tmp,dialog[TR],1.0E-12,1.0E12); 
values[TR][aktual_level].level=aktual_level;

clrscr();
printf("\nAuswahl des Schalttransistors");
printf("\n-----------------------------");
printf("\n\nIm folgenden werden die theoretischen Minimalanforderungen berechnet.");
printf("\nDanach wird ein Sicherheitsfaktor von %g beruecksichtigt.",designfactor);
printf("\nDiesen Werte sollte der eingesetzte Transistor mindestens erfuellen");
printf("\num einen sicheren Betrieb zu gewaehrleisten.\n");
printf("\nMinimalanforderung Spannungsfestigkeit des Schalttransistors:");
printf("\nWaehrend der Sperrphase liegt an der Primaerwicklung die maximale");
printf("\nEingangsspannung zuzueglich der um das Uebersetzungsverhaeltnis ue");
printf("\nuebersetzten Ausgangsspannung an:\n");
printf("\ndaher muss gelten: Uds>=Uimax+1/ue*(Uo+Uf)+Uimax. Beim Bipolartransistor");
printf("\netntsprechend Ucb0 und Uce0 statt Uds beim MOSFET.\n");
/*berechne W.3.1.2.1 mit W.3.1.2.6: formel im Text*/
/*Uotr=1/ue*(Uo+Uf)+Uimax*/
values[Uotr][aktual_level].value= 1/(values[TR][aktual_level].value)
     *(values[Uo][aktual_level].value+values[Uf][aktual_level].value);
values[Uotr][aktual_level].level=aktual_level; 

/*Udsmin=Uotr+Uimax*/
tmp=values[Udsmin][aktual_level].value=values[Uotr][aktual_level].value
     +values[Uimax][aktual_level].value;
values[Udsmin][aktual_level].level=aktual_level; 

printf("\nDer Schalttransistor muss in der Sperrphase mindestens %lg Volt",tmp);
printf("\nvertragen koennen.");
tmp=tmp*designfactor;

printf("\nBei einem Sicherheitsfaktor von %g sollte ein Transistor mit",designfactor);
printf("\nUds > %lg Volt ausgewaehlt werden.i\n",tmp);
values[Uds][aktual_level].value=readvalue(tmp,dialog[Uds],tmp,1.0E12);
values[Uds][aktual_level].level=aktual_level;


/* berechne maximalen Strom Idsm */
if (values[verlauf][aktual_level].value=dreieck) 
{
/*W.3.1.2.11: ICMmax=2*Pimax*((1/Uo')+(1/Uimin)) */
tmp=values[Idsmin][aktual_level].value=2*values[Pimax][aktual_level].value
     *( 1/(values[Uotr][aktual_level].value)
       +1/(values[Uimin][aktual_level].value) );
}
else
{
  /*W.3.1.2.13: ICMmax=(Pimax+Pimin)*((1/Uo')+(1/Uimin)) */
tmp=values[Idsmin][aktual_level].value
  =(values[Pimax][aktual_level].value+values[Pimin][aktual_level].value)
     *( 1/(values[Uotr][aktual_level].value)
       +1/(values[Uimin][aktual_level].value) );
}
values[Idsmin][aktual_level].level=aktual_level; 

printf("\nDer Schalttransistor mindestens %lg Ampere",tmp);
printf("\nvertragen koennen.");
tmp=tmp*designfactor;
printf("\nBei einem Sicherheitsfaktor von %g sollte ein Transistor mit",designfactor);
printf("\nIds > %lg Amp ausgewaehlt werden.\n",tmp);
values[Ids][aktual_level].value=readvalue(tmp,dialog[Ids],tmp,1.0E12); 
values[Ids][aktual_level].level=aktual_level; 


clrscr();
printf("\nDimensionierung des Transformators");
printf("\n-----------------------------");
printf("\n\nBeim der Dimensionierung Transformators sind kleine");
printf("\nStreu- und Wicklungsverluste das vorrangige Ziel.\n");

/*Eingabe Betriebsfrequenz*/
values[fw][aktual_level].value=readvalue(50E3,dialog[fw],16.0,100.0E6);
values[fw][aktual_level].level=aktual_level;

/* Berechnung der Primärinduktivitaet LP */
/* W seite 107: bei trapezförmigem Verlauf wird p=1 gesetzt. das Vereinfacht W.3.1.5.10=K7.12 zu K7.11 */

if (values[verlauf][aktual_level].value==dreieck)
{
/*berechne Lp fuer Dreieckfoermigen Stromverlauf nach K.7.11*/
/*  Lp=( 2*Pimax / (sqr(ICM)*fw )      sqr=quadrat von= hoch2  */
tmp=values[Lp][aktual_level].value=( 2*values[Pimax][aktual_level].value )
				  / ( 2* pow(values[Idsmin][aktual_level].value,2)
				  * values[fw][aktual_level].value );

}
else
{
/*  berechne Primärinduktivität Lp fuer trapezfoermigen Stromverlauf */
/*  mit W.3.1.5.10: ist identisch mit K7.12, da K von W abgeschrieben hat*/
/*  Lp=( Pimax*sqr(p+1) / (2*sqr(ICM)*fw*p )      sqr=quadrat von= hoh2  */


tmp=values[Lp][aktual_level].value=( values[Pimax][aktual_level].value * pow(values[p][aktual_level].value+1,2) )
				  / ( 2* pow(values[Idsmin][aktual_level].value,2)
				  * values[fw][aktual_level].value * values[p][aktual_level].value );
}

printf("\nDie Primärinduktivitaet Lp ergibt sich zu %lg mH\n",tmp*1000); /* Umrechnung H in mH */

/*Eingabe max. Zulässige Induktion Bmax */
values[Bmax][aktual_level].value=readvalue(0.3,dialog[Bmax],1.0E-12,1.0E12);  
values[Bmax][aktual_level].level=aktual_level; 

/*Eingabe effektiver magnetischer Kernquerschnitt Ae*/
values[Amin][aktual_level].value=readvalue(50,dialog[Amin],1.0E-12,1.0E3);  
values[Amin][aktual_level].level=aktual_level;


/*berechne Primärwindungszahl N1 mit K7.16 in Wuestehube ohne Faktor 1E4 Gleichung W.3.1.5.11*/
/* Verwende Faktor 1E6, da ich Eingabe in mm^2 bevorzuge statt cm^2 mit Faktor 1e4 */
/*  N1>=( Lp*ICMax*1E4 ) / ( Bmax*Amin ) */

tmp=( values[Lp][aktual_level].value * values[Idsmin][aktual_level].value *1E6)
				  / ( values[Bmax][aktual_level].value * values[Amin][aktual_level].value );

printf("\nDie minimale Primaerwindungszahl N1 ergibt sich zu %lg \n",tmp);
printf("\nDa sich nicht ganzzahlige Windungszahlen schlecht realisieren lassen");
printf("\nGeben Sie bitte die neue aufgerundete Windungszahl ein.");
printf("\nDie Mindestwindungszahl darf nicht unterschritten werden.\n");

/*runden irgendwie mit modf tmp = modf(tmp, &tmp); */

values[N1][aktual_level].value=readvalue(tmp,dialog[N1],tmp,1.0E12);
values[N1][aktual_level].level=aktual_level;


/*berechne Sekundaerwindungszahl N2 mit  K.7.17*/
/* N2=ue*N1 */

tmp=values[N2][aktual_level].value= values[TR][aktual_level].value * values[N1][aktual_level].value;
values[N2][aktual_level].level=aktual_level;
printf("\nDie Sekundaerwindungszahl N2 ergibt sich zu %lg",tmp);
printf("\nbei einem Uebersetzungsverhseltnis von %lg \n",values[TR][aktual_level].value);
printf("\nFalls N2 gerundet werden muss, ist es sinnvoll, die Rundung von N1 mitzubetrachten,");
printf("\num die Abweichungen vom Übersetzungsverhaeltnis gering zu halten.");

/*berechne Luftspalt s aus K7.22 identisch mit W.3.1.5.17*/
tmp=values[s][aktual_level].value=mue0 * pow(values[N1][aktual_level].value,2)
   *values[Amin][aktual_level].value/values[Lp][aktual_level].value;

printf("\nDer notwendige Luftspalt im Kern betraegt %lg mm\n",tmp);

printf("\n\nBitte wählen Sie einen passenden Kern aus dem Datenbuch aus.");
printf("\nBitte Notieren sie die Kernbezeichnung und den AL-Wert in H.");
printf("\nMit dem AL-Wert werden die Windungszahlen erneut errechnet.");
printf("\nDie alten Werte werden dabei überschrieben.");
printf("\nWird das nicht gewünscht, so ist für den AL-Wert 0 einzugeben.");
printf("\nDie Nachberechnung der Windungszahlen aufgrund des AL-Wertes erfolgt in diesem Fall nicht.\n");
/*Eingabe AL-Wert in uH*/
tmp=values[AL][aktual_level].value=readvalue(0.0,dialog[AL],1.0E-12,1.0E12);
values[AL][aktual_level].level=aktual_level;
if (tmp!=0)
{
/*berechne Windungszahlen aus dem AL-Wert mit K2.17 und K.7.17*/
/* Primärwindungszahl K2.17: NP=sqrt(Lp/AL) */
values[N1][aktual_level].value=sqrt(values[Lp][aktual_level].value/values[AL][aktual_level].value);

/* Sekundärwindungszahl aus K7.17:  N2=ue*N1 */
values[N2][aktual_level].value= values[TR][aktual_level].value * values[N1][aktual_level].value;
values[N2][aktual_level].level=aktual_level;

printf("\nDie neue Primärwindungszahl N1 ergibt sich zu %lg.",values[N1][aktual_level].value);
printf("\nDie neue Sekundaerwindungszahl N2 ergibt sich zu %lg.",values[N2][aktual_level].value);

} /*if (tmp!=0) */

           
wait_for_key();

/*weiter:
Uebersetzungsverhaeltnis berechnen mit Formel Kilgenstein (K7.9)
Aus Uebersetzungsverhaeltnis Uo^ berechnen, UCE>Uo^+Uimax.
Uo^ ist die Ruecktransformierte Ausgangsspannung!
Uo^=1/TR*(Usek+Uf+Uls)  nach w3.1.2.1
Ucb0>=1,25*Uimax+Uo^ W3.1.2.6
weiterhin fuer trapez- und dreieckstrom verschiedene max-stroeme
nach Wuestenhube und Kilgenstein (vergleichen!).

*/

} /* calculus*/


void datadisplay()
{
 int i,x;
 x=2; /* x-position des coursors bei Schleifen-Eintritt fuer more */
 clrscr();
 printf ("The current flyback converter design has the following specs:\n");
 printf ("-----------------------------------------------------------\n\n");

 for (i=0; i<lastitem; i++)
 {
 /*show all data*/
 printf("%s\t\%lg\n",dialog[i],values[i][aktual_level].value);
  if (x>10) {wait_for_key();x=0;} /*more*/
 x++;
 } /*for (i=0;i=lastitem;i++) */

}

/*
void datalog()
//save all data into a spreadsheedreadable ascii-file
//format: text;<TAB>data<CR>text;<TAB>data<cr>....//
{

FILE *dfile;
int i;
printf("\nStroring data to file");
dfile=fopen("flyback.txt","a+"); //append data to logfile 
dfile=stderr;
if(dfile=NULL)
  {
  printf("\nError opening flyback.txt for append!");
  }
else  //write data
  {
   for (i=0; i<lastitem; i++)
   {
   //show all data
   fprintf(dfile,"The Data of the new design is\n\n");
   fprintf(dfile,"%s\t\%lg\n",dialog[i],values[i][aktual_level].value);
   printf(".");
   } //for (i=0;i=lastitem;i++) 
   printf("\n\nStored Data to log: flyback.txt");
  }
i=fflush(dfile);
if (i!=0) printf("\nFehler beim fushen der Datei: %d",i);
i=fclose(dfile);
if (i!=0) printf("\nFehler beim schliessen der Datei: %d",i);
} //datalog

*/

void hello()
{
  clrscr();
  printf ("\nFlyback-calculator Version 0.1b by Ludwig Jaffe (DL1EHU)\n");
  printf ("-----------------------------------------------------------\n\n");
  printf ("\nThis Version is dialog based. So a fixed course of calculation will\n");
  printf ("be executed. The program will prompt for each desired Value.\n");
  printf ("Script based operation and a sophisticated automatic procecessing\n");
  printf ("depending on the given Values is reserved for further revisions.\n");
  printf ("\nSee the book 'Schaltnetzteile' of Joachim Wuestenhube - VDE-Verlag\n");
  printf ("for details of the calculus.\n");
  printf ("\nThis program is licensed under the GPL2. \n");
  printf ("\nFlyback helped me with my calculations for my first diploma-thesis.\n");
  printf ("It had to calculate and was written quite fast. So it is not\n");
  printf ("beautiful at all. And reading the source you will find\n");
  printf ("this as an example for awful programming :-)\n");
  printf ("to make it worse, the program uses mixed german and english language.\n");
  printf ("A better version may follow.\n");
  printf ("\nHistory:\n");
  printf ("While cleaning up my old discs and tapes I found\n");
  printf ("Stuff from my first diploma thesis.\n"); 
  printf ("For that I wrote a small quick an dirty tool to calculate\n"); 
  printf ("the transformer of a flyback converter.\n");
  printf ("\n>My power supply worked, but there IS NO WARRANTY\n");
  printf ("OF ANY KIND FOR CORRECTNESS AND USABILITY of this program.<\n");
  printf ("\nHaving found this little piece of code I think that it\n");
  printf ("might be useful for others. I did some minor edits\n");
  printf ("to make it compile under linux and released it into the internet.\n");
  printf ("Send comments to webmaster(at)openhardware.de.\n");
  
  wait_for_key();
}	

/*---------------------------------------------------------------------*/  
  
int main()
{

  int redo=0; /*1 for repaet*/	 
  do
  {
    hello();
    init_values(); /*variablen init*/
    printf ("\n\nthanks for reading :-)\n");
    calculus();
    datadisplay(); /*show all data of the design in a table */
    // printf("\n\nStore Data to log?  <y/n>");  
    //yesno does not work
    // datalog(); //log always
    fcloseall();  //schliesse alle geoeffneten streams//
    //printf("\n\nAnother calc?  <y/n>");  
    //yesno does not work//
    printf("\nEnd of calculation\n");
    printf("\nThe program will repeat. press Control+C to abort.\n");
    wait_for_key();
    redo=1;
  }
  while (redo=1);
  return (0);
}


