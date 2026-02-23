#include <launchd/launchd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <utils/misc/str.h>

typedef struct {
	uint64_t bitlen;
	uint32_t state[8];
	uint8_t data[64];
	uint32_t datalen;
} sha256_ctx;

static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

void sha256_transform(sha256_ctx *ctx, const uint8_t data[])
{
	uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; i++, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) |
		       (data[j + 2] << 8) | (data[j + 3]);
	for (; i < 64; i++)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; i++) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(sha256_ctx *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update(sha256_ctx *ctx, const uint8_t data[], size_t len)
{
	for (size_t i = 0; i < len; i++) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_final(sha256_ctx *ctx, uint8_t hash[32])
{
	uint32_t i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	} else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		for (i = 0; i < 56; i++)
			ctx->data[i] = 0;
	}

	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = (uint8_t)(ctx->bitlen);
	ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
	ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
	ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
	ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
	ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
	ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
	ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 4; i++) {
		hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0xff;
		hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0xff;
		hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0xff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0xff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0xff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0xff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0xff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0xff;
	}
}
#include <stddef.h>
#include <stdint.h>

// --- SHA-256 one-shot ---
void sha256(const uint8_t *data, size_t len, uint8_t hash[32])
{
	// Internal context
	typedef struct {
		uint64_t bitlen;
		uint32_t state[8];
		uint8_t data_buf[64];
		uint32_t datalen;
	} sha256_ctx;

	sha256_ctx ctx;
	ctx.datalen = 0;
	ctx.bitlen = 0;
	ctx.state[0] = 0x6a09e667;
	ctx.state[1] = 0xbb67ae85;
	ctx.state[2] = 0x3c6ef372;
	ctx.state[3] = 0xa54ff53a;
	ctx.state[4] = 0x510e527f;
	ctx.state[5] = 0x9b05688c;
	ctx.state[6] = 0x1f83d9ab;
	ctx.state[7] = 0x5be0cd19;

	static const uint32_t k[64] = {
	    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
	    0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
	    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
	    0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
	    0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
	    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
	    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
	    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
	    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

	void sha256_transform(sha256_ctx * c, const uint8_t d[64])
	{
		uint32_t a, b, c1, d1, e, f, g, h, t1, t2, m[64];
		int i, j;
		for (i = 0, j = 0; i < 16; i++, j += 4)
			m[i] = (d[j] << 24) | (d[j + 1] << 16) |
			       (d[j + 2] << 8) | d[j + 3];
		for (; i < 64; i++)
			m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) +
			       m[i - 16];
		a = c->state[0];
		b = c->state[1];
		c1 = c->state[2];
		d1 = c->state[3];
		e = c->state[4];
		f = c->state[5];
		g = c->state[6];
		h = c->state[7];
		for (i = 0; i < 64; i++) {
			t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
			t2 = EP0(a) + MAJ(a, b, c1);
			h = g;
			g = f;
			f = e;
			e = d1 + t1;
			d1 = c1;
			c1 = b;
			b = a;
			a = t1 + t2;
		}
		c->state[0] += a;
		c->state[1] += b;
		c->state[2] += c1;
		c->state[3] += d1;
		c->state[4] += e;
		c->state[5] += f;
		c->state[6] += g;
		c->state[7] += h;
	}

	// --- feed all input in one go ---
	size_t i;
	uint8_t buf[64];
	size_t full_blocks = len / 64;
	for (i = 0; i < full_blocks; i++)
		sha256_transform(&ctx, data + i * 64);

	// remaining bytes
	size_t rem = len % 64;
	for (i = 0; i < rem; i++)
		ctx.data_buf[i] = data[full_blocks * 64 + i];
	ctx.datalen = rem;

	// padding
	uint32_t idx = ctx.datalen;
	ctx.data_buf[idx++] = 0x80;
	while (idx < 56)
		ctx.data_buf[idx++] = 0x00;

	uint64_t bitlen = len * 8;
	ctx.data_buf[63] = (uint8_t)bitlen;
	ctx.data_buf[62] = (uint8_t)(bitlen >> 8);
	ctx.data_buf[61] = (uint8_t)(bitlen >> 16);
	ctx.data_buf[60] = (uint8_t)(bitlen >> 24);
	ctx.data_buf[59] = (uint8_t)(bitlen >> 32);
	ctx.data_buf[58] = (uint8_t)(bitlen >> 40);
	ctx.data_buf[57] = (uint8_t)(bitlen >> 48);
	ctx.data_buf[56] = (uint8_t)(bitlen >> 56);

	sha256_transform(&ctx, ctx.data_buf);

	for (i = 0; i < 4; i++) {
		hash[i] = (ctx.state[0] >> (24 - i * 8)) & 0xff;
		hash[i + 4] = (ctx.state[1] >> (24 - i * 8)) & 0xff;
		hash[i + 8] = (ctx.state[2] >> (24 - i * 8)) & 0xff;
		hash[i + 12] = (ctx.state[3] >> (24 - i * 8)) & 0xff;
		hash[i + 16] = (ctx.state[4] >> (24 - i * 8)) & 0xff;
		hash[i + 20] = (ctx.state[5] >> (24 - i * 8)) & 0xff;
		hash[i + 24] = (ctx.state[6] >> (24 - i * 8)) & 0xff;
		hash[i + 28] = (ctx.state[7] >> (24 - i * 8)) & 0xff;
	}
}

char *str_to_print = "";
uint64_t rbx_val;
#define O_ERROR 0
#define O_INFO 1
#define O_OKAY 2
#define O_FATAL 3
#define O_BUG 4
#define O_ALL 5
#define host "com.strawberry.tlc.launchd"
void print(char *str)
{
	str_to_print = str;
	asm volatile("int $0x30" : : "a"(0), "b"(str_to_print));
}

char *itoa(int64_t value, char *str, int base)
{
	char *ptr = str;
	char *ptr1 = str;
	char tmp_char;
	int64_t tmp_value;

	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefgh"
			 "ijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	if (tmp_value < 0 && base == 10) {
		*ptr++ = '-';
	}

	*ptr-- = '\0';

	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

char buf[32];
int split_whitespace(char *str, char **out, int max)
{
	int count = 0;

	while (*str && count < max) {
		// skip spaces
		while (*str == ' ')
			str++;

		if (*str == 0)
			break;

		// word start
		out[count++] = str;

		// go till end of word
		while (*str && *str != ' ')
			str++;

		// null-terminate word
		if (*str) {
			*str = 0;
			str++;
		}
	}

	return count;
}
void *malloc(size_t size)
{
	uint64_t result;
	asm volatile("int $0x30" : "=a"(result) : "a"(7), "b"(size) :);
	return (void *)(uintptr_t)result;
}
void free(void *ptr)
{
	asm volatile("int $0x30" : : "a"(24), "b"((uint64_t)ptr) : "memory");
}
void *read(char *name)
{
	uint64_t size;
	asm volatile("int $0x30" : "=a"(size) : "a"((uint64_t)12), "b"(name) :);

	void *result = malloc(size);
	asm volatile("int $0x30"
		     :
		     : "a"((uint64_t)6), "b"(name), "c"(result), "d"(size + 1)
		     :);
	return result;
}

void create_task(uint64_t addr, int pid, char *name, int debug)
{
	register uint64_t r8 asm("r8") = debug;

	asm volatile("int $0x30"
		     :
		     : "a"(3), "b"(addr), "c"((uint64_t)pid), "d"(name),
		       "r"(r8) // r8 holds debug
		     : "memory");
}
void scan(char *buf, size_t size)
{
	asm volatile("int $0x30" : : "a"(2), "b"(buf), "c"(size) : "memory");
}
void print_n(uint64_t n, int base)
{
	char bufferi[256];
	itoa((int64_t)n, bufferi, base);
	print(bufferi);
}
void vprintf(const char *fmt, va_list args)
{
	for (const char *p = fmt; *p != '\0'; p++) {
		if (*p != '%') {
			char s[2] = {*p, 0};
			print(s);
			continue;
		}

		p++; // skip '%'
		switch (*p) {
		case 'p': {
			void *val = va_arg(args, void *);
			print("0x");
			print_n((uint64_t)val, 16);
			break;
		}

		case 's': {
			char *val = va_arg(args, char *);
			print(val);
			break;
		}

		case 'd': {
			int val = va_arg(args, int);
			print_n((uint64_t)val, 10);
			break;
		}

		case 'x': {
			uint64_t val = va_arg(args, uint64_t);
			print("0x");
			print_n(val, 16);
			break;
		}

		case 'b': {
			uint64_t val = va_arg(args, uint64_t);
			print("0b");
			print_n(val, 2);
			break;
		}

		case 'c': {
			char ch = (char)va_arg(args, int); // promoted to int
			char s[2] = {ch, 0};
			print(s);
			break;
		}

		default: {
			print("?");
			break;
		}
		}
	}
}
void printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);

	va_end(args);
}
void clear_screen_usrland(uint32_t color)
{
	asm volatile("int $0x30" : : "a"(1), "b"(color));
}
void ulog(const char *hosted, int type, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *string = "";
	switch (type) {
	case O_OKAY:
		string = " OK ";
		break;
	case O_BUG:
		string = " EDGE CASE ";
		break;
	case O_ERROR:
		string = " NERR ";
		break;
	case O_FATAL:
		string = " FERR ";
		break;
	case O_INFO:
		string = " INFO ";
		break;
	case O_ALL:
		string = " ALL ";
		break;
	default:
		string = " UNDOC ";
		break;
	}
	print("[");
	print(string);
	print("]");
	print(" (");
	print(hosted);
	print(")");
	vprintf(fmt, args);

	va_end(args);
}
int isethernetactiveusr()
{
	uint64_t result;
	asm volatile("int $0x30" : "=a"(result) : "a"(26) :);
	return (int)result;
}
typedef struct {
	uint32_t e_magic;
	uint8_t e_bitness;
	uint8_t e_endianess;
	uint8_t e_hversion;
	uint8_t e_abi;
	uint64_t e_padding;
	uint16_t e_type;
	uint16_t e_instset;
	uint32_t e_version;
	uint64_t e_entryoff;
	uint64_t e_phoff;
	uint64_t e_soff;
	uint32_t e_flags;
	uint16_t e_hsize;
	uint16_t e_sphdr;
	uint16_t e_nphdr;
	uint16_t e_ssht;
	uint16_t e_nsht;
	uint16_t e_ishst;
} ELF64_EHDR;
typedef struct {
	uint32_t p_seg;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
} ELF64_PHDR;
/* Simple memory copy/set for freestanding environments */
void *mini_memcpy(void *dest, const void *src, uint64_t n)
{
	uint8_t *d = (uint8_t *)dest;
	const uint8_t *s = (const uint8_t *)src;
	while (n--)
		*d++ = *s++;
	return dest;
}

void *mini_memset(void *s, int c, uint64_t n)
{
	uint8_t *p = (uint8_t *)s;
	while (n--)
		*p++ = (uint8_t)c;
	return s;
}

void *full_elf_loader(void *raw_elf)
{
	ELF64_EHDR *ehdr = (ELF64_EHDR *)raw_elf;
	ELF64_PHDR *phdr = (ELF64_PHDR *)((uint8_t *)raw_elf + ehdr->e_phoff);

	// 1. Calculate the total memory footprint needed
	uint64_t min_vaddr = (uint64_t)-1;
	uint64_t max_vaddr = 0;
	int found_load = 0;

	for (uint16_t i = 0; i < ehdr->e_nphdr; i++) {
		if (phdr[i].p_seg == 1) { // PT_LOAD
			if (phdr[i].p_vaddr < min_vaddr)
				min_vaddr = phdr[i].p_vaddr;
			if (phdr[i].p_vaddr + phdr[i].p_memsz > max_vaddr) {
				max_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
			}
			found_load = 1;
		}
	}

	if (!found_load)
		return NULL;

	// 2. Allocate the buffer
	// Note: If you have strict alignment requirements, use posix_memalign
	uint64_t total_size = max_vaddr - min_vaddr;
	void *allocated_mem = malloc(total_size);
	if (!allocated_mem)
		return NULL;

	// delta is the shift between the ELF's preferred vaddr and our actual
	// malloc address
	uintptr_t delta = (uintptr_t)allocated_mem - min_vaddr;

	// 3. Map PT_LOAD segments into the new buffer
	for (uint16_t i = 0; i < ehdr->e_nphdr; i++) {
		if (phdr[i].p_seg == 1) { // PT_LOAD
			void *dest = (void *)(delta + phdr[i].p_vaddr);
			void *src =
			    (void *)((uint8_t *)raw_elf + phdr[i].p_offset);

			mini_memcpy(dest, src, phdr[i].p_filesz);

			// Handle BSS
			if (phdr[i].p_memsz > phdr[i].p_filesz) {
				mini_memset((uint8_t *)dest + phdr[i].p_filesz,
					    0,
					    phdr[i].p_memsz - phdr[i].p_filesz);
			}
		}
	}

	// 4. Process Relocations (Find PT_DYNAMIC)
	uint64_t *dyn_ptr = 0;
	for (uint16_t i = 0; i < ehdr->e_nphdr; i++) {
		if (phdr[i].p_seg == 2) { // PT_DYNAMIC
			dyn_ptr = (uint64_t *)(delta + phdr[i].p_vaddr);
			break;
		}
	}

	if (dyn_ptr) {
		uint64_t rela_addr = 0, rela_size = 0, rela_ent = 24;
		for (int i = 0; dyn_ptr[i] != 0; i += 2) {
			if (dyn_ptr[i] == 7)
				rela_addr = dyn_ptr[i + 1];
			if (dyn_ptr[i] == 8)
				rela_size = dyn_ptr[i + 1];
			if (dyn_ptr[i] == 9)
				rela_ent = dyn_ptr[i + 1];
		}

		if (rela_addr && rela_size) {
			for (uint64_t off = 0; off < rela_size;
			     off += rela_ent) {
				struct {
					uint64_t off;
					uint64_t info;
					int64_t add;
				} *rel;
				rel = (void *)(delta + rela_addr + off);

				if ((rel->info & 0xFFFFFFFF) ==
				    8) { // R_X86_64_RELATIVE
					uint64_t *patch =
					    (uint64_t *)(delta + rel->off);
					*patch = delta + rel->add;
				}
			}
		}
	}

	return (void *)(delta + ehdr->e_entryoff);
}
extern struct Process;
void launchd()
{
	clear_screen_usrland(0x000000);
	print("[ OK ] (com.strawberry.tlc.launchd] launchd has started!");
	char buffy[32];
	scan(buffy, 32);
	struct Process *procid;
	asm volatile("int $0x30" : "=a"(procid) : "a"(27), "b"(1) :);
	long my_value = (long)procid;
	asm volatile("movq %0, %%r12" : : "r"(my_value));
	asm volatile("int $0x30" : : "a"(28));
	print(read("/apoc.apoc"));
}