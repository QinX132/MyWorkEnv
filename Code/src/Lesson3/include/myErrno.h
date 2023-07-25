#ifndef _MY_ERRNO_H_
#define _MY_ERRNO_H_

#define MY_SUCCESS              0
#define MY_EPERM                EPERM           /* Operation not permitted */
#define MY_ENOENT               ENOENT          /* No such file or directory */
#define MY_ESRCH                ESRCH           /* No such process */
#define MY_EINTR                EINTR           /* Interrupted system call */
#define MY_EIO                  EIO             /* I/O error */
#define MY_ENXIO                ENXIO           /* No such device or address */
#define MY_E2BIG                E2BIG           /* Argument list too long */
#define MY_ENOEXEC              ENOEXEC         /* Exec format error */
#define MY_EBADF                EBADF           /* Bad file number */
#define MY_ECHILD               ECHILD          /* No child processes */
#define MY_EAGAIN               EAGAIN          /* Try again */
#define MY_ENOMEM               ENOMEM          /* Out of memory */
#define MY_EACCES               EACCES          /* Permission denied */
#define MY_EFAULT               EFAULT          /* Bad address */
#define MY_ENOTBLK              ENOTBLK         /* Block device required */
#define MY_EBUSY                EBUSY           /* Device or resource busy */
#define MY_EEXIST               EEXIST          /* File exists */
#define MY_EXDEV                EXDEV           /* Cross-device link */
#define MY_ENODEV               ENODEV          /* No such device */
#define MY_ENOTDIR              ENOTDIR         /* Not a directory */
#define MY_EISDIR               EISDIR          /* Is a directory */
#define MY_EINVAL               EINVAL          /* Invalid argument */
#define MY_ENFILE               ENFILE          /* File table overflow */
#define MY_EMFILE               EMFILE          /* Too many open files */
#define MY_ENOTTY               ENOTTY          /* Not a typewriter */
#define MY_ETXTBSY              ETXTBSY         /* Text file busy */
#define MY_EFBIG                EFBIG           /* File too large */
#define MY_ENOSPC               ENOSPC          /* No space left on device */
#define MY_ESPIPE               ESPIPE          /* Illegal seek */
#define MY_EROFS                EROFS           /* Read-only file system */
#define MY_EMLINK               EMLINK          /* Too many links */
#define MY_EPIPE                EPIPE           /* Broken pipe */
#define MY_EDOM                 EDOM            /* Math argument out of domain of func */
#define MY_ERANGE               ERANGE          /* Math result not representable */
#define MY_EDEADLK              EDEADLK         /* Resource deadlock would occur */
#define MY_ENAMETOOLONG         ENAMETOOLONG    /* File name too long */
#define MY_ENOLCK               ENOLCK          /* No record locks available */
#define MY_ENOSYS               ENOSYS          /* Function not implemented */
#define MY_ENOTEMPTY            ENOTEMPTY       /* Directory not empty */
#define MY_ELOOP                ELOOP           /* Too many symbolic links encountered */
#define MY_EWOULDBLOCK          EWOULDBLOCK     /* Operation would block */
#define MY_ENOMSG               ENOMSG          /* No message of desired type */
#define MY_EIDRM                EIDRM           /* Identifier removed */
#define MY_ECHRNG               ECHRNG          /* Channel number out of range */
#define MY_EL2NSYNC             EL2NSYNC        /* Level 2 not synchronized */
#define MY_EL3HLT               EL3HLT          /* Level 3 halted */
#define MY_EL3RST               EL3RST          /* Level 3 reset */
#define MY_ELNRNG               ELNRNG          /* Link number out of range */
#define MY_EUNATCH              EUNATCH         /* Protocol driver not attached */
#define MY_ENOCSI               ENOCSI          /* No CSI structure available */
#define MY_EL2HLT               EL2HLT          /* Level 2 halted */
#define MY_EBADE                EBADE           /* Invalid exchange */
#define MY_EBADR                EBADR           /* Invalid request descriptor */
#define MY_EXFULL               EXFULL          /* Exchange full */
#define MY_ENOANO               ENOANO          /* No anode */
#define MY_EBADRQC              EBADRQC         /* Invalid request code */
#define MY_EBADSLT              EBADSLT         /* Invalid slot */
#define MY_EDEADLOCK            EDEADLOCK
#define MY_EBFONT               EBFONT          /* Bad font file format */
#define MY_ENOSTR               ENOSTR          /* Device not a stream */
#define MY_ENODATA              ENODATA         /* No data available */
#define MY_ETIME                ETIME           /* Timer expired */
#define MY_ENOSR                ENOSR           /* Out of streams resources */
#define MY_ENONET               ENONET          /* Machine is not on the network */
#define MY_ENOPKG               ENOPKG          /* Package not installed */
#define MY_EREMOTE              EREMOTE         /* Object is remote */
#define MY_ENOLINK              ENOLINK         /* Link has been severed */
#define MY_EADV                 EADV            /* Advertise error */
#define MY_ESRMNT               ESRMNT          /* Srmount error */
#define MY_ECOMM                ECOMM           /* Communication error on send */
#define MY_EPROTO               EPROTO          /* Protocol error */
#define MY_EMULTIHOP            EMULTIHOP       /* Multihop attempted */
#define MY_EDOTDOT              EDOTDOT         /* RFS specific error */
#define MY_EBADMSG              EBADMSG         /* Not a data message */
#define MY_EOVERFLOW            EOVERFLOW       /* Value too large for defined data type */
#define MY_ENOTUNIQ             ENOTUNIQ        /* Name not unique on network */
#define MY_EBADFD               EBADFD          /* File descriptor in bad state */
#define MY_EREMCHG              EREMCHG         /* Remote address changed */
#define MY_ELIBACC              ELIBACC         /* Can not access a needed shared library */
#define MY_ELIBBAD              ELIBBAD         /* Accessing a corrupted shared library */
#define MY_ELIBSCN              ELIBSCN         /* .lib section in a.out corrupted */
#define MY_ELIBMAX              ELIBMAX         /* Attempting to link in too many shared libraries */
#define MY_ELIBEXEC             ELIBEXEC        /* Cannot exec a shared library directly */
#define MY_EILSEQ               EILSEQ          /* Illegal byte sequence */
#define MY_ERESTART             ERESTART        /* Interrupted system call should be restarted */
#define MY_ESTRPIPE             ESTRPIPE        /* Streams pipe error */
#define MY_EUSERS               EUSERS          /* Too many users */
#define MY_ENOTSOCK             ENOTSOCK        /* Socket operation on non-socket */
#define MY_EDESTADDRREQ         EDESTADDRREQ    /* Destination address required */
#define MY_EMSGSIZE             EMSGSIZE        /* Message too long */
#define MY_EPROTOTYPE           EPROTOTYPE      /* Protocol wrong type for socket */
#define MY_ENOPROTOOPT          ENOPROTOOPT     /* Protocol not available */
#define MY_EPROTONOSUPPORT      EPROTONOSUPPORT /* Protocol not supported */
#define MY_ESOCKTNOSUPPORT      ESOCKTNOSUPPORT /* Socket type not supported */
#define MY_EOPNOTSUPP           EOPNOTSUPP      /* Operation not supported on transport endpoint */
#define MY_EPFNOSUPPORT         EPFNOSUPPORT    /* Protocol family not supported */
#define MY_EAFNOSUPPORT         EAFNOSUPPORT    /* Address family not supported by protocol */
#define MY_EADDRINUSE           EADDRINUSE      /* Address already in use */
#define MY_EADDRNOTAVAIL        EADDRNOTAVAIL   /* Cannot assign requested address */
#define MY_ENETDOWN             ENETDOWN        /* Network is down */
#define MY_ENETUNREACH          ENETUNREACH     /* Network is unreachable */
#define MY_ENETRESET            ENETRESET       /* Network dropped connection because of reset */
#define MY_ECONNABORTED         ECONNABORTED    /* Software caused connection abort */
#define MY_ECONNRESET           ECONNRESET      /* Connection reset by peer */
#define MY_ENOBUFS              ENOBUFS         /* No buffer space available */
#define MY_EISCONN              EISCONN         /* Transport endpoint is already connected */
#define MY_ENOTCONN             ENOTCONN        /* Transport endpoint is not connected */
#define MY_ESHUTDOWN            ESHUTDOWN       /* Cannot send after transport endpoint shutdown */
#define MY_ETOOMANYREFS         ETOOMANYREFS    /* Too many references: cannot splice */
#define MY_ETIMEDOUT            ETIMEDOUT       /* Connection timed out */
#define MY_ECONNREFUSED         ECONNREFUSED    /* Connection refused */
#define MY_EHOSTDOWN            EHOSTDOWN       /* Host is down */
#define MY_EHOSTUNREACH         EHOSTUNREACH    /* No route to host */
#define MY_EALREADY             EALREADY        /* Operation already in progress */
#define MY_EINPROGRESS          EINPROGRESS     /* Operation now in progress */
#define MY_ESTALE               ESTALE          /* Stale file handle */
#define MY_EUCLEAN              EUCLEAN         /* Structure needs cleaning */
#define MY_ENOTNAM              ENOTNAM         /* Not a XENIX named type file */
#define MY_ENAVAIL              ENAVAIL         /* No XENIX semaphores available */
#define MY_EISNAM               EISNAM          /* Is a named type file */
#define MY_EREMOTEIO            EREMOTEIO       /* Remote I/O error */
#define MY_EDQUOT               EDQUOT          /* Quota exceeded */
#define MY_ENOMEDIUM            ENOMEDIUM       /* No medium found */
#define MY_EMEDIUMTYPE          EMEDIUMTYPE     /* Wrong medium type */
#define MY_ECANCELED            ECANCELED       /* Operation Canceled */
#define MY_ENOKEY               ENOKEY          /* Required key not available */
#define MY_EKEYEXPIRED          EKEYEXPIRED     /* Key has expired */
#define MY_EKEYREVOKED          EKEYREVOKED     /* Key has been revoked */
#define MY_EKEYREJECTED         EKEYREJECTED    /* Key was rejected by service */
/* for  mutexes */
#define MY_EOWNERDEAD           EOWNERDEAD      /* Owner died */
#define MY_ENOTRECOVERABLE      ENOTRECOVERABLE /* State not recoverable */
#define MY_ERFKILL              ERFKILL         /* Operation not possible due to RF-kill */
#define MY_EHWPOISON            EHWPOISON       /* Memory page has hardware error */


#ifndef _MY_ERR_NO_ENUM_AND_STRING_
#define _MY_ERR_NO_ENUM_AND_STRING_

typedef enum _MY_ERR_NO_ENUM
{
    MY_ERR_NO_START             = 255,
    MY_ERR_EXIT_WITH_SUCCESS    = 256,
    MY_ERR_PEER_CLOSED          = 257,
    
    MY_ERR_NO_ENUM_MAX 
}
MY_ERR_NO_ENUM;

static const char* sg_myTestErrnoStr[MY_ERR_NO_ENUM_MAX - MY_ERR_NO_START] = 
{
    [MY_ERR_NO_START - MY_ERR_NO_START]             = "unused error: errno start",
    [MY_ERR_EXIT_WITH_SUCCESS - MY_ERR_NO_START]    = "exit with success: not an error",
    [MY_ERR_PEER_CLOSED - MY_ERR_NO_START]          = "peer closed"
};

static inline const char*
My_StrErr(
    int Errno
    )
{
    return Errno < MY_ERR_NO_START ? strerror(Errno) : 
            (Errno < MY_ERR_NO_ENUM_MAX ? sg_myTestErrnoStr[Errno - MY_ERR_NO_START] : "UnknownErr");
}
#endif /* _MY_ERR_NO_ENUM_AND_STRING_ */

#endif
