/* inflate.h -- internal inflate state definition
 * Copyright (C) 1995-2009 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer decoding by inflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip decoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GUNZIP
#endif

/* Possible inflate modes between inflate() calls */
typedef enum {
    HEAD        =0,       /* i: waiting for magic header */
    FLAGS       =1,      /* i: waiting for method and flags (gzip) */
    TIME        =2,       /* i: waiting for modification time (gzip) */
    OS          =3,         /* i: waiting for extra flags and operating system (gzip) */
    EXLEN       =4,      /* i: waiting for extra length (gzip) */
    EXTRA       =5,      /* i: waiting for extra bytes (gzip) */
    NAME        =6,       /* i: waiting for end of file name (gzip) */
    COMMENT     =7,    /* i: waiting for end of comment (gzip) */
    HCRC        =8,       /* i: waiting for header crc (gzip) */
    DICTID      =9,     /* i: waiting for dictionary check value */
    DICT        =10,       /* waiting for inflateSetDictionary() call */
        TYPE    =11,       /* i: waiting for type bits, including last-flag bit */
        TYPEDO  =12,     /* i: same, but skip check to exit inflate on new block */
        STORED  =13,     /* i: waiting for stored size (length and complement) */
        COPY_   =14,      /* i/o: same as COPY below, but only first time in */
        COPY    =15,       /* i/o: waiting for input or output to copy stored block */
        TABLE   =16,      /* i: waiting for dynamic block table lengths */
        LENLENS =17,    /* i: waiting for code length code lengths */
        CODELENS=18,   /* i: waiting for length/lit and distance code lengths */
            LEN_=19,       /* i: same as LEN below, but only first time in */
            LEN =20,        /* i: waiting for length/lit/eob code */
            LENEXT  =21,     /* i: waiting for length extra bits */
            DIST    =22,       /* i: waiting for distance code */
            DISTEXT =23,    /* i: waiting for distance extra bits */
            MATCH   =24,      /* o: waiting for output space to copy string */
            LIT     =25,        /* o: waiting for output space to write literal */
    CHECK           =26,      /* i: waiting for 32-bit check value */
    LENGTH          =27,     /* i: waiting for 32-bit length (gzip) */
    DONE            =28,       /* finished check, done -- remain here until reset */
    BAD             =29,        /* got a data error -- remain here until reset */
    MEM             =30,        /* got an inflate() memory error -- remain here until reset */
    SYNC            =31        /* looking for synchronization bytes to restart inflate() */
} inflate_mode;

/*
    State transitions between above modes -

    (most modes can go to BAD or MEM on error -- not shown for clarity)

    Process header:
        HEAD -> (gzip) or (zlib) or (raw)
        (gzip) -> FLAGS -> TIME -> OS -> EXLEN -> EXTRA -> NAME -> COMMENT ->
                  HCRC -> TYPE
        (zlib) -> DICTID or TYPE
        DICTID -> DICT -> TYPE
        (raw) -> TYPEDO
    Read deflate blocks:
            TYPE -> TYPEDO -> STORED or TABLE or LEN_ or CHECK
            STORED -> COPY_ -> COPY -> TYPE
            TABLE -> LENLENS -> CODELENS -> LEN_
            LEN_ -> LEN
    Read deflate codes in fixed or dynamic block:
                LEN -> LENEXT or LIT or TYPE
                LENEXT -> DIST -> DISTEXT -> MATCH -> LEN
                LIT -> LEN
    Process trailer:
        CHECK -> LENGTH -> DONE
 */

/* state maintained between inflate() calls.  Approximately 10K bytes. */
struct inflate_state {
    inflate_mode mode;          /* current inflate mode */
    int last;                   /* true if processing last block */
    int wrap;                   /* bit 0 true for zlib, bit 1 true for gzip */
    int havedict;               /* true if dictionary provided */
    int flags;                  /* gzip header method and flags (0 if zlib) */
    unsigned dmax;              /* zlib header max distance (INFLATE_STRICT) */
    unsigned long check;        /* protected copy of check value */
    unsigned long total;        /* protected copy of output count */
    gz_headerp head;            /* where to save gzip header information */
        /* sliding window */
    unsigned wbits;             /* log base 2 of requested window size */
    unsigned wsize;             /* window size or zero if not using window */
    unsigned whave;             /* valid bytes in the window */
    unsigned wnext;             /* window write index */
    unsigned char FAR *window;  /* allocated sliding window, if needed */
        /* bit accumulator */
    unsigned long hold;         /* input bit accumulator */
    unsigned bits;              /* number of bits in "in" */
        /* for string and stored block copying */
    unsigned length;            /* literal or length of data to copy */
    unsigned offset;            /* distance back to copy string from */
        /* for table and code decoding */
    unsigned extra;             /* extra bits needed */
        /* fixed and dynamic code tables */
    code const FAR *lencode;    /* starting table for length/literal codes */
    code const FAR *distcode;   /* starting table for distance codes */
    unsigned lenbits;           /* index bits for lencode */
    unsigned distbits;          /* index bits for distcode */
        /* dynamic table building */
    unsigned ncode;             /* number of code length code lengths */
    unsigned nlen;              /* number of length code lengths */
    unsigned ndist;             /* number of distance code lengths */
    unsigned have;              /* number of code lengths in lens[] */
    code FAR *next;             /* next available space in codes[] */
    unsigned short lens[320];   /* temporary storage for code lengths */
    unsigned short work[288];   /* work area for code table building */
    code codes[ENOUGH];         /* space for code tables */
    int sane;                   /* if false, allow invalid distance too far */
    int back;                   /* bits back of last unprocessed length/lit */
    unsigned was;               /* initial length of match */
};
