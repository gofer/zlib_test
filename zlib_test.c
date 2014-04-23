#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <unistd.h>

/******************************************************************************
 * Zlib example code
 * 
 * Author:      gofer (@gofer_ex)
 * Last update: 22/April/2014
 * Description: deflate decompression
 * Licence:     Modified BSD Licence (New BSD Licence)
 ******************************************************************************/

/* maximum of input buffer size */
#define MAXIMUM_BUFFER_SIZE            4096

/* compress / decompress mode */
#define PROGRAM_MODE_COMPRESS          0
#define PROGRAM_MODE_DECOMPRESS        1

/* iff use debug mode */
/* #define __DEBUG__ 1 */

#ifdef __DEBUG__
void
debug_info() {
	fprintf(stdout, "ZLIB_VERSION:        %s\n", ZLIB_VERSION);
	fprintf(stdout, "MAXIMUM_BUFFER_SIZE: %d\n", MAXIMUM_BUFFER_SIZE);
	return;
}
#endif /* __DEBUG__ */

/******************************************************************************
 * get_string function
 * 
 * argument value: [char*]  buffer;   string recive buffer pointer
 *                 [size_t] buf_size; buffer size
 * retrun value:   [int]            ; revice string size
 * description:    Gets string in stdin. (This uses '\n' as delimiter.)
 ******************************************************************************/
int
get_string(buffer, buf_size)
	char   *buffer;
	size_t  buf_size;
{
	int i = 0;
	for(i = 0; i < buf_size-1; ++i)
	{
		buffer[i] = fgetc(stdin);
		if(buffer[i] == '\n') break;
	}
	buffer[i] = '\0';
	fflush(stdin);
	return i;
} /* get_string() */

/******************************************************************************
 * _decompress function
 * 
 * argument value: [FILE*] ifp; source file pointer
 *                 [FILE*] ofp; destination file pointer
 * retrun value:   [int]      ; exit code
 * description:    Decompress with zlib inflate() function.
 ******************************************************************************/
int
_decompress(ifp, ofp)
	FILE     *ifp;                 /* input file pointer */
	FILE     *ofp;                 /* output file pointer */
{
	int       ret       = 0;       /* zlib retrun status value */
	int       exit_code = 0;       /* exit code on this function */
	int       loop_flag = 0;       /* loop flag */
	int       rbuf_size = 0;       /* read buffer size */
	int       wbuf_size = 0;       /* write buffer size */
	char     *read_buf  = NULL;    /* read buffer */
	char     *write_buf = NULL;    /* write buffer */
	z_stream  z;                   /* zlib stream */
	int       zflush    = Z_NO_FLUSH;  /* zlib deflate flush */
	
	/* zlib stream initialize */
	z.zalloc = Z_NULL;     /* allocator is default */
	z.zfree  = Z_NULL;     /* free is default */
	z.opaque = Z_NULL;     /* opaque is default */
	ret = inflateInit(&z);
	if(ret != Z_OK)
	{
		fprintf(stderr, "[Error] inflateInit() is not Z_OK: %d\n", ret);
		exit_code = -1;
		goto END_OF_LOOP;
	}
	
	/* read buffer initialize */
	rbuf_size = MAXIMUM_BUFFER_SIZE * sizeof(char);
	read_buf  = (char*)malloc(rbuf_size);
	
	/* write buffer initialize */
	wbuf_size = MAXIMUM_BUFFER_SIZE * sizeof(char);
	write_buf = (char*)malloc(wbuf_size);
	
	/* loop initialize */
	loop_flag   = 1;               /* infinite loop */
	z.next_in   = read_buf;        /* read buffer pointer */
	z.avail_in  = 0;               /* read buffer size */
	z.next_out  = write_buf;       /* write buffer pointer */
	z.avail_out = wbuf_size;       /* write buffer size */
	
	/* main loop */
	while(loop_flag)
	{
		/* buffer set */
		if(z.avail_in == 0)
		{
			/* source file read */
			ret = fread(read_buf, sizeof(char), rbuf_size, ifp);
			if(ret < rbuf_size)
			{
				zflush = Z_NO_FLUSH;
			}
			
			/* zlib input stream set */
			z.next_in   = read_buf;        /* read buffer pointer */
			z.avail_in  = ret;             /* read buffer size */
		} /* if(z.avail_in == 0) */
		
		/* zlib inflate() */
		ret = inflate(&z, zflush);
		if(ret == Z_STREAM_END)
		{
			loop_flag = 0;             /* exit this loop */
		}
		if( !(                         /* if inflate is failed */
			(ret == Z_OK)              /* successfully continue loop */
			||
			(ret == Z_STREAM_END)      /* successfully end of loop */
		) )
		{
			fprintf(stderr, "[Error] inflate() is not Z_OK: %d (%s)\n", ret, z.msg);
			exit_code = -2;
			goto END_OF_LOOP;
		}
		
		/* buffer output in destination file */
		/*fprintf(stdout, "%s", write_buf);*/
		if(z.avail_out == 0)
		{
			/* destination file write */
			ret = fwrite(write_buf, sizeof(char), wbuf_size, ofp);
			if(ret != wbuf_size)
			{
				fprintf(stderr, "[Error] An error has occured at fwrite().\n");
			}
			
			/* zlib output stream set */
			z.next_out  = write_buf;       /* write buffer pointer */
			z.avail_out = wbuf_size;       /* write buffer size */
		} /* if(z.avail_out == 0) */
	} /* while(loop_flag) */
	
	/* write remain buffer in destination file */
	ret = fwrite(write_buf, sizeof(char), wbuf_size-z.avail_out, ofp);
	if(ret != wbuf_size-z.avail_out)
	{
		fprintf(stderr, "[Error] An error has occured at fwrite().\n");
	}
	
END_OF_LOOP:
	/* buffer destroy */
	free(write_buf);
	free(read_buf);
	
	/* zlib structure destroy */
	ret = inflateEnd(&z);
	if(ret != Z_OK)
	{
		fprintf(stderr, "[Error] inflateEnd() is not Z_OK: %d\n", ret);
		exit_code = -3;
	}
	
	return exit_code;
} /* _decompress() */

/******************************************************************************
 * _compress function
 * 
 * argument value: [FILE*] ifp;            source file pointer
 *                 [FILE*] ofp;            destination file pointer
 *                 [int]   compress_level; coompress level
 *                         Z_NO_COMPRESSION    Z_BEST_SPEED
 *                         Z_BEST_COMPRESSION  Z_DEFAULT_COMPRESSION
 * retrun value:   [int]      ; exit code
 * description:    Compress with zlib deflate() function.
 ******************************************************************************/
int
_compress(ifp, ofp, compress_level)
	FILE     *ifp;                 /* input file pointer */
	FILE     *ofp;                 /* output file pointer */
	int       compress_level;      /* compress level */
{
	int       ret       = 0;           /* zlib retrun status value */
	int       exit_code = 0;           /* exit code on this function */
	int       loop_flag = 0;           /* loop flag */
	int       rbuf_size = 0;           /* read buffer size */
	int       wbuf_size = 0;           /* write buffer size */
	char     *read_buf  = NULL;        /* read buffer */
	char     *write_buf = NULL;        /* write buffer */
	z_stream  z;                       /* zlib stream */
	int       zflush    = Z_NO_FLUSH;  /* zlib deflate flush */
	
	/* zlib stream initialize */
	z.zalloc = Z_NULL;     /* allocator is default */
	z.zfree  = Z_NULL;     /* free is default */
	z.opaque = Z_NULL;     /* opaque is default */
	ret = deflateInit(&z, compress_level);
	if(ret != Z_OK)
	{
		fprintf(stderr, "[Error] deflateInit() is not Z_OK: %d\n", ret);
		exit_code = -1;
		goto END_OF_LOOP;
	}
	
	/* read buffer initialize */
	rbuf_size = MAXIMUM_BUFFER_SIZE * sizeof(char);
	read_buf  = (char*)malloc(rbuf_size);
	
	/* write buffer initialize */
	wbuf_size = MAXIMUM_BUFFER_SIZE * sizeof(char);
	write_buf = (char*)malloc(wbuf_size);
	
	/* loop initialize */
	loop_flag   = 1;               /* infinite loop */
	z.next_in   = read_buf;        /* read buffer pointer */
	z.avail_in  = 0;               /* read buffer size */
	z.next_out  = write_buf;       /* write buffer pointer */
	z.avail_out = wbuf_size;       /* write buffer size */
	
	/* main loop */
	while(loop_flag)
	{
		/* buffer set */
		if(z.avail_in == 0)
		{
			/* source file read */
			ret = fread(read_buf, sizeof(char), rbuf_size, ifp);
			if(ret < rbuf_size)
			{
				zflush = Z_FINISH;       /* end this loop */
			}
			
			/* zlib input stream set */
			z.next_in   = read_buf;        /* read buffer pointer */
			z.avail_in  = ret;             /* read buffer size */
		} /* if(z.avail_in == 0) */
		
		/* zlib inflate() */
		ret = deflate(&z, zflush);
		if(ret == Z_STREAM_END)
		{
			loop_flag = 0;             /* exit this loop */
		}
		
		if( !(                         /* if deflate is failed */
			(ret == Z_OK)              /* successfully continue loop */
			||
			(ret == Z_STREAM_END)      /* successfully end of loop */
		) )
		{
			fprintf(stderr, "[Error] deflate() is not Z_OK: %d (%s)\n", ret, z.msg);
			exit_code = -2;
			goto END_OF_LOOP;
		}
		
		/* buffer output in destination file */
		/*fprintf(stdout, "%s", write_buf);*/
		if(z.avail_out == 0)
		{
			/* destination file write */
			ret = fwrite(write_buf, sizeof(char), wbuf_size, ofp);
			if(ret != wbuf_size)
			{
				fprintf(stderr, "[Error] An error has occured at fwrite().\n");
				exit_code = -3;
				goto END_OF_LOOP;
			}
			
			/* zlib output stream set */
			z.next_out  = write_buf;       /* write buffer pointer */
			z.avail_out = wbuf_size;       /* write buffer size */
		} /* if(z.avail_out == 0) */
	} /* while(loop_flag) */
	
	/* write remain buffer in destination file */
	ret = fwrite(write_buf, sizeof(char), wbuf_size-z.avail_out, ofp);
	if(ret != wbuf_size-z.avail_out)
	{
		fprintf(stderr, "[Error] An error has occured at fwrite().\n");
		exit_code = -4;
		goto END_OF_LOOP;
	}
	
	/* zlib structure destroy */
	ret = deflateEnd(&z);
	if(ret != Z_OK)
	{
		fprintf(stderr, "[Error] deflateEnd() is not Z_OK: %d\n", ret);
		exit_code = -5;
	}
	
END_OF_LOOP:
	/* buffer destroy */
	free(write_buf);
	free(read_buf);
	
	return exit_code;
} /* _compress() */

/******************************************************************************
 * usage function
 *
 * description: display usage
 ******************************************************************************/
void usage(const char* argv_0)
{
	fprintf(stderr, "usage: %s [-cdhlv] [src] [dst]\n", argv_0);
	fprintf(stderr, "\t-c:       圧縮モード(デフォルト)\n");
	fprintf(stderr, "\t-d:       解凍モード\n");
	fprintf(stderr, "\t-l [0-3]: 圧縮レベル指定\n");
	fprintf(stderr, "\t       0: 無圧縮     (Z_NO_COMPRESSION)\n");
	fprintf(stderr, "\t       1: 速度優先   (Z_BEST_SPEED)\n");
	fprintf(stderr, "\t       2: 圧縮率優先 (Z_BEST_COMPRESSION)\n");
	fprintf(stderr, "\t       3: デフォルト (Z_DEFAULT_COMPRESSION)\n");
	fprintf(stderr, "\t-h:       ヘルプ表示\n");
	fprintf(stderr, "\t-v:       バージョン情報\n");
}

/******************************************************************************
 * version function
 *
 * description: display version info
 ******************************************************************************/
void version()
{
	fprintf(stderr, "Zlib Example Program: Version 0.0.1a\n");
	fprintf(stderr, "Zlib Version: %s\n", ZLIB_VERSION);
}

/******************************************************************************
 * main function
 * 
 * argument value: [int]    argc; number of argeument
 *                 [char**] argv; pointer to top of argument array.
 * retrun value:   [int]        ; exit code
 * description:    main function.
 ******************************************************************************/
int
main(argc, argv)
	int    argc;
	char **argv;
{
	int       ret            = 0;      /* return value */
	int       buf_size       = 0;      /* buffer size */
	int       program_mode   = 0;      /* is decompress? 
	                                    * (decompress=1, compress=0)
	                                    */
	int       compress_level = Z_DEFAULT_COMPRESSION;
	                                   /* compression level:
	                                    * Z_NO_COMPRESSION
	                                    * Z_BEST_SPEED
	                                    * Z_BEST_COMPRESSION
	                                    * Z_DEFAULT_COMPRESSION
	                                    */
	char     *src_file       = NULL;   /* input file name */
	char     *dst_file       = NULL;   /* output file name */
	FILE     *ifp            = NULL;   /* input file pointer */
	FILE     *ofp            = NULL;   /* output file pointer */
	
#ifdef __DEBUG__
	debug_info();
#endif /* __DEBUG__ */
	
	/* input options with getopt() */
	while((ret = getopt(argc, argv, "cdhl:v")) != -1)
	{
		switch(ret)
		{
		case 'c':      /* compression option */
			program_mode = PROGRAM_MODE_COMPRESS;
			break;
		case 'd':      /* decompression option */
			program_mode = PROGRAM_MODE_DECOMPRESS;
			break;
		case 'h':      /* help option */
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'l':      /* compression level option */
			compress_level = atoi(optarg);
			switch(compress_level)
			{
			case 0:
				compress_level = Z_NO_COMPRESSION;
				break;
			case 1:
				compress_level = Z_BEST_SPEED;
				break;
			case 2:
				compress_level = Z_BEST_COMPRESSION;
				break;
			case 3:
				compress_level = Z_DEFAULT_COMPRESSION;
				break;
			default:
				fprintf(stderr, "Unknown level: %s\n", optarg);
				exit(EXIT_FAILURE);
				break;
			} /* switch(compress_level) */
			break;
		case 'v':      /* vesion information option */
			version();
			exit(EXIT_SUCCESS);
			break;
		case '?':      /* unknown option */
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		} /* switch(ret) */
	} /* while((ret = getopt(argc, argv, "cdhl:v")) != -1) */
	
	/* input soruce and destination file name */
	if(optind+2 == argc)
	{
		buf_size = (strlen(argv[optind+0]) + 1) * sizeof(char);
		src_file = (char*)malloc(buf_size);
		memmove(src_file, argv[optind+0], buf_size);
		fprintf(stdout, "src: %s\n", argv[optind+0]);
		
		buf_size = (strlen(argv[optind+1]) + 1) * sizeof(char);
		dst_file = (char*)malloc(buf_size);
		memmove(dst_file, argv[optind+1], buf_size);
		fprintf(stdout, "dst: %s\n", argv[optind+1]);
	} /* if(optind+2 == argc) */
	else if(optind+0 == argc)
	{
		buf_size = MAXIMUM_BUFFER_SIZE * sizeof(char);
		
		src_file = (char*)malloc(buf_size);
		fprintf(stdout, "Input Source File Name:      ");
		get_string(src_file, buf_size / sizeof(char));
		
		dst_file = (char*)malloc(buf_size);
		fprintf(stdout, "Input Destination File Name: ");
		get_string(dst_file, buf_size / sizeof(char));
	} /* else if(optind+0 == argc) */
	else
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/* open soruce file */
	ifp = fopen(src_file, "rb");
	if(ifp == NULL)
	{
		fprintf(stderr, "[Error] cannot open source file: %s\n", src_file);
		goto END_OF_PROGRAM;
	}
	
	/* open destination file */
	ofp = fopen(dst_file, "wb+");
	if(ofp == NULL)
	{
		fprintf(stderr, "[Error] cannot open destination file: %s\n", dst_file);
		fclose(ifp);
		goto END_OF_PROGRAM;
	}
	
	/* decompress or compress */
	if(program_mode == PROGRAM_MODE_DECOMPRESS)
	{
		/* decompress (inflate) */
		ret = _decompress(ifp, ofp);
		if(ret != 0)
		{
			fprintf(stderr, "[Error] decompress error: %d\n", ret);
		}
	} /* if(program_mode == PROGRAM_MODE_DECOMPRESS) */
	else
	{
		/* compress (deflate) */
		ret = _compress(ifp, ofp, compress_level);
		if(ret != 0)
		{
			fprintf(stderr, "[Error] compress error: %d\n", ret);
		}
	} /* else */
	
	/* close destination file */
	fclose(ofp);
	
	/* close source file */
	fclose(ifp);
	
END_OF_PROGRAM:
	/* file name destroy */
	free(dst_file);
	free(src_file);
	
	return EXIT_SUCCESS;
} /* main() */
