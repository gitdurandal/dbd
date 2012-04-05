/*
 * Packet Encryption Layer for Tiny SHell,
 * by Christophe Devine <devine@cr0.net>;
 * this program is licensed under the GPL.
 *
 * Shamelessly ripped by Kyle Barnthouse <durandal@gitbrew.org> for dbd, April 2012
 * Originally Ported to Win32 by Michel Blomgren <michel.blomgren@tigerteam.se> for sbd, June 2004
 *
 * WARNING!!!  The receiving buffer can _NOT_ be less than BUFSIZE (defined in
 * pel.h) otherwise a buffer overflow will occur! //Michel
 */

#ifdef WIN32
    #define __USE_WIN32_SOCKETS
    #include <winsock.h>
    #include <windows.h>
    #include <string.h>
    #include <process.h>
    #include <stdio.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <string.h>
#endif

#include "pel.h"
#include "aes.h"
#include "sha1.h"

/* global data */

int pel_errno;

struct pel_context
{
    /* AES-CBC-128 variables */

    aes_context SK;      /* Rijndael session key  */
    unsigned char LCT[16];      /* last ciphertext block */

    /* HMAC-SHA1 variables */

    unsigned char k_ipad[64];   /* inner padding  */
    unsigned char k_opad[64];   /* outer padding  */
    unsigned long int p_cntr;   /* packet counter */
};

struct pel_context send_ctx;    /* to encrypt outgoing data */
struct pel_context recv_ctx;    /* to decrypt incoming data */

unsigned char challenge[16] =   /* version-specific */
    "\x90\x6a\x9a\xee\x57\x03\x2f\xa4" \
    "\x5e\xab\x07\xdf\xc4\x76\xf6\xf0";
/* above: challenge for sbd */
/*
    "\x6c\xd0\x47\x2c\x6d\x4b\x68\x20" \
    "\xfc\x7d\x78\x91\xed\xe7\xa5\x44";
*/
/* (above: challenge for rrs, below: challenge from tsh)
    "\x58\x90\xAE\x86\xF1\xB9\x1C\xF6" \
    "\x29\x83\x95\x71\x1D\xDE\x58\x0D";
*/

unsigned char buffer[BUFSIZE + 16 + 20];

/* function declaration */

void pel_setup_context( struct pel_context *pel_ctx,
                        char *key, unsigned char IV[20] );

#ifdef WIN32
    int pel_send_all( SOCKET s, void *buf, size_t len, int flags );
    int pel_recv_all( SOCKET s, void *buf, size_t len, int flags );
#else
    int pel_send_all( int s, void *buf, size_t len, int flags );
    int pel_recv_all( int s, void *buf, size_t len, int flags );
#endif


/* session setup - client side */

#ifdef WIN32
    int pel_client_init( SOCKET server, char *key )
#else
    int pel_client_init( int server, char *key )
#endif
{
    int ret, len, pid;

    #ifdef WIN32
        SYSTEMTIME lpstm;
    #else
        struct timeval tv;
    #endif

    struct sha1_context sha1_ctx;
    unsigned char IV1[20], IV2[20];

    /* generate both initialization vectors */

    pid = getpid();

    #ifdef WIN32
        GetSystemTime(&lpstm);
    #else
        if( gettimeofday( &tv, NULL ) < 0 ) {
            pel_errno = PEL_SYSTEM_ERROR;
            return( PEL_FAILURE );
        }
    #endif

    sha1_starts( &sha1_ctx );
    #ifdef WIN32
        sha1_update( &sha1_ctx, (uint8 *) &lpstm,  sizeof( lpstm ) );
    #else
        sha1_update( &sha1_ctx, (uint8 *) &tv,  sizeof( tv  ) );
    #endif
    sha1_update( &sha1_ctx, (uint8 *) &pid, sizeof( pid ) );
    sha1_finish( &sha1_ctx, &buffer[ 0] );

    memcpy( IV1, &buffer[ 0], 20 );

    pid++;

    #ifdef WIN32
        /* Win32 has only millisec resolution, but we call it again anyway */
        GetSystemTime(&lpstm);
    #else
        if( gettimeofday( &tv, NULL ) < 0 )
        {
            pel_errno = PEL_SYSTEM_ERROR;

            return( PEL_FAILURE );
        }
    #endif

    sha1_starts( &sha1_ctx );
    #ifdef WIN32
        sha1_update( &sha1_ctx, (uint8 *) &lpstm,  sizeof( lpstm ) );
    #else
        sha1_update( &sha1_ctx, (uint8 *) &tv,  sizeof( tv  ) );
    #endif
    sha1_update( &sha1_ctx, (uint8 *) &pid, sizeof( pid ) );
    sha1_finish( &sha1_ctx, &buffer[20] );

    memcpy( IV2, &buffer[20], 20 );

    /* and pass them to the server */

    ret = pel_send_all( server, buffer, 40, 0 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    /* setup the session keys */

    pel_setup_context( &send_ctx, key, IV1 );
    pel_setup_context( &recv_ctx, key, IV2 );

    /* handshake - encrypt and send the client's challenge */

    ret = pel_send_msg( server, challenge, 16 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    /* handshake - decrypt and verify the server's challenge */

    len = sizeof(buffer);
    ret = pel_recv_msg( server, buffer, &len );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    if( len != 16 || memcmp( buffer, challenge, 16 ) != 0 )
    {
        pel_errno = PEL_WRONG_CHALLENGE;

        return( PEL_FAILURE );
    }

    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
} 

/* session setup - server side */

#ifdef WIN32
    int pel_server_init( SOCKET client, char *key )
#else
    int pel_server_init( int client, char *key )
#endif
{
    int ret, len;
    unsigned char IV1[20], IV2[20];

    /* get the IVs from the client */

    ret = pel_recv_all( client, buffer, 40, 0 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    memcpy( IV2, &buffer[ 0], 20 );
    memcpy( IV1, &buffer[20], 20 );

    /* setup the session keys */

    pel_setup_context( &send_ctx, key, IV1 );
    pel_setup_context( &recv_ctx, key, IV2 );

    /* handshake - decrypt and verify the client's challenge */

    len = sizeof(buffer);
    ret = pel_recv_msg( client, buffer, &len );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    if( len != 16 || memcmp( buffer, challenge, 16 ) != 0 )
    {
        pel_errno = PEL_WRONG_CHALLENGE;

        return( PEL_FAILURE );
    }

    /* handshake - encrypt and send the server's challenge */

    ret = pel_send_msg( client, challenge, 16 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
}

/* this routine computes the AES & HMAC session keys */

void pel_setup_context( struct pel_context *pel_ctx,
                        char *key, unsigned char IV[20] )
{
    int i;
    struct sha1_context sha1_ctx;

    sha1_starts( &sha1_ctx );
    sha1_update( &sha1_ctx, (uint8 *) key, strlen( key ) );
    sha1_update( &sha1_ctx, IV, 20 );
    sha1_finish( &sha1_ctx, buffer );

    aes_set_key( &pel_ctx->SK, buffer, 128 );

    memcpy( pel_ctx->LCT, IV, 16 );

    memset( pel_ctx->k_ipad, 0x36, 64 );
    memset( pel_ctx->k_opad, 0x5C, 64 );

    for( i = 0; i < 20; i++ )
    {
        pel_ctx->k_ipad[i] ^= buffer[i];
        pel_ctx->k_opad[i] ^= buffer[i];
    }

    pel_ctx->p_cntr = 0;
}

/* encrypt and transmit a message */

#ifdef WIN32
    int pel_send_msg( SOCKET sockfd, unsigned char *msg, int length )
#else
    int pel_send_msg( int sockfd, unsigned char *msg, int length )
#endif
{
    unsigned char digest[20];
    struct sha1_context sha1_ctx;
    int i, j, ret, blk_len;

    /* verify the message length */

    if( length <= 0 || length > BUFSIZE )
    {
        pel_errno = PEL_BAD_MSG_LENGTH;

        return( PEL_FAILURE );
    }

    /* write the message length at start of buffer */

    buffer[0] = ( length >> 8 ) & 0xFF;
    buffer[1] = ( length      ) & 0xFF;

    /* append the message content */

    memcpy( buffer + 2, msg, length );

    /* round up to AES block length (16 bytes) */

    blk_len = 2 + length;

    if( ( blk_len & 0x0F ) != 0 )
    {
        blk_len += 16 - ( blk_len & 0x0F );
    }

    /* encrypt the buffer with AES-CBC-128 */

    for( i = 0; i < blk_len; i += 16 )
    {
        for( j = 0; j < 16; j++ )
        {
            buffer[i + j] ^= send_ctx.LCT[j];
        }

        aes_encrypt( &send_ctx.SK, &buffer[i], &buffer[i] );
        memcpy( send_ctx.LCT, &buffer[i], 16 );
    }

    /* compute the HMAC-SHA1 of the ciphertext */

    buffer[blk_len    ] = ( send_ctx.p_cntr << 24 ) & 0xFF;
    buffer[blk_len + 1] = ( send_ctx.p_cntr << 16 ) & 0xFF;
    buffer[blk_len + 2] = ( send_ctx.p_cntr <<  8 ) & 0xFF;
    buffer[blk_len + 3] = ( send_ctx.p_cntr       ) & 0xFF;

    sha1_starts( &sha1_ctx );
    sha1_update( &sha1_ctx, send_ctx.k_ipad, 64 );
    sha1_update( &sha1_ctx, buffer, blk_len + 4 );
    sha1_finish( &sha1_ctx, digest );

    sha1_starts( &sha1_ctx );
    sha1_update( &sha1_ctx, send_ctx.k_opad, 64 );
    sha1_update( &sha1_ctx, digest, 20 );
    sha1_finish( &sha1_ctx, &buffer[blk_len] );

    /* increment the packet counter */

    send_ctx.p_cntr++;

    /* transmit ciphertext and message authentication code */

    ret = pel_send_all( sockfd, buffer, blk_len + 20, 0 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
}

/* receive and decrypt a message */

#ifdef WIN32
    int pel_recv_msg( SOCKET sockfd, unsigned char *msg, int *length )
#else
    int pel_recv_msg( int sockfd, unsigned char *msg, int *length )
#endif
{
    unsigned char temp[16];
    unsigned char hmac[20];
    unsigned char digest[20];
    struct sha1_context sha1_ctx;
    int i, j, ret, blk_len;

    /*
        first, check that length isn't below BUFSIZE, otherwise we're in
        trouble! this prevents a shameful possible buffer overflow!

        WARNING!! you *have to* initialize "length" before passing it to this
        function! e.g.:
            int len = sizeof(mybuf);
            pel_recv_msg(socket, mybuf, &len);

        (modification by Michel Blomgren <michel.blomgren@tigerteam.se>)
    */
    if (*length < BUFSIZE) {
        pel_errno = PEL_BAD_MSG_LENGTH;
        return (PEL_FAILURE);
    }



    /* receive the first encrypted block */

    ret = pel_recv_all( sockfd, buffer, 16, 0 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    /* decrypt this block and extract the message length */

    memcpy( temp, buffer, 16 );
    aes_decrypt( &recv_ctx.SK, buffer, buffer);

    for( j = 0; j < 16; j++ )
    {
        buffer[j] ^= recv_ctx.LCT[j];
    }

    *length = ( ((int) buffer[0]) << 8 ) + (int) buffer[1];

    /* restore the ciphertext */

    memcpy( buffer, temp, 16 );

    /* verify the message length */

    if( *length <= 0 || *length > BUFSIZE )
    {
        pel_errno = PEL_BAD_MSG_LENGTH;

        return( PEL_FAILURE );
    }

    /* round up to AES block length (16 bytes) */

    blk_len = 2 + *length;

    if( ( blk_len & 0x0F ) != 0 )
    {
        blk_len += 16 - ( blk_len & 0x0F );
    }

    /* receive the remaining ciphertext and the mac */

    ret = pel_recv_all( sockfd, &buffer[16], blk_len - 16 + 20, 0 );

    if( ret != PEL_SUCCESS ) return( PEL_FAILURE );

    memcpy( hmac, &buffer[blk_len], 20 );

    /* verify the ciphertext integrity */

    buffer[blk_len    ] = ( recv_ctx.p_cntr << 24 ) & 0xFF;
    buffer[blk_len + 1] = ( recv_ctx.p_cntr << 16 ) & 0xFF;
    buffer[blk_len + 2] = ( recv_ctx.p_cntr <<  8 ) & 0xFF;
    buffer[blk_len + 3] = ( recv_ctx.p_cntr       ) & 0xFF;

    sha1_starts( &sha1_ctx );
    sha1_update( &sha1_ctx, recv_ctx.k_ipad, 64 );
    sha1_update( &sha1_ctx, buffer, blk_len + 4 );
    sha1_finish( &sha1_ctx, digest );

    sha1_starts( &sha1_ctx );
    sha1_update( &sha1_ctx, recv_ctx.k_opad, 64 );
    sha1_update( &sha1_ctx, digest, 20 );
    sha1_finish( &sha1_ctx, digest );

    if( memcmp( hmac, digest, 20 ) != 0 )
    {
        pel_errno = PEL_CORRUPTED_DATA;

        return( PEL_FAILURE );
    }

    /* increment the packet counter */

    recv_ctx.p_cntr++;

    /* finally, decrypt and copy the message */

    for( i = 0; i < blk_len; i += 16 )
    {
        memcpy( temp, &buffer[i], 16 );
        aes_decrypt( &recv_ctx.SK, &buffer[i], &buffer[i] );

        for( j = 0; j < 16; j++ )
        {
            buffer[i + j] ^= recv_ctx.LCT[j];
        }

        memcpy( recv_ctx.LCT, temp, 16 );
    }

    memcpy( msg, &buffer[2], *length );

    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
}

/* send/recv wrappers to handle fragmented TCP packets */

#ifdef WIN32
    int pel_send_all( SOCKET s, void *buf, size_t len, int flags )
#else
    int pel_send_all( int s, void *buf, size_t len, int flags )
#endif
{
    int n;
    size_t sum = 0;
    char *offset = buf;

    while( sum < len )
    {
        n = send( s, (void *) offset, len - sum, flags );

        #ifdef WIN32
            if (n == SOCKET_ERROR) {
                pel_errno = PEL_SYSTEM_ERROR;
                return(PEL_FAILURE);
            }
        #else
            if( n < 0 )
            {
                pel_errno = PEL_SYSTEM_ERROR;

                return( PEL_FAILURE );
            }
        #endif

        sum += n;

        offset += n;
    }

    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
}

#ifdef WIN32
    int pel_recv_all( SOCKET s, void *buf, size_t len, int flags )
#else
    int pel_recv_all( int s, void *buf, size_t len, int flags )
#endif
{
    int n;
    size_t sum = 0;
    char *offset = buf;

    while( sum < len )
    {
        n = recv( s, (void *) offset, len - sum, flags );

        if( n == 0 )
        {
            pel_errno = PEL_CONN_CLOSED;
            return( PEL_FAILURE );
        }

        #ifdef WIN32
            if (n == SOCKET_ERROR) {
                pel_errno = PEL_SYSTEM_ERROR;
                return(PEL_FAILURE);
            }
        #else
            if( n < 0 )
            {
                pel_errno = PEL_SYSTEM_ERROR;

                return( PEL_FAILURE );
            }
        #endif

        sum += n;

        offset += n;
    }
        
    pel_errno = PEL_UNDEFINED_ERROR;

    return( PEL_SUCCESS );
}
