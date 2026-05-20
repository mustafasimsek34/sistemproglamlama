/*
 * TARSAU - ARŞIVLEME PROGRAMI
 * =============================
 * 
 * NASIL ÇALIŞTIRILIR:
 * 
 * 1. DERLEME:
 *    $ cd b241210383
 *    $ make
 *    Sonuç: tarsau programı oluşturulacak
 * 
 * 2. ARŞIV OLUŞTURMA (-b Modu):
 *    $ ./tarsau -b dosya1.txt dosya2.txt dosya3.txt -o arsiv.sau
 *    Parametreler:
 *      -b: Arşiv oluşturma modu
 *      dosya1, dosya2, ... : Birleştirilecek metin dosyaları
 *      -o: Çıktı dosyası adı
 * 
 * 3. ARŞIV AÇMA (-a Modu):
 *    $ ./tarsau -a arsiv.sau cikti_dizini
 *    Parametreler:
 *      -a: Arşiv açma modu
 *      arsiv.sau: Açılacak arşiv dosyası (.sau formatı)
 *      cikti_dizini: Dosyaların çıkarılacağı dizin (oluşturulmazsa yeni oluşturulur)
 * 
 * HATA KONTROLLERI:
 *   - Binary dosya: "X uyumsuzdur!" mesajı verir
 *   - Hatalı arşiv: "Arşiv dosyası uygunsuz veya bozuk!" mesajı verir
 * 
 * ÖRNEKLER:
 *   $ echo "test1" > f1.txt
 *   $ echo "test2" > f2.txt
 *   $ ./tarsau -b f1.txt f2.txt -o archive.sau
 *   $ ./tarsau -a archive.sau output
 *   $ cat output/f1.txt
 */

/*
 * tarsau - Bir arşiv programı (sıkıştırma yapmayan, salt birleştirme)
 * Kullanım:
 *   - Birleştirme: tarsau -b file1 file2 file3 -o archive.sau
 *   - Açma:       tarsau -a archive.sau [çıkış_dizini]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024)  // 200 MB
#define ORG_SIZE_BYTES 10
#define SEPARATOR '|'
#define FIELD_SEP ','

/* Dosya bilgilerini taşıyan yapı */
typedef struct {
    char filename[256];
    unsigned long size;
    unsigned int permissions;
} FileEntry;

/* Arşiv meta bilgileri */
typedef struct {
    int file_count;
    unsigned long total_size;
    unsigned long org_section_size;
    FileEntry files[MAX_FILES];
} ArchiveMetadata;

/* ===== UTILITY FONKSİYONLAR ===== */

/*
 * Metin dosyası mı kontrolü
 * Basit kontrol: dosyayı kısmen oku, kontrol et
 */
int is_text_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    
    unsigned char buf[512];
    size_t read = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    // Null byte arıyoruz - binary dosyalarda var
    for (size_t i = 0; i < read; i++) {
        if (buf[i] == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Dosya boyutunu byte cinsinden döndür
 */
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return -1;
    }
    return st.st_size;
}

/*
 * Dosya izinlerini al (sadece rwx bitler)
 */
unsigned int get_file_permissions(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_mode & 0777;
}

/*
 * İzinleri ayarlama
 */
int set_file_permissions(const char *filename, unsigned int perms) {
    return chmod(filename, perms);
}

/* ===== BUILD (BIRLEŞTIRME) MOD ===== */

/*
 * Organizasyon bölümünü string'e dönüştür
 * Format: 10 bayt (boyut) + |dosya,izin,boyut| kayıtları
 */
char *create_org_section(ArchiveMetadata *meta, unsigned long *out_size) {
    // Tahmini boyut
    char *buffer = malloc(1024 * 100);  // 100 KB yeterli
    if (!buffer) {
        fprintf(stderr, "Bellek tahsisi başarısız\n");
        return NULL;
    }
    
    // Organizasyon bölümünü oluştur (ilk 10 bayt hariç)
    char *org_content = buffer + ORG_SIZE_BYTES;
    char *ptr = org_content;
    
    for (int i = 0; i < meta->file_count; i++) {
        // Formatta NULL terminator olmayacak şekilde yaz
        int len = sprintf(ptr, "%c%s,%o,%lu%c",
                      SEPARATOR,
                      meta->files[i].filename,
                      meta->files[i].permissions,
                      meta->files[i].size,
                      SEPARATOR);
        ptr += len;
        // sprintf NULL terminator ekledi, geri git
        if (*(ptr - 1) == '\0') {
            ptr--;
        }
    }
    
    unsigned long org_content_size = ptr - org_content;
    *out_size = org_content_size + ORG_SIZE_BYTES;
    
    // İlk 10 bayta boyutu yazma (snprintf ile, NULL terminator olmadan)
    char size_str[11];
    snprintf(size_str, sizeof(size_str), "%010lu", *out_size);
    memcpy(buffer, size_str, ORG_SIZE_BYTES);
    
    return buffer;
}

/*
 * Arşiv dosyasını oluştur
 */
int build_archive(const char *output_file, int file_count, const char **input_files) {
    ArchiveMetadata meta = {0};
    
    // Dosyaları kontrol et ve meta bilgi topla
    for (int i = 0; i < file_count; i++) {
        if (!is_text_file(input_files[i])) {
            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", input_files[i]);
            return -1;
        }
        
        long size = get_file_size(input_files[i]);
        if (size < 0) {
            perror("Dosya okunamadı");
            return -1;
        }
        
        meta.total_size += size;
        if (meta.total_size > MAX_TOTAL_SIZE) {
            fprintf(stderr, "Toplam dosya boyutu 200 MB'ı aştı!\n");
            return -1;
        }
        
        strcpy(meta.files[i].filename, input_files[i]);
        meta.files[i].size = size;
        meta.files[i].permissions = get_file_permissions(input_files[i]);
        meta.file_count++;
    }
    
    // Organizasyon bölümü oluştur
    unsigned long org_size = 0;
    char *org_section = create_org_section(&meta, &org_size);
    if (!org_section) {
        return -1;
    }
    
    // Arşiv dosyasını aç ve yaz
    FILE *archive = fopen(output_file, "wb");
    if (!archive) {
        perror("Arşiv dosyası oluşturulamadı");
        free(org_section);
        return -1;
    }
    
    // Organizasyon bölümünü yaz
    if (fwrite(org_section, 1, org_size, archive) != org_size) {
        fprintf(stderr, "Organizasyon bölümü yazılırken hata\n");
        fclose(archive);
        free(org_section);
        return -1;
    }
    
    // Dosya içeriklerini yaz
    for (int i = 0; i < file_count; i++) {
        FILE *input = fopen(input_files[i], "rb");
        if (!input) {
            perror("Giriş dosyası okunamadı");
            fclose(archive);
            free(org_section);
            return -1;
        }
        
        unsigned char buffer[4096];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
            if (fwrite(buffer, 1, bytes_read, archive) != bytes_read) {
                fprintf(stderr, "Arşive yazılırken hata\n");
                fclose(input);
                fclose(archive);
                free(org_section);
                return -1;
            }
        }
        
        fclose(input);
    }
    
    fclose(archive);
    free(org_section);
    return 0;
}

/* ===== EXTRACT (AÇMA) MOD ===== */

/*
 * Organizasyon bölümünü oku ve meta bilgi çıkar
 */
int parse_org_section(FILE *archive, ArchiveMetadata *meta) {
    // İlk 10 baytı oku (organizasyon boyutu)
    char org_size_str[11];
    memset(org_size_str, 0, 11);
    
    if (fread(org_size_str, 1, ORG_SIZE_BYTES, archive) != ORG_SIZE_BYTES) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return -1;
    }
    
    unsigned long org_size = strtoul(org_size_str, NULL, 10);
    if (org_size == 0 || org_size < ORG_SIZE_BYTES) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return -1;
    }
    
    // Organizasyon içeriğini oku
    char *org_content = malloc(org_size + 1);
    if (!org_content) {
        fprintf(stderr, "Bellek tahsisi başarısız\n");
        return -1;
    }
    
    size_t to_read = org_size - ORG_SIZE_BYTES;
    if (fread(org_content, 1, to_read, archive) != to_read) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        free(org_content);
        return -1;
    }
    org_content[to_read] = '\0';
    
    // Kayıtları parse et
    char *ptr = org_content;
    meta->file_count = 0;
    meta->org_section_size = org_size;
    
    while (*ptr && meta->file_count < MAX_FILES) {
        if (*ptr == SEPARATOR) {
            ptr++;
            
            // Format: dosya_adı,izin,boyut|
            char filename[256];
            unsigned int perms;
            unsigned long size;
            char dummy;
            
            int parsed = sscanf(ptr, "%255[^,],%o,%lu%c",
                              filename, &perms, &size, &dummy);
            
            if (parsed < 3 || dummy != SEPARATOR) {
                break;
            }
            
            strcpy(meta->files[meta->file_count].filename, filename);
            meta->files[meta->file_count].permissions = perms;
            meta->files[meta->file_count].size = size;
            meta->file_count++;
            
            // Sonraki kayda git
            while (*ptr && *ptr != SEPARATOR) {
                ptr++;
            }
            if (*ptr == SEPARATOR) {
                ptr++;
            }
        } else {
            ptr++;
        }
    }
    
    free(org_content);
    return 0;
}

/*
 * Dizin oluştur (recursively)
 */
int create_directory(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    
    return mkdir(tmp, S_IRWXU);
}

/*
 * Arşivi aç ve dosyaları çıkar
 */
int extract_archive(const char *archive_file, const char *extract_dir) {
    FILE *archive = fopen(archive_file, "rb");
    if (!archive) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return -1;
    }
    
    ArchiveMetadata meta = {0};
    
    // Organizasyon bölümünü parse et
    if (parse_org_section(archive, &meta) != 0) {
        fclose(archive);
        return -1;
    }
    
    // Hedef dizini oluştur (varsa sorun değil)
    if (extract_dir) {
        create_directory(extract_dir);
    }
    
    // Dosyaları çıkar
    for (int i = 0; i < meta.file_count; i++) {
        char full_path[512];
        
        if (extract_dir) {
            snprintf(full_path, sizeof(full_path), "%s/%s", 
                    extract_dir, meta.files[i].filename);
        } else {
            snprintf(full_path, sizeof(full_path), "%s", 
                    meta.files[i].filename);
        }
        
        FILE *output = fopen(full_path, "wb");
        if (!output) {
            fprintf(stderr, "Dosya açılamadı: %s\n", full_path);
            fclose(archive);
            return -1;
        }
        
        // Dosya içeriğini oku ve yaz
        unsigned char buffer[4096];
        unsigned long remaining = meta.files[i].size;
        
        while (remaining > 0) {
            size_t to_read = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
            size_t bytes_read = fread(buffer, 1, to_read, archive);
            
            if (bytes_read == 0) {
                fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
                fclose(output);
                fclose(archive);
                return -1;
            }
            
            if (fwrite(buffer, 1, bytes_read, output) != bytes_read) {
                fprintf(stderr, "Çıkış dosyasına yazılırken hata\n");
                fclose(output);
                fclose(archive);
                return -1;
            }
            
            remaining -= bytes_read;
        }
        
        fclose(output);
        
        // İzinleri restore et
        if (set_file_permissions(full_path, meta.files[i].permissions) != 0) {
            perror("İzin ayarlanırken hata");
        }
    }
    
    fclose(archive);
    
    // Başarılı açma mesajı
    if (extract_dir) {
        printf("%s dizininde ", extract_dir);
    }
    for (int i = 0; i < meta.file_count; i++) {
        if (i > 0 && i == meta.file_count - 1) {
            printf("ve ");
        } else if (i > 0) {
            printf(", ");
        }
        printf("%s", meta.files[i].filename);
    }
    printf(" dosyaları açıldı.\n");
    
    return 0;
}

/* ===== KOMUT SATIRI PARSER ===== */

void print_usage(const char *prog) {
    printf("Kullanım:\n");
    printf("  %s -b dosya1 dosya2 ... [-o çıkış.sau]\n", prog);
    printf("  %s -a arşiv.sau [hedef_dizin]\n", prog);
}

int parse_args(int argc, char *argv[], int *mode, char **output_file,
               char **archive_file, char **extract_dir,
               int *file_count, char ***input_files) {
    
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }
    
    if (strcmp(argv[1], "-b") == 0) {
        // BUILD modu
        *mode = 1;  // BUILD
        *output_file = "a.sau";  // Varsayılan
        *file_count = 0;
        *input_files = malloc(sizeof(char *) * MAX_FILES);
        
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    *output_file = argv[++i];
                }
            } else {
                if (*file_count >= MAX_FILES) {
                    fprintf(stderr, "Maksimum dosya sayısı (32) aşıldı!\n");
                    return -1;
                }
                (*input_files)[*file_count] = argv[i];
                (*file_count)++;
            }
        }
        
        if (*file_count == 0) {
            fprintf(stderr, "Hiç giriş dosyası belirtilmedi!\n");
            return -1;
        }
        
    } else if (strcmp(argv[1], "-a") == 0) {
        // EXTRACT modu
        *mode = 2;  // EXTRACT
        
        if (argc < 3) {
            fprintf(stderr, "Arşiv dosyası belirtilmedi!\n");
            return -1;
        }
        
        *archive_file = argv[2];
        *extract_dir = (argc > 3) ? argv[3] : NULL;
        
    } else {
        print_usage(argv[0]);
        return -1;
    }
    
    return 0;
}

/* ===== MAIN ===== */

int main(int argc, char *argv[]) {
    int mode = 0;
    char *output_file = NULL;
    char *archive_file = NULL;
    char *extract_dir = NULL;
    int file_count = 0;
    char **input_files = NULL;
    
    // Komut satırını parse et
    if (parse_args(argc, argv, &mode, &output_file, &archive_file,
                   &extract_dir, &file_count, &input_files) != 0) {
        return 1;
    }
    
    int result = 0;
    
    if (mode == 1) {
        // BUILD modu
        result = build_archive(output_file, file_count, (const char **)input_files);
        if (result == 0) {
            printf("Dosyalar birleştirildi.\n");
        }
        free(input_files);
        
    } else if (mode == 2) {
        // EXTRACT modu
        result = extract_archive(archive_file, extract_dir);
    }
    
    return result == 0 ? 0 : 1;
}
