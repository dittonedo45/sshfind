#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netdb.h>
#include <jansson.h>
#include <getopt.h>

typedef
    enum {
    SF_NONE = 0,
    SF_TREE,
    SF_PROPER
} sf_PRINT_t;

struct sf_PRINT_st {
    char *name;
    sf_PRINT_t type;
} sf_main_types[] = {
    { "tree", SF_TREE },
    { "proper", SF_PROPER },
    { "none", SF_NONE },
    { NULL, -1 }
};

extern char *av_append_path_component(char *, char *);

static json_t *sf_dir(char *path, LIBSSH2_SFTP * ffp)
{
    json_t *a = json_array();
    do {
	LIBSSH2_SFTP_HANDLE *fp = libssh2_sftp_opendir(ffp, path);
	if (!fp)
	    break;
	do {
	    LIBSSH2_SFTP_ATTRIBUTES att;
	    char ent[1054];
	    while (1) {
		int t = libssh2_sftp_readdir(fp, ent, sizeof(ent),
					     &att);
		if (!t)
		    break;
		char *rp = av_append_path_component(path, ent);
		{
		    LIBSSH2_SFTP_STATVFS st;
		    libssh2_sftp_statvfs(ffp, rp, strlen(rp), &st);
		}
		json_t *b = json_object();
		if (att.permissions & LIBSSH2_SFTP_S_IFDIR
		    && strncmp("..", ent, strlen(ent))) {
		    json_object_set(b, "type_dir?", json_true());
		} else if (strncmp("..", ent, strlen(ent))) {
		    json_object_set(b, "type_dir?", json_false());
		}
		json_object_set(b, "file", json_string(rp));

		json_array_append(a, b);
	    };
	}
	while (0);
	libssh2_sftp_closedir(fp);
    } while (0);

    return a;
}

void sf_iter(char *path, LIBSSH2_SFTP * h, int level, int type_i)
{
    json_t *res = sf_dir(path, h);
    int index;
    json_t *element;

    json_array_foreach(res, index, element)
	// For Loop, of Results
    {
	json_t *type = json_object_get(element, "type_dir?");
	json_t *path = json_object_get(element, "file");
	const char *fpath = json_string_value(path);
	switch (type_i) {
	case SF_TREE:
	    {
		char *pp = strrchr(fpath, '/');
		if (pp && ++pp && strncmp("..", pp, strlen(pp))) {
		    for (int i = level; i; i--) {
			if (1 == i)
			    fputs("\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ", stdout);	// "├── "
			else
			    fputs("\xe2\x94\x82   ", stdout);	// "│   "
		    }
		    puts(pp);
		}
	    };
	    break;
	case SF_PROPER:
	default:
	    puts(fpath);
	    break;
	}
	if (json_boolean_value(type) == 1) {
	    sf_iter((char *) fpath, h, level + 1, type_i);
	}
    }
}


int main(argsc, args, env)
int argsc;
char **args, **env;
{
    struct addrinfo h, *r, *p;
    char *host = NULL;
    char *port = "22";
    char *user = NULL;
    char *path = ".";
    char *password = NULL;
    char *type = 0;

    struct option mylop[] = {
	{ "help", 0, 0, 'h' },
	{ "user", 1, 0, 'u' },
	{ "password", 1, 0, 'k' },
	{ "path", 1, 0, 'p' },
	{ "port", 1, 0, 'P' },
	{ "host", 1, 0, 'H' },
	{ "type", 1, 0, 't' },
	{.name = NULL }
    };

    int ret = 0;

    do {
	ret = getopt_long(argsc, args, "hu:p:P:H:k:t:", mylop, 0);
	if (ret == -1)
	    break;
	switch (ret) {
	case 'h':
	    fprintf(stderr, "%s <options> <arguments>?\n", *args);
	    fprintf(stderr,
		    "A tool for printing a the remote filesystem.\n");
	    {
		struct option *p = mylop;
		while (p && p->name) {
		    fprintf(stderr, "\toption --%s or -%c %s\n", p->name,
			    p->val, p->has_arg ? "<arg>" : "");
		}
		fprintf(stderr,
			"Program: Created by dittonedo45@gmail.com\n");
	    }
	    exit(1);		// In case -h was include by mistake.
	case 'H':
	    host = strdup(optarg);
	    break;
	case 'k':
	    password = strdup(optarg);
	    break;
	case 'p':
	    path = strdup(optarg);
	    break;
	case 'P':
	    port = strdup(optarg);
	    break;
	case 'u':
	    user = strdup(optarg);
	    break;
	case 't':
	    type = strdup(optarg);
	    break;
	};
    } while (1);
    if (!password || !user) {
	fprintf(stderr, "--user and --password are mandatory.\n");

	return 1;
    }
    int type_i = SF_PROPER;
    if (type) {
	struct sf_PRINT_st *p = sf_main_types;
	while (p && p->name) {
	    if (!strncmp(type, p->name, strlen(type))) {
		type_i = p->type;
		break;
	    }
	    p++;
	}
    }
    int sfd = -1;

    memset(&h, 0, sizeof(h));

    h.ai_socktype = SOCK_STREAM;
    h.ai_family = AF_UNSPEC;

    int rr = getaddrinfo(host, port, &h, &r);

    do {
	if (rr) {
	    puts(gai_strerror(rr));
	    break;
	}

	for (p = r; p; p = p->ai_next) {
	    int r = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	    sfd = r;
	    if (connect(sfd, p->ai_addr, p->ai_addrlen) != -1)
		break;
	}
	freeaddrinfo(r);
	if (sfd == -1)
	    break;
	libssh2_init(0);
	do {
	    LIBSSH2_SESSION *session = libssh2_session_init();
	    if (!session)
		break;
	    libssh2_session_startup(session, sfd);
	    do {
		int o = libssh2_userauth_password(session, user, password);
		if (o) {
		    fprintf(stderr, "%d\n", o);
		    break;
		}
		LIBSSH2_SFTP *ffp = libssh2_sftp_init(session);
		if (!ffp)
		    break;

		sf_iter(path, ffp, 2, type_i);
		libssh2_sftp_shutdown(ffp);
	    } while (0);
	    libssh2_session_disconnect(session, 0);
	} while (0);
	libssh2_exit();
    } while (0);
    return 0;
}
