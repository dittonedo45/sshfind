#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netdb.h>
#include <jansson.h>

static void dir(char *path, LIBSSH2_SFTP * ffp)
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
		if (att.permissions & LIBSSH2_SFTP_S_IFDIR
		    && strncmp("..", ent, strlen(ent)))
		    json_array_append(a, json_string(rp));
		puts(rp);
	    };
	} while (0);
	libssh2_sftp_closedir(fp);
    } while (0);
    {
	int i;
	json_t *e;
	json_array_foreach(a, i, e)
	    //
	{
	    dir(json_string_value(e), ffp);
	}
    }
}

int main(argsc, args, env)
int argsc;
char **args, **env;
{
    struct addrinfo h, *r, *p;
    char *host = 0;
    char *port = 0;
    char *user = 0;
    char *password = 0;

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
		dir("/", ffp);
		libssh2_sftp_shutdown(ffp);
	    } while (0);
	    libssh2_session_disconnect(session, 0);
	} while (0);
	libssh2_exit();
    } while (0);
    return 0;
}
