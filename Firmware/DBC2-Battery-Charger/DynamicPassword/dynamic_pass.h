#ifndef __DYNAMIC_PASS_H__
#define __DYNAMIC_PASS_H__

#include <stdint.h>

typedef enum {
    DYNAMIC_PASS_STATUS_NO_ERR = 0,
    DYNAMIC_PASS_STATUS_RANDNUM_ERR,
    DYNAMIC_PASS_STATUS_ENCODE_ERR,
    DYNAMIC_PASS_STATUS_DECODE_ERR,
    DYNAMIC_PASS_STATUS_UPDATE_ERR,
}DYNAMIS_PASS_STATUS_e;

struct DynamicPass_st;

//* Uygulamaya gore implementasyonu yapilmasi gerekli fonksiyonlardir
typedef int8_t (*IEncodePassword)(struct DynamicPass_st *DynamicPassObj);
typedef int8_t (*IDecodePassword)(struct DynamicPass_st *DynamicPassObj);
typedef int8_t (*IGenerateRandomValue)(struct DynamicPass_st *DynamicPassObj);
typedef int8_t (*IUpdatePassword)(struct DynamicPass_st *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE);

typedef struct DynamicPass_st{

    uint32_t key;                   // Sifreleme yapilirken kullanilacak degiskendir
    uint32_t random_val;            // Sifre uretmek icin kullanilacak random degisken

    uint32_t *passwords;            // Fabrika, servis, kullanici, ... sifrelerini tutar
    uint32_t *passwords_encoded;
    uint16_t pass_count;            // Kac seviye sifre uretilecegini belirtir

    IEncodePassword encodePassFunc; // Uretilen random sayi kullanilarak sifrelerin encodelanmis hallerini hazirlar, passwords_encoded dizinine bu degerler yazilmalidir
    IDecodePassword decodePassFunc; // passwords_encoded alanindaki degerleri cozumleyerek sifre degerini elde eder
    IGenerateRandomValue generateRandomValueFunc; // Random sayi uretmek icin kullanilir
    IUpdatePassword updatePasswordFunc; // Callback fonksiyonudur, uretilen sifreler ve encode degerlerini uygulama katmaninda bir yere yazmak vs icin kullanilabilir

}DynamicPass_t;

/**
 * @brief Kutuphane versiyon bilgisini dondurur
 * @return Versiyon degeri [string]
 */
const char *getDynamisPasswordVersion();

/**
 * @brief Dinamik sifreleme nesnesi icin ilk ayarlari yapar
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param pass_list Uretilecek olan farkli seviye dinamik sifrelerinin tutuldugu dizidir
 * @param pass_encode_list Encode'lama islemi sonuclarinin tutulacagi RAM alanidir
 * @param pass_count Kac adet sifre turu icin dinamik sifreler uretilecegini belirtir
 */ 
int8_t setDynamicPasswordSoftware(DynamicPass_t *DynamicPassObj, uint32_t *pass_list, uint32_t *pass_encode_list, uint16_t pass_count);

/**
 * @brief Algoritmanin calistirilacagi fonksiyondur
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @return DYNAMIS_PASS_STATUS_e turunden uygulama katmaninda kullanilmak uzere sonuc bilgisi dondurur
 */
DYNAMIS_PASS_STATUS_e runDynamicPassword(DynamicPass_t *DynamicPassObj);

/**
 * @brief 
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param encodePassFunc Uretilen random sayi kullanilarak sifrelerin encodelanmis hallerini hazirlayan fonksiyondur, passwords_encoded dizinine bu degerler yazilmalidir
 * @param decodePassFunc Encode'lanmis degerleri cozumleyen fonksiyondur, sonuclari passwords RAM alaninda tutulur
 * @param generateRandValFunc Dinamik sifrelerin uretilmesi icin saglanacak random sayiyi olusturan fonksiyondur
 * @param updatePasswordFunc Uygulama katmanina bilgi aktarmak icin kullanilir, modbus-registerlarina uretilen degerlerin iletilmesi gibi...
 * @return Sorun yoksa 0 dondurulur
 */
int8_t setDynamicPasswordInterface(DynamicPass_t *DynamicPassObj, 
                                IEncodePassword encodePassFunc,
                                IDecodePassword decodePassFunc,
                                IGenerateRandomValue generateRandValFunc,
                                IUpdatePassword updatePasswordFunc);

/**
 * @brief Exor isleminde kullanilan key degiskenini gunceller
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param key Uretilen rasgele sayilar ile exor islemine tabi tutulacak degerdir
 * @return Sorun yoksa 0 dondurulur
 */
int8_t setDynamicPasswordKeyValue(DynamicPass_t *DynamicPassObj, uint32_t key);

/**
 * @brief Kullanilmakta olan key degerini dondurur
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @return Key degeri dondurulur
 */
uint32_t getDynamicPasswordKeyValue(DynamicPass_t *DynamicPassObj);

/**
 * @brief Uretilen sifrenin DynamicPassObj nesnesine aktarilmasini saglar
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param random_val Dinamik sifreler uretilirken kullanilacak degerdir
 * @return Sorun yoksa 0 doner
 */
int8_t setDynamicPasswordRandomValue(DynamicPass_t *DynamicPassObj, uint32_t random_val);

/**
 * @brief Uretilmis sifre degerini dondurur
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @return Uretilmis olan random_value degerini dondurur
 */
uint32_t getDynamicPasswordRandomValue(DynamicPass_t *DynamicPassObj);

/**
 * @brief Uretilmis olan random degere gore encode'lama islemini gerceklestirir.
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param value Encode degeri
 * @param DYNAMIC_PASS_TYPE Hangi sifre icin encode isleminin yapilacagini belirtir.
 * @return Sorun yoksa 0 dondurulur
 */
int8_t setDynamicPasswordEncodeValue(DynamicPass_t * DynamicPassObj, uint32_t value, int16_t DYNAMIC_PASS_TYPE);

/**
 * @brief Encode'lanmis sifre degerini okumak icin kullanilir
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param DYNAMIC_PASS_TYPE Hangi sifrenin dondurulecegini belirtir
 * @return Encode'lanmis sifrenin degeri dondurur, kullanicilarin cihaza erisim icin kullanmasi gereken koddur
 */
uint32_t getDynamicPasswordEncodeValue(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE);

/**
 * @brief Encode'lanmis sifreleri cozup DynamicPassObj icinde yer alan ilgili alanlara yazilmasini saglamaktadir
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param value Sifre degeridir
 * @param DYNAMIC_PASS_TYPE Hangi sifre icin decode isleminin yapilacagini belirtir.
 * @return Sorun yoksa 0 doner
 */
int8_t setDynamicPasswordDecodeValue(DynamicPass_t *DynamicPassObj, uint32_t value, uint16_t DYNAMIC_PASS_TYPE);

/**
 * @brief İstenilen sifreyi okumak icin kullanilir
 * @param DynamicPassObj Dinamik sifreleme yapisini belirtir bknz. DynamicPass_t
 * @param DYNAMIC_PASS_TYPE Hangi sifrenin dondurulecegini belirtir
 * @return Sifre degerini dondurur
 */
uint32_t getDynamicPasswordDecodeValue(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE);
#endif /* __DYNAMIC_PASS_H__ */