

/* hdlc_send.h */

int hdlc_send_frame (int chan, unsigned char *fbuf, int flen, int bad_fcs);

int hdlc_send_flags (int chan, int flags, int finish);

int hdlc_send_header (int chan, unsigned  char *fbuf, int flen);

/* end hdlc_send.h */

