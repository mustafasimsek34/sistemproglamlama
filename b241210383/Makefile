# Makefile for tarsau archive program
# Kullanım: make (derle) | make clean (temizle)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 

TARGET = tarsau
SRCS = tarsau.c
OBJS = $(SRCS:.c=.o)

# Varsayılan hedef
all: $(TARGET)

# Derleme kuralı
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "✓ $(TARGET) başarıyla derlenmiştir"

# Nesne dosyası derleme
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Temizleme
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "✓ Eski dosyalar temizlendi"

# Yeniden derleme
rebuild: clean all

# Yardım
help:
	@echo "Makefile hedefleri:"
	@echo "  make        - Programı derle"
	@echo "  make clean  - Eski dosyaları temizle"
	@echo "  make rebuild- Temiz derle"

.PHONY: all clean rebuild help
