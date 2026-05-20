# TARSAU - Metin Dosyaları Arşivleme Programı

**Sistem Programlama Projesi | 2025-2026 Bahar**

---

## 📋 İÇİNDEKİLER

1. [Proje Amacı](#proje-amacı)
2. [Nasıl Çalışır](#nasıl-çalışır)
3. [Kurulum ve Derleme](#kurulum-ve-derleme)
4. [Kullanım](#kullanım)
5. [Teknik Mimarisi](#teknik-mimarisi)
6. [Geliştirme Rehberi](#geliştirme-rehberi)
7. [Kodun Yapısı](#kodun-yapısı)
8. [Test ve Doğrulama](#test-ve-doğrulama)

---

## 🎯 PROJE AMACI

### Neden Tarsau?

`tarsau` projesi şu amaçlarla geliştirilmiştir:

1. **Sistem Programlama Öğrenme**
   - C dilinde dosya I/O işlemleri
   - POSIX sistem çağrıları (open, read, write, chmod, stat)
   - Dinamik bellek yönetimi
   - Hata yönetimi ve exception handling

2. **Arşiv Format Tasarımı**
   - İkili format (binary format) tasarımı
   - Veri yapı serileştirmesi
   - Metadata yönetimi

3. **Modüler Kod Yazımı**
   - Fonksiyonları ayrı tutma
   - Yapı (struct) ve veri soyutlama
   - Temiz kod ilkeleri

### Hedef Kullanıcı

- Sistem yöneticileri: Metin dosyalarını hızlı arşivlemek
- Yazılımcılar: Sistem programlama örneği olarak

### Kısıtlamalar

- **Sıkıştırma yok** - Sadece birleştirme
- **Metin dosyaları yalnızca** - ASCII formatı
- **Maksimum 32 dosya** - Arşiv başına
- **200 MB sınırı** - Toplam boyut

---

## ⚙️ NASIL ÇALIŞIR

### Genel İş Akışı

```
Kullanıcı Girdisi
      ↓
[Komut Satırı Parser] → parse_args()
      ↓
   [2 Mod]
   ↙     ↘
[-b]      [-a]
BUILD    EXTRACT
↓         ↓
[build_archive]  [extract_archive]
↓                ↓
.sau Dosya      Metin Dosyalar
```

### -b Modu (Arşiv Oluşturma)

**Adım 1: Dosyaları Doğrula**
```c
for (int i = 0; i < file_count; i++) {
    if (!is_text_file(input_files[i]))  // Binary mi?
        return error;
    
    long size = get_file_size(input_files[i]);
    total_size += size;  // Sınır kontrolü
}
```

**Adım 2: Organizasyon Bölümü Oluştur**
```
Format: |file1,755,100|file2,644,50|
        ↑ pipe ile ayrılan kayıtlar
        dosya_adı,izinler(sekizli),boyut
```

**Adım 3: Arşiv Dosyasına Yaz**
```
[İlk 10 bayt: Org. boyutu]
[Organizasyon bölümü]
[File1 içeriği]
[File2 içeriği]
...
```

### -a Modu (Arşiv Açma)

**Adım 1: Organizasyon Oku**
```c
char org_size_str[11];
fread(org_size_str, 1, 10, archive);  // İlk 10 bayt
unsigned long org_size = strtoul(org_size_str, NULL, 10);
```

**Adım 2: Parse Et**
```c
while (*ptr == '|') {
    // |file1,755,100| → file1, 755, 100 çıkar
    sscanf(ptr, "%255[^,],%o,%lu", filename, &perms, &size);
}
```

**Adım 3: Dosyaları Çıkar**
```c
for (int i = 0; i < meta.file_count; i++) {
    FILE *out = fopen(full_path, "wb");
    
    // size kadar bayt oku ve yaz
    while (remaining > 0) {
        fread(buffer, ...);  // Arşivten oku
        fwrite(buffer, ...); // Dosyaya yaz
    }
    
    chmod(full_path, meta.files[i].permissions);  // İzin restore
}
```

---

## 📦 KURULUM VE DERLEME

### Gereksinimler

- **Linux/Unix** (WSL de çalışır)
- **GCC** (4.8+)
- **Make**
- **C99 standartı**

### Derleme

```bash
cd /mnt/c/Users/Dell/Desktop/b241210383

# Derle
make

# Temiz derle
make clean && make

# Yardım
make help
```

**Derleyici Bayrakları:**
```
-Wall -Wextra    : Tüm uyarılar
-std=c99         : C99 standardı (for döngüsü içinde int tanımlama)
-O2              : Optimizasyon
-finput-charset=UTF-8 : Türkçe karakterler için
```

---

## 🔧 KULLANIM

### Arşiv Oluşturma (-b)

```bash
./tarsau -b file1.txt file2.txt file3.txt -o archive.sau
```

**Çıktı:**
```
Dosyalar birleştirildi.
```

**Parametreler:**
- `-b` : Build modu
- `file1.txt file2.txt ...` : Giriş dosyaları
- `-o archive.sau` : Çıktı dosyası (opsiyonel, varsayılan: `a.sau`)

**Hata Örnekleri:**
```bash
./tarsau -b binary.exe file.txt
# Hata: binary.exe giriş dosyasının formatı uyumsuzdur!

./tarsau -b *.txt  # 33+ dosya
# Hata: Maksimum dosya sayısı (32) aşıldı!

./tarsau -b *.iso
# Hata: Toplam dosya boyutu 200 MB'ı aştı!
```

### Arşiv Açma (-a)

```bash
./tarsau -a archive.sau output_dir
```

**Çıktı:**
```
output_dir dizininde file1.txt, file2.txt ve file3.txt dosyaları açıldı.
```

**Parametreler:**
- `-a` : Archive modu
- `archive.sau` : Açılacak arşiv
- `output_dir` : Hedef dizin (opsiyonel, varsayılan: mevcut dizin)

**Hata Örneği:**
```bash
./tarsau -a fake.sau out
# Hata: Arşiv dosyası uygunsuz veya bozuk!
```

---

## 🏗️ TEKNIK MİMARİSİ

### Veri Yapıları

```c
// Her dosya için bilgi
typedef struct {
    char filename[256];           // Dosya adı
    unsigned long size;           // Boyut (bayt)
    unsigned int permissions;     // İzinler (octal)
} FileEntry;

// Arşiv meta bilgisi
typedef struct {
    int file_count;              // Dosya sayısı
    unsigned long total_size;    // Toplam boyut
    unsigned long org_section_size;  // Organizasyon boyutu
    FileEntry files[MAX_FILES];  // Dosya listesi
} ArchiveMetadata;
```

### .sau Format Detayları

**Header (İlk 10 bayt):**
```
0000000087  ← Organizasyon + bu başlık = 87 bayt (ASCII, 0-padded)
```

**Organizasyon Bölümü:**
```
|file1.txt,644,100|file2.txt,755,50|
|dosya1,izin,boyut|dosya2,izin,boyut|
```

**Dosya Verileri:**
```
[100 bayt file1.txt][50 bayt file2.txt]
Hiçbir ayırıcı yok, sadık sırasında yazılır
```

### Fonksiyon Haritası

```
main()
├─ parse_args()                    // Komut satırı
│
├─ BUILD (-b) → build_archive()
│  ├─ is_text_file()              // Binary kontrolü
│  ├─ get_file_size()             // Boyut al
│  ├─ get_file_permissions()      // İzin al
│  ├─ create_org_section()        // Organizasyon
│  └─ fwrite()                    // Dosyaya yaz
│
└─ EXTRACT (-a) → extract_archive()
   ├─ parse_org_section()         // Organizasyon parse
   ├─ create_directory()          // Dizin oluştur
   ├─ fread() / fwrite()          // Dosya I/O
   └─ set_file_permissions()      // İzin restore
```

---





### Sık Yapılan Hatalar

| Hata | Çözüm |
|------|-------|
| `fread/fwrite` boyut uyumsuzluğu | `size_t` dönüş değerini kontrol et |
| Memory leak | `malloc()` sonra `free()` çağrısını koy |
| Format hatası | Arşiv açamıyorsa, organize bölümü kontrol et |
| Permission denied | Root gerek, `chmod` kontrol et |

---



### Kod Bölümleri

**1. Include'lar:**
```c
#include <stdio.h>      // I/O
#include <stdlib.h>     // malloc/free
#include <sys/stat.h>   // stat
#include <unistd.h>     // chmod
```

**2. Constant'lar:**
```c
#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024)
#define ORG_SIZE_BYTES 10
#define SEPARATOR '|'
#define FIELD_SEP ','
```

**3. Fonksiyonlar:**
- **Utility:** is_text_file(), get_file_size(), ...
- **Build:** create_org_section(), build_archive()
- **Extract:** parse_org_section(), extract_archive()
- **System:** create_directory(), set_file_permissions()

---

## ✅ TEST VE DOĞRULAMA





**Test Senaryoları:**
1. ✅ Arşiv oluşturma
2. ✅ Arşiv açma
3. ✅ İçerik doğruluğu
4. ✅ Binary dosya reddi
5. ✅ Hatalı arşiv reddi
6. ✅ İzin koruması

---

## 📈 GELİŞTİRME FİKİRLERİ

### Eklenebilecek Özellikler

**Kısa Vadeli:**
- [ ] Dosya listesi görüntüleme (`-l` flag)
- [ ] Arşiv doğrulama (`-v` flag)
- [ ] Seçmeli arşiv açma

**Orta Vadeli:**
- [ ] gzip sıkıştırması
- [ ] Arşiv şifresi
- [ ] Artımsal arşiv

**Uzun Vadeli:**
- [ ] Dizin desteği
- [ ] Symbolic link
- [ ] Versiyonlama
- [ ] Checksum/integrty




