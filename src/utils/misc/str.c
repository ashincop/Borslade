#include <stddef.h>
#include <stdint.h>
#include <utils/misc/str.h>

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

size_t strlen(const char *s)
{
	size_t len = 0;
	while (s[len])
		len++;
	return len;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		unsigned char c1 = (unsigned char)s1[i];
		unsigned char c2 = (unsigned char)s2[i];

		if (c1 != c2)
			return c1 - c2;
		if (c1 == '\0')
			return 0;
	}
	return 0;
}

char *strcpy(char *dest, const char *src)
{
	char *d = dest;
	while (*src) {
		*d++ = *src++;
	}
	*d = '\0';
	return dest;
}
