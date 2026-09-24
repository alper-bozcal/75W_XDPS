#!/usr/bin/env python3
"""
DBC2 Battery Charger Controller
Bu script aşağıdaki işlemleri yapar:
1. İlk açılışta 9941 adresine 1234 yazar, 0.5 saniye bekler
2. Bu adresi okur, değer 4 değilse tekrar yazma işlemini dener (sonsuz döngü)
3. Modbus adresi 74 olan charger voltage parametresini set eder
4. Output voltage okuma parametresi 21 olan registerı saniyede bir okur
"""

import time
import sys
import os
from datetime import datetime
import minimalmodbus

# modbus_func.py dosyasını import et
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import modbus_func

def list_available_ports():
    """Mevcut COM portlarını listeler"""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        return [port.device for port in ports]
    except ImportError:
        print("pyserial kütüphanesi yüklü değil. pip install pyserial komutu ile yükleyebilirsiniz.")
        return []

def configure_modbus_settings(modbus_obj):
    """Modbus bağlantı ayarlarını yapılandırır"""
    try:
        # Modbus RTU ayarları
        modbus_obj.serial.baudrate = 9600        # Baud rate
        modbus_obj.serial.bytesize = 8           # Data bits
        modbus_obj.serial.parity = 'N'           # Parity (None)
        modbus_obj.serial.stopbits = 1           # Stop bits
        modbus_obj.serial.timeout = 1.0          # Timeout (seconds)
        modbus_obj.mode = minimalmodbus.MODE_RTU # RTU mode
        modbus_obj.clear_buffers_before_each_transaction = True
        
        print(f"Modbus ayarları:")
        print(f"  Baudrate: {modbus_obj.serial.baudrate}")
        print(f"  Timeout: {modbus_obj.serial.timeout}s")
        print(f"  Mode: RTU")
        
        return True
    except Exception as e:
        print(f"Modbus ayarları yapılandırılamadı: {e}")
        return False

def select_com_port():
    """Kullanıcıdan COM port seçimi yapar"""
    print("=== COM PORT SEÇİMİ ===")
    
    # Mevcut portları listele
    available_ports = list_available_ports()
    
    if available_ports:
        print("Mevcut COM portları:")
        for i, port in enumerate(available_ports, 1):
            print(f"  {i}. {port}")
        print(f"  {len(available_ports) + 1}. Manuel port girişi")
        
        while True:
            try:
                choice = input(f"\nSeçiminizi yapın (1-{len(available_ports) + 1}): ").strip()
                choice_num = int(choice)
                
                if 1 <= choice_num <= len(available_ports):
                    return available_ports[choice_num - 1]
                elif choice_num == len(available_ports) + 1:
                    manual_port = input("COM port adını girin (örn: COM1): ").strip()
                    return manual_port
                else:
                    print("Geçersiz seçim!")
            except ValueError:
                print("Lütfen geçerli bir sayı girin!")
    else:
        print("Otomatik port algılama başarısız.")
        manual_port = input("COM port adını manuel girin (örn: COM1): ").strip()
        return manual_port

def initialize_authentication(modbus_obj):
    """
    İlk açılışta 9941 adresine 1234 yazar ve doğrulama yapar
    Değer 4 olana kadar sonsuz döngüde dener
    """
    print("\n=== AUTHENTICATION İŞLEMİ ===")
    
    auth_register = 9941
    auth_value = 1234
    expected_response = 4
    
    # Önce register'ın mevcut değerini oku
    print(f"Register {auth_register} mevcut değeri okunuyor...")
    current_value = modbus_func.readRegister(auth_register, modbus_obj)
    if current_value is not None:
        print(f"Mevcut değer: {current_value}")
        if current_value == expected_response:
            print(f"✓ Authentication zaten tamamlanmış (değer: {current_value})")
            return True
    else:
        print("Mevcut değer okunamadı, authentication işlemine devam ediliyor...")
    
    attempt = 1
    max_attempts = 10  # Maksimum deneme sayısı
    
    while attempt <= max_attempts:
        print(f"\nDeneme {attempt}/{max_attempts}: Register {auth_register}'e {auth_value} yazılıyor...")
        
        # 9941 adresine 1234 yaz - gelişmiş retry ile
        if modbus_func.writeRegisterWithRetry(auth_register, auth_value, modbus_obj):
            print(f"✓ Yazma işlemi başarılı")
            
            # 0.5 saniye bekle
            print("0.5 saniye bekleniyor...")
            time.sleep(0.5)
            
            # Aynı adresi oku
            print(f"Register {auth_register} okunuyor...")
            read_value = modbus_func.readRegister(auth_register, modbus_obj)
            
            if read_value is not None:
                print(f"✓ Okunan değer: {read_value}")
                
                if read_value == expected_response:
                    print(f"✓ Authentication başarılı! Beklenen değer ({expected_response}) alındı.")
                    return True
                else:
                    print(f"✗ Beklenen değer {expected_response}, okunan değer {read_value}")
                    print("Yeniden denenecek...")
            else:
                print("✗ Register okuma hatası")
                print("Yeniden denenecek...")
        else:
            print("✗ Yazma işlemi başarısız")
            print("Yeniden denenecek...")
        
        attempt += 1
        
        if attempt > max_attempts:
            print(f"\n✗ {max_attempts} deneme sonrası authentication başarısız!")
            print("Lütfen aşağıdakileri kontrol edin:")
            print("  - Register 9941 yazılabilir mi?")
            print("  - Cihaz bu register'ı destekliyor mu?")
            print("  - Modbus protokolü doğru çalışıyor mu?")
            return False
        
        print(f"2 saniye sonra tekrar denenecek...")
        time.sleep(2)  # Hata durumunda biraz daha bekle

def set_charger_voltage(modbus_obj, voltage_raw):
    """
    Modbus adresi 74 olan charger voltage parametresini set eder
    voltage_raw: Ham değer (örn: 1500 = 15.00V)
    """
    charger_voltage_register = 74
    
    # Ham değeri gerçek voltaja çevir (1500 → 15.00V)
    voltage_actual = voltage_raw / 100.0
    
    modbus_func.writeRegisterWithRetry(9941, 1234, modbus_obj)
    time.sleep(0.5)
    success = modbus_func.writeRegisterWithRetry(charger_voltage_register, voltage_raw, modbus_obj)
    
    if not success:
        success = modbus_func.writeRegisterAdvanced(charger_voltage_register, voltage_raw, modbus_obj)
    
    return success

def read_output_voltage(modbus_obj):
    """
    Output voltage okuma parametresi 21 olan registerı okur
    Ham değeri gerçek voltaja çevirir (örn: 1500 → 15.00V)
    """
    output_voltage_register = 21
    
    voltage_raw = modbus_func.readRegister(output_voltage_register, modbus_obj)
    
    if voltage_raw is not None:
        # Ham değeri gerçek voltaja çevir (1500 → 15.00V)
        voltage_actual = voltage_raw / 100.0
        return voltage_actual
    else:
        print(f"✗ Output voltage okuma hatası (Register {output_voltage_register})")
        return None

def main_control_loop(modbus_obj):
    """
    Ana kontrol döngüsü - voltaj sırasını takip eder ve her voltaj için 5 örnek alır
    Voltaj sırası: [1250, 1300, 1350, 1400, 1450, 1500]
    Her voltaj değeri için 5 örnek alındıktan sonra bir sonraki voltaja geçer
    """
    voltage_sequence = [1250, 1500, 1300, 1450, 1350, 1400]
    samples_per_voltage = 5
    
    voltage_list_str = ' → '.join([f"{v/100:.2f}V" for v in voltage_sequence])
    print(f"\nVoltaj sırası: {voltage_list_str}")
    print(f"Her voltaj için {samples_per_voltage} örnek alınacak\n")
    
    try:
        voltage_index = 0
        sample_count = 0
        current_voltage = voltage_sequence[voltage_index]
        last_set_voltage = None
        error_messages = []

        while voltage_index < len(voltage_sequence):
            # İlk örnek veya yeni voltaj değeri ise voltajı ayarla
            if sample_count == 0:
                voltage_actual = current_voltage / 100.0
                print(f"\n=> Voltaj: {voltage_actual:.2f}V ({current_voltage})")
                if not set_charger_voltage(modbus_obj, current_voltage):
                    print(f"✗ Voltaj ayarlama hatası! Program durduruluyor.")
                    break
                last_set_voltage = voltage_actual
                time.sleep(0.5)  # Voltajın kararlı duruma gelmesi için bekleme

            # Örnek al
            current_time = datetime.now().strftime("%H:%M:%S")
            output_voltage = read_output_voltage(modbus_obj)
            sample_count += 1
            voltage_actual = current_voltage / 100.0

            # Set edilen voltaj ile okunan voltajı karşılaştır (ilk örnekten sonra)
            if output_voltage is not None:
                print(f"[{current_time}] Voltaj: {voltage_actual:.2f}V | Örnek {sample_count}/{samples_per_voltage} | Output: {output_voltage:.2f}V")
                # Kontrol: ilk örnekten SONRA, set edilen ve okunan voltaj farkı ±0.1V (100mV) üstünde mi?
                if sample_count > 1 and last_set_voltage is not None:
                    voltage_diff = abs(output_voltage - last_set_voltage)
                    if voltage_diff > 0.1:
                        msg = f"✗ HATA: Set edilen voltaj {last_set_voltage:.2f}V, okunan voltaj {output_voltage:.2f}V, fark {voltage_diff*1000:.0f}mV (>100mV)"
                        print(msg)
                        error_messages.append(msg)
                    else:
                        msg = f"✓ Doğru: Set edilen voltaj ile okunan voltaj arasındaki fark {voltage_diff*1000:.0f}mV (<=100mV)"
                        print(msg)
                elif sample_count == 1:
                    # İlk örnekten sonra sadece bilgi ver
                    print("✓ İlk okuma alındı.")
            else:
                print(f"[{current_time}] Voltaj: {voltage_actual:.2f}V | Örnek {sample_count}/{samples_per_voltage} | Output: OKUMA HATASI!")

            # Bu voltaj için yeterli örnek alındı mı?
            if sample_count >= samples_per_voltage:
                voltage_index += 1
                sample_count = 0
                if voltage_index < len(voltage_sequence):
                    time.sleep(0.5)  # Voltaj değişimi arasında kısa bekleme
            else:
                # 1 saniye bekle (örnekler arası)
                time.sleep(0.1)

            # Index kontrolü - sınır dışına çıkmasın
            if voltage_index < len(voltage_sequence):
                current_voltage = voltage_sequence[voltage_index]

        print(f"\n✓ Tüm voltaj sekansı tamamlandı!")
        if error_messages:
            print("\nHata Mesajları:")
            for msg in error_messages:
                print(msg)
    except KeyboardInterrupt:
        print("\nProgram durduruldu.")
    except Exception as e:
        print(f"\nHata: {e}")

def main():
    """Ana fonksiyon"""
    print("=== DBC2 BATTERY CHARGER CONTROLLER ===")
    print("Bu program DBC2 battery charger'ı kontrol eder.\n")
    
    # COM port seçimi
    com_port = select_com_port()
    print(f"\nSeçilen COM port: {com_port}")
    
    # Modbus device address (sabit)
    device_address = 1
    print(f"Device Address: {device_address} (sabit)")
    
    # Modbus bağlantısı oluştur
    print(f"\nModbus bağlantısı oluşturuluyor...")
    modbus_obj = modbus_func.create_modbus_instrument(com_port, device_address)
    
    if not modbus_obj:
        print("✗ Modbus bağlantısı oluşturulamadı!")
        return
    
    # Modbus ayarlarını yapılandır
    print("Modbus ayarları yapılandırılıyor...")
    if not configure_modbus_settings(modbus_obj):
        print("✗ Modbus ayarları yapılandırılamadı!")
        return
    
    # Bağlantıyı test et - birkaç farklı register ile
    print("Bağlantı test ediliyor...")
    test_registers = [1, 10, 21, 74]  # Bilinen registerlar
    connection_ok = False
    
    for test_reg in test_registers:
        print(f"Register {test_reg} test ediliyor...")
        if modbus_func.testConnection(modbus_obj, test_reg):
            print(f"✓ Register {test_reg} ile bağlantı başarılı")
            connection_ok = True
            break
        else:
            print(f"✗ Register {test_reg} yanıt vermiyor")
    
    if not connection_ok:
        print("\n✗ Hiçbir register yanıt vermiyor!")
        print("Lütfen aşağıdakileri kontrol edin:")
        print("  - Cihaz açık ve bağlı mı?")
        print("  - COM port doğru mu?")
        print("  - Baudrate doğru mu? (varsayılan 9600)")
        print("  - Device address doğru mu? (varsayılan 1)")
        
        retry = input("\nFarklı ayarlarla tekrar denemek ister misiniz? (y/n): ")
        if retry.lower() == 'y':
            return main()  # Programı yeniden başlat
        else:
            return
    
    print("✓ Modbus bağlantısı başarılı")
    
    # Authentication işlemi
    if not initialize_authentication(modbus_obj):
        print("✗ Authentication başarısız!")
        return
    
    confirm = input("\nOtomatik voltaj sekansını başlatmak istiyor musunuz? (y/n): ").strip().lower()
    if confirm != 'y':
        print("Program iptal edildi.")
        return
    
    main_control_loop(modbus_obj)

if __name__ == "__main__":
    main()