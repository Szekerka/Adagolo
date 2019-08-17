#include <EEPROMex.h>
#include <EEPROMVar.h>


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//I2C pins declaration
LiquidCrystal_I2C lcd(0x27, 16, 2);



//KONFIGURÁCIÓ FÜGGŐ

//relé
#define gep 1 //installált műszerek száma
int rele[1] = {12};//a relé digitális lába
int startgomb[1] = {5};//a reléhez tartozó indítógomb lába
int enterPin = 3; //  az encoder nyomógombjának a lába (alapállapotban zárt)

/*************************************************/

//MENÜ
int aktualisGep = 0;//a kijelzőn megjelenő profil azonosító száma
int menu = 1;// aktuális menümélység

//ADAGOLÓ TÖMB
long adagolo[gep][11];//a fő adatmátrix

//IDŐ
unsigned long milli = 0;
unsigned long micro = 0;

//IDŐ ÉS MENNYISÉG VÁLTOZŐK IDEIGLENES MÁSOLATA A KALIBRÁLÁSHOZ
unsigned long ido = 0;
unsigned long mennyiseg = 0;
unsigned long xx = 0;//változó
int kalibralas = 0;
unsigned long kalibraloIdo = 0;
unsigned long kalibraloInditas = 0;
unsigned long kalibraloLeallitas = 0;

//GOMBKEZELÉS
unsigned long enterTime = 0;//a gomb lenyomásának a pillanata
int enterMode = 0;//a gomb jelenlegi regisztrált állapota

//ENCODER
#define outputA 4
#define outputB 2
int counter = 0;
int aState;
int aLastState;

//virtuális számlálók a mert a pergésmenességhez két lépést számolunk egynek
int virtualast_count = 0;
int last_count = 0;//utolsó érték





void setup() {
  lcd.begin();//Kezdeti megjelenítés
  lcd.backlight();//A hátsó fény bekapcsolásához
  //lcd.backlight();// A hátsó lámpa kikapcsolása
  Serial.begin (9600);


  //ENCODER
  pinMode (outputA, INPUT);
  pinMode (outputB, INPUT);
  aLastState = digitalRead(outputA); //Elolvassa a kimenet kezdeti állapotátA

  pinMode (enterPin, INPUT); //encoder enter

  //[0] status 0/1
  //[1] relé lábkiosztása
  //[2] relé állapota 0/1 Alapállapotban nyitva ezért az a magas
  //[3] gomb lábkiosztása
  //[4] gomb állapota
  //[5] adagolásidő mikroszekundumban
  //[6] mennyiség
  //[7] indítás ideje
  //[8] leállítás ideje
  //[9] kalibráló idő
  //[10] kalibráló mennyiség


  //INICIALIZÁLÁS
  for (int i = 0; i < gep; i++) {
    //Serial.print(F("profil szama:"));
    //Serial.println(i);

    adagolo[i][0] = EEPROM.readInt(i * 100); //adagoló státusza. 100 byte-ot adok gépenként at EEPROM-ból
    //Serial.print(F("adagolo[i][0] adagolo státusza:"));
    //Serial.println(adagolo[i][0]);

    adagolo[i][1] = rele[i];//betöltöm a relék lábkiosztást
    //Serial.print(F("adagolo[i][1] rele labkiosztas:"));
    //Serial.println(adagolo[i][1]);

    adagolo[i][2] = 0; //relé állapota
    pinMode(rele[i], OUTPUT);
    digitalWrite(rele[i], HIGH);
    //Serial.print(F("adagolo[i][2] rele allapota:"));
    //Serial.println(adagolo[i][2]);

    adagolo[i][3] = startgomb[i];//betöltöm a startgomb lábkiosztást
    //Serial.print(F("adagolo[i][3] startgomb lábkiosztas:"));
    //Serial.println(adagolo[i][3]);

    adagolo[i][4] = 0;// gomb állapota 0 nincs megnyomva
    //Serial.print(F("adagolo[i][4] gomb allapota:"));
    //Serial.println(adagolo[i][4]);

    pinMode(startgomb[i], INPUT);

    if (EEPROM.readLong(i * 100 + 8) >= 0) {
      adagolo[i][5] = EEPROM.readLong(i * 100 + 8); //betölti az beállított adagolási időt
    }
    //Serial.print(F("adagolo[i][5] adagolasi ido:"));
    //Serial.println(adagolo[i][5]);

    if (EEPROM.readLong(i * 100 + 16) >= 0) {
      adagolo[i][6] = EEPROM.readLong(i * 100 + 16); //betölti a mennyiséget
    }
    //Serial.print(F("adagolo[i][6] adagolasi mennyiseg:"));
    //Serial.println(adagolo[i][6]);
    //EEPROM.updateLong(int address, uint32_t value);

    adagolo[i][7] = 0;// adagolás kezdete
    adagolo[i][8] = 0;//adagolás vége

    if (EEPROM.readLong(i * 100 + 24) > 0) {
      adagolo[i][9] = EEPROM.readLong(i * 100 + 24); //kalibráló idő
    }
    //Serial.print(F("adagolo[i][9] kalibralo ido:"));
    //Serial.println(adagolo[i][9]);

    if (EEPROM.readLong(i * 100 + 32) > 0) {
      adagolo[i][10] = EEPROM.readLong(i * 100 + 32); //kalibráló mennyiség
    }
    //Serial.print(F("adagolo[i][10] kalibralo mennyiseg:"));
    //Serial.println(adagolo[i][10]);
    //Serial.println("");

  }


  //bekapcsoláskor megjelenő profil
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mennyi  ");
  mennyiseg = adagolo[0][6];
  lcd.print(mennyiseg);
  lcd.print("ml");
  lcd.setCursor(0, 1);
  lcd.print("Ido     ");
  ido = adagolo[0][5];
  timeConverter(ido);//ki is írja a képernyőre

  //Serial.println(ido);
  //Serial.println("setup vege");

}

//mikroszekundumból olvashatót mumus
void timeConverter(long t) {
  //1 millio µs = 1 s
  int perc=  micro = (t / 1000000) / 60;// az összes másodpercen hány egész perc van

  int masodperc = micro =(t / 1000000) % 60; //mert visszaszorozhatom perc nélkül
  
  int tizedmasodperc = micro = (t-(perc*60*1000000)-(masodperc* 1000000) )/100000;


  lcd.print(perc);
  lcd.print(":");
  lcd.print(masodperc);
  lcd.print(".");
  lcd.print(tizedmasodperc);
/*
  Serial.print(t);
  Serial.println(" t/");
  Serial.print(perc);
  Serial.println(" perc");
  Serial.print(masodperc);
  Serial.println(" masodperc");
  Serial.print(tizedmasodperc);
  Serial.println(" tizedmasodperc");
*/

  //maradé k idő az encoder kerekítéséhez
  micro = t-(perc*60*1000000)-(masodperc* 1000000) ;

  t = 0;
  perc = 0;
  masodperc = 0;
  tizedmasodperc = 0;
  
}





//az adagolás időzítését micros() segítségével határozom meg, mikroszekundumban
//1000 mikroszekundum van milliszekundumban és 1 000 000 mikroszekundum egy másodperc alatt.
//4294967295 a micros átfordulása
//unsigned long  8byte helyfoglalással
void adagolas() {
  for (int i = 0; i < gep; i++) {
    if (adagolo[i][0]){ //ha az adagoló aktív
      if (adagolo[i][2] == 0) { //relé kikapcsolva
        if (digitalRead(adagolo[i][3]) == 0) { //ha az adagoló gombja nyomva  (0)
          
          //kalibrálás közben nem indítunk adagolást
          if (menu == 3) {return;}
          //ha a mennyiség 0, nem adagolunk
          if(mennyiseg == 0){return;}

          

          // && (adagolo[i][6] > 0)
          adagolo[i][4]++; //számoltatom pár kört a gombot a pergésmentesítés miatt
          if (adagolo[i][4] > 200){ //változtatható késleltetés a pergésmentesítéshez

            adagolo[i][4] = 0;//a gomb visszaállhat alaphelyzetbe
            adagolo[i][2] = 1;//relé bakapcsolva *****
            digitalWrite(adagolo[i][1], LOW);
            Serial.print(i);
            Serial.println(" rele bekapcsolva #1");
            micro = micros();

            //új beállítások elmentése a következő méréskor
            if ((menu = 1) && (mennyiseg != adagolo[i][6])) {
              adagolo[i][6] = mennyiseg;
              EEPROM.updateLong(i * 100 + 16, adagolo[i][6]);
              adagolo[i][5] = ido;
              EEPROM.updateLong(i * 100 + 8, adagolo[i][5]);
              Serial.println("uj adagolasi adatok mentese #01");
            }

            //az adagolás végének a meghatározása
            if ((4294967295 - micro) >= adagolo[i][5]) { //ha az átfordulásig több idő van mint ami az adagoláshoz szükséges
              adagolo[i][8] = micro + adagolo[i][5];// a zárás ideje ekkor lesz vagy ez után
            } else {
              adagolo[i][8] = adagolo[i][5] - (4294967295 - micro); //korrigálom az átfordulást
            }
            adagolo[i][7] = micro;//mindeképp mentem a kezdést is
            micro = 0;
          }//valóban jelet kaptunk
        } else { // ha a gomb állapot számolás közben szakadást találok visszaállítom 1-re
          adagolo[i][4] = 0; //gomb visszaállítása alaphelyzetbe
        }

        //////////////////////////
      } else { //ha a relé be van kapcsolva
        micro = micros();
        if (micro >= adagolo[i][8]) {
          if ((adagolo[i][8] - adagolo[i][5]) > 0) { //ha az indítás nullánál nagyobb
            digitalWrite(adagolo[i][1], HIGH);
            adagolo[i][2] = 0;//relé kikapcsolva *****
            adagolo[i][7] = 0;//nyitás idő törlése
            adagolo[i][8] = 0;//zárás idő törlése
            Serial.print(i);
            Serial.println(" rele ki #1");
          } else {
            if (micro < adagolo[i][7]) {
              digitalWrite(adagolo[i][1], HIGH);
              adagolo[i][2] = 0;//relé kikapcsolva *****
              adagolo[i][7] = 0;//nyitás idő törlése
              adagolo[i][8] = 0;//zárás idő törlése+
              Serial.print(i);
              Serial.println(" rele ki#2");
            }
          }
        }

        /*
        //Ha kalibrálásra lépünk akkor leálítja az aktuális adagolást
        if ((menu == 2) && (kalibralas == 0)) {
          digitalWrite(adagolo[i][1], HIGH);
          adagolo[i][2] = 0;//relé kikapcsolva *****
          adagolo[i][7] = 0;//nyitás idő törlése
          adagolo[i][8] = 0;//zárás idő törlése+
          Serial.println(i);
          Serial.println(" rele ki kalibral #3");
        }
        */
        micro = 0;
      }//************
    }//ha az adagoló aktív
  }//for vége
}


//tekerő ézékelésének a beállítása és pergésmentesítés
long pergesmentesites(long valtozo) {
  aState = digitalRead(outputA); //  A kimenet "aktuális" állapotát olvassa

  // Ha a kimenetA korábbi és aktuális állapota eltérő, ez azt jelenti, hogy inpulzus történt
  if (aState != aLastState) {
    // Ha a kimenetiB állapot különbözik a kimenet állapotától, ez azt jelenti, hogy a kódoló az óramutató járásával megegyező irányban forog
    if (digitalRead(outputB) != aState) {
      //Ha a vSzam kettőt lépett az szálálóhoz képest akkor
      //1-el növelem a számlálót majd kiegyenlítem a vSzam-ot
      //Így kettő kattanásra csak egyet lép de nem perget
      virtualast_count++;
      if ((virtualast_count - 2) == counter) {
        counter++;
        virtualast_count--;
      }
    } else {
      virtualast_count--;
      if ((virtualast_count + 2) == counter) {
        counter--;
        virtualast_count++;
      }
    }
    if (counter != last_count ) {
      if (last_count < counter) {
        valtozo++;
      }
      if (last_count > counter) {
        valtozo--;
      }
      if (valtozo < 0) {
        valtozo = 0;
      }
      last_count = counter;// itt írom felül az eddigi stabil utolsó értéke
      Serial.print("Encoder+- ");
      Serial.println(valtozo);
    }
  }
  aLastState = aState; // Frissíti a kimenet előző állapotát az aktuális állapotmal

  return valtozo;
}


int enterStatus() {
  if ((enterTime > 0) && (digitalRead(enterPin) == 1)) {
    //gomb felengedése után

    milli = millis() - enterTime;
    if (milli < 100) {
      enterTime = 0;
      enterMode = 0;
      Serial.print("tul rovid enter ");
      Serial.println(milli);
      return enterMode;
    }
    if (milli < 2000) {
      enterTime = 0;
      enterMode = 1;
      Serial.print("rovid enter ");
      Serial.println(milli);
      return enterMode;
    }
    if ((milli >= 2000) && (milli < 10000)) {
      enterTime = 0;
      enterMode = 2;
      Serial.print("hosszu enter ");
      Serial.println(milli);
      return enterMode;
    }
    if (milli >= 10000) {
      enterTime = 0;
      enterMode = 3;
      Serial.println("tul hosszu enter");
      Serial.println(milli);
      return enterMode;
    }

  }

  if (digitalRead(enterPin) == 0) {
    if (enterTime == 0) {
      enterTime = millis();//első lenyomáskor menti az idejét
    }
  }

  //alapesetben a gomb 0
  enterMode = 0;
  return enterMode;

}

void enter(int i ) {
//ha különböző menümélységben entert kapunk hogyan reagál
  if (menu == 1) { 
    menu = 2;//mennyiseg Kalibrálás
    mennyiseg = adagolo[i][10];
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Mennyi> ");
    lcd.print(adagolo[i][10]);
    lcd.print("ml");
    lcd.setCursor(0, 1);
    lcd.print("Ido     ");
    timeConverter(adagolo[0][9]);
    Serial.println("kalibralo mennyiseg beallit");
    check();
    return;
  }

  if (menu == 2) {
    menu = 3;//mennyiség Kalibrálás
    ido = adagolo[0][9];
    lcd.setCursor(0, 0);
    lcd.print("Mennyi  ");
    lcd.print(mennyiseg);
    lcd.print("ml");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Ido   > ");
    timeConverter(adagolo[0][9]);
    Serial.println("kalibralo ido beallitasa");
    check();
    return;
  }


  if (menu == 3) {

    if (kalibralas == 1) {
      digitalWrite(adagolo[i][1], HIGH);
      adagolo[i][2] = 0;//relé kikapcsolva *****
      kalibralas = 0;
      calibra();
      Serial.println(" rele ki,kalibra");
      return;
    }
    kalibraloIdo = 0;
    kalibraloLeallitas = 0;
    kalibraloInditas = 0;

    

    if (mennyiseg !=  adagolo[i][10]) { //elmentem a jó beállítást
      adagolo[i][10] = mennyiseg;
      mennyiseg = 0;
      EEPROM.updateLong(i * 100 + 32, adagolo[i][10]);
      Serial.println("k mennyiseg mentese");
    }
    if (ido !=  adagolo[i][9]) { //elmentem az jó beállítást
      adagolo[i][9] = ido;
      ido = 0;
      EEPROM.updateLong(i * 100 + 24, adagolo[i][9]);
      Serial.println("k ido mentese>");
    }

    //újraszámolni a beállítás időzítését
    adagolo[0][5] = adagolo[i][6] * (adagolo[i][9] / adagolo[i][10]);
    mennyiseg = adagolo[i][6];
    ido = adagolo[i][5];

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mennyi  ");
    lcd.print(mennyiseg);
    lcd.print("ml");
    lcd.setCursor(0, 1);
    lcd.print("Ido     ");
    timeConverter(ido );
    Serial.println("k mentese");
    menu = 1;//kalibrálás mentése és visszalépés a profilra
    check();
    return;
  }
}

void check() {
  
  Serial.print("profil:");
  Serial.print(aktualisGep);
  Serial.print(" menu:");
  Serial.print(menu);
  Serial.print(" mennyiseg:");
  Serial.print(mennyiseg);
  Serial.print(" ido:");
  Serial.println(ido);

}

void profiltorles(int i) {
  xx = 0;
  ido = 0;
  mennyiseg = 0;
  adagolo[i][2] = 0;//relé
  adagolo[i][4] = 0;//inditogomb nincs megnyomva
  adagolo[i][5] = 0;//idő
  adagolo[i][6] = 0;//mennyiséget
  adagolo[i][9] = 0;//kalbralos Ido
  adagolo[i][10] = 0;//kalibrakos mennyiseg
  EEPROM.updateLong(i * 100 + 8, 0); //idő
  EEPROM.updateLong(i * 100 + 16, 0); //mennyiséget
  EEPROM.updateLong(i * 100 + 24, 0); //kalbralos Ido
  EEPROM.updateLong(i * 100 + 32, 0); //kalibrakos mennyiseg
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mennyi  ");
  lcd.print(mennyiseg);
  lcd.print("ml");
  lcd.setCursor(0, 1);
  lcd.print("Ido     ");
  timeConverter(ido );
  Serial.println("reset");
  check();
}

void calibra() {
  kalibraloLeallitas = micros();
  if ((kalibraloLeallitas >= kalibraloInditas) && (kalibraloLeallitas <= 4294967295)) {
    kalibraloIdo = kalibraloLeallitas - kalibraloInditas;
  }
  if (kalibraloLeallitas < kalibraloInditas) {
    kalibraloIdo = (4294967295 - kalibraloInditas) + kalibraloLeallitas;
  }
  /*
  Serial.print("kalibraloInditas: ");
  Serial.println(kalibraloInditas);
  Serial.print(" kalibraloLeallitas: ");
  Serial.println(kalibraloLeallitas);
  Serial.print("kalibralo ido: ");
  Serial.println(kalibraloIdo);
  Serial.print("kalibralo ido: ");
  Serial.println(kalibraloIdo);
  Serial.println(" ");
*/
}

/**************/
void loop() {

  int i = 0;
  
  adagolas();
   
  int enSt = enterStatus();
  
 

  //ha entert nyomtak
  if (enSt > 0) {
    // ha röviden megyomjuk az entert 2s<
    if (enSt == 1) {
        if(adagolo[i][2] == 1){
          digitalWrite(adagolo[i][1], HIGH);
          adagolo[i][2] = 0;//relé kikapcsolva *****
          adagolo[i][7] = 0;//nyitás idő törlése
          adagolo[i][8] = 0;//zárás idő törlése+
          Serial.print(i);
          Serial.println(" rele ki,enter");
        }
      
      enter(i);
      check();
    }
    // ha hosszan megyomjuk az entert 2s<>10s
    if (enSt == 2) {
      // ha az idő kalibrálása közben nyomják hosszan akkor az
      // elengedés pillanatában elindul egy adagolás
      //ami egy újjabb rövid enterre áll le
      if (menu == 3) {
        /*********************/
        if (kalibralas == 0) { //kalibrálás indítása adagolással####
          adagolo[i][2] = 1;//relé bakapcsolva *****
          digitalWrite(adagolo[i][1], LOW);
          Serial.print(" rele bekapcsolva ");
          Serial.println("k adagolas inditasa");
          kalibraloInditas = micros();
          ido = 0;
          kalibralas = 1;
        }
      }
    }

    //profi adatok törlese
    //10s nál hosszabb enterre törli a profilt
    if (enSt == 3) {
      profiltorles(i);
    }

  }
  /*****************/
  //ha eltekerték
  if (menu == 1) {
    xx = pergesmentesites(mennyiseg);//változott e az értéke
    if (mennyiseg != xx) {
      mennyiseg = xx;
      lcd.setCursor(0, 0);
      lcd.print("Mennyi  ");
      lcd.print(mennyiseg);
      lcd.print("ml");
      ido = mennyiseg * (adagolo[i][9] / adagolo[i][10]);
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Ido     ");
      timeConverter(ido );
      Serial.println("adagolo allitasa");
      check();
    }
  }

  if (menu == 2) {
    xx = pergesmentesites(mennyiseg);//változott e az értéke
    if (mennyiseg != xx) {
      mennyiseg = xx;
      xx = 0;
      lcd.setCursor(0, 0);
      lcd.print("Mennyi>         ");
      lcd.setCursor(0, 0);
      lcd.print("Mennyi> ");
      lcd.print(mennyiseg);
      lcd.print("ml");
      Serial.println("mennyiseg kalibralasa");
      check();
    }
  }

  if (menu == 3) {

    if (kalibralas == 1) {
      calibra();

      if ((ido + 100000) <= kalibraloIdo) {
        ido = kalibraloIdo;
        kalibraloIdo = 0;

        lcd.setCursor(0, 1);
        lcd.print("Ido   >         ");
        lcd.setCursor(0, 1);
        lcd.print("Ido   > ");
        timeConverter(ido);
        Serial.println("ido kalibralasa adagoloval");
        check();
      }

    }



    //idö manuális állítása
    xx = pergesmentesites(ido);
    if (ido != xx) {
  
      if (ido < xx) {
        ido = ido - micro;
        micro = 0;
        ido = ido + 1000000;
      } else {
        ido = ido - micro;
        micro = 0;
        ido = ido - 1000000;
      }
      if (ido < 0) {
        ido = 0;
      }
      xx = 0;



      lcd.setCursor(0, 1);
      lcd.print("Ido   >         ");
      lcd.setCursor(0, 1);
      lcd.print("Ido   > ");
      timeConverter(ido);
      Serial.println("ido kalibral encoder");
      check();
    }
  }


}
