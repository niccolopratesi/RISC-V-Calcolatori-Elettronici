#include "internal.h"
#define DEC_BUFSIZE 12

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
        size_t in = 0, out = 0, tmp;
        char *aux, buf[DEC_BUFSIZE];
	void *addr;
        int cifre;
	bool islong;
	size_t cur_size;

        while(out < size - 1 && fmt[in]) {
                switch(fmt[in]) {
		case '%':
			islong = false;
			cifre = -1;
			cur_size = size - 1;
			// numero di cifre
			if (fmt[in + 1] >= '1' && fmt[in + 1] <= '9') {
				in++;
				cifre = fmt[in] - '0';
			}
			// limite per %s
			if (fmt[in + 1] == '.' && fmt[in + 2] == '*') {
				cur_size = out + va_arg(ap, size_t);
				if (cur_size < out || cur_size > size - 1)
					cur_size = size - 1;
				in += 2;
			}
			// long?
			if (fmt[in + 1] == 'l') {
				islong = true;
				if (cifre < 0)
					cifre = 16;
				in++;
			}
			switch(fmt[++in]) {
			case '\0':
				goto end;
			case 'p':
				if (islong)
					goto end;
				addr = va_arg(ap, void*);
				cifre = sizeof(void*) * 2;
				if(out > size - (cifre + 1))
					goto end;
				htostr(&str[out], (unsigned long long)addr, cifre);
				out += cifre;
				break;
			case 'd':
				tmp = (islong ? va_arg(ap, long) : va_arg(ap, int));
				itostr(buf, DEC_BUFSIZE, tmp);
				if(strlen(buf) > size - out - 1)
					goto end;
				for(aux = buf; *aux; ++aux)
					str[out++] = *aux;
				break;
			case 'x':
				if (cifre < 0)
					cifre = 8;
				tmp = (islong ? va_arg(ap, long) : va_arg(ap, int));
				if(out > size - (cifre + 1))
					goto end;
				htostr(&str[out], tmp, cifre);
				out += cifre;
				break;
			case 's':
				if (islong)
					goto end;
				aux = va_arg(ap, char *);
				while(out < cur_size && *aux)
					str[out++] = *aux++;
				break;
			case 'c':
				if (islong)
					goto end;
				tmp = va_arg(ap, int);
				if (out < size - 1)
					str[out++] = tmp;

			}
			++in;
			break;
		default:
			str[out++] = fmt[in++];
                }
        }
end:
        str[out++] = 0;

        return out;
}
