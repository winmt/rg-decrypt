/*
	A universal firmware decryption tool for Ruijie Networks.
	Author : winmt (https://github.com/winmt)
	Date : 2023/2/26
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>

typedef unsigned char uint8_t;
#define BYTE(x, n) (*((uint8_t *)&(x)+n))

void error_msg(char *msg)
{
	puts(msg);
	exit(-1);
}

int num1 = 1, num2 = 0x10001;

void decrypt(uint8_t *enc_buf, uint8_t *dec_buf, int length)
{
	for (int i = 0; i < length; i++)
	{
		int sum = (uint8_t)num1 + (uint8_t)num2 + BYTE(num2, 1) + BYTE(num2, 2);
		BYTE(num2, sizeof(num2)/sizeof(uint8_t)-1) = sum % 2;
		
		for (int j = 0; j < 6; j++)
			*((uint8_t *)&num1 + j) = *((uint8_t *)&num1 + j + 1);
		
		uint8_t key = 0;
		for (int k = 0; k < 8; k++)
			key |= *((uint8_t *)&num1 + k) << k;
		*(uint8_t *)(dec_buf + i) = *(uint8_t *)(enc_buf + i) ^ key;
	}
}

int main(int argc, char **argv, const char **envp)
{
	if (argc < 2) error_msg("Usage: ./rg-decrypt [encrypted_firmware_path]");
	
	char *enc_path = strdup(argv[1]);
	char *dec_path = malloc(strlen(argv[1]) + 0x10);
	strcpy(dec_path, argv[1]);
	strcat(dec_path, ".decrypted");
	
	struct stat stat_buf;
	int stat_fd = stat(enc_path, &stat_buf);
	if (stat_fd < 0) error_msg("The encrypted firmware does not exist !");
	int size = stat_buf.st_size;
	
	uint8_t *enc_buf = (uint8_t *)malloc(0x1000);
	uint8_t *dec_buf = (uint8_t *)malloc(0x1000);
	
	int enc_fd = open(enc_path, O_RDONLY);
	if (enc_fd < 0) error_msg("Error to open the encrypted firmware !");
	
	int dec_fd = open(dec_path, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE | S_IRGRP);
	if (dec_fd < 0) error_msg("Error to create the decrypted firmware !");
	
	if (read(enc_fd, enc_buf, 22) != 22) error_msg("Error to read from the encrypted firmware !");
	size -= 22;
	
	while(size > 0)
	{
		int len = size;
		if (size > 0x1000) len = 0x1000;
		
		memset(enc_buf, 0, sizeof(enc_buf));
		memset(dec_buf, 0, sizeof(dec_buf));
		
		if (read(enc_fd, enc_buf, len) != len) error_msg("Error to read from the encrypted firmware !");
		decrypt(enc_buf, dec_buf, len);
		if (write(dec_fd, dec_buf, len) != len) error_msg("Error to write into the decrypted firmware !");
		size -= len;
	}
	
	free(enc_buf);
	free(dec_buf);
	close(enc_fd);
	close(dec_fd);
	return 0;
}
