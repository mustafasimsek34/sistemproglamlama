# tarsau - Metin Dosyaları Arşivleme Programı

## Genel Bakış

`tarsau`, `tar`, `rar` veya `zip` gibi çalışan ancak **sıkıştırma yapmayan** bir arşivleme programıdır. Yalnızca ASCII metin dosyalarını birleştirerek `.sau` formatında arşiv dosyaları oluşturur ve bu arşivleri açabilir.

## Kurulum

### Linux/Unix ortamında:

```bash
cd /path/to/tarsau
make
sudo cp tarsau /usr/local/bin/  # (opsiyonel)
```

### Temizleme:
```bash
make clean
```

## Kullanım

### 1. Arşiv Oluşturma (-b modu)

**Syntax:**
```bash
./tarsau -b dosya1.txt dosya2.txt dosya3.txt -o çıkış.sau
```

**Parametreler:**
- `-b`: Birleştirme modu (build)
- `dosya1.txt dosya2.txt ...`: Birleştirilecek dosyalar (maksimum 32)
- `-o çıkış.sau`: Çıktı dosyasının adı (varsayılan: `a.sau`)

**Çıktı:**
```
Dosyalar birleştirildi.
```

**Kısıtlamalar:**
- Yalnızca ASCII metin dosyaları kabul edilir
- Toplam dosya boyutu 200 MB'ı geçemez
- Maksimum 32 dosya

**Örnekler:**
```bash
# Varsayılan çıktı dosyasını kullanarak
./tarsau -b file1.txt file2.txt file3.txt

# Özel çıktı dosyası adı
./tarsau -b report.txt data.txt output.txt -o myarchive.sau
```

### 2. Arşivi Açma (-a modu)

**Syntax:**
```bash
./tarsau -a arşiv.sau [hedef_dizin]
```

**Parametreler:**
- `-a`: Açma modu (archive/extract)
- `arşiv.sau`: Açılacak arşiv dosyası
- `[hedef_dizin]`: Dosyaların çıkarılacağı dizin (opsiyonel, varsayılan: mevcut dizin)

**Çıktı:**
```
hedef_dizin dizininde dosya1.txt, dosya2.txt ve dosya3.txt dosyaları açıldı.
```

**Örnekler:**
```bash
# Dosyaları mevcut dizine aç
./tarsau -a myarchive.sau

# Dosyaları belirtilen dizine aç
./tarsau -a myarchive.sau ./extracted/

# Hedef dizin otomatik oluşturulur
./tarsau -a backup.sau ./restore/
```

## .sau Format Yapısı

`.sau` dosyası iki ana bölümden oluşur:

### 1. Organizasyon (İçerik) Bölümü

```
┌─ 10 bayt (ASCII) ─┬──────────── Kayıtlar ────────┐
│ Org. Boyutu      │ |dosya,izin,boyut|dosya,...  │
│ (örn: 0000000065) │                              │
└──────────────────┴──────────────────────────────┘
```

**Format detayları:**
- **İlk 10 bayt**: Organizasyon bölümünün toplam boyutu (ORG_SIZE dahil), 0 ile pad'lenmiş ASCII sayı
  ```
  Örnek: "0000000065" → 65 bayt
  ```
  
- **Kayıtlar**: `|` karakteriyle başlar ve biter
  ```
  Örnek: |file1.txt,644,1024|file2.txt,755,2048|
  
  Format: |dosya_adı,izinler,boyut|
  - dosya_adı: Orijinal dosya adı (boşluk yok)
  - izinler: POSIX sekizli format (örn: 644, 755, 600)
  - boyut: Dosya boyutu (bayt cinsinden, ondalık)
  ```

### 2. Arşivlenmiş Dosyalar Bölümü

Organizasyon bölümü sona erdikten sonra, her dosyanın içeriği art arda (boşluksuz) yazılır. Sıra organizasyonda belirtilen sırayla aynıdır.

```
[file1 içeriği][file2 içeriği][file3 içeriği]...
```

### Tam Örnek

**Dosyalar:**
```
file1.txt (100 bayt, izin: 755)
file2.txt (50 bayt, izin: 644)
```

**İçerik:**
```
0000000043|file1.txt,755,100|file2.txt,644,50|[file1 içeriği 100 bayt][file2 içeriği 50 bayt]
```

## Teknik Detaylar

### Desteklenen Özellikler

✓ **Metin dosyaları**: ASCII format (karakter başına 1 bayt)
✓ **İzin koruması**: POSIX rwx bitleri (chmod)
✓ **Hata yönetimi**: Tüm durumlar kontrol edilir, temiz çıkış
✓ **Bellek güvenliği**: Bellek sızıntısı yok, dinamik tahsis
✓ **Dizin oluşturma**: Hedef dizin otomatik oluşturulur

### Kısıtlamalar

- Yalnızca metin dosyaları (null byte içermeyen)
- Maksimum 32 dosya per arşiv
- Maksimum 200 MB toplam boyut
- Sıkıştırma yapılmaz

### Hata Mesajları

| Mesaj | Nedeni | Çözüm |
|-------|--------|-------|
| `dosya.txt giriş dosyasının formatı uyumsuzdur!` | Binary/metin olmayan dosya | Yalnızca metin dosyası kullan |
| `Toplam dosya boyutu 200 MB'ı aştı!` | Çok büyük toplam boyut | Daha az/küçük dosya kullan |
| `Maksimum dosya sayısı (32) aşıldı!` | 32'den fazla dosya | Daha az dosya kullan |
| `Arşiv dosyası uygunsuz veya bozuk!` | Arşiv formatı hatalı | Arşiv dosyasını kontrol et |

## Örnek Oturum

### Arşiv Oluşturma

```bash
$ echo "İçerik 1" > t1
$ echo "İçerik 2" > t2
$ echo "İçerik 3" > t3
$ echo "İçerik 4" > t4.txt
$ echo "İçerik 5" > t5.dat

$ ./tarsau -b t1 t2 t3 t4.txt t5.dat -o s1.sau
Dosyalar birleştirildi.

$ ls -lh s1.sau
-rw-r--r-- 1 user group 87 May 20 17:07 s1.sau
```

### Arşivi Açma

```bash
$ ./tarsau -a s1.sau d1
d1 dizininde t1, t2, t3, t4.txt ve t5.dat dosyaları açıldı.

$ ls -la d1/
total 0
drwxrwxrwx 1 user user 4096 May 20 17:07 .
drwxrwxrwx 1 user user 4096 May 20 17:07 ..
-rwxrwxrwx 1 user user    8 May 20 17:07 t1
-rwxrwxrwx 1 user user    8 May 20 17:07 t2
-rwxrwxrwx 1 user user    8 May 20 17:07 t3
-rwxrwxrwx 1 user user    8 May 20 17:07 t4.txt
-rwxrwxrwx 1 user user    8 May 20 17:07 t5.dat

$ cat d1/t1
İçerik 1
```

## Kod Mimarisi

### Fonksiyonlar

**Utility Fonksiyonları:**
- `is_text_file()`: Dosyanın metin format olup olmadığını kontrol et
- `get_file_size()`: Dosya boyutunu al
- `get_file_permissions()`: İzinleri al
- `set_file_permissions()`: İzinleri ayarla

**Build Modülü:**
- `create_org_section()`: Organizasyon bölümü oluştur
- `build_archive()`: Arşiv dosyasını oluştur

**Extract Modülü:**
- `parse_org_section()`: Organizasyon bölümünü oku ve parse et
- `create_directory()`: Dizin oluştur
- `extract_archive()`: Arşivi aç ve dosyaları çıkar

**Parser:**
- `parse_args()`: Komut satırı parametrelerini işle
- `print_usage()`: Kullanım bilgisini göster

**Main:**
- `main()`: Program giriş noktası

### Veri Yapıları

```c
typedef struct {
    char filename[256];
    unsigned long size;
    unsigned int permissions;
} FileEntry;

typedef struct {
    int file_count;
    unsigned long total_size;
    unsigned long org_section_size;
    FileEntry files[MAX_FILES];
} ArchiveMetadata;
```

## Derleme ve Test

### Derleme

```bash
make           # Programı derle
make clean     # Eski dosyaları temizle
make rebuild   # Temiz derle
```

### Test

```bash
# Test betiğini çalıştır
bash test.sh
```

Test betiği:
1. Test dosyaları oluşturur
2. Arşiv oluşturur
3. Arşivi açar
4. İçeriği doğrular
5. İzinleri kontrol eder

## Sınırlamalar ve Gelecek Geliştirmeler

**Mevcut Sınırlamalar:**
- Sıkıştırma yok
- Yalnızca metin dosyaları
- Dizin desteği yok (yalnızca düz dosyalar)
- Symbolic link yok
- Versiyon control yok

**Olası Geliştirmeler:**
- Sıkıştırma modları (gzip, bzip2)
- İçerik şifresi
- Artımsal arşiv
- Dosya listesi göster
- Arşiv doğrulama (checksum)

## Lisans

Bu program eğitim amaçlıdır. Serbestçe kullanabilir ve değiştirebilirsiniz.

## Yazar

