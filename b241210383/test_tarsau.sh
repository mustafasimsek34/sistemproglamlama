#!/bin/bash
# TARSAU TEST SCRIPTI

echo "========================================"
echo "TARSAU PROGRAMI - TEST SCRIPTI"
echo "========================================"

[ -f tarsau.c ] || { echo "HATA: tarsau.c bulunamadı!"; exit 1; }

echo ""
echo "[1/7] Derleniyor..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ ! -f tarsau ]; then
    echo "HATA: Derleme başarısız!"
    exit 1
fi
echo "[OK] Derleme başarılı"

echo ""
echo "[2/7] Test dosyaları oluşturuluyor..."
echo "içerik t1" > t1
echo "içerik t2" > t2
echo "içerik t3" > t3
echo "içerik t4" > t4.txt
echo "içerik t5" > t5.dat
echo "[OK] 5 dosya oluşturuldu"

echo ""
echo "[3/7] Arşiv oluşturuluyor..."
./tarsau -b t1 t2 t3 t4.txt t5.dat -o s1.sau > /tmp/out.txt 2>&1
if grep -q "başarıyla oluşturuldu" /tmp/out.txt; then
    echo "[OK] Arşiv oluşturuldu: s1.sau"
else
    echo "HATA: Arşiv oluşturulamadı!"
    cat /tmp/out.txt
    exit 1
fi

echo ""
echo "[4/7] Arşiv açılıyor..."
rm -rf d1
./tarsau -a s1.sau d1 > /tmp/out.txt 2>&1
if grep -q "başarıyla açıldı" /tmp/out.txt; then
    echo "[OK] Arşiv açıldı: d1/"
else
    echo "HATA: Arşiv açılamadı!"
    cat /tmp/out.txt
    exit 1
fi

echo ""
echo "[5/7] Dosyalar kontrol ediliyor..."
if [ -f d1/t1 ] && [ -f d1/t2 ] && [ -f d1/t3 ] && [ -f d1/t4.txt ] && [ -f d1/t5.dat ]; then
    echo "[OK] Tüm dosyalar çıkarıldı"
else
    echo "HATA: Bazı dosyalar eksik!"
    ls d1/
    exit 1
fi

echo ""
echo "[6/7] Dosya içeriği kontrol ediliyor..."
CONTENT1=$(cat d1/t1)
if [ "$CONTENT1" = "içerik t1" ]; then
    echo "[OK] Dosya içeriği doğru"
else
    echo "HATA: Dosya içeriği yanlış!"
    exit 1
fi

echo ""
echo "[7/7] Binary dosya reddi testi..."
printf '\x00\x01\x02' > binary.bin
OUTPUT=$(./tarsau -b binary.bin 2>&1)
if echo "$OUTPUT" | grep -q "uyumsuzdur"; then
    echo "[OK] Binary dosya başarıyla reddedildi"
else
    echo "HATA: Binary dosya reddi başarısız!"
    exit 1
fi

echo ""
echo "========================================"
echo "SONUÇ: TÜM TESTLER BAŞARILI"
echo "========================================"
