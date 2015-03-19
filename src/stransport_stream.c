/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifdef GIT_SECURE_TRANSPORT

#include <CoreFoundation/CoreFoundation.h>
#include <Security/SecureTransport.h>
#include <Security/SecCertificate.h>

#include "git2/transport.h"

#include "socket_stream.h"

int stransport_error(OSStatus ret)
{
	switch (ret) {
	case noErr:
		giterr_clear();
		return 0;
	case errSSLXCertChainInvalid:
	case errSSLBadCert:
		return GIT_ECERTIFICATE;
	default:
		giterr_set(GITERR_NET, "SecureTransport error %d", ret);
		return -1;
	}
}

typedef struct {
	git_stream parent;
	git_stream *io;
	SSLContextRef ctx;
	CFDataRef der_data;
	git_cert_x509 cert_info;
} stransport_stream;

int stransport_connect(git_stream *stream)
{
	stransport_stream *st = (stransport_stream *) stream;
	int error;
	OSStatus ret;

	if ((error = git_stream_connect(st->io)) < 0)
		return error;

	if ((ret = SSLHandshake(st->ctx)) != noErr)
		return stransport_error(ret);

	return 0;
}

int stransport_certificate(git_cert **out, git_stream *stream)
{
	stransport_stream *st = (stransport_stream *) stream;
	SecTrustRef trust;
	SecCertificateRef sec_cert;
	SecTrustResultType sec_res;
	OSStatus ret;

	if ((ret = SSLCopyPeerTrust(st->ctx, &trust)) != noErr)
		return stransport_error(ret);

	if ((ret = SecTrustEvaluate(trust, &sec_res)) != noErr)
		return stransport_error(ret);

	sec_cert = SecTrustGetCertificateAtIndex(trust, 0);
	st->der_data = SecCertificateCopyData(sec_cert);
	CFRelease(trust);

	if (st->der_data == NULL) {
		giterr_set(GITERR_SSL, "retrieved invalid certificate data");
		return -1;
	}

	st->cert_info.cert_type = GIT_CERT_X509;
	st->cert_info.data = (void *) CFDataGetBytePtr(st->der_data);
	st->cert_info.len = CFDataGetLength(st->der_data);

	*out = (git_cert *)&st->cert_info;
	return 0;
}

static OSStatus write_cb(SSLConnectionRef conn, const void *data, size_t *len)
{
	git_stream *io = (git_stream *) conn;
	ssize_t ret;

	ret = git_stream_write(io, data, *len, 0);
	if (ret < 0) {
		*len = 0;
		return -1;
	}

	*len = ret;

	return noErr;
}

ssize_t stransport_write(git_stream *stream, const char *data, size_t len, int flags)
{
	stransport_stream *st = (stransport_stream *) stream;
	size_t data_len, processed;
	OSStatus ret;

	GIT_UNUSED(flags);

	data_len = len;
	if ((ret = SSLWrite(st->ctx, data, data_len, &processed)) != noErr)
		return stransport_error(ret);

	return processed;
}

static OSStatus read_cb(SSLConnectionRef conn, void *data, size_t *len)
{
	git_stream *io = (git_stream *) conn;
	ssize_t ret;
	size_t left, requested;

	requested = left = *len;
	do {
		ret = git_stream_read(io, data + (requested - left), left);
		if (ret < 0) {
			*len = 0;
			return -1;
		}

		left -= ret;
	} while (left);

	*len = requested;

	if (ret == 0)
		return errSSLClosedGraceful;

	return noErr;
}

ssize_t stransport_read(git_stream *stream, void *data, size_t len)
{
	stransport_stream *st = (stransport_stream *) stream;
	size_t processed;
	OSStatus ret;

	if ((ret = SSLRead(st->ctx, data, len, &processed)) != noErr)
		return stransport_error(ret);

	return processed;
}

int stransport_close(git_stream *stream)
{
	stransport_stream *st = (stransport_stream *) stream;
	OSStatus ret;

	if ((ret = SSLClose(st->ctx)) != noErr)
		return stransport_error(ret);

	return git_stream_close(st->io);
}

void stransport_free(git_stream *stream)
{
	stransport_stream *st = (stransport_stream *) stream;

	git_stream_free(st->io);
	CFRelease(st->ctx);
	if (st->der_data)
		CFRelease(st->der_data);
	git__free(st);
}

int git_stransport_stream_new(git_stream **out, const char *host, const char *port)
{
	stransport_stream *st;
	int error;
	OSStatus ret;

	assert(out && host);

	st = git__calloc(1, sizeof(stransport_stream));
	GITERR_CHECK_ALLOC(st);

	if ((error = git_socket_stream_new(&st->io, host, port)) < 0){
		git__free(st);
		return error;
	}

	st->ctx = SSLCreateContext(NULL, kSSLClientSide, kSSLStreamType);
	if (!st->ctx) {
		giterr_set(GITERR_NET, "failed to create SSL context");
		return -1;
	}

	if ((ret = SSLSetIOFuncs(st->ctx, read_cb, write_cb)) != noErr ||
	    (ret = SSLSetConnection(st->ctx, st->io)) != noErr ||
	    (ret = SSLSetPeerDomainName(st->ctx, host, strlen(host))) != noErr) {
		git_stream_free((git_stream *)st);
		return stransport_error(ret);
	}

	st->parent.version = GIT_STREAM_VERSION;
	st->parent.encrypted = 1;
	st->parent.connect = stransport_connect;
	st->parent.certificate = stransport_certificate;
	st->parent.read = stransport_read;
	st->parent.write = stransport_write;
	st->parent.close = stransport_close;
	st->parent.free = stransport_free;

	*out = (git_stream *) st;
	return 0;
}

#endif
