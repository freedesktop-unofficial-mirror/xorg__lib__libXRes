/*
   Copyright (c) 2002  XFree86 Inc
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include <X11/extensions/XResproto.h>
#include <X11/extensions/XRes.h>
#include <assert.h>
#include <limits.h>

#ifndef HAVE__XEATDATAWORDS
static inline void _XEatDataWords(Display *dpy, unsigned long n)
{
# ifndef LONG64
    if (n >= (ULONG_MAX >> 2))
        _XIOError(dpy);
# endif
    _XEatData (dpy, n << 2);
}
#endif

static XExtensionInfo _xres_ext_info_data;
static XExtensionInfo *xres_ext_info = &_xres_ext_info_data;
static const char *xres_extension_name = XRES_NAME;

#define XResCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xres_extension_name, val)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xres_ext_info)

static XExtensionHooks xres_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    close_display,                      /* close_display */
    NULL,                               /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    NULL,                               /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xres_ext_info,
                                   xres_extension_name,
                                   &xres_extension_hooks,
                                   0, NULL)

Bool XResQueryExtension (
    Display *dpy,
    int *event_base_return,
    int *error_base_return
)
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
        *event_base_return = info->codes->first_event;
        *error_base_return = info->codes->first_error;
        return True;
    } else {
        return False;
    }
}

Status XResQueryVersion(
    Display *dpy,
    int *major_version_return,
    int *minor_version_return
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryVersionReply rep;
    xXResQueryVersionReq *req;

    XResCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XResQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryVersion;
    req->client_major = XRES_MAJOR_VERSION;
    req->client_minor = XRES_MINOR_VERSION;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return 0;
    }
    *major_version_return = rep.server_major;
    *minor_version_return = rep.server_minor;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}


Status XResQueryClients (
    Display *dpy,
    int *num_clients,
    XResClient **clients
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryClientsReq *req;
    xXResQueryClientsReply rep;
    XResClient *clnts;
    int result = 0;

    *num_clients = 0;
    *clients = NULL;

    XResCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XResQueryClients, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryClients;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return 0;
    }

    if(rep.num_clients) {
        if (rep.num_clients < (INT_MAX / sizeof(XResClient)))
            clnts = Xmalloc(sizeof(XResClient) * rep.num_clients);
        else
            clnts = NULL;

        if (clnts != NULL) {
            xXResClient scratch;
            int i;

            for(i = 0; i < rep.num_clients; i++) {
                _XRead(dpy, (char*)&scratch, sz_xXResClient);
                clnts[i].resource_base = scratch.resource_base;
                clnts[i].resource_mask = scratch.resource_mask;
            }
            *clients = clnts;
            *num_clients = rep.num_clients;
            result = 1;
        } else {
            _XEatDataWords(dpy, rep.length);
        }
    }

    UnlockDisplay (dpy);
    SyncHandle ();
    return result;
}

Status XResQueryClientResources (
    Display *dpy,
    XID xid,
    int *num_types,
    XResType **types
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryClientResourcesReq *req;
    xXResQueryClientResourcesReply rep;
    XResType *typs;
    int result = 0;

    *num_types = 0;
    *types = NULL;

    XResCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XResQueryClientResources, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryClientResources;
    req->xid = xid;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return 0;
    }

    if(rep.num_types) {
        if (rep.num_types < (INT_MAX / sizeof(XResType)))
            typs = Xmalloc(sizeof(XResType) * rep.num_types);
        else
            typs = NULL;

        if (typs != NULL) {
            xXResType scratch;
            int i;

            for(i = 0; i < rep.num_types; i++) {
                _XRead(dpy, (char*)&scratch, sz_xXResType);
                typs[i].resource_type = scratch.resource_type;
                typs[i].count = scratch.count;
            }
            *types = typs;
            *num_types = rep.num_types;
            result = 1;
        } else {
            _XEatDataWords(dpy, rep.length);
        }
    }

    UnlockDisplay (dpy);
    SyncHandle ();
    return result;
}

Status XResQueryClientPixmapBytes (
    Display *dpy,
    XID xid,
    unsigned long *bytes
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryClientPixmapBytesReq *req;
    xXResQueryClientPixmapBytesReply rep;

    *bytes = 0;

    XResCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XResQueryClientPixmapBytes, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryClientPixmapBytes;
    req->xid = xid;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return 0;
    }

#ifdef LONG64
    *bytes = (rep.bytes_overflow * 4294967295) + rep.bytes;
#else
    *bytes = rep.bytes_overflow ? 0xffffffff : rep.bytes;
#endif

    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}

static Bool ReadClientValues(
   Display              *dpy,
   long                 num_ids,
   XResClientIdValue   *client_ids /* out */
)
{
    int c;
    for (c = 0; c < num_ids; ++c) {
        XResClientIdValue* client = client_ids + c;
        long int value;
        _XRead32 (dpy, &value, 4);
        client->spec.client = value;
        _XRead32 (dpy, &value, 4);
        client->spec.mask = value;
        _XRead32 (dpy, &value, 4);
        client->length = value;
        client->value = malloc(client->length);
        _XRead32 (dpy, client->value, client->length);
    }
    return True;
}

Status XResQueryClientIds (
   Display            *dpy,
   long                num_specs,
   XResClientIdSpec   *client_specs,   /* in */
   long               *num_ids,        /* out */
   XResClientIdValue **client_ids      /* out */
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryClientIdsReq *req;
    xXResQueryClientIdsReply rep;
    int c;

    *num_ids = 0;

    XResCheckExtension (dpy, info, 0);
    LockDisplay (dpy);
    GetReq (XResQueryClientIds, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryClientIds;
    req->length += num_specs * 2; /* 2 longs per client id spec */
    req->numSpecs = num_specs;

    for (c = 0; c < num_specs; ++c) {
        Data32(dpy, &client_specs[c].client, 4);
        Data32(dpy, &client_specs[c].mask, 4);
    }

    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
        goto error;
    }

    *client_ids = calloc(rep.numIds, sizeof(**client_ids));
    *num_ids = rep.numIds;

    if (!ReadClientValues(dpy, *num_ids, *client_ids)) {
        goto error;
    }

    UnlockDisplay (dpy);
    SyncHandle ();
    return Success;

 error:
    XResClientIdsDestroy (*num_ids, *client_ids);
    *client_ids = NULL;

    UnlockDisplay (dpy);
    SyncHandle ();
    return !Success;
}

void XResClientIdsDestroy (
   long                num_ids,
   XResClientIdValue  *client_ids
)
{
    int c;
    for (c = 0; c < num_ids; ++c) {
        free(client_ids[c].value);
    }
    free(client_ids);
}

XResClientIdType XResGetClientIdType(
    XResClientIdValue* value
)
{
    int bit;
    XResClientIdType idType = 0;
    Bool found = False;
    for (bit = 0; bit < XRES_CLIENT_ID_NR; ++bit) {
        if (value->spec.mask & (1 << bit)) {
            assert(!found);
            found = True;
            idType = bit;
        }
    }

    assert(found);

    return idType;
}

pid_t XResGetClientPid(
    XResClientIdValue* value
)
{
    if (value->spec.mask & XRES_CLIENT_ID_PID_MASK && value->length >= 4) {
        return (pid_t) * (CARD32*) value->value;
    } else {
        return (pid_t) -1;
    }
}

static Status ReadResourceSizeSpec(
   Display               *dpy,
   XResResourceSizeSpec  *size
)
{
    long int value;
    _XRead32(dpy, &value, 4);
    size->spec.resource = value;
    _XRead32(dpy, &value, 4);
    size->spec.type = value;
    _XRead32(dpy, &value, 4);
    size->bytes = value;
    _XRead32(dpy, &value, 4);
    size->ref_count = value;
    _XRead32(dpy, &value, 4);
    size->use_count = value;
    return 0;
}

static Status ReadResourceSizeValues(
   Display                *dpy,
   long                    num_sizes,
   XResResourceSizeValue  *sizes)
{
    int c;
    int d;
    for (c = 0; c < num_sizes; ++c) {
        long int num;
        ReadResourceSizeSpec(dpy, &sizes[c].size);
        _XRead32(dpy, &num, 4);
        sizes[c].num_cross_references = num;
        sizes[c].cross_references = num ? calloc(num, sizeof(*sizes[c].cross_references)) : NULL;
        for (d = 0; d < num; ++d) {
            ReadResourceSizeSpec(dpy, &sizes[c].cross_references[d]);
        }
    }
    return Success;
}

Status XResQueryResourceBytes (
   Display            *dpy,
   XID                 client,
   long                num_specs,
   XResResourceIdSpec *resource_specs, /* in */
   long               *num_sizes, /* out */
   XResResourceSizeValue **sizes /* out */
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXResQueryResourceBytesReq *req;
    xXResQueryResourceBytesReply rep;
    int c;

    *num_sizes = 0;

    XResCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XResQueryResourceBytes, req);
    req->reqType = info->codes->major_opcode;
    req->XResReqType = X_XResQueryResourceBytes;
    req->length += num_specs * 2; /* 2 longs per client id spec */
    req->client = client;
    req->numSpecs = num_specs;

    for (c = 0; c < num_specs; ++c) {
        Data32(dpy, &resource_specs[c].resource, 4);
        Data32(dpy, &resource_specs[c].type, 4);
    }

    *num_sizes = 0;
    *sizes = NULL;

    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
        goto error;
    }

    *sizes = calloc(rep.numSizes, sizeof(**sizes));
    *num_sizes = rep.numSizes;

    if (ReadResourceSizeValues(dpy, *num_sizes, *sizes) != Success) {
        goto error;
    }

    UnlockDisplay (dpy);
    SyncHandle ();
    return Success;

 error:
    XResResourceSizeValuesDestroy(*num_sizes, *sizes);

    UnlockDisplay (dpy);
    SyncHandle ();
    return !Success;
}

void XResResourceSizeValuesDestroy (
   long                num_sizes,
   XResResourceSizeValue *sizes
)
{
    int c;
    for (c = 0; c < num_sizes; ++c) {
        free(sizes[c].cross_references);
    }
    free(sizes);
}
