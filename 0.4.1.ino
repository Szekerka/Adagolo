#include <EEPROMex.h>
#include <EEPROMVar.h>


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//I2C pins declaration
LiquidCrystal_I2C lcd(0x27, 16, 2);



//KONFIGURÁLANDÓ
//relé
#define gep 1 //installált gépek száma
int rele[1] = {12};//reléhez tartózó digitális lábkiosztás
int startgomb[1] = {5};//mindegyik reléhez tartozik egy gomb is
int enterPin = 3; //  lábkiosztás  (alapállapotba zárt)

/*************************************************/

//MENÜ
int aktualisGep = 0;//a kijelzőn megjelenő profil száma
int menu = 1;// jelenlegi menümélység
//int i;

//ADAGOLÓ TÖMB
long adagolo[gep][11];

//IDŐ
unsigned long milli = 0;//változik
unsigned long micro = 0;//változik

//IDŐ ÉS MENNYISÉG VÁLTOZŐK MÁSOLATA A KALIBRÁLÁSHOZ
unsigned long ido = 0;//változik
unsigned long mennyiseg = 0;//változik
unsigned long xx = 0;//változik
int kalibralas = 0;
unsigned long kalibraloIdo = 0;//változik
unsigned long kalibraloInditas = 0;//változik
unsigned long kalibraloLeallitas = 0;//változik

//GOMBKEZELÉS
unsigned long enterTime = 0;//a gomb lenyomásának a pillanata
int enterMode = 0;//a gomb jelenlegi regisztrált állapota

//TEKERŐ
#define outputA 4
#define outputB 2
int counter = 0;
int aState;
int aLastState;

int virtualast_count = 0;//virtuális számláló
int last_count = 0;//utolsó érték





void setup() {
        lcd.begin();//Kezdeti megjelenítés
        lcd.backlight();//A hátsó fény bekapcsolásához
        //lcd.backlight();// A hátsó lámpa kikapcsolása
        Serial.begin (9600);

        //lcd.setCursor(0,0);
        //lcd.print("Inicializalas...");
        Serial.println("Inicializalas...");


        //encoder
        pinMode (outputA,INPUT);
        pinMode (outputB,INPUT);
        aLastState = digitalRead(outputA); //Elolvassa a kimenet kezdeti állapotátA

        pinMode (enterPin,INPUT);//ok gomb (encoderé)

        //[0] status AKTÍV/INAKTÍV
        //[1] relé lábkiosztása
        //[2] relé állapota (0 vagy 1)
        //[3] gomb lábkiosztása
        //[4] gomb állapota
        //[5] adagolásidő mikroszekundumban
        //[6] mennyiség
        //[7] indítás ideje
        //[8]leállítás ideje

        //INICIALIZÁLÁS
        for (int i = 0; i < gep; i++) {
                Serial.print("profil szama:");
                Serial.println(i);

                adagolo[i][0] = EEPROM.readInt(i*100); //adagoló státusza. 100 byte-ot adok gépenként at EEPROM-ból
                Serial.print("adagolo[i][0] adagolo státusza:");
                Serial.println(adagolo[i][0]);

                adagolo[i][1] = rele[i];//betöltöm a relék lábkiosztást
                Serial.print("adagolo[i][1] rele labkiosztas:");
                Serial.println(adagolo[i][1]);

                adagolo[i][2] = 0; //relé állapota
                pinMode(rele[i], OUTPUT);
                digitalWrite(rele[i], LOW);
                Serial.print("adagolo[i][2] rele allapota:");
                Serial.println(adagolo[i][2]);

                adagolo[i][3] = startgomb[i];//betöltöm a startgomb lábkiosztást
                Serial.print("adagolo[i][3] startgomb lábkiosztas:");
                Serial.println(adagolo[i][3]);

                adagolo[i][4] = 0;// gomb állapota 0 azt jelenti nincs megnyomva
                Serial.print("adagolo[i][4] gomb allapota:");
                Serial.println(adagolo[i][4]);

                pinMode(startgomb[i], INPUT);

                if(EEPROM.readLong(i*100+8) >= 0) {adagolo[i][5] = EEPROM.readLong(i*100+8);}//betölti az beállított adagolási időt
                Serial.print("adagolo[i][5] adagolasi ido:");
                Serial.println(adagolo[i][5]);

                if(EEPROM.readLong(i*100+16) >= 0) {adagolo[i][6] = EEPROM.readLong(i*100+16);}//betölti a mennyiséget
                Serial.print("adagolo[i][6] adagolasi mennyiseg:");
                Serial.println(adagolo[i][6]);
                //EEPROM.updateLong(int address, uint32_t value);

                adagolo[i][7] = 0;// adagolás kezdete
                adagolo[i][8] = 0;//adagolás vége

                if(EEPROM.readLong(i*100+24) > 0) {adagolo[i][9] = EEPROM.readLong(i*100+24);}//kalibráló idő
                  Serial.print("adagolo[i][9] kalibralo ido:");
                  Serial.println(adagolo[i][9]);

                if(EEPROM.readLong(i*100+32) > 0) {adagolo[i][10] = EEPROM.readLong(i*100+32);}//kalibráló mennyiség
                  Serial.print("adagolo[i][10] kalibralo mennyiseg:");
                  Serial.println(adagolo[i][10]);
                  Serial.println("________________");
                  Serial.println(" ");

        }


        //bekapcsoláskor megjelenő profil
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Mennyi  ");
        mennyiseg = adagolo[0][6];
        lcd.print(mennyiseg);
        lcd.print("ml");
        lcd.setCursor(0,1);
        lcd.print("Ido     ");
        ido = adagolo[0][5];
        timeConverter(ido);//ki is írja a képernyőre

        Serial.println(ido);
        Serial.println("setup vege");

}

//mikroszekundumból olvasható
void timeConverter(long t){
  int perc = ((t/1000000) / 60);
  int masodperc = ((t/1000000) % 60);//mert visszaszorozhatom perc nélkül
  //int tizedmasodperc =((((t/1000000) % 60)*60)/10);//mert tizedmásodperc
  //még majd a végét esetleg kerekítem
  lcd.print(perc);
  lcd.print(":");
  lcd.print(masodperc);
  //lcd.print(".");
  //lcd.print(tizedmasodperc);
  t = 0;
  perc = 0;
  masodperc = 0;
  //tizedmasodperc = 0;
}





//az adagolás időzítését micros() segítségével határozom meg, mikroszekundumban
//1000 mikroszekundum van milliszekundumban és 1 000 000 mikroszekundum egy másodperc alatt.
//4294967295 a micros átfordulása
//unsigned long  8byte helyfoglalással
void adagolas(){
        for (int i = 0; i < gep; i++) {
                if(adagolo[i][0]) {//ha az adagoló aktív
                        if(adagolo[i][2] == 0) {//relé kikapcsolva
                                if(digitalRead(adagolo[i][3]) == 0) {//ha az adagoló gombja nyomva  (0)
                                      if(menu == 3){return;}//kalibrálás közben nincs adagolás
                                        adagolo[i][4]++; //számoltatom pár kört a gombot a pergésmentesítés miatt
                                        if(adagolo[i][4] > 200) {//változtatható késleltetés a pergésmentesítéshez
                                                adagolo[i][4] = 0;//a gomb visszaállhat alaphelyzetbe
                                                adagolo[i][2] = 1;//relé bakapcsolva *****
                                                digitalWrite(adagolo[i][1], HIGH);
                                                Serial.println(i);
                                                Serial.println(" rele bekapcsolva");
                                                micro = micros();

                                                //új beállítások elmentése a következő méréskor
                                                if((menu = 1) && (mennyiseg != adagolo[i][6])){
                                                  adagolo[i][6] = mennyiseg;
                                                  EEPROM.updateLong(i*100+16, adagolo[i][6]);
                                                  adagolo[i][5] = ido;
                                                  EEPROM.updateLong(i*100+8, adagolo[i][5]);
                                                  Serial.println("uj adagolasi adatok mentese #01");
                                                }

                                                //az adagolás végének a meghatározása
                                                if((4294967295 - micro) >= adagolo[i][5]) {//ha az átfordulásig több idő van mint ami az adagoláshoz szükséges
                                                        adagolo[i][8] = micro + adagolo[i][5];// a zárás ideje ekkor lesz vagy ez után
                                                }else{
                                                        adagolo[i][8] = adagolo[i][5] -(4294967295 - micro);//korrigálom az átfordulást
                                                }
                                                adagolo[i][7] = micro;//mindeképp mentem a kezdést is
                                                micro = 0;
                                        }//valóban jelet kaptunk
                                }else{// ha a gomb állapot számolás közben szakadást találok visszaállítom 1-re
                                        adagolo[i][4]= 0;//gomb visszaállítása alaphelyzetbe
                                }

                                //////////////////////////
                        }else{//ha a relé be van kapcsolva
                                micro= micros();
                                if(micro >= adagolo[i][8]) {
                                        if((adagolo[i][8] - adagolo[i][5]) > 0) {//ha az indítás nullánál nagyobb
                                                digitalWrite(adagolo[i][1], LOW);
                                                adagolo[i][2] = 0;//relé kikapcsolva *****
                                                adagolo[i][7] = 0;//nyitás idő törlése
                                                adagolo[i][8] = 0;//zárás idő törlése
                                                Serial.println(i);
                                                Serial.println(" rele kikapcsolva");
                                        }else{
                                                if(micro < adagolo[i][7]) {
                                                        digitalWrite(adagolo[i][1], LOW);
                                                        adagolo[i][2] = 0;//relé kikapcsolva *****
                                                        adagolo[i][7] = 0;//nyitás idő törlése
                                                        adagolo[i][8] = 0;//zárás idő törlése+
                                                        Serial.println(i);
                                                        Serial.println(" rele kikapcsolva");
                                                }
                                        }
                                }
                                //Ha kalibrálásra lépünk akkor leálítja az aktuális adagolást
                                if((menu == 3)&&(kalibralas == 0)){
                                  digitalWrite(adagolo[i][1], LOW);
                                  adagolo[i][2] = 0;//relé kikapcsolva *****
                                  adagolo[i][7] = 0;//nyitás idő törlése
                                  adagolo[i][8] = 0;//zárás idő törlése+
                                  Serial.println(i);
                                  Serial.println(" rele kikapcsolva mert kalibrálunk");
                                }
                                micro = 0;
                        }//************
                }//ha az adagoló aktív
        }//for vége
}


//tekerő ézékelésének a beállítása és pergésmentesítés
long pergesmentesites(long valtozo){
        aState = digitalRead(outputA); //  A kimenet "aktuális" állapotát olvassa

        // Ha a kimenetA korábbi és aktuális állapota eltérő, ez azt jelenti, hogy inpulzus történt
        if (aState != aLastState) {
                // Ha a kimenetiB állapot különbözik a kimenet állapotától, ez azt jelenti, hogy a kódoló az óramutató járásával megegyező irányban forog
                if (digitalRead(outputB) != aState) {
                        //Ha a vSzam kettőt lépett az szálálóhoz képest akkor
                        //1-el növelem a számlálót majd kiegyenlítem a vSzam-ot
                        //Így kettő kattanásra csak egyet lép de nem perget
                        virtualast_count++;
                        if((virtualast_count - 2)== counter) {
                                counter++;
                                virtualast_count--;
                        }
                } else {
                        virtualast_count--;
                        if((virtualast_count + 2)== counter) {
                                counter--;
                                virtualast_count++;
                        }
                }
                if(counter != last_count ){
                        if(last_count < counter){valtozo++;}
                        if(last_count > counter){valtozo--;}
                        if(valtozo < 0){valtozo = 0;}
                        last_count = counter;// itt írom felül az eddigi stabil utolsó értéke
                        Serial.print("Encoder+- ");
                        Serial.println(valtozo);
                }
        }
        aLastState = aState; // Frissíti a kimenet előző állapotát az aktuális állapotmal

        return valtozo;
}


int enterStatus() {
        if((enterTime > 0) && (digitalRead(enterPin) == 1)){
          //gomb felengedése után

                milli = millis()-enterTime;
                if(milli < 100){
                        enterTime = 0;
                        enterMode = 0;
                        Serial.print("tul rovid enter ");
                        Serial.println(milli);
                        return enterMode;
                }
                if(milli < 2000){
                        enterTime = 0;
                        enterMode = 1;
                        Serial.print("rovid enter ");
                        Serial.println(milli);
                        return enterMode;
                }
                if((milli >= 2000)&&(milli < 10000)) {
                        enterTime = 0;
                        enterMode = 2;
                        Serial.print("hosszu enter ");
                        Serial.println(milli);
                        return enterMode;
                }
                if(milli >= 10000) {
                        enterTime = 0;
                        enterMode = 3;
                        Serial.println("tul hosszu enter");
                        Serial.println(milli);
                        return enterMode;
                }

        }

        if(digitalRead(enterPin) == 0) {
                if(enterTime == 0){
                  enterTime = millis();//első lenyomás
                  }
        }

        //alapesetben a gomb 0
        enterMode = 0;
        return enterMode;

}

void enter(int i ){

  if(menu == 1){//alapállapot
    menu = 2;//mennyiseg Kalibrálás***************
    mennyiseg = adagolo[i][10];
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print("Mennyi> ");
    lcd.print(adagolo[i][10]);
    lcd.print("ml");
    lcd.setCursor(0,1);
    lcd.print("Ido     ");
    timeConverter(adagolo[0][9]);
    Serial.println("kalibralo mennyiseg beallitasa");
    check();
    return;
    }

   if(menu == 2){
    menu = 3;//mennyiség Kalibrálás
    ido = adagolo[0][9];
    lcd.setCursor(0,0);
    lcd.print("Mennyi  ");
    lcd.print(mennyiseg);
    lcd.print("ml");
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("Ido   > ");
    timeConverter(adagolo[0][9]);
    Serial.println("kalibralo ido beallitasa");
    check();
    return;
   }





  if(menu== 3){

    if(kalibralas == 1){
      digitalWrite(adagolo[i][1], LOW);
      adagolo[i][2] = 0;//relé kikapcsolva *****
      kalibralas = 0;
      calibra();
      Serial.println(" rele kikapcsolva ka kalibralas adagolas vege");
      return;
    }
    kalibraloIdo = 0;
    kalibraloLeallitas = 0;
    kalibraloInditas = 0;

    menu = 1;//kalibrálás mentése és visszalépés a profilra

    if(mennyiseg !=  adagolo[i][10]){//elmentem az jó beállítást
      adagolo[i][10] = mennyiseg;
      mennyiseg = 0;
      EEPROM.updateLong(i*100+32, adagolo[i][10]);
      Serial.println("kalibralo mennyiseg mentese");
    }
    if(ido !=  adagolo[i][9]){//elmentem az jó beállítást
      adagolo[i][9] = ido;
      ido = 0;
      EEPROM.updateLong(i*100+24, adagolo[i][9]);
      Serial.println("kalibralo ido mentese>");
    }

    //újraszámolni a beállítás időzítését
    adagolo[0][5] = adagolo[i][6] * (adagolo[i][9]/adagolo[i][10]);
    mennyiseg = adagolo[i][6];
    ido = adagolo[i][5];

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mennyi  ");
    lcd.print(mennyiseg);
    lcd.print("ml");
    lcd.setCursor(0,1);
    lcd.print("Ido     ");
    timeConverter(ido );
    Serial.println("kalibralas mentese visszalepes a profiba>");
    check();
    return;
  }
}

void check(){
  Serial.print(" menu:");
  Serial.print(menu);
  Serial.print(" mennyiseg:");
  Serial.print(mennyiseg);
  Serial.print(" ido:");
  Serial.println(ido);

  }

  void profiltorles(int i){
    xx = 0;
    ido = 0;
    mennyiseg = 0;
    adagolo[i][2] = 0;//relé
    adagolo[i][4] = 0;//inditogomb nincs megnyomva
    adagolo[i][5] = 0;//idő
    adagolo[i][6] = 0;//mennyiséget
    adagolo[i][9] = 0;//kalbralos Ido
    adagolo[i][10] = 0;//kalibrakos mennyiseg
    EEPROM.updateLong(i*100+8, 0);//idő
    EEPROM.updateLong(i*100+16, 0);//mennyiséget
    EEPROM.updateLong(i*100+24, 0);//kalbralos Ido
    EEPROM.updateLong(i*100+32, 0);//kalibrakos mennyiseg
    Serial.println("profil alaphejzetbe allitas>");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mennyi  ");
    lcd.print(mennyiseg);
    lcd.print("ml");
    lcd.setCursor(0,1);
    lcd.print("Ido     ");
    timeConverter(ido );
    Serial.println("profil törlese kesz");
    check();
  }

void calibra(){
  kalibraloLeallitas = micros();
   if((kalibraloLeallitas >= kalibraloInditas)&&(kalibraloLeallitas <=4294967295)) {
    kalibraloIdo = kalibraloLeallitas - kalibraloInditas;
   }
   if(kalibraloLeallitas < kalibraloInditas){
     kalibraloIdo = (4294967295-kalibraloInditas)+kalibraloLeallitas;
    }
    Serial.print("kalibraloInditas: ");
    Serial.println(kalibraloInditas);
    Serial.print(" kalibraloLeallitas: ");
    Serial.println(kalibraloLeallitas);
    Serial.print("kalibralo ido: ");
    Serial.println(kalibraloIdo);
    Serial.print("kalibralo ido: ");
    Serial.println(kalibraloIdo);
    Serial.println(" ");
    
}

/********************************************/
void loop(){

  int i = 0;


    adagolas();


  int enSt = enterStatus();
  //ha entert nyomtak
  if(enSt > 0){
    // ha röviden megyomták az entert
    if(enSt == 1){
      enter(i);
      check();
    }

    if(enSt == 2){
      if(menu == 3){
        /*********************/
        if(kalibralas == 0){//kalibrálás indítása adagolással####
          adagolo[i][2] = 1;//relé bakapcsolva *****
          digitalWrite(adagolo[i][1], HIGH);
          Serial.print(" rele bekapcsolva ");
          Serial.println("kalibralo adagolas inditasa");
          kalibraloInditas = micros();
          ido= 0;
          kalibralas = 1;
        }
      }
    }

    //profi adatok törlese
    if(enSt == 3){
      profiltorles(i);
    }

  }
    /*****************/
    //ha eltekerték
    if(menu == 1){
      xx = pergesmentesites(mennyiseg);//mennyiség átállítása
      if(mennyiseg != xx){
        mennyiseg = xx;
        lcd.setCursor(0,0);
        lcd.print("Mennyi  ");
        lcd.print(mennyiseg);
        lcd.print("ml");
        ido = mennyiseg * (adagolo[i][9]/adagolo[i][10]);
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("Ido     ");
        timeConverter(ido );
        Serial.println("adagolo allitasa>");
        check();
      }
    }

    if(menu == 2){
      xx = pergesmentesites(mennyiseg);
      if(mennyiseg != xx){
        mennyiseg = xx;
        xx = 0;
        lcd.setCursor(0,0);
        lcd.print("Mennyi>         ");
        lcd.setCursor(0,0);
        lcd.print("Mennyi> ");
        lcd.print(mennyiseg);
        lcd.print("ml");
        Serial.println("mennyiseg kalibralasa>");
        check();
      }
    }

    if(menu == 3){

      if(kalibralas == 1){
        calibra();

         if((ido + 1000000)<= kalibraloIdo){
          ido = kalibraloIdo;
          kalibraloIdo = 0; 
          Serial.print("ido adagolo kalivóbralas közben ");
          Serial.println(ido);
          
          lcd.setCursor(0,1);
          lcd.print("Ido   >         ");
          lcd.setCursor(0,1);
          lcd.print("Ido   > ");
          timeConverter(ido);
          Serial.println("ido kalibralasa>");
          check();
          }

      }

      


      xx = pergesmentesites(ido);
      if(ido != xx){
        if(ido < xx){
          ido = ido + 1000000;
        }else{
          ido = ido - 1000000;
        }
        if(ido < 0){
          ido = 0;
        }
        xx = 0;

        

        lcd.setCursor(0,1);
        lcd.print("Ido   >         ");
        lcd.setCursor(0,1);
        lcd.print("Ido   > ");
        timeConverter(ido);
        Serial.println("ido kalibralasa>");
        check();
      }
    }


}
