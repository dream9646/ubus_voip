#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

struct rc4_state {
	unsigned char	perm[256];
	unsigned char	index1;
	unsigned char	index2;
};

static __inline void
swap_bytes(unsigned char *a, unsigned char *b)
{
	unsigned char temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

static void
rc4_init(struct rc4_state *const state, const unsigned char *key, int keylen)
{
	unsigned char j;
	int i;

	/* Initialize state with identity permutation */
	for (i = 0; i < 256; i++)
		state->perm[i] = (unsigned char)i; 
	state->index1 = 0;
	state->index2 = 0;
  
	/* Randomize the permutation using key data */
	for (j = i = 0; i < 256; i++) {
		j += state->perm[i] + key[i % keylen]; 
		swap_bytes(&state->perm[i], &state->perm[j]);
	}
}

/*
 * Encrypt some data using the supplied RC4 state buffer.
 * The input and output buffers may be the same buffer.
 * Since RC4 is a stream cypher, this function is used
 * for both encryption and decryption.
 */
static void
rc4_crypt(struct rc4_state *const state,
	const unsigned char *inbuf, unsigned char *outbuf, int buflen)
{
	int i;
	unsigned char j;

	for (i = 0; i < buflen; i++) {

		/* Update modification indicies */
		state->index1++;
		state->index2 += state->perm[state->index1];

		/* Modify permutation */
		swap_bytes(&state->perm[state->index1],
		    &state->perm[state->index2]);

		/* Encrypt/decrypt next byte */
		j = state->perm[state->index1] + state->perm[state->index2];
		outbuf[i] = inbuf[i] ^ state->perm[j];
	}
}

int
fencrypt_encrypt(char *src_file, char *dst_file, char *keystr)
{
	char buff[1024];
	FILE *fp1, *fp2;
	int rlen, wlen;
//	int offset;
	struct rc4_state state;

	rc4_init(&state, (unsigned char *)keystr, strlen(keystr));

	snprintf(buff, 1024, "gzip -c -9 %s", src_file);
	fp1=popen(buff, "r");
	if (fp1==NULL)
		return -1;
	
	fp2=fopen(dst_file, "w");
	if (fp2==NULL)
	{
		pclose(fp1);
		return -2;
	}
		
	while ((rlen=fread(buff, 1, 1024, fp1))>0) {
		if (rlen<0) {
			fprintf(stderr, "%s read error: %s\n", src_file, strerror(errno));
			return -3;
		}

		rc4_crypt(&state, (unsigned char*)buff, (unsigned char*)buff, rlen);

		wlen=fwrite(buff, 1, rlen, fp2);
		if (wlen<0 || wlen!=rlen) {
			fprintf(stderr, "%s write error: %s\n", dst_file, strerror(errno));
			return -4;
		}
	}
	
	pclose(fp1);
	fclose(fp2);
	return 0;
}

int
fencrypt_decrypt(char *src_file, char *dst_file, char *keystr)
{
	char buff[1024];
	FILE *fp1, *fp2;
	int rlen, wlen;
	struct rc4_state state;

	rc4_init(&state, (unsigned char *)keystr, strlen(keystr));

	fp1=fopen(src_file, "r");
	if (fp1==NULL)
		return -1;

	snprintf(buff, 1024, "gzip -d > %s", dst_file);
	fp2=popen(buff, "w");
	if (fp2==NULL)
	{
		fclose(fp1);
		return -2;
	}
		
	while ((rlen=fread(buff, 1, 1024, fp1))>0) {
		if (rlen<0) {
			fprintf(stderr, "%s read error: %s\n", src_file, strerror(errno));
			fclose(fp1);
			pclose(fp2);
			return -3;
		}

		rc4_crypt(&state, (unsigned char*)buff, (unsigned char*)buff, rlen);
		
		wlen=fwrite(buff, 1, rlen, fp2);
		if (wlen<0 || wlen!=rlen) {
			fprintf(stderr, "%s write error: %s\n", dst_file, strerror(errno));
			fclose(fp1);
			pclose(fp2);
			return -4;
		}
	}
	
	fclose(fp1);
	pclose(fp2);
	return 0;
}

#ifdef MAKE_FENCRYPT
int
main(int argc, char **argv)
{
	int is_decrypt=0, ret;
	char *prog_name, *src_file, *dst_file, *keystr;

	prog_name=argv[0];

	if (argc>=2) {
		if (strcmp(argv[1], "-d")==0) {
			is_decrypt=1;
			argc--;
			argv++;
		}
	}
			
	if (argc==4) {
		src_file=argv[1];
		dst_file=argv[2];
		keystr=argv[3];
		if (is_decrypt) {
			ret=fencrypt_decrypt(src_file, dst_file, keystr);
		} else {
			ret=fencrypt_encrypt(src_file, dst_file, keystr);		
		}
		exit(ret);
	}
	
	fprintf(stderr, "syntax: %s [-d] <src_file> <dst_file> <key>\n", prog_name);
	exit(1);
}

#endif
